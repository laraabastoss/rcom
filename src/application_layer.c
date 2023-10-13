// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer ll;
    ll.nRetransmissions = nTries;
    ll.timeout = timeout;
    ll.role = strcmp(role, "tx") ? LlRx : LlTx;
    ll.baudRate = baudRate;
    strcpy(ll.serialPort, serialPort);

    printf("ll created \n");
    
    llopen(ll);
}
