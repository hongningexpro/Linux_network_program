#include "common.h"

struct iovec *vs=NULL, *vc=NULL;

void sig_int_server(int signo){
    printf("catch a exit signal\n");
    free(vs);
    exit(0);
}

void sig_int_client(int signo){
    printf("catch a exit signal\n");
    free(vc);
    exit(0);
}

void proc_conn_server(int sock){
    char buffer[30];
    ssize_t size = 0;
    struct msghdr msg;

    struct iovec *v = (struct iovec*)malloc(3*sizeof(struct iovec));
    if(!v){
        printf("Not enough memory\n");
        return;
    }
    
    vs = v;

    msg.msg_name = NULL;            /* 没有名字域 */
    msg.msg_namelen = 0;            /* 名字域长度为0 */
    msg.msg_control = NULL;         /* 没有控制域 */
    msg.msg_controllen = 0;         /* 控制域长度为0 */
    msg.msg_iov = v;
    msg.msg_iovlen = 30;
    msg.msg_flags = 0;

    v[0].iov_base = buffer;
    v[1].iov_base = buffer+10;
    v[2].iov_base = buffer+20;
    v[0].iov_len = v[1].iov_len = v[2].iov_len = 10;

    for(;;){
        size = recvmsg(sock, &msg, 0);
        if(0 == size){
            return;
        }

        sprintf(v[0].iov_base, "%ld ", size);
        sprintf(v[1].iov_base, "bytes alt");
        sprintf(v[2].iov_base, "ogether\n");

        v[0].iov_len = strlen(v[0].iov_base);
        v[1].iov_len = strlen(v[1].iov_base);
        v[2].iov_len = strlen(v[2].iov_base);
        sendmsg(sock, &msg, 0);
    }
}

void proc_conn_client(int sock){
    char buffer[30];
    ssize_t size = 0;
    struct msghdr msg;
    int i;

    struct iovec *v = (struct iovec*)malloc(3*sizeof(struct iovec));
    if(!v){
        printf("Not enough memory\n");
        return;
    }

    vc = v;
    msg.msg_name = NULL;            /* 没有名字域 */
    msg.msg_namelen = 0;            /* 名字域长度为0 */
    msg.msg_control = NULL;         /* 没有控制域 */
    msg.msg_controllen = 0;         /* 控制域长度为0 */
    msg.msg_iov = v;
    msg.msg_iovlen = 30;
    msg.msg_flags = 0;

    v[0].iov_base = buffer;
    v[1].iov_base = buffer+10;
    v[2].iov_base = buffer+20;
    v[0].iov_len = v[1].iov_len = v[2].iov_len = 10;

    for(;;){
        size = read(0, v[0].iov_base, 10);
        if(size>0){
            v[0].iov_len = size;
            sendmsg(sock, &msg, 0);
            v[0].iov_len = v[1].iov_len = v[2].iov_len = 10;

            perror("ss");
            size = recvmsg(sock, &msg, 0);
            printf("size:%ld\n", size);
            for(i=0; i<3; ++i){
                if(v[i].iov_len>0){
                    write(1, v[i].iov_base, v[i].iov_len);
                }
            }
        }
    }
}
