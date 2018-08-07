#include <stdio.h>
#ifdef TYPES_H
#include <sys/types.h>
#endif /* TYPES_H */
#include <sys/stat.h>
#include "machdr.h"
#include "rdfile.h"
#include "rdfileopt.h"
#ifndef DIRENT_H
#include <sys/dir.h>
#define dirstruct direct
#else /* DIRENT_H */
#include <dirent.h>
#define dirstruct dirent
#endif /* DIRENT_H */
#include "../util/curtime.h"
#include "../util/masks.h"
#include "../util/util.h"

#ifdef AUFSPLUS
#define AUFS
#endif /* AUFSPLUS */
#ifdef AUFS
#define APPLESHARE
#endif /* AUFS */
#ifdef APPLEDOUBLE
#define APPLESHARE
#endif /* APPLEDOUBLE */

#define NOTFOUND	0
#define ISFILE		1
#define	INFOFILE	2
#define INFOEXT		3
#define	SKIPFILE	4
#define	MACBINARY	5
#define	DIRECTORY	6
#ifdef APPLESHARE
#define	SHAREFILE	7
#endif /* APPLESHARE */

#define DATA_FORMAT	1
#define RSRC_FORMAT	2
#define UNIX_FORMAT	3

extern char *malloc();
extern char *realloc();
extern char *strcpy();
extern char *strncpy();
extern char *strcat();
extern void exit();

static void check_files();
static void read_file();
static void enter_dir();
static void exit_dir();
static int get_stdin_file();

char file_info[INFOBYTES];
char *data_fork, *rsrc_fork;
int data_size, rsrc_size;
static int max_data_size, max_rsrc_size;

typedef struct filelist {
    int nfiles;
    char **files;
    int *kind;
    struct filelist *previous;
    int current;
#ifdef APPLESHARE
    int shared_dir;
#endif /* APPLESHARE */
} filelist;

static int data_only;
static int no_recurse;
static int read_stdin;
static filelist global_files;
static filelist *current_files;
static char f_auth[5];
static char f_type[5];
static char f_name[] = ".foldername";
#ifdef APPLESHARE
#ifdef AUFS
#include "aufs.h"
static char infodir[] = ".finderinfo";
static char rsrcdir[] = ".resource";
static void read_aufs_info();
#endif /* AUFS */
#ifdef APPLEDOUBLE
#include "appledouble.h"
static char infodir[] = ".AppleDouble";
static void read_appledouble_info();
#endif /* APPLEDOUBLE */
#endif /* APPLESHARE */
static char filename[255];
static int filekind;

void setup(argc, argv)
int argc;
char **argv;
{
    if(argc == 0) {
	read_stdin = 1;
    } else {
	read_stdin = 0;
	global_files.previous = NULL;
	global_files.nfiles = argc;
	global_files.files = argv;
	global_files.current = 0;
	current_files = &global_files;
	check_files(1);
    }
}

static void check_files(initial)
int initial;
{
    struct stat stbuf;
    int i, j, n;
    char filename[255], filename1[255];

    /* Check the method to read the file */
    current_files->current = 0;
    /* Set initially to NOTFOUND or DIRECTORY */
    n = current_files->nfiles;
    current_files->kind = (int *)malloc((unsigned)n * sizeof(int));
    if(current_files->kind == NULL) {
	(void)fprintf(stderr, "Insufficient memory\n");
	exit(1);
    }
    for(i = 0; i < n; i++) {
	current_files->kind[i] = NOTFOUND;
	if(stat(current_files->files[i], &stbuf) >= 0) {
	    if((stbuf.st_mode & S_IFMT) == S_IFDIR) {
		/* We found a directory */
		current_files->kind[i] = DIRECTORY;
		continue;
	    }
	    current_files->kind[i] = ISFILE;
	}
    }
    /* That is enough for data_only mode */
    if(data_only) {
	return;
    }
#ifdef APPLESHARE
    /* First check whether we are in a folder on a shared volume */
    i = 1;
#ifdef AUFS
    if(stat(rsrcdir,&stbuf) < 0) {
	i = 0;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
	    i = 0;
	}
    }
    if(stat(infodir,&stbuf) < 0) {
	i = 0;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
	    i = 0;
	}
    }
