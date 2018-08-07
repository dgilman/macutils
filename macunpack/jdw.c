#include "macunpack.h"
#ifdef JDW
#include "jdw.h"
#include "globals.h"
#include "huffman.h"
#include "../fileio/wrfile.h"
#include "../fileio/machdr.h"
#include "../util/util.h"
#include "../util/masks.h"

extern void de_huffman();
extern void set_huffman();
extern void read_tree();
extern void clrhuff();

static void jdw_wrfile();
static void jdw_wrfork();
static void jdw_block();

void jdw(ibytes)
unsigned long ibytes;
{
    char fauth[5], ftype[5];
    int filel, i;
    unsigned int rsrcLength, dataLength;

    set_huffman(HUFF_BE);
    for(i = 0; i < 6; i++) (void)getb(infp);
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = 0;
    }
    for(i = 0; i < 4; i++) {
	info[I_TYPEOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_AUTHOFF + i] = getb(infp);
    }
    for(i = 0; i < 8; i++) {
	info[I_FLAGOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_DLENOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_RLENOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_CTIMOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_MTIMOFF + i] = getb(infp);
    }
    filel = getb(infp);
    info[I_NAMEOFF] = filel;
    i = filel;
    for(i = 1; i <= filel; i++) {
	info[I_NAMEOFF + i] = getb(infp);
    }
    (void)getb(infp);
    rsrcLength = get4(info + I_RLENOFF);
    dataLength = get4(info + I_DLENOFF);
    ibytes -= filel + 40;
    write_it = 1;
    if(list) {
	transname(info + I_NAMEOFF + 1, text, (int)info[I_NAMEOFF]);
	transname(info + I_TYPEOFF, ftype, 4);
	transname(info + I_AUTHOFF, fauth, 4);
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
    jdw_wrfile((unsigned long)rsrcLength, (unsigned long)dataLength);
}

static void jdw_wrfile(rsrcLength, dataLength)
unsigned long rsrcLength, dataLength;
{
    if(write_it) {
	define_name(text);
	start_info(info, rsrcLength, dataLength);
	start_data();
    }
    if(verbose) {
	(void)fprintf(stderr, "\tData: ");
    }
    jdw_wrfork(dataLength);
    if(write_it) {
	start_rsrc();
    }
    if(verbose) {
	(void)fprintf(stderr, ", Rsrc: ");
    }
    jdw_wrfork(rsrcLength);
    if(write_it) {
	end_file();
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
}

static void jdw_wrfork(length)
unsigned long length;
{
    int olength, ilength, i;
    unsigned long origlength, comprlength;

    if(length == 0) {
	(void)fprintf(stderr, "empty");
	return;
    }
    (void)fprintf(stderr, "Huffman compressed ");
    comprlength = 0;
    origlength = length;
    while(length > 0) {
	olength = getb(infp) & BYTEMASK;
	olength = (olength << 8) | (getb(infp) & BYTEMASK);
	ilength = getb(infp) & BYTEMASK;
	ilength = (ilength << 8) | (getb(infp) & BYTEMASK);
	if(write_it) {
	    jdw_block(olength);
	} else {
	    for(i = 0; i < ilength; i++) {
		(void)getb(infp);
	    }
	}
	comprlength += ilength + 4;
	length -= olength;
    }
    if(verbose) {
	(void)fprintf(stderr, "(%4.1f%%)", 100.0 * comprlength / origlength);
    }
}

static void jdw_block(olength)
int olength;
{
    bytesread = 0;
    read_tree();
    /* Put reading back at a word boundary! */
    while(bytesread & 3) {
	(void)getb(infp);
	bytesread++;
    }
    clrhuff();
    de_huffman((unsigned long)olength);
}
#else /* JDW */
int jdw; /* keep lint and some compilers happy */
#endif /* JDW */

