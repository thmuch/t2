/* T2 Headerfile                          */
/* Copyright (c)1998-06-04 by Thomas Much */

#ifndef T2TELNET_H
#define T2TELNET_H


#define OPTION_TRANSBIN       0 /* todo */
#define OPTION_ECHO           1
#define OPTION_TERMINALTYPE  24
#define OPTION_NAWS          31

#define OPTION_SB_TTSEND      1
#define OPTION_SB_TTIS        0

#define STAT_NORMAL 0
#define STAT_IAC    1
#define STAT_WILL   2
#define STAT_DO     3
#define STAT_WONT   4
#define STAT_DONT   5

#define TC_SE    240
#define TC_NOP   241
#define TC_DATA  242
#define TC_BREAK 243
#define TC_INTR  244
#define TC_AO    245
#define TC_AYT   246
#define TC_EC    247
#define TC_EL    248
#define TC_GO    249
#define TC_SB    250
#define TC_WILL  251
#define TC_WONT  252
#define TC_DO    253
#define TC_DONT  254
#define TC_IAC   255

#define SENDBUFLEN    64
#define OPTIONBUFLEN 512

#define min(a,b) (((a)<(b))?(a):(b))

#define Con(a) Cconws(a)
#define crlf Con("\r\n")


#endif
