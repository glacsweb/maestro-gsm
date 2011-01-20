/* 
 * Glacsweb serial.c
 * central serial port handling code
 */

/** \file
 * Low level serial port handlers.
 *
 * Copyright (C) 2003, 2004 Kirk Martinez, Alistair Riddoch,
 *                          The University of Southampton
 */


#include "serial.h"

#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/** Run debug code if debugging is enabled. */
#define debug(prg) { if (debug_flag) { prg } }

/** Control whether debug code is run.
 *  Set to non-zero if debug code should be run.
 */
static const int debug_flag = 0;

/** Get the baud rate from a string, and return the speed_t value.
 *  Accepts the baud name as a string. Any of the standard baud rates
 *  supported by standard serial ports should be accepted.
 *  @param baud_string is a string describing the baud rate
 *  @param serial_speed is a pointer used to return the speed
 *  @return zero if baud rate is recognised, non-zero otherwise
 */
int SERGetBaud(const char * baud_string, speed_t * serial_speed)
{
    assert(baud_string != NULL);
    assert(serial_speed != NULL);

    if (strcmp("0", baud_string) == 0) {
        *serial_speed = B0;
        return 0;
    } else if (strcmp("50", baud_string) == 0) {
        *serial_speed = B50;
        return 0;
    } else if (strcmp("75", baud_string) == 0) {
        *serial_speed = B75;
        return 0;
    } else if (strcmp("110", baud_string) == 0) {
        *serial_speed = B110;
        return 0;
    } else if (strcmp("134", baud_string) == 0) {
        *serial_speed = B134;
        return 0;
    } else if (strcmp("150", baud_string) == 0) {
        *serial_speed = B150;
        return 0;
    } else if (strcmp("200", baud_string) == 0) {
        *serial_speed = B200;
        return 0;
    } else if (strcmp("300", baud_string) == 0) {
        *serial_speed = B300;
        return 0;
    } else if (strcmp("1200", baud_string) == 0) {
        *serial_speed = B1200;
        return 0;
    } else if (strcmp("1800", baud_string) == 0) {
        *serial_speed = B1800;
        return 0;
    } else if (strcmp("2400", baud_string) == 0) {
        *serial_speed = B2400;
        return 0;
    } else if (strcmp("4800", baud_string) == 0) {
        *serial_speed = B4800;
        return 0;
    } else if (strcmp("9600", baud_string) == 0) {
        *serial_speed = B9600;
        return 0;
    } else if (strcmp("19200", baud_string) == 0) {
        *serial_speed = B19200;
        return 0;
    } else if (strcmp("38400", baud_string) == 0) {
        *serial_speed = B38400;
        return 0;
    } else if (strcmp("57600", baud_string) == 0) {
        *serial_speed = B57600;
        return 0;
    } else if (strcmp("115200", baud_string) == 0) {
        *serial_speed = B115200;
        return 0;
    } else if (strcmp("230400", baud_string) == 0) {
        *serial_speed = B230400;
        return 0;
    }
    return 1;
}

/** Create and initialise a new SerialPort structure.
 *  For internal use only.
 *  @return a pointer to the new serial port structure on the heap.
 */
static SerialPort * new_SerialPort()
{
    SerialPort * sp = calloc(1, sizeof(SerialPort));
    if (sp == NULL) {
        // erexit("Out of memory\n");
        return NULL;
    } else {
        sp->sp_fd = -1;
        sp->sp_logfp = NULL;
    }
    return sp;
}

/** Open a serial port, and set the baud rate.
 *  @param serialportname string giving the filename of the serial device
 *  @param serial_speed baud rate to use
 *  @param logfilename string giving the filename of a log file to be used
 *  in association with this serial port.
 *  @return a pointer to the new serial port structure on the heap.
 */
SerialPort * SEROpenPort(const char * serialportname,
                         speed_t serial_speed,
                         char * logfilename)
{
    struct termios term;
    SerialPort * sp;

    assert(serialportname != NULL);

    sp = new_SerialPort();
    if (sp == NULL) {
        // The error should already have been reported.
        return sp;
    }

    if (logfilename != NULL) {
        // open log file for appending
        sp->sp_logfp = fopen(logfilename, "a");
        if (sp->sp_logfp == NULL) {
            // erexit("Failed to open serial port log file!");
            return NULL;
        }
    }
    
    // now the serial port
    sp->sp_fd = open (serialportname, O_RDWR | O_NOCTTY );
    if (sp->sp_fd == -1) {
        // erexit ("can not open serial port!\n");
        return NULL;
    }

    if (tcgetattr(sp->sp_fd, &term) != 0) {
        // erexit("getattr failed!");
        return NULL;
    }
    // set BAUD rate
    if (cfsetospeed(&term, serial_speed) != 0) {
        fprintf(stderr, "SEROpenPort(): Failed to set out speed\n");
    }
    if (cfsetispeed(&term, serial_speed) != 0) {
        fprintf(stderr, "SEROpenPort(): Failed to set out speed\n");
    }
    cfmakeraw(&term);

	term.c_iflag |= IGNBRK;
    // Set the serial port mode NOW
    if (tcsetattr(sp->sp_fd, TCSANOW, &term) != 0) {
        fprintf(stderr, "SEROpenPort(): Failed to set port attrs\n");
        // erexit("setattr failed!");
        return NULL;
    }
    
    if (sp->sp_logfp != NULL) {
        fprintf(sp->sp_logfp, "Serial port %s opened ok.\n", serialportname);
    }

    return sp;
}

