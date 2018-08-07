#include <stdio.h>
#include "../fileio/fileglob.h"
#include "masks.h"
#include "util.h"

extern void exit();

#define MACTIMOFFS    1904

static int mlength[] = {0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337};

unsigned long get4(bp)
char *bp;
{
    register int i;
    long value = 0;

    for(i = 0; i < 4; i++) {
	value <<= 8;
	value |= (*bp & BYTEMASK);
	bp++;
    }
    return value;
}

/* For if integers are stored wrong-endian. */
unsigned long get4i(bp)
char *bp;
{
    register int i;
    long value = 0;

    bp += 3;
    for(i = 0; i < 4; i++) {
        value <<= 8;
        value |= (*bp & BYTEMASK);
        bp--;
    }
    return value;
}

unsigned long get2(bp)
char *bp;
{
    register int i;
    int value = 0;

    for(i = 0; i < 2; i++) {
	value <<= 8;
	value |= (*bp & BYTEMASK);
	bp++;
    }
    return value;
}

/* For if integers are stored wrong-endian. */
unsigned long get2i(bp)
char *bp;
{
    register int i;
    long value = 0;

    bp += 1;
    for(i = 0; i < 2; i++) {
        value <<= 8;
        value |= (*bp & BYTEMASK);
        bp--;
    }
    return value;
}

unsigned char getb(fp)
FILE *fp;
{
    int c;

    bytes_read += 1;
    c = getc(fp);
    if(c == EOF) {
	(void)fprintf(stderr, "\nPremature EOF\n");
	exit(1);
    }
    return c & BYTEMASK;
}

void copy(d, s, n)
char *d, *s;
int n;
{
    while(--n >= 0) {
	*d++ = *s++;
    }
}

int do_query()
{
    char *tp, temp[10];

    (void)fprintf(stderr, "? ");
    (void) fflush(stdout);
    (void) read(2, temp, sizeof(temp));
    temp[sizeof(temp) - 1] = 0;
    tp = temp;
    while(*tp != 0) {
	if(*tp == 'y' || *tp == 'Y') {
	    return 1;
	} else {
	    tp++;
	}
    }
    return 0;
}

void put4(dest, value)
char *dest;
unsigned long value;
{
    *dest++ = (value >> 24) & BYTEMASK;
    *dest++ = (value >> 16) & BYTEMASK;
    *dest++ = (value >> 8) & BYTEMASK;
    *dest++ = value & BYTEMASK;
}

void put2(dest, value)
char *dest;
unsigned long value;
{
    *dest++ = (value >> 8) & BYTEMASK;
    *dest++ = value & BYTEMASK;
}

void do_indent(indent)
int indent;
{
    int i;

    for(i = 0; i < indent; i++) {
	(void)fputc(' ', stderr);
    }
}

real_time set_time(year, month, day, hours, minutes, seconds)
int year, month, day, hours, minutes, seconds;
{
    real_time toset;

    toset.year = year;
    toset.month = month;
    toset.day = day;
    toset.hours = hours;
    toset.minutes = minutes;
    toset.seconds = seconds;
    return toset;
}

unsigned long tomactime(time)
real_time time;
{
    long accum;
    int year;

    accum = time.month - 3;
    year = time.year;
    if(accum < 0) {
	accum += 12;
	year--;
    }
    accum = time.day + mlength[accum] + 59;
    accum += (year - MACTIMOFFS) * 365 + year / 4 - MACTIMOFFS / 4;
    accum = ((accum * 24 + time.hours) * 60 + time.minutes) * 60 + time.seconds;
    return (unsigned)accum;
}

real_time frommactime(accum)
unsigned long accum;
{
long tmp1, tmp2;
real_time time;

    time.seconds = tmp1 = accum % 60;
    accum /= 60;
    time.minutes = tmp1 = accum % 60;
    accum /= 60;
    time.hours = tmp1 = accum % 24;
    accum /= 24;
    tmp1 = (long)accum - 60;
    tmp2 = tmp1 % 1461;
    if(tmp2 < 0) {
	tmp2 += 1461;
    }
    tmp1 = (tmp1 - tmp2) / 1461;
    time.year = tmp1 * 4;
    tmp1 = tmp2 / 365;
    if(tmp1 > 3) {
	tmp1 = 3;
    }
    time.year += tmp1 + MACTIMOFFS;
    tmp2 -= tmp1 * 365;
    tmp1 = 12;
    while(mlength[--tmp1] > tmp2);
    time.day = tmp2 + 1 - mlength[tmp1];
    time.month = tmp1 + 3;
    if(tmp1 > 9) {
	time.month = tmp1 - 9;
	time.year++;
    }
    return time;
}

