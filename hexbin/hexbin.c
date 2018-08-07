#ifdef TYPES_H
#include <sys/types.h>
#endif /* TYPES_H */
#include <sys/stat.h>
#include "globals.h"
#include "crc.h"
#include "readline.h"
#include "../util/masks.h"
#include "../util/util.h"
#include "../util/patchlevel.h"
#include "../fileio/wrfile.h"
#include "../fileio/wrfileopt.h"
#include "../fileio/machdr.h"
#include "../fileio/kind.h"
#include "../util/curtime.h"
#include "hexbin.h"

#define LOCALOPT	"ilvcn:qVH"

extern void exit();
extern void backtrans();
#ifdef DL
extern void dl();
#endif /* DL */
#ifdef HECX
extern void hecx();
#endif /* HECX */
#ifdef HQX
extern void hqx();
#endif /* HQX */
#ifdef MU
extern void mu();
#endif /* MU */

static void usage();
static void do_files();
static int find_header();

static char options[128];

int main(argc, argv)
int argc;
char **argv;
{
    char *filename;
    char macname[32];
    extern int optind;
    extern char *optarg;
    int errflg;
    int c;

    set_wrfileopt(0);
    (void)strcat(options, get_wrfileopt());
    (void)strcat(options, LOCALOPT);
    errflg = 0;
    filename = "";
    macname[0] = 0;

    while((c = getopt(argc, argv, options)) != EOF) {
	if(!wrfileopt((char)c)) {
	    switch(c) {
	    case 'l':
		listmode++;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'i':
		info_only++;
		break;
	    case 'c':
		uneven_lines++;
		break;
	    case 'n':
		backtrans(macname, optarg);
		break;
	    case '?':
		errflg++;
		break;
	    case 'H':
		give_wrfileopt();
		(void)fprintf(stderr, "Hexbin specific options:\n");
		(void)fprintf(stderr,
			"-i:\tgive information only, do not convert\n");
		(void)fprintf(stderr, "-l:\tgive listing\n");
		(void)fprintf(stderr,
			"-v:\tgive verbose listing, including lines skipped\n");
		(void)fprintf(stderr,
			"-c:\tdo not check for equal line lengths\n");
		(void)fprintf(stderr, "-n nm:\tname to be generated\n");
		(void)fprintf(stderr,
			"-V:\tgive information about this version\n");
		(void)fprintf(stderr, "-H:\tthis message\n");
		(void)fprintf(stderr, "Default is silent conversion\n");
		exit(0);
	    case 'V':
		(void)fprintf(stderr, "Version %s, ", VERSION);
		(void)fprintf(stderr, "patchlevel %d", PATCHLEVEL);
		(void)fprintf(stderr, "%s.\n", get_mina());
		(void)fprintf(stderr, "Hexified files recognized:\n");
#ifdef DL
		(void)fprintf(stderr, "\tDownload (.dl)\n");
#endif /* DL */
#ifdef HECX
		(void)fprintf(stderr, "\tBinHex 2.0 (.hex)\n");
		(void)fprintf(stderr, "\tBinHex 3.0 (.hcx)\n");
#endif /* HECX */
#ifdef HQX
		(void)fprintf(stderr, "\tBinHex 4.0 (.hqx)\n");
#endif /* HQX */
#ifdef MU
		(void)fprintf(stderr, "\tUUTool (.mu)\n");
#endif /* MU */
		exit(0);
	    }
	}
    }
    if(errflg) {
	usage();
	exit(1);
    }
    if(info_only || verbose) {
	listmode++;
    }

    do {
	if(optind == argc) {
	    filename = "-";
	} else {
	    filename = argv[optind];
	    optind++;
#ifdef SCAN
	    do_idf(filename, UNIX_NAME);
#endif /* SCAN */
	}
	do_files(filename, macname);
    } while(optind < argc);
    exit(0);
    /* NOTREACHED */
}

static char *extensions[] = {
#ifdef DL
    ".dl",
#endif /* DL */
#ifdef HECX
    ".hex",
    ".Hex",
    ".hcx",
    ".Hcx",
#endif /* HECX */
#ifdef HQX
    ".hqx",
    ".Hqx",
#endif /* HQX */
#ifdef MU
    ".mu",
#endif /* MU */
    "",
    NULL
};