#endif /* AUFS */
#ifdef APPLEDOUBLE
    if(stat(infodir,&stbuf) < 0) {
	i = 0;
    } else {
	if((stbuf.st_mode & S_IFMT) != S_IFDIR) {
	    i = 0;
	}
    }
#endif /* APPLEDOUBLE */
    current_files->shared_dir = i;
#endif /* APPLESHARE */
    for(i = 0; i < n; i++) {
	if(current_files->kind[i] == NOTFOUND) {
	    j = 0;
	} else if(current_files->kind[i] == ISFILE) {
	    /* Check whether the file is special */
#ifdef APPLESHARE
	    if(!current_files->shared_dir &&
	       !strcmp(current_files->files[i], f_name)) {
		current_files->kind[i] = SKIPFILE;
		continue;
	    }
#else /* APPLESHARE */
	    if(!strcmp(current_files->files[i], f_name)) {
		current_files->kind[i] = SKIPFILE;
		continue;
	    }
#endif /* APPLESHARE */
	    j = 1;
	} else if(current_files->kind[i] == SKIPFILE) {
	    continue;
	} else if(!initial) { /* DIRECTORY */
	    /* Check whether the directory is special */
	    if(!strcmp(current_files->files[i], ".") ||
	       !strcmp(current_files->files[i], "..")) {
		current_files->kind[i] = SKIPFILE;
	    }
#ifdef APPLESHARE
#ifdef AUFS
	    if(current_files->shared_dir &&
	       (!strcmp(current_files->files[i], infodir) ||
		!strcmp(current_files->files[i], rsrcdir))) {
		current_files->kind[i] = SKIPFILE;
	    }
#endif /* AUFS */
#ifdef APPLEDOUBLE
	    if(current_files->shared_dir &&
	       !strcmp(current_files->files[i], infodir)) {
		current_files->kind[i] = SKIPFILE;
	    }
#endif /* APPLEDOUBLE */
#endif /* APPLESHARE */
	    continue;
	} else { /* Take all directories from command line! */
	    continue;
	}
#ifdef APPLESHARE
	/* Check whether file is in shared format */
	if(j & current_files->shared_dir) {
	    j = 0;
	    filename[0] = 0;
	    (void)strcat(filename, infodir);
	    (void)strcat(filename, "/");
	    (void)strcat(filename, current_files->files[i]);
	    /* There ought to be an associated file in the info direcory */
	    if(stat(filename, &stbuf) >= 0) {
		current_files->kind[i] = SHAREFILE;
		continue;
	    }
	}
#endif /* APPLESHARE */
	/* If file not found check for the same with .info extension */
	if(!j) {
	    filename[0] = 0;
	    (void)strcat(filename, current_files->files[i]);
	    (void)strcat(filename, ".info");
	    /* Found a .info file, else no such file found */
	    if(stat(filename, &stbuf) >= 0) {
		current_files->kind[i] = INFOEXT;
	    }
	    continue;
	}
	/* Now we found the file.  Check first whether the name ends with
	   .info */
	j = strlen(current_files->files[i]) - 5;
	if(!strncmp(current_files->files[i] + j, ".info", 5)) {
	    /* This is a .info file.  Set as INFOFILE */
	    current_files->kind[i] = INFOFILE;
	    /* Now remove from list of files the same with .data or .rsrc
	       extension */
	    filename[0] = 0;
	    (void)strcat(filename, current_files->files[i]);
	    filename[j] = 0;
	    (void)strcpy(filename1, filename);
	    (void)strcat(filename, ".data");
	    (void)strcat(filename1, ".rsrc");
	    for(j = i + 1; j < n; j++) {
		if(!strcmp(filename, current_files->files[j])) {
		    /* Associated .data file */
		    current_files->kind[j] = SKIPFILE;
		    continue;
		}
		if(!strcmp(filename1, current_files->files[j])) {
		    /* Associated .rsrc file */
		    current_files->kind[j] = SKIPFILE;
		}
	    }
	    continue;
	}
	if(!strncmp(current_files->files[i] + j, ".data", 5) ||
	   !strncmp(current_files->files[i] + j, ".rsrc", 5)) {
	    /* .data or .rsrc file found.  Check whether there is an
	       associated .info file in the filelist */
	    filename[0] = 0;
	    (void)strcat(filename, current_files->files[i]);
	    filename[j] = 0;
	    (void)strcat(filename, ".info");
	    for(j = i + 1; j < n; j++) {
		if(!strcmp(filename, current_files->files[j])) {
		    /* Found an associated .info file! */
		    current_files->kind[i] = SKIPFILE;
		    break;
		}
	    }
	    if(j < n) {
		continue;
	    }
	}
	/* Finally nothing special */
	current_files->kind[i] = MACBINARY;
    }
}

