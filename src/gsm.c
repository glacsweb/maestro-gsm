/** \file gsm.c
 * Library functions to talk to the GSM modem.
 * 
 * Copyright (C) 2004 Alistair Riddoch, The University of Southampton
 *
 */

#include "gsm.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define debug(prg) { if (debug_flag) { prg } }

static const int debug_flag = 0;

/** Read a CR LF terminated line from the serial port.
 *  The CR LF characters are stripped, and the buffer is NULL terminated
 *  so the result can be used as a string.
 *
 *  @param sp serial port to read from
 *  @param buffer buffer to store the line in
 *  @param buflen size of buffer to read data into
 *  @return the number of bytes read, or minus one if an error or timeout
 *  occured.
 */
static int get_line(SerialPort * sp, char * const buffer, int buflen)
{
    int state = 0;
    int count = 0;

    assert(sp != NULL);
    assert(buffer != NULL);

    for (;;) {
        int c = SERGetByteTimeout(sp, 900000);
        debug( printf("0x%x,'%c'\n", c, c); );
        if (c == -1) {
            LOGWrite(GWL_DEBUG, "Timout\n");
            return -1;
        } else if (c == '\r') {
            if (state != 0) {
                LOGWrite(GWL_DEBUG, "Unexpected <CR>");
                return -1;
            }
            buffer[count] = 0;
            state = 1;
        } else if (c == '\n') {
            if (state != 1) {
                LOGWrite(GWL_DEBUG, "Unexpected <LF>");
                return -1;
            }
            debug( printf("EOL\n"); );
            return count;
        } else {
            if (state != 0) {
                LOGWrite(GWL_DEBUG, "Normal character after <CR>");
                return -1;
            }
            buffer[count++] = c;
            assert(count <= buflen);
            if (count == buflen) {
                LOGWrite(GWL_DEBUG, "Overflow.");
                return -1;
            }
        }
    }
}

static int debug_mode = 0;

int GSMDebugMode()
{
    debug_mode = 1;
    return 0;
}

/** Message to turn on echoing of commands */
static const char * const	E1_MESSAGE		= "ATE1\r\n";

/** Send the AT command which enables echo on the GSM modem. This ensures
 *  that all future commands are echoed as expected.
 */
int GSMEchoOn(SerialPort * sp)
{
    char linebuf[256];
    int count;

    if (debug_mode) {
        return 0;
    }

    SERPutString(sp, E1_MESSAGE);
    get_line(sp, linebuf, 256); // Either blank, or echoed command
    get_line(sp, linebuf, 256); // Blank
    count = get_line(sp, linebuf, 256); // OK Response

    SERFlushChannel(sp, 100000);

    if (strncmp(linebuf, "OK", 2) != 0) {
        LOGWrite(GWL_ERROR, "Failed to enable ECHO mode.");
        return 1;
    }

    return 0;
}

/** Send a command to the GSM modem, and listen for the echoed response.
 *  @return zero if the message was echoed back correctly, and one otherwise.
 */
int GSMSendCommand(SerialPort * sp, const char * const msg)
{
    char linebuf[256];
    int count;

    SERPutString(sp, msg);

    count = get_line(sp, linebuf, 256);

    if (count <= 0) {
        return 1;
    }

    if (strncmp(linebuf, msg, strlen(linebuf)) != 0) {
        LOGWrite(GWL_ERROR, "Message was not echoed correctly.");
        return 1;
    }
    return 0;
}

/** Message to read network registration status */
static const char * const	CREG_MESSAGE		= "AT+CREG?\r\n";
/** Prefix of reported network registration status message */
static const char * const	CREG_MESSAGE_RES	= "+CREG: ";
/** Minimum length of reported network registration status message */
static const int		CREG_MESSAGE_RES_LEN	= 10;

/** Message to read signal strength */
static const char * const	CSQ_MESSAGE		= "AT+CSQ\r\n";
/** Prefix of reported signal strength message */
static const char * const	CSQ_MESSAGE_RES		= "+CSQ: ";
/** Minimum length of reported signal strength message */
static const int		CSQ_MESSAGE_RES_LEN	= 9;

/** Message to put modem into SMS mode */
static const char * const	CMGF_MESSAGE		= "AT+CMGF=1\r\n";

