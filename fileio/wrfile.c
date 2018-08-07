#ifdef TYPES_H
#include <sys/types.h>
#endif /* TYPES_H */
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include "machdr.h"
#include "wrfile.h"
#include "wrfileopt.h"
#include "../util/util.h"
#ifdef AUFSPLUS
#include "../util/curtime.h"
#define AUFS
#endif /* AUFSPLUS */
#ifdef AUFS
#include "aufs.h"
#define APPLESHARE
#endif /* AUFS */
#ifdef APPLEDOUBLE
#include "appledouble.h"
#include "../util/curtime.h"
#define APPLESHARE
#endif /* APPLEDOUBLE */

#define TEXT 0
#define DATA 1
#define RSRC 2
#define FULL 3
#define MACB 4
#define FORK 5
#define APSH 6
#define MACS 7
#define UNIX 8
#ifdef SCAN
#define MACI 9
#endif /* SCAN */

extern char *malloc();
extern char *realloc();
extern char *strcpy();
extern char *strncpy();
extern char *strcat();
extern void exit();

#ifdef UNDEF /* Do not declare sprintf; not portable (but lint will complain) */
char *sprintf();
#endif /* UNDEF */

#ifdef AUFS
static void check_aufs();
static void aufs_namings();
static void wr_aufs_info();
#endif /* AUFS */
#ifdef APPLEDOUBLE
static void check_appledouble();
static void appledouble_namings();
static void wr_appledouble_info();
#endif /* APPLEDOUBLE */
#ifdef APPLESHARE
static void mk_share_name();
#endif /* APPLESHARE */

#ifndef BSD
/* all those stupid differences! */
#define bcopy(src,dest,length)	memcpy((dest),(src),(length))
#define bzero(block,length)	memset((block),0,(length))
#endif /* BSD */

#define INFO_FORK	1
#define RSRC_FORK	2
#define DATA_FORK	3

static char f_info[I_NAMELEN];
static char f_data[I_NAMELEN*3];
static char f_rsrc[I_NAMELEN];
static char f_text[I_NAMELEN];
static char f_unix[I_NAMELEN];
static char f_bin[I_NAMELEN];
static char f_folder[] = ".foldername";
static char share_name[256];
#ifdef APPLESHARE
static char hex[] = "0123456789abcdef";
#endif /* APPLESHARE */
#ifdef AUFS
static char infodir[] = ".finderinfo";
static char rsrcdir[] = ".resource";
#define INFOSZ	sizeof(infodir)
#define RSRCSZ	sizeof(rsrcdir)
static char f_info_aufs[I_NAMELEN*3+INFOSZ];
static char f_rsrc_aufs[I_NAMELEN*3+RSRCSZ];
#endif /* AUFS */
#ifdef APPLEDOUBLE
static char infodir[] = ".AppleDouble";
#define INFOSZ	sizeof(infodir)
static char f_info_appledouble[I_NAMELEN*3+INFOSZ];
#endif /* APPLEDOUBLE */

static int mode = MACB;
static int mode_restricted = 0;
static int mode_s_restricted = 0;
char *out_buffer, *out_ptr;

static char init_buffer[128];
static char *buffer = &(init_buffer[0]);
static char *rbuffer = NULL, *dbuffer = NULL;
static char *ptr;
static unsigned long rsz, dsz, totsize, maxsize;

void define_name(text)
char *text;
{
    (void)sprintf(f_info, "%s.info", text);
    (void)sprintf(f_rsrc, "%s.rsrc", text);
    (void)sprintf(f_data, "%s.data", text);
    (void)sprintf(f_text, "%s.text", text);
    (void)sprintf(f_bin, "%s.bin", text);
    (void)sprintf(f_unix, "%s", text);
#ifdef APPLESHARE
/* Do not do namestuffing here.  We want to base again on the information in
   the info header, so this is delayed
*/
#endif /* APPLESHARE */
}

