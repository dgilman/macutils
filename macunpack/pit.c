#include "macunpack.h"
#ifdef PIT
#include "../fileio/wrfile.h"
#include "../fileio/fileglob.h"
#include "../fileio/kind.h"
#include "globals.h"
#include "pit.h"
#include "../fileio/machdr.h"
#include "crc.h"
#include "../util/masks.h"
#include "../util/util.h"
#include "huffman.h"

extern void read_tree();
extern void de_huffman();
extern void set_huffman();

static int pit_filehdr();
static void pit_wrfile();
static void pit_nocomp();
static void pit_huffman();

void pit()
{
    struct pit_header filehdr;
    char pithdr[4];
    int decode, synced, ch;
    unsigned long data_crc, crc;

    updcrc = binhex_updcrc;
    crcinit = binhex_crcinit;
    set_huffman(HUFF_BE);
    synced = 0;
    while(1) {
	if(!synced) {
	    if(fread(pithdr, 1, 4, infp) != 4) {
		(void)fprintf(stderr, "Premature EOF\n");
#ifdef SCAN
		do_error("macunpack: Premature EOF");
#endif /* SCAN */
		exit(1);
	    }
	}
	synced = 0;
	if(strncmp(pithdr, "PEnd", 4) == 0) {
	    break;
	}
	if(strncmp(pithdr, "PMa", 3) != 0) {
	    (void)fprintf(stderr, "File contains non PackIt info %.4s\n",
		    pithdr);
#ifdef SCAN
	    do_error("macunpack: File contains non PackIt info");
#endif /* SCAN */
	    exit(1);
	}
	switch(pithdr[3]) {
	case 'g':
	case '4':
	    break;
	case '5':
	case '6':
	    if(pithdr[3] == '6') {
		(void)fprintf(stderr, "DES-");
	    }
	    (void)fprintf(stderr, "Encrypted file found, trying to sync");
	    while(!synced) {
		ch = getb(infp);
		if(ch == 'P') {
		    pithdr[0] = ch;
		    pithdr[1] = ch = getb(infp);
		    if(ch == 'M') {
			pithdr[2] = ch = getb(infp);
			if(ch == 'a') {
			    pithdr[3] = ch = getb(infp);
			    if((ch >= '4' && ch <= '6') || ch == 'g') {
				synced = 1;
			    }
			}
		    } else if(ch == 'E') {
			pithdr[2] = ch = getb(infp);
			if(ch == 'n') {
			    pithdr[3] = ch = getb(infp);
			    if(ch == 'd') {
				synced = 1;
			    }
			}
		    }
		    if(!synced) {
			(void)ungetc(ch, infp);
		    }
		}
	    }
	    (void)fprintf(stderr, ", done.\n");
#ifdef SCAN
	    do_idf("", PROTECTED);
#endif /* SCAN */
	    continue;
	default:
	    (void)fprintf(stderr, "File contains non PackIt info %.4s\n",
		    pithdr);
#ifdef SCAN
	    do_error("macunpack: File contains non PackIt info");
#endif /* SCAN */
	    exit(1);
	}
	bytes_read = 0;
	if(pithdr[3] == '4') {
	    read_tree();
	    decode = huffman;
	} else {
	    decode = nocomp;
	}
	if(pit_filehdr(&filehdr, decode) == -1) {
	    (void)fprintf(stderr, "Can't read file header\n");
#ifdef SCAN
	    do_error("macunpack: Can't read file header");
#endif /* SCAN */
	    exit(1);
	}
	bytes_written = filehdr.rlen + filehdr.dlen;
	start_info(info, filehdr.rlen, filehdr.dlen);
	start_data();
	pit_wrfile(filehdr.dlen, decode);
	data_crc = (*updcrc)(INIT_CRC, out_buffer, filehdr.dlen);
	start_rsrc();
	pit_wrfile(filehdr.rlen, decode);
	data_crc = (*updcrc)(data_crc, out_buffer, filehdr.rlen);
	if(decode == nocomp) {
	    crc = getb(infp);
	    crc = (crc << 8) | getb(infp);
	} else {
	    crc = (getihuffbyte() & BYTEMASK) |
		  ((getihuffbyte() & BYTEMASK) << 8);
	}
	if(crc != data_crc) {
	    (void)fprintf(stderr,
		    "CRC error in file: need 0x%04x, got 0x%04x\n",
		    (int)crc, (int)data_crc);
#ifdef SCAN
	    do_error("macunpack: CRC error in file");
#endif /* SCAN */
	    exit(1);
	}
	if(verbose) {
	    if(decode == nocomp) {
		(void)fprintf(stderr, "\tNo compression");
	    } else {
		(void)fprintf(stderr, "\tHuffman compressed (%4.1f%%)",
			100.0 * bytes_read / bytes_written);
	    }
	}
	if(write_it) {
	    end_file();
	}
	if(verbose) {
	    (void)fprintf(stderr, ".\n");
	}
    }
}

