#define FILEHDRSIZE	22
#define TOTALSIZE	64
#define L_HSIZE		0
#define L_HCRC		1
#define L_METHOD	2
#define L_PSIZE		7
#define L_UPSIZE	11
#define L_LASTMOD	15
#define L_ATTRIBUTE	19

/* Level 0 and level 1 headers */
#define L_NLENGTH	21
#define L_NAME		22
/* Offset after name */
#define L_CRC		0
#define L_ETYPE		2
#define L_EXTENDSZ	3
#define L_EXTEND	4

/* Level 2 header */
#define L_2CRC		21
#define L_2ETYPE	23
#define L_2EXTENDSZ	24
#define L_2EXTEND	25

/* Extension definition, EXTEND defines the size of the extension. */
#define L_KIND		0	/* After EXTEND */
#define L_ENAME		2	/* Extension name, EXTEND-3 bytes long */
/* Offset after name */
#define L_EEXTENDSZ	0
#define L_EEXTEND	1

typedef struct fileHdr { /* 58 bytes */
	unsigned char	hsize;
	unsigned char	hcrc;
	char		method[5];
	unsigned long	psize;
	unsigned long	upsize;
	unsigned long	lastmod;
	unsigned short	attribute;
	unsigned char	nlength;
	char		name[32];
	unsigned short	crc;
	unsigned char	etype;
	unsigned char	extendsize;
	char		*extend;
	char		*data;
};

/* Currently known methods: */
#define	lh0	0
#define	lh1	1
#define lh2	2
#define lh3	3
#define lh4	4
#define	lh5	5
#define lz4	6
#define	lz5	7
#define	lzs	8

extern char *lzh_pointer;
extern char *lzh_data;
extern char *lzh_finfo;
extern int lzh_fsize;
extern int lzh_kind;
extern char *lzh_file;

