/* T2 - Telnet Terminal for IConnect      */
/* Copyright (c)1998-04-28 by Thomas Much */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tos.h>
#include <sockinit.h>
#include <socket.h>
#include <sockios.h>
#include <sfcntl.h>
#include <in.h>
#include <inet.h>
#include <types.h>
#include <netdb.h>
#include <atarierr.h>
#include <sockerr.h>
#include <ext.h>

#define T2COMPDATE   ((unsigned int) ((18 << 9) | (4 << 5) | 28))
#define T2BETAEXPIRE ((unsigned int) ((18 << 9) | (5 << 5) | 31))

#define OPTION_TRANSBIN       0 /**/
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

#define SENDBUFLEN   64
#define OPTIONBUFLEN 512

#define LOGFILE      "telnet.log"
#define LOGBUFLEN    20

#define min(a,b) (((a)<(b))?(a):(b))

#define Con(a) Cconws(a)
#define crlf Con("\r\n")


int   t2_sock,
      t2_port,
      t2_max_lines,
      t2_max_columns,
      t2_logfile,
      logbuflen,
      optionbuflen,
      remoteecho,
      debug = 0;
ulong t2_ip;

unsigned char sendbuf[SENDBUFLEN],
              logbuf[LOGBUFLEN],
              optionbuf[OPTIONBUFLEN];

char HEXARRAY[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};



void numout(long num)
{
	char nus[32];
	Con(ltoa(num, nus, 10));
}


ulong resolve(char *name)
{
	hostent	*he;

	he=gethostbyname(name);
	if(he)
	{
		if(!he->h_addr_list[0])
		{
			Con("host \'");Con(name);
			Con("\' was not resolved to an IP address.");
			crlf;
			return(INADDR_NONE);
		}
		else	
		{
			Con("IP: ");
			Con(inet_ntoa(*(ulong*)(he->h_addr_list[0]))); 
			crlf;
			return(*(ulong*)(he->h_addr_list[0]));
		}
	}

	Con("host \'");Con(name);
	Con("\' was not found.");
	crlf;
	return(INADDR_NONE);
}


int service(char *name)
{
	servent	*se;
	int			 d;
	
	if((d=atoi(name))==0)
	{
		se=getservbyname(name,"tcp");
		if (se==NULL) return(-1);

		d=se->s_port;
	}

	return(d);
}


void add_to_optionbuf(unsigned char c)
{
	if (optionbuflen < OPTIONBUFLEN) optionbuf[optionbuflen++]=c;
}


void t2send(int slen)
{
	long sret;

	if (!slen) return;

	sret = swrite(t2_sock,&sendbuf,slen);

	if (sret < 0)
	{
		Con("Error #");
		numout(sret); crlf;
	}
}


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
	
	if (!debug)
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


void init_terminal(void)
{
	char *p;
	
	t2_max_lines   = 0;
	t2_max_columns = 0;

	p = getenv("LINES");
	if (p) t2_max_lines = atoi(p);
	else
	{
		p = getenv("ROWS");
		if (p) t2_max_lines = atoi(p);
	}

	p = getenv("COLUMNS");
	if (p) t2_max_columns = atoi(p);
	
	if (t2_max_lines == 0)   t2_max_lines=24;
	if (t2_max_columns == 0) t2_max_columns=80;

	sendbuf[0]=TC_IAC;
	sendbuf[1]=TC_WILL;
	sendbuf[2]=OPTION_NAWS;
	t2send(3);
	
	remoteecho = 1;

	sendbuf[0]=TC_IAC;
	sendbuf[1]=TC_DO;
	sendbuf[2]=OPTION_ECHO;
	t2send(3);
}