static int pit_filehdr(f, compr)
struct pit_header *f;
int compr;
{
    register int i;
    unsigned long crc;
    int n;
    char hdr[HDRBYTES];
    char ftype[5], fauth[5];

    for(i = 0; i < INFOBYTES; i++)
	info[i] = '\0';

    if(compr == huffman) {
	for(i = 0; i < HDRBYTES; i++) {
	    hdr[i] = getihuffbyte();
	}
    } else {
	if(fread(hdr, 1, HDRBYTES, infp) != HDRBYTES) {
	    return -1;
	}
    }
    crc = INIT_CRC;
    crc = (*updcrc)(crc, hdr, HDRBYTES - 2);

    f->hdrCRC = get2(hdr + H_HDRCRC);
    if(f->hdrCRC != crc) {
	(void)fprintf(stderr,
		"\tHeader CRC mismatch: got 0x%04x, need 0x%04x\n",
		f->hdrCRC & WORDMASK, (int)crc);
	return -1;
    }

    n = hdr[H_NLENOFF] & BYTEMASK;
    if(n > H_NAMELEN) {
	n = H_NAMELEN;
    }
    info[I_NAMEOFF] = n;
    copy(info + I_NAMEOFF + 1, hdr + H_NAMEOFF, n);
    transname(hdr + H_NAMEOFF, text, n);
    text[n] = '\0';

    f->rlen = get4(hdr + H_RLENOFF);
    f->dlen = get4(hdr + H_DLENOFF);

    write_it = 1;
    if(list) {
	transname(hdr + H_TYPEOFF, ftype, 4);
	transname(hdr + H_AUTHOFF, fauth, 4);
	do_indent(indent);
	(void)fprintf(stderr,
		"name=\"%s\", type=%4.4s, author=%4.4s, data=%ld, rsrc=%ld",
		text, ftype, fauth, (long)f->dlen, (long)f->rlen);
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

	copy(info + I_TYPEOFF, hdr + H_TYPEOFF, 4);
	copy(info + I_AUTHOFF, hdr + H_AUTHOFF, 4);
	copy(info + I_FLAGOFF, hdr + H_FLAGOFF, 2);
	copy(info + I_LOCKOFF, hdr + H_LOCKOFF, 2);
	copy(info + I_DLENOFF, hdr + H_DLENOFF, 4);
	copy(info + I_RLENOFF, hdr + H_RLENOFF, 4);
	copy(info + I_CTIMOFF, hdr + H_CTIMOFF, 4);
	copy(info + I_MTIMOFF, hdr + H_MTIMOFF, 4);
    }
    return 1;
}

static void pit_wrfile(bytes, type)
unsigned long bytes;
int type;
{
    if(bytes == 0) {
	return;
    }
    switch(type) {
    case nocomp:
	pit_nocomp(bytes);
	break;
    case huffman:
	pit_huffman(bytes);
    }
}

/*---------------------------------------------------------------------------*/
/*	No compression							     */
/*---------------------------------------------------------------------------*/
static void pit_nocomp(ibytes)
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
/*	Huffman compression						     */
/*---------------------------------------------------------------------------*/
static void pit_huffman(obytes)
unsigned long obytes;
{
    de_huffman(obytes);
}
#else /* PIT */
int pit; /* keep lint and some compilers happy */
#endif /* PIT */

