#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/uio.h>

#define SYS_ERR(e) {\
    perror(e);\
    exit(0);\
}

void sig_int_server(int signo);
void sig_int_client(int signo);

void proc_conn_client(int socket);
void proc_conn_server(int socket);

#endif
