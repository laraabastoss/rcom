// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include <stdio.h>

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);

unsigned char * getControlPacket(const unsigned int c, const char* filename, long int length, unsigned int* size);


unsigned char * getData(FILE* fd, long int fileLength);

void parseDataPacket(const unsigned char* packet, const unsigned int packetSize, unsigned char* buffer);



#endif // _APPLICATION_LAYER_H_
