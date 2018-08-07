extern char *out_buffer, *out_ptr;

extern void define_name();
extern void start_info();
extern void start_rsrc();
extern void start_data();
extern void end_file();
#ifdef SCAN
extern void do_idf();
#endif /* SCAN */
extern void do_mkdir();
extern void enddir();
extern char *get_mina();

