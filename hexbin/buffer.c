#include "globals.h"
#include "../util/util.h"
#include "buffer.h"
#include "../fileio/wrfile.h"

extern char *malloc();
extern char *realloc();
extern void exit();

char *data_fork, *rsrc_fork;
int data_size, rsrc_size;
static int max_data_size, max_rsrc_size;
static int do_data;

void put_byte(c)
char c;
{
    if(do_data) {
	if(data_size >= max_data_size) {
	    if(max_data_size == 0) {
		data_fork = malloc(1024);
	    } else {
		data_fork = realloc(data_fork, (unsigned)max_data_size + 1024);
	    }
	    max_data_size += 1024;
	    if(data_fork == NULL) {
		(void)fprintf(stderr, "Insufficient memory.\n");
		exit(1);
	    }
	}
	data_fork[data_size++] = c;
    } else {
	if(rsrc_size >= max_rsrc_size) {
	    if(max_rsrc_size == 0) {
		rsrc_fork = malloc(1024);
	    } else {
		rsrc_fork = realloc(rsrc_fork, (unsigned)max_rsrc_size + 1024);
	    }
	    max_rsrc_size += 1024;
	    if(rsrc_fork == NULL) {
		(void)fprintf(stderr, "Insufficient memory.\n");
		exit(1);
	    }
	}
	rsrc_fork[rsrc_size++] = c;
    }
}

void set_put(data)
int data;
{
    do_data = data;
    if(do_data) {
	data_size = 0;
    } else {
	rsrc_size = 0;
    }
}

void end_put()
{
    if(info_only) {
	return;
    }
    start_info(info, (unsigned long)rsrc_size, (unsigned long)data_size);
    if(data_size != 0) {
	start_data();
	copy(out_ptr, data_fork, data_size);
    }
    if(rsrc_size != 0) {
	start_rsrc();
	copy(out_ptr, rsrc_fork, rsrc_size);
    }
    end_file();
}

