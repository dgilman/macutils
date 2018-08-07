#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/masks.h"
#include "../util/util.h"

static int mcb_read;

static void mcb_wrfile();

void mcb(hdr, rsrcLength, dataLength, toread)
char *hdr;
unsigned long rsrcLength, dataLength;
int toread;
{
    register int i;
    int n;
    char ftype[5], fauth[5];

    mcb_read = toread;
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = hdr[i];
    }

    n = hdr[I_NAMEOFF] & BYTEMASK;
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    info[I_NAMEOFF] = n;
    transname(hdr + I_NAMEOFF + 1, text, n);
    if(hdr[I_LOCKOFF] & 1) {
	hdr[I_FLAGOFF + 1] = PROTCT_MASK;
	hdr[I_LOCKOFF] &= ~1;
    }

    write_it = 1;
    if(list) {
	transname(hdr + I_TYPEOFF, ftype, 4);
	transname(hdr + I_AUTHOFF, fauth, 4);
	do_indent(indent);
	(void)fprintf(stderr,
		"name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		text, ftype, fauth, (long)dataLength, (long)rsrcLength);
	if(info_only) {
	    write_it = 0;
	}
	if(query) {
	    write_it = do_query();
	} else {
	    (void)fputc('\n', stderr);
	}
    }

    if(write_it) {
	define_name(text);
	start_info(info, rsrcLength, dataLength);
	start_data();
    }
    mcb_wrfile(dataLength);
    if(write_it) {
	start_rsrc();
    }
    mcb_wrfile(rsrcLength);
    if(write_it) {
	end_file();
    }
}

static void mcb_wrfile(ibytes)
unsigned long ibytes;
{
    int n;

    if(write_it) {
	if(ibytes == 0) {
	    return;
	}
	n = fread(out_buffer, 1, (int)ibytes, infp);
	if(n != ibytes) {
	    (void)fprintf(stderr, "Premature EOF\n");
	    exit(1);
	}
	mcb_read -= n;
	n = ((n + 127) / 128) * 128 - n;
	if(n > mcb_read) {
	    n = mcb_read;
	}
	mcb_read -= n;
	while(n-- > 0) {
	    if(getc(infp) == EOF) {
		(void)fprintf(stderr, "Premature EOF\n");
		exit(1);
	    }
	}
    } else {
	n = ((ibytes + 127) / 128) * 128;
	if(n > mcb_read) {
	    n = mcb_read;
	}
	mcb_read -= n;
	while(n-- > 0) {
	    if(getc(infp) == EOF) {
		(void)fprintf(stderr, "Premature EOF\n");
		exit(1);
	    }
	}
    }
}

