/***************************************************************************

Version History:

Ver.No	Comment							By
===========================================================================
1.0 	first version (seems to to things right)   		andy@mssx
1.1     modified to take ^D as end-of-edit and dump first sec.  wsiebeck
1.2	Added functionality of readstr				wsiebeck
1.3	Added flag '-r' for read-only				kkaempf
1.4	Added flag '-s' for small-screen (256 Bytes)		kkaempf
	Added flag '-v' for version				kkaempf
1.5	Support large disks					kkaempf
1.6	EBCDIC added		 				kkaempf

I declare this program as freeware, i.e. you may duplicate it, give it
to your friends, and transfer it to any machine you like, as long as
you do not change or delete the build in copyright message.

	Andreas Pleschutznig
	Teichhofweg 2
	8044 Graz
	Austria

Comments and bug reports to:
	andy@mssx	(mcvax!tuvie!mssx!andy)


*****************************************************************************/

#define VERSION "BPE Version 1.6"

#include <curses.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include "curstr.h"

#define	BELL	0x07
#define ASCX	67
#define ASCY	6
#define HEXY	6
#define HEXX	16

#define CTL(x) (x&0x1f)

#define beep() {putchar(0x07),fflush(stdout);}

#if 0
#define KEY_UP CTL('E')
#define KEY_DOWN CTL('X')
#define KEY_LEFT CTL('S')
#define KEY_RIGHT CTL('D')
#endif

/* Max values  */
#define MAXSECSIZE 512
#define MAXROWS 32
#define MAXCOLS 16

/* Real values */
int sec_size;
int view_rows;
int view_cols;

int     sec_mode = 0;			/* flag for sector_mode (raw!) */
int     ebcdic = 0;                     /* flag for ebcdic display */
int     path;				/* path to file to patch */
long long    filpos;				/* position in file */
long long    fsize;				/* size of file */
unsigned char secbuf[MAXSECSIZE];		/* sector read buffer */
int     rawdevice = 0;			/* entscheidet, ob man sich um filegroesse
					 * kuemmert ... */
int	read_only = 0;			/* wenn 1, dann nicht schreiben */

 /*int     donix();*//* default signal handling routine */
char    filename[256];
int     length;			/* length of read sector */

struct INPUTSTR inp;


static int command();

static void
usage(name)
    char *name;
{
  fprintf(stderr, "Usage: %s [-r] [-s] filename\n", name);
  exit(1);
}

static void
version()
{
  printf("%s (23.09.95) (kwk)\n", VERSION);
  exit(1);
}


static unsigned char
conv(unsigned char i, int as_char)
{
    if (i == 0x40) return ' ';
    if ((i > 0x49) && (i < 0x50)) {
        return "¢.<(+|"[i-0x4a];
    }
    if (i == 0x50) return '&';
    if ((i > 0x59) && (i < 0x60)) {
        return "!$*);¬"[i-0x5a];
    }
    if ((i >= 0x60) && (i < 0x62)) {
        return "-/"[i-0x62];
    }
    if ((i > 0x69) && (i < 0x70)) {
        return "|,%_>?"[i-0x6a];
    }
    if ((i > 0x78) && (i < 0x80)) {
        return "`:#@'=\""[i-0x79];
    }
    if ((i > 0x80) && (i < 0x8a)) {
        return "abcdefghi"[i-0x81];
    }
    if ((i > 0x90) && (i < 0x9a)) {
        return "jklmnopqr"[i-0x91];
    }
    if ((i > 0xa0) && (i < 0xaa)) {
        return "~stubwxyz"[i-0xa1];
    }
    if ((i > 0xc0) && (i < 0xca)) {
        return "ABCDEFGHI"[i-0xc1];
    }
    if ((i > 0xd0) && (i < 0xda)) {
        return "JKLMNOPQR"[i-0xd1];
    }
    if ((i > 0xe1) && (i < 0xea)) {
        return "STUVWXYZ"[i-0xe2];
    }
    if ((i >= 0xf0) && (i < 0xfa)) {
        return '0' + (i - 0xf0);
    }
    return (as_char) ? '.' : i;
}

