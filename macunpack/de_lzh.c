#include "macunpack.h"
#ifdef ZMA
#define DELZH
#endif /* ZMA */
#ifdef LZH
#define DELZH
#endif /* LZH */
#ifdef DELZH
#include "globals.h"
#include "../util/masks.h"
#include "../fileio/wrfile.h"
#include "bits_be.h"

/* This code is valid for bitsused upto 15. */
#define DICBIT    13    /* 12(-lh4-) or 13(-lh5-) */
#define UCHAR_MAX    255
#define THRESHOLD    3

static int decoded;
static int bitsused;
static unsigned int blocksize;
static unsigned int decode_c();
static unsigned int decode_p();
static void make_table();

/* lzh compression */
void de_lzh(ibytes, obytes, data, bits)
long ibytes;
long obytes;
char **data;
int bits;
{
    unsigned int i, r, c;
    int remains;

    bit_be_inbytes = ibytes;
    bit_be_filestart = *data;
    bitsused = bits;
    bit_be_init_getbits();
    blocksize = 0;
    decoded = 0;
    r = 0;
    for(;;) {
	c = decode_c();
	if(decoded) {
	    *data = bit_be_filestart;
	    return;
	}
	if(c <= UCHAR_MAX) {
	    out_ptr[r++] = c;
	    obytes--;
	    if(obytes == 0) {
		*data = bit_be_filestart;
		return;
	    }
	} else {
	    remains = c - (UCHAR_MAX + 1 - THRESHOLD);
	    i = (r - decode_p() - 1);
	    while(--remains >= 0) {
		out_ptr[r++] = out_ptr[i++];
		obytes--;
		if(obytes == 0) {
		    *data = bit_be_filestart;
		    return;
		}
	    }
	}
    }
}

#define MAXMATCH 256
#define NC (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
#define CBIT 9
#define CODE_BIT 16
#define NP (DICBIT + 1)
#define NT (CODE_BIT + 3)
#define PBIT 4  /* smallest integer such that (1U << PBIT) > NP */
#define TBIT 5  /* smallest integer such that (1U << TBIT) > NT */
#if NT > NP
# define NPT NT
#else
# define NPT NP
#endif

static unsigned int left[2 * NC - 1], right[2 * NC - 1];
static unsigned char c_len[NC], pt_len[NPT];
static unsigned int c_table[4096], pt_table[256];

static void read_pt_len(nn, nbit, i_special)
int nn;
int nbit;
int i_special;
{
    int i, c, n;
    unsigned int mask;

    n = bit_be_getbits(nbit);
    if (n == 0) {
	c = bit_be_getbits(nbit);
	for (i = 0; i < nn; i++) {
	    pt_len[i] = 0;
	}
	for (i = 0; i < 256; i++) {
	    pt_table[i] = c;
	}
    } else {
	i = 0;
	while (i < n) {
	    c = bit_be_bitbuf >> (BITBUFSIZ - 3);
	    if (c == 7) {
		mask = (unsigned) 1 << (BITBUFSIZ - 1 - 3);
		while (mask & bit_be_bitbuf) {
		    mask >>= 1;
		    c++;
		}
	    }
	    bit_be_fillbuf((c < 7) ? 3 : c - 3);
	    pt_len[i++] = c;
	    if (i == i_special) {
		c = bit_be_getbits(2);
		while (--c >= 0) {
		    pt_len[i++] = 0;
		}
	    }
	}
	while (i < nn) {
	    pt_len[i++] = 0;
	}
	make_table(nn, pt_len, 8, pt_table);
    }
}

static void read_c_len()
{
    int i, c, n;
    unsigned int mask;

    n = bit_be_getbits(CBIT);
    if (n == 0) {
	c = bit_be_getbits(CBIT);
	for (i = 0; i < NC; i++) {
	    c_len[i] = 0;
	}
	for (i = 0; i < 4096; i++) {
	    c_table[i] = c;
	}
    } else {
	i = 0;
	while (i < n) {
	    c = pt_table[bit_be_bitbuf >> (BITBUFSIZ - 8)];
	    if (c >= NT) {
		mask = (unsigned) 1 << (BITBUFSIZ - 1 - 8);
		do {
		    if (bit_be_bitbuf & mask) {
			c = right[c];
		    } else {
			c = left [c];
		    }
		    mask >>= 1;
		} while (c >= NT);
	    }
	    bit_be_fillbuf((int)pt_len[c]);
	    if (c <= 2) {
		if (c == 0) {
		    c = 1;
		} else if (c == 1) {
		    c = bit_be_getbits(4) + 3;
		} else {
		    c = bit_be_getbits(CBIT) + 20;
		}
		while (--c >= 0) {
		    c_len[i++] = 0;
		}
	    } else {
		c_len[i++] = c - 2;
	    }
	}
	while (i < NC) {
	    c_len[i++] = 0;
	}
	make_table(NC, c_len, 12, c_table);
    }
}

