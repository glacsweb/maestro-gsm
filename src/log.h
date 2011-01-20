/*
 * Glacsweb log.h
 * Copyright (C) 2004 Alistair Riddoch, The University of Southampton
 */

#ifndef GLACSWEB_LOG_H
#define GLACSWEB_LOG_H
#define _GNU_SOURCE
#include <stdio.h>

typedef enum log_targets {
                           GWT_SYSLOG = 1 << 0,
                           GWT_STDERR = 1 << 1,
                           GWT_FILE = 1 << 2
                         } LogTarget;

typedef enum log_level {
                         GWL_FATAL = 0,
                         GWL_ERROR = 1,
                         GWL_WARNING = 2,
                         GWL_INFO = 3,
                         GWL_VERBOSE = 4,
                         GWL_DEBUG = 5
                       } LogLevel;

/* Have to embed stringify... for complex CPP reasons! */
#define LOG_STRINGIFY2(x) #x
#define LOG_STRINGIFY(x) LOG_STRINGIFY2(x)

#define LOG_printf( level, format, ... ) do {				\
		char *msg = NULL;					\
		if( asprintf( &msg, format, ## __VA_ARGS__ ) == -1  ) {	\
			LOGWrite( GWL_DEBUG, "Out of memory in " __FILE__ " on line " LOG_STRINGIFY(__LINE__) ); \
		} else {						\
			LOGWrite( level, msg );				\
			free(msg);					\
		}							\
	} while (0)

/** Set the filename to be used by the logger */
void LOGFilename(const char *);

/** Get the filename being used by the logger */
const char *LOGFilename_get( void );

/** Initialise the log subsystem */
int  LOGInit(LogTarget, LogLevel max, const char * prefix);

/** Close down the log subsystem */
void LOGShutdown();

/** Set the maximum level */
void LOGSetLevel(LogLevel max);

/** Write a message to the log file */
void LOGWrite(LogLevel, const char *);

/** Put the date into a string suitable for filenames */
void LOGdate_string(char *str);

#endif /* GLACSWEB_LOG_H */
