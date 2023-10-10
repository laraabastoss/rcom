// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // check connection
    int fd = open(connectionParameters.serialPort,O_RDWR|O_NOCTTY);
    if (fd < 0){

    perror(connectionParameters.serialPort);
    exit(-1);

    }

    LinkLayerCurrentState current_state = START;
    unsigned char byte;

    switch(connectionParameters.role){
        case (LlRx):{
            while (current_state !=STOP){
                if (read(fd,&byte,1)>0){
                    receiver_state_machine(byte,current_state);
                }
            
            }
            
            //Send UA
            usigned char UA[BUF_SIZE] = {FLAG, A_R, C_UA, A_R ^ C_UA, FLAG};
            int written_bytes = write(fd,UA,BUF_SIZE);
            if (written_bytes < 0){
                exit(-1);
            }
            else printf("%d bytes written\n",written_bytes)

        }
            

    }

    return 1;
}
int receiver_state_machine(unsigned char byte,LinkLayerCurrentState current_state){
    switch (current_state){
        case (START):{

            if (byte == FLAG){
                current_state = FLAG_RCV;                     
            }

            break;

        }
        case (FLAG_RCV):{

            if (byte == A_T){
                current_state = A_RCV;
            }

            else if (byte != FLAG){
                current_state = START;
            }

            break;

        }

        case (A_RCV):{

            if (byte == C_SET){
                current_state = C_RCV;
            }

            else if (byte == FLAG){
                current_state = FLAG_RCV;
            }
            
            else{
                current_state = START;
            }

            break;

        }

        case (C_RCV):{

            if (byte == A_T ^ C_SET){
                current_state = BCC_OK;
            }
            
            else if (byte == FLAG){
                current_state = FLAG_RCV;
            }
            
            else{
                current_state = START;
            }

            break;
        }

        case (BCC_OK):{

            if (byte == FLAG){
                current_state = STOP;
            }

            else{
                current_state = START;
            }

            break;

        }
    
    }
}
////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