static unsigned int decode_c()
{
    unsigned int j, mask;

    if (blocksize == 0) {
	blocksize = bit_be_getbits(16);
	if (blocksize == 0) {
	    decoded = 1;
	    return 0;
	}
	read_pt_len(NT, TBIT, 3);
	read_c_len();
	read_pt_len(bitsused + 1, PBIT, -1);
    }
    blocksize--;
    j = c_table[bit_be_bitbuf >> (BITBUFSIZ - 12)];
    if (j >= NC) {
	mask = (unsigned) 1 << (BITBUFSIZ - 1 - 12);
	do {
	    if (bit_be_bitbuf & mask) {
		j = right[j];
	    } else {
		j = left [j];
	    }
	    mask >>= 1;
	} while (j >= NC);
    }
    bit_be_fillbuf((int)c_len[j]);
    return j;
}

static unsigned int decode_p()
{
    unsigned int j, mask;

    j = pt_table[bit_be_bitbuf >> (BITBUFSIZ - 8)];
    if (j > bitsused) {
	mask = (unsigned) 1 << (BITBUFSIZ - 1 - 8);
	do {
	    if (bit_be_bitbuf & mask) {
		j = right[j];
	    } else {
		j = left [j];
	    }
	    mask >>= 1;
	} while (j > bitsused);
    }
    bit_be_fillbuf((int)pt_len[j]);
    if (j != 0) {
	j = ((unsigned) 1 << (j - 1)) + bit_be_getbits((int) (j - 1));
    }
    return j;
}

static void make_table(nchar, bitlen, tablebits, table)
int nchar;
unsigned char bitlen[];
int tablebits;
unsigned int table[];
{
    unsigned int count[17], weight[17], start[18], *p;
    unsigned int i, k, len, ch, jutbits, avail, nextcode, mask;

    for (i = 1; i <= 16; i++) {
	count[i] = 0;
    }
    for (i = 0; i < nchar; i++) {
	count[bitlen[i]]++;
    }

    start[1] = 0;
    for (i = 1; i <= 16; i++) {
	start[i + 1] = start[i] + (count[i] << (16 - i));
    }

    jutbits = 16 - tablebits;
    for (i = 1; i <= tablebits; i++) {
	start[i] >>= jutbits;
	weight[i] = (unsigned) 1 << (tablebits - i);
    }
    while (i <= 16) {
       weight[i] = (unsigned) 1 << (16 - i);
       i++;
    }

    i = start[tablebits + 1] >> jutbits;
    if (i != (unsigned int)((unsigned) 1 << 16)) {
	k = 1 << tablebits;
	while (i != k) {
	    table[i++] = 0;
	}
    }

    avail = nchar;
    mask = (unsigned) 1 << (15 - tablebits);
    for (ch = 0; ch < nchar; ch++) {
	if ((len = bitlen[ch]) == 0) {
	    continue;
	}
	nextcode = start[len] + weight[len];
	if (len <= tablebits) {
	    for (i = start[len]; i < nextcode; i++) {
		table[i] = ch;
	    }
	} else {
	    k = start[len];
	    p = &table[k >> jutbits];
	    i = len - tablebits;
	    while (i != 0) {
		if (*p == 0) {
		    right[avail] = left[avail] = 0;
		    *p = avail++;
		}
		if (k & mask) {
		    p = &right[*p];
		} else {
		    p = &left[*p];
		}
		k <<= 1;
		i--;
	    }
	    *p = ch;
	}
	start[len] = nextcode;
    }
}
#else /* DELZH */
int delzh; /* keep lint and some compilers happy */
#endif /* DELZH */