void start_info(info, rsize, dsize)
char *info;
unsigned long rsize, dsize;
{
    int rs, ds;

    rsz = rsize;
    dsz = dsize;
    rs = (((rsz + 127) >> 7) << 7);
    ds = (((dsz + 127) >> 7) << 7);
    totsize = rs + ds + 128;
    if(buffer == &(init_buffer[0])) {
	buffer = (char *)malloc((unsigned)totsize);
    } else if(maxsize < totsize) {
	buffer = (char *)realloc(buffer, (unsigned)totsize);
    }
    maxsize = totsize;
    if(buffer == NULL) {
	(void)fprintf(stderr, "Insufficient memory, aborting\n");
	exit(1);
    }
    dbuffer = buffer + 128;
    rbuffer = dbuffer + ds;
    (void)bzero(buffer, (int)totsize);
    ptr = buffer;
    (void)bcopy(info, ptr, 128);
#ifdef AUFS
/* Now we do filenaming etc. */
    if(mode == APSH) {
	aufs_namings();
    }
#endif /* AUFS */
#ifdef APPLEDOUBLE
/* Now we do filenaming etc. */
    if(mode == APSH) {
	appledouble_namings();
    }
#endif /* APPLEDOUBLE */
}

void start_rsrc()
{
    out_buffer = out_ptr = rbuffer;
}

void start_data()
{
    out_buffer = out_ptr = dbuffer;
}

void end_file()
{
    FILE *fp;
    int i, c;

    buffer[I_FLAGOFF] &= (~INITED_MASK);
    switch(mode) {
    case FULL:
    case FORK:
	fp = fopen(f_info, "w");
	if(fp == NULL) {
	    perror(f_info);
	    exit(1);
	}
	(void)fwrite(buffer, 1, 128, fp);
	(void)fclose(fp);
	if(rsz != 0 || mode == FULL) {
	    fp = fopen(f_rsrc, "w");
	    if(fp == NULL) {
		perror(f_rsrc);
		exit(1);
	    }
	    (void)fwrite(rbuffer, 1, (int)rsz, fp);
	    (void)fclose(fp);
	}
	if(dsz != 0 || mode == FULL) {
	    fp = fopen(f_data, "w");
	    if(fp == NULL) {
		perror(f_data);
		exit(1);
	    }
	    (void)fwrite(dbuffer, 1, (int)dsz, fp);
	    (void)fclose(fp);
	}
	break;
    case RSRC:
	fp = fopen(f_rsrc, "w");
	if(fp == NULL) {
	    perror(f_rsrc);
	    exit(1);
	}
	(void)fwrite(rbuffer, 1, (int)rsz, fp);
	(void)fclose(fp);
	break;
    case DATA:
	fp = fopen(f_data, "w");
	if(fp == NULL) {
	    perror(f_data);
	    exit(1);
	}
	(void)fwrite(dbuffer, 1, (int)dsz, fp);
	(void)fclose(fp);
	break;
    case TEXT:
	fp = fopen(f_text, "w");
	if(fp == NULL) {
	    perror(f_data);
	    exit(1);
	}
	for(i = 0; i < dsz; i++) {
	    c = dbuffer[i];
	    if(c == '\012' || c == '\015') {
		dbuffer[i] = '\027' -c;
	    }
	}
	(void)fwrite(dbuffer, 1, (int)dsz, fp);
	(void)fclose(fp);
	break;
    case UNIX:
	fp = fopen(f_unix, "w");
	if(fp == NULL) {
	    perror(f_data);
	    exit(1);
	}
	for(i = 0; i < dsz; i++) {
	    c = dbuffer[i];
	    if(c == '\012' || c == '\015') {
		dbuffer[i] = '\027' -c;
	    }
	}
	(void)fwrite(dbuffer, 1, (int)dsz, fp);
	(void)fclose(fp);
	break;
    case MACB:
	fp = fopen(f_bin, "w");
	if(fp == NULL) {
	    perror(f_bin);
	    exit(1);
	}
	if(buffer[I_FLAGOFF + 1] & PROTCT_MASK) {
	    buffer[I_LOCKOFF] = 1;
	}
	buffer[I_FLAGOFF + 1] = 0;
	buffer[I_LOCKOFF + 1] = 0;
	(void)fwrite(buffer, 1, (int)totsize, fp);
	(void)fclose(fp);
	break;
    case MACS:
#ifdef SCAN
    case MACI:
#endif /* SCAN */
	if(buffer[I_FLAGOFF + 1] & PROTCT_MASK) {
	    buffer[I_LOCKOFF] = 1;
	}
	buffer[I_FLAGOFF + 1] = 0;
	buffer[I_LOCKOFF + 1] = 0;
	(void)fwrite(buffer, 1, (int)totsize, stdout);
	break;
#ifdef AUFS
    case APSH:
	fp = fopen(f_info_aufs, "w");
	if(fp == NULL) {
	    perror(f_info_aufs);
	    exit(1);
	}
	wr_aufs_info(fp);
	(void) fclose(fp);
	fp = fopen(f_rsrc_aufs, "w");
	if(fp == NULL) {
	    perror(f_rsrc_aufs);
	    exit(1);
	}
	(void)fwrite(rbuffer, 1, (int)rsz, fp);
	(void)fclose(fp);
	fp = fopen(f_data, "w");
	if(fp == NULL) {
	    perror(f_data);
	    exit(1);
	}
	(void)fwrite(dbuffer, 1, (int)dsz, fp);
	(void)fclose(fp);
	break;
#endif /* AUFS */
#ifdef APPLEDOUBLE
    case APSH:
	fp = fopen(f_info_appledouble, "w");
	if(fp == NULL) {
	    perror(f_info_appledouble);
	    exit(1);
	}
	wr_appledouble_info(fp);
	(void)fwrite(rbuffer, 1, (int)rsz, fp);
	(void)fclose(fp);
	fp = fopen(f_data, "w");
	if(fp == NULL) {
	    perror(f_data);
	    exit(1);
	}
	(void)fwrite(dbuffer, 1, (int)dsz, fp);
	(void)fclose(fp);
	break;
#endif /* APPLEDOUBLE */
    }
}

