/**
 * Glacsweb gwgsm.c
 * main prog for talking to GSM unit
 * Copyright (C) 2003, 2004 Kirk Martinez, Alistair Riddoch,
 *                          The University of Southampton
 */


#include "gsm.h"
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <unistd.h>

#define debug(prg) { if (debug_flag) { prg } }

static const int debug_flag = 0;

//-------------------- INITIALISE ------------------
static SerialPort * initialise(const char * port, speed_t baud)
{
    SerialPort * sp;
    LOGWrite(GWL_DEBUG, "Initialise gwgsm.");

    // now the serial port
    sp = SEROpenPort(port, baud, NULL);
    if (sp == NULL) {
        LOGWrite(GWL_FATAL, "Can not open serial port!");
        return NULL;
    }

    SERFlushChannel(sp, 100000);

    /* Send a couple of newlines to wake up the translators and/or modem! */
    GSMWakeUp(sp);

    GSMEchoOn(sp);

    LOGWrite(GWL_DEBUG, "Initialise gwgsm complete.");

    return sp;
}


//-------------------- USAGE ------------------------
static void usage(const char * prgname)
{
    fprintf(stderr, "Usage: %s  [-b <baud_rate>] [-p <serialport>] {check|message|send} ... \n\n", prgname);
    fprintf(stderr, "  -d                debug, write messages to files\n");
    fprintf(stderr, "  -b <baud_rate>    set the serial baud rate\n");
    fprintf(stderr, "  -p <serialport>   set the serial port device\n\n");
    fprintf(stderr, "     check          check that the modem is associated\n");
    fprintf(stderr, "                    with a network, and has enough\n");
    fprintf(stderr, "                    signal to send messages.\n"
	            "     check-gprs     check that the modem is associated\n"
	            "                    with a GPRS network, and force attachment.\n");
    fprintf(stderr, "     message        send a command line message\n");
    fprintf(stderr, "     send           send a file as a sequence of  messages\n\n");

}

static void usage_check(const char * prgname)
{
    fprintf(stderr, "Usage: %s  [-b <baud_rate>] [-p <serialport>] check\n", prgname);
}

static void usage_message(const char * prgname)
{
    fprintf(stderr, "Usage: %s  [-b <baud_rate>] [-p <serialport>] message <number> <message>\n", prgname);
}

static void usage_send(const char * prgname)
{
    fprintf(stderr, "Usage: %s  [-b <baud_rate>] [-p <serialport>] send <number> <file> \n", prgname);
}


//-------------------- MAIN ------------------------
int main (int argc, char **argv) 
{
    const char * option_serialport = "/dev/gprs";
    const char * cmd;
    SerialPort * sp;
    speed_t option_baud = B9600;
    int option_debug = 0;

    LOGInit(GWT_STDERR, GWL_INFO, "Glacsweb GSM");

    while (1) {
        char mesg[1024]; // For logfile
        int c = getopt(argc, argv, "p:b:d");
        if (c == -1) {
            break;
        } else if (c == 'p') {
            int ret;
            struct stat sbuf;
            debug( printf("Got port %s.\n", optarg); );
            option_serialport = optarg;
            ret = stat(option_serialport, &sbuf);
            if (ret == -1) {
                sprintf(mesg, "%s does not exist\n", option_serialport);
                LOGWrite(GWL_ERROR, mesg);
            } else if (!S_ISCHR(sbuf.st_mode)) {
                sprintf(mesg, "%s is not a device\n", option_serialport);
                LOGWrite(GWL_ERROR, mesg);
            }
        } else if (c == 'b') {
            debug( printf("Got baud %s.\n", optarg); );
            if (SERGetBaud(optarg, &option_baud)) {
                sprintf(mesg, "Unknown baud rate %s", optarg);
                LOGWrite(GWL_ERROR, mesg);
            }
        } else if (c == 'd') {
            debug( printf("Got debug flag.\n"); );
            option_debug = 1;
        }
    }

    if ((argc - optind) < 1) {
        usage(argv[0]);
        return 1;
    }

    if (option_debug != 0) {
        GSMDebugMode();
    }

    // set up rs232
    sp = initialise(option_serialport, option_baud);
    
    if (sp == NULL) {
        return 1;
    }

    cmd = argv[optind];

    if (strcmp(cmd, "send") == 0) {
        int status;

        LOGWrite(GWL_DEBUG, "Performing send command");

        if ((argc - optind) != 3) {
            usage_send(argv[0]);
            return 1;
        }

        if (GSMSetSMSMode(sp) != 0) {
            LOGWrite(GWL_ERROR, "Unable to set SMS mode");
            return 1;
        }

        status = GSMWaitSignal(sp, 5);
        if ((status < 0) || (status == 1)) {
            LOGWrite(GWL_ERROR, "Modem not able to send");
            return 1;
        }
        
        if (GSMSendFile(sp, argv[optind + 1], argv[optind + 2]) == 0) {
            return 0;
        }

        LOGWrite(GWL_ERROR, "GSM file sending failed");
        return 1;
    } else if (strcmp(cmd, "check") == 0) {
        int status;

        LOGWrite(GWL_DEBUG, "Performing check command");

        if ((argc - optind) != 1) {
            usage_check(argv[0]);
            return 1;
        }

        // Check the GSM signal and network association
        status = GSMCheckSignal(sp);
        if ((status < 0) || (status == 1)) {
            return 1;
        }
        return 0;
    } else if (strcmp(cmd, "message") == 0) {
        int status;

        LOGWrite(GWL_DEBUG, "Performing message command");

        if ((argc - optind) != 3) {
            usage_message(argv[0]);
            return 1;
        }

        if (GSMSetSMSMode(sp) != 0) {
            LOGWrite(GWL_ERROR, "Unable to set SMS mode");
            return 1;
        }

        status = GSMWaitSignal(sp, 5);
        if ((status < 0) || (status == 1)) {
            LOGWrite(GWL_ERROR, "Modem not ready to send");
            return 1;
        }

        status = GSMSendMessage(sp, argv[optind + 1], argv[optind + 2]);
    } else if (strcmp(cmd, "check-gprs") == 0) {
	    LOGWrite(GWL_DEBUG, "Performing check-gprs command");

	    if( GSMAttachGPRS(sp) )
		    return 1;	    

	    if( GSMCheckGPRS(sp) != 0 )
		    return 1;

	    return 0;
    }
    return 1;
}
