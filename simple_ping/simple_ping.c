#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>

#if 0
struct icmp{
    u_int8_t icmp_type; /* 消息类型 */
    u_int8_t icmp_code; /* 消息类型的子码 */
    u_int16_t icmp_cksum;   /* 校验和 */
    union{
        struct ih_idseq{        /* 显示数据报 */
            u_int16_t icd_id;   /* 数据报ID */
            u_int16_t icd_seq;  /* 数据报的序号 */
        }ih_idseq;
    }icmp_hun;
#define icmp_id     icmp_hun.in_idseq.icd_id
#define icmp_seq    icmp_hun.ih_idseq.icd_seq;
    union{
        u_int8_t    id_data[1]; /* 数据 */
    }icmp_dun;
#define icmp_data   icmp_dun.id_data;
};
#endif 

typedef struct pingm_packet{
    struct timeval tv_begin;    /* 发送的时间 */
    struct timeval tv_end;      /* 接收到的时间 */
    short seq;                  /* 序列号 */
    int flag;                   /* 1.表示已经发送但没有接收到的回应包， 0表示接收到的回应包 */
}pingm_packet;
static pingm_packet pingpacket[128];

static pingm_packet *icmp_findpacket(int seq);
static unsigned short icmp_cksum(unsigned char *data, int len);
static struct timeval icmp_tvsub(struct timeval end, struct timeval begin);
static void icmp_statistics();
static void icmp_pack(struct icmp *icmph, int seq, struct timeval *tv, int length);
static int icmp_unpack(char *buf, int len);
static void *icmp_recv(void *argv);
static void *icmp_send(void *argv);
static void icmp_sigint(int signo);
static void icmp_usage();

#define     K   1024
#define     BUFFERSIZE  72  /* 发送缓冲区大小 */
static unsigned char send_buff[BUFFERSIZE];
static unsigned char recv_buff[2*K];    /* 为了防止接收溢出，接收缓冲区设置大一些 */
static struct sockaddr_in dest;     /* 目的地址 */
static int rawsock = 0;     /* 发送和接收线程需要的socket描述符 */
static pid_t pid = 0;       /* 进程PID */
static int alive = 0;       /* 是否收到退出信号 */
static short packet_send = 0;/* 已经发送的数据包有多少 */
static short packet_recv = 0;/* 已经接收的数据包有多少 */
static char dest_str[80];       /* 目标主机字符串 */
static struct timeval tv_begin, tv_end, tv_interval; /* 本程序开始发送，结束和时间间隔 */

static void icmp_usage(){
    /* simple_ping 加IP地址或者域名*/
    printf("simple_ping aaa.bbb.ccc.ddd\n");
}

