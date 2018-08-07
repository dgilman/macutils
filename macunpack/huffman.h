#define	HUFF_BE		0
#define	HUFF_LE		1

typedef struct node {
    int flag, byte;
    struct node *one, *zero;
} node;

extern int (*get_bit)();
extern void clrhuff();

extern struct node nodelist[];
extern int bytesread;

