/*
 * Glacsweb gsm.h
 * Copyright (C) 2004 Alistair Riddoch, The University of Southampton
 */

#ifndef GLACSWEB_GSM_H
#define GLACSWEB_GSM_H

#include "serial.h"

char * GSMEncodeBytes(const BYTE * const data, size_t len);
BYTE * GSMDecodeBytes(const char * const data);

int GSMDebugMode();
int GSMEchoOn(SerialPort *);
int GSMCheckSignal(SerialPort *);
int GSMWaitSignal(SerialPort * , int retries);
int GSMSetSMSMode(SerialPort *);
int GSMSendMessage(SerialPort *, const char * const, const char * const);
int GSMSendBlock(SerialPort *, const char * const, const char * const,
                 int, const BYTE *, size_t);
int GSMSendFile(SerialPort *, const char * const, const char * const);

int GSMWakeUp(SerialPort *);

int GSMAttachGPRS(SerialPort *);
int GSMCheckGPRS(SerialPort *);

#endif /* GLACSWEB_GSM_H */
