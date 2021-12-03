
/* Version curstr.c RELEASE: 1.1 vom 89/11/11     */
static char id[] = "@(#) curstr.c RELEASE: 1.1 vom 89/11/11";


/*
**
**  Supportfunktion zu rutils
**
**  Eingabe von Strings mit Editiermoeglichkeit und Laengenbegrenzung
**
**  Version: 2.1  01.05.89
**
**	Updated: 1.0  10.03.89  siebeck@infoac.rmi.de
**	   	 1.1  21.03.89  siebeck@infoac.rmi.de
**				Erkennung von Funktionstasten
**	    	 2.0  18.04.89  siebeck Endgueltige Versionen fuer
**				Abnahme. (Auch Mighty!)
**		 2.1  01.05.89  kkaempf Amiga keyboard support
**
**  Moeglich sind:
**
**  Cursor links/rechts
**  Cursor Home/End
**  Insert/Overwrite
**
**
**  Es findet Typpruefung statt. Moegliche Typen:
**
**  - ganzzahlige Eingabe
**  - Fliesskomma
**  - Nur Grossbuchstaben (mit Wandlung)
**  - Beliebige Zeichen
**  - verdeckte Eingabe (fuer Passwoerter)
**
**  Beenden der Eingabe durch Return, Pfeil hoch, Pfeil runter, F1 bis F6
**
**  Loeschen der Eingabe mit Esc, bei leerem Eingabefeld wird auch mit
**  Esc beendet.
**
**  Vorgaben koennen als Default uebergeben werden, der Cursor kann zu
**  Beginn an beliebige Position im Default gesetzt werden.
**
**  Die Eingabe findet an der aktuellen Cursorposition auf dem Bildschirm
**  statt.
**
*/


#include <string.h>
#include <ctype.h>

#if !AMIGA
#include <curses.h>
#else
#include <libraries/dos.h>
extern unsigned long Window;
extern int CurX,
        CurY;

#define CSI	0x9b
int     stdscr;			/* dummy argument */

#ifndef memset
#define memset(b,c,l) setmem(b,l,c)
#endif

#define move(y,x) gotoxy(y,x)
#endif

#include "curstr.h"

#if (BSD_42 || CTMFRA || AMIGA || XENIX)

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (1==0)
#endif

#undef MSDOS
#endif

#ifdef __TURBOC__
#define MSDOS
#endif

#define INPUTTEST 0

#if !XENIX
#include <stdio.h>
#endif

#ifdef MSDOS
#include <bios.h>
#include <mem.h>
#include <keyboard.h>
#endif

#if XENIX
#undef void
#define M_TERMINFO
#undef WINDOW
#undef NONSTANDARD
#endif


/**************************************************************************/

static char *
kette(anz, ch)
int     anz;
char    ch;
{
    static char tempst[256];

    memset(tempst, ch, sizeof (tempst));
    tempst[anz] = '\0';
    return tempst;

}

/**************************************************************************/

/*
**  Einfuegen eines Zeichens innerhalb eines Strings
**
**  i   ch      Einzufuegendes Zeichen
**  i   st      Stelle, an der eingefuegt wird
**  io  str     String, in dem eingefuegt wird
**
*/

static void
insert(ch, str, st, maxlen)
char    ch;
char    str[];
int     st;
int     maxlen;

{
    int     i;

    i = maxlen;
    while (i > st) {
	str[i] = str[i - 1];
	i--;
    }
    str[maxlen] = '\0';
    str[st] = ch;

}

/**************************************************************************/


/*
**  Loeschen eines Zeichens innerhalb eines Strings
**
**  i   st      Stelle, an der geloescht wird
**  io  str     String, in dem geloescht wird
**
*/

static void
delete(str, st, maxlen)
char    str[];
int     st;
int     maxlen;

{
    int     i;

    for (i = st; i < maxlen; i++) {
	str[i] = str[i + 1];
    }
    str[maxlen - 1] = FIELDCHAR;
    str[maxlen] = '\0';

}

