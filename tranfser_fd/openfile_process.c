#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

ssize_t send_fd(int fd, void *data, size_t bytes, int sendfd){
    struct msghdr msghdr_send;
    struct iovec iov[1];
    size_t n;
    int newfd;
    /* 方便操作msg的结构 */
    union{
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *pcmsghdr = NULL;
    msghdr_send.msg_control = control_un.control;   /* 控制消息 */
    msghdr_send.msg_controllen = sizeof(control_un.control);

    pcmsghdr = CMSG_FIRSTHDR(&msghdr_send);     /* 取得第一个消息头 */
    pcmsghdr->cmsg_len = CMSG_LEN(sizeof(int));
    pcmsghdr->cmsg_level = SOL_SOCKET;          /* 用于控制消息 */
    pcmsghdr->cmsg_type = SCM_RIGHTS;
    *((int*)CMSG_DATA(pcmsghdr)) = sendfd;      /* socket值 */

    msghdr_send.msg_name = NULL;
    msghdr_send.msg_namelen = 0;

    iov[0].iov_base = data;
    iov[0].iov_len = bytes;
    msghdr_send.msg_iov = iov;
    msghdr_send.msg_iovlen = 1;
    sendmsg(fd, &msghdr_send, 0);
}

int main(int argc, char *argv[]){
    int fd;
    ssize_t n;
    if(4 != argc){
        printf("socketpair error\n");
    }
    if((fd = open(argv[2], atoi(argv[3])))<0){
        /* 打开输入的文件名称 */
        printf("oepn failed\n");
        return 0;
    }
    if((n = send_fd(atoi(argv[1]), "", 1, fd))<0){
        /* 发送文件描述符 */
        perror("send failed\n");
        return 0;
    }
    return 0;
}