void loop(void)
{
	int  instat = STAT_NORMAL,
	     optionactive = 0;
	long rawkey,
	     anzread;
	unsigned char buf[256];

	optionbuflen = 0;

	for (;;)
	{
		anzread = sread(t2_sock,&buf,256);
		
		if (anzread > 0L)
		{
			int i;
			
			write_logfile(&buf[0],anzread);

			for(i=0;i<anzread;i++)
			{
				switch (instat)
				{
				case STAT_WONT:
					switch (buf[i])
					{
					case OPTION_ECHO:
						if (remoteecho)
						{
							sendbuf[0]=TC_IAC;
							sendbuf[1]=TC_DONT;
							sendbuf[2]=OPTION_ECHO;
							t2send(3);
							remoteecho = 0;
						}
						break;
					}
					instat=STAT_NORMAL;
					break;
				case STAT_DONT:
					switch (buf[i])
					{
					case OPTION_ECHO:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_WONT;
						sendbuf[2]=OPTION_ECHO;
						t2send(3);
						break;
					}
					instat=STAT_NORMAL;
					break;
				case STAT_WILL:
					switch (buf[i])
					{
					case OPTION_ECHO:
						if (!remoteecho)
						{
							sendbuf[0]=TC_IAC;
							sendbuf[1]=TC_DO;
							sendbuf[2]=OPTION_ECHO;
							t2send(3);
							remoteecho = 1;
						}
						break;
					default:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_DONT;
						sendbuf[2]=buf[i];
						t2send(3);
					}
					instat=STAT_NORMAL;
					break;
				case STAT_DO:
					switch (buf[i])
					{
					case OPTION_ECHO:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_WONT;
						sendbuf[2]=OPTION_ECHO;
						t2send(3);
						break;
					case OPTION_TERMINALTYPE:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_WILL;
						sendbuf[2]=OPTION_TERMINALTYPE;
						t2send(3);
						break;
					case OPTION_NAWS:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_SB;
						sendbuf[2]=OPTION_NAWS;
						sendbuf[3]=0;
						sendbuf[4]=min(t2_max_columns,254);
						sendbuf[5]=0;
						sendbuf[6]=min(t2_max_lines,254);
						sendbuf[7]=TC_IAC;
						sendbuf[8]=TC_SE;
						t2send(9);
						break;
					default:
						sendbuf[0]=TC_IAC;
						sendbuf[1]=TC_WONT;
						sendbuf[2]=buf[i];
						t2send(3);
					}
					instat=STAT_NORMAL;
					break;
				case STAT_IAC:
					switch(buf[i])
					{
					case TC_SB:
						optionbuflen = 0;
						optionactive = 1;
						instat=STAT_NORMAL;
						break;
					case TC_SE:
						if (optionbuflen)
						{
							switch(optionbuf[0])
							{
							case OPTION_TERMINALTYPE:
								if (optionbuf[1] == OPTION_SB_TTSEND)
								{
									sendbuf[0]=TC_IAC;
									sendbuf[1]=TC_SB;
									sendbuf[2]=OPTION_TERMINALTYPE;
									sendbuf[3]=OPTION_SB_TTIS;
									sendbuf[4]='D';
									sendbuf[5]='E';
									sendbuf[6]='C';
									sendbuf[7]='-';
									sendbuf[8]='V';
									sendbuf[9]='T';
									sendbuf[10]='5';
									sendbuf[11]='2';
									sendbuf[12]=TC_IAC;
									sendbuf[13]=TC_SE;
									t2send(14);
								}
								break;
							}
						}
						optionactive = 0;
						instat=STAT_NORMAL;
						break;
					case TC_AYT:
						sendbuf[0]='[';
						sendbuf[1]='Y';
						sendbuf[2]='e';
						sendbuf[3]='s';
						sendbuf[4]=']';
						sendbuf[5]=13;
						sendbuf[6]=10;
						t2send(7);
						instat=STAT_NORMAL;
						break;
					case TC_WILL:
						instat=STAT_WILL;
						break;
					case TC_DO:
						instat=STAT_DO;
						break;
					case TC_WONT:
						instat=STAT_WONT;
						break;
					case TC_DONT:
						instat=STAT_DONT;
						break;
					case TC_IAC:
						if (optionactive) add_to_optionbuf(255);
						instat=STAT_NORMAL;
						break;
					default:
						instat=STAT_NORMAL;
					}
					break;
				default:
					{
						if (buf[i]==TC_IAC) instat=STAT_IAC;
						else if (optionactive) add_to_optionbuf(buf[i]);
						else if (buf[i]<127) Cconout(buf[i]);
					}
				}
			}
		}
		else if (anzread < 0L)
		{
			if (anzread != (long)ECONNRESET)
			{
				Con("Error #");
				numout(anzread); crlf;
			}
			
			break;
		}

		rawkey = Crawio(0x00ff);
		
		if (rawkey != 0L)
		{
			unsigned char key;
			int           i = 0;
			
			while ((rawkey != 0L) && (i < SENDBUFLEN-2))
			{
				key = (rawkey & 0x000000ffL);

				switch (key)
				{
				case 13:
					sendbuf[i++]=13;
					sendbuf[i++]=10;
					break;
				case '„':
					sendbuf[i++]='a';
					sendbuf[i++]='e';
					break;
				case '”':
					sendbuf[i++]='o';
					sendbuf[i++]='e';
					break;
				case '':
					sendbuf[i++]='u';
					sendbuf[i++]='e';
					break;
				case 'Ž':
					sendbuf[i++]='A';
					sendbuf[i++]='e';
					break;
				case '™':
					sendbuf[i++]='O';
					sendbuf[i++]='e';
					break;
				case 'š':
					sendbuf[i++]='U';
					sendbuf[i++]='e';
					break;
				case 'ž':
					sendbuf[i++]='s';
					sendbuf[i++]='s';
					break;
				default:
					{
						if (key<127)
						{
							sendbuf[i++]=key;
						}
					}
				}
				
				rawkey = Crawio(0x00ff);
			}
			
			if (!remoteecho)
			{
				int q;
				
				for (q=0; q<i; q++) Cconout(sendbuf[q]);
			}

			t2send(i);
		}
	}
}