int main(int argc, char *argv[]){
    struct hostent *host = NULL;
    struct protoent *protocol = NULL;
    char protoname[] = "icmp";
    unsigned long inaddr = 1;
    int size = 128*K;

    if(argc < 2){
        icmp_usage();
        return -1;
    }

    protocol = getprotobyname(protoname);
    if(NULL == protocol){
        perror("getprotobyname()");
        return -1;
    }

    memcpy(dest_str, argv[1], strlen(argv[1])+1);
    memset(pingpacket, 0, sizeof(pingm_packet)*128);

    rawsock = socket(AF_INET, SOCK_RAW, protocol->p_proto);
    if(rawsock < 0){
        perror("socket");
        return -1;
    }

    pid = getuid(); /* 为了与其他进程的ping程序区别，加入pid */
    setsockopt(rawsock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;  
    inaddr = inet_addr(argv[1]);
    if(inaddr == INADDR_NONE){
        host = gethostbyname(argv[1]);
        if(NULL == host){
            perror("gethostbyname");
            return -1;
        }
        memcpy((char*)&dest.sin_addr, host->h_addr, host->h_length);
    }else{
        memcpy((char*)&dest.sin_addr, &inaddr, sizeof(inaddr));
    }

    inaddr = dest.sin_addr.s_addr;
    printf("PING %s (%d.%d.%d.%d) 56(84) bytes of data.\n",
            dest_str,
            (inaddr&0x000000FF)>>0,
            (inaddr&0x0000FF00)>>8,
            (inaddr&0x00FF0000)>>16,
            (inaddr&0xFF000000)>>24);
    signal(SIGINT, icmp_sigint);

    alive = 1;
    pthread_t send_id, recv_id;
    int err = 0;
    err = pthread_create(&send_id, NULL, icmp_send, NULL);
    if(err < 0){
        return -1;
    }
    err = pthread_create(&recv_id, NULL, icmp_recv, NULL);
    if(err < 0){
        return -1;
    }

    pthread_join(send_id, NULL);
    pthread_join(recv_id, NULL);
    close(rawsock);
    icmp_statistics();
    return 0;
}

/*CRC16 校验和计算 icmp_cksum
参数:
    data:数据
    len:数据长度
返回值:
    计算结果，short类型
*/
static unsigned short icmp_cksum(unsigned char *data, int len){
    int sum = 0;
    int odd = len & 0x01;
    /* 将数据按照2字节为单位累加起来 */
    while( len & 0xfffe){
        sum += *(unsigned short*)data;
        data += 2;
        len -=2;
    }
    /* 判断是否为奇数个数据，若ICMP报文头为奇数个字节，会剩下最后一个字节 */
    if(odd){
        unsigned short tmp = ((*data)<<8)&0xff00;
        sum += tmp;
    }
    sum = (sum>>16) + (sum & 0xffff); /* 高低位相加 */
    sum += (sum >> 16); /* 将溢出位加入 */

    return ~sum;    /* 返回值反值 */
}

/* 设置ICMP报文头 */
static void icmp_pack(struct icmp *icmph, int seq, struct timeval *tv, int length){
    unsigned char i = 0;
    /* 设置报头 */
    icmph->icmp_type = ICMP_ECHO;   /* ICMP回显请求 */
    icmph->icmp_code = 0;       /* code值为0 */
    icmph->icmp_cksum = 0;  /* 先讲cksum填写0，便于之后的cksum计算 */
    icmph->icmp_seq = seq;
    icmph->icmp_id = pid & 0xffff;  /* 填写PID */
    for(i=0;i<length;++i){
        icmph->icmp_data[i] = i;
    }
    /* 计算校验和 */
    icmph->icmp_cksum = icmp_cksum((unsigned char*)icmph, length);
}

/* 解压接收到的包，并打印信息 */
static int icmp_unpack(char *buf, int len){
    int i, iphdrlen;
    struct ip *ip = NULL;
    struct icmp *icmp = NULL;
    int rtt;

    ip = (struct ip*)buf;       /* IP头部 */
    iphdrlen = ip->ip_hl*4;     /* IP头部长度 */
    icmp = (struct icmp*)(buf+iphdrlen);    /* ICMP段的地址 */
    len -=iphdrlen;         /* 判断长度是否为ICMP包 */

    if(len < 8){
        printf("ICMP packet \'s length is less than 8\n");
        return -1;
    }
    /* ICMP类型为ICMP_ECHOREPLY并且为本进程的PID */
    if((icmp->icmp_type == ICMP_ECHOREPLY)&&(icmp->icmp_id == pid)){
        struct timeval tv_internel, tv_recv, tv_send;
        /* 在发送表格中查找已经发送的包，按照seq */
        pingm_packet *packet = icmp_findpacket(icmp->icmp_seq);
        if(NULL == packet)
            return -1;

        packet->flag = 0;   /* 取消标志 */
        tv_send = packet->tv_begin; /* 获取本包的发送时间 */

        gettimeofday(&tv_recv, NULL);   /* 读取此时间，计算时间差 */
        tv_internel = icmp_tvsub(tv_recv, tv_send);
        rtt = tv_internel.tv_sec*1000 + tv_internel.tv_usec/1000;
        /*打印结果，包含
         *ICMP段长度
         *源IP地址
         *包的序列号
         *TTL
         *时间差
         */
        printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%d ms\n",
                len,
                inet_ntoa(ip->ip_src),
                icmp->icmp_seq,
                ip->ip_ttl,
                rtt);
        packet_recv++;  /* 接收包数量增加 */
    }else{
        return -1;
    }
}

