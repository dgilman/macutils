#include "hexbin.h"
#ifdef MU
#include "globals.h"
#include "readline.h"
#include "../util/masks.h"
#include "../util/util.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "buffer.h"
#include "printhdr.h"

extern void exit();

static void do_mu_fork();
static int mu_comp_to_bin();
static int mu_convert();

/* mu format -- process .mu files */
void mu(macname)
char *macname;
{
    int n;

    for(n = 0; n < INFOBYTES; n++) {
	info[n] = 0;
    }

    /* set up name for output files */
    if(macname[0] == '\0') {
	n = 0;
	while(line[n] != '"') {
	    n++;
	}
	macname = line + n + 1;
	line[strlen(line) - 1] = 0;
    }
    n = strlen(macname);
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    (void)strncpy(mh.m_name, macname, n);
    mh.m_name[n] = '\0';
    info[I_NAMEOFF] = n;
    (void)strncpy(info + I_NAMEOFF + 1, mh.m_name, n);

    if(listmode) {
	(void)fprintf(stderr, "This file is in \"mu\" format.\n");
    }
    transname(mh.m_name, trname, n);
    define_name(trname);
    print_header0(0);
    set_put(0);
    set_put(1);
    do_mu_fork();
    mh.m_datalen = data_size;
    if(!readline()) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("hexbin: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
    if(strncmp(line, "begin ", 6)) {
	(void)fprintf(stderr, "No UU header found.\n");
#ifdef SCAN
	do_error("hexbin: No UU header found");
#endif /* SCAN */
	exit(1);
    }
    if(!strncmp(line + 10, " .rsrc", 6)) {
	set_put(0);
	do_mu_fork();
	mh.m_rsrclen = rsrc_size;
	if(!readline()) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("hexbin: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	if(strncmp(line, "begin ", 6)) {
	    (void)fprintf(stderr, "No UU header found.\n");
#ifdef SCAN
	    do_error("hexbin: No UU header found");
#endif /* SCAN */
	    exit(1);
	}
    } else {
	mh.m_rsrclen = 0;
    }
    if(strncmp(line + 10, " .finfo", 7)) {
	(void)fprintf(stderr, "No finder info found.\n");
#ifdef SCAN
	do_error("hexbin: No finder info found");
#endif /* SCAN */
	exit(1);
    }
    if(!readline()) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("hexbin: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
    (void)mu_convert(line, info + I_TYPEOFF);
    if(!readline()) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("hexbin: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
    if(mu_convert(line, line)) {
	(void)fprintf(stderr, "Long finderinfo.\n");
#ifdef SCAN
	do_error("hexbin: Long finderinfo");
#endif /* SCAN */
	exit(1);
    }
    if(!readline()) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("hexbin: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
    if(strncmp(line, "end", 3)) {
	(void)fprintf(stderr, "\"end\" line missing.\n");
#ifdef SCAN
	do_error("hexbin: \"end\" line missing");
#endif /* SCAN */
	exit(1);
    }

    (void)strncpy(mh.m_type, info + I_TYPEOFF, 4);
    (void)strncpy(mh.m_author, info + I_AUTHOFF, 4);
    print_header1(0, 0);
    put4(info + I_DLENOFF, (unsigned long)mh.m_datalen);
    put4(info + I_RLENOFF, (unsigned long)mh.m_rsrclen);
    put4(info + I_CTIMOFF, (unsigned long)mh.m_createtime);
    put4(info + I_MTIMOFF, (unsigned long)mh.m_modifytime);
    print_header2(0);
    end_put();
}

static void do_mu_fork()
{
    long newbytes;

    while(readline()) {
	if(line[0] == 0) {
	    continue;
	}
	newbytes = mu_comp_to_bin();
	if(newbytes != 0) {
	    continue;
	}
	if(!readline()) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("hexbin: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	if(strncmp(line, "end", 3)) {
	    (void)fprintf(stderr, "\"end\" line missing.\n");
#ifdef SCAN
	    do_error("hexbin: \"end\" line missing");
#endif /* SCAN */
	    exit(1);
	}
	return;
    }
    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
    do_error("hexbin: Premature EOF");
#endif /* SCAN */
    exit(1);
    /*NOTREACHED*/
}

static int mu_comp_to_bin()
{
    char obuf[BUFSIZ];
    int outcount, n;

    outcount = mu_convert(line, obuf);
    for(n = 0; n < outcount; n++) {
	put_byte(obuf[n]);
    }
    return outcount;
}

#define SIXB(c) (((c)-0x20) & 0x3f)

static int mu_convert(ibuf, obuf)
char *ibuf, *obuf;
{
    register char *ip = ibuf;
    register char *op = obuf;
    register int n, outcount;
    int numread, incount;

    numread = strlen(ip);
    outcount = SIXB(ip[0]);
    incount = ((outcount / 3) + 1) * 4;
    for(n = numread; n < incount; n++) {  /* restore lost spaces */
	ip[n] = ' ';
    }
    ip++;

    n = 0;
    while(n <= outcount) {
	*op++ = SIXB(ip[0]) << 2 | SIXB(ip[1]) >> 4;
	*op++ = SIXB(ip[1]) << 4 | SIXB(ip[2]) >> 2;
	*op++ = SIXB(ip[2]) << 6 | SIXB(ip[3]);
	ip += 4;
	n += 3;
    }
    return outcount;
}
#else /* MU */
int mu; /* keep lint and some compilers happy */
#endif /* MU */

