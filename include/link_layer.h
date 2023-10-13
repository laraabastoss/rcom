// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_
typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

typedef enum
{
  START,
  FLAG_RCV,
  A_RCV,
  C_RCV,
  BCC_OK,
  STOP,
}LinkLayerCurrentState;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5

//Frame constants
#define BAUDRATE 38400
#define FLAG 0x7E
#define A_T 0x03
#define A_R 0x01
#define C_SET 0x03
#define C_UA 0x07
#define ESC 0x7d
#define C(N) (N << 6)
#define C_RR(N) ((N << 7) | 0x05)
#define C_REJ(N) ((N <<7) | 0x01)



//Global variables
volatile int STOP = FALSE;
int alarmEnabled = FALSE;
int alarmCount = 0;


// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(int fd, const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

//Receiver state machine to validate SET
int receiver_state_machine(unsigned char byte,LinkLayerCurrentState current_state);

//Alarm handler
void alarmHandler();

#endif // _LINK_LAYER_H_