int nextfile()
{
    int i;

    if(read_stdin) {
	return get_stdin_file();
    }
    i = current_files->current;
again:
    if(i == current_files->nfiles) {
	if(current_files->previous == NULL) {
	    return ISATEND;
	} else {
	    exit_dir();
	    current_files->current++;
	    return ENDDIR;
	}
    }
    filename[0] = 0;
    (void)strcat(filename, current_files->files[i]);
    filekind = current_files->kind[i];
    switch(filekind) {
    case DIRECTORY:
	if(no_recurse) {
	    (void)fprintf(stderr, "Directory %s skipped.\n", filename);
	    i++;
	    current_files->current = i;
	    goto again;
	}
	enter_dir();
	return ISDIR;
    case SKIPFILE:
	i++;
	current_files->current = i;
	goto again;
    case NOTFOUND:
	(void)fprintf(stderr, "File %s not found.\n", filename);
	exit(1);
    default:
	read_file();
	current_files->current = i + 1;
	return ISFILE;
    }
}

static void read_file()
{
    FILE *fd;
    int c, j, lname, skip;
    struct stat stbuf;
#ifdef APPLESHARE
    char filename1[255];
#endif /* APPLESHARE */

    switch(filekind) {
    case ISFILE:
	if(stat(filename, &stbuf) < 0) {
	    (void)fprintf(stderr, "Cannot stat file %s\n", filename);
	    exit(1);
	}
	for(j = 0; j < INFOBYTES; j++) {
	    file_info[j] = 0;
	}
	(void)strcpy(file_info + I_NAMEOFF + 1, filename);
	file_info[I_NAMEOFF] = strlen(filename);
	put4(file_info + I_CTIMOFF, (unsigned long)stbuf.st_ctime + TIMEDIFF);
	put4(file_info + I_MTIMOFF, (unsigned long)stbuf.st_mtime + TIMEDIFF);
	if(data_only == RSRC_FORMAT) {
	    rsrc_size = stbuf.st_size;
	    data_size = 0;
	    if(rsrc_size > max_rsrc_size) {
		if(rsrc_fork == NULL) {
		    rsrc_fork = malloc((unsigned)rsrc_size);
		} else {
		    rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
		}
		max_rsrc_size = rsrc_size;
	    }
	    if(f_type[0] == 0) {
		(void)strncpy(file_info + I_TYPEOFF, "RSRC", 4);
	    } else {
		(void)strncpy(file_info + I_TYPEOFF, f_type, 4);
	    }
	    if(f_auth[0] == 0) {
		(void)strncpy(file_info + I_AUTHOFF, "RSED", 4);
	    } else {
		(void)strncpy(file_info + I_AUTHOFF, f_auth, 4);
	    }
	    put4(file_info + I_RLENOFF, (unsigned long)rsrc_size);
	    if((fd = fopen(filename, "r")) == NULL) {
		(void)fprintf(stderr, "Cannot open file %s\n", filename);
		exit(1);
	    }
	    if(fread(rsrc_fork, 1, rsrc_size, fd) != rsrc_size) {
		(void)fprintf(stderr, "Short file %s\n", filename);
		exit(1);
	    }
	    (void)fclose(fd);
	} else {
	    data_size = stbuf.st_size;
	    rsrc_size = 0;
	    if(data_size > max_data_size) {
		if(data_fork == NULL) {
		    data_fork = malloc((unsigned)data_size);
		} else {
		    data_fork = realloc(data_fork, (unsigned)data_size);
		}
		max_data_size = data_size;
	    }
	    if(f_type[0] == 0) {
		(void)strncpy(file_info + I_TYPEOFF, "TEXT", 4);
	    } else {
		(void)strncpy(file_info + I_TYPEOFF, f_type, 4);
	    }
	    if(f_auth[0] == 0) {
		(void)strncpy(file_info + I_AUTHOFF, "MACA", 4);
	    } else {
		(void)strncpy(file_info + I_AUTHOFF, f_auth, 4);
	    }
	    put4(file_info + I_DLENOFF, (unsigned long)data_size);
	    if((fd = fopen(filename, "r")) == NULL) {
		(void)fprintf(stderr, "Cannot open file %s\n", filename);
		exit(1);
	    }
	    if(fread(data_fork, 1, data_size, fd) != data_size) {
		(void)fprintf(stderr, "Short file %s\n", filename);
		exit(1);
	    }
	    (void)fclose(fd);
	    if(data_only == UNIX_FORMAT) {
		for(j = 0; j < data_size; j++) {
		    c = data_fork[j];
		    if(c == '\012' || c == '\015') {
			data_fork[j] = '\027' -c;
		    }
		}
	    }
	}
	break;
    case INFOEXT:
	(void)strcat(filename, ".info");
    case INFOFILE:
	lname = strlen(filename) - 5;
	if((fd = fopen(filename, "r")) == NULL) {
	    (void)fprintf(stderr, "Cannot open file %s\n", filename);
	    exit(1);
	}
	if(fread(file_info, 1, INFOBYTES, fd) != INFOBYTES) {
	    (void)fprintf(stderr, "Cannot read info header %s\n", filename);
	}
	(void)fclose(fd);
	data_size = get4(file_info + I_DLENOFF);
	rsrc_size = get4(file_info + I_RLENOFF);
	if(data_size > max_data_size) {
	    if(data_fork == NULL) {
		data_fork = malloc((unsigned)data_size);
	    } else {
		data_fork = realloc(data_fork, (unsigned)data_size);
	    }
	    max_data_size = data_size;
	}
	if(rsrc_size > max_rsrc_size) {
	    if(rsrc_fork == NULL) {
		rsrc_fork = malloc((unsigned)rsrc_size);
	    } else {
		rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
	    }
	    max_rsrc_size = rsrc_size;
	}
	if(data_size != 0) {
	    filename[lname] = 0;
	    (void)strcat(filename, ".data");
	    if((fd = fopen(filename, "r")) == NULL) {
		(void)fprintf(stderr, "Cannot open data fork %s\n", filename);
		exit(1);
	    }
	    if(fread(data_fork, 1, data_size, fd) != data_size) {
		(void)fprintf(stderr, "Premature EOF on %s\n", filename);
	    }
	    (void)fclose(fd);
	}
	if(rsrc_size != 0) {
	    filename[lname] = 0;
	    (void)strcat(filename, ".rsrc");
	    if((fd = fopen(filename, "r")) == NULL) {
		(void)fprintf(stderr, "Cannot open rsrc fork %s\n", filename);
		exit(1);
	    }
	    if(fread(rsrc_fork, 1, rsrc_size, fd) != rsrc_size) {
		(void)fprintf(stderr, "Premature EOF on %s\n", filename);
	    }
	    (void)fclose(fd);
	}
	break;
    case MACBINARY:
	if((fd = fopen(filename, "r")) == NULL) {
	    (void)fprintf(stderr, "Cannot open file %s\n", filename);
	    exit(1);
	}
	if(fread(file_info, 1, INFOBYTES, fd) != INFOBYTES) {
	    (void)fprintf(stderr, "Short file %s\n", filename);
	    exit(1);
	}
	if(file_info[0] != 0) {
	    (void)fprintf(stderr, "File is not MacBinary: %s\n", filename);
	    exit(1);
	}
	data_size = get4(file_info + I_DLENOFF);
	rsrc_size = get4(file_info + I_RLENOFF);
	if(file_info[I_LOCKOFF] & 1) {
	    file_info[I_FLAGOFF + 1] = PROTCT_MASK;
	    file_info[I_LOCKOFF] &= ~1;
	}
	if(data_size != 0) {
	    if(data_size > max_data_size) {
		if(data_fork == NULL) {
		    data_fork = malloc((unsigned)data_size);
		} else {
		    data_fork = realloc(data_fork, (unsigned)data_size);
		}
		max_data_size = data_size;
	    }
	    if(fread(data_fork, 1, data_size, fd) != data_size) {
		(void)fprintf(stderr, "Short file %s\n", filename);
		exit(1);
	    }
	    skip = (((data_size + 127) >> 7) << 7) - data_size;
	    for(j = 0; j < skip; j++) {
		(void)fgetc(fd);
	    }
	}
	if(rsrc_size != 0) {
	    if(rsrc_size > max_rsrc_size) {
		if(rsrc_fork == NULL) {
		    rsrc_fork = malloc((unsigned)rsrc_size);
		} else {
		    rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
		}
		max_rsrc_size = rsrc_size;
	    }
	    if(fread(rsrc_fork, 1, rsrc_size, fd) != rsrc_size) {
		(void)fprintf(stderr, "Short file %s\n", filename);
		exit(1);
	    }
	}
	break;
#ifdef APPLESHARE
    case SHAREFILE:
#ifdef AUFS
	(void)strcpy(filename1, infodir);
	(void)strcat(filename1, "/");
	(void)strcat(filename1, filename);
	if((fd = fopen(filename1, "r")) == NULL) {
	    (void)fprintf(stderr, "Cannot open file %s\n", filename1);
	}
	read_aufs_info(fd);
	(void)fclose(fd);
	(void)strcpy(filename1, rsrcdir);
	(void)strcat(filename1, "/");
	(void)strcat(filename1, filename);
	if(stat(filename1, &stbuf) >= 0) {
	    rsrc_size = stbuf.st_size;
	    put4(file_info + I_RLENOFF, (unsigned long)rsrc_size);
	    if(rsrc_size > 0) {
		if(rsrc_size > max_rsrc_size) {
		    if(rsrc_fork == NULL) {
			rsrc_fork = malloc((unsigned)rsrc_size);
		    } else {
			rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
		    }
		    max_rsrc_size = rsrc_size;
		}
		if((fd = fopen(filename1, "r")) == NULL) {
		    (void)fprintf(stderr, "Cannot open file %s\n", filename1);
		    exit(1);
		}
		if(fread(rsrc_fork, 1, rsrc_size, fd) != rsrc_size) {
		    (void)fprintf(stderr, "Short file %s\n", filename1);
		    exit(1);
		}
		(void)fclose(fd);
	    }
	}
	if(stat(filename, &stbuf) >= 0) {
	    data_size = stbuf.st_size;
	    put4(file_info + I_DLENOFF, (unsigned long)data_size);
	    if(data_size > 0) {
		if(data_size > max_data_size) {
		    if(data_fork == NULL) {
			data_fork = malloc((unsigned)data_size);
		    } else {
			data_fork = realloc(data_fork, (unsigned)data_size);
		    }
		    max_data_size = data_size;
		}
		if((fd = fopen(filename, "r")) == NULL) {
		    (void)fprintf(stderr, "Cannot open file %s\n", filename);
		    exit(1);
		}
		if(fread(data_fork, 1, data_size, fd) != data_size) {
		    (void)fprintf(stderr, "Short file %s\n", filename1);
		    exit(1);
		}
		(void)fclose(fd);
	    }
	}
#endif /* AUFS */
#ifdef APPLEDOUBLE
	(void)strcpy(filename1, infodir);
	(void)strcat(filename1, "/");
	(void)strcat(filename1, filename);
	if((fd = fopen(filename1, "r")) == NULL) {
	    (void)fprintf(stderr, "Cannot open file %s\n", filename1);
	}
	read_appledouble_info(fd);
	rsrc_size = get4(file_info + I_RLENOFF);
	if(rsrc_size > 0) {
	    if(rsrc_size > max_rsrc_size) {
		if(rsrc_fork == NULL) {
		    rsrc_fork = malloc((unsigned)rsrc_size);
		} else {
		    rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
		}
		max_rsrc_size = rsrc_size;
	    }
	    if(fread(rsrc_fork, 1, rsrc_size, fd) != rsrc_size) {
		(void)fprintf(stderr, "Short file %s\n", filename1);
		exit(1);
	    }
	}
	(void)fclose(fd);
	if(stat(filename, &stbuf) >= 0) {
	    data_size = stbuf.st_size;
	    put4(file_info + I_DLENOFF, (unsigned long)data_size);
	    if(data_size > 0) {
		if(data_size > max_data_size) {
		    if(data_fork == NULL) {
			data_fork = malloc((unsigned)data_size);
		    } else {
			data_fork = realloc(data_fork, (unsigned)data_size);
		    }
		    max_data_size = data_size;
		}
		if((fd = fopen(filename, "r")) == NULL) {
		    (void)fprintf(stderr, "Cannot open file %s\n", filename);
		    exit(1);
		}
		if(fread(data_fork, 1, data_size, fd) != data_size) {
		    (void)fprintf(stderr, "Short file %s\n", filename1);
		    exit(1);
		}
		(void)fclose(fd);
	    }
	}
#endif /* APPLEDOUBLE */
	break;
#endif /* APPLESHARE */
    }
}

