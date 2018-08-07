#include "macunpack.h"
#ifdef DIA
#include "globals.h"
#include "dia.h"
#include "../util/curtime.h"
#include "../util/masks.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "../util/util.h"

extern char *malloc();
extern char *realloc();

static unsigned char *dia_archive;
static int dia_archive_size;
static int dia_max_archive_size;
static int dia_finfo;
static int dia_method;
static unsigned char *dia_archive_ptr;
static unsigned char *dia_header_ptr;
static unsigned char *dia_header_last;
static int dia_forklength;
static int dia_cforklength;
static unsigned char dia_bitbuf[BCHUNKSIZE];
static int dia_LZtab[BCHUNKSIZE];
static unsigned char *dia_bit_base;
static int dia_imask;

static void dia_folder();
static void dia_file();
static void dia_getlength();
static void dia_skipfork();
static void dia_getfork();
static void dia_getblock();
static int dia_decode();
static int dia_prevbit();

void dia(bin_hdr)
unsigned char *bin_hdr;
{
    int i, folder, nlength;
    unsigned char hdr;
    unsigned char *header;

    dir_skip = 0;
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = 0;
    }
    if(in_data_size > dia_max_archive_size) {
	if(dia_archive == NULL) {
	    dia_archive = (unsigned char *)malloc((unsigned)in_data_size);
	} else {
	    dia_archive = (unsigned char *)realloc((char *)dia_archive,
						   (unsigned)in_data_size);
	}
	if(dia_archive == 0) {
	    (void)fprintf(stderr, "Insufficient memory.\n");
	    exit(1);
	}
	dia_max_archive_size = in_data_size;
    }
    dia_archive_size = in_data_size;
    if(fread((char *)dia_archive, 1, in_data_size, infp) != in_data_size) {
	(void)fprintf(stderr, "Can't read archive.\n");
#ifdef SCAN
	do_error("macunpack: Can't read archive");
#endif /* SCAN */
	exit(1);
    }
    nlength = bin_hdr[I_NAMEOFF] & BYTEMASK;
    if(!strncmp((char *)bin_hdr + I_NAMEOFF + nlength - 1, " \272", 2)) {
	nlength -= 2;
    }
    info[I_NAMEOFF] = nlength;
    for(i = 1; i <= nlength; i++) {
	info[I_NAMEOFF + i] = bin_hdr[I_NAMEOFF + i];
    }
    hdr = *dia_archive;
    folder = hdr & IS_FOLDER;
    dia_finfo = hdr & F_INFO;
    if(hdr & VOLUME) {
	(void)fprintf(stderr, "Multi-segment archives not implemented.\n");
#ifdef SCAN
	do_error("macunpack: Multi-segment archive");
#endif /* SCAN */
	exit(1);
    }
    if(hdr & CRYPTED) {
	(void)fprintf(stderr, "Encrypted archives not implemented.\n");
#ifdef SCAN
	do_idf("", PROTECTED);
#endif /* SCAN */
	exit(1);
    }
    i = (hdr & N_BLOCKS) + 1;
    header = (unsigned char *)malloc((unsigned)(i * CHUNKSIZE));
    dia_archive_ptr = dia_archive + 1;
    dia_header_last = header;
    dia_method = 0;
    while(i-- > 0) {
	dia_getblock(&dia_archive_ptr, &dia_header_last);
    }
    dia_header_ptr = header;
    if(folder) {
	dia_folder((unsigned char *)NULL);
    } else {
	dia_file(*dia_header_ptr++, (unsigned char *)NULL);
    }
    free((char *)header);
}

