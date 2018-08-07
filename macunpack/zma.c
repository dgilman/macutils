#include "macunpack.h"
#ifdef ZMA
#include "globals.h"
#include "zma.h"
#include "crc.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "../util/masks.h"
#include "../util/util.h"

extern char *malloc();
extern char *realloc();
extern void de_lzh();

/* We do allow for possible backpointing, so we allocate the archive in core */
static char *zma_archive;
static char *zma_current;
static char *zma_filestart;
static unsigned long zma_length;
static long zma_archlength;

static int zma_filehdr();
static void zma_folder();
static void zma_mooz();
static void zma_wrfile();
static void zma_nocomp();
static void zma_lzh();

void zma(start, length)
    char *start;
    unsigned long length;
{
    struct fileHdr filehdr;
    int i, toread;

    if(length != 0) {
	if(zma_archlength < length) {
	    if(zma_archlength == 0) {
		zma_archive = malloc((unsigned)length);
	    } else {
		zma_archive = realloc(zma_archive, (unsigned)length);
	    }
	    zma_archlength = length;
	    if(zma_archive == NULL) {
		(void)fprintf(stderr, "Insufficient memory, aborting\n");
		exit(1);
	    }
	}
	if(fread(zma_archive, 1, (int)length, infp) != length) {
	    (void)fprintf(stderr, "Can't read archive.\n");
#ifdef SCAN
	    do_error("macunpack: Can't read archive");
#endif /* SCAN */
	    exit(1);
	}
	zma_length = get4(zma_archive + ZMAHDRS + 1);
	if(zma_length != length) {
	    (void)fprintf(stderr, "Archive length mismatch.\n");
#ifdef SCAN
	    do_error("macunpack: Archive length mismatch");
#endif /* SCAN */
	    exit(1);
	}
    } else {
	zma_length =  get4(start + ZMAHDRS + 1);
	if(zma_archlength < zma_length) {
	    if(zma_archlength == 0) {
		zma_archive = malloc((unsigned)zma_length);
	    } else {
		zma_archive = realloc(zma_archive, (unsigned)zma_length);
	    }
	    zma_archlength = zma_length;
	    if(zma_archive == NULL) {
		(void)fprintf(stderr, "Insufficient memory, aborting\n");
		exit(1);
	    }
	}
	if(zma_archive == NULL) {
	    (void)fprintf(stderr, "Insufficient memory, aborting\n");
	    exit(1);
	}
	for(i = 0; i <= ZMAHDRS2; i++) {
	    zma_archive[i] = start[i];
	}
	toread = zma_length - ZMAHDRS2 - 1;
	if(fread(zma_archive + ZMAHDRS2 + 1, 1, toread, infp) != toread) {
	    (void)fprintf(stderr, "Can't read archive.\n");
#ifdef SCAN
	    do_error("macunpack: Can't read archive");
#endif /* SCAN */
	    exit(1);
	}
    }
    /* Consistency checks */
    if(zma_archive[0] != 0) {
	(void)fprintf(stderr, "Not a \"Zoom\" archive after all, aborting\n");
	exit(1);
    }
    if(strncmp(zma_archive + 1, ZMAHDR, ZMAHDRS)) {
	(void)fprintf(stderr, "Not a \"Zoom\" archive after all, aborting\n");
	exit(1);
    }
    zma_current = zma_archive + 8;
    updcrc = arc_updcrc;
    crcinit = arc_crcinit;
    while(zma_current != zma_archive) {
	if(zma_filehdr(&filehdr, 0) == -1) {
	    (void)fprintf(stderr, "Can't find file header./n");
#ifdef SCAN
	    do_error("macunpack: Can't find file header");
#endif /* SCAN */
	    exit(1);
	}
	zma_filestart = zma_current + filehdr.hlen;
	if(filehdr.what == z_dir) {
	    zma_folder(filehdr);
	} else {
	    zma_mooz(filehdr);
	}
	zma_current = zma_archive + filehdr.next;
    }
}

static int zma_filehdr(f, skip)
struct fileHdr *f;
int skip;
{
    register int i;
    int n;
    char ftype[5], fauth[5];

    if(zma_current - zma_archive + Z_HDRSIZE > zma_length) {
	return -1;
    }
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = '\0';
    }

    n = zma_current[Z_FNAME] & BYTEMASK;
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    info[I_NAMEOFF] = n;
    copy(info + I_NAMEOFF + 1, zma_current + Z_FNAME + 1, n);
    transname(zma_current + Z_FNAME + 1, text, n);

    f->what = zma_current[Z_WHAT];
    f->rsrcLength = get4(zma_current + Z_URLEN);
    f->dataLength = get4(zma_current + Z_UDLEN);
    f->compRLength = get4(zma_current + Z_CRLEN);
    f->compDLength = get4(zma_current + Z_CDLEN);
    f->rsrcCRC = get2(zma_current + Z_RCRC);
    f->dataCRC = get2(zma_current + Z_DCRC);
    f->hlen = zma_current[Z_HLEN];
    f->next = get4(zma_current + Z_NEXT);
    if(f->what == z_dir) { /* A hack */
	f->conts = get4(zma_current + Z_AUTH);
    }
    /* Set rsrc fork sizes correctly */
    f->rsrcLength -= f->dataLength;
    f->compRLength -= f->compDLength;

    write_it = !skip;
    if(f->what & 0x80) {
	write_it = 0;
	f->what = -f->what;
	f->deleted = 1;
	return 0;
    }
    f->deleted = 0;
    if(list && !skip) {
	do_indent(indent);
	if(f->what == z_dir) {
	    (void)fprintf(stderr, "folder=\"%s\"", text);
	} else {
	    transname(zma_current + Z_TYPE, ftype, 4);
	    transname(zma_current + Z_AUTH, fauth, 4);
	    (void)fprintf(stderr,
		    "name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		    text, ftype, fauth,
		    (long)f->dataLength, (long)f->rsrcLength);
	}
	switch(f->what) {
	case z_plug:
	    (void)fputc('\n', stderr);
	    (void)fprintf(stderr,
		    "\tFile uses custom processing, cannot handle.\n");
	    write_it = 0;
	    return 0;
	case z_dir:
	case z_file:
	case z_plain:
	    break;
	default:
	    (void)fputc('\n', stderr);
	    (void)fprintf(stderr,
		    "\tEh, do not understand this (%d); skipped.\n", f->what);
	    write_it = 0;
	    return 0;
	}

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

	if(f->what != z_dir) {
	    copy(info + I_TYPEOFF, zma_current + Z_TYPE, 4);
	    copy(info + I_AUTHOFF, zma_current + Z_AUTH, 4);
	    copy(info + I_FLAGOFF, zma_current + Z_FLAGS, 2);
	    copy(info + I_DLENOFF, zma_current + Z_UDLEN, 4);
	    put4(zma_current + Z_URLEN, f->rsrcLength);
	    copy(info + I_RLENOFF, zma_current + Z_URLEN, 4);
	    copy(info + I_CTIMOFF, zma_current + Z_MDATE, 4);
	    copy(info + I_MTIMOFF, zma_current + Z_MDATE, 4);
	}
    }
    return 1;
}