static void enter_dir()
{
    DIR *directory;
    struct dirstruct *curentry;
    FILE *fd;
    int n, j, namlen;
    int listsize, cursize;
    char *filetable;
    filelist *new_files;
#ifdef APPLESHARE
    char filename1[255];
#endif /* APPLESHARE */

    for(j = 0; j < INFOBYTES; j++) {
	file_info[j] = 0;
    }
    (void)strcpy(file_info + I_NAMEOFF + 1, filename);
    file_info[I_NAMEOFF] = strlen(filename);
    directory = opendir(filename);
    if(directory == NULL) {
	(void)fprintf(stderr, "Cannot read directory %s\n", filename);
	exit(1);
    }
    listsize = 1024;
    filetable = malloc((unsigned)listsize);
    cursize = 0;
    n = 0;
    while((curentry = readdir(directory)) != NULL) {
	namlen = strlen(curentry->d_name);
	if(namlen + 1 > listsize - cursize) {
	    listsize += 1024;
	    filetable = realloc(filetable, (unsigned)listsize);
	}
	(void)strcpy(filetable + cursize, curentry->d_name);
	cursize += (namlen + 1);
	n++;
    }
    filetable = realloc(filetable, (unsigned)cursize);
    (void)closedir(directory);
    new_files = (filelist *)malloc(sizeof(filelist));
    new_files->nfiles = n;
    new_files->files = (char **)malloc((unsigned)n * sizeof(char **));
    new_files->kind = (int *)malloc((unsigned)n * sizeof(int));
    new_files->previous = current_files;
    new_files->current = 0;
    cursize = 0;
    for(j = 0; j < n; j++) {
	new_files->files[j] = filetable + cursize;
	cursize += (strlen(filetable + cursize) + 1);
    }
    (void)chdir(filename);
#ifdef APPLESHARE
    if((fd = fopen(f_name, "r")) != NULL) {
	if(fread(file_info, 1, INFOBYTES, fd) != INFOBYTES) {
	    (void)fprintf(stderr, "File error on %s\n", f_name);
	    exit(1);
	}
	file_info[I_NAMEOFF] |= 0x80;
	(void)fclose(fd);
    } else {
#ifdef AUFS
	(void)strcpy(filename1, "../");
	(void)strcat(filename1, infodir);
	(void)strcat(filename1, "/");
	(void)strcat(filename1, filename);
	if((fd = fopen(filename1, "r")) != NULL) {
	    read_aufs_info(fd);
	    (void)fclose(fd);
	}
#endif /* AUFS */
#ifdef APPLEDOUBLE
	(void)strcpy(filename1, infodir);
	(void)strcat(filename1, "/.Parent");
	if((fd = fopen(filename1, "r")) != NULL) {
	    read_appledouble_info(fd);
	    (void)fclose(fd);
	}
#endif /* APPLEDOUBLE */
	file_info[I_NAMEOFF] |= 0x80;
    }
#else /* APPLESHARE */
    if((fd = fopen(f_name, "r")) != NULL) {
	if(fread(file_info, 1, INFOBYTES, fd) != INFOBYTES) {
	    (void)fprintf(stderr, "File error on %s\n", f_name);
	    exit(1);
	}
	file_info[I_NAMEOFF] |= 0x80;
	(void)fclose(fd);
    }
#endif /* APPLESHARE */
    current_files = new_files;
    check_files(0);
}

