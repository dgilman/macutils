#include "macunpack.h"
#ifdef SIT
#include "globals.h"
#include "sit.h"
#include "crc.h"
#include "../util/util.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "../util/masks.h"
#include "huffman.h"

extern void de_compress();
extern void core_compress();
extern void de_huffman();
extern void de_huffman_end();
extern void read_tree();
extern void set_huffman();
extern void de_lzah();
extern unsigned char (*lzah_getbyte)();

typedef struct methodinfo {
	char *name;
	int number;
};

static struct methodinfo methods[] = {
    {"NoComp",  nocomp},
    {"RLE",     rle},
    {"LZC",     lzc},
    {"Huffman", huffman},
    {"LZAH",    lzah},
    {"FixHuf",  fixhuf},
    {"MW",      mw},
};
static int sit_nodeptr;

static int readsithdr();
static int sit_filehdr();
static int sit_valid();
static int sit_checkm();
static char *sit_methname();
static void sit_folder();
static void sit_unstuff();
static void sit_wrfile();
static void sit_skip();
static void sit_nocomp();
static void sit_rle();
static void sit_lzc();
static void sit_huffman();
static void sit_lzah();
static unsigned char sit_getbyte();
static void sit_fixhuf();
static void sit_dosplit();
static void sit_mw();
static void sit_mw_out();
static int sit_mw_in();

static short code6[258] = {
   1024,  512,  256,  256,  256,  256,  128,  128,
    128,  128,  128,  128,  128,  128,  128,  128,
    128,  128,   64,   64,   64,   64,   64,   64,
     64,   64,   64,   64,   64,   64,   64,   64,
     64,   64,   64,   64,   64,   64,   64,   64,
     64,   64,   64,   64,   64,   64,   64,   64,
     64,   64,   32,   32,   32,   32,   32,   32,
     32,   32,   32,   32,   32,   32,   32,   32,
     32,   32,   16,   16,   16,   16,   16,   16,
     16,   16,   16,   16,   16,   16,   16,   16,
     16,   16,   16,   16,   16,   16,   16,   16,
     16,   16,   16,   16,   16,   16,   16,   16,
     16,   16,   16,   16,   16,   16,   16,   16,
     16,   16,   16,   16,   16,   16,   16,   16,
     16,   16,   16,    8,    8,   16,   16,    8,
      8,    8,    8,    8,    8,    8,    8,    8,
      8,    8,    8,    8,    8,    8,    8,    8,
      8,    8,    8,    8,    8,    8,    8,    8,
      8,    8,    8,    8,    8,    8,    8,    8,
      8,    8,    8,    8,    8,    8,    8,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    4,    4,
      4,    4,    4,    4,    4,    4,    1,    1,
      1,    1};
static char sit_buffer[32768];
static short sit_dict[16385];
static unsigned long sit_avail;
static int sit_bits_avail;

