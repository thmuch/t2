/* T2 - Telnet Terminal for IConnect      */
/* Copyright (c)1998-06-04 by Thomas Much */


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

#include "telnet.h"
#include "logfile.h"


int   t2_sock,
      t2_port,
      t2_max_lines,
      t2_max_columns,
      optionbuflen,
      remoteecho,
      t2debug = 0;

ulong t2_ip;

unsigned char sendbuf[SENDBUFLEN],
              optionbuf[OPTIONBUFLEN];




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

	Con("T2 - Telnet Terminal for IConnect (release 01, 1998-06-04)"); crlf; Con("Copyright (c)1998 Thomas Much, Application Systems Heidelberg Software GmbH");
	crlf;crlf;
	
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
			if (!strcmp(argv[i],"-d")) t2debug=1;
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
		Con("Connected."); crlf;
		
		sfcntl(t2_sock,F_SETFL,O_NDELAY);

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
