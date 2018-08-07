#include "macunpack.h"
#ifdef LZH
#include "globals.h"
#include "lzh.h"
#include "crc.h"
#include "../fileio/wrfile.h"
#include "../fileio/machdr.h"
#include "../util/masks.h"
#include "../util/util.h"
#include "bits_be.h"

#define LZ5LOOKAHEAD    18    /* look ahead buffer size for LArc */
#define LZ5BUFFSIZE	8192
#define LZ5MASK		8191
#define LZSLOOKAHEAD	17
#define LZSBUFFSIZE	4096
#define LZSMASK		4095
#define LZBUFFSIZE	8192	/* Max of above buffsizes */

extern char *malloc();
extern char *realloc();
extern void de_lzah();
extern unsigned char (*lzah_getbyte)();
extern void de_lzh();

typedef struct methodinfo {
	char *name;
	int number;
};

static struct methodinfo methods[] = {
    {"-lh0-", lh0},
    {"-lh1-", lh1},
    {"-lh2-", lh2},
    {"-lh3-", lh3},
    {"-lh4-", lh4},
    {"-lh5-", lh5},
    {"-lz4-", lz4},
    {"-lz5-", lz5},
    {"-lzs-", lzs}
};
static char *lzh_archive;
static char *lzh_pointer;
static char *lzh_data;
static char *lzh_finfo;
static int lzh_fsize;
static int lzh_kind;
static int oldsize;
static char *lzh_file;
static int lzh_filesize;
static char *lzh_current;
static char *tmp_out_ptr;
static char lzh_lzbuf[LZBUFFSIZE];

static int lzh_filehdr();
static int lzh_checkm();
static char *lzh_methname();
static void lzh_wrfile();
static void lzh_skip();
static void lzh_nocomp();
#ifdef UNTESTED
static void lzh_lzss1();
static void lzh_lzss2();
#endif /* UNTESTED */
static void lzh_lzah();
static unsigned char lzh_getbyte();
#ifdef UNDEF
static void lzh_lh2();
static void lzh_lh3();
#endif /* UNDEF */
#ifdef UNTESTED
static void lzh_lzh12();
#endif /* UNTESTED */
static void lzh_lzh13();

