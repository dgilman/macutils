#include <stdio.h>
#include "../fileio/machdr.h"
#include "../fileio/rdfile.h"
#include "../fileio/rdfileopt.h"
#include "../util/patchlevel.h"

extern char *malloc();
extern char *realloc();
extern char *strcat();
extern void exit();
extern void transname();
extern void do_indent();

#define LOCALOPT	"ilqVH"

static void usage();

static char options[128];
static char *dir_stack;
static int dir_ptr = -64;
static int dir_max;

int main(argc, argv)
int argc;
char **argv;
{
    int c, i, j, n;
    extern int optind;
    extern char *optarg;
    int errflg;
    char text[32], ftype[5], fauth[5];
    int dir_skip = 0, write_it, query = 0, list = 0, info_only = 0;
    int indent = 0;

    (void)strcat(options, get_rdfileopt());
    (void)strcat(options, LOCALOPT);
    errflg = 0;

    while((c = getopt(argc, argv, options)) != EOF) {
	if(!rdfileopt((char)c)) {
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
		give_rdfileopt();
		(void)fprintf(stderr, "Macstream specific options:\n");
		(void)fprintf(stderr,
			"-i:\tgive information only, do not write\n");
		(void)fprintf(stderr, "-l:\tgive listing\n");
		(void)fprintf(stderr,
			"-q:\tquery for every file/folder before writing\n");
		(void)fprintf(stderr,
			"-V:\tgive information about this version\n");
		(void)fprintf(stderr, "-H:\tthis message\n");
		(void)fprintf(stderr, "Default is silent writing\n");
		exit(0);
	    case 'V':
		(void)fprintf(stderr, "Version %s, ", VERSION);
		(void)fprintf(stderr, "patchlevel %d", PATCHLEVEL);
		(void)fprintf(stderr, "%s.\n", get_minb());
		exit(0);
	    }
	}
    }
    if(errflg || optind == argc) {
	usage();
	exit(1);
    }

    if(info_only || query) {
	list++;
    }

    setup(argc - optind, argv + optind);
    while((i = nextfile()) != ISATEND) {
	if(dir_skip) {
	    if(i == ISDIR) {
		dir_skip++;
	    } else if(i == ENDDIR) {
		dir_skip--;
	    }
	    continue;
	}

	write_it = 1;
	n = file_info[I_NAMEOFF] & 0x7f;
	transname(file_info + I_NAMEOFF + 1, text, n);
	if(i == ISFILE) {
	    transname(file_info + I_TYPEOFF, ftype, 4);
	    transname(file_info + I_AUTHOFF, fauth, 4);
	}
	if(list) {
	    if(i == ISFILE) {
		do_indent(indent);
		(void)fprintf(stderr,
		    "name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		    text, ftype, fauth, (long)data_size, (long)rsrc_size);
	    } else if(i == ISDIR) {
		do_indent(indent);
		dir_ptr += 64;
		if(dir_ptr == dir_max) {
		    if(dir_max == 0) {
			dir_stack = malloc(64);
		    } else {
			dir_stack = realloc(dir_stack, (unsigned)dir_max + 64);
		    }
		    dir_max += 64;
		    if(dir_stack == NULL) {
			(void)fprintf(stderr, "Insufficient memory\n");
			exit(1);
		    }
		}
		for(j = 0; j <= n; j++) {
		    dir_stack[dir_ptr + j] = text[j];
		}
		(void)fprintf(stderr, "folder=\"%s\"", text);
		indent++;
	    } else {
		indent--;
		do_indent(indent);
		(void)fprintf(stderr, "leaving folder \"%s\"",
			dir_stack + dir_ptr);
		dir_ptr -= 64;
	    }
	    if(info_only) {
		write_it = 0;
	    }
	    if(query) {
		if(i != ENDDIR) {
		    write_it = do_query();
		} else {
		    (void)fputc('\n', stderr);
		}
		if(!write_it && i == ISDIR) {
		    dir_skip = 1;
		    indent--;
		    dir_ptr -= 64;
		}
	    } else {
		(void)fputc('\n', stderr);
	    }
	}

	if(write_it) {
	    (void)fwrite(file_info, 1, 128, stdout);
	    if(i == ISFILE) {
		if(data_size != 0) {
		    (void)fwrite(data_fork, 1, data_size, stdout);
		    i = (((data_size + 127) >> 7) << 7) - data_size;
		    while(i-- > 0) {
			(void)fputc(0, stdout);
		    }
		}
		if(rsrc_size != 0) {
		    (void)fwrite(rsrc_fork, 1, rsrc_size, stdout);
		    i = (((rsrc_size + 127) >> 7) << 7) - rsrc_size;
		    while(i-- > 0) {
			(void)fputc(0, stdout);
		    }
		}
	    }
	}
    }
    exit(0);
    /* NOTREACHED */
}

static void usage()
{
    (void)fprintf(stderr, "Usage: macstream [-%s] files\n", options);
    (void)fprintf(stderr, "Use \"macstream -H\" for help.\n");
}

