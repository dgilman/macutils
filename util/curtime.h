#ifdef TYPES_H
#include <sys/types.h>
#endif /* TYPES_H */

#ifdef BSD
#include <sys/time.h>
extern time_t time();
#else /* BSD */
#include <time.h>
#endif /* BSD */

/* Mac time of 00:00:00 GMT, Jan 1, 1970 */
#define TIMEDIFF 0x7c25b080