static void zma_folder(fhdr)
struct fileHdr fhdr;
{
    int i;
    char loc_name[64];
    struct fileHdr filehdr;

    for(i = 0; i < 64; i++) {
	loc_name[i] = text[i];
    }
    zma_current = zma_archive + fhdr.conts;
    if(write_it || info_only) {
	if(write_it) {
	    do_mkdir(text, info);
	}
	indent++;
	while(zma_current != zma_archive) {
	    if(zma_filehdr(&filehdr, 0) == -1) {
		(void)fprintf(stderr, "Can't find file header.\n");
#ifdef SCAN
		do_error("macunpack: Can't find file header");
#endif /* SCAN */
		exit(1);
	    }
	    zma_filestart = zma_current + filehdr.hlen;
	    if(filehdr.what == z_dir) {
		zma_folder(filehdr);
	    } else {
		zma_mooz(filehdr);
	    }
	    zma_current = zma_archive + filehdr.next;
	}
	if(write_it) {
	    enddir();
	}
	indent--;
	if(list) {
	    do_indent(indent);
	    (void)fprintf(stderr, "leaving folder \"%s\"\n", loc_name);
	}
    }
}

static void zma_mooz(filehdr)
struct fileHdr filehdr;
{
    unsigned long crc;

    if(write_it) {
	start_info(info, filehdr.rsrcLength, filehdr.dataLength);
    }
    if(verbose) {
	(void)fprintf(stderr, "\tData: ");
    }
    if(write_it) {
	start_data();
    }
    zma_wrfile(filehdr.compDLength, filehdr.dataLength, filehdr.what);
    if(write_it) {
	crc = (*updcrc)(INIT_CRC, out_buffer, filehdr.dataLength);
	if(filehdr.dataCRC != crc) {
	    (void)fprintf(stderr,
		"CRC error on data fork: need 0x%04x, got 0x%04x\n",
		(int)filehdr.dataCRC, (int)crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on data fork");
#endif /* SCAN */
	    exit(1);
	}
    }
    if(verbose) {
	(void)fprintf(stderr, ", Rsrc: ");
    }
    if(write_it) {
	start_rsrc();
    }
    zma_wrfile(filehdr.compRLength, filehdr.rsrcLength, filehdr.what);
    if(write_it) {
	crc = (*updcrc)(INIT_CRC, out_buffer, filehdr.rsrcLength);
	if(filehdr.rsrcCRC != crc) {
	    (void)fprintf(stderr,
		"CRC error on resource fork: need 0x%04x, got 0x%04x\n",
		(int)filehdr.rsrcCRC, (int)crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on resource fork");
#endif /* SCAN */
	    exit(1);
	}
	end_file();
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
}

static void zma_wrfile(ibytes, obytes, type)
unsigned long ibytes, obytes;
char type;
{
    if(ibytes == 0) {
	if(verbose) {
	    (void)fprintf(stderr, "empty");
	}
	return;
    }
    switch(type) {
    case z_plain:		/* no compression */
	if(verbose) {
	    (void)fprintf(stderr, "No compression");
	}
	if(write_it) {
	    zma_nocomp(ibytes);
	}
	break;
    case z_file:		/* lzh compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "LZH compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    zma_lzh(ibytes);
	}
	break;
    default:
	(void)fprintf(stderr, "Unknown compression method %2x\n", type);
#ifdef SCAN
	do_idf("", UNKNOWN);
#endif /* SCAN */
	exit(1);
    }
}

/*---------------------------------------------------------------------------*/
/*	No compression							     */
/*---------------------------------------------------------------------------*/
static void zma_nocomp(ibytes)
unsigned long ibytes;
{
    int n = ibytes;
    char *ptr = out_buffer;

    while(n-- > 0) {
	*ptr++ = *zma_filestart++;
    }
}

/*---------------------------------------------------------------------------*/
/*	LZ compression plus Huffman encoding				     */
/*---------------------------------------------------------------------------*/
static void zma_lzh(ibytes)
unsigned long ibytes;
{
    /* Controlled by ibutes only */
    de_lzh((long)ibytes, (long)(-1), &zma_filestart, 13);
}
#else /* ZMA */
int zma; /* keep lint and some compilers happy */
#endif /* ZMA */