#ifdef SCAN
void do_idf(name, kind)
char *name;
int kind;
{
    int n;

    if(mode != MACI) {
	return;
    }
    n = strlen(name);
    (void)bzero(buffer, INFOBYTES);
    buffer[I_NAMEOFF + 1] = kind;
    put4(buffer + I_DLENOFF, (unsigned long)n);
    (void)fwrite(buffer, 1, INFOBYTES, stdout);
    if(n != 0) {
	(void)fwrite(name, 1, n, stdout);
	n = (((n + 127) >> 7) << 7) - n;
	while(n-- > 0) {
	    (void)fputc(0, stdout);
	}
    }
}
#endif /* SCAN */

void do_mkdir(name, header)
char *name, *header;
{
struct stat sbuf;
FILE *fp;
#ifdef NOMKDIR
char command[21]; /* Systems without mkdir system call but more than 14
		     char file names?  Ridiculous! */
int sysreturn;
#endif /* MKDIR */
#ifdef APPLESHARE
char dirinfo[I_NAMELEN*3+INFOSZ+10];
#endif /* APPLESHARE */

#ifndef SCAN
    if(mode == MACS) {
#else /* SCAN */
    if(mode == MACS || mode == MACI) {
#endif /* SCAN */
        header[I_NAMEOFF] |= 0x80;
	(void)fwrite(header, 1, INFOBYTES, stdout);
	header[I_NAMEOFF] &= 0x7f;
	return;
    }
#ifdef APPLESHARE
    if(mode == APSH) {
	(void)bcopy(header, buffer, INFOBYTES);
	mk_share_name();
    } else {
	(void)strcpy(share_name, name);
    }
#else /* APPLESHARE */
    (void)strcpy(share_name, name);
#endif /* APPLESHARE */
    if(stat(share_name, &sbuf) == -1) {  /* directory doesn't exist */
#ifndef NOMKDIR
	if(mkdir(share_name, 0777) == -1) {
	    (void)fprintf(stderr, "Can't create subdirectory %s\n", share_name);
	    exit(1);
	}
#else /* NOMKDIR */
	sprintf(command, "mkdir %s", share_name);
	if((sysreturn = system(command)) != 0) {
	    (void)fprintf(stderr, "Can't create subdirectory %s\n", share_name);
	    exit(sysreturn);
	}
#endif /* NOMKDIR */
    } else {		/* something exists with this name */
	if((sbuf.st_mode & S_IFMT) != S_IFDIR) {
	    (void)fprintf(stderr, "Directory name %s already in use\n",
		share_name);
	    exit(1);
	}
    }
    (void)chdir(share_name);
#ifdef APPLESHARE
#ifdef AUFS
    if(mode == APSH) {
	if(stat(rsrcdir, &sbuf) == -1) {  /* directory doesn't exist */
	    if(mkdir(rsrcdir, 0777) == -1) {
	 	(void)fprintf(stderr, "Can't create subdirectory %s\n",
			rsrcdir);
	 	exit(1);
	    }
	} else {
	    if((sbuf.st_mode & S_IFMT) != S_IFDIR) {
		(void)fprintf(stderr, "Directory name %s already in use\n",
			rsrcdir);
		exit(1);
	    }
	}
	if(stat(infodir, &sbuf) == -1) {  /* directory doesn't exist */
	    if(mkdir(infodir, 0777) == -1) {
	 	(void)fprintf(stderr, "Can't create subdirectory %s\n",
			infodir);
	 	exit(1);
	    }
	} else {
	    if((sbuf.st_mode & S_IFMT) != S_IFDIR) {
		(void)fprintf(stderr, "Directory name %s already in use\n",
			infodir);
		exit(1);
	    }
	}
	dirinfo[0] = 0;
	(void)strcat(dirinfo, "../");
	(void)strcat(dirinfo, infodir);
	(void)strcat(dirinfo, "/");
	(void)strcat(dirinfo, share_name);
	fp = fopen(dirinfo, "w");
	if(fp == NULL) {
	    perror(dirinfo);
	    exit(1);
	}
	wr_aufs_info(fp);
	(void)fclose(fp);
    } else {
	fp = fopen(f_folder, "w");
	if(fp == NULL) {
	    perror(f_folder);
	    exit(1);
	}
        header[I_NAMEOFF] |= 0x80;
	(void)fwrite(header, 1, INFOBYTES, fp);
	header[I_NAMEOFF] &= 0x7f;
	(void)fclose(fp);
    }
#endif /* AUFS */
#ifdef APPLEDOUBLE
    if(mode == APSH) {
	if(stat(infodir, &sbuf) == -1) {  /* directory doesn't exist */
	    if(mkdir(infodir, 0777) == -1) {
	 	(void)fprintf(stderr, "Can't create subdirectory %s\n",
			infodir);
	 	exit(1);
	    }
	} else {
	    if((sbuf.st_mode & S_IFMT) != S_IFDIR) {
		(void)fprintf(stderr, "Directory name %s already in use\n",
			infodir);
		exit(1);
	    }
	}
	dirinfo[0] = 0;
	(void)strcat(dirinfo, infodir);
	(void)strcat(dirinfo, "/.Parent");
	fp = fopen(dirinfo, "w");
	if(fp == NULL) {
	    perror(dirinfo);
	    exit(1);
	}
	rsz = 0;
	wr_appledouble_info(fp);
	(void)fclose(fp);
    } else {
	fp = fopen(f_folder, "w");
	if(fp == NULL) {
	    perror(f_folder);
	    exit(1);
	}
	header[I_NAMEOFF] |= 0x80;
	(void)fwrite(header, 1, INFOBYTES, fp);
	header[I_NAMEOFF] &= 0x7f;
	(void)fclose(fp);
    }
#endif /* APPLEDOUBLE */
#else /* APPLESHARE */
    fp = fopen(f_folder, "w");
    if(fp == NULL) {
	perror(f_folder);
	exit(1);
    }
    header[I_NAMEOFF] |= 0x80;
    (void)fwrite(header, 1, INFOBYTES, fp);
    header[I_NAMEOFF] &= 0x7f;
    (void)fclose(fp);
#endif /* APPLESHARE */
}

void enddir()
{
char header[INFOBYTES];
int i;

#ifndef SCAN
    if(mode == MACS) {
#else /* SCAN */
    if(mode == MACS || mode == MACI) {
#endif /* SCAN */
	for(i = 0; i < INFOBYTES; i++) {
	    header[i] = 0;
	}
	header[I_NAMEOFF] = 0x80;
	(void)fwrite(header, 1, INFOBYTES, stdout);
    } else {
	(void)chdir("..");
    }
}

#ifdef APPLESHARE
#ifdef AUFS
static void check_aufs()
{
    /* check for .resource/ and .finderinfo/ */
    struct stat stbuf;
    int error = 0;

    if(stat(rsrcdir,&stbuf) < 0) {
	error ++;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		  error ++;
	}
    }
    if(stat(infodir,&stbuf) < 0) {
	error ++;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		  error++;
	}
    }
    if(error) {
	(void)fprintf(stderr, "Not in an Aufs folder.\n");
	exit(1);
    }
}