static void exit_dir()
{
    filelist *old_files;
    int i;

    for(i = 0; i < INFOBYTES; i++) {
	file_info[i] = 0;
    }
    file_info[I_NAMEOFF] = 0x80;
    old_files = current_files;
    /* Do some garbage collection here! */
    current_files = current_files->previous;
    (void)free(old_files->files[0]);
    (void)free((char *)old_files->files);
    (void)free((char *)old_files->kind);
    (void)free((char *)old_files);
    (void)chdir("..");
}

#ifdef APPLESHARE
#ifdef AUFS
static void read_aufs_info(fd)
FILE *fd;
{
    FileInfo theinfo;
    int i, n;
    struct stat stbuf;

    for(i = 0; i < INFOBYTES; i++) {
	file_info[i] = 0;
    }
    bzero((char *) &theinfo, sizeof(theinfo));
    if(fread((char *)&theinfo, 1, sizeof(theinfo), fd) != sizeof(theinfo)) {
	(void)fprintf(stderr, "Short AUFS info header for %s\n", filename);
	exit(1);
    }
    if(theinfo.fi_magic1 & BYTEMASK != FI_MAGIC1 ||
       theinfo.fi_version & BYTEMASK != FI_VERSION ||
       theinfo.fi_magic & BYTEMASK != FI_MAGIC) {
	(void)fprintf(stderr, "Magic number mismatch on %s\n", filename);
	exit(1);
    }
    bcopy(theinfo.fi_fndr, file_info + I_TYPEOFF, 4);
    bcopy(theinfo.fi_fndr + 4, file_info + I_AUTHOFF, 4);
    bcopy(theinfo.fi_fndr + 8, file_info + I_FLAGOFF, 2);
    if(theinfo.fi_bitmap & FI_BM_MACINTOSHFILENAME) {
	n = strlen(theinfo.fi_macfilename);
	(void)strncpy(file_info + I_NAMEOFF + 1, (char *)theinfo.fi_macfilename,
		n);
    } else if(theinfo.fi_bitmap & FI_BM_SHORTFILENAME) {
	n = strlen(theinfo.fi_shortfilename);
	(void)strncpy(file_info + I_NAMEOFF + 1,
		(char *)theinfo.fi_shortfilename, n);
    } else {
	n = strlen(filename);
	(void)strncpy(file_info + I_NAMEOFF + 1, filename, n);
    }
    file_info[I_NAMEOFF] = n;
#ifdef AUFSPLUS
    if(theinfo.fi_datemagic == FI_MAGIC &&
       (theinfo.fi_datevalid & (FI_CDATE | FI_MDATE)) ==
		(FI_CDATE | FI_MDATE)) {
	put4(file_info + I_CTIMOFF, get4(theinfo.fi_ctime) + TIMEDIFF);
	put4(file_info + I_MTIMOFF, get4(theinfo.fi_mtime) + TIMEDIFF);
    } else {
	if(fstat(fileno(fd), &stbuf) >= 0) {
	    put4(file_info + I_CTIMOFF,
		(unsigned long)stbuf.st_ctime + TIMEDIFF);
	    put4(file_info + I_MTIMOFF,
		(unsigned long)stbuf.st_mtime + TIMEDIFF);
	}
    }
#else /* AUFSPLUS */
    if(fstat(fileno(fd), &stbuf) >= 0) {
	put4(file_info + I_CTIMOFF, (unsigned long)stbuf.st_ctime + TIMEDIFF);
	put4(file_info + I_MTIMOFF, (unsigned long)stbuf.st_mtime + TIMEDIFF);
    }
#endif /* AUFSPLUS */
}
#endif /* AUFS */

