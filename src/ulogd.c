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

#define VER "1.4-0"

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

int ulogd(char *log_socket, int udp_port, int verb)
{
    int s, i, n, active=1;
    struct sockaddr_un sa, ca;
    struct sockaddr_in sai, cai;
    SA *ca_ptr;
    int ca_len = 0;
    int stdoutlog = 1;

    if (t_gran < 0) t_gran = 0;
    if (verb)
	fprintf(stderr, "INFO: ulogd " VER ", socket: %s, port: %d, output prefix: %s, granularity: %d\n", log_socket ? log_socket : "<udp>", udp_port, prefix ? prefix : "<stdout>", t_gran);

    if (prefix)
	stdoutlog = 0;
    else
	f = stdout;

    DoLog("-- ulog " VER " started --");

    s = (udp_port > 0) ? socket(AF_INET, SOCK_DGRAM, 0) : socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (s == -1) { perror("ERROR: failed to open socket"); return 1; };
    if (log_socket) {
        unlink(log_socket);
        bzero(&sa, sizeof(sa));
        sa.sun_family = AF_LOCAL;
        strcpy(sa.sun_path, log_socket);
        ca_ptr = (SA*) &ca;
        ca_len = sizeof(ca);
        i = bind(s, (SA*)&sa, sizeof(sa));
    } else {
        int optval = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
        bzero(&sai, sizeof(sai));
        sai.sin_family = AF_INET;
        sai.sin_addr.s_addr = htonl(INADDR_ANY);
        sai.sin_port = htons(udp_port);
        ca_ptr = (SA*) &cai;
        ca_len = sizeof(cai);
        i = bind(s, (SA*)&sai, sizeof(sai));
    }
    if (i == -1) { perror("ERROR: failed to bind socket"); return 2; };
    while (active) {
	socklen_t len = ca_len;
	n = recvfrom(s, buf, MAXLINE, 0, ca_ptr, &len);
	if (n > 0) buf[n] = 0;
	DoLog(buf);
	fflush(f);
    }
    close(s);
    DoLog("-- ulog exited --");
    if (f) fclose(f);
    if (log_socket) unlink(log_socket);
    return 0;
}

int main(int argc, char **argv)
{
    int i, verb = 0;
    int udp_port=-1;
    char *log_socket = 0;

    i = 0;
    while (++i < argc)
	if (argv[i][0] == '-')
	    switch(argv[i][1]) {
	    case 's': if (++i < argc) log_socket = argv[i]; else { fprintf(stderr, "\nERROR: -s is missing the <socket> argument\n\n"); return 1; } break;
	    case 'p': if (++i < argc) prefix = argv[i]; else { fprintf(stderr, "\nERROR: -p is missing the <prefix> argument\n\n"); return 1; } break;
	    case 'g': if (++i < argc) t_gran = atoi(argv[i]); else { fprintf(stderr, "\nERROR: -g is missing the <granularity> argument\n\n"); return 1; } break;
	    case 't': if (++i < argc) trigger = argv[i]; else { fprintf(stderr, "\nERROR: -t is missing the <trigger> argument\n\n"); return 1; } break;
	    case 'h': printf("\n\n %s [-h] [-v] [-p <prefix>] [-g <granularity>] [-t <trigger>] { -s <socket> | -u <port> }\n\n", argv[0]); return 0;
            case 'u': if (++i < argc) udp_port = atoi(argv[i]); else { fprintf(stderr, "\nERROR: -u is missing the <port> argument\n\n"); return 1; } break;
	    case 'v': { int j = 1; while (argv[i][j] == 'v') { j++; verb++; }; break; }
	    default: fprintf(stderr, "\nERROR: unknown argument %s\n\n", argv[i]); return 1;
	    }
	else {
	    fprintf(stderr, "\nERROR: unknown argument %s\n\n", argv[i]); return 1;
	}

    if (!log_socket && udp_port < 1) { fprintf(stderr, "\nERROR: both -s <socket> and -u <port> not specified although one is mandatory\n\n"); return 1; }
    if (log_socket && udp_port > 0) { fprintf(stderr, "\nERROR: both -s <socket> and -u <port> specified although mutually exclusive\n\n"); return 1; }

    ulogd(log_socket, udp_port, verb);
}

/* ---- R API ---- */

#include <Rinternals.h>

void C_ulogd(int *sPort) {
	char *log_socket = 0;
	int verb = 0;

	ulogd(log_socket, *sPort, verb);
}
