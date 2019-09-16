#ifndef _PACKET_H_
#define _PACKET_H_

#define MAX_CMD_LEN     512

typedef struct{
    int  len;
    char command[MAX_CMD_LEN];
}stPacket;

#endif
