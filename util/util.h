typedef struct real_time {
	int year;
	int month;
	int day;
	int hours;
	int minutes;
	int seconds;
} real_time;

extern unsigned long get4();
extern unsigned long get4i();
extern unsigned long get2();
extern unsigned long get2i();
extern unsigned char getb();
extern void copy();
extern int do_query();
extern void put4();
extern void put2();
extern void do_indent();
extern real_time set_time();
extern unsigned long tomactime();
extern real_time frommactime();

