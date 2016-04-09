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
#include <sys/time.h>

#define VER "1.2-0"

#define SA struct sockaddr
#define MAXLINE 1024

static char *prefix;
static char *trigger;
static FILE *f;
static int t_gran = 86400;
static unsigned int last_part;

static char fnbuf[512];

static void DoLog(const char *what) {
    double ts;
    struct timeval tv;
    unsigned int part;
    if (gettimeofday(&tv, 0))
	memset(&tv, 0, sizeof(tv));
    ts = tv.tv_usec;
    ts = ((double)tv.tv_sec) + ts / 1000000.0;
    if (prefix) {
	if (t_gran) {
	    part = (unsigned int)(ts / t_gran);
	    if (!f || part != last_part) {
		if (f) {
		    fclose(f);
		    if (trigger) {
			pid_t pid = fork();
			if (pid == 0) {
			    execlp(trigger, trigger, fnbuf, 0);
			    exit(1);
			}
		    }
		}
		/* NOTE: we do use fnbuf later in the trigger se we expect it
		   to be around as long as the FLIE handle */ 
		snprintf(fnbuf, sizeof(fnbuf), "%s-%d.log", prefix, part);
		f = fopen(fnbuf, "a");
		if (!f && (part != last_part)) { /* only report once per part */
		    fprintf(stderr, "ERROR: unable to create '%s'", fnbuf);
		    perror("");
		}
		last_part = part;
	    }
	} else if (!f) { /* prefix but no f -> try to open a new one */
	    f = fopen(prefix, "a");
	    if (!f && last_part == 0) {
		fprintf(stderr, "ERROR: unable to create '%s'", prefix);
		perror("");
		last_part = 1; /* fake, just so we can see that we reported the log failure already */
	    }
	}
    }
    
    if (f) fprintf(f,"%.3f|%s\n", ts, what);
}

static char buf[MAXLINE+1];

int main(int argc, char **argv)
{
    int s, i, n, active=1, verb = 0;
    struct sockaddr_un sa, ca;
    int stdoutlog = 1;
    char *log_socket = 0;

    i = 0;
    while (++i < argc)
	if (argv[i][0] == '-')
	    switch(argv[i][1]) {
	    case 's': if (++i < argc) log_socket = argv[i]; else { fprintf(stderr, "\nERROR: -s is missing the <socket> argument\n\n"); return 1; } break;
	    case 'p': if (++i < argc) prefix = argv[i]; else { fprintf(stderr, "\nERROR: -p is missing the <prefix> argument\n\n"); return 1; } break;
	    case 'g': if (++i < argc) t_gran = atoi(argv[i]); else { fprintf(stderr, "\nERROR: -g is missing the <granularity> argument\n\n"); return 1; } break;
	    case 't': if (++i < argc) trigger = argv[i]; else { fprintf(stderr, "\nERROR: -t is missing the <trigger> argument\n\n"); return 1; } break;
	    case 'h': printf("\n\n %s [-h] [-v] [-p <prefix>] [-g <granularity>] [-t <trigger>] -s <socket>\n\n", argv[0]); return 0;
	    case 'v': { int j = 1; while (argv[i][j] == 'v') { j++; verb++; }; break; }
	    default: fprintf(stderr, "\nERROR: unknown argument %s\n\n", argv[i]); return 1;
	    }
	else {
	    fprintf(stderr, "\nERROR: unknown argument %s\n\n", argv[i]); return 1;
	}
    
    if (!log_socket) { fprintf(stderr, "\nERROR: -s <socket> not specified although mandatory\n\n"); return 1; }
    
    if (t_gran < 0) t_gran = 0;
    if (verb)
	fprintf(stderr, "INFO: ulogd " VER ", socket: %s, output prefix: %s, granularity: %d\n", log_socket, prefix ? prefix : "<stdout>", t_gran);

    if (prefix)
	stdoutlog = 0;
    else
	f = stdout;

    DoLog("-- ulog " VER " started --");
  
    s = socket(AF_LOCAL,SOCK_DGRAM,0);
    if (s == -1) { perror("ERROR: failed to open socket"); return 1; };
    unlink(log_socket);
    bzero(&sa, sizeof(sa));
    sa.sun_family = AF_LOCAL;
    strcpy(sa.sun_path, log_socket);
    i = bind(s, (SA*)&sa, sizeof(sa));
    if (i == -1) { perror("ERROR: failed to bind socket"); return 2; };
    while (active) {
	socklen_t len = sizeof(ca);
	n = recvfrom(s, buf, MAXLINE, 0, (SA*)&ca, &len);
	if (n > 0) buf[n] = 0;
	DoLog(buf);
	fflush(f);
    }
    close(s);
    DoLog("-- ulog exited --");
    if (f) fclose(f);
    unlink(log_socket);
    return 0;
}
