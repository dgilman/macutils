#define	IS_FOLDER	0x80
#define	F_INFO		0x40
#define VOLUME		0x30
#define	CRYPTED		0x08
#define N_BLOCKS	0x07

#define	LEAVE_FOLDER	0x80
#define	ONLY_FOLDER	0x40
#define	FOLDER		0x20
#define	DATE_PRESENT	0x10
#define	HAS_DATA	0x08
#define	HAS_RSRC	0x04
#define	SHORT_DATA	0x02
#define	SHORT_RSRC	0x01
#define	REMAINS		0x1f

#define CHUNKSIZE	32767
#define BCHUNKSIZE	(CHUNKSIZE * 16 / 7)

#define	NOCOMP		1
#define COMP		2

