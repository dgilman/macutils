#define INIT_CRC crcinit

extern unsigned long arc_crcinit;
extern unsigned long binhex_crcinit;
extern unsigned long zip_crcinit;

extern unsigned long arc_updcrc();
extern unsigned long binhex_updcrc();
extern unsigned long zip_updcrc();

extern unsigned long crcinit;
extern unsigned long (*updcrc)();

