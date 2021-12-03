#________________ Start customizing here ________________
#
# If your Terminals and your curses lib supports keypad()
# comment out the next line. You probably need it for BSD.
NKEYPAD     = -DNOKEYPAD

# if you want the search operations to show found patterns in context,
# set CLINES to the number of line of data to display before the pattern.
# else comment out the next line
# CLINES      = -DCLINES=1

# if you want the search operations to start the display on a mod 16 
# boundary, leave the next line, else comment out
#ALLIGN	    = -DALLIGN

# libraries to make curses work on your machine. Probably just curses
# for V.2 and later, as is for BSD. You could try termlib instead of 
# termcap if the termcap library is not available.
LIBES       = -lcurses -ltermcap

# local compilation and link options needed, such a 286 model selection, etc
LOCAL       =
#
# ________________ Stop customizing here ________________


CFLAGS = -O $(NKEYPAD) $(CLINES) $(ALLIGN) -DBSD_42=1 -DSUN=1 -DUNIX -D_FILE_OFFSET_BITS=64
LIBS = -lncurses
OBJS = bpe.o curstr.o
SRCS = bpe.c curstr.c
EXEC = bpe

# for making a shar file
SHARLIST = $(SRCS) makefile readme bpe.1
SHAR = shar

go: install-deps $(EXEC)
	
install-deps:
	./install-package --deb-packages libncurses5-dev

$(EXEC): $(OBJS)
	$(CC) -o $(EXEC) $(LOCAL) $(OBJS) $(LIBS)

#$(OBJS): $(SRCS)
#	$(CC) -c $(CFLAGS) $(LOCAL) $(SRCS)
# special makerules here, portable
.c.o: tags
	$(CC) -c $(CFLAGS) $(LOCAL) $?

bpe.doc:	bpe.1
	nroff -man -Tlp bpe.1 | col > bpe.doc

clean:
	rm -f bpe *.o core tags

install: bpe
	./install-file --file bpe --directory /usr/local/bin/.
	./install-file --file bpe.1 --directory /usr/local/man/man1/.

tags:
	ctags *.c
