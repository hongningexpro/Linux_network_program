#include "common.h"

void usage(){
    printf("Usage: ./client ip port\n");
    printf("Example: ./client 127.0.0.1 6633\n");
    exit(0);
}


int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in server_addr;
    
    if(3!=argc){
        usage();
    }



    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    signal(SIGINT, sig_int_client);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == sock){
        SYS_ERR("create socket failed");
    }
    #if 0
    if(-1 == bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        SYS_ERR("bind failed");
    }
    #endif

    if(-1 == connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        SYS_ERR("connect failed");
    }

    proc_conn_client(sock);
    close(sock);

    return 0;
}