static void aufs_namings()
{
    mk_share_name();
    (void)sprintf(f_info_aufs, "%s/%s", infodir, share_name);
    (void)sprintf(f_rsrc_aufs, "%s/%s", rsrcdir, share_name);
    (void)sprintf(f_data, "%s", share_name);
}

static void wr_aufs_info(fp)
FILE *fp;
{
    FileInfo theinfo;
    int n;

    bzero((char *) &theinfo, sizeof theinfo);
    theinfo.fi_magic1 = FI_MAGIC1;
    theinfo.fi_version = FI_VERSION;
    theinfo.fi_magic = FI_MAGIC;
    theinfo.fi_bitmap = FI_BM_MACINTOSHFILENAME;

    /* AUFS stores Unix times. */
#ifdef AUFSPLUS
    theinfo.fi_datemagic = FI_MAGIC;
    theinfo.fi_datevalid = FI_CDATE | FI_MDATE;
    put4(theinfo.fi_ctime, get4(buffer + I_CTIMOFF) - TIMEDIFF);
    put4(theinfo.fi_mtime, get4(buffer + I_MTIMOFF) - TIMEDIFF);
    put4(theinfo.fi_utime, (unsigned long)time((time_t *)0));
#endif /* AUFSPLUS */
    bcopy(buffer + I_TYPEOFF, theinfo.fi_fndr, 4);
    bcopy(buffer + I_AUTHOFF, theinfo.fi_fndr + 4, 4);
    bcopy(buffer + I_FLAGOFF, theinfo.fi_fndr + 8, 2);
    if((n = buffer[I_NAMEOFF] & 0xff) > F_NAMELEN) {
	n = F_NAMELEN;
    }
    (void)strncpy((char *)theinfo.fi_macfilename, buffer + I_NAMEOFF + 1,n);
    /* theinfo.fi_macfilename[n] = '\0'; */
    (void)strcpy((char *)theinfo.fi_comnt,
	"Converted by Unix utility to Aufs format");
    theinfo.fi_comln = strlen((char *)theinfo.fi_comnt);
    (void)fwrite((char *) &theinfo, 1, sizeof theinfo, fp);
}
#endif /* AUFS */

