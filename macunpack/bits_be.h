#define	BITBUFSIZ	16

extern unsigned int bit_be_bitbuf;
extern char *bit_be_filestart;
extern int bit_be_inbytes;

extern void bit_be_fillbuf();
extern unsigned int bit_be_getbits();
extern void bit_be_init_getbits();

