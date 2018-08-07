#include "globals.h"
#include "../fileio/machdr.h"

char info[INFOBYTES];
char text[F_NAMELEN+1];

int list, info_only, query, write_it, indent, dir_skip;
FILE *infp;
int in_data_size = -1;
int in_rsrc_size = -1;
int in_ds, in_rs, ds_skip, rs_skip;

