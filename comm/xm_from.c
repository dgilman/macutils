#include "comm.h"
#ifdef XM
#include <stdio.h>
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/masks.h"
#include "globals.h"
#include "protocol.h"

extern int tgetc();
extern int tgetrec();
extern void tputc();

static void receive_part();
static int receive_sync();
static int receive_rec();

char info[INFOBYTES];

void xm_from()
{
unsigned long data_size, rsrc_size;
char text[64];

    if(receive_sync() == ACK) {
	receive_part(info, DATABYTES, 1);
	transname(info + I_NAMEOFF + 1, text, info[I_NAMEOFF]);
	define_name(text);
	data_size = get4(info + I_DLENOFF);
	rsrc_size = get4(info + I_RLENOFF);
	start_info(info, rsrc_size, data_size);
	start_data();
	receive_part(out_buffer, data_size, 1);
	start_rsrc();
	receive_part(out_buffer, rsrc_size, 0);
	end_file();
    }
}

static void receive_part(info, size, more)
char *info;
int size, more;
{
int recno = 1, i, status, naks = 0;

    status = 0;
    while(status != EOT) {
	status = receive_rec(info, DATABYTES, recno);
	switch(status) {
	case EOT:
	    if(!pre_beta) {
		tputc(ACK);
	    }
	    if(more) {
		tputc(NAK);
	    }
	    size = 0;
	    break;
	case ACK:
	    tputc(ACK);
	    naks = 0;
	    size -= DATABYTES;
	    info += DATABYTES;
	    recno = (recno + 1) & MAXRECNO;
	    break;
	case DUP:
	    tputc(ACK);
	    naks = 0;
	    break;
	case NAK:
	    if(naks++ < RETRIES) {
		tputc(NAK);
		break;
	    }
	case CAN:
	    tputc(CAN);
	    cleanup(-1);
	}
    }
}

static int receive_sync()
{
int c;

    for(;;) {
	c = tgetc(60);
	switch(c) {
	case ESC:
	    break;
	case CAN:
	    cleanup();
	    break;
	case EOT:
	case TMO:
	    return c;
	default:
	    continue;
	}
	c = tgetc(1);
	if(c == 'a') {
	    break;;
	}
    }
    tputc(ACK);
    return ACK;
}

static int receive_rec(buf, bufsize, recno)
char *buf;
int bufsize, recno;
{
int i, cksum, c, rec, recbar;
char *bp;

    c = tgetc(SOHTIMO);
    switch(c) {
    case EOT:
    case CAN:
	return c;
    case SOH:
	break;
    case TMO:
    default:
	return NAK;
    }
    rec = tgetc(CHRTIMO);
    if(rec == TMO) {
	return NAK;
    }
    recbar = tgetc(CHRTIMO);
    if(recbar == TMO) {
	return NAK;
    }
    if(rec + recbar != MAXRECNO) {
	return NAK;
    }
    if(tgetrec(buf, bufsize, LINTIMO) == TMO) {
	return NAK;
    }
    bp = buf;
    cksum = 0;
    for(i = 0; i < bufsize; i++) {
	cksum += *bp++ & BYTEMASK;
    }
    c = tgetc(CHRTIMO);
    if(c == TMO) {
	return NAK;
    }
    if(c != (cksum & BYTEMASK)) {
	return NAK;
    }
    if(rec == recno - 1) {
	return DUP;
    }
    if(rec != recno) {
	return CAN;
    }
    return ACK;
}

#else /* XM */
int xm_from; /* Keep lint and some compilers happy */
#endif /* XM */
