#define C_SIGNATURE    0
#define C_VOLUME       1
#define C_XMAGIC       2
#define C_IOFFSET      4
#define CPTHDRSIZE     8

#define C_HDRCRC       0
#define C_ENTRIES      4
#define C_COMMENT      6
#define CPTHDR2SIZE    7

#define CHDRSIZE       (CPTHDRSIZE+CPTHDR2SIZE)

#define F_FNAME          0
#define F_FOLDER        32
#define F_FOLDERSIZE	33
#define F_VOLUME        35
#define F_FILEPOS       36
#define F_FTYPE         40
#define F_CREATOR       44
#define F_CREATIONDATE  48
#define F_MODDATE       52
#define F_FNDRFLAGS     56
#define F_FILECRC       58
#define F_CPTFLAG       62
#define F_RSRCLENGTH    64
#define F_DATALENGTH    68
#define F_COMPRLENGTH   72
#define F_COMPDLENGTH   76
#define FILEHDRSIZE     80

typedef long	OSType;

typedef struct cptHdr {			/* 8 bytes */
	unsigned char	signature;	/* = 1 -- for verification */
	unsigned char	volume;		/* for multi-file archives */
	unsigned short	xmagic;		/* verification multi-file consistency*/
	unsigned long	offset;		/* index offset */
/* The following are really in header2 at offset */
	unsigned long	hdrcrc;		/* header crc */
	unsigned short	entries;	/* number of index entries */
	unsigned char	commentsize;	/* number of bytes comment that follow*/
};

typedef struct fileHdr {		/* 78 bytes */
	unsigned char	fName[32];	/* a STR32 */
	unsigned char	folder;		/* set to 1 if a folder */
	unsigned short	foldersize;	/* number of entries in folder */
	unsigned char	volume;		/* for multi-file archives */
	unsigned long	filepos;	/* position of data in file */
	OSType	fType;			/* file type */
	OSType	fCreator;		/* er... */
	unsigned long	creationDate;
	unsigned long	modDate;	/* !restored-compat w/backup prgms */
	unsigned short FndrFlags;	/* copy of Finder flags.  For our
						purposes, we can clear:
						busy,onDesk */
	unsigned long	fileCRC;	/* crc on file */
	unsigned short	cptFlag;	/* cpt flags */
	unsigned long	rsrcLength;	/* decompressed lengths */
	unsigned long	dataLength;
	unsigned long	compRLength;	/* compressed lengths */
	unsigned long	compDLength;
};


/* file format is:
	cptArchiveHdr
		file1data
			file1RsrcFork
			file1DataFork
		file2data
			file2RsrcFork
			file2DataFork
		.
		.
		.
		fileNdata
			fileNRsrcFork
			fileNDataFork
	cptIndex
*/



/* cpt flags */
#define encryp	1	/* file is encrypted */
#define crsrc	2	/* resource fork is compressed */
#define cdata	4	/* data fork is compressed */
/*      ????	8	/* unknown */

#define CIRCSIZE	8192

