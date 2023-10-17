// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <math.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayer;
    strcpy(linkLayer.serialPort,serialPort);
    linkLayer.role = strcmp(role, "tx") ? LlRx : LlTx;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    unsigned char *packet_received = (unsigned char *)malloc(16);
    
    FILE *file;

    int fd = llopen(linkLayer);

    switch(linkLayer.role){
        case LlTx: {
            file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Error\n");
                exit(-1);
            }

            int prev = ftell(file);
            fseek(file,0L,SEEK_END);
            long int fileSize = ftell(file)-prev;
  
            fseek(file,prev,SEEK_SET);
            printf("%i",fileSize);

            unsigned int startPacketSize;
            unsigned char *startPacket = getControlPacket(2, filename, fileSize, &startPacketSize);

            if(llwrite(fd, startPacket, startPacketSize) == -1){ 
                printf("Exit: error in start packet\n");
                exit(-1);
            }
            int bytesLeft = fileSize;
            unsigned char sequence = 0;
            unsigned char* content = getData(file, fileSize);

        
               while (bytesLeft >= 0) { 

                printf("a:");
                printf("%i \n",bytesLeft);

                int dataSize = bytesLeft > (long int) MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesLeft;
                unsigned char* data = (unsigned char*) malloc(dataSize);
                memcpy(data, content, dataSize);
                int packetSize;
                unsigned char* packet = getDataPacket(sequence, data, dataSize, &packetSize);
                
               /* if(llwrite(fd, packet, packetSize) == -1) {
                    printf("Exit: error in data packets\n");
                    exit(-1);
                }*/

                  for (int i=0;i<packetSize;i++){
                    printf("%i--",packet[i]);
                }
                printf("\n");
                
                bytesLeft -= (long int) MAX_PAYLOAD_SIZE; 
                content += dataSize; 
                sequence = (sequence + 1) % 255;   
            }

          

            unsigned char *endPacket = getControlPacket(3, filename, fileSize, &startPacketSize);

            if(llwrite(fd, endPacket, startPacketSize) == -1){ 
                printf("Exit: error in start packet\n");
                exit(-1);
            }
            break;
            
        }
        case LlRx:{

                unsigned char *packet = (unsigned char *)malloc(16);
                int packetSize = -1;
                while ((packetSize = llread(fd, packet)) < 0);
                printf("size: %i \n",packetSize);
                for (int i=0;i<packetSize;i++){
                    printf("%i\n",packet[i]);
                }
                file = fopen(filename,"wb+");
                while ((packetSize = llread(fd, packet)) < 0);
                printf("size2: %i \n",packetSize);
                for (int i=0;i<packetSize;i++){
                    printf("%i\n",packet[i]);
                }
                file = fopen(filename,"wb+");


        }
        
    }
    unsigned char i = 12;

    /*if (linkLayer.role == LlTx){
        llwrite(fd,&i,1);
    }  */

    /*if (linkLayer.role == LlRx){
        llread(fd, packet_received);
    } */
    fclose(file);
    //llclose(fd);

}

unsigned char * getControlPacket(const unsigned int c, const char* filename, long int length, unsigned int* size){

    const int L1 = (int) ceil(log2f((float)length)/8.0);
    const int L2 = strlen(filename);
    *size = 1+2+L1+2+L2;
    unsigned char *packet = (unsigned char*)malloc(*size);
    
    unsigned int pos = 0;
    packet[pos++]=c;
    packet[pos++]=0;
    packet[pos++]=L1;

    for (unsigned char i = 0 ; i < L1 ; i++) {
        packet[2+L1-i] = length & 0xFF;
        length >>= 8;
    }
    pos+=L1;
    packet[pos++]=1;
    packet[pos++]=L2;
    memcpy(packet+pos, filename, L2);
    return packet;
}

unsigned char * getDataPacket(unsigned char sequence, unsigned char *data, int dataSize, int *packetSize){

    *packetSize = 1 + 1 + 2 + dataSize;
    unsigned char* packet = (unsigned char*)malloc(*packetSize);

    packet[0] = 1;   
    packet[1] = sequence;
    packet[2] = dataSize >> 8 & 0xFF;
    packet[3] = dataSize & 0xFF;
    memcpy(packet+4, data, dataSize);

    return packet;
}

unsigned char * getData(FILE* fd, long int fileLength) {
    unsigned char* content = (unsigned char*)malloc(sizeof(unsigned char) * fileLength);
    fread(content, sizeof(unsigned char), fileLength, fd);
    return content;
}