#define LBSIZE 4096
#define ESIZE 256
#define NBRA 5
#define CBRA 1
#define CCHR 2
#define CDOT 4
#define CCL 6
#define NCCL 8
#define CDOL 10
#define CEOF 11
#define CKET 12
#define CBACK 14
#define CCIRC 15
#define STAR 01
#define MAXFILE 100

int  peekc, ninbuf, io, nbra;

char linebuf[LBSIZE], expbuf[ESIZE+4], filebuf[LBSIZE], *braslist[NBRA], *braelist[NBRA];
char *nextip, line[70], *linp  = line, *loc1;

int mflag = 0, match = 0;
char* files[MAXFILE], *fname, *regex;

int advance(char *lp, char *ep); int cclass(char *set, int c, int af);
void commands(void); void compile(char* s); int execute(void); int getfile(void);
void exfile(const char* filename); void putchr_(int ac); void puts_(char *sp);
void putsf(char *sp); void search(void); int backref(int i, char *lp);
