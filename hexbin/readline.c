#include "globals.h"
#include "readline.h"

char line[1024];	/* Allow a lot! */

/* Read a line.  Allow termination by CR or LF or both.  Also allow for
   a non-terminated line at end-of-file.  Returns 1 if a line is read,
   0 otherwise. */
int readline()
{
    int ptr = 0, c;

    while(1) {
	if(was_macbin && to_read-- <= 0) {
	    c = EOF;
	} else {
	    c = getc(ifp);
	}
	if(c == EOF || c == '\n' || c == '\r' || ptr == 1023) {
	    break;
	}
	line[ptr++] = c;
    }
    line[ptr++] = 0;
    if(c == EOF) {
	if(ptr == 1) {
	    return 0;
	} else {
	    return 1;
	}
    }
    c = getc(ifp);
    if(c != '\n' || c != '\r') {
	(void)ungetc(c, ifp);
    } else {
	to_read--;
    }
    return 1;
}