static void
header(char *left, char *mid, char *right)
{
    mvprintw(0, 0, "%s", left);
    mvprintw(0, 89 - strlen(right), "%s", right);
    mvprintw(0, 40 - strlen(mid) / 2, "%s", mid);
}


static void
werr(char *errstr)
{
    beep();
    move(LINES - 1, 0);
    standout();
    printw("%s", errstr);
    standend();
    refresh();
    sleep(2);
    move(LINES - 1, 0);
    clrtoeol();
    refresh();
}


int
getcr()
{
    int     i;

    for (i = 0; i != 13; i = getkey());
}


static void
help()
{
    WINDOW *win;

    win = newwin(0, 0, 0, 0);
    wclear(win);
    mvwprintw(win, 3, 10, "Valid Commands are :");
    mvwprintw(win, 5, 15, "D - Dump one page from current file position");
    mvwprintw(win, 6, 15, "S - Set current file pointer");
    mvwprintw(win, 7, 15, "F - Find string in file (beginning from curr. position)");
    mvwprintw(win, 8, 15, "H - Find hex string in file (beginning from current position)");
    mvwprintw(win, 9, 15, "+ - Display next sector");
    mvwprintw(win, 10, 15, "N - Display next sector");
    mvwprintw(win, 11, 15, "- - Display previous sector");
    mvwprintw(win, 12, 15, "P - Display previous sector");
    mvwprintw(win, 13, 15, "e - Edit ASCII portion of file");
    mvwprintw(win, 14, 15, "E - Edit binary portion of file");
    mvwprintw(win, 15, 15, "W - Write modified sector back to disk");
    mvwprintw(win, 16, 15, "R - Toggle raw mode, raw ON ignores file size");
    mvwprintw(win, 17, 15, "M - Toggle sector mode, Position (cmd S) is in 256 incs");
    mvwprintw(win, 19, 15, "Q - Quit Program");
    wstandout(win);
    mvwprintw(win, 24, 0, "Continue with CR");
    wstandend(win);
    wrefresh(win);
    getcr();
    delwin(win);
    touchwin(stdscr);
    refresh();
}


int
disp(int length)
{
    int     i,
            j;

    j = 0;
    standout();
    mvprintw(4, 0, " ADDRESS        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F    ASCII             EBCDIC          ");
    mvprintw(5, 0, "=====================================================================================================");
    standend();
    for (i = 0; i < view_rows; i++) {
	standout();
	mvprintw(ASCY + i, 0, "%012llX", filpos + i * view_cols);
	standend();
	for (j = 0; j < view_cols; j++) {
            unsigned char v;
	    if ((i * view_cols + j) >= length) {
		clrtoeol();
		goto Disp1;
	    }
            v = secbuf[i * view_cols + j] & 0xFF;
	    mvprintw(ASCY + i, HEXX + j * 3, "%02X", v);
	}
Disp1:
	for (j = 0; j < view_cols; j++) {
            unsigned char v;
	    if ((i * view_cols + j) >= length) {
		clrtobot();
		goto Disp2;
	    }
            v = secbuf[i * view_cols + j] & 0xFF;
	    if ((v >= ' ') && (v < 0x7f))
		mvprintw(ASCY + i, ASCX + j, "%c", v & 0x7f);
	    else
		mvprintw(ASCY + i, ASCX + j, ".");
	}
Disp2:
	for (j = 0; j < view_cols; j++) {
            unsigned char v;
	    if ((i * view_cols + j) >= length) {
		clrtobot();
		goto Disp3;
	    }
            v = secbuf[i * view_cols + j] & 0xFF;
            v = conv(v, 1);
	    if ((v >= ' ') && (v < 0x7f))
		mvprintw(ASCY + i, ASCX + j + view_cols + 2, "%c", v & 0x7f);
	    else
		mvprintw(ASCY + i, ASCX + j + view_cols + 2, ".");
	}
    }
Disp3:
    refresh();
}


int
rdsec()
{
    mvprintw(2, 30, "Size : %012llX", fsize);
    mvprintw(2, 51, "Rel. Position : %012llX", filpos);
    lseek(path, filpos, 0);
    length = read(path, secbuf, sec_size);
    return (length);
}


