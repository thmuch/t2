/* T2 Logfile Header                      */
/* Copyright (c)1998-06-04 by Thomas Much */

#ifndef T2LOGFILE_H
#define T2LOGFILE_H


#define LOGFILE   "telnet.log"
#define LOGBUFLEN 20



void open_logfile(void);
void write_logfile(unsigned char *buf, long len);
void close_logfile(void);


#endif
