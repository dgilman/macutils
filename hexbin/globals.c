#include "globals.h"
#include "../fileio/machdr.h"
#include "../fileio/wrfile.h"
#include "../fileio/kind.h"

struct macheader mh;

char info[INFOBYTES];
char trname[64];

int listmode;
int info_only;
int verbose;
int uneven_lines;
int to_read;
int was_macbin;

FILE *ifp;

#ifdef SCAN
void do_error(string)
char *string;
{
    do_idf(string, ERROR);
}
#endif /* SCAN */

