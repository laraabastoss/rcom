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

    printf("ll created \n");

    unsigned char i = 1;
    int fd = llopen(linkLayer);
    printf("%i",fd);
    if (linkLayer.role == LlTx){
        printf("entrou");
        llwrite(fd,&i,1);
    }   

}
