#define INITCRC binhex_crcinit

extern unsigned long crc;
extern unsigned long binhex_crcinit;
extern unsigned long binhex_updcrc();

extern void comp_q_crc();
extern void comp_q_crc_n();
extern void verify_crc();