static void dia_folder(name)
unsigned char *name;
{
    unsigned char lname[32];
    int i, length, doit;
    unsigned char indicator, *old_ptr;

    if(name != NULL) {
	for(i = 0; i < INFOBYTES; i++) {
	    info[i] = 0;
	}
	length = *name++ & REMAINS;
	info[I_NAMEOFF] = length;
	for(i = 1; i <= length; i++) {
	    info[I_NAMEOFF + i] = *name++;
	}
    } else {
	length = info[I_NAMEOFF];
    }
    if(dia_finfo) {
	dia_header_ptr += 20;
    }
    if(!dir_skip) {
	doit = 1;
	if(list) {
	    transname(info + I_NAMEOFF + 1, (char *)lname, length);
	    do_indent(indent);
	    (void)fprintf(stderr, "folder=\"%s\"", lname);
	    if(query) {
		doit = do_query();
	    } else {
		(void)fputc('\n', stderr);
	    }
	    if(doit) {
		indent++;
	    } else {
		dir_skip = 1;
	    }
	}
	if(doit && !info_only) {
	    do_mkdir((char *)lname, info);
	}
    } else {
	dir_skip++;
    }
    while(dia_header_ptr < dia_header_last) {
	indicator = *dia_header_ptr;
	if(indicator & LEAVE_FOLDER) {
	    *dia_header_ptr = indicator & ~LEAVE_FOLDER;
	    break;
	} else if(indicator & ONLY_FOLDER) {
	    if(indicator == ONLY_FOLDER) {
		dia_header_ptr++;
	    } else {
		*dia_header_ptr -= 1;
	    }
	    break;
	} else if(indicator & FOLDER) {
	    old_ptr = dia_header_ptr;
	    dia_header_ptr += (indicator & REMAINS) + 1;
	    dia_folder(old_ptr);
	} else {
	    dia_header_ptr++;
	    old_ptr = dia_header_ptr;
	    dia_header_ptr += (*old_ptr & REMAINS) + 1;
	    dia_file(indicator, old_ptr);
	}
    }
    if(!dir_skip) {
	if(doit) {
	    indent--;
	    if(!info_only) {
		enddir();
	    }
	    do_indent(indent);
	    (void)fprintf(stderr, "leaving folder \"%s\"\n", lname);
	}
    } else {
	dir_skip--;
    }
}

