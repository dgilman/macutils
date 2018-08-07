#define	J_MAGIC		0
#define J_TYPE		6
#define J_AUTH		10
#define	J_FINFO		14
#define	J_DATALENGTH	22
#define J_RSRCLENGTH	26
#define	J_CTIME		30
#define	J_MTIME		34
#define	J_FLENGTH	38

typedef struct fileHdr {
	char		magic[6];
	unsigned long	type;
	unsigned long	auth;
	char		finfo[8];
	unsigned long	dataLength;
	unsigned long	rsrcLength;
	unsigned long	ctime;
	unsigned long	mtime;
	char		flength;
	char		fname[32];	/* actually flength */
};

