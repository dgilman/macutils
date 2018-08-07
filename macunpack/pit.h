#define H_NAMELEN 63

#define H_NLENOFF 0
#define H_NAMEOFF 1
#define H_TYPEOFF 64
#define H_AUTHOFF 68
#define H_FLAGOFF 72
#define	H_LOCKOFF 74
#define H_DLENOFF 76
#define H_RLENOFF 80
#define H_CTIMOFF 84
#define H_MTIMOFF 88
#define H_HDRCRC  92
#define HDRBYTES  94

struct pit_header {		/* Packit file header (92 bytes) */
	unsigned char nlen;	/* number of characters in packed file name */
	char name[63];		/* name of packed file */
	char type[4];		/* file type */
	char auth[4];		/* file creator */
	unsigned short flags;	/* file flags (?) */
	unsigned short lock;	/* unknown */
	unsigned long dlen;	/* number of bytes in data fork */
	unsigned long rlen;	/* number of bytes in resource fork */
	unsigned long ctim;	/* file creation time */
	unsigned long mtim;	/* file modified time */
	unsigned short hdrCRC;	/* CRC */
};

#define nocomp	0
#define huffman	1