void lzh(kind)
int kind;
{
    struct fileHdr filehdr;
    int m, i, j;
    char loc_name[64];
    char dirinfo[INFOBYTES];

    updcrc = arc_updcrc;
    crcinit = arc_crcinit;
    write_it = 1;
    lzh_fsize = 0;
    lzh_kind = kind;
    if(lzh_archive == NULL) {
	lzh_archive = malloc((unsigned)in_data_size);
	oldsize = in_data_size;
    } else if(in_data_size > oldsize) {
	lzh_archive = realloc(lzh_archive, (unsigned)in_data_size);
	oldsize = in_data_size;
    }
    if(lzh_archive == NULL) {
	(void)fprintf(stderr, "Insufficient memory for archive.\n");
	exit(1);
    }
    if(fread(lzh_archive, 1, in_data_size, infp) != in_data_size) {
	(void)fprintf(stderr, "Can't read archive.\n");
#ifdef SCAN
	do_error("macunpack: Can't read archive");
#endif /* SCAN */
	exit(1);
    }
    lzh_pointer = lzh_archive;
    while(1) {
	if(in_data_size == 0) {
	    break;
	}
	if(lzh_filehdr(&filehdr) == 0) {
	    break;
	}
	m = lzh_checkm(&filehdr);
	if(m < 0) {
	    (void)fprintf(stderr,
		    "Skipping file: \"%s\"; unknown method: %.5s.\n",
		    text, filehdr.method);
	    lzh_skip(&filehdr);
	    continue;
	}
	if(!write_it) {
	    /*  We are skipping a folder.  Skip the file if lzh_finfo is a
		prefix of or identical to the folder info in the file. */
	    if(lzh_fsize <= filehdr.extendsize &&
	       !strncmp(lzh_finfo, filehdr.extend, lzh_fsize)) {
		/* It was true, so we skip. */
		lzh_skip(&filehdr);
		continue;
	    }
	    /*  We have left the folder we were skipping. */
	}
	/*  Now we must leave folders until lzh_finfo is a proper prefix or
	    identical to the folder info in the file. */
	while(lzh_fsize > filehdr.extendsize ||
	      strncmp(lzh_finfo, filehdr.extend, lzh_fsize)) {
	    /*  Not a proper prefix, leave folder.  First determine which! */
	    i = lzh_fsize - 1;
	    while(--i >= 0 && lzh_finfo[i] != ':');
	    i = i + 1;
	    transname(lzh_finfo + i, loc_name, lzh_fsize - i - 1);
	    lzh_fsize = i;
	    if(write_it) {
		indent--;
		if(!info_only) {
		    enddir();
		}
		if(list) {
		    do_indent(indent);
		    (void)fprintf(stderr, "leaving folder \"%s\"\n", loc_name);
		}
	    }
	    write_it = 1;
	}
	write_it = 1;
	/*  lzh_finfo is a proper prefix or identical, just show so. */
	lzh_finfo = filehdr.extend;
	/*  Now enter directories while lzh_finfo is smaller than extend. */
	while(lzh_fsize < filehdr.extendsize) {
	    i = lzh_fsize;
	    while(lzh_finfo[++i] != ':');
	    transname(lzh_finfo + lzh_fsize, loc_name, i - lzh_fsize);
	    for(j = 0; j < INFOBYTES; j++) {
		dirinfo[j] = 0;
	    }
	    dirinfo[I_NAMEOFF] = i - lzh_fsize;
	    copy(dirinfo + I_NAMEOFF + 1, lzh_finfo + lzh_fsize, i - lzh_fsize);
	    lzh_fsize = i + 1;
	    if(list) {
		do_indent(indent);
		(void)fprintf(stderr, "folder=\"%s\"", loc_name);
		if(query) {
		    write_it = do_query();
		} else {
		    (void)fputc('\n', stderr);
		}
		if(write_it) {
		    indent++;
		}
	    }
	    if(write_it && !info_only) {
		do_mkdir(loc_name, dirinfo);
	    }
	    if(!write_it) {
		break;
	    }
	}
	if(!write_it) {
	    lzh_skip(&filehdr);
	} else {
	    lzh_wrfile(&filehdr, m);
	}
    }
    /*  Leaving some more directories! */
    while(lzh_fsize != 0) {
	i = lzh_fsize - 1;
	while(--i >= 0 && lzh_finfo[i] != ':');
	i = i + 1;
	transname(lzh_finfo + i, loc_name, lzh_fsize - i - 1);
	lzh_fsize = i;
	if(write_it) {
	}
	if(write_it) {
	    indent--;
	    if(!info_only) {
		enddir();
	    }
	    if(list) {
		do_indent(indent);
		(void)fprintf(stderr, "leaving folder \"%s\"\n", loc_name);
	    }
	}
    }
}

static int lzh_filehdr(f)
struct fileHdr *f;
{
    register int i;
    char *hdr;
    int c;
    int ext_ptr;
    int chk_sum = 0;
    char *ptr;

