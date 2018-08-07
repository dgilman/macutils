#include "macunpack.h"
#ifdef DD
#ifndef CPT
#define CPT
#endif /* CPT */
#endif /* DD */
#ifdef CPT
#include "globals.h"
#include "cpt.h"
#include "crc.h"
#include "../util/util.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "../util/masks.h"
#include "huffman.h"

#define	ESC1		0x81
#define	ESC2		0x82
#define NONESEEN	0
#define ESC1SEEN	1
#define ESC2SEEN	2

extern char *malloc();
extern char *realloc();
extern int free();

static void cpt_uncompact();
static unsigned char *cpt_data;
static unsigned long cpt_datamax;
static unsigned long cpt_datasize;
static unsigned char cpt_LZbuff[CIRCSIZE];
static unsigned int cpt_LZptr;
static unsigned char *cpt_char;
static unsigned long cpt_crc;
static unsigned long cpt_inlength;
static unsigned long cpt_outlength;
static int cpt_outstat;
static unsigned char cpt_savechar;
static unsigned long cpt_newbits;
static int cpt_bitsavail;
static int cpt_blocksize;
/* Lengths is twice the max number of entries, and include slack. */
#define SLACK	6
static node cpt_Hufftree[512 + SLACK], cpt_LZlength[128 + SLACK],
	    cpt_LZoffs[256 + SLACK];

static int readcpthdr();
static int cpt_filehdr();
static void cpt_folder();
static void cpt_uncompact();
static void cpt_wrfile();
void cpt_wrfile1();
static void cpt_outch();
static void cpt_rle();
static void cpt_rle_lzh();
static void cpt_readHuff();
static int cpt_get6bits();
static int cpt_getbit();

