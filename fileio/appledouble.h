#define	FI_MAGIC	333319
#define	FI_VERSION	1
#define	FI_FILL5	5
#define	FI_FILL6	2
#define	FI_HLEN		589
#define	FI_FILL7	3
#define	FI_NAMPTR	86
#define	FI_FILL9	4
#define	FI_COMMPTR	341
#define	FI_FILL12	7
#define	FI_TIMEPTR	541
#define	FI_TIMESIZE	16
#define	FI_FILL15	9
#define	FI_INFOPTR	557
#define	FI_INFOSIZE	32

/* All as char[n] because of possible alignment problems.  But is this needed?
   Is this stuff in host order or in client order?  Assuming client order for
   the moment.  Will not be a problem on big-endian machines. */
typedef struct {
	char	fi_magic[4];	/* magic header */
	char	fi_version[2];	/* version number */
	char	fi_fill1[4];	/* = 0, ???? */
	char	fi_fill2[4];	/* = 0, ???? */
	char	fi_fill3[4];	/* = 0, ???? */
	char	fi_fill4[4];	/* = 0, ???? */
	char	fi_fill5[4];	/* = 5, ???? */
	char	fi_fill6[4];	/* = 2, ???? */
	char	fi_hlen[4];	/* = 589, header length */
	char	fi_rsrc[4];	/* resource length */
	char	fi_fill7[4];	/* = 3, ???? */
	char	fi_namptr[4];	/* = 86, filename pointer */
	char	fi_namlen[4];	/* Mac filename length */
	char	fi_fill9[4];	/* = 4, ???? */
	char	fi_commptr[4];	/* = 341, comment pointer */
	char	fi_commsize[4];	/* = 0, comment size */
	char	fi_fill12[4];	/* = 7, ???? */
	char	fi_timeptr[4];	/* = 541, pointer to times */
	char	fi_timesize[4];	/* = 16, size of times */
	char	fi_fill15[4];	/* = 9, ???? */
	char	fi_infoptr[4];	/* = 557, finder info pointer */
	char	fi_infosize[4];	/* = 32, finder info size */
	char	fi_name[255];	/* Macintosh filename */
	char	fi_comment[200];/* = 0, Comment */
	char	fi_ctime[4];	/* Creation time (Unix time) */
	char	fi_mtime[4];	/* Modification time (Unix time) */
	char	fi_fill19[4];	/* = 0, ???? */
	char	fi_fill20[4];	/* = 0, ???? */
	char	fi_type[4];	/* File type */
	char	fi_auth[4];	/* File creator */
	char	fi_finfo[24];	/* Finder info */
} FileInfo;