#ifdef APPLEDOUBLE
static void check_appledouble()
{
    /* check for .AppleDouble/ */
    struct stat stbuf;
    int error = 0;

    if(stat(infodir,&stbuf) < 0) {
	error ++;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
		  error++;
	}
    }
    if(error) {
	(void)fprintf(stderr, "Not in an AppleDouble folder.\n");
	exit(1);
    }
}

static void appledouble_namings()
{
    mk_share_name();
    (void)sprintf(f_info_appledouble, "%s/%s", infodir, share_name);
    (void)sprintf(f_data, "%s", share_name);
}

static void wr_appledouble_info(fp)
FILE *fp;
{
    FileInfo theinfo;
    int n;

    bzero((char *) &theinfo, sizeof theinfo);
    put4(theinfo.fi_magic, (unsigned long)FI_MAGIC);
    put2(theinfo.fi_version, (unsigned long)FI_VERSION);
    put4(theinfo.fi_fill5, (unsigned long)FI_FILL5);
    put4(theinfo.fi_fill6, (unsigned long)FI_FILL6);
    put4(theinfo.fi_hlen, (unsigned long)FI_HLEN);
    put4(theinfo.fi_fill7, (unsigned long)FI_FILL7);
    put4(theinfo.fi_namptr, (unsigned long)FI_NAMPTR);
    put4(theinfo.fi_fill9, (unsigned long)FI_FILL9);
    put4(theinfo.fi_commptr, (unsigned long)FI_COMMPTR);
    put4(theinfo.fi_fill12, (unsigned long)FI_FILL12);
    put4(theinfo.fi_timeptr, (unsigned long)FI_TIMEPTR);
    put4(theinfo.fi_timesize, (unsigned long)FI_TIMESIZE);
    put4(theinfo.fi_fill15, (unsigned long)FI_FILL15);
    put4(theinfo.fi_infoptr, (unsigned long)FI_INFOPTR);
    put4(theinfo.fi_infosize, (unsigned long)FI_INFOSIZE);

    bcopy(buffer + I_TYPEOFF, theinfo.fi_type, 4);
    bcopy(buffer + I_AUTHOFF, theinfo.fi_auth, 4);
    bcopy(buffer + I_FLAGOFF, theinfo.fi_finfo, 2);
    /* AppleDouble stores Unix times. */
    put4(theinfo.fi_ctime, get4(buffer + I_CTIMOFF) - TIMEDIFF);
    put4(theinfo.fi_mtime, get4(buffer + I_MTIMOFF) - TIMEDIFF);
    if((n = buffer[I_NAMEOFF] & 0xff) > F_NAMELEN) {
	n = F_NAMELEN;
    }
    put4(theinfo.fi_namlen, (unsigned long)n);
    (void)strncpy((char *)theinfo.fi_name, buffer + I_NAMEOFF + 1,n);
    /* theinfo.fi_macfilename[n] = '\0'; */
    (void)strcpy((char *)theinfo.fi_comment,
	"Converted by Unix utility to AppleDouble format");
    put4(theinfo.fi_commsize, (unsigned long)strlen(theinfo.fi_comment));
    put4(theinfo.fi_rsrc, (unsigned long)rsz);
    /*  Still TODO */
    /*  char	fi_ctime[4];	/* Creation time (Unix time) */
    /*  char	fi_mtime[4];	/* Modification time (Unix time) */
    (void)fwrite((char *) &theinfo, 1, sizeof theinfo, fp);
}
#endif /* APPLEDOUBLE */

