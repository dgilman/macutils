#include "../util/masks.h"
#include "bits_be.h"

unsigned int bit_be_bitbuf;
char *bit_be_filestart;
int bit_be_inbytes;

static unsigned int bit_be_subbitbuf;
static int bit_be_bitcount;

void bit_be_fillbuf(n)  /* Shift bit_be_bitbuf n bits left, read n bits */
int n;
{
    bit_be_bitbuf <<= n;
    while (n > bit_be_bitcount) {
	bit_be_bitbuf |= bit_be_subbitbuf << (n -= bit_be_bitcount);
	if(bit_be_inbytes == 0) {
	    bit_be_subbitbuf = 0;
	} else {
	    bit_be_subbitbuf = *bit_be_filestart++ & BYTEMASK;
	    bit_be_inbytes--;
	}
	bit_be_bitcount = 8;
    }
    bit_be_bitbuf |= bit_be_subbitbuf >> (bit_be_bitcount -= n);
    bit_be_bitbuf &= WORDMASK;
}

unsigned int bit_be_getbits(n)
int n;
{
    unsigned int x;

    x = bit_be_bitbuf >> (BITBUFSIZ - n);
    bit_be_fillbuf(n);
    return x;
}

void bit_be_init_getbits()
{
    bit_be_bitbuf = 0;
    bit_be_subbitbuf = 0;
    bit_be_bitcount = 0;
    bit_be_fillbuf(BITBUFSIZ);
}

