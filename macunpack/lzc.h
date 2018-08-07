#define HEADERBYTES 48
#define MAGIC1	"\253\315\000\060"
#define MAGIC2	"\037\235"

#define C_DLENOFF	4
#define C_DLENOFFC	8
#define C_RLENOFF	12
#define C_RLENOFFC	16
#define C_MTIMOFF	24
#define C_CTIMOFF	28
#define C_TYPEOFF	32
#define C_AUTHOFF	36
#define C_FLAGOFF	40

typedef struct fileHdr {
	unsigned long	magic1;
	unsigned long	dataLength;
	unsigned long	dataCLength;
	unsigned long	rsrcLength;
	unsigned long	rsrcCLength;
	unsigned long	unknown1;
	unsigned long	mtime;
	unsigned long	ctime;
	unsigned long	filetype;
	unsigned long	fileauth;
	unsigned long	flag1;
	unsigned long	flag2;
};

