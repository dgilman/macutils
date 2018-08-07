#include "hexbin.h"
#ifdef HQX
#include "globals.h"
#include "readline.h"
#include "crc.h"
#include "buffer.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/util.h"
#include "printhdr.h"

extern void exit();

static void get_header();
static void oflush();
static int getq();
static long get2q();
static long get4q();
static getqbuf();

static char *g_macname;

/* New stuff which hopes to improve the speed. */

#define RUNCHAR 0x90

#define DONE 0x7F
#define SKIP 0x7E
#define FAIL 0x7D

static char lookup[256] = {
/*       ^@    ^A    ^B    ^C    ^D    ^E    ^F    ^G   */
/* 0*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       \b    \t    \n    ^K    ^L    \r    ^N    ^O   */
/* 1*/	FAIL, FAIL, SKIP, FAIL, FAIL, SKIP, FAIL, FAIL,
/*       ^P    ^Q    ^R    ^S    ^T    ^U    ^V    ^W   */
/* 2*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       ^X    ^Y    ^Z    ^[    ^\    ^]    ^^    ^_   */
/* 3*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*              !     "     #     $     %     &     '   */
/* 4*/	FAIL, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
/*        (     )     *     +     ,     -     .     /   */
/* 5*/	0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, FAIL, FAIL,
/*        0     1     2     3     4     5     6     7   */
/* 6*/	0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, FAIL,
/*        8     9     :     ;     <     =     >     ?   */
/* 7*/	0x14, 0x15, DONE, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        @     A     B     C     D     E     F     G   */
/* 8*/	0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
/*        H     I     J     K     L     M     N     O   */
/* 9*/	0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, FAIL,
/*        P     Q     R     S     T     U     V     W   */
/*10*/	0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, FAIL,
/*        X     Y     Z     [     \     ]     ^     _   */
/*11*/	0x2C, 0x2D, 0x2E, 0x2F, FAIL, FAIL, FAIL, FAIL,
/*        `     a     b     c     d     e     f     g   */
/*12*/	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, FAIL,
/*        h     i     j     k     l     m     n     o   */
/*13*/	0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, FAIL, FAIL,
/*        p     q     r     s     t     u     v     w   */
/*14*/	0x3D, 0x3E, 0x3F, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        x     y     z     {     |     }     ~    ^?   */
/*15*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*16*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
};

static int stop = 0;

static unsigned char obuf[BUFSIZ];
static unsigned char *op = obuf;
static unsigned char *oq;

#define S_HEADER    0
#define S_DATAOPEN    1
#define S_DATAWRITE    2
#define S_DATAC1    3
#define S_DATAC2    4
#define S_RSRCOPEN    5
#define S_RSRCWRITE    6
#define S_RSRCC1    7
#define S_RSRCC2    8
#define S_EXCESS    9

static int ostate = S_HEADER;

static unsigned long calc_crc;
static unsigned long file_crc;

static long todo;

#define output(c) { *op++ = (c); if(op >= &obuf[BUFSIZ]) oflush(); }

void hqx(macname)
char *macname;
{
    int n, normlen, c;
    register char *in, *out;
    register int b6, b8, data, lastc = 0;
    char state68 = 0, run = 0, linestate, first = 1;

    g_macname = macname;

    ostate = S_HEADER;
    stop = 0;

    while(!stop) {
	n = strlen((char *)line);
	while(n > 0 && line[n - 1] == ' ') {
	    n--;
	}
	out = line+n;
	if(uneven_lines) {
	     goto skipcheck;
	}
	if(first) {
	    normlen = n;
	}
	/* Check line for intermediate garbage */
	linestate = SKIP;
	for(in = line; in < out; in++) {
	    if((linestate = lookup[*in & 0xff]) == FAIL ||
		((linestate == DONE) && !first)) {
		break;
	    }
	}
	if(linestate != FAIL && n != normlen && linestate != DONE) {
	    c = fgetc(ifp);
	    (void)ungetc(c, ifp);
	    if(lookup[c] == DONE) {
		linestate = DONE;
	    }
	}
	if(linestate == FAIL || (n != normlen && linestate != DONE)) {
	    if(verbose && n > 0) {
		*out = 0;
		(void)fprintf(stderr, "Skip:%s\n", line);
	    }
	    if(readline()) {
		continue;
	    } else {
		break;
	    }
	}
skipcheck:
	in = line;
	do {
	    if((b6 = lookup[*in & 0xff]) >= 64) {
		switch (b6) {
		case DONE:
		    first = !first;
		    if(first) {
			goto done;
		    }
		case SKIP:
		    break;
		default:
		    if(uneven_lines) {
			break;
		    }
		    (void)fprintf(stderr, "bad char '%c'(%d)\n", *in, *in);
		    goto done;
		}
	    } else {
		/* Pack 6 bits to 8 bits */
		switch (state68++) {
		case 0:
		    b8 = b6<<2;
		    continue; /* No data byte */
		case 1:
		    data = b8 | (b6>>4);
		    b8 = (b6&0xF) << 4;
		    break;
		case 2:
		    data = b8 | (b6>>2);
		    b8 = (b6&0x3) << 6;
		    break;
		case 3:
		    data = b8 | b6;
		    state68 = 0;
		    break;
		}
		if(!run) {
		    if(data == RUNCHAR) {
			run = 1;
		    } else {
			output(lastc = data);
		    }
		}
		else {
		    if(data == 0) {
			output(lastc = RUNCHAR);
		    } else {
			while(--data > 0) {
			    output(lastc);
			}
		    }
		    run = 0;
		}
	    }
	} while(++in < out);
	if(!stop) {
	    if(!readline()) {
		break;
	    }
	}
    }
