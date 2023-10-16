// Link layer protocol implementation

#include "link_layer.h"
#include <signal.h>

// MISC
volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;
int timeout = 0;
int retransmitions = 0;
unsigned char tramaTx = 0;
unsigned char tramaRx = 1;
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    // check connection
    int fd = set_serial_port(connectionParameters);

    LinkLayerStateMachine state = START;

    switch (connectionParameters.role) {

        case LlTx: {
            (void) signal(SIGALRM, alarmHandler);

            state = transmiter_state_machine(fd,state);
            if (state != STOP_R){
                return -1;
            }

            break;  
        }

        case LlRx: {
            receiver_state_machine(fd,state);
            unsigned char UA[5] = {FLAG, A_RE, C_UA, A_RE ^ C_UA, FLAG};
            write(fd, UA, 5);
            break; 
        }

        default:
            return -1;
            break;
    }
    
    return fd;
}

   


////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize)
{
    unsigned char *information_frame = (unsigned char *) malloc(bufSize + 6);
    information_frame[0] = FLAG;
    
    information_frame[1] = A_ER;
    information_frame[2] = C_N(tramaTx);
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
            information_frame[current_index++] = (buf[i] ^ 0x20);
        }
        else{
            information_frame[current_index++] = buf[i];
        }
        
    }

    information_frame[current_index++] = BCC2;
    information_frame[current_index++] = FLAG;
    alarmCount = 0;
    int accepted = 0;
    int rejected = 0;
    unsigned char byte = '\0';
    LinkLayerStateMachine current_state = START;
    //(void) signal(SIGALRM, alarmHandler);

    while(alarmCount < retransmitions){
        alarmEnabled = TRUE;
        alarm(0);
        alarm(timeout);
        printf("alarm write\n");
        accepted = 0; 
        rejected = 0;
        
        while (alarmEnabled == TRUE && accepted == 0 && rejected ==0 ){
            

            int written_bytes = write(fd, information_frame, current_index);

            if (written_bytes < 0){
                exit(-1);
            }
         
                unsigned char answer = control_frame_state_machine(fd, byte,current_state);

                printf("outside answer: %d\n", answer);
                
                if (answer == C_RR(0) || answer == C_RR(1)){
                    accepted = 1;
                    tramaTx += 1;
                    tramaTx %= 2;
                    alarmCount = retransmitions;
                }

                else if (answer == C_REJ(0) || answer == C_REJ(1)){
                    rejected = 1;
                }

            }

        }
    
    

    free(information_frame);
    if (accepted) return current_index;
    else {
        llclose(fd);
        printf("not accepted\n");
        return -1;
    }
    
   return 1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(int fd, unsigned char *packet)
{

    unsigned char byte, control_field;
    LinkLayerStateMachine current_state = START;
    int current_index = 0;
    
    while (current_state != STOP_R){
        if (read(fd, &byte, 1) > 0){
            switch (current_state) {
                case START:
                    
                    if (byte == FLAG) current_state = A_RCV;
                    break;
                
                case FLAG_RCV:

                    if (byte == A_ER) current_state = A_RCV;
                    else if (byte != FLAG) current_state = START;
                    break;

                case A_RCV:

                    if (byte == C_N(0) || byte == C_N(1)){

                        current_state = C_RCV;
                        control_field = byte;

                    }
                    else if (byte == FLAG) current_state = FLAG_RCV;
                    else if (byte == C_DISC){

                        unsigned char DISC[5] = {FLAG, A_RE, C_DISC, A_RE ^ C_DISC, FLAG};
                        write(fd, DISC, 5);
                        return 0;

                    }
                    else current_state = START;
                    break;
                
                case C_RCV:
                    
                    if (byte == (A_ER ^ control_field)) current_state = READING;
                    else if (byte == FLAG) current_state = FLAG_RCV;
                    else current_state = START;
                    break;
                
                case READING:
                    if (byte == ESC) current_state = FOUND_STUFFING;
                    else if (byte == FLAG){
                        
                        unsigned char bcc2 = packet[current_index-1];
                        current_index--;
                        packet[current_index] = '\0';
                        unsigned char bcc2_test = packet[0];

                        for (unsigned int i = 1; i < current_index; i++){

                            bcc2_test ^= packet[i];

                        }

                        if (bcc2 == bcc2_test){

                            current_state = STOP_R;
                            unsigned char ACCEPTED[5] = {FLAG, A_RE, C_RR(tramaRx), A_RE ^ C_RR(tramaRx), FLAG};
                            write(fd, ACCEPTED, 5);
                            tramaRx = (tramaRx + 1)%2;
                            return current_index;
                        }

                        else{

                            unsigned char REJECTED[5] = {FLAG, A_RE, C_REJ(tramaRx), A_RE ^ C_REJ(tramaRx), FLAG};
                            write(fd, REJECTED, 5);
                            return -1;

                        }
                    }
                    else{
                        packet[current_index++] = byte;
                    }
                    break;
                case FOUND_STUFFING:
                    current_state = READING;
                    if (byte == (ESC^0x20) || byte == (FLAG^0x20)) packet[current_index++] = (byte^0x20);
                    break;
                default:
                    break;

            }
        }
    }

    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    LinkLayerStateMachine current_state;
    unsigned char byte;
    alarmEnabled = TRUE;
    alarmCount = 0;

    while (alarmCount < retransmitions && current_state!=STOP){

        unsigned char DISC[5] = {FLAG, A_ER, C_DISC, A_ER ^ C_DISC, FLAG};
        write(showStatistics, DISC, 5);
        alarm(0);
        alarm(timeout);
        printf("alarm close\n");
        alarmEnabled = TRUE;
        while (alarmEnabled == TRUE && current_state!=STOP_R){

            if (read(showStatistics, &byte,1) > 0){
                 
                    switch (current_state) {

                        case START:

                            if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            break;
                        case FLAG_RCV:

                            if (byte == A_ER){
                                current_state = A_RCV;
                            }

                            else if (byte != FLAG) current_state = START;

                            break;

                        case A_RCV:

                            if (byte == C_DISC){
                                current_state = C_RCV;
                            }

                            else if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            else current_state = START;

                            break;

                        case C_RCV:

                            if (byte == (A_ER ^ C_DISC)){
                                current_state = BCC_OK;
                            }

                            else if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            else current_state = START;

                            break;
                        case BCC_OK:

                            if (byte == FLAG){
                                current_state = STOP_R;
                            }

                            else current_state = START;

                            break;

                        default: 

                            break;
                    }   
            }
        }
    }

    if (current_state != STOP_R) return -1;
    unsigned char UA[5] = {FLAG, A_ER, C_UA, A_ER ^ C_UA, FLAG};
    write(showStatistics, UA, 5);

    return close(showStatistics);
}