static void dia_file(indicator, name)
unsigned char indicator, *name;
{
    unsigned char lname[32];
    int i, length, doit;
    int n_data, n_rsrc;
    unsigned char *old_archive_ptr;
    char ftype[5], fauth[5];
    int dataLength, rsrcLength;
    int cdataLength, crsrcLength;
    int dataMethod, rsrcMethod;
    unsigned long curtime;

    if(name != NULL) {
	for(i = 0; i < INFOBYTES; i++) {
	    info[i] = 0;
	}
	length = *name++ & REMAINS;
	info[I_NAMEOFF] = length;
	for(i = 1; i <= length; i++) {
	    info[I_NAMEOFF + i] = *name++;
	}
    } else {
	length = info[I_NAMEOFF];
    }
    for(i = 0; i < 4; i++) {
	info[I_TYPEOFF + i] = *dia_header_ptr++;
    }
    for(i = 0; i < 4; i++) {
	info[I_AUTHOFF + i] = *dia_header_ptr++;
    }
    if(indicator & DATE_PRESENT) {
	for(i = 0; i < 4; i++) {
	    info[I_CTIMOFF + i] = *dia_header_ptr++;
	}
	for(i = 0; i < 4; i++) {
	    info[I_MTIMOFF + i] = *dia_header_ptr++;
	}
    } else {
	curtime = (unsigned long)time((time_t *)0) + TIMEDIFF;
	put4(info + I_CTIMOFF, curtime);
	put4(info + I_MTIMOFF, curtime);
    }
    if(dia_finfo) {
	for(i = 0; i < 6; i++) {
	    info[I_FLAGOFF + i] = *dia_header_ptr++;
	}
    }
    n_data = 0;
    if(indicator & HAS_DATA) {
	if(indicator & SHORT_DATA) {
	    n_data = 1;
	} else {
	    n_data = *dia_header_ptr++ + 1;
	}
    }
    n_rsrc = 0;
    if(indicator & HAS_RSRC) {
	if(indicator & SHORT_RSRC) {
	    n_rsrc = 1;
	} else {
	    n_rsrc = *dia_header_ptr++ + 1;
	}
    }
    if(!dir_skip) {
	old_archive_ptr = dia_archive_ptr;
	dia_getlength(n_data);
	dataLength = dia_forklength;
	cdataLength = dia_cforklength;
	dataMethod = dia_method;
	dia_getlength(n_rsrc);
	rsrcLength = dia_forklength;
	crsrcLength = dia_cforklength;
	rsrcMethod = dia_method;
	dia_archive_ptr = old_archive_ptr;
	put4(info + I_DLENOFF, (unsigned long)dataLength);
	put4(info + I_RLENOFF, (unsigned long)rsrcLength);
	if(list) {
	    transname(info + I_NAMEOFF + 1, (char *)lname, length);
	    do_indent(indent);
	    transname(info + I_TYPEOFF, ftype, 4);
	    transname(info + I_AUTHOFF, fauth, 4);
	    (void)fprintf(stderr,
		    "name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		    lname, ftype, fauth, (long)dataLength, (long)rsrcLength);
	    if(info_only) {
		doit = 0;
	    } else {
		doit = 1;
	    }
	    if(query) {
		doit = do_query();
	    } else {
		(void)fputc('\n', stderr);
	    }
	} else {
	    doit = 1;
	}
    } else {
	dia_skipfork(n_data);
	dia_skipfork(n_rsrc);
	return;
    }
    if(doit) {
	define_name((char *)lname);
	start_info(info, (unsigned long)rsrcLength, (unsigned long)dataLength);
    }
    if(verbose) {
	(void)fprintf(stderr, "\tData: ");
	if(dataLength == 0) {
	    (void)fprintf(stderr, "empty");
	} else if(dataMethod == NOCOMP) {
	    (void)fprintf(stderr, "No compression");
	} else {
	    if(dataMethod != COMP) {
		(void)fprintf(stderr, "Partial ");
	    }
	    (void)fprintf(stderr, "LZFK compressed (%4.1f%%)",
		100.0 * cdataLength / dataLength);
	}
    }
    if(doit) {
	start_data();
	dia_getfork(n_data);
    } else {
	dia_skipfork(n_data);
    }
    if(verbose) {
	(void)fprintf(stderr, ", Rsrc: ");
	if(rsrcLength == 0) {
	    (void)fprintf(stderr, "empty");
	} else if(rsrcMethod == NOCOMP) {
	    (void)fprintf(stderr, "No compression");
	} else {
	    if(rsrcMethod != COMP) {
		(void)fprintf(stderr, "Partial ");
	    }
	    (void)fprintf(stderr, "LZFK compressed (%4.1f%%)",
		100.0 * crsrcLength / rsrcLength);
	}
    }
    if(doit) {
	start_rsrc();
	dia_getfork(n_rsrc);
    } else {
	dia_skipfork(n_rsrc);
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
    if(doit) {
	end_file();
    }
}

static void dia_getlength(nblocks)
int nblocks;
{
    int length;
    unsigned char *arch_ptr, *block_ptr;
    unsigned char block[CHUNKSIZE];

    dia_method = 0;
    dia_forklength = 0;
    dia_cforklength = 0;
    while(nblocks > 1) {
	nblocks--;
	length = get2((char *)dia_archive_ptr);
	if(length >= 0x8000) {
	    length = 0x10000 - length;
	    dia_method |= NOCOMP;
	} else {
	    dia_method |= COMP;
	}
	dia_forklength += CHUNKSIZE;
	dia_cforklength += length + 2;
	dia_archive_ptr += length + 2;
    }
    if(nblocks == 1) {
	arch_ptr = dia_archive_ptr;
	block_ptr = block;
	dia_getblock(&arch_ptr, &block_ptr);
	dia_forklength += block_ptr - block;
	dia_cforklength += arch_ptr - dia_archive_ptr;
	dia_archive_ptr = arch_ptr;
    }
}

static void dia_skipfork(nblocks)
int nblocks;
{
    int length;

    while(nblocks-- > 0) {
	length = get2((char *)dia_archive_ptr);
	if(length >= 0x8000) {
	    length = 0x10000 - length;
	}
	dia_archive_ptr += length + 2;
    }
}

static void dia_getfork(nblocks)
int nblocks;
{
    while(nblocks-- > 0) {
	dia_getblock(&dia_archive_ptr, (unsigned char **)&out_ptr);
    }
}

static void dia_getblock(archive_ptr, block_ptr)
unsigned char **archive_ptr, **block_ptr;
{
    int length, i;
    unsigned char *arch_ptr, *bl_ptr;

    arch_ptr = *archive_ptr;
    bl_ptr = *block_ptr;
    length = get2((char *)arch_ptr);
    arch_ptr += 2;
    if(length >= 0x8000) {
	length = 0x10000 - length;
	for(i = 0; i < length; i++) {
	    *bl_ptr++ = *arch_ptr++;
	}
	*block_ptr += length;
	dia_method |= NOCOMP;
    } else {
	*block_ptr += dia_decode(arch_ptr, bl_ptr, length);
	dia_method |= COMP;
    }
    *archive_ptr += length + 2;
}

static int dia_decode(ibuff, obuff, in_length)
unsigned char *ibuff, *obuff; int in_length;
{
    int nbits, set_zero, i, j;
    unsigned char *bitbuf_ptr;
    int count[4];
    int *LZtab_ptr;
    unsigned char *out_ptr, *buf_ptr, *in_ptr;
    int omask;
    int LZcount;
    int *length_ptr, *nchars_ptr;
    int *offsn_ptr, length, nchars, offset;
    int *offs_ptr[4];
    int nwords;

    in_ptr = ibuff + in_length;
    nbits = *--in_ptr;
    nbits |= (*--in_ptr << 8);
    if(nbits == WORDMASK) {
	nbits = *--in_ptr;
	nbits |= (*--in_ptr << 8);
	nbits = nbits + WORDMASK;
    }
    bitbuf_ptr = dia_bitbuf + BCHUNKSIZE;
    *--bitbuf_ptr = *--in_ptr;
    set_zero = 0;
    dia_bit_base = bitbuf_ptr;
    dia_imask = 1 << (7 - (nbits & 7));
    if(dia_prevbit()) {
	set_zero = 1;
    }
    for(i = 0; i < nbits; i++) {
	if(set_zero) {
	    *--bitbuf_ptr = 0;
	} else {
	    *--bitbuf_ptr = *--in_ptr;
	}
	if(dia_prevbit()) {
	    set_zero = !set_zero;
	}
    }
    /* Now we have the bits in longitudal order; reorder them */
    nwords = ((dia_bit_base - bitbuf_ptr) >> 1);
    for(i = 0; i < nwords; i++) {
	dia_LZtab[i] = 0;
    }
    omask = 1;
    for(i = 0; i < 16; i++) {
	j = nwords;
	LZtab_ptr = dia_LZtab + nwords;
	while(j-- > 0) {
	    LZtab_ptr--;
	    if(dia_prevbit()) {
		*LZtab_ptr |= omask;
	    }
	}
	omask <<= 1;
    }
    LZcount = nwords / 3;
    /*  At this point we have in LZtab LZcount triples.  Each triple consists
	of the following parts:
		nchars:	the number of characters to take from input
		length:	the number of characters - 1 to copy from output
		offset:	the offset in the output buffer
	The ordering is as follows:
	1.	lengths
	2.	nchars
	3.	offsets for length 0
	4.	offsets for length 1
	5.	offsets for length 2
	6.	offsets for length 3
	7.	offsets for other lengths
    */
    /*	Now first count the occurences of lengths 0 to 3 */
    count[0] = 0;
    count[1] = 0;
    count[2] = 0;
    count[3] = 0;
    for(i = 0; i < LZcount; i++) {
	if((j = dia_LZtab[i]) < 4) {
	    count[j]++;
	}
    }
    length_ptr = dia_LZtab;
    nchars_ptr = dia_LZtab + LZcount;
    offs_ptr[0] = nchars_ptr + LZcount;
    offs_ptr[1] = offs_ptr[0] + count[0];
    offs_ptr[2] = offs_ptr[1] + count[1];
    offs_ptr[3] = offs_ptr[2] + count[2];
    offsn_ptr = offs_ptr[3] + count[3];
    out_ptr = obuff;
    for(i = 0; i < LZcount; i++) {
	length = *length_ptr++;
	nchars = *nchars_ptr++;
	if(length < 4) {
	    offset = *offs_ptr[length]++;
	} else {
	    offset = *offsn_ptr++;
	}
	while(nchars-- > 0) {
	    *out_ptr++ = *ibuff++;
	}
	buf_ptr = out_ptr - length - offset - 1;
	while(length-- >= 0) {
	    *out_ptr++ = *buf_ptr++;
	}
    }
    i = in_ptr - ibuff;
    while(i-- > 0) {
	*out_ptr++ = *ibuff++;
    }
    return out_ptr - obuff;
}

static int dia_prevbit()
{
    int c;

    if(dia_imask == 0x100) {
	dia_bit_base--;
	dia_imask = 1;
    }
    c = *dia_bit_base & dia_imask;
    dia_imask <<= 1;
    return c;
}
#else /* DIA */
int dia; /* keep lint and some compilers happy */
#endif /* DIA */

