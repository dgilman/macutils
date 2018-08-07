#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/util.h"
#include "../util/masks.h"

extern char *malloc();
extern char *realloc();

static char *dir_stack;
static int dir_ptr = -64;
static int dir_max;

void dir(hdr)
char *hdr;
{
int doit;

    if((hdr[I_NAMEOFF] & BYTEMASK) == 0x80) {
	if(dir_skip) {
	    dir_skip--;
	    return;
	}
	indent--;
	if(list) {
	    do_indent(indent);
	    (void)fprintf(stderr, "leaving folder \"%s\"\n",
		    dir_stack + dir_ptr);
	}
	if(!info_only) {
	    enddir();
	}
	dir_ptr -= 64;
	return;
    }
    if(dir_skip) {
	dir_skip++;
	return;
    }
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
    transname(hdr + I_NAMEOFF + 1, dir_stack + dir_ptr,
	      (int)(hdr[I_NAMEOFF] & 0x7f));
    doit = 1;
    if(list) {
	do_indent(indent);
	(void)fprintf(stderr, "folder=\"%s\"", dir_stack + dir_ptr);
	if(query) {
	    doit = do_query();
	} else {
	    (void)fputc('\n', stderr);
	}
    }
    if(!doit) {
	dir_ptr -= 64;
	dir_skip = 1;
	return;
    }
    if(!info_only) {
	do_mkdir(dir_stack + dir_ptr, hdr);
    }
    indent++;
}