/** Message format to send SMS message */
static const char * const	CMGS_MESSAGE		= "AT+CMGS=%s\r\n";

/** Send an SMS message using the GSM modem.
 *  The message to be sent shall be less than 171 bytes long.
 *  @return zero if the message is sent successfully, non-zero otherwise.
 */
int GSMSendMessage(SerialPort * sp, const char * const number,
                   const char * const msg)
{
    char cmd[256];
    char buf[256];
    int count;

    assert(sp != NULL);
    assert(number != NULL);
    assert(msg != NULL);

    if (strlen(msg) > 170) {
        LOGWrite(GWL_ERROR, "Message is too long.");
        return 1;
    }

    if (strlen(number) > 80) {
        LOGWrite(GWL_ERROR, "Phone number is ludicrously long.");
        return 1;
    }

    debug(fprintf(stderr, "Sending message to number %s with text \"%s\"\n",
                          number, msg););

    // ?? Not sure why this is here pjb08r 02/13
    if (debug_mode) {
        return 0;
    }

    sprintf(cmd, CMGS_MESSAGE, number);
    GSMSendCommand(sp, cmd);

    // blank line?
    get_line(sp, buf, 256);

    count = SERGetBytesTimeout(sp, buf, 2, 50000);
    if (count != 2) {
        LOGWrite(GWL_ERROR, "Error waiting for message prompt.");
        return 1;
    }
    debug( fprintf(stderr, "0x%x, 0x%x\n", buf[0], buf[1]); );
    if ((buf[0] != '>') || (buf[1] != ' ')) {
        LOGWrite(GWL_ERROR, "Did not get message prompt.");
        return 1;
    }

    SERPutString(sp, msg);
    SERPutByte(sp, 0x1a);

    // Read the messsage back, including any prompts.
    SERGetBytesTimeout(sp, buf, 256, 500000);

    sleep(1);

    get_line(sp, buf, 256);

    return 0;
}

char * GSMEncodeBytes(const BYTE * const data, size_t len)
{
    char * text;
    char * cptr;
    int i;

    text = calloc(len * 2 + 1, sizeof(char));

    if (text == NULL) {
        return NULL;
    }

    for (cptr = text, i = 0; i < len; ++i, cptr += 2) {
        snprintf(cptr, 3, "%2.2x", data[i]);
    }

    return text;
}

BYTE * GSMDecodeBytes(const char * const text)
{
// Don't need this functionaloty in Norway, so I'll postpone.
#if 0
    BYTE * data;
    size_t len;

    len = strlen(text)

    if ((len % 2) != 0) {
        LOGWrite(GWL_ERROR, "Text should be multiple of 2 long.");
        return NULL;
    }

    data = calloc(len / 2, sizeof(BYTE));

    for
#endif
    return NULL;
}

/** Check the network association and signal strength of the GSM modem.
 *
 * @param sp pointer to serial port to be used to talk to the modem.
 * @return zero if the modem is associated with a GSM network, and has
 * good enough signal strength to send messages, or 1 if GSM network
 * is not available, 2 if GSM signal strength is too weak, or -1 if
 * an error occured talking to the modem.
 */