static void do_files(filename, macname)
char *filename;	/* input file name -- extension optional */
char *macname;	/* name to use on the mac side of things */
{
    char namebuf[256];
    char **ep;
    struct stat stbuf;
    long curtime;
    int qformat;
    int again;

    if(filename[0] == '-') {
	ifp = stdin;
	filename = "stdin";
    } else {
	/* find input file and open it */
	for(ep = extensions; *ep != NULL; ep++) {
	    (void)sprintf(namebuf, "%s%s", filename, *ep);
	    if(stat(namebuf, &stbuf) == 0) {
		break;
	    }
	}
	if(*ep == NULL) {
	    perror(namebuf);
	    exit(1);
	}
	ifp = fopen(namebuf, "r");
	if(ifp == NULL) {
	    perror(namebuf);
	    exit(1);
	}
    }
    again = 0;
nexttry:
    if(ifp == stdin) {
	curtime = (long)time((time_t *)0) + TIMEDIFF;
	mh.m_createtime = curtime;
	mh.m_modifytime = curtime;
    } else {
	mh.m_createtime = stbuf.st_mtime + TIMEDIFF;
	mh.m_modifytime = stbuf.st_mtime + TIMEDIFF;
    }

    qformat = find_header(again); /* eat mailer header &cetera, intuit format */

    switch (qformat) {
#ifdef DL
    case form_dl:
	dl(macname, filename);
	break;
#endif /* DL */
#ifdef HECX
    case form_hecx:
	hecx(macname, filename);
	break;
#endif /* HECX */
#ifdef HQX
    case form_hqx:
	hqx(macname);
	again = 1;
	goto nexttry;
#endif /* HQX */
#ifdef MU
    case form_mu:
	mu(macname);
	again = 1;
	goto nexttry;
#endif /* MU */
    default:
	break;
    }
    (void)fclose(ifp);
}

/* eat characters until header detected, return which format */
static int find_header(again)
int again;
{
    int c, dl_start, llen;
    char *cp;
    char header[INFOBYTES];
    int ds, rs;

    if(again && was_macbin) {
	while(to_read-- > 0) {
	    c = fgetc(ifp);
	}
    }
    was_macbin = 0;
    c = fgetc(ifp);
    (void)ungetc(c, ifp);
    if(c == 0) {
	was_macbin = 1;
	if(fread(header, 1, INFOBYTES, ifp) != INFOBYTES) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("hexbin: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	ds = get4(header + I_DLENOFF);
	rs = get4(header + I_RLENOFF);
	ds = (((ds + 127) >> 7) << 7);
	rs = (((rs + 127) >> 7) << 7);
	to_read = ds + rs;
	if(strncmp(header + I_TYPEOFF, "TEXT", 4)) {
	    (void)fprintf(stderr, "This file is not a proper BinHexed file.\n");
#ifdef SCAN
	    do_error("hexbin: not a proper BinHexed file");
#endif /* SCAN */
	    exit(1);
	}
	if(listmode) {
	    (void)fprintf(stderr, "This file is probably packed by ");
	    if(!strncmp(header + I_AUTHOFF, "BNHQ", 4)) {
		(void)fprintf(stderr, "\"BinHex\"");
	    } else if(!strncmp(header + I_AUTHOFF, "BthX", 4)) {
		(void)fprintf(stderr, "\"BinHqx\"");
	    } else if(!strncmp(header + I_AUTHOFF, "BnHq", 4)) {
		(void)fprintf(stderr, "\"StuffIt\"");
	    } else if(!strncmp(header + I_AUTHOFF, "ttxt", 4)) {
		(void)fprintf(stderr, "\"Compactor\"");
	    } else if(!strncmp(header + I_AUTHOFF, "BSWU", 4)) {
		(void)fprintf(stderr, "\"UUTool\"");
	    } else {
		(void)fprintf(stderr, "an unknown program");
	    }
	    (void)fprintf(stderr, ".\n");
	}
    }
    /*	look for "(This file ...)" or "(Convert with...)" line. */
    /*	or for "begin " */
    /*	dl format starts with a line containing only the symbols '@' to 'O',
	or '|'. */
    while(readline()) {
	llen = strlen(line);
#ifdef HQX
	if((strncmp(line, "(This file", 10) == 0) ||
	    (strncmp(line, "(Convert with", 13) == 0)) {
	    if(verbose) {
		(void)fprintf(stderr, "Skip:%s\n", line);
	    }
	    break;
	}
#endif /* HQX */
#ifdef MU
	if(strncmp(line, "begin ", 6) == 0) {
	    return form_mu;
	}
#endif /* MU */
#ifdef DL
	/* Do not allow bogus false starts */
	if(llen > 40 && (llen & 1) == 0) {
	    dl_start = 1;
	    for(cp = &line[0]; *cp != 0; cp++) {
		if((*cp < '@' || *cp > 'O') && *cp != '|') {
		    dl_start = 0;
		    break;
		}
	    }
	    if(dl_start && cp > &line[1]) {
		return form_dl;
	    }
	}
#endif /* DL */
	if(llen != 0 && verbose) {
	    (void)fprintf(stderr, "Skip:%s\n", line);
	}
    }
    while(readline()) {
	switch (line[0]) {
#ifdef HQX
	case ':':
	    return form_hqx;
#endif /* HQX */
#ifdef HECX
	case '#':
	    return form_hecx;
#endif /* HECX */
	default:
	    break;
	}
    }

    if(!again) {
	(void)fprintf(stderr, "unexpected EOF\n");
#ifdef SCAN
	do_error("hexbin: unexpected EOF");
#endif /* SCAN */
	exit(1);
    }
    return form_none;
}

static void usage()
{
    (void)fprintf(stderr, "Usage: hexbin [-%s] [filenames]\n", options);
    (void)fprintf(stderr, "Use \"hexbin -H\" for help.\n");
}

