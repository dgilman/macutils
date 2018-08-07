#include "hexbin.h"
#ifdef DL
#include "globals.h"
#include "crc.h"
#include "readline.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/util.h"
#include "buffer.h"
#include "printhdr.h"

extern void exit();

static long dl_fork();
static int nchar();
static int nextc();

static char *icp = &line[0];

/* oldest format -- process .dl files */
void dl(macname, filename)
char *macname, *filename;
{
    int n;

    if(listmode) {
	(void)fprintf(stderr, "This file is in \"dl\" format.\n");
    }
    for(n = 0; n < INFOBYTES; n++) {
	info[n] = 0;
    }
    /* set up for Mac name */
    if(macname[0] == '\0') {
	/* strip directories */
	macname = search_last(filename, '/');
	if(macname == NULL) {
	    macname = filename;
	} else {
	    macname++;
	}
	/* strip extension */
	n = strlen(macname);
	if(n > 3) {
	    n -= 3;
	    if(!strncmp(macname + n, ".dl", 3)) {
		macname[n] = '\0';
	    }
	}
    }
    n = strlen(macname);
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    (void)strncpy(mh.m_name, macname, n);
    (void)strncpy(mh.m_type, "APPL", 4);
    (void)strncpy(mh.m_author, "????", 4);
    mh.m_name[n] = '\0';
    transname(mh.m_name, trname, n);
    define_name(trname);
    print_header0(0);
    print_header1(0, 0);
    set_put(1);
    mh.m_datalen = 0;
    set_put(0);
    mh.m_rsrclen = dl_fork();
    info[I_NAMEOFF] = n;
    (void)strncpy(info + I_NAMEOFF + 1, mh.m_name, n);
    (void)strncpy(info + I_TYPEOFF, mh.m_type, 4);
    (void)strncpy(info + I_AUTHOFF, mh.m_author, 4);
    put4(info + I_DLENOFF, (unsigned long)mh.m_datalen);
    put4(info + I_RLENOFF, (unsigned long)mh.m_rsrclen);
    put4(info + I_CTIMOFF, (unsigned long)mh.m_createtime);
    put4(info + I_MTIMOFF, (unsigned long)mh.m_modifytime);
    print_header2(0);
    end_put();
}

static long dl_fork()
{
    register unsigned long i, v, c;
    register unsigned long n, bytes;

    n = 0;
    bytes = 0;
    v = 0;
    crc = 0;
    while((i = nchar()) != '|') {
	if(i < '@' || i > 'O') {
	    continue;
	}
	v = (v << 4) | (i & 0xF);
	if((++n & 1) == 0) {
	    put_byte((char)v);
	    crc += v;
	    v = 0;
	    bytes++;
	}
    }
    c = 0;
    for(i = 0 ; i < 8 ; i++) {
	c = (c << 4) | (nchar() & 0xF);
    }
    verify_crc(bytes + crc, c);
    return bytes;
}

static int nchar()
{
    int i;

    if((i = nextc()) == EOF) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("hexbin: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
    return i & 0177;
}

static int nextc()
{
    while(*icp == 0) {
	if(readline() == 0) {
	    return EOF;
	}
	icp = &line[0];
    }
    return *icp++;
}
#else /* DL */
int dl; /* keep lint and some compilers happy */
#endif /* DL */

