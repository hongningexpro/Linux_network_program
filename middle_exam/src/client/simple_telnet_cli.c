#include "common.h"
#include "Packet.h"

void usage(){
    printf("Usage: ./simp_telnet_cli [srv_ip] [srv_port]\n");
    printf("Example: ./simp_telnet_cli 127.0.0.1 6633\n");
    exit(0);
}

int create_client(char *argv[]);
void handle_client(int sock);

int main(int argc, char *argv[]){
    int sock;
    if(argc != 3) {
        usage();
    }

    sock = create_client(argv);

    handle_client(sock);

    close(sock);

    return 0;
}

int create_client(char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == sock){
        SYS_ERR("create socket error");
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)) == -1){
        close(sock);
        SYS_ERR("connect error");
    }


    return sock;
}

void handle_client(int sock){
    char buffer[1024];
    char recv_buffer[4096];
    int msg_len;
    int ret;
    stPacket pack_msg;

    memset(buffer, 0, 1024);
    while(gets(buffer)){
        memset(&pack_msg, 0, sizeof(stPacket));
        memset(recv_buffer, 0, 4096);

        msg_len = strlen(buffer) ;
        if(!msg_len){
            continue;
        }
        pack_msg.len = htonl(msg_len);
        memcpy(pack_msg.command, buffer, msg_len);

        ret = write(sock, (void*)&pack_msg, msg_len+4);
        if(-1 == ret){
            SYS_ERR("write error");
        }

        while((ret=read(sock, recv_buffer, 4096))>=0){
            if(!ret){
                return;
            }
            write(1, recv_buffer, ret);
            if(ret < 4096){
                /* 如果是正好等于4096字节,那没脾气的... */
                break;
            }
        }

        memset(buffer, 0, 1024);
    }
}