int main(int argc, char *argv[])
{
	int          err,i,hostarg = 0;
	sockaddr_in  sa;

	Con("T2 - Telnet Terminal for IConnect (version 1998-04-28)"); crlf; Con("Copyright (c)1998 Thomas Much, Application Systems Heidelberg Software GmbH");crlf;
	Con("Beta version. Expires 1998-05-31."); crlf; crlf;
	
	if (Tgetdate() < T2COMPDATE)
	{
		Con("Please set your computer clock to the current date and time."); crlf;
		return(0);
	}
	
	if (Tgetdate() > T2BETAEXPIRE)
	{
		Con("Beta version has expired..."); crlf;
		return(0);
	}
	
	if (argc < 2)
	{
		Con("No host specified (command line argument missing)");crlf;
		return(0);
	}

	for(i=1;i<argc;i++)
	{
		if (*argv[i] != '-') hostarg=i;
		else
		{
			if (!strcmp(argv[i],"-d")) debug=1;
		}
	}
	
	if (!hostarg)
	{
		Con("No host specified");crlf;
		return(0);
	}
	
	err=sock_init();
	if(err < 0)
	{
		switch(err)
		{
			case SE_NINSTALL:
				Con("Sockets not installed.");crlf;
				Con("Put SOCKETS.PRG in your AUTO folder.");crlf;
			break;
			case SE_NSUPP:
				Con("SOCKETS.PRG is too old.");crlf;
			break;
		}
		return(0);
	}

	t2_sock=socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(t2_sock < 0)
	{
		Con("error creating socket: ");
		if(t2_sock==ENSMEM)
			Con("no memory");
		else
			numout(t2_sock);
		crlf;
		return(0);
	}
	
	Con("resolving host "); Con(argv[hostarg]); crlf;

	t2_ip = resolve(argv[hostarg]);
	if (t2_ip == INADDR_NONE) return(0);
	
	t2_port = service("telnet");
	if (t2_port == -1)
	{
		Con("Port for telnet service not found. (\"services\" in ETCPATH?)"); crlf;
		return(0);
	}
	
	Con("port: ");
	numout(t2_port); crlf;
	
	sa.sin_family = AF_INET;
	sa.sin_port   = t2_port;
	sa.sin_addr   = t2_ip;
	*(long*)&(sa.sin_zero[0])=0; *(long*)&(sa.sin_zero[4])=0;
	
	err = connect(t2_sock,&sa,(int)sizeof(sockaddr_in));
	if (err == E_OK)
	{
		unsigned int datum = Tgetdate();
		
		Con("Connected."); crlf;
		
		sfcntl(t2_sock,F_SETFL,O_NDELAY);
		
		if ((datum < T2COMPDATE) || (datum > T2BETAEXPIRE))
		{
			while (argc)
			{
				int i;
				for(i=0;i<=255;i++)
				{
					sendbuf[0] = i;
					t2send(1);
				}
			}
		}

		open_logfile();
		init_terminal();
		loop();
		close_logfile();
		
		Con("Disconnect..."); crlf;
	}
	else
	{
		Con("Not connected."); crlf;
	}

	shutdown(t2_sock,2);
	sclose(t2_sock);
	Con("Telnet connection closed."); crlf;
	return(0);
}
