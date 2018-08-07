#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"

char info[INFOBYTES];
char text[F_NAMELEN+1];

int list, verbose, info_only, query, write_it, indent, dir_skip, no_dd;
FILE *infp;
int in_data_size = -1;
int in_rsrc_size = -1;
int in_ds, in_rs, ds_skip, rs_skip;

#ifdef SCAN
void do_error(string)
char *string;
{
    do_idf(string, ERROR);
}
#endif /* SCAN */