static void mk_share_name()
{
    int ch;
    char *mp, *up;

    mp = buffer + 2;
    up = &(share_name[0]);
    while(ch = *mp++) {
	if(isascii(ch) && ! iscntrl(ch) && isprint(ch) && ch != '/') {
	    *up++ = ch;
	} else {
	    *up++ = ':';
	    *up++ = hex[(ch >> 4) & 0xf];
	    *up++ = hex[ch & 0xf];
	}
    }
    *up = 0;
}
#endif /* APPLESHARE */

int wrfileopt(c)
char c;
{
    switch(c) {
    case 'b':
	mode = MACB;
	break;
    case 'r':
	if(mode_restricted) {
	    return 0;
	}
	mode = RSRC;
	break;
    case 'd':
	if(mode_restricted) {
	    return 0;
	}
	mode = DATA;
	break;
    case 'u':
	if(mode_restricted) {
	    return 0;
	}
	mode = TEXT;
	break;
    case 'U':
	if(mode_restricted) {
	    return 0;
	}
	mode = UNIX;
	break;
    case 'f':
	mode = FORK;
	break;
    case '3':
	mode = FULL;
	break;
    case 's':
	if(mode_s_restricted) {
	    return 0;
	}
	mode = MACS;
	break;
#ifdef SCAN
    case 'S':
	if(mode_s_restricted) {
	    return 0;
	}
	mode = MACI;
	break;
#endif /* SCAN */
    case 'a':
#ifdef APPLESHARE
#ifdef AUFS
	check_aufs();
	mode = APSH;
	break;
#endif /* AUFS */
#ifdef APPLEDOUBLE
	check_appledouble();
	mode = APSH;
	break;
#endif /* APPLEDOUBLE */
#else /* APPLESHARE */
	(void)fprintf(stderr, "Sorry, Apple-Unix sharing is not supported.\n");
	(void)fprintf(stderr, "Recompile or omit -a option.\n");
	exit(1);
#endif /* APPLESHARE */
    default:
	return 0;
    }
    return 1;
}