int
dump()
{
    int     i,
            j;

    length = rdsec();
    disp(length);
    return (length);
}


void
wrsec()
{
    lseek(path, filpos, 0);
    write(path, secbuf, length);
}


static void
edit_ascii()
{
  unsigned int     inval = 0;
  int     cury,
          curx;

  if (length == 0)
    length = dump();
  move(2, 0);
  clrtoeol();

  printw("End ASCII editing with ESC");
  curx = cury = 0;
  while (inval != ESC)
    {
      move(ASCY + cury, ASCX + curx);
      refresh();
      inval = getkey();
      switch (inval)
	{
	  case KEY_UP:
	    if (cury)
	      cury--;
	    else
	      beep();
	    break;
	  case KEY_DOWN:
	    if (cury < (view_rows-1))
	      cury++;
	    else
	      beep();
	    break;
	  case KEY_RIGHT:
	    if (curx < (view_cols-1))
	      curx++;
	    else
		beep();
	    break;
	  case KEY_LEFT:
	    if (curx)
	      curx--;
	    else
	      beep();
	    break;
	  default:
	    if ((inval >= 0x20) && (inval <= 0x7e))
	      {
		secbuf[cury * view_cols + curx] = inval;
		curx++;
		if (curx > (view_cols-1))
		  {
		    curx = 0;
		    cury++;
		  }
		if (cury > (view_rows-1))
		  cury = 0;
		disp(length);
	      }
	    else if (inval == ESC)
	      {
		move(2, 0);
		clrtoeol();
		return;
	      }
	    break;
	}
    }
    move(2, 0);
    clrtoeol();
}

static unsigned int
gethex(int cury, int curx)
{
    unsigned int     val;
    int     inlen;
    unsigned int     value;

    inlen = 0;
    while (inlen < 2) {
	val = getkey();
	if (val > 255)
	    return (val);
	if (val == ESC)
	    return (-1);
	if (val > '9')
	    val &= ~0x20;
	if (((val >= '0') && (val <= '9')) ||
	    ((val >= 'A') && (val <= 'F'))) {
	    if (val <= '9')
		val -= '0';
	    else
		val = val - 'A' + 0x0a;
	    switch (inlen) {
	      case 0:
		value = val << 4;
		secbuf[cury * view_cols + curx] = value;
		disp(length);
		move(HEXY + cury, HEXX + curx * 3 + 1);
		refresh();
		break;
	      case 1:
		value += val;
		break;
	    }
	    inlen++;
	}
    }
    return (value);
}

static void
edit_hex()
{
    unsigned int     inval = 0;
    int     cury,
            curx;

    if (length == 0)
	length = dump();
    move(2, 0);
    clrtoeol();
/*        printw("End editing with CNTRL-C");  wolle */
    printw("End HEX editing with ESC");
    curx = cury = 0;
    while (inval != -1) {
	move(HEXY + cury, HEXX + curx * 3);
	refresh();
	inval = gethex(cury, curx);
	switch (inval) {
	  case KEY_UP:
	    if (cury)
		cury--;
	    else
		beep();
	    break;
	  case KEY_DOWN:
	    if (cury < (view_rows-1))
		cury++;
	    else
		beep();
	    break;
	  case KEY_RIGHT:
	    if (curx < (view_cols-1))
		curx++;
	    else
		beep();
	    break;
	  case KEY_LEFT:
	    if (curx)
		curx--;
	    else
		beep();
	    break;
	  default:
	    if ((inval >= 0x00) && (inval <= 0xff)) {
		secbuf[cury * view_cols + curx] = inval;
		curx++;
		if (curx > (view_cols-1)) {
		    curx = 0;
		    cury++;
		}
		if (cury > (view_rows-1))
		    cury = 0;
		disp(length);
	    } else if (inval == -1) {
		move(2, 0);
		clrtoeol();
		return;
	    }
	    break;
	}
    }
    move(2, 0);
    clrtoeol();
}


static int
testchar(unsigned char *buffer, unsigned char *string, int length)
{
    register int i;

    i = 0;
    while (i < length) {
	if (buffer[i] != string[i])
	    break;
	i++;
    }
    if (i == length)
	return (0);
    return (1);
}