/**************************************************************************/

static int
char_isval(ch, valid, gueltig)
char    ch;
int     valid;
char   *gueltig;
{
    switch (valid & TYPEMASK) {
      case ANYCHAR:
	return ((ch > 31) && (ch <= 126)) ? TRUE : FALSE;
      case ALPHA:
	return isprint(ch) || (strchr("{|}[]\\", ch) != NULL);
      case FEST:
	return strchr("+-01234567890", ch) != NULL;
      case FLOAT:
	return strchr("+-01234567890.", ch) != NULL;
      case NUMONLY:
	return isdigit(ch);
      case HAVEVALID:
	return strchr(gueltig, ch) != NULL;
      default:
	return FALSE;
    }
}

/**************************************************************************/

static void
chkins(ins)
/*
**	Anzeige, ob derzeit Einfuege- oder Ueberschreibmode aktiv ist
**
**	Anzeige erfolgt in der untersten Zeile rechts
**
*/
int     ins;
{
    if (ins)
	mvaddstr(0, COLS - 6, "EINFG.");
    else {
	standend();
	mvaddstr(0, COLS - 6, "      ");
	standout();
    }
    refresh();

}

/**************************************************************************/

#if UNIX
int
getkey()
/*
**
**  Abholen eines Zeichens vom Keyboard
**
**	Die Zeichen werden umgemapt, um den Gebrauch von Funktionstasten
**	zu ermoeglichen. Die Problematik ist, dass curses meines Wissens
**	nicht ohne weiteres die in curses.h und tinfo.h beschriebenen
**	Tastencodes zurueckgibt.
**
**	getkey bleibt haengen, bis eine Taste gedrueckt wird.
**
**	Rueckgabe:		Tastencode, wenn definiert && erkannt
**				0, wenn Taste nicht erkannt wurde
**
*/

