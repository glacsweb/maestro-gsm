/*
 * Glacsweb serials.h
 * Copyright (C) 2003, 2004 Kirk Martinez, Alistair Riddoch,
 *                          The University of Southampton
 */

#ifndef GLACSWEB_SERIAL_H
#define GLACSWEB_SERIAL_H

#include "types.h"

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>

/** Structure to hold data to handle an open serial port. Used by
 *  all code that uses standard serial ports to talk to devices.
 */
typedef struct serial_port {
    /** File descriptor of serial port */
    int         sp_fd;
    /** File pointer of log file associated with this port */
    FILE *      sp_logfp;
} SerialPort;

// New clean OO API for handling many ports

int SERGetBaud(const char * baud_string, speed_t * serial_speed);
SerialPort * SEROpenPort(const char * serialportname,
                         speed_t serial_speed,
                         char * logfilename);
void SERClosePort(SerialPort * sp);
BYTE SERGetByte(SerialPort * sp);
void SERPutByte(SerialPort * sp, BYTE b);
int  SERPutString(SerialPort * sp, const char * s);
void SERFlushChannel(SerialPort * sp, int usec);
int  SERQueryChannel(SerialPort * sp, int usec);
int  SERGetByteTimeout(SerialPort * sp, int usec);

/* Clear any incoming data that hasn't yet been read from the serial port */
void SERClearIncoming( SerialPort *sp );

/** Read some bytes from a serial port with a timeout.
 *  Wait at most usec microseconds for data to be available on the
 *  given serial port, and then read some bytes if it they are available.
 *  This is repeated until enough bytes have been read.
 *  @param sp serial port to read from.
 *  @param buffer pointer to buffer where bytes are stored.
 *  @param count number of bytes to read.
 *  @param usec number of microseconds to wait.
 *  @return number of bytes read, or -1 on timeout. */
int  SERGetBytesTimeout(SerialPort * sp, BYTE * buffer, int count, int usec);

#endif // GLACSWEB_SERIAL_H
