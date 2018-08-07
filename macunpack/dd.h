#define	MAGIC1		"DDAR"
#define	MAGIC2		"\253\315\000\124"

/* Initial header */
#define ARCHHDRCRC	 76
#define ARCHHDRSIZE	 78

/* File headers */
#define D_MAGIC		  0
#define D_FILL1		  4
#define D_FNAME		  8
#define D_ISDIR		 72
#define D_ENDDIR	 73
#define D_DATALENGTH	 74
#define D_RSRCLENGTH	 78
#define D_CTIME		 82
#define D_MTIME		 86
#define D_FTYPE		 90
#define D_CREATOR	 94
#define D_FNDRFLAGS	 98
#define D_FILL2		100
#define D_DATACRC	118
#define D_RSRCCRC	120
#define D_HDRCRC	122
#define FILEHDRSIZE	124

/* Compressed file header */
#define	C_MAGIC		 0
#define	C_DATALENGTH	 4
#define	C_DATACLENGTH	 8
#define	C_RSRCLENGTH	12
#define	C_RSRCCLENGTH	16
#define	C_DATAMETHOD	20
#define	C_RSRCMETHOD	21
#define	C_INFO1		22
#define	C_MTIME		24
#define	C_CTIME		28
#define	C_FTYPE		32
#define	C_CREATOR	36
#define	C_FNDRFLAGS	40
#define	C_FILL1		42
#define	C_DATACRC	48
#define	C_RSRCCRC	50
#define	C_INFO2		52
#define	C_DATAINFO	54
#define	C_RSRCINFO	56
#define	C_FILL2		58
#define C_DATACRC2	78
#define C_RSRCCRC2	80
#define	C_HDRCRC	82
#define	CFILEHDRSIZE	84

typedef long OSType;

typedef struct fileHdr {		/* 124 bytes */
	unsigned char	magic[4];	/* "DDAR" */
	unsigned char	fill1[4];	/* ??? */
	unsigned char	fName[64];	/* a STR63 */
	unsigned char	isdir;		/* starts a directory? */
	unsigned char	enddir;		/* terminates a directory? */
	unsigned long	dataLength;	/* lengths */
	unsigned long	rsrcLength;
	unsigned long	creationDate;
	unsigned long	modDate;
	OSType	fType;			/* file type */
	OSType	fCreator;		/* er... */
	unsigned short FndrFlags;	/* copy of Finder flags.  For our
						purposes, we can clear:
						busy,onDesk */
	unsigned char	fill2[18];	/* ??? */
	unsigned short	datacrc;	/* checksum */
	unsigned short	rsrccrc;
	unsigned short	hdrcrc;		/* true crc */
};

typedef struct fileCHdr {		/* 84 bytes */
	unsigned char	magic[4];	/* "\253\315\000\124" */
	unsigned long	dataLength;	/* lengths */
	unsigned long	dataCLength;
	unsigned long	rsrcLength;
	unsigned long	rsrcCLength;
	unsigned char	datamethod;	/* compression method used */
	unsigned char	rsrcmethod;
	unsigned char	info1;		/* flags ??? */
	unsigned char	fill3;
	unsigned long	modDate;
	unsigned long	creationDate;
	OSType	fType;			/* file type */
	OSType	fCreator;		/* er... */
	unsigned short FndrFlags;	/* copy of Finder flags.  For our
						purposes, we can clear:
						busy,onDesk */
	unsigned char	fill1[6];	/* ??? */
	unsigned short	datacrc;	/* checksum */
	unsigned short	rsrccrc;
	unsigned char	info2;		/* flags ??? */
	unsigned char	fill4;
	unsigned short	datainfo;	/* ??? */
	unsigned short	rsrcinfo;	/* ??? */
	unsigned char	fill2[20];	/* ??? */
	unsigned short	datacrc2;	/* other checksum */
	unsigned short	rsrccrc2;
	unsigned short	hdrcrc;		/* true crc */
};

#define	DD_FILE	0
#define	DD_COPY	1
#define	DD_SDIR	2
#define	DD_EDIR	3
#define	DD_IVAL	4

/* Methods used */
#define	nocomp		0
#define lzc		1
#define method2		2
#define rle		3
#define huffman		4
#define method5		5
#define	method6		6
#define lzss		7
#define	cpt_compat	8
#define	method9		9

#define	ESC		0x144	/* Repeat packing escape */

