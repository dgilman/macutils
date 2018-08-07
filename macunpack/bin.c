#include "macunpack.h"
#ifdef BIN
#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "../util/util.h"
#include "../util/masks.h"

extern void mcb();

void bin(header, data_size, UMcp)
char *header;
int data_size, UMcp;
{
    char hdr[INFOBYTES];
    unsigned long rsrcLength, dataLength;

    hdr[0] = getb(infp);
    (void)ungetc(hdr[0], infp);
    if(hdr[0] != 0) {
	if(!strncmp(header + I_AUTHOFF, "BnHq", 4) && hdr[0] == '(') {
	    do_indent(indent);
	    (void)fprintf(stderr, "Sorry, this is a fake BinHex 5.0 file.  ");
	    (void)fprintf(stderr, "Debinhex with hexbin first.\n");
#ifdef SCAN
	    do_error("macunpack: fake BinHex 5.0");
#endif /* SCAN */
	} else {
	    do_indent(indent);
	    (void)fprintf(stderr, "Sorry, contents not recognized.\n");
#ifdef SCAN
	    do_error("macunpack: contents not recognized");
#endif /* SCAN */
	}
	do_indent(indent);
	(void)fprintf(stderr, "Copying as a plain file.\n");
#ifdef SCAN
	do_idf("", COPY);
#endif /* SCAN */
	mcb(header, (unsigned long)in_data_size, (unsigned long)in_rsrc_size,
	    in_ds + in_rs);
	ds_skip = 0;
	rs_skip = 0;
	in_ds = 0;
	in_rs = 0;
	return;
    }
    if(fread(hdr, 1, INFOBYTES - 1, infp) != INFOBYTES) {
	(void)fprintf(stderr, "Can't read file header\n");
#ifdef SCAN
	do_error("macunpack: Can't read file header");
#endif /* SCAN */
	exit(1);
    }
    rsrcLength = get4(hdr + I_RLENOFF);
    dataLength = get4(hdr + I_DLENOFF);
    if(UMcp) {
	/* Why this?  Moreover, we are losing the bundle bit! */
	put4(hdr + I_RLENOFF, ++rsrcLength);
	put4(hdr + I_DLENOFF, ++dataLength);
    }
    mcb(hdr, rsrcLength, dataLength, data_size - INFOBYTES);
}
#else /* BIN */
int bin; /* keep lint and some compilers happy */
#endif /* BIN */

