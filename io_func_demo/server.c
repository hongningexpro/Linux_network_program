#include "common.h"

void usage(){
    printf("Usage: ./server port\n");
    printf("Example: ./server 6633\n");
    exit(0);
}


int main(int argc, char *argv[]){
    int listen_sock, conn_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t clilen;
    pid_t   pid;
    if(2 != argc){
        usage();
    }
    
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    signal(SIGINT, sig_int_server);

    listen_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == listen_sock){
        SYS_ERR("create socket failed");
    }

    if(-1 == bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        SYS_ERR("bind failed");
    }

    if(-1 == listen(listen_sock, 5)){
        SYS_ERR("listen failed");
    }

    while(1){
        conn_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &clilen);
        if(-1 == conn_sock){
            SYS_ERR("accept failed");
        }
        pid = fork();
        if(-1 == pid){
            SYS_ERR("fork failed");
        }else if(0 == pid){
            close(listen_sock);
            proc_conn_server(conn_sock);
            close(conn_sock);
            exit(0);
        }else{
            close(conn_sock);
        }
    }
    close(listen_sock);
}
