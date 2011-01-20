/**
 * Glacsweb gwgsm.c
 * simply do AT and expect OK from GSM modem
 * Copyright (C) 09 Kirk Martinez, Philip Basford (in Iceland)
 *                          The University of Southampton
 * DONT TRY THIS AT HOME! oonly on base with loads of execs ready!
 */


#include "gsm.h"
#include "log.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <unistd.h>
/* max num of char reads to try
*/
#define MAXATTEMPTS 10
#define MAXRESTARTCOUNT 3
#define debug(prg) { if (debug_flag) { prg } }

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

    return sp;
}

int main (int argc, char **argv) 
{
    const char * option_serialport = "/dev/gprs";
    SerialPort * sp;
    speed_t option_baud = B57600;
	int b;
	int count = MAXATTEMPTS;
	int restartcount =  MAXRESTARTCOUNT;

    LOGInit(GWT_STDERR, GWL_INFO, "Glacsweb gsmat");


    // set up rs232
    sp = initialise(option_serialport, option_baud);
    
    if (sp == NULL) {
        return 1;
    }


if( SERPutString(sp,"at\n\r") == 0) {
	while(restartcount--){
		while(count--){
			b = SERGetByteTimeout(sp,2000000);
			if( b == -1){
				printf("timed out\n");
				if( SERPutString(sp,"at\n") == 0) {
					printf("put AT\n");
				}else{
					printf("put failed\n");
				}
			}else{
				printf("got: %d after %d attempts %d powercycles\n", b, MAXATTEMPTS - count, MAXRESTARTCOUNT - restartcount);
				SERFlushChannel(sp,100000);
				return(0);
				}
			}
		system("/home/root/scripts/gprs-off");
		usleep(5000000);
		system("/home/root/scripts/gprs-on");
		usleep(30000000);
		}
	}
else {
	printf("failed to send AT\n");
	return(-2);
	}
return(1);
}
