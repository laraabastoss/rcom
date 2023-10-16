// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort,serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    unsigned char *packet_received = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);
    
    unsigned char* packet = (unsigned char *)malloc(4);

    packet[0] = 1;
    packet[1] = 0;
    packet[2] = 1 & 0xFF;
    //memcpy(packet+4, 0x01, 1);

    unsigned char i = 12;
    int fd = llopen(linkLayer);
    if (linkLayer.role == LlTx){
        llwrite(fd,&i,1);
    }  

    if (linkLayer.role == LlRx){
        llread(fd, packet_received);
    } 

}
