#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/kind.h"
#include "../util/util.h"

extern void dir();
extern void mcb();
extern void do_indent();

static void skip_file();
#ifdef SCAN
static void get_idf();
#endif /* SCAN */

void macbinary()
{
    char header[INFOBYTES];
    int c;

    while(1) {
	if((c = fgetc(infp)) == EOF) {
	    break;
	}
	(void)ungetc(c, infp);
	if(fread(header, 1, INFOBYTES, infp) != INFOBYTES) {
	    (void)fprintf(stderr, "Can't read MacBinary header.\n");
	    exit(1);
	}
	if(header[I_NAMEOFF] & 0x80) {
	    dir(header);
	    continue;
	}
	in_data_size = get4(header + I_DLENOFF);
	in_rsrc_size = get4(header + I_RLENOFF);
	in_ds = (((in_data_size + 127) >> 7) << 7);
	in_rs = (((in_rsrc_size + 127) >> 7) << 7);
	ds_skip = in_ds - in_data_size;
	rs_skip = in_rs - in_rsrc_size;
	if(dir_skip != 0) {
	    skip_file(in_ds + in_rs);
	    continue;
	}
#ifdef SCAN
	if(header[I_NAMEOFF] == 0) {
	    get_idf((int)header[I_NAMEOFF + 1]);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* SCAN */
	if(header[0] == 0 /* MORE CHECKS HERE! */) {
	    mcb(header, (unsigned long)in_rsrc_size,
			(unsigned long)in_data_size, in_ds + in_rs);
	    continue;
	} else {
	    (void)fprintf(stderr, "Unrecognized header.\n");
	    exit(1);
	}
    }
}

static void skip_file(skip)
    int skip;
{
    char buff[1024];
    int n;

    while(skip > 0) {
	n = (skip < 1024 ? skip : 1024);
	if(fread(buff, 1, n, infp) != n) {
	    (void)fprintf(stderr, "Incomplete file.\n");
	    exit(1);
	}
	skip -= n;
    }
}

#ifdef SCAN
static void get_idf(kind)
    int kind;
{
    char filename[1024], filename1[255];

    if(fread(filename, 1, in_data_size, infp) != in_data_size) {
	(void)fprintf(stderr, "Incomplete file.\n");
	exit(1);
    }
    filename[in_data_size] = 0;
    if(list) {
	do_indent(indent);
	switch(kind) {
	case UNIX_NAME:
	    (void)fprintf(stderr, "Unix filename: \"%s\"\n", filename);
	    break;
	case PACK_NAME:
	    transname(filename, filename1, in_data_size);
	    (void)fprintf(stderr, "Packed filename: \"%s\"\n", filename1);
	    break;
	case ARCH_NAME:
	    transname(filename, filename1, in_data_size);
	    (void)fprintf(stderr, "Archive name: \"%s\"\n", filename1);
	    break;
	case UNKNOWN:
	    (void)fprintf(stderr, "Unknown method detected\n");
	    break;
	case ERROR:
	    (void)fprintf(stderr, "Error detected\n");
	    break;
	case PROTECTED:
	    (void)fprintf(stderr, "Protected file detected\n");
	    break;
	case COPY:
	    (void)fprintf(stderr, "Copied file found\n");
	    break;
	default:
	    (void)fprintf(stderr, "Do not understand this identification\n");
	    exit(1);
	}
    }
}
#endif /* SCAN */

