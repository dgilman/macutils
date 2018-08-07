#include "../fileio/machdr.h"
#include "../fileio/rdfile.h"

extern int dorep;
extern unsigned long binhex_crcinit;
extern unsigned long binhex_updcrc();

#define RUNCHAR 0x90

static int pos_ptr;
static char codes[] =
    "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
static int state;
static int savebits;
static int rep_char;
static int rep_count;

void doheader();
void dofork();
void outbyte();
void finish();
void outbyte1();
void out6bit();
void outchar();

void dofile()
{
    (void)printf("(This file must be converted; you knew that already.)\n");
    (void)printf("\n");
    pos_ptr = 1;
    state = 0;
    rep_char = -1;
    rep_count = 0;
    outchar(':');
    doheader();
    dofork(data_fork, data_size);
    dofork(rsrc_fork, rsrc_size);
    finish();
    (void)putchar(':');
    (void)putchar('\n');
}

void doheader()
{
unsigned long crc;
int i, n;

    crc = binhex_crcinit;
    n = file_info[I_NAMEOFF];
    crc = binhex_updcrc(crc, file_info + I_NAMEOFF, n + 1);
    for(i = 0; i <= n; i++) {
	outbyte(file_info[I_NAMEOFF + i]);
    }
    n = 0;
    crc = binhex_updcrc(crc, (char *)&n, 1);
    outbyte(0);
    crc = binhex_updcrc(crc, file_info + I_TYPEOFF, 4);
    for(i = 0; i < 4; i++) {
	outbyte(file_info[I_TYPEOFF + i]);
    }
    crc = binhex_updcrc(crc, file_info + I_AUTHOFF, 4);
    for(i = 0; i < 4; i++) {
	outbyte(file_info[I_AUTHOFF + i]);
    }
    crc = binhex_updcrc(crc, file_info + I_FLAGOFF, 2);
    for(i = 0; i < 2; i++) {
	outbyte(file_info[I_FLAGOFF + i]);
    }
    crc = binhex_updcrc(crc, file_info + I_DLENOFF, 4);
    for(i = 0; i < 4; i++) {
	outbyte(file_info[I_DLENOFF + i]);
    }
    crc = binhex_updcrc(crc, file_info + I_RLENOFF, 4);
    for(i = 0; i < 4; i++) {
	outbyte(file_info[I_RLENOFF + i]);
    }
    outbyte((int)(crc >> 8));
    outbyte((int)(crc & 0xff));
}

void dofork(fork, size)
char *fork;
int size;
{
unsigned long crc;
int i;

    crc = binhex_updcrc(binhex_crcinit, fork, size);
    for(i = 0; i < size; i++) {
	outbyte(fork[i]);
    }
    outbyte((int)(crc >> 8));
    outbyte((int)(crc & 0xff));
}

void outbyte(b)
int b;
{
    b &= 0xff;
    if(dorep && (b == rep_char)) {
	if(++rep_count == 254) {
	    outbyte1(RUNCHAR);
	    outbyte1(255);
	    rep_char = -1;
	    rep_count = 0;
	}
    } else {
	if(rep_count > 0) {
	    if(rep_count > 3) {
		outbyte1(RUNCHAR);
		outbyte1(rep_count + 1);
	    } else {
		while(rep_count-- > 0) {
		    outbyte1(rep_char);
		}
	    }
	}
	outbyte1(b);
	if(b == RUNCHAR) {
	    outbyte1(0);
	    rep_char = -1;
	} else {
	    rep_char = b;
	}
	rep_count = 0;
    }
}

void finish()
{
    if(rep_count > 0) {
	if(rep_count > 3) {
	    outbyte1(RUNCHAR);
	    outbyte1(rep_count + 1);
	} else {
	    while(rep_count-- > 0) {
		outbyte1(rep_char);
	    }
	}
    }
    switch(state) {
    case 1:
	out6bit(savebits << 4);
	break;
    case 2:
	out6bit(savebits << 2);
	break;
    default:
	break;
    }
}

void outbyte1(b)
int b;
{
    switch(state) {
    case 0:
	out6bit(b >> 2);
	savebits = b & 0x3;
	state = 1;
	break;
    case 1:
	b |= (savebits << 8);
	out6bit(b >> 4);
	savebits = b & 0xf;
	state = 2;
	break;
    case 2:
	b |= (savebits << 8);
	out6bit(b >> 6);
	out6bit(b & 0x3f);
	state = 0;
	break;
    }
}

void out6bit(c)
char c;
{
    outchar(codes[c & 0x3f]);
}

void outchar(c)
char c;
{
    (void)putchar(c);
    if(++pos_ptr > 64) {
	(void)putchar('\n');
	pos_ptr = 1;
    }
}

