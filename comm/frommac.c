#include <stdio.h>
#include "comm.h"
#include "../util/patchlevel.h"
#include "../fileio/machdr.h"
#include "globals.h"
#include "../fileio/fileglob.h"
#include "../fileio/wrfile.h"

#define LOCALOPT	"lmxyzoTVH"

extern void exit();
extern void setup_tty();
extern void reset_tty();

extern char info[];

static void usage();

static char options[128];
static int multi_file = 0;
static int listmode = 0;

int main(argc, argv)
int argc;
char **argv;
{
    extern int optind;
    extern char *optarg;
    int errflg;
    int c;
    char tname[64];
    char fauth[5];
    char ftype[5];

    set_wrfileopt(0);
    (void)strcat(options, get_wrfileopt());
    (void)strcat(options, LOCALOPT);
    errflg = 0;

    while((c = getopt(argc, argv, options)) != EOF) {
	if(!wrfileopt((char)c)) {
	    switch(c) {
	    case 'l':
		listmode++;
		break;
	    case 'm':
		multi_file++;
		break;
	    case 'x':
		xfertype = XMODEM;
#ifndef XM
		(void)fprintf(stderr, "XMODEM not supported\n");
		exit(1);
#else /* XM */
		break;
#endif /* XM */
	    case 'y':
		xfertype = YMODEM;
#ifndef YM
		(void)fprintf(stderr, "YMODEM not supported\n");
		exit(1);
#else /* YM */
		break;
#endif /* YM */
	    case 'z':
		xfertype = ZMODEM;
#ifndef ZM
		(void)fprintf(stderr, "ZMODEM not supported\n");
		exit(1);
#else /* ZM */
		break;
#endif /* ZM */
	    case 'o':
		pre_beta++;
		break;
	    case 'T':
		time_out++;
		break;
	    case '?':
		errflg++;
		break;
	    case 'H':
		give_wrfileopt();
		(void)fprintf(stderr, "Frommac specific options:\n");
		(void)fprintf(stderr, "-l:\tgive listing\n");
		(void)fprintf(stderr, "-m:\tmulti-file transfer\n");
#ifdef XM
		(void)fprintf(stderr, "-x:\tuse XMODEM protocol\n");
#endif /* XM */
#ifdef YM
		(void)fprintf(stderr, "-y:\tuse YMODEM protocol\n");
#endif /* YM */
#ifdef ZM
		(void)fprintf(stderr, "-z:\tuse ZMODEM protocol\n");
#endif /* ZM */
		(void)fprintf(stderr, "-o:\tuse pre-beta protocol\n");
		(void)fprintf(stderr, "-T:\tdetect time-outs\n");
		(void)fprintf(stderr,
			"-V:\tgive information about this version\n");
		(void)fprintf(stderr, "-H:\tthis message\n");
		(void)fprintf(stderr,
		  "Default is silent receival of a single file using XMODEM\n");
		exit(0);
	    case 'V':
		(void)fprintf(stderr, "Version %s, ", VERSION);
		(void)fprintf(stderr, "patchlevel %d", PATCHLEVEL);
		(void)fprintf(stderr, "%s.\n", get_mina());
		(void)fprintf(stderr, "Supported protocols:\n");
#ifdef XM
		(void)fprintf(stderr, "\tXMODEM\n");
#endif /* XM */
#ifdef YM
		(void)fprintf(stderr, "\tYMODEM\n");
#endif /* YM */
#ifdef ZM
		(void)fprintf(stderr, "\tZMODEM\n");
#endif /* ZM */
		exit(0);
	    }
	}
    }
    if(errflg) {
	usage();
	exit(1);
    }

    do {
	setup_tty();
	switch(xfertype) {
	case XMODEM:
#ifdef XM
	    xm_from();
	    break;
#endif /* XM */
#ifdef YM
	case YMODEM:
	    ym_from();
	    break;
#endif /* YM */
#ifdef ZM
	case ZMODEM:
	    zm_from();
	    break;
#endif /* ZM */
	}
	reset_tty();
	if(listmode) {
	    transname(info + I_NAMEOFF + 1, tname, info[I_NAMEOFF]);
	    transname(info + I_AUTHOFF, fauth, 4);
	    transname(info + I_TYPEOFF, ftype, 4);
	    (void)fprintf(stderr,
		    "name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		    tname, ftype, fauth,
		    get4(info + I_DLENOFF), get4(info + I_RLENOFF));
	    (void)fprintf(stderr, "\n");
	}
    } while(multi_file);
    exit(0);
    /* NOTREACHED */
}

static void usage()
{
    (void)fprintf(stderr, "Usage: frommac [-%s]\n", options);
    (void)fprintf(stderr, "Use \"frommac -H\" for help.\n");
}

