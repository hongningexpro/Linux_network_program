#include "common.h"

void sig_int_server(int signo){
    printf("Server exit\n");
    exit(0);
}

void sig_int_client(int signo){
    printf("Client exit\n");
    exit(0);
}

void proc_conn_server(int sock){
    ssize_t size = 0;
    char buffer[1024];

    for(;;){
        memset(buffer, 0 , 1024);
        size = recv(sock, buffer, 1024, 0);

        if(0 == size){
            break;
        }

        sprintf(buffer, "%ld bytes altogether\n", size);
        send(sock, buffer, strlen(buffer)+1, 0);
    }
}

void proc_conn_client(int sock){
    ssize_t size = 0;
    char buffer[1024];

    for(;;){
        memset(buffer, 0 ,1024);
        size = read(0, buffer, 1024);

        if(size>0){
            send(sock, buffer, size, 0);
            size = recv(sock, buffer, 1024, 0);
            write(1, buffer, size);
        }
    }
}
