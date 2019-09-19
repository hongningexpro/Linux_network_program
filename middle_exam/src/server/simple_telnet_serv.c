#include "common.h"
#include "Packet.h"

void usage(){
    printf("Usage: ./simpl_telnet_serv  port\n");
    printf("Example: ./simpl_telnet_serv 6633\n");
    exit(0);
}

int create_server(char **argv);
void handle_server(int sock);

int main(int argc, char *argv[]){
    int sock;
    if(2 != argc){
        usage();
    }

    sock = create_server(argv);
    handle_server(sock);

    close(sock);

    return 0;
}

int create_server(char **argv){
    int sock;
    int option = 1;
    struct sockaddr_in local_addr;

    memset(&local_addr, 0, sizeof(struct sockaddr_in));

    if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
        SYS_ERR("create socket error");
    }

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));   /* 设置地址复用 */

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(atoi(argv[1]));
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock, (struct sockaddr*)&local_addr, sizeof(struct sockaddr)) == -1){
        SYS_ERR("bind error");
    }

    if(listen(sock, 1) == -1){
        SYS_ERR("listen error");
    }
    return sock;
}

void handle_server(int sock){
    int con_sock;
    int ret;
    int i_len;
    struct sockaddr_in peer_addr;
    socklen_t   cli_len;
    char buffer[512];
    char path[128];

    while(1){
        memset(&peer_addr, 0, sizeof(struct sockaddr_in));
        con_sock = accept(sock, (struct sockaddr*)&peer_addr, &cli_len);
        if(-1 == con_sock){
            SYS_ERR("accept error");
        }


        while(1){
            memset(&i_len, 0, sizeof(i_len));
            memset(buffer, 0, sizeof(buffer));
            /* 先预读四个字节 */
            ret = read(con_sock, (void*)&i_len, 4);
            if(-1 == ret){
                /* 关于信号引起的错误先不处理了 。。。*/
                SYS_ERR("read i_len error");
            }else if(!ret){
                break;
            }
            i_len = ntohl(i_len);
            ret = read(con_sock, buffer, i_len);
            if(-1 == ret){
                SYS_ERR("read buffer error");
            }
            dup2(con_sock, 1);
            dup2(con_sock, 2);
            if(!memcmp(buffer, "quit", 4)){
                /* 如果对端输入quit，则退出 */ 
                break;
            }else if(!memcmp(buffer, "cd", 2)){
                memset(path, 0, 128);
                sscanf(buffer, "cd %s\n", path);
                ret = chdir(path);
                if(ret != -1){
                    write(con_sock, "change dir successfully!\n", strlen("change dir successfully!\n"));
                }else{
                    write(con_sock, "change dir failed!\n", strlen("change dir failed!\n"));
                }
                continue;
            }
            system(buffer);
        }
        shutdown(con_sock, SHUT_RDWR);
    }
}
