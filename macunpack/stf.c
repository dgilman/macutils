#include "macunpack.h"
#ifdef STF
#include "stf.h"
#include "globals.h"
#include "huffman.h"
#include "../util/curtime.h"
#include "../fileio/wrfile.h"
#include "../fileio/machdr.h"
#include "../util/util.h"

extern void de_huffman();
extern void set_huffman();

typedef struct{
    int num;
    int next;
} table_struct;

static table_struct table[511];
static char length[256];

static void stf_wrfile();
static void stf_wrfork();
static void stf_construct();

void stf(ibytes)
unsigned long ibytes;
{
    char magic[3], fauth[5], ftype[5];
    int filel, i;
    unsigned int rsrcLength, dataLength;
    unsigned long curtime;

    set_huffman(HUFF_LE);
    for(i = 0; i < 3; i++) {
	magic[i] = getb(infp);
    }
    if(strncmp(magic, MAGIC, 3)) {
	(void)fprintf(stderr, "Error in magic header.\n");
#ifdef SCAN
	do_error("macunpack: Error in magic header");
#endif /* SCAN */
	exit(1);
    }
    for(i = 0; i < INFOBYTES; i++) {
	info[i] = 0;
    }
    filel = getb(infp);
    info[I_NAMEOFF] = filel;
    i = filel;
    for(i = 1; i <= filel; i++) {
	info[I_NAMEOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_TYPEOFF + i] = getb(infp);
    }
    for(i = 0; i < 4; i++) {
	info[I_AUTHOFF + i] = getb(infp);
    }
    curtime = (unsigned long)time((time_t *)0) + TIMEDIFF;
    put4(info + I_CTIMOFF, curtime);
    put4(info + I_MTIMOFF, curtime);
    rsrcLength = 0;
    for(i = 0; i < 4; i++) {
	rsrcLength = (rsrcLength << 8) + getb(infp);
    }
    put4(info + I_RLENOFF, (unsigned long)rsrcLength);
    dataLength = 0;
    for(i = 0; i < 4; i++) {
	dataLength = (dataLength << 8) + getb(infp);
    }
    put4(info + I_DLENOFF, (unsigned long)dataLength);
    ibytes -= filel + 20;
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
    stf_wrfile((unsigned long)rsrcLength, (unsigned long)dataLength, ibytes);
}

static void stf_wrfile(rsrcLength, dataLength, ibytes)
unsigned long rsrcLength, dataLength, ibytes;
{
    unsigned long num = 0;

    if(write_it) {
	define_name(text);
	start_info(info, rsrcLength, dataLength);
	start_rsrc();
	stf_wrfork(&num, rsrcLength, 0);
	start_data();
	stf_wrfork(&num, rsrcLength + dataLength, (int)(rsrcLength & 0xfff));
	end_file();
    } else {
	for(num = 0; num < ibytes; num++) {
	    (void)getb(infp);
	}
    }
    if(verbose) {
	(void)fprintf(stderr, "\tHuffman compressed (%4.1f%%).\n",
		100.0 * ibytes / (rsrcLength + dataLength));
    }
}

static void stf_wrfork(num, towrite, offs)
unsigned long *num, towrite;
int offs;
{
    int c, k, max, i, i1;
    char *tmp_out_ptr;

    while(*num < towrite) {
	if((*num & 0xfff) == 0) {
	    clrhuff();
	    c = getb(infp) & 0xff;
	    k = c;
	    max = 0;
	    for(i = 0; i < k; i++) {
		c = getb(infp) & 0xff;
		nodelist[i + 1].flag = 1;
		nodelist[i + 1].byte = i + 1;
		table[i + 1].num = c;
		table[i + 1].next = 0;
		if(c > max) {
		    max = c;
		}
	    }
	    for(i = k; i < 32; i++) {
		nodelist[i + 1].flag = 1;
		nodelist[i + 1].byte = i + 1;
		table[i + 1].num = 0;
		table[i + 1].next = 0;
	    }
	    k = 0;
	    for(i = 0; i <= max; i++) {
		for(i1 = 1; i1 < 33; i1++) {
		    if(table[i1].num == i) {
			table[k].next = i1;
			k = i1;
		    }
		}
	    }
	    stf_construct(32);
	    tmp_out_ptr = out_ptr;
	    out_ptr = length;
	    de_huffman((unsigned long)256);
	    out_ptr = tmp_out_ptr;
	    for(i = 1; i < 257; i++) {
		table[i].num = 0x40000000 >> length[i - 1];
		nodelist[i].flag = 1;
		nodelist[i].byte = i - 1;
		table[i].next = 0;
	    }
	    k = 0;
	    for(i = 1; i < 0x40000000; i <<= 1) {
		for(i1 = 1; i1 < 257; i1++) {
		    if(table[i1].num == i) {
			table[k].next = i1;
			k = i1;
		    }
		}
	    }
	    stf_construct(256);
	}
	i = 0x1000 - offs;
	offs = 0;
	if(i > towrite - *num) {
	    i = towrite - *num;
	}
	de_huffman((unsigned long)i);
	*num += i;
    }
}

static void stf_construct(n)
int n;
{
    int i, i1, i2, j1, k;

    j1 = n + 1;
    i = table[0].next;
    i1 = table[i].next;
    while(table[i1].next != 0) {
	k = table[i].num + table[i1].num;
	table[j1].num = k;
	nodelist[j1].flag = 0;
	nodelist[j1].zero = nodelist + i;
	nodelist[j1].one = nodelist + i1;
	i2 = i1;
	i = table[i2].next;
	while(i != 0 && table[i].num <= k) {
	    i2 = i;
	    i = table[i].next;
	}
	table[j1].next = i;
	table[i2].next = j1;
	i = table[i1].next;
	i1 = table[i].next;
	j1++;
    }
    table[0].num = table[i].num + table[i1].num;
    nodelist[0].flag = 0;
    nodelist[0].zero = nodelist + i;
    nodelist[0].one = nodelist + i1;
}
#else /* STF */
int stf; /* keep lint and some compilers happy */
#endif /* STF */

