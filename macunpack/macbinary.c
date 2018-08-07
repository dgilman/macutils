#include "macunpack.h"
#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"
#include "zmahdr.h"
#include "../util/util.h"

extern void dir();
extern void mcb();
#ifdef BIN
extern void bin();
#endif /* BIN */
#ifdef JDW
extern void jdw();
#endif /* JDW */
#ifdef STF
extern void stf();
#endif /* STF */
#ifdef LZC
extern void lzc();
#endif /* LZC */
#ifdef ASQ
extern void asq();
#endif /* ASQ */
#ifdef ARC
extern void arc();
#endif /* ARC */
#ifdef PIT
extern void pit();
#endif /* PIT */
#ifdef SIT
extern void sit();
#endif /* SIT */
#ifdef DIA
extern void dia();
#endif /* DIA */
#ifdef CPT
extern void cpt();
#endif /* CPT */
#ifdef ZMA
extern void zma();
#endif /* ZMA */
#ifdef LZH
extern void lzh();
#endif /* LZH */
#ifdef DD
extern void dd_file();
extern void dd_arch();
#endif /* DD */

static void skip_file();
#ifdef SCAN
static void get_idf();
#endif /* SCAN */

#define Z	(ZMAHDRS2 + 1)

static int info_given;

void macbinary()
{
    char header[INFOBYTES];
    int c;

    while(1) {
	if((c = fgetc(infp)) == EOF) {
	    break;
	}
	(void)ungetc(c, infp);
	if(fread(header, 1, Z, infp) != Z) {
	    (void)fprintf(stderr, "Can't read MacBinary header.\n");
#ifdef SCAN
	    do_error("macunpack: Can't read MacBinary header");
#endif /* SCAN */
	    exit(1);
	}
#ifdef ZMA
	if(!strncmp(header + 1, ZMAHDR, ZMAHDRS2)) {
	    /* Can distinguish zoom data forks only this way from macbinary */
	    if(verbose) {
		(void)fprintf(stderr, "This is a \"Zoom\" archive.\n");
	    }
	    zma(header, (unsigned long)0);
	    exit(0);
	}
#endif /* ZMA */
	if(fread(header + Z, 1, INFOBYTES - Z, infp) != INFOBYTES - Z) {
	    (void)fprintf(stderr, "Can't read MacBinary header.\n");
#ifdef SCAN
	    do_error("macunpack: Can't read MacBinary header");
#endif /* SCAN */
	    exit(1);
	}
	if(verbose && !info_given) {
	    do_indent(indent);
	    (void)fprintf(stderr, "This is \"MacBinary\" input.\n");
	    info_given = 1;
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
	header[I_NAMEOFF + 1 + header[I_NAMEOFF]] = 0;
#ifdef BIN
	if(!strncmp(header + I_TYPEOFF, "TEXT", 4) &&
	   !strncmp(header + I_AUTHOFF, "BnHq", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"BinHex 5.0\" packed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    bin(header, in_data_size, 0);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "TEXT", 4) &&
	   !strncmp(header + I_AUTHOFF, "GJBU", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"MacBinary 1.0\" packed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    bin(header, in_data_size, 0);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	/* Recognize only if creator is UMcp.  UMCP uses ttxt as default. */
	if(!strncmp(header + I_TYPEOFF, "TEXT", 4) &&
	   !strncmp(header + I_AUTHOFF, "UMcp", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"UMCP\" packed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    bin(header, in_data_size, 1);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* BIN */
#ifdef JDW
	if(!strncmp(header + I_TYPEOFF, "Smal", 4) &&
	   !strncmp(header + I_AUTHOFF, "Jdw ", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"Compress It\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    jdw((unsigned long)in_data_size);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* JDW */
#ifdef STF
	if(!strncmp(header + I_TYPEOFF, "COMP", 4) &&
	   !strncmp(header + I_AUTHOFF, "STF ", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"ShrinkToFit\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    stf((unsigned long)in_data_size);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* STF */
#ifdef LZC
	if(!strncmp(header + I_TYPEOFF, "ZIVM", 4) &&
	   !strncmp(header + I_AUTHOFF, "LZIV", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"MacCompress(M)\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    lzc(header);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "ZIVU", 4) &&
	   !strncmp(header + I_AUTHOFF, "LZIV", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"MacCompress(U)\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    lzc(header);
	    continue;
	}
#endif /* LZC */
#ifdef ASQ
	if(!strncmp(header + I_TYPEOFF, "ArCv", 4) &&
	   !strncmp(header + I_AUTHOFF, "TrAS", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"AutoSqueeze\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    lzh(0);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* ASQ */
#ifdef ARC
	if(!strncmp(header + I_TYPEOFF, "mArc", 4) &&
	   !strncmp(header + I_AUTHOFF, "arc*", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"ArcMac\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    arc();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "arc@", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"ArcMac\" self extracting archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    arc();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* ARC */
#ifdef PIT
	if(!strncmp(header + I_TYPEOFF, "PIT ", 4) &&
	   !strncmp(header + I_AUTHOFF, "PIT ", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"PackIt\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    pit();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* PIT */
#ifdef SIT
	if(!strncmp(header + I_TYPEOFF, "SIT!", 4) &&
	   !strncmp(header + I_AUTHOFF, "SIT!", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"StuffIt\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    sit();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "SITD", 4) &&
	   !strncmp(header + I_AUTHOFF, "SIT!", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"StuffIt Deluxe\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    sit();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "aust", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"StuffIt\" self extracting archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    sit();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* SIT */
#ifdef DIA
	if(!strncmp(header + I_TYPEOFF, "Pack", 4) &&
	   !strncmp(header + I_AUTHOFF, "Pack", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"Diamond\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    dia((unsigned char *)header);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "Pack", 4) &&
	   in_data_size != 0) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"Diamond\" self extracting archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    dia((unsigned char *)header);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* DIA */
#ifdef CPT
	if(!strncmp(header + I_TYPEOFF, "PACT", 4) &&
	   !strncmp(header + I_AUTHOFF, "CPCT", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"Compactor\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    cpt();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "EXTR", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"Compactor\" self extracting archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    cpt();
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* CPT */
#ifdef ZMA
	if(!strncmp(header + I_TYPEOFF, "zooM", 4) &&
	   !strncmp(header + I_AUTHOFF, "zooM", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"Zoom\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    zma((char *)NULL, (unsigned long)in_data_size);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "Mooz", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"Zoom\" self extracting archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    zma((char *)NULL, (unsigned long)in_data_size);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* ZMA */
#ifdef LZH
	if(!strncmp(header + I_TYPEOFF, "LARC", 4) &&
	   !strncmp(header + I_AUTHOFF, "LARC", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"MacLHa (LHARC)\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    lzh(0);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "LHA ", 4) &&
	   !strncmp(header + I_AUTHOFF, "LARC", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"MacLHa (LHA)\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    lzh(1);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* LZH */
#ifdef DD
	if((!strncmp(header + I_TYPEOFF, "DD01", 4) ||
	    !strncmp(header + I_TYPEOFF, "DDF?", 3) ||
	    !strncmp(header + I_TYPEOFF, "DDf?", 3)) &&
	   !strncmp(header + I_AUTHOFF, "DDAP", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr,
			"This is a \"DiskDoubler\" compressed file.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
	    dd_file((unsigned char *)header);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "DDAR", 4) &&
	   !strncmp(header + I_AUTHOFF, "DDAP", 4)) {
	    if(verbose) {
		do_indent(indent);
		(void)fprintf(stderr, "This is a \"DiskDoubler\" archive.\n");
	    }
#ifdef SCAN
	    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
	    dd_arch((unsigned char *)header);
	    skip_file(ds_skip + in_rs);
	    continue;
	}
	if(!strncmp(header + I_TYPEOFF, "APPL", 4) &&
	   !strncmp(header + I_AUTHOFF, "DSEA", 4)) {
	    if(verbose) {
		do_indent(indent);
	    }
	    c = getc(infp);
	    (void)ungetc(c, infp);
	    if(c == 'D') {
		if(verbose) {
		    (void)fprintf(stderr,
			"This is a \"DiskDoubler\" self extracting archive.\n");
#ifdef SCAN
		    do_idf(header + I_NAMEOFF + 1, ARCH_NAME);
#endif /* SCAN */
		}
		dd_arch((unsigned char *)header);
	    } else {
		if(verbose) {
		    (void)fprintf(stderr,
			"This is a \"DiskDoubler\" self decompressing file.\n");
#ifdef SCAN
		    do_idf(header + I_NAMEOFF + 1, PACK_NAME);
#endif /* SCAN */
		}
		dd_file((unsigned char *)header);
	    }
	    skip_file(ds_skip + in_rs);
	    continue;
	}
#endif /* DD */
	if(header[0] == 0 /* MORE CHECKS HERE! */) {
	    mcb(header, (unsigned long)in_rsrc_size,
			(unsigned long)in_data_size, in_ds + in_rs);
	    continue;
	} else {
	    (void)fprintf(stderr, "Unrecognized archive type.\n");
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
#ifdef SCAN
	    do_error("macunpack: Incomplete file");
#endif /* SCAN */
	    exit(1);
	}
	skip -= n;
    }
}

#ifdef SCAN
static void get_idf(kind)
int kind;
{
    char filename[255];

    if(fread(filename, 1, in_data_size, infp) != in_data_size) {
	(void)fprintf(stderr, "Incomplete file.\n");
#ifdef SCAN
	    do_error("macunpack: Incomplete file");
#endif /* SCAN */
	exit(1);
    }
    filename[in_data_size] = 0;
    do_idf(filename, kind);
}
#endif /* SCAN */