#ifdef APPLEDOUBLE
/* This version assumes that the AppleDouble info header is always the same
   size and format.  I have not yet seen something that will lead me to
   believe different.
*/
static void read_appledouble_info(fd)
FILE *fd;
{
    FileInfo theinfo;
    int i, n;

    for(i = 0; i < INFOBYTES; i++) {
	file_info[i] = 0;
    }
    bzero((char *) &theinfo, sizeof(theinfo));
    if(fread((char *)&theinfo, 1, sizeof(theinfo), fd) != sizeof(theinfo)) {
	(void)fprintf(stderr, "Short AppleDouble info header for %s\n",
		filename);
	exit(1);
    }
    if(get4(theinfo.fi_magic) != FI_MAGIC ||
       get2(theinfo.fi_version) != FI_VERSION) {
	(void)fprintf(stderr, "Magic number mismatch on %s\n", filename);
	exit(1);
    }
    bcopy(theinfo.fi_type, file_info + I_TYPEOFF, 4);
    bcopy(theinfo.fi_auth, file_info + I_AUTHOFF, 4);
    bcopy(theinfo.fi_finfo, file_info + I_FLAGOFF, 2);
    n = get4(theinfo.fi_namlen);
    (void)strncpy(file_info + I_NAMEOFF + 1, theinfo.fi_name, n);
    file_info[I_NAMEOFF] = n;
    put4(file_info + I_CTIMOFF, get4(theinfo.fi_ctime) + TIMEDIFF);
    put4(file_info + I_MTIMOFF, get4(theinfo.fi_mtime) + TIMEDIFF);
    rsrc_size = get4(theinfo.fi_rsrc);
    put4(file_info + I_RLENOFF, (unsigned long)rsrc_size);
}
#endif /* APPLEDOUBLE */
#endif /* APPLESHARE */