int GSMCheckSignal(SerialPort * sp)
{
    char linebuf[256];
    char * sptr = NULL;
    int count;
    int status, signal;

    assert(sp != NULL);

    if (debug_mode) {
        return 0;
    }

    GSMSendCommand(sp, CREG_MESSAGE);

    get_line(sp, linebuf, 256);
    count = get_line(sp, linebuf, 256);

    if (count < CREG_MESSAGE_RES_LEN) {
        LOGWrite(GWL_ERROR, "Network registration response short\n");
        SERFlushChannel(sp, 50000);
        return -1;
    }
    assert(strlen(linebuf) < 256);
    if (strncmp(linebuf, CREG_MESSAGE_RES, strlen(CREG_MESSAGE_RES)) != 0) {
        LOGWrite(GWL_ERROR, "Network registration response does not match expected.");
        SERFlushChannel(sp, 50000);
        return -1;
    }
    sptr = strchr(linebuf, ',');
    if (sptr == NULL) {
        LOGWrite(GWL_ERROR, "',' not found in CREG message response.");
        SERFlushChannel(sp, 50000);
        return -1;
    }
    ++sptr;
    status = strtol(sptr, NULL, 10);
    debug( printf("Network status %d\n", status); );
    SERFlushChannel(sp, 50000);
    if ((status != 1) && (status != 5)) {
        LOGWrite(GWL_ERROR, "ERROR: Not registed with network.");
        return 1;
    }

    GSMSendCommand(sp, CSQ_MESSAGE);

    // Get blank line
    get_line(sp, linebuf, 256);
    count = get_line(sp, linebuf, 256);

    if (count < CSQ_MESSAGE_RES_LEN) {
        LOGWrite(GWL_ERROR, "Network signal response short.");
        SERFlushChannel(sp, 50000);
        return -1;
    }
    assert(strlen(linebuf) < 256);
    if (strncmp(linebuf, CSQ_MESSAGE_RES, strlen(CSQ_MESSAGE_RES)) != 0) {
        LOGWrite(GWL_ERROR, "Network signal response does not match expected.");
        SERFlushChannel(sp, 50000);
        return -1;
    }
    sptr = &linebuf[6];
    signal = strtol(sptr, NULL, 10);
    debug( printf("Signal strength %d\n", signal); );
    SERFlushChannel(sp, 50000);
    if (signal < 5) {
        LOGWrite(GWL_ERROR, "ERROR: Signal strength too weak.");
        return 2;
    }

    return 0;
}

/** Check the network association and signal strength of the GSM modem.
 *
 * @param sp pointer to serial port to be used to talk to the modem.
 * @return zero if the modem is associated with a GSM network, and has
 * good enough signal strength to send messages, or 1 if GSM network
 * is not available, 2 if GSM signal strength is too weak, or -1 if
 * an error occured talking to the modem.
 */
int GSMWaitSignal(SerialPort * sp, int retries)
{
    int i, res = 1;

    if (debug_mode) {
        return 0;
    }

    for (i = 0; i < retries; ++i) {
        res = GSMCheckSignal(sp);

        if (res < 1) {
            // Success or outright failure
            return res;
        }
        // Not associated with network, or low signal - try again

        sleep(5);
    }
    return res;
}

/** Put the GSM modem into a mode where it can send SMS messages.
 *  @return zero if the command was accepted by the modem, non-zero
 *  otherwise.
 */
int GSMSetSMSMode(SerialPort * sp)
{
    char linebuf[256];
    int count;

    if (debug_mode) {
        return 0;
    }

    assert(sp != NULL);

    GSMSendCommand(sp, CMGF_MESSAGE);

    get_line(sp, linebuf, 256);

    count = get_line(sp, linebuf, 256);
    if (count <= 0) {
        return 1;
    }
    if (strcmp(linebuf, "ERROR") == 0) {
        LOGWrite(GWL_ERROR, "Error from GSM modem setting SMS mode.");
        return 1;
    }
    if (strcmp(linebuf, "OK") == 0) {
        LOGWrite(GWL_DEBUG, "OK from GSM modem setting SMS mode.");
        return 0;
    }
    LOGWrite(GWL_ERROR, "Unknown response from GSM modem setting SMS mode.");
    return 1;
}

/** Message format for building an SMS message. Message contains a header
 *  line with filename and block number, plus up to two lines of
 *  data encoded as ASCII hex.
 */
static const char * const BINARY_MESSAGE_FORMAT = "%s %x\n%s\n%s\n";

/** Send a block of binary data as an SMS message.
 *  @param sp serial port used to communicate with the modem.
 *  @param number string giving the telephone number to be dialied.
 *  Must contain numerics and + (for international dialing) only.
 *  @param name to be used in the header.
 *  @param block_number to be used in the header.
 *  @param block pointer to binary data to be sent.
 *  @param len length of block to send. Must not exceed 64 bytes.
 */
