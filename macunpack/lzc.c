#include "macunpack.h"
#ifdef LZC
#include "globals.h"
#include "lzc.h"
#include "../util/util.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../util/masks.h"

extern void de_compress();
extern void core_compress();
extern void mcb();

static void lzc_zivm();
static void lzc_wrfile();
static void lzc_zivu();

void lzc(ohdr)
char *ohdr;
{
    core_compress((char *)NULL);
    if(!strncmp(ohdr + I_TYPEOFF, "ZIVM", 4)) {
	lzc_zivm(ohdr);
    } else {
	lzc_zivu(ohdr);
    }
}

static void lzc_zivm(ohdr)
char *ohdr;
{
    char hdr[HEADERBYTES];
    unsigned long dataLength, rsrcLength, dataCLength, rsrcCLength;
    char ftype[5], fauth[5];

    if(fread(hdr, 1, HEADERBYTES, infp) != HEADERBYTES) {
	(void)fprintf(stderr, "Can't read file header\n");
#ifdef SCAN
	do_error("macunpack: Can't read file header");
#endif /* SCAN */
	exit(1);
    }
    if(strncmp(hdr, MAGIC1, 4)) {
	(void)fprintf(stderr, "Magic header mismatch\n");
#ifdef SCAN
	do_error("macunpack: Magic header mismatch");
#endif /* SCAN */
	exit(1);
    }
    dataLength = get4(hdr + C_DLENOFF);
    dataCLength = get4(hdr + C_DLENOFFC);
    rsrcLength = get4(hdr + C_RLENOFF);
    rsrcCLength = get4(hdr + C_RLENOFFC);

    write_it = 1;
    if(list) {
	copy(info, ohdr, INFOBYTES);
	copy(info + I_TYPEOFF, hdr + C_TYPEOFF, 4);
	copy(info + I_AUTHOFF, hdr + C_AUTHOFF, 4);
	copy(info + I_DLENOFF, hdr + C_DLENOFF, 4);
	copy(info + I_RLENOFF, hdr + C_RLENOFF, 4);
	copy(info + I_CTIMOFF, hdr + C_CTIMOFF, 4);
	copy(info + I_MTIMOFF, hdr + C_MTIMOFF, 4);
	copy(info + I_FLAGOFF, hdr + C_FLAGOFF, 8);

	transname(ohdr + I_NAMEOFF + 1, text, (int)ohdr[I_NAMEOFF]);
	transname(hdr + C_TYPEOFF, ftype, 4);
	transname(hdr + C_AUTHOFF, fauth, 4);
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
    }
    if(write_it) {
	start_info(info, rsrcLength, dataLength);
    }
    if(verbose) {
	(void)fprintf(stderr, "\tData: ");
    }
    if(write_it) {
	start_data();
    }
    lzc_wrfile(dataLength, dataCLength);
    if(verbose) {
	(void)fprintf(stderr, ", Rsrc: ");
    }
    if(write_it) {
	start_rsrc();
    }
    lzc_wrfile(rsrcLength, rsrcCLength);
    if(write_it) {
	end_file();
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
}

static void lzc_wrfile(obytes, ibytes)
unsigned long obytes, ibytes;
{
    int n, nbits;
    char subheader[3];

    if(ibytes == 0) {
	if(verbose) {
	    (void)fprintf(stderr, "empty");
	}
	return;
    }
    if(ibytes == obytes) {
	if(verbose) {
	    (void)fprintf(stderr, "No compression");
	}
	if(write_it) {
	    n = fread(out_buffer, 1, (int)ibytes, infp);
	    if(n != ibytes) {
		(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		do_error("macunpack: Premature EOF");
#endif /* SCAN */
		exit(1);
	    }
	} else {
	    n = ibytes;
	    while(n-- > 0) {
		if(getc(infp) == EOF) {
		    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		    do_error("macunpack: Premature EOF");
#endif /* SCAN */
		    exit(1);
		}
	    }
	}
    } else {
	if(fread(subheader, 1, 3, infp) != 3) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("macunpack: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	if(strncmp(subheader, MAGIC2, 2)) {
	    (void)fprintf(stderr, "Magic subheader mismatch\n");
#ifdef SCAN
	    do_error("macunpack: Magic subheader mismatch");
#endif /* SCAN */
	    exit(1);
	}
	nbits = subheader[2] & 0x7f;
	if(verbose) {
	    (void)fprintf(stderr, "LZC(%d) compressed (%4.1f%%)",
		    nbits, 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    de_compress(ibytes - 3, nbits);
	} else {
	    n = ibytes - 3;
	    while(n-- > 0) {
		if(getc(infp) == EOF) {
		    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		    do_error("macunpack: Premature EOF");
#endif /* SCAN */
		    exit(1);
		}
	    }
	}
    }
}

static void lzc_zivu(ohdr)
char *ohdr;
{
    (void)fprintf(stderr,
	    "\tMacCompress(Unix) not yet implemented, copied as MacBinary\n");
    mcb(ohdr, (unsigned long)in_rsrc_size, (unsigned long)in_data_size,
	in_ds + in_rs);
}
#else /* LZC */
int lzc; /* keep lint and some compilers happy */
#endif /* LZC */

