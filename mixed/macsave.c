#include "globals.h"
#include "../util/patchlevel.h"
#include "../fileio/wrfile.h"
#include "../fileio/wrfileopt.h"
#include "../util/util.h"

#define LOCALOPT	"ilqVH"

extern char *strcat();
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
    set_s_wrfileopt(1);
    (void)strcat(options, get_wrfileopt());
    (void)strcat(options, LOCALOPT);
    errflg = 0;

    while((c = getopt(argc, argv, options)) != EOF) {
	if(!wrfileopt((char)c)) {
	    switch(c) {
	    case 'l':
		list++;
		break;
	    case 'q':
		query++;
		break;
	    case 'i':
		info_only++;
		break;
	    case '?':
		errflg++;
		break;
	    case 'H':
		give_wrfileopt();
		(void)fprintf(stderr, "Macsave specific options:\n");
		(void)fprintf(stderr,
			"-i:\tgive information only, do not save\n");
		(void)fprintf(stderr, "-l:\tgive listing\n");
		(void)fprintf(stderr,
			"-q:\tquery for every file/folder before saving\n");
		(void)fprintf(stderr,
			"-V:\tgive information about this version\n");
		(void)fprintf(stderr, "-H:\tthis message\n");
		(void)fprintf(stderr, "Default is silent saving\n");
		exit(0);
	    case 'V':
		(void)fprintf(stderr, "Version %s, ", VERSION);
		(void)fprintf(stderr, "patchlevel %d", PATCHLEVEL);
		(void)fprintf(stderr, "%s.\n", get_mina());
		exit(0);
	    }
	}
    }
    if(errflg || optind != argc) {
	usage();
	exit(1);
    }

    infp = stdin;

    if(info_only || query) {
	list++;
    }
    c = getc(infp);
    (void)ungetc(c, infp);
    switch(c) {
    case 0:
	macbinary();
	break;
    default:
	(void)fprintf(stderr, "Input is not MacBinary\n");
	exit(1);
    }
    exit(0);
    /* NOTREACHED */
}

static void usage()
{
    (void)fprintf(stderr, "Usage: macsave [-%s]\n", options);
    (void)fprintf(stderr, "Use \"macsave -H\" for help.\n");
}