//alarm handler
void alarmHandler(int signal) {
    alarmEnabled = FALSE;
    alarmCount++;
    printf("\n count++ \n");
}

// receiver state machine to validate SET
void receiver_state_machine(int fd, LinkLayerStateMachine current_state){

       unsigned char byte;

       while (current_state != STOP_R) {

                if (read(fd, &byte, 1) > 0) {
                 
                    switch (current_state) {

                        case START:

                            if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            break;
                        case FLAG_RCV:

                            if (byte == A_ER){
                                current_state = A_RCV;
                            }

                            else if (byte != FLAG) current_state = START;

                            break;

                        case A_RCV:

                            if (byte == C_SET){
                                current_state = C_RCV;
                            }

                            else if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            else current_state = START;

                            break;

                        case C_RCV:

                            if (byte == (A_ER ^ C_SET)){
                                current_state = BCC_OK;
                            }

                            else if (byte == FLAG){
                                current_state = FLAG_RCV;
                            }

                            else current_state = START;

                            break;
                        case BCC_OK:

                            if (byte == FLAG){
                                current_state = STOP_R;
                            }

                            else current_state = START;

                            break;

                        default: 

                            break;
                    }
                }
            }  
  
}

LinkLayerStateMachine transmiter_state_machine(int fd,LinkLayerStateMachine current_state){

   unsigned char byte;
   while (alarmCount < retransmitions && current_state != STOP_R) {

                unsigned char SET[5] = {FLAG, A_ER, C_SET, A_ER ^ C_SET, FLAG};
                write(fd, SET, 5);
                alarm(0);
                alarm(timeout);
                printf("alarm open\n");
                alarmEnabled = TRUE;
                
                while (alarmEnabled == TRUE && current_state != STOP_R) {

                    if (read(fd, &byte, 1) > 0) {
                      
                        switch (current_state) {

                            case START:

                                if (byte == FLAG){
                                    current_state = FLAG_RCV;
                                }

                                break;

                            case FLAG_RCV:

                                if (byte == A_RE){
                                    current_state = A_RCV;
                                } 

                                else if (byte != FLAG){
                                    current_state = START;
                                }
                                break;

                            case A_RCV:

                                if (byte == C_UA){
                                    current_state = C_RCV;
                                } 
                                else if (byte == FLAG){
                                    current_state = FLAG_RCV;
                                } 

                                else current_state = START;

                                break;

                            case C_RCV:

                                if (byte == (A_RE ^ C_UA)){
                                    current_state = BCC_OK;
                                } 

                                else if (byte == FLAG){
                                    current_state = FLAG_RCV;
                                }

                                else current_state = START;

                                break;

                            case BCC_OK:

                                if (byte == FLAG){
                                
                                    current_state = STOP_R;
                                }

                                else current_state = START;

                                break;

                            default: 
                                break;
                        }
                    }
                } 
              
            }
            return current_state;
}

int control_frame_state_machine(int fd, unsigned char byte, LinkLayerStateMachine state){

    int answer = 0;
    LinkLayerStateMachine current_state = START;
    while (current_state != STOP_R && alarmEnabled == TRUE) {  
        
        if (read(fd, &byte, 1) > 0 || 1) {
            switch (current_state){
            case (START):{

                if (byte == FLAG){
                    current_state = FLAG_RCV;                     
                }

                break;

            }
            case (FLAG_RCV):{

                if (byte == A_RE){
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
                    printf("answer: %d\n", answer);

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

                if (byte == (A_RE ^ answer)){
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

            default:

                break;

            }
        }
    }
    printf("final answer: %d\n", answer);
    return answer;
     
}



int set_serial_port(LinkLayer connectionParameters){

    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    if (fd < 0){

        perror(connectionParameters.serialPort);
        exit(-1);
    }
    
    retransmitions = connectionParameters.nRetransmissions;
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
    return fd;
}