void cpt()
{
    struct cptHdr cpthdr;
    struct fileHdr filehdr;
    char *cptindex;
    int cptindsize;
    char *cptptr;
    int i;

    updcrc = zip_updcrc;
    crcinit = zip_crcinit;
    cpt_crc = INIT_CRC;
    if(readcpthdr(&cpthdr) == 0) {
	(void)fprintf(stderr, "Can't read archive header\n");
#ifdef SCAN
	do_error("macunpack: Can't read archive header");
#endif /* SCAN */
	exit(1);
    }

    cptindsize = cpthdr.entries * FILEHDRSIZE;
    if(cpthdr.commentsize > cptindsize) {
	cptindsize = cpthdr.commentsize;
    }
    cptindex = malloc((unsigned)cptindsize);
    if(cptindex == NULL) {
	(void)fprintf(stderr, "Insufficient memory, aborting\n");
	exit(1);
    }
    cptptr = cptindex;
    if(fread(cptptr, 1, (int)cpthdr.commentsize, infp) != cpthdr.commentsize) {
	(void)fprintf(stderr, "Can't read comment.\n");
#ifdef SCAN
	do_error("macunpack: Can't read comment");
#endif /* SCAN */
	exit(1);
    }
    cpt_crc = (*updcrc)(cpt_crc, cptptr, cpthdr.commentsize);

    for(i = 0; i < cpthdr.entries; i++) {
	*cptptr = getc(infp);
	cpt_crc = (*updcrc)(cpt_crc, cptptr, 1);
	if(*cptptr & 0x80) {
	    cptptr[F_FOLDER] = 1;
	    *cptptr &= 0x3f;
	} else {
	    cptptr[F_FOLDER] = 0;
	}
	if(fread(cptptr + 1, 1, *cptptr, infp) != *cptptr) {
	    (void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
	    do_error("macunpack: Can't read file header");
#endif /* SCAN */
	    exit(1);
	}
	cpt_crc = (*updcrc)(cpt_crc, cptptr + 1, *cptptr);
	if(cptptr[F_FOLDER]) {
	    if(fread(cptptr + F_FOLDERSIZE, 1, 2, infp) != 2) {
		(void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
		do_error("macunpack: Can't read file header");
#endif /* SCAN */
		exit(1);
	    }
	    cpt_crc = (*updcrc)(cpt_crc, cptptr + F_FOLDERSIZE, 2);
	} else {
	    if(fread(cptptr + F_VOLUME, 1, FILEHDRSIZE - F_VOLUME, infp) !=
		FILEHDRSIZE - F_VOLUME) {
		(void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
		do_error("macunpack: Can't read file header");
#endif /* SCAN */
		exit(1);
	    }
	    cpt_crc = (*updcrc)(cpt_crc, cptptr + F_VOLUME,
				FILEHDRSIZE - F_VOLUME);
	}
	cptptr += FILEHDRSIZE;
    }
    if(cpt_crc != cpthdr.hdrcrc) {
	(void)fprintf(stderr, "Header CRC mismatch: got 0x%08x, need 0x%08x\n",
		(int)cpthdr.hdrcrc, (int)cpt_crc);
#ifdef SCAN
	do_error("macunpack: Header CRC mismatch");
#endif /* SCAN */
	exit(1);
    }

    cptptr = cptindex;
    for(i = 0; i < cpthdr.entries; i++) {
	if(cpt_filehdr(&filehdr, cptptr) == -1) {
	    (void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
	    do_error("macunpack: Can't read file header");
#endif /* SCAN */
	    exit(1);
	}
	if(filehdr.folder) {
	    cpt_folder(text, filehdr, cptptr);
	    i += filehdr.foldersize;
	    cptptr += filehdr.foldersize * FILEHDRSIZE;
	} else {
	    cpt_uncompact(filehdr);
	}
	cptptr += FILEHDRSIZE;
    }
    (void)free(cptindex);
}

static int readcpthdr(s)
struct cptHdr *s;
{
    char temp[CHDRSIZE];

    if(fread(temp, 1, CPTHDRSIZE, infp) != CPTHDRSIZE) {
	return 0;
    }

    if(temp[C_SIGNATURE] != 1) {
	(void)fprintf(stderr, "Not a Compactor file\n");
	return 0;
    }

    cpt_datasize = get4(temp + C_IOFFSET);
    s->offset = cpt_datasize;
    if(cpt_datasize > cpt_datamax) {
	if(cpt_datamax == 0) {
	    cpt_data = (unsigned char *)malloc((unsigned)cpt_datasize);
	} else {
	    cpt_data = (unsigned char *)realloc((char *)cpt_data,
						(unsigned)cpt_datasize);
	}
	cpt_datamax = cpt_datasize;
    }
    if(cpt_data == NULL) {
	(void)fprintf(stderr, "Insufficient memory, aborting\n");
	exit(1);
    }

    if(fread((char *)(cpt_data + CPTHDRSIZE), 1,
	(int)s->offset - CPTHDRSIZE, infp) != s->offset - CPTHDRSIZE) {
	return 0;
    }

    if(fread(temp + CPTHDRSIZE, 1, CPTHDR2SIZE, infp) != CPTHDR2SIZE) {
	return 0;
    }

    cpt_crc = (*updcrc)(cpt_crc, temp + CPTHDRSIZE + C_ENTRIES, 3);
    s->hdrcrc = get4(temp + CPTHDRSIZE + C_HDRCRC);
    s->entries = get2(temp + CPTHDRSIZE + C_ENTRIES);
    s->commentsize = temp[CPTHDRSIZE + C_COMMENT];

    return 1;
}

static int cpt_filehdr(f, hdr)
struct fileHdr *f;
char *hdr;
{
    register int i;
    int n;
    char ftype[5], fauth[5];

    for(i = 0; i < INFOBYTES; i++) {
	info[i] = '\0';
    }

    n = hdr[F_FNAME] & BYTEMASK;
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    info[I_NAMEOFF] = n;
    copy(info + I_NAMEOFF + 1, hdr + F_FNAME + 1, n);
    transname(hdr + F_FNAME + 1, text, n);

    f->folder = hdr[F_FOLDER];
    if(f->folder) {
	f->foldersize = get2(hdr + F_FOLDERSIZE);
    } else {
	f->cptFlag = get2(hdr + F_CPTFLAG);
	f->rsrcLength = get4(hdr + F_RSRCLENGTH);
	f->dataLength = get4(hdr + F_DATALENGTH);
	f->compRLength = get4(hdr + F_COMPRLENGTH);
	f->compDLength = get4(hdr + F_COMPDLENGTH);
	f->fileCRC = get4(hdr + F_FILECRC);
	f->FndrFlags = get2(hdr + F_FNDRFLAGS);
	f->filepos = get4(hdr + F_FILEPOS);
	f->volume = hdr[F_VOLUME];
    }

    write_it = 1;
    if(list) {
	do_indent(indent);
	if(f->folder) {
	    (void)fprintf(stderr, "folder=\"%s\"", text);
	} else {
	    transname(hdr + F_FTYPE, ftype, 4);
	    transname(hdr + F_CREATOR, fauth, 4);
	    (void)fprintf(stderr,
		    "name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		    text, ftype, fauth,
		    (long)f->dataLength, (long)f->rsrcLength);
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

	if(!f->folder) {
	    copy(info + I_TYPEOFF, hdr + F_FTYPE, 4);
	    copy(info + I_AUTHOFF, hdr + F_CREATOR, 4);
	    copy(info + I_FLAGOFF, hdr + F_FNDRFLAGS, 2);
	    copy(info + I_DLENOFF, hdr + F_DATALENGTH, 4);
	    copy(info + I_RLENOFF, hdr + F_RSRCLENGTH, 4);
	    copy(info + I_CTIMOFF, hdr + F_CREATIONDATE, 4);
	    copy(info + I_MTIMOFF, hdr + F_MODDATE, 4);
	}
    }
    return 1;
}

static void cpt_folder(name, fileh, cptptr)
char *name;
struct fileHdr fileh;
char *cptptr;
{
    int i, nfiles;
    char loc_name[64];
    struct fileHdr filehdr;

    for(i = 0; i < 64; i++) {
	loc_name[i] = name[i];
    }
    if(write_it || info_only) {
	cptptr += FILEHDRSIZE;
	nfiles = fileh.foldersize;
	if(write_it) {
	    do_mkdir(text, info);
	}
	indent++;
	for(i = 0; i < nfiles; i++) {
	    if(cpt_filehdr(&filehdr, cptptr) == -1) {
		(void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
		do_error("macunpack: Can't read file header");
#endif /* SCAN */
		exit(1);
	    }
	    if(filehdr.folder) {
		cpt_folder(text, filehdr, cptptr);
		i += filehdr.foldersize;
		cptptr += filehdr.foldersize * FILEHDRSIZE;
	    } else {
		cpt_uncompact(filehdr);
	    }
	    cptptr += FILEHDRSIZE;
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

static void cpt_uncompact(filehdr)
struct fileHdr filehdr;
{
    if(filehdr.cptFlag & 1) {
	(void)fprintf(stderr, "\tFile is password protected, skipping file\n");
#ifdef SCAN
	do_idf("", PROTECTED);
#endif /* SCAN */
	return;
    }
    if(write_it) {
	start_info(info, filehdr.rsrcLength, filehdr.dataLength);
	cpt_crc = INIT_CRC;
	cpt_char = cpt_data + filehdr.filepos;
    }
    if(verbose) {
	(void)fprintf(stderr, "\tRsrc: ");
	if(filehdr.compRLength == 0) {
	    (void)fprintf(stderr, "empty");
	} else if(filehdr.cptFlag & 2) {
	    (void)fprintf(stderr, "RLE/LZH compressed (%4.1f%%)",
		    100.0 * filehdr.compRLength / filehdr.rsrcLength);
	} else {
	    (void)fprintf(stderr, "RLE compressed (%4.1f%%)",
		    100.0 * filehdr.compRLength / filehdr.rsrcLength);
	}
    }
    if(write_it) {
	start_rsrc();
	cpt_wrfile(filehdr.compRLength, filehdr.rsrcLength,
		   filehdr.cptFlag & 2);
	cpt_char = cpt_data + filehdr.filepos + filehdr.compRLength;
    }
    if(verbose) {
	(void)fprintf(stderr, ", Data: ");
	if(filehdr.compDLength == 0) {
	    (void)fprintf(stderr, "empty");
	} else if(filehdr.cptFlag & 4) {
	    (void)fprintf(stderr, "RLE/LZH compressed (%4.1f%%)",
		    100.0 * filehdr.compDLength / filehdr.dataLength);
	} else {
	    (void)fprintf(stderr, "RLE compressed (%4.1f%%)",
		    100.0 * filehdr.compDLength / filehdr.dataLength);
	}
    }
    if(write_it) {
	start_data();
	cpt_wrfile(filehdr.compDLength, filehdr.dataLength,
		   filehdr.cptFlag & 4);
	if(filehdr.fileCRC != cpt_crc) {
	    (void)fprintf(stderr,
		"CRC error on file: need 0x%08lx, got 0x%08lx\n",
		(long)filehdr.fileCRC, (long)cpt_crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on file");
#endif /* SCAN */
	    exit(1);
	}
	end_file();
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
}

static void cpt_wrfile(ibytes, obytes, type)
unsigned long ibytes, obytes;
unsigned short type;
{
    if(ibytes == 0) {
	return;
    }
    cpt_outstat = NONESEEN;
    cpt_inlength = ibytes;
    cpt_outlength = obytes;
    cpt_LZptr = 0;
    cpt_blocksize = 0x1fff0;
    if(type == 0) {
	cpt_rle();
    } else {
	cpt_rle_lzh();
    }
    cpt_crc = (*updcrc)(cpt_crc, out_buffer, obytes);
}

void cpt_wrfile1(in_char, ibytes, obytes, type, blocksize)
unsigned char *in_char;
unsigned long ibytes, obytes, blocksize;
int type;
{
    cpt_char = in_char;
    if(ibytes == 0) {
	return;
    }
    cpt_outstat = NONESEEN;
    cpt_inlength = ibytes;
    cpt_outlength = obytes;
    cpt_LZptr = 0;
    cpt_blocksize = blocksize;
    if(type == 0) {
	cpt_rle();
    } else {
	cpt_rle_lzh();
    }
}

static void cpt_outch(ch)
unsigned char ch;
{
    cpt_LZbuff[cpt_LZptr++ & (CIRCSIZE - 1)] = ch;
    switch(cpt_outstat) {
    case NONESEEN:
	if(ch == ESC1 && cpt_outlength != 1) {
	    cpt_outstat = ESC1SEEN;
	} else {
	    cpt_savechar = ch;
	    *out_ptr++ = ch;
	    cpt_outlength--;
	}
	break;
    case ESC1SEEN:
	if(ch == ESC2) {
	    cpt_outstat = ESC2SEEN;
	} else {
	    cpt_savechar = ESC1;
	    *out_ptr++ = ESC1;
	    cpt_outlength--;
	    if(cpt_outlength == 0) {
		return;
	    }
	    if(ch == ESC1 && cpt_outlength != 1) {
		return;
	    }
	    cpt_outstat = NONESEEN;
	    cpt_savechar = ch;
	    *out_ptr++ = ch;
	    cpt_outlength--;
	}
	break;
    case ESC2SEEN:
	cpt_outstat = NONESEEN;
	if(ch != 0) {
	    while(--ch != 0) {
		*out_ptr++ = cpt_savechar;
		cpt_outlength--;
		if(cpt_outlength == 0) {
		    return;
		}
	    }
	} else {
	    *out_ptr++ = ESC1;
	    cpt_outlength--;
	    if(cpt_outlength == 0) {
		return;
	    }
	    cpt_savechar = ESC2;
	    *out_ptr++ = cpt_savechar;
	    cpt_outlength--;
	}
    }
}

/*---------------------------------------------------------------------------*/
/*	Run length encoding						     */
/*---------------------------------------------------------------------------*/
static void cpt_rle()
{
    while(cpt_inlength-- > 0) {
	cpt_outch(*cpt_char++);
    }
}

/*---------------------------------------------------------------------------*/
/*	Run length encoding plus LZ compression plus Huffman encoding	     */
/*---------------------------------------------------------------------------*/
static void cpt_rle_lzh()
{
    int block_count;
    unsigned int bptr;
    int Huffchar, LZlength, LZoffs;

    get_bit = cpt_getbit;
    cpt_LZbuff[CIRCSIZE - 3] = 0;
    cpt_LZbuff[CIRCSIZE - 2] = 0;
    cpt_LZbuff[CIRCSIZE - 1] = 0;
    cpt_LZptr = 0;
    while(cpt_outlength != 0) {
	cpt_readHuff(256, cpt_Hufftree);
	cpt_readHuff(64, cpt_LZlength);
	cpt_readHuff(128, cpt_LZoffs);
	block_count = 0;
	cpt_newbits = (*cpt_char++ << 8);
	cpt_newbits = cpt_newbits | *cpt_char++;
	cpt_newbits = cpt_newbits << 16;
	cpt_bitsavail = 16;
	while(block_count < cpt_blocksize && cpt_outlength != 0) {
	    if(cpt_getbit()) {
		Huffchar = gethuffbyte(cpt_Hufftree);
		cpt_outch((unsigned char)Huffchar);
		block_count += 2;
	    } else {
		LZlength = gethuffbyte(cpt_LZlength);
		LZoffs = gethuffbyte(cpt_LZoffs);
		LZoffs = (LZoffs << 6) | cpt_get6bits();
		bptr = cpt_LZptr - LZoffs;
		while(LZlength-- > 0) {
		    cpt_outch(cpt_LZbuff[bptr++ & (CIRCSIZE - 1)]);
		}
		block_count += 3;
	    }
	}
    }
}

/* Based on unimplod from unzip; difference are noted below. */
typedef struct sf_entry {
    int Value;
    int BitLength;
} sf_entry;

/* See routine LoadTree.  The parameter tree (actually an array and
   two integers) are only used locally in this version and hence locally
   declared.  The parameter nodes has been renamed Hufftree.... */
static void cpt_readHuff(size, Hufftree)
int size;
struct node *Hufftree;
{
    sf_entry tree_entry[256 + SLACK]; /* maximal number of elements */
    int tree_entries;
    int tree_MaxLength; /* finishes local declaration of tree */

    int treeBytes, i, len;  /* declarations from ReadLengths */

    /* declarations from SortLengths */
    sf_entry *ejm1;
    int j;
    sf_entry *entry;
/*  int i already above */
    sf_entry tmp;
    int entries;
    unsigned a, b;

    /* declarations from GenerateTrees */
    int codelen, lvlstart, next, parents;
/*  int i, j already above */

    /* for Compactor */
    int tree_count[32];
    /* end declarations */

    /* next paraphrased from ReadLengths with adaption for Compactor. */
    treeBytes = *cpt_char++;
    if(size < treeBytes * 2) { /* too many entries, something is wrong! */
	(void)fprintf(stderr, "Bytes is: %d, expected: %d\n", treeBytes,
		size / 2);
#ifdef SCAN
	do_error("macunpack: error in coding tree");
#endif /* SCAN */
	exit(1);
    }
    for(i = 0; i < 32; i++) {
	tree_count[i] = 0;
    }
    i = 0;
    tree_MaxLength = 0;
    tree_entries = 0;
    while(treeBytes-- > 0) { /* adaption for Compactor */
	len = (*cpt_char) >> 4;
	if(len != 0) { /* only if length unequal zero */
	    if(len > tree_MaxLength) {
		tree_MaxLength = len;
	    }
	    tree_count[len]++;
	    tree_entry[tree_entries].Value = i;
	    tree_entry[tree_entries++].BitLength = len;
	}
	i++;
	len = *cpt_char++ & NIBBLEMASK;
	if(len != 0) { /* only if length unequal zero */
	    if(len > tree_MaxLength) {
		tree_MaxLength = len;
	    }
	    tree_count[len]++;
	    tree_entry[tree_entries].Value = i;
	    tree_entry[tree_entries++].BitLength = len;
	}
	i++;
    }

    /* Compactor allows unused trailing codes in its Huffman tree! */
    j = 0;
    for(i = 0; i <= tree_MaxLength; i++) {
	j = (j << 1) + tree_count[i];
    }
    j = (1 <<tree_MaxLength) - j;
    /* Insert the unused entries for sorting purposes. */
    for(i = 0; i < j; i++) {
	tree_entry[tree_entries].Value = size;
	tree_entry[tree_entries++].BitLength = tree_MaxLength;
    }

    /* adaption from SortLengths */
    entry = &(tree_entry[0]);
    entries = tree_entries;
    for(i = 0; ++i < entries;) {
	tmp = entry[i];
	b = tmp.BitLength;
	j = i;
	while((j > 0) && ((a = (ejm1 = &(entry[j - 1]))->BitLength) >= b)) {
	    if((a == b) && (ejm1->Value <= tmp.Value)) {
		break;
	    }
	    *(ejm1 + 1) = *ejm1;
	    --j;
	}
	entry[j] = tmp;
    }

    /* Adapted from GenerateTrees */
    i = tree_entries - 1;
    /* starting at the upper end (and reversing loop) because of Compactor */
    lvlstart = next = size * 2 + SLACK - 1;
    /* slight adaption because of different node format used */
    for(codelen = tree_MaxLength; codelen >= 1; --codelen) {
	while((i >= 0) && (tree_entry[i].BitLength == codelen)) {
	    Hufftree[next].byte = tree_entry[i].Value;
	    Hufftree[next].flag = 1;
	    next--;
	    i--;
	}
	parents = next;
	if(codelen > 1) {
	    /* reversed loop */
	    for(j = lvlstart; j > parents + 1; j-= 2) {
		Hufftree[next].one = &(Hufftree[j]);
		Hufftree[next].zero = &(Hufftree[j - 1]);
		Hufftree[next].flag = 0;
		next--;
	    }
	}
	lvlstart = parents;
    }
    Hufftree[0].one = &(Hufftree[next + 2]);
    Hufftree[0].zero = &(Hufftree[next + 1]);
    Hufftree[0].flag = 0;
}

static int cpt_get6bits()
{
int b = 0, cn;

    b = (cpt_newbits >> 26) & 0x3f;
    cpt_bitsavail -= 6;
    cpt_newbits <<= 6;
    if(cpt_bitsavail < 16) {
	cn = (*cpt_char++ << 8);
	cn |= *cpt_char++;
	cpt_newbits |= (cn << (16 - cpt_bitsavail));
	cpt_bitsavail += 16;
    }
    return b;
}

static int cpt_getbit()
{
int b;

    b = (cpt_newbits >> 31) & 1;
    cpt_bitsavail--;
    if(cpt_bitsavail < 16) {
	cpt_newbits |= (*cpt_char++ << 8);
	cpt_newbits |= *cpt_char++;
	cpt_bitsavail += 16;
    }
    cpt_newbits <<= 1;
    return b;
}
#else /* CPT */
int cpt; /* keep lint and some compilers happy */
#endif /* CPT */