static void
find_string()
{
    int     stlen,
            retkey;
    static unsigned char string[60] = { 0 };
    int     found;
    int     searchpos;

    move(2, 0);
    clrtoeol();
    printw("String to search : ");
    move(2, 19);
    refresh();
/*	echo();
	getstr(string);*/
    inp.maxlen = 40;
    inp.ein[0] = '\0';
    retkey = readstr(&inp);
/*	noecho();*/
    move(2, 0);
    clrtoeol();
    if (retkey != VKEY_RETURN)
	return;
    strcpy(string, inp.ein);
    printw("Searching for '%s'", string);
    found = 0;
    searchpos = 0;
    stlen = strlen(string);
    while (found == 0) {
	while ((sec_size - searchpos) >= stlen) {
	    if (testchar(secbuf + searchpos, string, stlen))
		searchpos++;
	    else {
		filpos += searchpos;
		found = 1;
		break;
	    }
	}
	if (found == 0) {
	    filpos += searchpos;
	    searchpos = 0;
	}
	if (rdsec() == 0) {
	    found = 1;
	}
	refresh();
    }
    move(2, 0);
    clrtoeol();
}


static void
find_hex()
{
    int     stlen,
            retkey;
    unsigned char    string[20];
    int     found;
    int     searchpos;
    char *iptr;
    unsigned char *sptr;
    int val;

    move(2, 0);
    clrtoeol();
    printw("Hexval to search : ");
    move(2, 19);
    refresh();
/*	echo();
	getstr(string);*/
    inp.maxlen = 40;
    inp.ein[0] = '\0';
    retkey = readstr(&inp);
/*	noecho();*/
    move(2, 0);
    clrtoeol();
    if (retkey != VKEY_RETURN)
	return;

    stlen = strlen(inp.ein);
    if (stlen & 1 == 1) {		/* odd len, cancel last digit */
      stlen--;
      string[stlen] = '\0';
    }

    printw("Searching for '%s'", inp.ein);

    iptr = inp.ein;
    sptr = string;		/* copy inp.ein(hex) to string(bin) */

    while (*iptr != '\0') {
	if (isupper(*iptr))
	  *iptr = tolower(*iptr);
	if (isdigit(*iptr))
	  val = *iptr - '0';
	else {
	  val = *iptr - 'a' + 10;
	  if (val > 15) {
	    werr("Error, bad hex digit");
	    return;
	  }
	}
	*sptr = val << 4;
	iptr++;
	if (isupper(*iptr))
	  *iptr = tolower(*iptr);
	if (isdigit(*iptr))
	  val = *iptr - '0';
	else {
	  val = *iptr - 'a' + 10;
	  if (val > 15) {
	    werr("Error, bad hex digit");
	    return;
	  }
	}
	*sptr += val;
	iptr++;
	sptr++;
    }
    found = 0;
    searchpos = 0;
    stlen >>= 1;
    while (found == 0) {
	while ((sec_size - searchpos) >= stlen) {
	    if (testchar(secbuf + searchpos, string, stlen))
		searchpos++;
	    else {
		filpos += searchpos;
		found = 1;
		break;
	    }
	}
	if (found == 0) {
	    filpos += searchpos;
	    searchpos = 0;
	}
	if (rdsec() == 0) {
	    found = 1;
	}
	refresh();
    }
    move(2, 0);
    clrtoeol();
}


static void
set()
{
    int     retkey;

    move(2, 0);
    clrtoeol();
    printw("New %s Position (Size is %lx) : ",
	   sec_mode ? "Sector" : "File", fsize);
    refresh();
    move(2, 18);
    strcpy(inp.validchars, "0123456789abcdef");
    inp.valid = HAVEVALID;
    inp.ein[0] = '\0';
    retkey = readstr(&inp);
    if (retkey != VKEY_RETURN)
	return;
    inp.valid = ANYCHAR;
    sscanf(inp.ein, "%llx", &filpos);
    if (sec_mode)
	filpos *= sec_size;
    move(2, 0);
    clrtoeol();
}


