#define INFOBYTES 128

/* The following are copied out of macput.c/macget.c */
#define I_NAMEOFF 1
/* 65 <-> 80 is the FInfo structure */
#define I_TYPEOFF 65
#define I_AUTHOFF 69
#define I_FLAGOFF 73
#define I_LOCKOFF 81
#define I_DLENOFF 83
#define I_RLENOFF 87
#define I_CTIMOFF 91
#define I_MTIMOFF 95

#define F_NAMELEN 63
#define I_NAMELEN 69    /* 63 + strlen(".info") + 1 */

#define INITED_MASK	1
#define PROTCT_MASK	0x40

