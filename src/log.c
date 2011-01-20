/*
 * Glacsweb log.c
 */
/** \file
 * Logging functions, with levels, multiple output channels, and syslog.
 * to use it you must initialise first, then log messages to specific output
 *
 * Copyright (C) 2008 Alistair Riddoch & Robert Spanton The University of Southampton
 */
/* For asprintf */
#define _GNU_SOURCE
#include "log.h"
#include "log_files.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <assert.h>

/** Run debug code if debugging is enabled. */
#define debug(prg) { if (debug_flag) { prg } }

/** Control whether debug code is run.
 *  Set to non-zero if debug code should be run.
 */
static const int debug_flag = 0;

/** Flag indicating whether the log system has been initialised.
 *  Set to non-zero when the log system is successfully started up.
 */
static int log_initialised = 0;

/** Bitmask defining which log systems messages should be sent to. */
static LogTarget log_targets = 0;

/** Level indicating the maximum level of messages that should be logged.
 *  Messages above this level (of lower priority) are discarded.
 */
static LogLevel log_level = 2;

/** Filename of the file currently being used for logging. */
static const char * log_fname = NULL;
/** Prefix used when writing log messages to indicate their source. */
static const char * log_prefix = "Glacsweb";

/** File pointer of the file currently being used for logging. */
static FILE * log_fileptr;

/* Returns 1 if the log "debug" file exists (/tmp/gwlog), 0 otherwise */
static int LOGDebug_file_exists( void );

/** Set the filename of the file to be used for logging.
 *  Must be called before LOGInit if file logging is enabled.
 *  @param filename string containing the name of the file to be as a log.
 */
void LOGFilename(const char * filename)
{
    assert(log_initialised == 0);
    
    log_fname = filename;
}

/** Initialise the log subsystems.
 *  Initialise the log targets indicated, and set the maximum level
 *  and log prefix. If logging to file is required then the filename
 *  must have already been set using LOGFilename().
 *  If an error occurs, logging is still enabled, but those targets
 *  which encountered an error will not be enabled.
 *  @param lt bitmask defining which targets to set up.
 *  @param max maximum log level which should be logged.
 *  @param prefix to be used when writing log messages to indicate their source.
 *  @return zero if initialisation is successful, non-zero otherwise.
 */
int LOGInit(LogTarget lt, LogLevel max, const char * prefix)
{
    int ret = 0;

    assert(log_initialised == 0);

    log_targets = lt;
    log_level = max;
    log_prefix = prefix;

    if ((log_targets & GWT_SYSLOG) == GWT_SYSLOG) {
        debug(fprintf(stderr, "Enabling logging to syslog\n"););

        openlog(log_prefix, LOG_PID, LOG_DAEMON);
    }

    if ((log_targets & GWT_STDERR) == GWT_STDERR) {
        debug(fprintf(stderr, "Enabling logging to stderr\n"););
    }

    if ((log_targets & GWT_FILE) == GWT_FILE) {
        debug(fprintf(stderr, "Enabling logging to file\n"););

	if( log_fname == NULL ) {
		char yymmdd[16];
		LOGdate_string(yymmdd);

		if( asprintf( &log_fname, LOG_DIR "/%s-log", yymmdd ) == -1 ) {
			fprintf( stderr, "Out of memory\n" );
			return -1;
		}
		debug( fprintf(stderr, "Using log file: %s\n", log_fname); );
	}

        log_fileptr = fopen(log_fname, "a");
        if (log_fileptr == NULL) {
            log_targets &= ~GWT_FILE;
            ret = -1;
        }
    }

    log_initialised = 1;

    return ret;
}

/** Shut down the log subsystems.
 *  Close any of the log targets which need to be closed.
 */
void LOGShutdown()
{
    assert(log_initialised != 0);

    if ((log_targets & GWT_SYSLOG) == GWT_SYSLOG) {
        closelog();
    }

    if ((log_targets & GWT_STDERR) == GWT_STDERR) {
    }

    if ((log_targets & GWT_FILE) == GWT_FILE) {
        assert(log_fileptr != NULL);
        fclose(log_fileptr);
    }
    
    log_initialised = 0;
}

/** Change the maximum level of messages that should be logged.
 *  The maximum level can be changed only after logging has been
 *  initialised.
 *  @param max maximum log level which should be logged.
 */
void LOGSetLevel(LogLevel max)
{
    assert(log_initialised != 0);

    log_level = max;
}

/** Get a string representing the given log level.
 *  @param ll log level which this message is at.
 *  @return string representing log level.
 */
static const char * level_name(LogLevel ll)
{
    switch(ll) {
      case GWL_FATAL:
        return "FATAL";
        break;
      case GWL_ERROR:
        return "ERROR";
        break;
      case GWL_WARNING:
        return "WARNING";
        break;
      case GWL_INFO:
        return "INFO";
        break;
      case GWL_VERBOSE:
        return "VERBOSE";
        break;
      case GWL_DEBUG:
        return "DEBUG";
        break;
      default:
        return "UNKNWON";
        break;
    }
}

/** Send a message to the log system.
 *  If the level given is less than or equal to the maximum level,
 *  the message will be logged to the enabled log targets.
 *  @param ll log level which this message is at.
 *  @param msg contents of log message.
 */
void LOGWrite(LogLevel ll, const char * msg)
{
    char t_str[64];
    time_t t;
    struct tm lt;

    if (log_initialised == 0)
        return;

    if (ll > log_level)
        return;

    if ((log_targets & GWT_SYSLOG) == GWT_SYSLOG)
        syslog(LOG_INFO, msg);

    t = time(NULL);
    localtime_r( &t, &lt );
    strftime( t_str, 64, "%Y-%m-%d %T", &lt );

    /* If the environment variable GWLSTDOUT is set, log will go to stdout unless we're already logging to stderr */
    if ((getenv("GWLSTDOUT") != NULL) && !(log_targets & GWT_STDERR))
      fprintf(stdout, "%s: %s: %s\n", log_prefix, level_name(ll), msg);

    if ((log_targets & GWT_STDERR) == GWT_STDERR
	|| LOGDebug_file_exists() )
        fprintf(stderr, "%s: %s: %s\n", log_prefix, level_name(ll), msg);

    if ((log_targets & GWT_FILE) == GWT_FILE) {
        assert(log_fileptr != NULL);

        fprintf(log_fileptr, "%s %s: %s: %s\n", t_str, log_prefix, level_name(ll), msg);
        fflush(log_fileptr);
    }
}

/** helpful function to make a date prefix for files
* @param pointer to string where date will be put
*/
void LOGdate_string(char * str)
{
	struct timeval timeCalc;
	struct tm *T;
	int d, m, y, h, mins;

	gettimeofday(&timeCalc, NULL);
	T = localtime(&timeCalc.tv_sec);

	d = T->tm_mday;
	/* Month returned is Jan=0 so we have to add one! */
	m = T->tm_mon + 1;
	y = T->tm_year - 100;
	mins = T->tm_min ;
	h = T->tm_hour ;
	sprintf(str, "%.2d%.2d%.2d%.2d%.2d", y, m, d, h, mins);
}

const char *LOGFilename_get( void )
{
	return log_fname;
}

static int LOGDebug_file_exists( void )
{
	if( access( "/tmp/gwlog", F_OK ) == 0 )
		/* Exists */
		return 1;
	else
		return 0;
}