int GSMSendBlock(SerialPort * sp, const char * const number,
                 const char * const name, int block_number,
                 const BYTE * block, const size_t len)
{
    char msg[181];
    char * line_one;
    char * line_two = "";
    int msg_len;
    int size_one = (len >= 32) ? 32 : len;
    int size_two = len - 32;

    assert(sp != NULL);
    assert(number != NULL);
    assert(name != NULL);
    assert(block_number > 0);
    assert(block != NULL);
    assert(len > 0);

    line_one = GSMEncodeBytes(block, size_one);
    if (line_one == NULL) {
        LOGWrite(GWL_FATAL, "Out of memory.");
        return 1;
    }

    if (len > 32) {
        line_two = GSMEncodeBytes(block + 32, size_two);
        if (line_two == NULL) {
            LOGWrite(GWL_FATAL, "Out of memory.");
            return 1;
        }
    }

    msg_len = snprintf(msg, 0, BINARY_MESSAGE_FORMAT, name, block_number,
                       line_one, line_two);
    if (msg_len >= 180) {
        LOGWrite(GWL_ERROR, "Header too long writing binary block");
        return 1;
    }

    snprintf(msg, 181, BINARY_MESSAGE_FORMAT, name, block_number, line_one,
             line_two);

    free(line_one);
    if (len > 32) {
        free(line_two);
    }

    if (debug_mode) {
        printf("%s", msg);
    }

    return GSMSendMessage(sp, number, msg);
}

/** Send the contents of a file as a sequence of SMS messages.
 *  @param sp serial port used to communicate with the modem.
 *  @param number string giving the telephone number to be dialied.
 *  Must contain numerics and + (for international dialing) only.
 *  @param filename name of the file containing the data to be sent.
 */
int GSMSendFile(SerialPort * sp, const char * const number,
                const char * const filename)
{
    FILE * fp;
    BYTE buffer[64];
    int done = 0;
    int ret = 0;
    int n = 0;

    assert(sp != NULL);

    fp = fopen(filename, "r");

    if (fp == NULL) {
        LOGWrite(GWL_ERROR, "Could not open file for sending.");
        return 1;
    }

    while (!done) {
        size_t len = fread(buffer, 1, 64, fp);

        if (len == 0) {
            if (ferror(fp) != 0) {
                LOGWrite(GWL_ERROR, "Error reading from file.");
                ret = 1;
            } else {
                ret = 0;
            }
            done = 1;
            
            continue;
        }

        LOGWrite(GWL_DEBUG, "Sending a block");
        ++n;
        debug( fprintf(stderr, "%dnth block is %d bytes\n", n, len); );
        if (GSMSendBlock(sp, number, filename, n, buffer, len) != 0) {
            LOGWrite(GWL_ERROR, "GSM error sending file");
            done = 1;
        }

        if (feof(fp) != 0) {
            ret = 0;
            done = 1;
        }
        
    };

    return ret;
}

static const char * const WAKE_UP_MESSAGE = "\r\n";

int GSMWakeUp(SerialPort * sp)
{
	SERPutString(sp,WAKE_UP_MESSAGE);
	SERFlushChannel(sp,100000);
	return 0;
}

static const char * const CGATT_MESSAGE = "AT+CGATT=1\r\n";

int GSMAttachGPRS(SerialPort *sp)
{
	char linebuf[256];
	int count;

	GSMSendCommand(sp, CGATT_MESSAGE);
	get_line( sp, linebuf, 256 );

	count = get_line( sp, linebuf, 256 );

	if( count <= 0 )
		return 1;

	if (strcmp(linebuf, "ERROR") == 0) {
		LOGWrite(GWL_ERROR, "Error from GSM modem attaching to GPRS.");
		return 1;
	}
	if (strcmp(linebuf, "OK") == 0) {
		LOGWrite(GWL_DEBUG, "OK from GSM modem attaching to GPRS.");
		return 0;
	}
	LOGWrite(GWL_ERROR, "Unknown response from GSM modem attaching to GPRS.");
	return 1;
}

static const char * const CGREG_MESSAGE = "AT+CGREG?\r\n";

int GSMCheckGPRS(SerialPort *sp)
{
	char linebuf[256];
	int count;
	char *sptr;

	GSMSendCommand(sp, CGREG_MESSAGE);
	get_line( sp, linebuf, 256 );

	/* Response should be at least this: */
	count = get_line( sp, linebuf, 256 );
	if( count < strlen("+CGREG: 0,0") )
	{
		LOGWrite(GWL_ERROR, "Response too short for CREG command.");
		return -1;
	}

	sptr = strchr(linebuf, ',');
	if( sptr[1] != '1' && sptr[1] != '5' )
		return 1;

	return 0;	
}