/*计算时间差time_sub
参数:
    end,接收到的时间
    begin,开始发送的时间
返回值:
    使用的时间
*/
static struct timeval icmp_tvsub(struct timeval end, struct timeval begin){
    struct timeval tv;
    /* 计算差值 */
    tv.tv_sec = end.tv_sec - begin.tv_sec;
    tv.tv_usec = end.tv_usec - begin.tv_usec;
    /* 如果接收时间的usec值小于发送时的usec值，从usec域错位 */
    if(tv.tv_usec<0){
        tv.tv_sec--;
        tv.tv_usec +=1000000;
    }
    return tv;
}

/* 发送ICMP回显请求包 */
static void *icmp_send(void *argv){
    /* 保存程序开始发送数据的时间 */
    gettimeofday(&tv_begin, NULL);
    while(alive){
        int size = 0;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        /* 在发送包状态数组中找到一个空闲位置 */
        pingm_packet *packet = icmp_findpacket(-1);
        if(packet){
            packet->seq = packet_send;  /* 设置seq */
            packet->flag = 1;           /* 已经使用 */
            gettimeofday(&packet->tv_begin, NULL);  /* 发送时间 */

        }
        icmp_pack((struct icmp*)send_buff, packet_send, &tv, 64);   /* 打包数据 */
        size = sendto(rawsock, send_buff, 64, 0, (struct sockaddr*)&dest, sizeof(dest));    /* 发送给目的地址 */
        if(size < 0){
            perror("sendto error");
            continue;
        }
        packet_send++;  /* 计数增加 */
        /* 每隔1s, 发送一个ICMP回显请求包 */
        sleep(1);
    }
}

/* 接收ping目的主机的回复 */
static void *icmp_recv(void *argv){
    /* 轮询等待时间 */
    struct timeval tv;
    tv.tv_usec = 200;
    tv.tv_sec = 0;
    fd_set readfd;
    /* 当没有信号发出一直接收数据 */
    while(alive){
        int ret = 0;
        FD_ZERO(&readfd);
        FD_SET(rawsock, &readfd);
        ret = select(rawsock+1, &readfd, NULL, NULL, &tv);
        switch(ret){
        case -1:
            /* 错误发生 */
            break;
        case 0:
            /* 超时 */
            break;
        default:
            {
                /* 收到一个包 */
                int fromlen = 0;
                struct sockaddr from;
                /* 接收数据 */
                int size = recv(rawsock, recv_buff, sizeof(recv_buff), 0);
                if(EINTR == errno){
                    perror("recvfrom error");
                    continue;
                }
                /* 解包，并设置相关变量 */
                ret = icmp_unpack(recv_buff, size);
                if(-1 == ret){
                    continue;
                }
            }
            break;
        }
    }
}

/* 终端信号处理函数SIGINT */
static void icmp_sigint(int signo){
    alive = 0;  /* 告诉接收和发送线程结束程序 */
    gettimeofday(&tv_end, NULL);    /* 读取程序结束时间 */
    tv_interval = icmp_tvsub(tv_end, tv_begin); /* 计算总共所用时间 */
    return;
}

/*查找一个合适的包位置
 * 当seq为-1时，表示查找空包
 * 其他值表示查找seq对应的包*/
static pingm_packet *icmp_findpacket(int seq){
    int i = 0;
    pingm_packet *found = NULL;
    /* 查找包的位置 */
    if(-1 == seq){
        for(i=0;i<128;++i){
            if(pingpacket[i].flag == 0){
                found = &pingpacket[i];
                break;
            }
        }
    }else if(seq>=0){       /* 查找对应seq的包 */
        for(i=0;i<128;++i){
            if(pingpacket[i].seq == seq){
                found = &pingpacket[i];
                break;
            }
        }
    }
    return found;
}

/* 打印全部ICMP发送接收统计结果 */
static void icmp_statistics(){
    long time = (tv_interval.tv_sec*1000) + (tv_interval.tv_usec/1000);
    printf("--- %s ping statistics ---\n", dest_str);   /* 目的IP地址 */
    printf("%d packets transmitted, %d received, %d%c packet loss, time %dms \n",
            packet_send,
            packet_recv,
            (packet_send-packet_recv)*100/packet_send,   /* 丢失百分比 */
            '%',
            time);
}