void sit()
{
    struct sitHdr sithdr;
    struct fileHdr filehdr;
    int i;

    set_huffman(HUFF_BE);
    core_compress((char *)NULL);
    updcrc = arc_updcrc;
    crcinit = arc_crcinit;
    if(readsithdr(&sithdr) == 0) {
	(void)fprintf(stderr, "Can't read file header\n");
#ifdef SCAN
	do_error("macunpack: Can't read file header");
#endif /* SCAN */
	exit(1);
    }

    for(i = 0; i < sithdr.numFiles; i++) {
	if(sit_filehdr(&filehdr, 0) == -1) {
	    (void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
	    do_error("macunpack: Can't read file header");
#endif /* SCAN */
	    exit(1);
	}
	if(!sit_valid(filehdr)) {
	    continue;
	}
	if(filehdr.compRMethod == sfolder) {
	    sit_folder(text);
	} else {
	    sit_unstuff(filehdr);
	}
    }
}

static int readsithdr(s)
struct sitHdr *s;
{
    char temp[SITHDRSIZE];

    if(fread(temp, 1, SITHDRSIZE, infp) != SITHDRSIZE) {
	return 0;
    }

    if(strncmp(temp + S_SIGNATURE,  "SIT!", 4) != 0 ||
	strncmp(temp + S_SIGNATURE2, "rLau", 4) != 0) {
	(void)fprintf(stderr, "Not a StuffIt file\n");
	return 0;
    }

    s->numFiles = get2(temp + S_NUMFILES);
    s->arcLength = get4(temp + S_ARCLENGTH);

    return 1;
}

static int sit_filehdr(f, skip)
struct fileHdr *f;
int skip;
{
    register int i;
    unsigned long crc;
    int n;
    char hdr[FILEHDRSIZE];
    char ftype[5], fauth[5];

    for(i = 0; i < INFOBYTES; i++) {
	info[i] = '\0';
    }
    if(fread(hdr, 1, FILEHDRSIZE, infp) != FILEHDRSIZE) {
	(void)fprintf(stderr, "Can't read file header\n");
	return -1;
    }
    crc = INIT_CRC;
    crc = (*updcrc)(crc, hdr, FILEHDRSIZE - 2);

    f->hdrCRC = get2(hdr + F_HDRCRC);
    if(f->hdrCRC != crc) {
	(void)fprintf(stderr, "Header CRC mismatch: got 0x%04x, need 0x%04x\n",
		f->hdrCRC & WORDMASK, (int)crc);
	return -1;
    }

    n = hdr[F_FNAME] & BYTEMASK;
    if(n > F_NAMELEN) {
	n = F_NAMELEN;
    }
    info[I_NAMEOFF] = n;
    copy(info + I_NAMEOFF + 1, hdr + F_FNAME + 1, n);
    transname(hdr + F_FNAME + 1, text, n);

    f->compRMethod = hdr[F_COMPRMETHOD];
    f->compDMethod = hdr[F_COMPDMETHOD];
    f->rsrcLength = get4(hdr + F_RSRCLENGTH);
    f->dataLength = get4(hdr + F_DATALENGTH);
    f->compRLength = get4(hdr + F_COMPRLENGTH);
    f->compDLength = get4(hdr + F_COMPDLENGTH);
    f->rsrcCRC = get2(hdr + F_RSRCCRC);
    f->dataCRC = get2(hdr + F_DATACRC);

    write_it = !skip;
    if(list && !skip) {
	if(f->compRMethod != efolder) {
	    do_indent(indent);
	}
	if(f->compRMethod == sfolder) {
	    (void)fprintf(stderr, "folder=\"%s\"", text);
	} else if(f->compRMethod != efolder) {
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
	if(f->compRMethod != efolder) {
	    if(query) {
		write_it = do_query();
	    } else {
		(void)fputc('\n', stderr);
	    }
	}
    }

    if(write_it) {
	define_name(text);

	if(f->compRMethod != sfolder) {
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

static int sit_valid(f)
struct fileHdr f;
{
    int fr = f.compRMethod, fd = f.compDMethod;

    if(fr == sfolder || fr == efolder) {
	return 1;
    }
    if((fr & prot) || (fd & prot)) {
	(void)fprintf(stderr, "\tFile is password protected");
#ifdef SCAN
	do_idf("", PROTECTED);
#endif /* SCAN */
    } else if(fr >= prot || fd >= prot) {
	(void)fprintf(stderr, "\tUnknown stuffit flags: %x %x", fr, fd);
#ifdef SCAN
	do_idf("", UNKNOWN);
#endif /* SCAN */
    } else if(((1 << fr) & sknown) && ((1 << fd) & sknown)) {
	if(sit_checkm(fr) && sit_checkm(fd)) {
	    return 1;
	}
	if(!sit_checkm(fr)) {
	    (void)fprintf(stderr, "\tMethod \"%s\" not implemented",
		    sit_methname(fr));
	} else {
	    (void)fprintf(stderr, "\tMethod \"%s\" not implemented",
		    sit_methname(fd));
	}
#ifdef SCAN
	do_idf("", UNKNOWN);
#endif /* SCAN */
    } else {
	(void)fprintf(stderr, "\tUnknown compression methods: %x %x", fr, fd);
#ifdef SCAN
	do_idf("", UNKNOWN);
#endif /* SCAN */
    }
    (void)fprintf(stderr, ", skipping file.\n");
    sit_skip(f.compRLength);
    sit_skip(f.compDLength);
    return 0;
}

static int sit_checkm(f)
int f;
{
    switch(f) {
    case nocomp:
	return 1;
    case rle:
	return 1;
    case lzc:
	return 1;
    case huffman:
	return 1;
    case lzah:
	return 1;
    case fixhuf:
	return 1;
    case mw:
	return 1;
    default:
	return 0;
    }
    /* NOTREACHED */
}

static char *sit_methname(n)
int n;
{
int i, nmeths;
    nmeths = sizeof(methods) / sizeof(struct methodinfo);
    for(i = 0; i < nmeths; i++) {
	if(methods[i].number == n) {
	    return methods[i].name;
	}
    }
    return NULL;
}

static void sit_folder(name)
char *name;
{
    int i, recurse;
    char loc_name[64];
    struct fileHdr filehdr;

    for(i = 0; i < 64; i++) {
	loc_name[i] = name[i];
    }
    if(write_it || info_only) {
	if(write_it) {
	    do_mkdir(text, info);
	}
	indent++;
	while(1) {
	    if(sit_filehdr(&filehdr, 0) == -1) {
		(void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
		do_error("macunpack: Can't read file header");
#endif /* SCAN */
		exit(1);
	    }
	    if(!sit_valid(filehdr)) {
		continue;
	    }
	    if(filehdr.compRMethod == sfolder) {
		sit_folder(text);
	    } else if(filehdr.compRMethod == efolder) {
		break;
	    } else {
		sit_unstuff(filehdr);
	    }
	}
	if(write_it) {
	    enddir();
	}
	indent--;
	if(list) {
	    do_indent(indent);
	    (void)fprintf(stderr, "leaving folder \"%s\"\n", loc_name);
	}
    } else {
	recurse = 0;
	while(1) {
	    if(sit_filehdr(&filehdr, 1) == -1) {
		(void)fprintf(stderr, "Can't read file header #%d\n", i+1);
#ifdef SCAN
		do_error("macunpack: Can't read file header");
#endif /* SCAN */
		exit(1);
	    }
	    if(filehdr.compRMethod == sfolder) {
		recurse++;
	    } else if(filehdr.compRMethod == efolder) {
		recurse--;
		if(recurse < 0) {
		    break;
		}
	    } else {
		sit_skip(filehdr.compRLength);
		sit_skip(filehdr.compDLength);
	    }
	}
    }
}

static void sit_unstuff(filehdr)
struct fileHdr filehdr;
{
    unsigned long crc;

    if(write_it) {
	start_info(info, filehdr.rsrcLength, filehdr.dataLength);
    }
    if(verbose) {
	(void)fprintf(stderr, "\tRsrc: ");
    }
    if(write_it) {
	start_rsrc();
    }
    sit_wrfile(filehdr.compRLength, filehdr.rsrcLength, filehdr.compRMethod);
    if(write_it) {
	crc = (*updcrc)(INIT_CRC, out_buffer, filehdr.rsrcLength);
	if(filehdr.rsrcCRC != crc) {
	    (void)fprintf(stderr,
		"CRC error on resource fork: need 0x%04x, got 0x%04x\n",
		filehdr.rsrcCRC, (int)crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on resource fork");
#endif /* SCAN */
	    exit(1);
	}
    }
    if(verbose) {
	(void)fprintf(stderr, ", Data: ");
    }
    if(write_it) {
	start_data();
    }
    sit_wrfile(filehdr.compDLength, filehdr.dataLength, filehdr.compDMethod);
    if(write_it) {
	crc = (*updcrc)(INIT_CRC, out_buffer, filehdr.dataLength);
	if(filehdr.dataCRC != crc) {
	    (void)fprintf(stderr,
		"CRC error on data fork: need 0x%04x, got 0x%04x\n",
		filehdr.dataCRC, (int)crc);
#ifdef SCAN
	    do_error("macunpack: CRC error on data fork");
#endif /* SCAN */
	    exit(1);
	}
	end_file();
    }
    if(verbose) {
	(void)fprintf(stderr, ".\n");
    }
}

static void sit_wrfile(ibytes, obytes, type)
unsigned long ibytes, obytes;
unsigned char type;
{
    if(ibytes == 0) {
	if(verbose) {
	    (void)fprintf(stderr, "empty");
	}
	return;
    }
    switch(type) {
    case nocomp:		/* no compression */
	if(verbose) {
	    (void)fprintf(stderr, "No compression");
	}
	if(write_it) {
	    sit_nocomp(ibytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case rle:		/* run length encoding */
	if(verbose) {
	    (void)fprintf(stderr,
		    "RLE compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_rle(ibytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case lzc:			/* LZC compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "LZC compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_lzc(ibytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case huffman:		/* Huffman compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "Huffman compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_huffman(obytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case lzah:			/* LZAH compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "LZAH compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_lzah(obytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case fixhuf:		/* FixHuf compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "FixHuf compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_fixhuf(ibytes);
	} else {
	    sit_skip(ibytes);
	}
	break;
    case mw:			/* MW compression */
	if(verbose) {
	    (void)fprintf(stderr,
		    "MW compressed (%4.1f%%)", 100.0 * ibytes / obytes);
	}
	if(write_it) {
	    sit_mw(ibytes);
	} else {
	    sit_skip(ibytes);
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

/* skip stuffit file */
static void sit_skip(ibytes)
unsigned long ibytes;
{
    while(ibytes != 0) {
	if(getc(infp) == EOF) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("macunpack: Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	ibytes--;
    }
}

/*---------------------------------------------------------------------------*/
/*	Method 0: No compression					     */
/*---------------------------------------------------------------------------*/
static void sit_nocomp(ibytes)
unsigned long ibytes;
{
    int n;

    n = fread(out_buffer, 1, (int)ibytes, infp);
    if(n != ibytes) {
	(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	do_error("macunpack: Premature EOF");
#endif /* SCAN */
	exit(1);
    }
}

/*---------------------------------------------------------------------------*/
/*	Method 1: Run length encoding					     */
/*---------------------------------------------------------------------------*/
static void sit_rle(ibytes)
unsigned long ibytes;
{
    int ch, lastch, n, i;

    while(ibytes != 0) {
	ch = getb(infp) & BYTEMASK;
	ibytes--;
	if(ch == ESC) {
	    n = (getb(infp) & BYTEMASK) - 1;
	    ibytes--;
	    if(n < 0) {
		*out_ptr++ = ESC;
		lastch = ESC;
		n = 1;
	    } else {
		for(i = 0; i < n; i++) {
		    *out_ptr++ = lastch;
		}
	    }
	} else {
	    *out_ptr++ = ch;
	    lastch = ch;
	}
    }
}

/*---------------------------------------------------------------------------*/
/*	Method 2: LZC compressed					     */
/*---------------------------------------------------------------------------*/
static void sit_lzc(ibytes)
unsigned long ibytes;
{
    de_compress(ibytes, 14);
}

/*---------------------------------------------------------------------------*/
/*	Method 3: Huffman compressed					     */
/*---------------------------------------------------------------------------*/
static void sit_huffman(obytes)
unsigned long obytes;
{
    read_tree();
    de_huffman(obytes);
}

/*---------------------------------------------------------------------------*/
/*	Method 5: LZ compression plus adaptive Huffman encoding		     */
/*---------------------------------------------------------------------------*/
static void sit_lzah(obytes)
unsigned long obytes;
{
    lzah_getbyte = sit_getbyte;
    de_lzah(obytes);
}

static unsigned char sit_getbyte()
{
    return getb(infp);
}

/*---------------------------------------------------------------------------*/
/*	Method 6: Compression with a fixed Huffman encoding		     */
/*---------------------------------------------------------------------------*/
static void sit_fixhuf(ibytes)
unsigned long ibytes;
{
    int i, sum, codes, sym, num;
    char byte_int[4], byte_short[2];
    long size;
    int sign;
    char *tmp_ptr, *ptr, *end_ptr;

    sum = 0;
    for(i = 0; i < 258; i++) {
	sum += code6[i];
	nodelist[i + 1].flag = 1;
    }
    sit_nodeptr = 258;
    sit_dosplit(0, sum, 1, 258);
    while(ibytes > 0) {
	if(fread(byte_int, 1, 4, infp) != 4) {
	    (void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
	    do_error("Premature EOF");
#endif /* SCAN */
	    exit(1);
	}
	ibytes -= 4;
	size = (long)get4(byte_int);
	sign = 0;
	if(size < 0) {
	    size = - size;
	    sign = 1;
	}
	size -= 4;
	if(sign) {
	    ibytes -= size;
	    if(fread(sit_buffer, 1, (int)size, infp) != size) {
		(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		do_error("Premature EOF");
#endif /* SCAN */
		exit(1);
	    }
	} else {
	    ibytes -= size;
	    if(fread(byte_int, 1, 4, infp) != 4) {
		(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		do_error("Premature EOF");
#endif /* SCAN */
		exit(1);
	    }
	    size -= 4;
	    if(fread(byte_short, 1, 2, infp) != 2) {
		(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		do_error("Premature EOF");
#endif /* SCAN */
		exit(1);
	    }
	    size -= 2;
	    codes = get2(byte_short);
	    for(i = 1; i <= codes; i++) {
		nodelist[i].byte = getb(infp);
	    }
	    size -= codes;
	    clrhuff();
	    nodelist[257].byte = 0x100;
	    nodelist[258].byte = 0x100;
	    tmp_ptr = out_ptr;
	    out_ptr = &(sit_buffer[0]);
	    bytesread = 0;
	    de_huffman_end(0x100);
	    while(bytesread < size) {
		(void)getb(infp);
		bytesread++;
	    }
	    size = get4(byte_int);
	    out_ptr = tmp_ptr;
	}
	ptr = sit_buffer;
	end_ptr = ptr + size;
	while(ptr < end_ptr) {
	    num = *ptr++ & BYTEMASK;
	    if(num < 0x80) {
		while(num-- >= 0) {
		    *out_ptr++ = *ptr++;
		}
	    } else if(num != 0x80) {
		sym = *ptr++;
		while(num++ <= 0x100) {
		    *out_ptr++ = sym;
		}
	    }
	}
    }
}

static void sit_dosplit(ptr, sum, low, upp)
int ptr, sum, low, upp;
{
    int i, locsum;

    sum = sum / 2;
    locsum = 0;
    i = low;
    while(locsum < sum) {
	locsum += code6[i++ - 1];
    }
    if(low == i - 1) {
	nodelist[ptr].zero = nodelist + low;
    } else {
	nodelist[ptr].zero = nodelist + ++sit_nodeptr;
	sit_dosplit(sit_nodeptr, sum, low, i - 1);
    }
    if(upp == i) {
	nodelist[ptr].one = nodelist + upp;
    } else {
	nodelist[ptr].one = nodelist + ++sit_nodeptr;
	sit_dosplit(sit_nodeptr, sum, i, upp);
    }
}

/*---------------------------------------------------------------------------*/
/*	Method 8: Compression with a MW encoding			     */
/*---------------------------------------------------------------------------*/
static void sit_mw(ibytes)
unsigned long ibytes;
{
    int ptr;
    int max, max1, bits;
    char *out_buf;

    out_buf = out_buffer;
    sit_bits_avail = 0;
    sit_avail = 0;
start_over:
    max = 256;
    max1 = max + max;
    bits = 9;
    ptr = sit_mw_in(bits, &ibytes);
    if(ptr == max) {
	goto start_over;
    }
    if(ptr > max || ptr < 0) {
	out_buffer = out_buf;
	return;
    }
    sit_dict[255] = ptr;
    sit_mw_out(ptr);
    while(1) {
	ptr = sit_mw_in(bits, &ibytes);
	if(ptr == max) {
	    goto start_over;
	}
	if(ptr > max || ptr < 0) {
	    out_buffer = out_buf;
	    return;
	}
	sit_dict[max++] = ptr;
	if(max == max1) {
	    max1 <<= 1;
	    bits++;
	}
	sit_mw_out(ptr);
    }
}

static void sit_mw_out(ptr)
int ptr;
{
    int stack_ptr;
    int stack[16384];

    stack_ptr = 1;
    stack[0] = ptr;
    while(stack_ptr) {
	ptr = stack[--stack_ptr];
	while(ptr >= 256) {
	    stack[stack_ptr++] = sit_dict[ptr];
	    ptr = sit_dict[ptr - 1];
	}
	*out_buffer++ = ptr;
    }
}

static int sit_mw_in(bits, ibytes)
int bits;
unsigned long *ibytes;
{
    int res, res1;

    while(bits > sit_bits_avail) {
	if(*ibytes == 0) {
	    return -1;
	}
	(*ibytes)--;
	sit_avail += (getb(infp) & BYTEMASK) << sit_bits_avail;
	sit_bits_avail += 8;
    }
    res1 = sit_avail >> bits;
    res = sit_avail ^ (res1 << bits);
    sit_avail = res1;
    sit_bits_avail -= bits;
    return res;
}

#else /* SIT */
int sit; /* keep lint and some compilers happy */
#endif /* SIT */
