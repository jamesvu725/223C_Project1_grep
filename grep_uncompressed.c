#include <unistd.h> // write, read, close
#include <stdio.h> // EOF
#include <stdlib.h> // exit
#include <fcntl.h> // open
#include <string.h> // strlen
#include <dirent.h> // directory

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

int  peekc, ninbuf, io, nbra, mflag = 0, match = 0;
char linebuf[LBSIZE], expbuf[ESIZE+4], filebuf[LBSIZE], *braslist[NBRA], *braelist[NBRA];
char *nextip, line[70], *linp  = line, *loc1;
char *files[MAXFILE], *fname, *regex;

int advance(char *lp, char *ep);
int cclass(char *set, int c, int af);
void commands(void);
void compile(char* s);
int execute(void);
int getfile(void);
void exfile(const char* filename);
void putchr_(int ac);
void puts_(char *sp);
void putsf(char *sp);
void search(void);
int backref(int i, char *lp);

typedef struct dirent dirent;
DIR* dir;
dirent* in_dir;

int main(int argc, char *argv[]) {
  if (argc < 3) {
	puts_("Not enough arguments");
	return 0;
  } // ends program if not enough arguments
  regex = argv[1]; // stores argument in regex
  fname = argv[2]; // stores argument in fname
  int nfiles = 0; // set number of files to 0

  if ((dir = opendir(fname)) != NULL) { // opens directory if not null
    while((in_dir = readdir(dir)) != NULL) { // loops through files in directory
      if (in_dir->d_name[0] != '.' && in_dir->d_name[strlen(in_dir->d_name)-1] != '~') { // ignores hidden or temporary files
        files[nfiles++] = in_dir->d_name; // stores the filename in files array and increment nfiles
		if (nfiles >= MAXFILE) { // checks if there are too many files
		  puts_("Too many files");
		  exit(2);
      }
    }
    mflag = 1; // sets multiple files flag
    closedir(dir); // close the directory
  } else if (argc > 3) { // condition for multiple files
    files[0] = fname; // stores first file in the array
    mflag = 1; // set multiple files flag
	++nfiles; // increment nfiles
    for (size_t i = 3; i < argc; ++i) {
      files[nfiles++] = argv[i]; // loops through arguments and stores it in files array
    }
  }
  compile(regex); // compiles the regex string and puts it in expbuf
  if (mflag == 1) { // if multiple files flag is true
    for (int i = 0; i < nfiles; ++i) { // loops through the files array
      fname = files[i]; // set fname to the file in files array
      exfile(fname); // opens and read content from file and stores it in filebuf
      search(); // search for regex in file and prints the lines that matches
    }
  } else {
    exfile(fname); // opens and read content from file and stores it in filebuf
    search(); // search for regex in file and prints the lines that matches
  }
  if (!match) {
	puts_("No matches found");
	exit(1);
  } // prints no matches found if match flag is down
  exit(0); // exits program successfully
  return 0;
}
// removed goto; checks for pattern in linebuf
int advance(char *lp, char *ep) {
  char *curlp;
  int i;
  for (;;) {
    switch (*ep++) {
      case CCHR:  if (*ep++ == *lp++) { continue; } return(0);
      case CDOT:  if (*lp++) { continue; } return(0);
      case CDOL:  if (*lp==0) { continue; } return(0);
      case CEOF:  return(1);
      case CCL:   if (cclass(ep, *lp++, 1)) { ep += *ep; continue; } return(0);
      case NCCL:  if (cclass(ep, *lp++, 0)) { ep += *ep; continue; } return(0);
      case CBRA:  braslist[(unsigned)*ep++] = lp; continue;
      case CKET:  braelist[(unsigned)*ep++] = lp; continue;
      case CBACK:
        if (braelist[i = *ep++] == 0) {
		  puts_("Syntax Error");
		  exit(2);
		}
        if (backref(i, lp)) {
		  lp += braelist[i] - braslist[i];
		  continue;
		}
		return(0);
      case CBACK|STAR:
        if (braelist[i = *ep++] == 0) {
		  puts_("Syntax Error");
		  exit(2);
		}
		curlp = lp;
        while (backref(i, lp)) { lp += braelist[i] - braslist[i]; }
        while (lp >= curlp) {
          if (advance(lp, ep)) { return(1); }
          lp -= braelist[i] - braslist[i];
        }
		continue;
      case CDOT|STAR:  curlp = lp;
	    while (*lp++) { }
        do {
		  lp--;
		  if (advance(lp, ep)) { return(1); }
		} while (lp > curlp);
		return(0);
      case CCHR|STAR:
	    curlp = lp;
		while (*lp++ == *ep) { }
		++ep;
        do {
		  lp--;
		  if (advance(lp, ep)) { return(1); }
		} while (lp > curlp);
		return(0);
      case CCL|STAR:
      case NCCL|STAR:
        curlp = lp;
		while (cclass(ep, *lp++, ep[-1] == (CCL|STAR))) { }
		ep += *ep;
        do {
		  lp--;
		  if (advance(lp, ep)) { return(1); }
		} while (lp > curlp);
		return(0);
      default:
	    puts_("Syntax Error");
		exit(2);
    }
  }
}
// original backref, used in advance
int backref(int i, char *lp) {
  char *bp;
  bp = braslist[i];
  while (*bp++ == *lp++) {
    if (bp >= braelist[i]) { return(1); }
  }
  return(0);
}
// needed for regular expression
int cclass(char *set, int c, int af) {
  if (c == 0) { return 0; }
  int n = *set++;
  while (--n) {
	if (*set++ == c) { return(af); }
  }
  return(!af);
}
// editted so it doesn't use getchr and uses regex instead
// removed goto statements and \n statements since regex shouldn't have it
// compiles the pattern or regular expression and puts it in expbuf
void compile(char* s) {
  int c, cclcnt;
  nbra = 0;
  char *ep = expbuf, *lastep = 0, bracket[NBRA], *bracketp = bracket, *sp = s;
  if (*sp=='^') { ++sp;  *ep++ = CCIRC; }
  peekc = *sp;
  for (;;) {
    c = *sp++;
	if (c == '\0') {
	  *ep++ = CEOF;
	  return;
	}
    if (c != '*') { lastep = ep; }
    switch (c) {
    case '\\':
      if ((c = *sp++) == '(') {
        *bracketp++ = nbra;
		*ep++ = CBRA;
		*ep++ = nbra++;
		continue;
      }
      if (c == ')') {
		*ep++ = CKET;
		*ep++ = *--bracketp;
		continue;
	  }
      if (c >= '1' && c < '1' + NBRA) {
		*ep++ = CBACK;
		*ep++ = c - '1';
		continue;
	  }
      *ep++ = CCHR;
	  *ep++ = c;
	  continue;
    case '.': *ep++ = CDOT; continue;
    case '*':
      if (lastep == 0 || *lastep == CBRA || *lastep == CKET) {
        *ep++ = CCHR;
		*ep++ = c;
		continue;
      }
      *lastep |= STAR;
	  continue;
    case '$':
      if ((peekc = *sp++) != '\0') {
		*ep++ = CCHR;
		*ep++ = c;
		continue;
	  }
      --sp;
	  *ep++ = CDOL;
	  continue;
    case '[':
      *ep++ = CCL;
	  *ep++ = 0;
	  cclcnt = 1;
      if ((c = *sp++) == '^') {
		c = *sp++;
		ep[-2] = NCCL;
	  }
      do {
        if (c == '-' && ep[-1] != 0) {
          if ((c = *sp++) == ']') {
			*ep++ = '-';
			cclcnt++;
			break;
		  }
          while (ep[-1] < c) {
			*ep = ep[-1] + 1;
			ep++;
			cclcnt++;
		  }
        }
        *ep++ = c;
		cclcnt++;
      } while ((c = *sp++) != ']');
      lastep[1] = cclcnt;
	  continue;
    default:
      *ep++ = CCHR;
	  *ep++ = c;
    }
  }
}
// deleted most of execute, uses only the regular algorithm portion
// returns 1 if pattern is found in line
int execute(void) {
  char *p1 = linebuf, *p2 = expbuf;
  do {
	if (advance(p1, p2)) { return(1); }
  } while (*p1++);
  return(0);
}
// took first line from caseread
// opens the file and store its content in filebuf
void exfile(const char* filename) {
  if ((io = open(filename, 0)) < 0) {
	puts_("Cannot open file");
	exit(2);
  } // try to open file
  getfile();
  close(io);
  io = -1; // get content from file and stores it in filebuf
}
// get content from file and stores it in filebuf
// original getfile, added clear buffer statements, removed a few statements
int getfile(void) {
  int c;
  char *lp = linebuf, *fp = nextip;
  memset(linebuf, 0, sizeof(linebuf));
  memset(filebuf, 0, sizeof(filebuf)); // clear buffer
  do {
    if (--ninbuf < 0) {
      if ((ninbuf = (int)read(io, filebuf, LBSIZE)-1) < 0) { return(EOF); }
      fp = filebuf;
	  while(fp < &filebuf[ninbuf]) {
		if (*fp++ & 0200) { break; }
	  }
	  fp = filebuf;
    }
    c = *fp++;
	if (c=='\0') { continue; }
    *lp++ = c;
  } while (c != '\n');
  *--lp = 0;
  nextip = fp;
  return(0);
}
// print out a character
void putchr_(int ac) {
  char *lp = linp;
  int c = ac;
  *lp++ = c;
  if (c == '\n' || lp >= &line[64]) {
    linp = line;
	write(1,line, lp - line);
	return;
  }
  linp = lp;
}
// prints out the string with newline at the end
void puts_(char *sp) {
  while (*sp) {
    putchr_(*sp++);
  }
  putchr_('\n');
}
// prints out the string with colon at the end // for multiple files
void putsf(char *sp) {
  while (*sp) {
	putchr_(*sp++);
  }
  putchr_(':');
}
// searches for regex in the file and prints out the line if found
void search(void) {
  char *fp = filebuf, *lp = linebuf;
  // loops through filebuf and copies each line to linebuf
  // searches for regex in linebuf and prints it out if found
  while (*fp != '\0') { // while filebuf is not EOF
    if (*fp == '\n') {
	  *lp++ = '\0';// when filebuf hits newline, linebuf ends
      lp = linebuf; //sets lp back to the beginning of the buffer
      if (execute()) { // runs execute to find if linebuf has the pattern
        if (mflag) { putsf(fname); } // prints out filename: if multiple files
        puts_(linebuf); // prints out linebuf
        match = 1; // match flag set
      }
      ++fp; //goes to next character in filebuf
    } else { *lp++ = *fp++; } // copies filebuf to linebuf
  }
}
