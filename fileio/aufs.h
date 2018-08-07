#define MAXCLEN 199		/* max size of a comment string */
#define FINFOLEN 32		/* Finder info is 32 bytes */
#define MAXMACFLEN 31		/* max Mac file name length */
#define FI_MAGIC1 255
#define FI_VERSION 0x10		/* version major 1, minor 0 */
				/* if we have more than 8 versions wer're */
				/* doiong something wrong anyway */
#define FI_MAGIC 0xda
#define FI_BM_SHORTFILENAME 0x1	/* is this included? */
#define FI_BM_MACINTOSHFILENAME 0x2 /* is this included? */
#define FI_MDATE 0x01		/* mtime & utime are valid */
#define FI_CDATE 0x02		/* ctime is valid */

typedef struct {
    char	fi_fndr[FINFOLEN];	/* finder info */
    short	fi_attr;		/* attributes */
    char	fi_magic1;		/* addional magic word check */
    char	fi_version;		/* version number */
    char	fi_magic;		/* magic word check */
    char	fi_bitmap;		/* bitmap of included info */
    char	fi_shortfilename[12+1];	/* possible short file name */
    char	fi_macfilename[32+1];	/* possible macintosh file name */
    char	fi_comln;		/* comment length */
    char	fi_comnt[MAXCLEN+1];	/* comment string */
#ifdef AUFSPLUS
    char	fi_datemagic;		/* sanity check */
    char	fi_datevalid;		/* validity flags */
    char	fi_ctime[4];		/* mac file create time */
    char	fi_mtime[4];		/* mac file modify time */
    char	fi_utime[4];		/* (real) time mtime was set */
#endif /* AUFSPLUS */
} FileInfo;

