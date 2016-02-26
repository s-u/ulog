#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#define SA struct sockaddr
#define MAXLINE 1024

#ifdef LOG_ASIS
#define DoLog(S) if(f){ fprintf(f, "%s\n", S); }
#else
#define DoLog(S) if(f){ time(&tim); strcpy(ts,ctime(&tim)); c=ts; while (*c && *c!='\n') c++; *c=0; fprintf(f,"%s | %s\n",ts,S); }
#endif

int main(int argc, char **argv)
{
  int s,i,n,len,active=1;
  struct sockaddr_un sa,ca;
  char buf[MAXLINE+1], *c, ts[128];
  FILE *f;
  time_t tim;
  int stdoutlog=1;
  char *LogFile, *log_socket;

  if (argc<2) { fprintf(stderr,"not enough parameters.\n\nusage: ulogd <socket> [<log file>]\n"); return 5; };

  log_socket=argv[1];
  if (argc>2) {
    LogFile=argv[2];
    f=fopen(LogFile,"a");
    if (!f) { fprintf(stderr,"cannot create log file\n"); return 3; };
    stdoutlog=0;
  } else f=stdout;

  DoLog("-- ulog started --");
  
  s=socket(AF_LOCAL,SOCK_DGRAM,0);
  if (s==-1) { fprintf(stderr,"failed to open socket\n"); return 1; };
  unlink(log_socket);
  bzero(&sa,sizeof(sa));
  sa.sun_family=AF_LOCAL;
  strcpy(sa.sun_path,log_socket);
  i=bind(s,(SA*)&sa,sizeof(sa));
  if (i==-1) { fprintf(stderr,"failed to bind socket\n"); return 2; };
  while (active) {
    len=sizeof(ca);
    n=recvfrom(s,buf,MAXLINE,0,(SA*)&ca,&len);
    if (n > 0) buf[n] = 0;
    if (!strcmp(buf,"LogRestart")) {
      DoLog("LogClose due to LogRestart");
      if (!stdoutlog) {
	if (f) fclose(f);      
	f=fopen(LogFile,"a");
      } else fflush(f);
    };
    if (!strcmp(buf,"LogOpen")) if(!stdoutlog && !f) f=fopen(LogFile,"a");
    if (!strcmp(buf,"LogShutdown")) active=0;
    DoLog(buf);
    fflush(f);
    if (!strcmp(buf,"LogFlush")) if(f) fflush(f);
    if (!strcmp(buf,"LogClose") && !stdoutlog) { fclose(f); f=0; };
  };  
  close(s);
  DoLog("-- ulog exited --");
  unlink(log_socket);
};