/** Close and free a serial port.
 *  Cleans up, closes and deletes a serial port. If the port had a log file,
 *  close the file.
 *  @param sp serial port to be closed.
 */
void SERClosePort(SerialPort * sp)
{
    if (sp->sp_fd != -1) {
        close(sp->sp_fd);
    }
    if (sp->sp_logfp != NULL) {
        fclose(sp->sp_logfp);
    }
    free(sp);
}

/** Get a byte from a serial port.
 *  Read a byte from a serial port. This blocks if no data is available.
 *  @param sp serial port to read from.
 *  @return value of byte read.
 */
BYTE SERGetByte(SerialPort * sp) 
{
    BYTE byte;

    assert(sp != NULL);
    assert(sp->sp_fd != -1);

    read(sp->sp_fd, &byte, 1);
    return byte;
}

/** Put a byte to a serial port.
 *  Write a byte to a serial port. This blocks if the serial port is not
 *  ready to accept new data.
 *  @param sp serial port to write to.
 *  @param b value of byte to be written.
 */
void SERPutByte(SerialPort * sp, BYTE b)
{
    assert(sp != NULL);
    assert(sp->sp_fd != -1);

    write(sp->sp_fd, &b, 1);
}

/** Put a string of bytes to a serial port.
 *  Write a sequence of bytes to a serial port. This blocks if the serial
 *  port is not ready to accept new data.
 *  @param sp serial port to write to.
 *  @param s pointer to string of bytes to be written
 *  @return zero if write succeeded, non-zero otherwise.
 */
int SERPutString(SerialPort * sp, const char * s)
{
    int l;

    assert(sp != NULL);
    assert(sp->sp_fd != -1);

    l = strlen(s);
    if (write(sp->sp_fd, s, l) != l) {
        return -1;
    } else {
        return 0;
    }
}

/** Clear any bytes that arrive at a serial port for a period of time.
 *  Use select to wait for data to arrive at the serial port, and then
 *  read all data available.
 *  @param sp serial port to read from.
 *  @param usec maximum number of microseconds to wait if no data is available
 *  immediatly.
 */
void SERFlushChannel(SerialPort * sp, int usec)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    assert(sp->sp_fd != -1);

    for (;;) {
        FD_ZERO(&rfds);
        FD_SET(sp->sp_fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = usec;

        retval = select(sp->sp_fd + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select");
            return;
        } else if (retval == 0) {
            debug( printf("Done flushing serial channel\n"); );
            return;
        } else {
            char buf[256];
            debug( printf("Reading unwanted data\n"); );
            read(sp->sp_fd, buf, 256);
        }
    }
}

/** Test whether there is data waiting on a serial port.
 *  Use select to wait for data to be available at the serial port for a
 *  certain period of time.
 *  @param sp serial port to check.
 *  @param usec maximum time in microseconds to wait for data.
 *  @return one if data is now available, zero otherwise.
 */
int SERQueryChannel(SerialPort * sp, int usec)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    assert(sp->sp_fd != -1);

    FD_ZERO(&rfds);
    FD_SET(sp->sp_fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = usec;

    retval = select(sp->sp_fd + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
        perror("select");
        return 0;
    } else if (retval == 0) {
        debug( printf("No data on channel\n"); );
        return 0;
    } else {
        // debug( printf("Some data on channel\n"); );
        return 1;
    }
}


/** Read a byte from a serial port with a timeout.
 *  Wait at most usec microseconds for data to be available on the
 *  given serial port, and then read one byte if it is available.
 *  @param sp serial port to read from.
 *  @param usec number of microseconds to wait.
 *  @return value of byte read, or -1 if a timeout occured.
 */
int SERGetByteTimeout(SerialPort * sp, int usec)
{
        assert(sp->sp_fd != -1);

        if (SERQueryChannel(sp, usec))
        {
                return SERGetByte(sp);
        }
        else
        {
                debug( printf("TIMEOUT"); );
                return -1;
        }
}

/** Read some bytes from a serial port with a timeout.
 *  Wait at most usec microseconds for data to be available on the
 *  given serial port, and then read some bytes if it they are available.
 *  This is repeated until enough bytes have been read. 
 *  @param sp serial port to read from.
 *  @param buffer pointer to buffer where bytes are stored.
 *  @param count number of bytes to read.
 *  @param usec number of microseconds to wait.
 *  @return number of bytes read.
 */
int SERGetBytesTimeout(SerialPort * sp, BYTE * buffer, int count, int usec)
{
	int ret, done = 0;

	assert(sp->sp_fd != -1);

	while (count) {
		if (!SERQueryChannel(sp, usec))
			return done;
		else {
			ret = read(sp->sp_fd, buffer, count);
			if (ret < 1) {
				debug( printf("read error"); );
				return done;
			}
			buffer += ret;
			done += ret;
			count -= ret;
		}
	}
	return done;
}

void SERClearIncoming( SerialPort *sp )
{
	assert(sp->sp_fd != -1);

	SERFlushChannel( sp, 0 );

	/* Flush the serial port input buffer */
	tcflush( sp->sp_fd, TCIFLUSH );
}
