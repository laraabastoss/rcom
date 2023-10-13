// Link layer protocol implementation

#include "link_layer.h"
#include <signal.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
int frame_number = 0;
int nRetransmissions = 0;
int timeout = 0;
int alarmEnabled = FALSE;
int alarmCount = 0;
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // check connection
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0){

        perror(connectionParameters.serialPort);
        exit(-1);
    }

    printf("n erro\n");
    
    nRetransmissions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;


    struct termios oldtio;
    struct termios newtio;
    
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }
    
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    printf("começar estados\n");
    
    LinkLayerCurrentState current_state = START;
    unsigned char byte;

    printf("Start current_state\n");

    switch(connectionParameters.role){

        case (LlTx):{
            
            printf("Entrou transmiter\n");
            
            unsigned char SET[BUF_SIZE] = {FLAG, A_T, C_SET, (A_T ^ C_SET), FLAG};
            (void) signal(SIGALRM, alarmHandler);
            unsigned char byte;

            while(alarmCount < connectionParameters.nRetransmissions){
                printf("entrou alarme\n");
                int written_bytes = write(fd, SET, BUF_SIZE);
                sleep(1);
                printf("%d bytes written\n", written_bytes);
                if (written_bytes < 0){
                    exit(-1);
                }
                alarm(connectionParameters.timeout);
                alarmEnabled = TRUE;
                while (alarmEnabled == TRUE && current_state != STOP){
                    int bytes = read(fd, &byte, 1);
                    if (bytes > 0) transmiter_state_machine(byte, current_state);

                }
            }

            break;

        }
        
        case (LlRx):{
            printf("Entrou receiver\n");
            
            while (current_state !=STOP){
                if (read(fd,&byte,1)>0){
                    printf("%u bytes", byte);
                    receiver_state_machine(byte,current_state);
                }
            
            }
            
            //Send UA
            unsigned char UA[BUF_SIZE] = {FLAG, A_R, C_UA, (A_R ^ C_UA), FLAG};
            int written_bytes = write(fd,UA,BUF_SIZE);
            sleep(1);
            if (written_bytes < 0){
                exit(-1);
            }
            else printf("%d bytes written\n",written_bytes);

            break;

        }
            

    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    unsigned char *information_frame = (unsigned char *) malloc(bufSize + 6);
    information_frame[0] = FLAG;
    information_frame[1] = A_T;
    information_frame[2] = C(frame_number);
    information_frame[3] = (information_frame[1] ^ information_frame[2]);
    memcpy(information_frame+4,buf, bufSize);
    unsigned char BCC2 = buf[0];
    for (unsigned int i = 1 ; i < bufSize ; i++){
        BCC2 ^= buf[i];
    }
    int current_size = bufSize + 6;
    int current_index = 4;

    // stuffing
    for (unsigned int i = 0 ; i<bufSize ; i++){
        if (buf[i] == FLAG || buf[i] == ESC){
            information_frame = realloc(information_frame,++current_size);
            information_frame[current_index++] = ESC;
        }
        information_frame[current_index++] = (buf[i] ^ 0x20);
    }

    information_frame[current_index++] = BCC2;
    information_frame[current_index++] = FLAG;
    alarmCount = 0;
    int accepted = 0;
    int rejected = 0;
    unsigned char byte;
    LinkLayerCurrentState current_state = START;
    while(alarmCount < nRetransmissions){

        int written_bytes = write(fd, information_frame, current_index);
        if (written_bytes < 0){
            exit(-1);
        }
        (void) signal(SIGALRM, alarmHandler);
        alarm(timeout);
        alarmEnabled = TRUE;
        accepted = 0; 
        rejected =0;
        
        while (alarmEnabled == TRUE && accepted == 0 && rejected ==0 && current_state!=STOP){
            int bytes = read(fd, &byte, 1);
            if (bytes > 0){
                unsigned char answer = control_frame_state_machine(byte,current_state);

                if (answer == C_RR(0) || answer == C_RR(1)){
                    accepted = 1;
                    frame_number += 1;
                    frame_number %= 2;
                }

                else if (answer == C_REJ(0) || answer == C_REJ(1)){
                    rejected = 1;
                }

            }

        }
     if (accepted) break;
    }

    free(information_frame);
    if (accepted) return current_index;
    else {
        llclose(fd);
        return -1;
    }
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


//alarm handler
void alarmHandler(){
    alarmEnabled = FALSE;
    alarmCount++;
}


// receiver state machine to validate SET
int receiver_state_machine(unsigned char byte,LinkLayerCurrentState current_state){
    switch (current_state){
        case (START):{

            printf("Start state");
            
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

            if (byte == (A_T ^ C_SET)){
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

        case (STOP):{
            break;
        }
    
    }

    return 0;
}

int transmiter_state_machine(unsigned char byte,LinkLayerCurrentState current_state){
    switch (current_state){
        case (START):{

            if (byte == FLAG){
                current_state = FLAG_RCV;                     
            }

            break;

        }
        case (FLAG_RCV):{

            if (byte == A_R){
                current_state = A_RCV;
            }

            else if (byte != FLAG){
                current_state = START;
            }

            break;

        }

        case (A_RCV):{

            if (byte == C_UA){
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

            if (byte == (A_R ^ C_UA)){
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

        case (STOP):{
            break;
        }
    
    }

    return 0;
}

int control_frame_state_machine(unsigned char byte, LinkLayerCurrentState current_state){
    unsigned char answer = 0;
     switch (current_state){
        case (START):{

            if (byte == FLAG){
                current_state = FLAG_RCV;                     
            }

            break;

        }
        case (FLAG_RCV):{

            if (byte == A_R){
                current_state = A_RCV;
            }

            else if (byte != FLAG){
                current_state = START;
            }

            break;

        }

        case (A_RCV):{

            if (byte == C_RR(0) || byte == C_RR(1) || byte == C_REJ(0) || byte == C_REJ(1) ){
                current_state = C_RCV;
                answer = byte;

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

            if (byte == (A_R ^ answer)){
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

        case (STOP):{
            break;
        }
    
    }
    return answer;
}