    if(in_data_size <= 0) {
	return 0;
    }
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = '\0';
    }
    hdr = lzh_pointer;
    in_data_size -= 2;
    lzh_pointer += 2;
    if(in_data_size < 0) {
	in_data_size++;
    }
    f->hsize = (unsigned char)hdr[L_HSIZE];
    if(f->hsize == 0) {
	return 0;
    }
    f->hcrc = (unsigned char)hdr[L_HCRC];
    ptr = hdr + L_METHOD;
    in_data_size -= f->hsize;
    lzh_pointer += f->hsize;
    copy(&(f->method[0]), hdr + L_METHOD, 5);
    f->psize = get4i(hdr + L_PSIZE);
    f->upsize = get4i(hdr + L_UPSIZE);
    f->lastmod = get4i(hdr + L_LASTMOD);
    f->attribute = hdr[L_ATTRIBUTE + 1];
    if(f->attribute < 2) {
	for(i = 0; i < f->hsize; i++) {
	    chk_sum += *ptr++;
	}
	chk_sum &= BYTEMASK;
	if(chk_sum != f->hcrc) {
	    (void)fprintf(stderr,
		    "Header checksum error; got %.2x, must be %.2x.\n",
		    chk_sum, f->hcrc);
#ifdef SCAN
	    do_error("macunpack: Header checksum error");
#endif /* SCAN */
	    exit(1);
	}
	f->nlength = (unsigned char)hdr[L_NLENGTH];
	info[I_NAMEOFF] = f->nlength;
	copy(info + I_NAMEOFF + 1, hdr + L_NAME, (int)f->nlength);
	transname(hdr + L_NAME, text, (int)f->nlength);
	ext_ptr = L_NLENGTH + f->nlength + 1;
	f->crc = get2i(hdr + ext_ptr + L_CRC);
	if(f->attribute == 1) {
	    f->etype = hdr[ext_ptr + L_ETYPE];
	    f->extendsize = hdr[ext_ptr + L_EXTENDSZ];
	    f->extend = hdr + ext_ptr + L_EXTEND;
	} else {
	    f->extend = NULL;
	    f->extendsize = 0;
	}
    } else if(f->attribute == 2) {
	in_data_size += 2;
	lzh_pointer -= 2;
	f->nlength = hdr[L_2EXTENDSZ] - 3;
	info[I_NAMEOFF] = f->nlength;
	copy(info + I_NAMEOFF + 1, hdr + L_2EXTEND + 2, (int)f->nlength);
	transname(hdr + L_2EXTEND + 2, text, (int)f->nlength);
	ext_ptr = 
	f->crc = get2i(hdr + L_2CRC);
	f->etype = hdr[L_2ETYPE];
	ext_ptr = L_2EXTEND + 2 + f->nlength;
	f->extendsize = hdr[ext_ptr + L_EEXTENDSZ];
	f->extend = hdr + ext_ptr + L_EEXTEND;
    } else {
	(void)fprintf(stderr, "Unknown file header format (%d).\n",
		(int)f->attribute);
#ifdef SCAN
	do_error("macunpack: Unknown file header format");
#endif /* SCAN */
	exit(1);
    }
    if(f->extend != NULL) {
	if(f->extendsize > 5) {
	    f->extend += 2;
	    hdr = f->extend;
	    f->extendsize -= 3;
	    for(i = 0; i < f->extendsize; i++) {
		c = *hdr++;
		if((c & BYTEMASK) == BYTEMASK) {
		    hdr[-1] = ':';
		    c = ':';
		}
	    }
	    c = *hdr++;
	    if(c == 5) {
		  hdr += 5;
	    }
	} else {
	     if(f->extendsize == 5) {
		hdr = f->extend;
		f->extend = NULL;
		f->extendsize = 0;
		hdr += 5;
	    } else {
		hdr = f->extend;
		f->extend = NULL;
		f->extendsize = 0;
	    }
	}
    } else {
	hdr = hdr + ext_ptr;
    }
    lzh_data = hdr;
    if(f->attribute != 0) {
	lzh_data++;
    }
    return 1;
}

static int lzh_checkm(f)
struct fileHdr *f;
{
    int i, nummeth;
    char *meth;

    meth = f->method;
    nummeth = sizeof(methods) / sizeof(struct methodinfo);
    for(i = 0; i < nummeth; i++) {
	if(!strncmp(methods[i].name, meth, 5)) {
	    return methods[i].number;
	}
    }
    return -1;
}

static char *lzh_methname(n)
int n;
{
    if(n > sizeof(methods) / sizeof(struct methodinfo)) {
	return NULL;
    }
    return methods[n].name;
}

