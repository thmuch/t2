/* T2 - Logfile                           */
/* Copyright (c)1998-06-04 by Thomas Much */


#include <tos.h>

#include "logfile.h"


extern int t2debug;

int logbuflen,
    t2_logfile;

unsigned char logbuf[LOGBUFLEN];

char HEXARRAY[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};




void add_to_logbuf(unsigned char c)
{
	logbuf[logbuflen++] = c;
	
	if (logbuflen == LOGBUFLEN)
	{
		int  i;
		char hbuf[3];

		hbuf[2] = ' ';
		
		for (i=0;i<LOGBUFLEN;i++)
		{
			hbuf[0] = HEXARRAY[(logbuf[i] >> 4) & 0x0f];
			hbuf[1] = HEXARRAY[logbuf[i] & 0x0f];

			Fwrite(t2_logfile,3,&hbuf);
		}

		hbuf[0] = ' ';
		hbuf[1] = ' ';

		Fwrite(t2_logfile,3,&hbuf);
		Fwrite(t2_logfile,3,&hbuf);
		
		for (i=0;i<LOGBUFLEN;i++)
		{
			char esc = '.';
			
			if ((logbuf[i] < 32) || (logbuf[i] > 126))
				Fwrite(t2_logfile,1,&esc);
			else
				Fwrite(t2_logfile,1,&logbuf[i]);
		}

		hbuf[0] = 13;
		hbuf[1] = 10;
		Fwrite(t2_logfile,2,&hbuf);
		
		logbuflen = 0;
	}
}



void write_logfile(unsigned char *buf, long len)
{
	int i;

	if (t2_logfile != -1)
	{
		for(i=0;i<len;i++) add_to_logbuf(*buf++);
	}
}



void open_logfile(void)
{
	long ret;
	char buf[16];
	
	if (!t2debug)
	{
		t2_logfile = -1;
		return;
	}

	ret = Fopen(LOGFILE,FO_RW);
	
	if (ret < 0L)
	{
		ret = Fcreate(LOGFILE,0);
		
		if (ret < 0L)
		{
			t2_logfile = -1;
			return;
		}
	}
	
	t2_logfile = (int)ret;
	
	Fseek(0,t2_logfile,2);
	
	buf[0]=13;
	buf[1]=10;
	buf[2]='-';
	buf[3]='-';
	buf[4]='-';
	buf[5]='-';
	buf[6]='-';
	buf[7]='-';
	buf[8]='-';
	buf[9]='-';
	buf[10]='-';
	buf[11]='-';
	
	Fwrite(t2_logfile,12,&buf);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,10,&buf[2]);
	Fwrite(t2_logfile,2,&buf);
	Fwrite(t2_logfile,2,&buf);
	
	logbuflen = 0;
}



void close_logfile(void)
{
	if (t2_logfile != -1)
	{
		char hbuf[2];

		while (logbuflen) add_to_logbuf(0);

		hbuf[0] = 13;
		hbuf[1] = 10;
		Fwrite(t2_logfile,2,&hbuf);

		Fclose(t2_logfile);
	}
}