{
    int     ch;

#if (SUN && BSD_42)
    int     i;

#endif

    ch = getch();

    if ((ch == BS) || (ch == 0x7f))
	return 'H' - '@';
    if (ch == 'L' - '@')
	return ch;
    if (ch == 'W' - '@')
	return ch;
    if ((ch == 0x0a) || (ch == 0x0d))
	return CR;
    if ((' ' <= ch) && (ch <= '~'))
	return ch;

#if ((SUN && BSD_42) || (XENIX))
    if ((ch < ' ') && (ch != ESC)) {
	switch (ch) {
	  case 'G' - '@':
	    return KEY_DC;
	  case 'V' - '@':
	    return KEY_IC;
	  case 'Y' - '@':
	    return KEY_IL;
	  case 'A' - '@':
	    return KEY_HOME;
	  case 'E' - '@':
	    return KEY_F(11);	/* ^E == Edit */
	  case 'T' - '@':
	    return KEY_F(12);	/* ^T == Toggle src/dest */
	  case 'R' - '@':
	    return KEY_F(13);	/* ^R == rereaddir */
	  case 'F' - '@':
	    return KEY_F(14);
	  case 'X' - '@':
	    return KEY_F(15);	/* ^X == eXecute cmd */
	  case 'I' - '@':
	    return KEY_F(16);	/* ^I == Exit here */
	  case 'U' - '@':
	    return ESC;		/* ^U == Eing.Loeschen */
	}
    }
#endif

    if (ch != ESC)
	return 0;
    /* ESC ... */
    ch = getch();

    if (ch == ESC)		/* zweimal ESC: ESC zurueckgeben! */
	return ch;

    if ((ch != '[') && (ch != 'O'))
	return 0;		/* invalid key */

    ch = getch();		/* now we have the function */

    switch (ch) {
      case 'A':
	ch = KEY_UP;
	break;
      case 'B':
	ch = KEY_DOWN;
	break;
      case 'C':
	ch = KEY_RIGHT;
	break;
      case 'D':
	ch = KEY_LEFT;
	break;

#if (XENIX && IBM_AT)
      case 'L':
	ch = KEY_IC;
	break;
      case 'F':
	ch = KEY_C1;
	break;
      case 'H':
	ch = KEY_HOME;
	break;
      default:

#ifdef IX386
	if ((ch < 'P') || (ch > 'Y'))
	    return 0;
	ch = KEY_F(0) - 'O' + ch;
#else
	if ((ch < 'M') || (ch > 'V'))
	    return 0;
	ch = KEY_F(0) - 'L' + ch;
#endif

#endif // (XENIX && IBM_AT)

#if CTMFRA
      case 'l':
	ch = KEY_RIGHT;
	break;
      case 'n':
	ch = KEY_DC;
	break;
      case 'p':
	ch = KEY_IC;
	break;
      case 'q':
	ch = KEY_IL;
	break;
      case 'r':
	ch = KEY_DOWN;
	break;
      case 't':
	ch = KEY_LEFT;
	break;
      case 'w':
	ch = KEY_HOME;
	break;
      case 'x':
	ch = KEY_UP;
	break;
      default:
	if ((ch < 'Q') || (ch > 'Z'))
	    ch = 0;
	else
	    ch = KEY_F(ch - (int) 'P');
#endif // CTMFRA

#if (SUN && BSD_42)
      default:
	if (!isdigit(ch)) {
	    ch = 0;
	    break;
	}
	i = 0;
	while (ch != 'z') {
	    i = i * 10 + (ch - '0');
	    ch = getch();
	}
	switch (i) {
	  case 225:
	    ch = KEY_F(2);
	    break;
	  case 226:
	    ch = KEY_F(3);
	    break;
	  case 227:
	    ch = KEY_F(4);
	    break;
	  case 228:
	    ch = KEY_F(5);
	    break;
	  case 229:
	    ch = KEY_F(6);
	    break;
	  case 230:
	    ch = KEY_F(7);
	    break;
	  case 231:
	    ch = KEY_F(8);
	    break;
	  case 232:
	    ch = KEY_F(9);
	    break;
	  case 214:
	    ch = KEY_IC;
	    break;
	  case 216:
	    ch = KEY_DC;
	    break;
	  case 218:
	    ch = KEY_HOME;
	    break;
	  case 220:
	    ch = KEY_F(1);
	    break;
	  case 222:
	    ch = KEY_F(10);
	    break;
	  default:
	    ch = 0;
	    break;
	}
#endif
    }				/* switch */

    return ch;

}

#endif				/* UNIX */

#ifdef MSDOS
int
getkey(void)
/* Uses the BIOS to read the next keyboard character */
{
    int     key,
            lo,
            hi;

    for (;;) {
	key = bioskey(1);	/* get keyboard status */
	if (key)
	    break;
    }
    key = bioskey(0);
    lo = key & 0X00FF;
    hi = (key & 0XFF00) >> 8;
    if (lo != 0) {
	return lo;
    }
    switch (hi + 256) {
      case F1:
	return KEY_F(1);
      case F2:
	return KEY_F(2);
      case F3:
	return KEY_F(3);
      case F4:
	return KEY_F(4);
      case F5:
	return KEY_F(5);
      case F6:
	return KEY_F(6);
      case F7:
	return KEY_F(7);
      case F8:
	return KEY_F(8);
      case F9:
	return KEY_F(9);
      case F10:
	return KEY_F(10);
      case INSKEY:
	return KEY_IC;
      case DELKEY:
	return KEY_DC;
      case UPKEY:
	return KEY_UP;
      case DOWNKEY:
	return KEY_DOWN;
      case LEFTKEY:
	return KEY_LEFT;
      case RIGHTKEY:
	return KEY_RIGHT;
      case HOMEKEY:
	return KEY_HOME;
      case ENDKEY:
	return KEY_IL;
    }
    return 0;
}				/* getkey */

#endif

