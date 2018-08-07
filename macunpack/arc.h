#define	MAGIC1		0	/* Should be 0x1b, marks Mac extension */
#define	KIND		1	/* KIND == 0 marks end of archive */
#define	FNAME		2
#define	FILLER		33
#define	FTYPE		34
#define	FAUTH		38
#define	FINFO		42
#define	FDATA		50
#define	FRSRC		54
#define	FILLER		58
#define	MAGIC2		59	/* Should be 0x1a, true Arc header start */
#define	KIND2		60	/* Should be identical to KIND */
#define	FNAME2		61	/* A PC-ified version of the filename */
#define	SIZE		74
#define	DATE		78
#define	TIME		80
#define	CRC		82
#define	SIZE2		84	/* Not present if KIND == 1 */
#define	HEADERBYTES	88

typedef struct fileHdr { /* 84 or 88 bytes */
	char		magic1;
	char		kind;
	char		fname[31];
	char		filler;		/* ??? */
	char		ftype[4];
	char		fauth[4];
	char		finfo[8];
	unsigned long	dataLength;
	unsigned long	rsrcLength;
	char		filler;
	char		magic2;
	char		kind2;
	char		fname2[13];
	unsigned long	size;
	unsigned short	date;
	unsigned short	time;
	unsigend short	crc;
	unsigned long	size2;	/* Identical to size; this is wrong for Arc! */
};

#define	smallstored	1
#define	stored		2
#define	packed		3
#define	squeezed	4
#define	crunched1	5
#define	crunched2	6
#define	crunched3	7
#define	crunched4	8
#define	squashed	9

