#include "hexbin.h"
#ifdef HECX
#include "globals.h"
#include "crc.h"
#include "readline.h"
#include "../util/masks.h"
#include "../util/util.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "buffer.h"
#include "printhdr.h"

extern void exit();

static void do_o_forks();
static long make_file();
static void comp_c_crc();
static void comp_e_crc();
static int comp_to_bin();
static int hex_to_bin();
static int hexit();

static int compressed;

/* old format -- process .hex and .hcx files */
void hecx(macname, filename)
char *macname, *filename;
{
    int n;

    for(n = 0; n < INFOBYTES; n++) {
	info[n] = 0;
    }
    compressed = 0;
    /* set up name for output files */
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
	if(n > 4) {
	    n -= 4;
	    if(!strncmp(macname + n, ".hex", 4) ||
	       !strncmp(macname + n, ".hcx", 4)) {
		macname[n] = '\0';
	    }
	}
    }
    n = strlen(macname);
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    (void)strncpy(mh.m_name, macname, n);
    mh.m_name[n] = '\0';

    /* "#TYPEAUTH$flag"  line already read */
    n = strlen(line);
    if(n >= 6 && line[0] == '#' && line[n-5] == '$') {
	if(n >= 10) {
	    (void)strncpy(mh.m_type, &line[1], 4);
	}
	if(n >= 14) {
	    (void)strncpy(mh.m_author, &line[5], 4);
	}
	(void)sscanf(&line[n-4], "%4hx", &mh.m_flags);
    }
    transname(mh.m_name, trname, n);
    define_name(trname);
    do_o_forks();
    if(listmode) {
	if(!compressed) {
	    (void)fprintf(stderr, "This file is in \"hex\" format.\n");
	} else {
	    (void)fprintf(stderr, "This file is in \"hcx\" format.\n");
	}
    }
    print_header0(0);
    print_header1(0, 0);
    info[I_NAMEOFF] = n;
    (void)strncpy(info + I_NAMEOFF + 1, mh.m_name, n);
    (void)strncpy(info + I_TYPEOFF, mh.m_type, 4);
    (void)strncpy(info + I_AUTHOFF, mh.m_author, 4);
    put2(info + I_FLAGOFF, (unsigned long)mh.m_flags);
    put4(info + I_DLENOFF, (unsigned long)mh.m_datalen);
    put4(info + I_RLENOFF, (unsigned long)mh.m_rsrclen);
    put4(info + I_CTIMOFF, (unsigned long)mh.m_createtime);
    put4(info + I_MTIMOFF, (unsigned long)mh.m_modifytime);
    print_header2(0);
    end_put();
}

static void do_o_forks()
{
    int forks = 0, found_crc = 0;
    unsigned long calc_crc, file_crc;

    crc = 0;    /* calculate a crc for both forks */

    set_put(0);
    set_put(1);
    while(!found_crc && readline()) {
	if(line[0] == 0) {
	    continue;
	}
	if(forks == 0 && strncmp(line, "***COMPRESSED", 13) == 0) {
	    compressed++;
	    continue;
	}
	if(strncmp(line, "***DATA", 7) == 0) {
	    set_put(1);
	    mh.m_datalen = make_file(compressed);
	    forks++;
	    continue;
	}
	if(strncmp(line, "***RESOURCE", 11) == 0) {
	    set_put(0);
	    mh.m_rsrclen = make_file(compressed);
	    forks++;
	    continue;
	}
	if(compressed && strncmp(line, "***CRC:", 7) == 0) {
	    found_crc++;
	    calc_crc = crc;
	    (void)sscanf(&line[7], "%lx", &file_crc);
	    break;
	}
	if(!compressed && strncmp(line, "***CHECKSUM:", 12) == 0) {
	    found_crc++;
	    calc_crc = crc & BYTEMASK;
	    (void)sscanf(&line[12], "%lx", &file_crc);
	    file_crc &= BYTEMASK;
	    break;
	}
    }

    if(found_crc) {
	verify_crc(calc_crc, file_crc);
    } else {
	(void)fprintf(stderr, "missing CRC\n");
#ifdef SCAN
	do_error("hexbin: missing CRC");
#endif /* SCAN */
	exit(1);
    }
}

static long make_file(compressed)
int compressed;
{
    register long nbytes = 0L;

    while(readline()) {
	if(line[0] == 0) {
	    continue;
	}
	if(strncmp(line, "***END", 6) == 0) {
	    break;
	}
	if(compressed) {
	    nbytes += comp_to_bin();
	} else {
	    nbytes += hex_to_bin();
	}
    }
    return nbytes;
}

static void comp_c_crc(c)
unsigned char c;
{
    crc = (crc + c) & WORDMASK;
    crc = ((crc << 3) & WORDMASK) | (crc >> 13);
}

static void comp_e_crc(c)
unsigned char c;
{
    crc += c;
}

#define SIXB(c) (((c)-0x20) & 0x3f)

static int comp_to_bin()
{
    char obuf[BUFSIZ];
    register char *ip = line;
    register char *op = obuf;
    register int n, outcount;
    int numread, incount;

    numread = strlen(line);
    outcount = (SIXB(ip[0]) << 2) | (SIXB(ip[1]) >> 4);
    incount = ((outcount / 3) + 1) * 4;
    for(n = numread; n < incount; n++) {  /* restore lost spaces */
	line[n] = ' ';
    }

    n = 0;
    while(n <= outcount) {
	*op++ = SIXB(ip[0]) << 2 | SIXB(ip[1]) >> 4;
	*op++ = SIXB(ip[1]) << 4 | SIXB(ip[2]) >> 2;
	*op++ = SIXB(ip[2]) << 6 | SIXB(ip[3]);
	ip += 4;
	n += 3;
    }

    for(n = 1; n <= outcount; n++) {
	comp_c_crc((unsigned)obuf[n]);
	put_byte(obuf[n]);
    }
    return outcount;
}

static int hex_to_bin()
{
    register char *ip = line;
    register int n, outcount;
    int c;

    n = strlen(line);
    outcount = n / 2;
    for(n = 0; n < outcount; n++) {
	c = hexit((int)*ip++);
	comp_e_crc((unsigned)(c = (c << 4) | hexit((int)*ip++)));
	put_byte((char)c);
    }
    return outcount;
}

static int hexit(c)
int c;
{
    if('0' <= c && c <= '9') {
	return c - '0';
    }
    if('A' <= c && c <= 'F') {
	return c - 'A' + 10;
    }

    (void)fprintf(stderr, "illegal hex digit: %c", c);
#ifdef SCAN
    do_error("hexbin: illegal hex digit");
#endif /* SCAN */
    exit(1);
    /* NOTREACHED */
}
#else /* HECX */
int hecx; /* keep lint and some compilers happy */
#endif /* HECX */

