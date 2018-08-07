#define S_SIGNATURE    0
#define S_NUMFILES     4
#define S_ARCLENGTH    6
#define S_SIGNATURE2  10
#define	S_VERSION     14
#define SITHDRSIZE    22

#define F_COMPRMETHOD    0
#define F_COMPDMETHOD    1
#define F_FNAME          2
#define F_FTYPE         66
#define F_CREATOR       70
#define F_FNDRFLAGS     74
#define F_CREATIONDATE  76
#define F_MODDATE       80
#define F_RSRCLENGTH    84
#define F_DATALENGTH    88
#define F_COMPRLENGTH   92
#define F_COMPDLENGTH   96
#define F_RSRCCRC      100
#define F_DATACRC      102
#define F_HDRCRC       110
#define FILEHDRSIZE    112

typedef long OSType;

typedef struct sitHdr {			/* 22 bytes */
	OSType	signature;		/* = 'SIT!' -- for verification */
	unsigned short	numFiles;	/* number of files in archive */
	unsigned long	arcLength;	/* length of entire archive incl.
						hdr. -- for verification */
	OSType	signature2;		/* = 'rLau' -- for verification */
	unsigned char	version;	/* version number */
	char reserved[7];
};

typedef struct fileHdr {		/* 112 bytes */
	unsigned char	compRMethod;	/* rsrc fork compression method */
	unsigned char	compDMethod;	/* data fork compression method */
	unsigned char	fName[64];	/* a STR63 */
	OSType	fType;			/* file type */
	OSType	fCreator;		/* er... */
	unsigned short FndrFlags;	/* copy of Finder flags.  For our
						purposes, we can clear:
						busy,onDesk */
	unsigned long	creationDate;
	unsigned long	modDate;	/* !restored-compat w/backup prgms */
	unsigned long	rsrcLength;	/* decompressed lengths */
	unsigned long	dataLength;
	unsigned long	compRLength;	/* compressed lengths */
	unsigned long	compDLength;
	unsigned short rsrcCRC;		/* crc of rsrc fork */
	unsigned short dataCRC;		/* crc of data fork */
	char reserved[6];
	unsigned short hdrCRC;		/* crc of file header */
};


/* file format is:
	sitArchiveHdr
		file1Hdr
			file1RsrcFork
			file1DataFork
		file2Hdr
			file2RsrcFork
			file2DataFork
		.
		.
		.
		fileNHdr
			fileNRsrcFork
			fileNDataFork
*/



/* compression methods */
#define nocomp	0	/* just read each byte and write it to archive */
#define rle	1	/* RLE compression */
#define lzc	2	/* LZC compression */
#define huffman	3	/* Huffman compression */
#define lzah	5	/* LZ with adaptive Huffman */
#define fixhuf	6	/* Fixed Huffman table */
#define mw	8	/* Miller-Wegman encoding */
/* this bit says whether the file is protected or not */
#define prot	16	/* password protected bit */
/* rsrc & data compress are identical here: */
#define sfolder	32	/* start of folder */
#define efolder	33	/* end of folder */
#define sknown	0x16f	/* known compression methods */

/* all other numbers are reserved */

#define	ESC	0x90	/* repeat packing escape */

