#include <stdio.h>
#include <signal.h>
#ifndef TERMIOS_H
#include <sgtty.h>
#else /* TERMIOS_H */
#include <termios.h>
#endif /* TERMIOS_H */
#include <setjmp.h>
#include "../util/masks.h"
#include "protocol.h"
#include "globals.h"

void cleanup();
void timedout();
int tgetc();
void tputc();

static jmp_buf timobuf;

#ifndef TERMIOS_H
static struct sgttyb otty, ntty;
#else /* TERMIOS_H */
static struct termios otty, ntty;
#endif /* TERMIOS_H */
static int ttyfd;
static int signal_set;

void setup_tty()
{
    ttyfd = fileno(stderr);
    if(!signal_set) {
	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGINT, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGTERM, cleanup);
	if(time_out) {
	    (void)signal(SIGALRM, timedout);
	}
	signal_set = 1;
    }
#ifndef TERMIOS_H
    (void)ioctl(ttyfd, TIOCGETP, &otty);
    ntty = otty;
    ntty.sg_flags = RAW | ANYP;
    (void)ioctl(ttyfd, TIOCSETP, &ntty);
#else /* TERMIOS_H */
    (void)tcgetattr(ttyfd, &otty);
    ntty = otty;
    ntty.c_lflag &= ~(ICANON | ISIG | ECHO);
    ntty.c_iflag &= IXOFF;
    ntty.c_oflag &= ~(OPOST);
    ntty.c_cflag &= ~(PARENB | PARODD);
    ntty.c_cc[VMIN] = 1;
    ntty.c_cc[VTIME] = 0;
    (void)tcsetattr(ttyfd, TCSAFLUSH, &ntty);
#endif /* TERMIOS_H */
}

void reset_tty()
{
    (void)sleep(1); /* Wait for output to drain */
#ifndef TERMIOS_H
    (void)ioctl(ttyfd, TIOCSETP, &otty);
#else /* TERMIOS_H */
    (void)tcsetattr(ttyfd, TCSAFLUSH, &otty);
#endif /* TERMIOS_H */
}

void cleanup(sig) int sig;
{
    reset_tty();
    exit(sig);
}

void timedout()
{
    (void)signal(SIGALRM, timedout);
    longjmp(timobuf, 1);
}

int tgetc(timeout)
int timeout;
{
char c;
int i;

    if(time_out) {
	if(setjmp(timobuf)) {
	    return TMO;
	}
	(void)alarm(timeout);
    }
    i = read(ttyfd, &c, 1);
    if(time_out) {
	(void)alarm(0);
    }
    if(i == 0) {
	return EOT;
    } else {
	return c & BYTEMASK;
    }
}

tgetrec(buf, count, timeout)
char *buf;
int count, timeout;
{
int i, tot = 0, cc = count;

    if(time_out) {
	if(setjmp(timobuf)) {
	    return TMO;
	}
	(void)alarm(timeout);
    }
    while(tot < count) {
	i = read(ttyfd, buf, cc);
	if(i < 0) {
	    continue;
	}
	tot += i;
	cc -= i;
	buf += i;
    }
    if(time_out) {
	(void)alarm(0);
    }
    return 0;
}

void tputc(c)
int c;
{
char cc;

    cc = c & BYTEMASK;
    (void)write(ttyfd, &cc, 1);
}

void tputrec(buf, count)
char *buf;
int count;
{
    (void)write(ttyfd, buf, count);
}