#if AMIGA
int
getkey()
{

    unsigned char ibuf[2];
    int     val;
    unsigned char last;

    if (Read(Window, ibuf, 1L) <= 0L)
	return 0;

    switch (ibuf[0]) {
      case CSI:
	break;
      case 0x08:
	return KEY_BACKSPACE;
      case 0x1b:
	return KEY_CLEAR;	/* ?? */
      case 0x7f:
	return KEY_DC;
      default:
	return (int) ibuf[0];
    }

    val = 0;
    last = '\0';

    for (;;) {

	last = ibuf[0];

	if (Read(Window, ibuf, 1L) <= 0L)
	    return 0;

	switch (ibuf[0]) {

	  case '@':
	    if (last == ' ')
		return KEY_RIGHT;	/* S */
	    break;
	  case 'A':
	    if (last == ' ')
		return KEY_LEFT;/* S */
	    else
		return KEY_UP;
	    break;
	  case 'B':
	    return KEY_DOWN;
	    break;
	  case 'C':
	    return KEY_RIGHT;
	    break;
	  case 'D':
	    return KEY_LEFT;
	    break;
	  case 'T':
	    return KEY_UP;	/* S */
	    break;
	  case 'S':
	    return KEY_DOWN;	/* S */
	    break;
	  case '~':
	    if (last == '?')
		return KEY_IC;	/* ?? */
	    else {
		if (val < 10)
		    return KEY_F0 + val + 1;
		else
		    return KEY_F0 + val - 9;	/* S */
	    }
	    break;
	  case ' ':
	  case '?':
	    break;
	  default:
	    if (isdigit(ibuf[0])) {
		val *= 10;
		val += (ibuf[0] - '0');
		continue;
	    }
	}
    }
}

#endif

/**************************************************************************/

/*
**
**  Eingabe eines Strings an der aktuellen Cursorposition
**
**	Beschreibung siehe Fileheader
**
*/

int
readstr(inp)
struct INPUTSTR *inp;
{
    int     ch,
            curpos,
            einlen,
            x,
            y,
            hidden = FALSE,
            done = FALSE,
            ins = FALSE,
            returnkey = 0;
    static char eingabe[MAXFIELDLEN + 1],	/* keep off stack ! */
            fieldstr[MAXFIELDLEN + 1];

#if UNIX_AND_STUPID
    nonl();
    noecho();			/* turn echo off, we do our own */
    crmode();
#endif

#if !AMIGA
    getyx(stdscr, y, x);
#else
    x = CurX;
    y = CurY;
#endif

    strcpy(fieldstr, kette(MAXFIELDLEN, FIELDCHAR));
    strcpy(eingabe, inp->ein);
    einlen = strlen(eingabe);
    if (inp->maxlen > MAXFIELDLEN)
	inp->maxlen = MAXFIELDLEN;
    strncat(eingabe, fieldstr,
	    inp->maxlen - strlen(eingabe));	/* Append fieldmarkers */
    curpos = inp->ind;
    hidden = ((inp->valid & HIDDEN) == HIDDEN);
    chkins(ins);
    wstandout(stdscr);

#if CTMFRA
    if (!(hidden))
	inp->valid = ANYCHAR;
#endif