static void lzh_wrfile(filehdr, method)
struct fileHdr *filehdr;
int method;
{
    char ftype[5], fauth[5];
    int rsrcLength, dataLength;
    int doit;
    char *mname;
    unsigned long crc;

    if(filehdr->upsize > lzh_filesize) {
	if(lzh_filesize == 0) {
	    lzh_file = malloc((unsigned)filehdr->upsize);
	} else {
	    lzh_file = realloc(lzh_file, (unsigned)filehdr->upsize);
	}
	if(lzh_file == NULL) {
	    (void)fprintf(stderr, "Insufficient memory to unpack file.\n");
	    exit(1);
	}
    }
    switch(method) {
    case lz4:
	lzh_nocomp((unsigned long)128);
	break;
#ifdef UNTESTED
    case lz5:
	lzh_lzss1((unsigned long)128);
	break;
    case lzs:
	lzh_lzss2((unsigned long)128);
	break;
#endif /* UNTESTED */
    case lh0:
	lzh_nocomp((unsigned long)128);
	break;
    case lh1:
	lzh_lzah((unsigned long)128);
	break;
#ifdef UNDEF
    case lh2:
	lzh_lh2((unsigned long)128);
	break;
    case lh3:
	lzh_lh3((unsigned long)128);
	break;
#endif /* UNDEF */
#ifdef UNTESTED
    case lh4:
	lzh_lzh12((unsigned long)128);
	break;
#endif /* UNTESTED */
    case lh5:
	lzh_lzh13((unsigned long)128);
	break;
    default:
	mname = lzh_methname(method);
	if(mname != NULL) {
	    do_indent(indent);
	    (void)fprintf(stderr,
		    "\tSorry, packing method not yet implemented.\n");
	    do_indent(indent);
	    (void)fprintf(stderr, "File = \"%s\"; ", text);
	    (void)fprintf(stderr, "method = %s, skipping file.\n", mname);
	    lzh_skip(filehdr);
	    return;
	}
	(void)fprintf(stderr,
		"There is something very wrong with this program!\n");
#ifdef SCAN
	do_error("macunpack: program error");
#endif /* SCAN */
	exit(1);
    }
    /* Checks whether everything is packed as MacBinary. */
    if(*lzh_file != 0 /* More checks possible here. */) {
	do_indent(indent);
	(void)fprintf(stderr, "File = \"%s\" ", text);
	(void)fprintf(stderr, "not packed in MacBinary, skipping file.\n");
#ifdef SCAN
	do_error("macunpack: not MacBinary");
#endif /* SCAN */
	lzh_skip(filehdr);
	return;
    }
    copy(info, lzh_file, 128);
    rsrcLength = get4(info + I_RLENOFF);
    dataLength = get4(info + I_DLENOFF);
    transname(info + I_TYPEOFF, ftype, 4);
    transname(info + I_AUTHOFF, fauth, 4);
    if(list) {
	do_indent(indent);
	(void)fprintf(stderr,
		"name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		text, ftype, fauth, (long)dataLength, (long)rsrcLength);
    }
    if(info_only) {
	doit = 0;
    } else {
	doit = 1;
    }
    if(query) {
	doit = do_query();
    } else if(list) {
	(void)fputc('\n', stderr);
    }
    if(doit) {
	define_name(text);
	start_info(info, (unsigned long)rsrcLength, (unsigned long)dataLength);
    }
    switch(method) {
    case lz4:
	if(verbose) {
	    (void)fprintf(stderr, "\tNo Compression (%.5s)", filehdr->method);
	}
	if(doit) {
	    lzh_nocomp(filehdr->upsize);
	}
	break;
#ifdef UNTESTED
    case lz5:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZSS (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzss1(filehdr->upsize);
	}
	break;
    case lzs:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZSS (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzss2(filehdr->upsize);
	}
	break;
#endif /* UNTESTED */
    case lh0:
	if(verbose) {
	    (void)fprintf(stderr, "\tNo Compression (%.5s)", filehdr->method);
	}
	if(doit) {
	    lzh_nocomp(filehdr->upsize);
	}
	break;
    case lh1:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZAH (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzah(filehdr->upsize);
	}
	break;
#ifdef UNDEF
    case lh2:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZAH (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lh2(filehdr->upsize);
	}
	break;
    case lh3:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZH (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzh3(filehdr->upsize);
	}
	break;
#endif /* UNDEF */
#ifdef UNTESTED
    case lh4:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZH (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzh12(filehdr->upsize);
	}
	break;
#endif /* UNTESTED */
    case lh5:
	if(verbose) {
	    (void)fprintf(stderr, "\tLZH (%.5s) compressed (%4.1f%%)",
		    filehdr->method, 100.0 * filehdr->psize / filehdr->upsize);
	}
	if(doit) {
	    lzh_lzh13(filehdr->upsize);
	}
    }
    if(doit) {
	crc = (*updcrc)(INIT_CRC, lzh_file, filehdr->upsize);
	if(filehdr->crc != crc) {
	    (void)fprintf(stderr,
		    "CRC error on file: need 0x%04x, got 0x%04x\n",
		    filehdr->crc, (int)crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on file");
#endif /* SCAN */
	    exit(1);
	}
	start_data();
	copy(out_ptr, lzh_file + 128, (int)(filehdr->upsize - 128));
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
    if(doit) {
	end_file();
    }
    lzh_skip(filehdr);
}

static void lzh_skip(filehdr)
struct fileHdr *filehdr;
{
    lzh_pointer += filehdr->psize;
    in_data_size -= filehdr->psize;
}

/*---------------------------------------------------------------------------*/
/*	-lz4- and -lh0: No compression					     */
/*---------------------------------------------------------------------------*/
static void lzh_nocomp(obytes)
unsigned long obytes;
{
    copy(lzh_file, lzh_data, (int)obytes);
}

#ifdef UNTESTED
/*---------------------------------------------------------------------------*/
/*	-lz5-: LZSS compression, variant 1				     */
/*---------------------------------------------------------------------------*/
static void lzh_lzss1(obytes)
unsigned long obytes;
{
    int mask, ch, lzcnt, lzptr, ptr, count;
    char *p = lzh_lzbuf;
    int i, j;

    for(i = 0; i < 256; i++) {
	for(j = 0; j < 13; j++) {
	    *p++ = i;
	}
    }
    for(i = 0; i < 256; i++) {
	*p++ = i;
    }
    for(i = 0; i < 256; i++) {
	*p++ = 255 - i;
    }
    for(i = 0; i < 128; i++) {
	*p++ = 0;
    }
    for(i = 0; i < 128; i++) {
	*p++ = ' ';
    }

    tmp_out_ptr = out_ptr;
    out_ptr = lzh_file;
    ptr = LZ5BUFFSIZE - LZ5LOOKAHEAD;
    count = 0;
    lzh_current = lzh_data;
    while(obytes != 0) {
	if(count == 0) {
	    mask = *lzh_current++ & BYTEMASK;
	    count = 8;
	}
	count--;
	ch = *lzh_current++ & BYTEMASK;
	if ((mask & 1) != 0) {
	    *out_ptr++ = ch;
	    lzh_lzbuf[ptr++] = ch;
	    ptr &= LZ5MASK;
	    obytes--;
	} else {
	    lzcnt = *lzh_current++;
	    lzptr = (ch & 0x00ff) | ((lzcnt << 4) & 0x0f00);
	    lzcnt = (lzcnt & 0x000f) + 3;
	    obytes -= lzcnt;
	    do {
		ch = lzh_lzbuf[lzptr++];
		lzh_lzbuf[ptr++] = ch;
		*out_ptr++ = ch;
		lzptr &= LZ5MASK;
		ptr &= LZ5MASK;
	    } while (--lzcnt != 0) ;
	}
	mask >>= 1;
    }
    out_ptr = tmp_out_ptr;
}

/*---------------------------------------------------------------------------*/
/*	-lzs-: LZSS compression, variant 2				     */
/*---------------------------------------------------------------------------*/
static void lzh_lzss2(obytes)
unsigned long obytes;
{
    int ch, lzcnt, lzptr, ptr, i;

    tmp_out_ptr = out_ptr;
    out_ptr = lzh_file;
    ptr = LZSBUFFSIZE - LZSLOOKAHEAD;
    for(i = 0; i < ptr; i++) {
	lzh_lzbuf[i] = ' ';
    }
    for(i = ptr; i < LZSBUFFSIZE; i++) {
	lzh_lzbuf[i] = 0;
    }
    bit_be_init_getbits();
    bit_be_filestart = lzh_data;
    bit_be_inbytes = -1;
    while(obytes != 0) {
	if(bit_be_getbits(1) == 0) {
	    ch = bit_be_getbits(8);
	    *out_ptr++ = ch;
	    lzh_lzbuf[ptr++] = ch;
	    ptr &= LZSMASK;
	    obytes--;
	} else {
	    lzptr = bit_be_getbits(11);
	    lzcnt = bit_be_getbits(4) + 3;
	    obytes -= lzcnt;
	    do {
		ch = lzh_lzbuf[lzptr++];
		lzh_lzbuf[ptr++] = ch;
		*out_ptr++ = ch;
		lzptr &= LZSMASK;
		ptr &= LZSMASK;
	    } while (--lzcnt != 0) ;
	}
    }
    out_ptr = tmp_out_ptr;
}
#endif /* UNTESTED */

/*---------------------------------------------------------------------------*/
/*	-lh1-: LZ compression plus adaptive Huffman encoding		     */
/*---------------------------------------------------------------------------*/
static void lzh_lzah(obytes)
unsigned long obytes;
{
    lzh_current = lzh_data + 2; /* SKIPPING BLOCKSIZE! */
    tmp_out_ptr = out_ptr;
    out_ptr = lzh_file;
    lzah_getbyte = lzh_getbyte;
    de_lzah(obytes);
    out_ptr = tmp_out_ptr;
}

static unsigned char lzh_getbyte()
{
    return *lzh_current++;
}

#ifdef UNDEF
/*---------------------------------------------------------------------------*/
/*	-lh2-: LZ** compression						     */
/*---------------------------------------------------------------------------*/
static void lzh_lh2(obytes)
unsigned long obytes;
{
}

/*---------------------------------------------------------------------------*/
/*	-lh3-: LZ** compression						     */
/*---------------------------------------------------------------------------*/
static void lzh_lh3(obytes)
unsigned long obytes;
{
}
#endif /* UNDEF */

#ifdef UNTESTED
/*---------------------------------------------------------------------------*/
/*	-lh4-: LZ(12) compression plus Huffman encoding			     */
/*---------------------------------------------------------------------------*/
static void lzh_lzh12(obytes)
unsigned long obytes;
{
    lzh_current = lzh_data;
    tmp_out_ptr = out_ptr;
    out_ptr = lzh_file;
    /* Controlled by obytes only */
    de_lzh((long)(-1), (long)obytes, &lzh_current, 12);
    out_ptr = tmp_out_ptr;
}
#endif /* UNTESTED */

/*---------------------------------------------------------------------------*/
/*	-lh5-: LZ(13) compression plus Huffman encoding			     */
/*---------------------------------------------------------------------------*/
static void lzh_lzh13(obytes)
unsigned long obytes;
{
    lzh_current = lzh_data;
    tmp_out_ptr = out_ptr;
    out_ptr = lzh_file;
    /* Controlled by obytes only */
    de_lzh((long)(-1), (long)obytes, &lzh_current, 13);
    out_ptr = tmp_out_ptr;
}
#else /* LZH */
int lzh; /* keep lint and some compilers happy */
#endif /* LZH */

