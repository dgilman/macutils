#include "printhdr.h"
#include "globals.h"

/* print out header information in human-readable format */
void print_header0(skip)
int skip;
{
    if(listmode) {
	(void)fprintf(stderr, "name=\"%s\", ", trname);
	if(skip) {
	    (void)fprintf(stderr, "\n");
	}
    }
}

/* print out header information in human-readable format */
void print_header1(skip1, skip2)
int skip1, skip2;
{
    char ftype[5], fauth[5];

    transname(mh.m_type, ftype, 4);
    transname(mh.m_author, fauth, 4);
    if(listmode) {
	if(skip1) {
	    (void)fprintf(stderr, "\t");
	}
	(void)fprintf(stderr, "type=%4.4s, author=%4.4s, ", ftype, fauth);
	if(skip2) {
	    (void)fprintf(stderr, "\n");
	}
    }
}

/* print out header information in human-readable format */
void print_header2(skip)
{
    if(listmode) {
	if(skip) {
	    (void)fprintf(stderr, "\t");
	}
	(void)fprintf(stderr, "data=%ld, rsrc=%ld\n",
	    mh.m_datalen, mh.m_rsrclen);
    }
}

