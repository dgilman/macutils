#include "macunpack.h"
#include "globals.h"
#include "../util/patchlevel.h"
#include "../fileio/wrfile.h"
#include "../fileio/wrfileopt.h"
#include "../fileio/kind.h"
#include "../util/util.h"

#define LOCALOPT	"ilvqVH"

extern char *strcat();
#ifdef STF
extern void stf();
#endif /* STF */
#ifdef PIT
extern void pit();
#endif /* PIT */
#ifdef SIT
extern void sit();
#endif /* SIT */
#ifdef CPT
extern void cpt();
#endif /* CPT */
void macbinary();

static void usage();

static char options[128];

int main(argc, argv)
int argc;
char *argv[];
{
    int c;
    extern int optind;
    extern char *optarg;
    int errflg;

    set_wrfileopt(0);
    (void)strcat(options, get_wrfileopt());
    (void)strcat(options, LOCALOPT);
    errflg = 0;

    while((c = getopt(argc, argv, options)) != EOF) {
#ifdef SCAN
	if(c == 'S') {
	    no_dd++;
	}
#endif /* SCAN */
	if(!wrfileopt((char)c)) {
	    switch(c) {
	    case 'l':
		list++;
		break;
	    case 'q':
		query++;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'i':
		info_only++;
		break;
	    case '?':
		errflg++;
		break;
	    case 'H':
		give_wrfileopt();
		(void)fprintf(stderr, "Macunpack specific options:\n");
		(void)fprintf(stderr,
			"-i:\tgive information only, do not unpack\n");
		(void)fprintf(stderr, "-l:\tgive listing\n");
		(void)fprintf(stderr, "-v:\tgive verbose listing\n");
		(void)fprintf(stderr,
			"-q:\tquery for every file/folder before unpacking\n");
		(void)fprintf(stderr,
			"-V:\tgive information about this version\n");
		(void)fprintf(stderr, "-H:\tthis message\n");
		(void)fprintf(stderr, "Default is silent unpacking\n");
		exit(0);
	    case 'V':
		(void)fprintf(stderr, "Version %s, ", VERSION);
		(void)fprintf(stderr, "patchlevel %d", PATCHLEVEL);
		(void)fprintf(stderr, "%s.\n", get_mina());
		(void)fprintf(stderr, "Archive/file types recognized:\n");
#ifdef BIN
		(void)fprintf(stderr,
			"\tBinHex 5.0, MacBinary 1.0 and UMCP (with caveat)\n");
#endif /* BIN */
#ifdef JDW
		(void)fprintf(stderr, "\tCompress It\n");
#endif /* JDW */
#ifdef STF
		(void)fprintf(stderr, "\tShrinkToFit\n");
#endif /* STF */
#ifdef LZC
		(void)fprintf(stderr, "\tMacCompress\n");
#endif /* LZC */
#ifdef ASQ
		(void)fprintf(stderr, "\tAutoSqueeze\n");
#endif /* ASQ */
#ifdef ARC
		(void)fprintf(stderr, "\tArcMac\n");
#endif /* ARC */
#ifdef PIT
		(void)fprintf(stderr, "\tPackIt\n");
#endif /* PIT */
#ifdef SIT
		(void)fprintf(stderr, "\tStuffIt and StuffIt Deluxe\n");
#endif /* SIT */
#ifdef DIA
		(void)fprintf(stderr, "\tDiamond\n");
#endif /* DIA */
#ifdef CPT
		(void)fprintf(stderr, "\tCompactor\n");
#endif /* CPT */
#ifdef ZMA
		(void)fprintf(stderr, "\tZoom\n");
#endif /* ZMA */
#ifdef LZH
		(void)fprintf(stderr, "\tMacLHa\n");
#endif /* LZH */
#ifdef DD
		(void)fprintf(stderr, "\tDiskDoubler and AutoDoubler\n");
#endif /* DD */
		exit(0);
	    }
	}
    }
    if(errflg) {
	usage();
	exit(1);
    }

    if(optind == argc) {
	infp = stdin;
    } else {
	if((infp = fopen(argv[optind], "r")) == NULL) {
	    (void)fprintf(stderr,"Can't open input file \"%s\"\n",argv[optind]);
	    exit(1);
	}
#ifdef SCAN
	do_idf(argv[optind], UNIX_NAME);
#endif /* SCAN */
    }

    if(info_only || verbose || query) {
	list++;
    }
    c = getc(infp);
    (void)ungetc(c, infp);
    switch(c) {
    case 0:
	macbinary();
	break;
#ifdef STF
    case 'R':
	if(verbose) {
	    (void)fprintf(stderr, "This is a \"ShrinkToFit\" packed file.\n");
	}
	stf(~(unsigned long)1);
	break;
#endif /* STF */
#ifdef PIT
    case 'P':
	if(verbose) {
	    (void)fprintf(stderr, "This is a \"PackIt\" archive.\n");
	}
	pit();
	break;
#endif /* PIT */
#ifdef SIT
    case 'S':
	if(verbose) {
	    (void)fprintf(stderr, "This is a \"StuffIt\" archive.\n");
	}
	sit();
	break;
#endif /* SIT */
#ifdef CPT
    case 1:
	if(verbose) {
	    (void)fprintf(stderr, "This is a \"Compactor\" archive.\n");
	}
	cpt();
	break;
#endif /* CPT */
    default:
	(void)fprintf(stderr, "Unrecognized archive type\n");
	exit(1);
    }
    exit(0);
    /* NOTREACHED */
}

static void usage()
{
    (void)fprintf(stderr, "Usage: macunpack [-%s] [filename]\n", options);
    (void)fprintf(stderr, "Use \"macunpack -H\" for help.\n");
}