done:
    oflush();
    if(!stop && ostate != S_EXCESS) {
	(void)fprintf(stderr, "premature EOF\n");
#ifdef SCAN
	do_error("hexbin: premature EOF");
#endif /* SCAN */
	exit(1);
    }
    end_put();
    print_header2(verbose);
}

static void get_header()
{
    int n;
    unsigned long calc_crc, file_crc;

    crc = INITCRC;			/* compute a crc for the header */

    for(n = 0; n < INFOBYTES; n++) {
	info[n] = 0;
    }
    n = getq();			/* namelength */
    n++;				/* must read trailing null also */
    getqbuf(trname, n);		/* read name */
    if(g_macname[0] == '\0') {
	g_macname = trname;
    }

    n = strlen(g_macname);
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    (void)strncpy(mh.m_name, g_macname, n);
    mh.m_name[n] = '\0';

    getqbuf(mh.m_type, 4);
    getqbuf(mh.m_author, 4);
    mh.m_flags = get2q();
    mh.m_datalen = get4q();
    mh.m_rsrclen = get4q();

    calc_crc = crc;
    file_crc = get2q();
    verify_crc(calc_crc, file_crc);
    if(listmode) {
	(void)fprintf(stderr, "This file is in \"hqx\" format.\n");
    }
    transname(mh.m_name, trname, n);
    define_name(trname);
    print_header0(0);
    print_header1(0, verbose);
    info[I_NAMEOFF] = n;
    (void)strncpy(info + I_NAMEOFF + 1, mh.m_name, n);
    (void)strncpy(info + I_TYPEOFF, mh.m_type, 4);
    (void)strncpy(info + I_AUTHOFF, mh.m_author, 4);
    put2(info + I_FLAGOFF, (unsigned long)mh.m_flags);
    put4(info + I_DLENOFF, (unsigned long)mh.m_datalen);
    put4(info + I_RLENOFF, (unsigned long)mh.m_rsrclen);
    put4(info + I_CTIMOFF, (unsigned long)mh.m_createtime);
    put4(info + I_MTIMOFF, (unsigned long)mh.m_modifytime);
}

static void oflush()
{
    int n, i;

    oq = obuf;
    while(oq < op && !stop) {
	switch (ostate) {
	case S_HEADER:
	    get_header();
	    ++ostate;
	    break;
	case S_DATAOPEN:
	    set_put(1);
	    todo = mh.m_datalen;
	    crc = INITCRC;
	    ++ostate;
	    break;
	case S_RSRCOPEN:
	    set_put(0);
	    todo = mh.m_rsrclen;
	    crc = INITCRC;
	    ++ostate;
	    break;
	case S_DATAWRITE:
	case S_RSRCWRITE:
	    n = op-oq;
	    if(n > todo) {
		n = todo;
	    }
	    for(i = 0; i < n; i++) {
		put_byte((char)(oq[i]));
	    }
	    comp_q_crc_n(oq, oq+n);
	    oq += n;
	    todo -= n;
	    if(todo <= 0) {
		++ostate;
	    }
	    break;
	case S_DATAC1:
	case S_RSRCC1:
	    calc_crc = crc;
	    file_crc = getq() << 8;
	    ++ostate;
	    break;
	case S_DATAC2:
	case S_RSRCC2:
	    /* Skip crc bytes */
	    file_crc |= getq();
	    verify_crc(calc_crc, file_crc);
	    ++ostate;
	    break;
	case S_EXCESS:
	    (void)fprintf(stderr, "%d excess bytes ignored\n", op-oq);
	    oq = op;
	    break;
	}
    }
    op = obuf;
}

static int getq()
{
    int c;

    if(oq >= op) {
	(void)fprintf(stderr, "premature EOF\n");
#ifdef SCAN
	do_error("hexbin: premature EOF");
#endif /* SCAN */
	exit(1);
    }
    c = *oq++ & 0xff;
    comp_q_crc((unsigned)c);
    return c;
}

/* get2q(); q format -- read 2 bytes from input, return short */
static long get2q()
{
    short high = getq() << 8;
    return high | getq();
}

/* get4q(); q format -- read 4 bytes from input, return long */
static long get4q()
{
    int i;
    long value = 0;

    for(i = 0; i < 4; i++) {
	value = (value<<8) | getq();
    }
    return value;
}

/* getqbuf(); q format -- read n characters from input into buf */
static getqbuf(buf, n)
    char *buf;
    int n;
{
    int i;

    for(i = 0; i < n; i++) {
	*buf++ = getq();
    }
}
#else /* HQX */
int hqx; /* keep lint and some compilers happy */
#endif /* HQX */