static int get_stdin_file()
{
    int i, skip;

    i = fgetc(stdin);
    if(i == EOF) {
	return ISATEND;
    }
    (void)ungetc(i, stdin);
    if(fread(file_info, 1, INFOBYTES, stdin) != INFOBYTES) {
	(void)fprintf(stderr, "Short input\n");
	exit(1);
    }
    if(file_info[0] != 0) {
	(void)fprintf(stderr, "File is not MacBinary: %s\n", filename);
	exit(1);
    }
    data_size = get4(file_info + I_DLENOFF);
    rsrc_size = get4(file_info + I_RLENOFF);
    if(file_info[I_LOCKOFF] & 1) {
	file_info[I_FLAGOFF + 1] = PROTCT_MASK;
	file_info[I_LOCKOFF] &= ~1;
    }
    if(data_size != 0) {
	if(data_size > max_data_size) {
	    if(data_fork == NULL) {
		data_fork = malloc((unsigned)data_size);
	    } else {
		data_fork = realloc(data_fork, (unsigned)data_size);
	    }
	    max_data_size = data_size;
	}
	if(fread(data_fork, 1, data_size, stdin) != data_size) {
	    (void)fprintf(stderr, "Short input\n");
	    exit(1);
	}
	skip = (((data_size + 127) >> 7) << 7) - data_size;
	for(i = 0; i < skip; i++) {
	    (void)fgetc(stdin);
	}
    }
    if(rsrc_size != 0) {
	if(rsrc_size > max_rsrc_size) {
	    if(rsrc_fork == NULL) {
		rsrc_fork = malloc((unsigned)rsrc_size);
	    } else {
		rsrc_fork = realloc(rsrc_fork, (unsigned)rsrc_size);
	    }
	    max_rsrc_size = rsrc_size;
	}
	if(fread(rsrc_fork, 1, rsrc_size, stdin) != rsrc_size) {
	    (void)fprintf(stderr, "Short input\n");
	    exit(1);
	}
	skip = (((rsrc_size + 127) >> 7) << 7) - rsrc_size;
	for(i = 0; i < skip; i++) {
	    (void)fgetc(stdin);
	}
    }
    if(file_info[I_NAMEOFF] & 0x80) {
	if((file_info[I_NAMEOFF] & 0xff) == 0x80) {
	    return ENDDIR;
	}
	return ISDIR;
    }
    return ISFILE;
}