void give_wrfileopt()
{
    (void)fprintf(stderr, "File output options:\n");
    (void)fprintf(stderr, "-b:\tMacBinary (default)\n");
    if(!mode_s_restricted) {
	(void)fprintf(stderr, "-s:\tMacBinary stream to standard output\n");
#ifdef SCAN
	(void)fprintf(stderr,
	    "-S:\tas -s but with indication of orignal Unix filename\n");
#endif /* SCAN */
    }
    (void)fprintf(stderr, "-f:\tthree fork mode, skipping empty forks\n");
    (void)fprintf(stderr, "-3:\tthe same, writing also empty forks\n");
    if(!mode_restricted) {
	(void)fprintf(stderr, "-r:\tresource forks only\n");
	(void)fprintf(stderr, "-d:\tdata forks only\n");
	(void)fprintf(stderr,
	    "-u:\tdata forks only with Mac -> Unix text file translation\n");
	(void)fprintf(stderr,
	    "-U:\tas -u, but filename will not have an extension\n");
    }
#ifdef APPLESHARE
#ifdef AUFS
    (void)fprintf(stderr, "-a:\tAUFS format\n");
#endif /* AUFS */
#ifdef APPLEDOUBLE
    (void)fprintf(stderr, "-a:\tAppleDouble format\n");
#endif /* APPLEDOUBLE */
#else /* APPLESHARE */
    (void)fprintf(stderr, "-a:\tnot supported, needs recompilation\n");
#endif /* APPLESHARE */
}

void set_wrfileopt(restricted)
{
    mode_restricted = restricted;
}

void set_s_wrfileopt(restricted)
{
    mode_s_restricted = restricted;
}

char *get_wrfileopt()
{
    static char options[20];

    (void)strcpy(options, "b");
    if(!mode_s_restricted) {
	(void)strcat(options, "s");
#ifdef SCAN
	(void)strcat(options, "S");
#endif /* SCAN */
    }
    (void)strcat(options, "f3");
    if(!mode_restricted) {
	(void)strcat(options, "rduU");
    }
    (void)strcat(options, "a");
    return options;
}

char *get_mina()
{
#ifdef APPLESHARE
#ifdef AUFS
    return ", AUFS supported";
#endif /* AUFS */
#ifdef APPLEDOUBLE
    return ", AppleDouble supported";
#endif /* APPLEDOUBLE */
#else /* APPLESHARE */
    return ", no Apple-Unix sharing supported";
#endif /* APPLESHARE */
}

