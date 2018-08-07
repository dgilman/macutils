#include "zmahdr.h"

#define	Z_HDRSIZE	78

#define	Z_WHAT		0	/* What kind of data? */
#define	Z_HLEN		1	/* Header length */
#define	Z_BFLAGS	2	/* Boolean flags */
#define	Z_NEXT		4	/* Pointer to next entry */
#define	Z_CRLEN		8	/* Length compressed resource */
#define	Z_CDLEN		12	/* Length compressed data */
#define	Z_URLEN		16	/* Length uncompressed resource */
#define	Z_UDLEN		20	/* Length uncompressed data */
#define	Z_TYPE		24	/* File type */
#define	Z_AUTH		28	/* File creator */
#define Z_CONTS		28	/* Directory contents pointer; overlayed */
#define	Z_MDATE		32	/* Date */
#define	Z_COMMENT	36	/* Comment offset, currently unused */
#define	Z_FLAGS		40	/* Finder flags */
#define	Z_DCRC		42	/* Data crc */
#define	Z_RCRC		44	/* Resource crc */
#define	Z_FNAME		46	/* File name length and name */

typedef struct fileHdr {		/* 78 bytes */
	char		deleted;	/* Not in original, split off from: */
	char		what;		/* What kind?  Negative if deleted */
	unsigned char	hlen ;		/* Header length */
	unsigned short	boolFlags;	/* Boolean flags */
	unsigned long	next;		/* Next entry */
	unsigned long	compRLength;	/* The compressed lengths. */
	unsigned long	compDLength;	/* For dirs, the second is # entries */
	unsigned long	rsrcLength;	/* The uncompressed lengths. */
	unsigned long	dataLength;
	unsigned long	fType;		/* file type */
	unsigned long	fCreator;	/* er... */
	unsigned long	modDate;	/* !restored-compat w/backup prgms */
	unsigned long	comment;	/* Comment offset */
	unsigned short	FndrFlags;	/* copy of Finder flags.  For our
						purposes, we can clear:
						busy,onDesk */
	unsigned short	dataCRC;	/* Data fork crc */
	unsigned short	rsrcCRC;	/* Resource fork crc */
	unsigned char	fName[32];	/* a STR32 */
	/* The following are overlayed in the original structure */
	unsigned long	conts;		/* Pointer to directory contents */
};

/* zma types (see what) */
#define	z_noth	0	/* ??? */
#define z_file	1	/* file is compressed */
#define z_plain	2	/* file is uncompressed */
#define	z_dir	3	/* directory */
#define	z_plug	4	/* for plug in, not supported */

