// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <math.h>

LinkLayer connectionParameters;


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename, IntroduceError error)
{

    strcpy(connectionParameters.serialPort,serialPort);
    if (strcmp(role,"tx")){
           connectionParameters.role = LlRx;
    }
    else{
        connectionParameters.role = LlTx;
    }
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    
    FILE *file;

    int fd = llopen(connectionParameters,error);

    switch(connectionParameters.role){

        case LlRx:{

                file = fopen(filename,"wb+");
                unsigned char *packet = (unsigned char *)malloc(MAX_PAYLOAD_SIZE);

                while(1){

                    int packetSize = llread(fd, packet);
                    if (packetSize>=0){
                        break;
                   } 
                } 

                while (1) {    
                    int packetSize;
                    while (1){
                        packetSize = llread(fd, packet);
                        if (packetSize>=0) break;
                    }
                        
    
                    if(packetSize == 0) break;

                    else if(packet[0] != 3){
                        unsigned char *buffer = (unsigned char*)malloc(packetSize);
                        parseDataPacket(packet, packetSize, buffer);
                        fwrite(buffer, sizeof(unsigned char), packetSize-4, file);
                        free(buffer);
                    }

                    else break;
                
                }

                 while(1){

                    int packetSize = llread(fd, packet);
                    if (packetSize>=0){
                        break;
                   } 
                } 


                break;
           

        }
        case LlTx: {

            file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Error\n");
                exit(-1);
            }

            fseek(file, 0, SEEK_END); 
            int fileSize = ftell(file); 
            fseek(file, 0, SEEK_SET);
    

            unsigned int startPacketSize;
            unsigned char *startPacket = parseControl(2, filename, fileSize, &startPacketSize);

            if(llwrite(fd, startPacket, startPacketSize) == -1){ 
                printf("Exit: error in start packet\n");
                exit(-1);
            }

            int bytesLeft = fileSize;
            unsigned char sequence = 0;
            unsigned char* content = getData(file, fileSize);

        
            while (bytesLeft >= 0) { 

      
                int byteSent;
                int sizeofIframe;
                switch (error)
                {
                case FrameSize:
                    sizeofIframe = 100;
                    break;
                
                default:
                    sizeofIframe = MAX_PAYLOAD_SIZE;
                    break;
                }

                if (sizeofIframe - 4 < bytesLeft){
                    byteSent = sizeofIframe - 4;
                }
                
                else{
                    byteSent = bytesLeft;
                }
                 

                unsigned char* dataPacket = (unsigned char*)malloc(byteSent + 4);

                dataPacket[0] = 1;   
                dataPacket[1] = sequence;
                dataPacket[2] = byteSent >> 8 & 0xFF;
                dataPacket[3] = byteSent & 0xFF;
                memcpy(dataPacket+4, content, byteSent);

                
                if(llwrite(fd, dataPacket, byteSent + 4) == -1) {
                    printf("Exit: error in data packets\n");
                    exit(-1);
                }

              
                bytesLeft -= (long int) sizeofIframe - 4; 
                content += byteSent; 
                sequence = (sequence + 1) % 255;   
            }

          

            unsigned char *endPacket = parseControl(3, filename, fileSize, &startPacketSize);

            if(llwrite(fd, endPacket, startPacketSize) == -1){ 
                printf("Exit: error in start packet\n");
                exit(-1);
            }

            llclose(fd);

            break;

           
            
        }
        
    default:
        break;
        
    }
    


    fclose(file);
    

}

unsigned char * parseControl(const unsigned int c, const char* filename, long int length, unsigned int* size){
    unsigned L1 = sizeof(size);
    unsigned L2 = strlen(filename);
    *size = 3+L1+L2;
    unsigned char *packet = (unsigned char*)malloc(3+L1+L2);
    
    unsigned int pos = 0;
    packet[0]=c;
    packet[1]=0;
    packet[2]=L1;

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


unsigned char * getData(FILE* fd, long int fileLength) {
    unsigned char* content = (unsigned char*)malloc(sizeof(unsigned char) * fileLength);
    fread(content, sizeof(unsigned char), fileLength, fd);
    return content;
}

void parseDataPacket(const unsigned char* packet, const unsigned int packetSize, unsigned char* buffer) {
    memcpy(buffer,packet+4,packetSize-4);
    buffer += packetSize+4;
}

