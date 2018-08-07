#include "macunpack.h"
#ifdef JDW
#define DEHUFFMAN
#endif /* JDW */
#ifdef STF
#define DEHUFFMAN
#endif /* STF */
#ifdef PIT
#define DEHUFFMAN
#endif /* PIT */
#ifdef SIT
#define DEHUFFMAN
#endif /* SIT */
#ifdef CPT
#define DEHUFFMAN
#endif /* CPT */
#ifdef DEHUFFMAN
#include "globals.h"
#include "../util/masks.h"
#include "../fileio/wrfile.h"
#include "huffman.h"
#include "../util/util.h"

int (*get_bit)();
int bytesread;
/* 515 because StuffIt Classic needs more than the needed 511 */
struct node nodelist[515];
static int getbit_be();
static int getbit_le();
static int getdecodebyte();

static node *nodeptr, *read_sub_tree();

static int bit;

void de_huffman(obytes)
unsigned long obytes;
{
    while(obytes != 0) {
	*out_ptr++ = gethuffbyte(nodelist);
	obytes--;
    }
    return;
}

void de_huffman_end(term)
unsigned int term;
{
    int c;

    while((c = gethuffbyte(nodelist)) != term) {
	*out_ptr++ = c;
    }
}

void set_huffman(endian)
int endian;
{
    if(endian == HUFF_LE) {
	get_bit = getbit_le;
    } else if(endian == HUFF_BE) {
	get_bit = getbit_be;
    }
}

void read_tree()
{
    nodeptr = nodelist;
    bit = 0;		/* put us on a boundary */
    (void)read_sub_tree();
}

/* This routine recursively reads the Huffman encoding table and builds
   a decoding tree. */
static node *read_sub_tree()
{
    node *np;

    np = nodeptr++;
    if((*get_bit)() == 1) {
	np->flag = 1;
	np->byte = getdecodebyte();
    } else {
	np->flag = 0;
	np->zero = read_sub_tree();
	np->one  = read_sub_tree();
    }
    return np;
}

/* This routine returns the next bit in the input stream (MSB first) */
static int getbit_be()
{
    static int b;

    if(bit == 0) {
	b = getb(infp) & BYTEMASK;
	bit = 8;
	bytesread++;
    }
    bit--;
    return (b >> bit) & 1;
}

/* This routine returns the next bit in the input stream (LSB first) */
static int getbit_le()
{
    static int b;

    if(bit == 0) {
	b = getb(infp) & BYTEMASK;
	bit = 8;
	bytesread++;
    }
    bit--;
    return (b >> (7 - bit)) & 1;
}

void clrhuff()
{
    bit = 0;
}

int gethuffbyte(l_nodelist)
node *l_nodelist;
{
    register node *np;

    np = l_nodelist;
    while(np->flag == 0) {
	np = (*get_bit)() ? np->one : np->zero;
    }
    return np->byte;
}

int getihuffbyte()
{
    return gethuffbyte(nodelist);
}

static int getdecodebyte()
{
    register int i, b;

    b = 0;
    for(i = 8; i > 0; i--) {
	b = (b << 1) + (*get_bit)();
    }
    return b;
}
#else /* DEHUFFMAN */
int dehuffman; /* keep lint and some compilers happy */
#endif /* DEHUFFMAN */