int rdfileopt(c)
char c;
{
extern char *optarg;
char name[32];

    switch(c) {
    case 'd':
	data_only = DATA_FORMAT;
	break;
    case 'u':
    case 'U':
	data_only = UNIX_FORMAT;
	break;
    case 'r':
	data_only = RSRC_FORMAT;
	break;
    case 'c':
	backtrans(name, optarg);
	(void)strncpy(f_auth, name, 4);
	break;
    case 't':
	backtrans(name, optarg);
	(void)strncpy(f_type, name, 4);
	break;
    default:
	return 0;
    }
    return 1;
}

void give_rdfileopt()
{
    (void)fprintf(stderr, "File input options:\n");
    (void)fprintf(stderr, "-r:\tread as resource files\n");
    (void)fprintf(stderr, "-d:\tread as data files\n");
    (void)fprintf(stderr,
	"-u:\tread as data files with Unix -> Mac text file translation\n");
    (void)fprintf(stderr, "-U:\ta synonym for -u\n");
    (void)fprintf(stderr,
	"-c cr:\tcreator if one of the above options is used\n");
    (void)fprintf(stderr,
	"-t ty:\tfiletype if one of the above options is used\n");
}

void set_norecurse()
{
    no_recurse = 1;
}

char *get_rdfileopt()
{
    static char options[] = "rduUc:t:";

    return options;
}

char *get_minb()
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

