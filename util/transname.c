#include <sys/types.h>
#include <sys/dir.h>

char *strncpy();

#ifdef MAXNAMLEN	/* 4.2 BSD */
#define FNAMELEN MAXNAMLEN
#else
#define FNAMELEN DIRSIZ
#endif

/* Create a Unix file name based on the Mac filename.  First off we have
 * possible problems with filename sizes (Sys V) and also with allowable
 * characters.  Mac filename characters can be anything from 1 to 254 (I
 * think 0 and 255 are not allowed) with colon disallowed.  Unix filenames
 * have also a lot of freedom, but cannot contain 0 or '/'.  Also on Unix
 * non-printable are a trouble (you will never see the filename correctly;
 * and it may even lock your terminal with some versions of Unix).
 * So first off non-printable characters are mapped to underscore.
 * Although there are Unix systems that allow the high order bit set in
 * a filename character in all programs, nearly all implementations do not
 * allow that, so also characters in the range 0200-0377 are mapped to
 * underscore (except as noted below).  Some people would also like to
 * remap characters that are special to some shells (open brackets,
 * asterisks, exclamation point (csh), etc.)  I did elect not to do so
 * because there is no end.  (The previous code disallowed a lot, but not
 * the braces that are special to some shells, obviously he was a C-shell user!)
 * Characters in the range 0200-0377 are in part accented letters
 * (the Swedes, Norwegians and Danes would not agree, but who listens to
 * them!); those are mapped to the unaccented version.  All other characters
 * in this range are mapped to underscore.  Note: this is based on the
 * Geneva font!
 * This stuff is now largely table driven.
 * One day I may modify this so that an environment variable may be used
 * to define mappings. */

static char char_mapping[] = {
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '!',  '"',  '#',  '$',  '%',  '&', '\'',
	 '(',  ')',  '*',  '+',  ',',  '-',  '.',  '_',
	 '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	 '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	 '@',  'A',  'B',  'C',  'D',  'E',  'F',  'G',
	 'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
	 'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
	 'X',  'Y',  'Z',  '[', '\\',  ']',  '^',  '_',
	 '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	 'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	 'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	 'x',  'y',  'z',  '{',  '|',  '}',  '~',  '_',
#ifndef LATIN1
	 'A',  'A',  'C',  'E',  'N',  'O',  'U',  'a',
	 'a',  'a',  'a',  'a',  'a',  'c',  'e',  'e',
	 'e',  'e',  'i',  'i',  'i',  'i',  'n',  'o',
	 'o',  'o',  'o',  'o',  'u',  'u',  'u',  'u',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  'O',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  'o',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  'A',  'A',  'O',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 'y',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
#else /* LATIN1 */
	0304, 0305, 0307, 0311, 0321, 0326, 0334, 0341,
	0340, 0342, 0344, 0343, 0345, 0347, 0351, 0350,
	0352, 0353, 0355, 0354, 0356, 0357, 0361, 0363,
	0362, 0364, 0366, 0365, 0372, 0371, 0373, 0374,
	 '_', 0260, 0242, 0243, 0247, 0267, 0266, 0337,
	0256, 0251,  '_', 0264, 0250,  '_', 0306, 0330,
	 '_', 0261,  '_',  '_', 0245,  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_', 0346, 0370,
	0277, 0241, 0254,  '_',  '_',  '_',  '_', 0253,
	0273,  '_',  '_', 0300, 0303, 0325,  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_', 0367, 0244,
	0377,  '_',  '_',  '_',  '_',  '_',  '_',  '_',
#endif /* LATIN1 */
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_',
	 '_',  '_',  '_',  '_',  '_',  '_',  '_',  '_'};

void transname(name, namebuf, n)
char *name, *namebuf;
int n;
{
    char *np;

    /* make sure host file name doesn't get truncated beyond recognition */
    if (n > FNAMELEN - 2) {
	n = FNAMELEN - 2;
    }
    (void)strncpy(namebuf, name, n);
    namebuf[n] = '\0';

    /* now: translate name */
    for (np = namebuf; *np; np++){
	*np = char_mapping[*np & 0xff];
    }
#ifdef NODOT
    if(*namebuf == '.') {
	*namebuf = '_';
    }
#endif /* NODOT */
}

