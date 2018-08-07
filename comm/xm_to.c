#include "comm.h"
#ifdef XM
#include "../fileio/machdr.h"
#include "../fileio/rdfile.h"
#include "../util/masks.h"
#include "globals.h"
#include "protocol.h"

extern int tgetc();
extern void tputc();
extern void tputrec();

static void send_part();
static int send_sync();
static void send_rec();

void xm_to()
{
    if(send_sync() == ACK) {
	send_part(file_info, DATABYTES, 1);
	send_part(data_fork, data_size, 1);
	send_part(rsrc_fork, rsrc_size, 0);
    }
}

static void send_part(info, size, more)
char *info;
int size, more;
{
int recno = 1, i, status;

    while(size > 0) {
	for(i = 0; i < RETRIES; i++) {
	    send_rec(info, DATABYTES, recno);
	    status = tgetc(ACKTIMO);
	    if(status != NAK) {
		break;
	    }
	}
	if(status == NAK || status == CAN) {
	    cleanup(-1);
	}
	size -= DATABYTES;
	info += DATABYTES;
	recno = (recno + 1) & MAXRECNO;
    }
    tputc(EOT);
    if(!pre_beta) {
	status = tgetc(ACKTIMO);
    }
    if(more) {
	status = tgetc(ACKTIMO);
    }
}

static int send_sync()
{
int c, i;

    for(i = 0; i < 3; i++) {
	tputc(ESC);
	tputc('a');
	while((c = tgetc(ACKTIMO)) != TMO) {
	    switch(c) {
	    case CAN:
	    case EOT:
	    case ACK:
		return c;
	    default:
		continue;
	    }
	}
    }
    return CAN;
}

static void send_rec(buf, bufsize, recno)
char *buf;
int bufsize, recno;
{
int i, cksum;
char *bp;

    cksum = 0;
    bp = buf;
    for(i = 0; i < bufsize; i++) {
	cksum += *bp++ & BYTEMASK;
    }
    tputc(SOH);
    tputc((unsigned char)recno);
    tputc((unsigned char)(MAXRECNO - recno));
    tputrec(buf, bufsize);
    tputc((char)(cksum & BYTEMASK));
}

#else /* XM */
int xm_to; /* Keep lint and some compilers happy */
#endif /* XM */