    while (!done) {
	if (hidden) {
	    mvaddstr(y, x, kette(einlen, HIDECHAR));
	    mvaddstr(y, x + einlen, kette(inp->maxlen - einlen, FIELDCHAR));
	} else {
	    mvaddstr(y, x, eingabe);	/* show default */
	}
	move(y, x + curpos);
	refresh();
	while ((ch = getkey()) == 0);	/* FIXME */
	if ((ch < 128) && isalpha(ch) && inp->conv2upper) {
	    if (islower(ch))
		ch = toupper(ch);
	}
	switch (ch) {
	  case KEY_IC:
	    ins = !ins;
	    chkins(ins);
	    break;
	  case KEY_BACKSPACE:
	  case BS:
	  case KEY_LEFT:
	    if (curpos == 0)
		break;
	    curpos--;
	    if (ch == KEY_LEFT)	/* Fallthru fuer BS */
		break;
	  case KEY_DC:
	    if (curpos < einlen) {
		delete(eingabe, curpos, inp->maxlen);
		einlen--;
	    }
	    break;
	  case KEY_RIGHT:
	    if (curpos < einlen)
		curpos++;
	    break;
	  case KEY_HOME:
	    curpos = 0;
	    break;
	  case KEY_IL:
	    curpos = einlen;
	    break;
	  case KEY_UP:
	    returnkey = VKEY_UP;
	    done = TRUE;
	    break;
	  case KEY_DOWN:
	    returnkey = VKEY_DOWN;
	    done = TRUE;
	    break;
	  case ESC:
	    if (einlen > 0) {
		eingabe[0] = '\0';
		strcpy(eingabe, kette(inp->maxlen, FIELDCHAR));
		einlen = curpos = 0;
	    } else {
		returnkey = ESC;
		done = TRUE;
	    }
	    break;
	  case CR:
	    returnkey = VKEY_RETURN;
	    done = TRUE;
	    break;
	  case KEY_F(1):
	    returnkey = VKEY_F1;
	    done = TRUE;
	    break;
	  case KEY_F(2):
	    returnkey = VKEY_F2;
	    done = TRUE;
	    break;
	  case KEY_F(3):
	    returnkey = VKEY_F3;
	    done = TRUE;
	    break;
	  case KEY_F(4):
	    returnkey = VKEY_F4;
	    done = TRUE;
	    break;
	  case KEY_F(5):
	    returnkey = VKEY_F5;
	    done = TRUE;
	    break;
	  case KEY_F(6):
	    returnkey = VKEY_F6;
	    done = TRUE;
	    break;
	  case KEY_F(7):
	    returnkey = VKEY_F7;
	    done = TRUE;
	    break;
	  case KEY_F(8):
	    returnkey = VKEY_F8;
	    done = TRUE;
	    break;
	  case KEY_F(9):
	    returnkey = VKEY_F9;
	    done = TRUE;
	    break;
	  case KEY_F(10):
	    returnkey = VKEY_F10;
	    done = TRUE;
	    break;
	  default:
	    if ((ch <= 255) && (char_isval(ch, inp->valid, inp->validchars))) {
		if (ins) {
		    if (einlen < inp->maxlen) {
			insert((char) ch, eingabe, curpos, inp->maxlen);
			curpos++;
			einlen++;
		    }
		} else {
		    if (curpos < inp->maxlen) {
			eingabe[curpos] = (char) ch;
			curpos++;
			if (curpos > einlen)
			    einlen++;
		    }
		}
	    }
	}			/* switch */

    }				/* !done */

    wstandend(stdscr);
    mvaddstr(y, x, kette(inp->maxlen, ' '));
    eingabe[einlen] = '\0';
    strcpy(inp->ein, eingabe);
    if (hidden)
	mvaddstr(y, x, kette(einlen, HIDECHAR));
    else
	mvaddstr(y, x, eingabe);

    refresh();

#if UNIX_AND_STUPID
    echo();
    nl();
    nocrmode();
#endif

    return returnkey;
}



#if INPUTTEST
main()
{
    struct INPUTSTR inp;
    int     i,
            x = 5,
            y = 5;

    strcpy(inp.ein, "default blah");
    inp.help[0] = '\0';
    inp.maxlen = 30;
    inp.valid = ANYCHAR;
    inp.ind = 0;
    initscr();
    clear();
    refresh();
    noecho();
    for (;;) {
	move(y, x);
	i = readstr(&inp);
	y++;
	x++;
	move(20, 1);
	wprintw(stdscr, "(%d/%d)>%s<", i, strlen(inp.ein), inp.ein);
	clrtoeol();
	if (i == 27)
	    break;
	refresh();
    }
    echo();
    endwin();
}

#endif
