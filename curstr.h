/*
** curstr.h
**
** Include for curses based keyboard handling
**
** Created:		  ??.01.89  siebeck
**
** Version:	2.00  01.05.89
**
** Updated: 2.00  01.05.89  kkaempf, full amiga keyboard support
**
*/

#ifdef BSD
#define SUN 1
#define UNIX 1
#define BSD_42 1
#define XENIX 0
#undef MSDOS
#define AMIGA 0
#endif

#ifdef SYSV
#define SUN 0
#define UNIX 1
#define BSD_42 0
#define XENIX 1
#undef MSDOS
#define AMIGA 0
#endif

#ifdef IX386
#define SUN 0
#define UNIX 1
#define BSD_42 0
#define XENIX 1
#undef MSDOS
#define AMIGA 0
#define IBM_AT 1
#endif

#ifdef SCOXENIX
#define SUN 0
#define UNIX 1
#define BSD_42 0
#define XENIX 1
#define IBM_AT 1
#endif

#ifdef __TURBOC__
#define SUN 0
#define UNIX 0
#define BSD_42 0
#define XENIX 0
#undef MSDOS
#define MSDOS 1
#define AMIGA 0
#endif

#define MAXFIELDLEN 80
#define MAXVALIDLEN 26

#ifndef HBEGLEN
#define HBEGLEN 20
#endif

#ifdef HELP

#ifndef HELPFILENAME
#define HELPFILENAME "BENHUR.HLP"
#endif

#endif

#define ANYCHAR 0
#define ALPHA 1
#define FEST 2
#define FLOAT 4
#define NUMONLY 8
#define HAVEVALID 16
#define TYPEMASK 255
#define HIDDEN 256
#define MULSEL 512
#define FIELDCHAR '_'
#define HIDECHAR '*'

/* diverse keycodes */
#define CR	0x0d
#define	LF	0x0a
#define	ESC	0x1b
#define BELL	0x07
#define	BS	0x08



struct INPUTSTR {
    char    ein[MAXFIELDLEN + 1];	/* Buffer fuer die Eingabe */
    char    help[HBEGLEN + 1];	/* FIXME */
    char    validchars[MAXVALIDLEN + 1];	/* Liste der gueltigen Zeichen */
    int     ind;		/* An welcher Stelle steht der Cursor ? */
    int     valid;		/* Welche Zeichen sind gueltig ? */
    int     maxlen;		/* Maximale Laenge der Eingabe */
    int     conv2upper;		/* Soll in GROSSBUCHSTABEN gew. werden? */
};

#ifndef KEY_BREAK
#define KEY_BREAK	0401	/* break key (unreliable) */
#define KEY_DOWN	0402	/* The four arrow keys ... */
#define KEY_UP		0403
#define KEY_LEFT	0404
#define KEY_RIGHT	0405	/* ... */
#define KEY_HOME	0406	/* Home key (upward+left arrow) */
#define KEY_BACKSPACE	0407	/* backspace (unreliable) */
#define KEY_F0		0410	/* Function keys.  Space for 64 */
#define KEY_F(n)	(KEY_F0+(n))	/* keys is reserved. */
#define KEY_DL		0510	/* Delete line */
#define KEY_IL		0511	/* Insert line */
#define KEY_DC		0512	/* Delete character */
#define KEY_IC		0513	/* Insert char or enter insert mode */
#define KEY_EIC		0514	/* Exit insert char mode */
#define KEY_CLEAR	0515	/* Clear screen */
#define KEY_EOS		0516	/* Clear to end of screen */
#define KEY_EOL		0517	/* Clear to end of line */
#define KEY_SF		0520	/* Scroll 1 line forward */
#define KEY_SR		0521	/* Scroll 1 line backwards (reverse) */
#define KEY_NPAGE	0522	/* Next page */
#define KEY_PPAGE	0523	/* Previous page */
#define KEY_STAB	0524	/* Set tab */
#define KEY_CTAB	0525	/* Clear tab */
#define KEY_CATAB	0526	/* Clear all tabs */
#define KEY_ENTER	0527	/* Enter or send (unreliable) */
#define KEY_SRESET	0530	/* soft (partial) reset (unreliable) */
#define KEY_RESET	0531	/* reset or hard reset (unreliable) */
#define KEY_PRINT	0532	/* print or copy */
#define KEY_LL		0533	/* home down or bottom (lower left) */
 /* The keypad is arranged like this: */
 /* a1    up    a3   */
 /* left   b2  right  */
 /* c1   down   c3   */
#define KEY_A1		0534	/* upper left of keypad */
#define KEY_A3		0535	/* upper right of keypad */
#define KEY_B2		0536	/* center of keypad */
#define KEY_C1		0537	/* lower left of keypad */
#define KEY_C3		0540	/* lower right of keypad */
#endif				/* KEY_BREAK */


/*
**	Prototypes
*/

#ifdef __TURBOC__
int     readstr(struct INPUTSTR * inp);
void    errordsp(char *s);
int     menu(char *mstr[]);
int     getkey(void);
#else
int     readstr();
void    errordsp();
int     menu();
int     getkey();

#endif

#define VKEY_RETURN	0
#define VKEY_F1		1
#define VKEY_F2		2
#define VKEY_F3		3
#define VKEY_F4		4
#define VKEY_F5		5
#define VKEY_F6		6
#define VKEY_F7		7
#define VKEY_F8		8
#define VKEY_F9		9
#define VKEY_F10	10
#define VKEY_UP		11
#define VKEY_DOWN	12


/* EOF */