static int
command()
{
  int     inval;

  header (VERSION, filename, "(C) 1988 MSS Graz, WSiebeck, Kkaempf");
  inval = 0;
  dump ();			/* wolle */
  while ((inval != 'q') && (inval != 'Q'))
    {
      move(2, 0);
      mvprintw(2, 0, "COMMAND : ");
      refresh();
      inval = getkey();
      switch (inval)
	{
	  case 'q':
	  case 'Q':
	    break;
	  case 'r':
	  case 'R':
	  case KEY_F(2):
	    rawdevice = !rawdevice;
	    werr(rawdevice ? "Raw mode ON" : "Raw mode OFF");
	    break;
	  case KEY_F(1):
	  case '?':
	    help();
	    break;
	  case 'h':
	  case 'H':
	    find_hex();
	    dump();
	    break;
	  case 'f':
	  case 'F':
	    find_string();
	    dump();
	    break;
	  case 'm':
	  case 'M':
	    sec_mode = !sec_mode;
	    break;
	  case '+':
	  case '=':
	  case 'n':
	  case 'N':
	  case KEY_DOWN:
	    if (!rawdevice && (filpos + sec_size > fsize))
	      {
		werr("Beyond EOF !");
		filpos = fsize - sec_size;
		if (filpos < 0)
		    filpos = 0;
		dump();
		break;
	      }
	    filpos += sec_size;
	    dump();
	    break;
	  case '-':
	  case 'p':
	  case 'P':
	  case KEY_UP:
	    filpos -= sec_size;
	    if (filpos < 0)
		filpos = 0;
	    dump();
	    break;
	  case 'D':
	  case 'd':
	    dump();
	    break;
	  case 's':
	  case 'S':
	    set();
	    dump();
	    break;
	  case 'e':
	    if (read_only)
	      werr("Read only !");
	    else
	      edit_ascii();
	    break;
	  case 'E':
	    if (read_only)
	      werr("Read only !");
	    else
	      edit_hex();
	    break;
	  case 'w':
	  case 'W':
	    if (read_only)
	      werr("Read only !");
	    else
	      wrsec();
	    break;
	  default:
	    if (inval)
		werr("Invalid Command !");
	}
    }

}

int
main(argc, argv)
    int argc;
    char *argv[];
{
  FILE *tstfile;
  char *fname = NULL;
  int i;

  if (argc < 2) {
    usage (argv[0]);
  }

  sec_size = MAXSECSIZE;
  view_cols = MAXCOLS;

  i = 1;
  while (i < argc)
    {
      if (argv[i][0] == '-')
	{
	  switch (argv[i][1])
	    {
	      case 'e':
		ebcdic = 1;
		break;
	      case 'r':
		read_only = 1;
		break;
	      case 's':
		sec_size = 256;
		break;
	      case 'v':
		version ();
		break;
	      default:
		usage (argv[0]);
		break;
	    }
	}
      else
	{
	  if (fname)
	    {
	      fprintf (stderr, "Multiple files specified\n");
	      exit (1);
	    }
	  fname = argv[i];
	}
      i++;
    }

  view_rows = sec_size / view_cols;

  if ((tstfile = fopen (fname, "r")) != NULL) {
    fseek (tstfile, 0L, 2);
    fsize = ftell (tstfile);	/* determine file size */
    fclose (tstfile);
  }

  do
    {
      if ((path = open (fname, (read_only?O_RDONLY:O_RDWR))) != -1)
      	break;
      if (read_only == 0)
	{
	  read_only = 1;
	  continue;
	}
      fprintf (stderr, "%s: Can't open '%s'\n", argv[0], fname);
      exit (1);
    }
  while (1);

  sprintf (filename, "%s", fname);
  initscr ();
  refresh ();

  inp.help[0] = '\0';
  inp.valid = ANYCHAR;
  inp.ind = 0;
  inp.maxlen = 20;
  inp.conv2upper = 0;

  signal (SIGINT, SIG_IGN);
  signal (SIGQUIT, SIG_IGN);

  nonl ();
  noecho ();
  crmode ();

  filpos = 0;			/* set global position to 0 */
  length = 0;
  command ();
  echo ();
  nl ();
  nocrmode ();
  move (LINES - 1, 0);
  refresh ();
  endwin ();
  close (path);
}
