/* crc.c; do crc calculation. */

#include <stdio.h>
#include "hexbin.h"
#include "crc.h"
#include "../util/masks.h"
#include "globals.h"

extern void exit();

unsigned long crc;

#ifdef HQX
void comp_q_crc(c)
register unsigned int c;
{
    unsigned char cc = c;

    crc = binhex_updcrc(crc, &cc, 1);
}

void comp_q_crc_n(s, e)
register unsigned char *s, *e;
{
    crc = binhex_updcrc(crc, s, e - s);
}
#endif /* HQX */

void verify_crc(calc_crc, file_crc)
unsigned long calc_crc, file_crc;
{
    calc_crc &= WORDMASK;
    file_crc &= WORDMASK;

    if(calc_crc != file_crc) {
        (void)fprintf(stderr, "CRC mismatch: got 0x%04lx, need 0x%04lx\n",
		file_crc, calc_crc);
#ifdef SCAN
	do_error("hexbin: CRC error");
#endif /* SCAN */
	exit(1);
    }
}

