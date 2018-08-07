CFLAGS = -O $(CF)

SRCS1 =	macsave.c \
	globals.c \
	macbinary.c \
	dir.c \
	mcb.c

SRCS2 =	macstream.c

OBJS1 =	macsave.o \
	globals.o \
	macbinary.o \
	dir.o \
	mcb.o

OBJS2 =	macstream.o

TNAME =	../util/transname
BNAME = ../util/backtrans
UNAME =	../util/util
ONAME =	../fileio/wrfile
INAME = ../fileio/rdfile
GNAME =	../fileio/fileglob
XOBJS1=	$(TNAME).o $(UNAME).o $(ONAME).o $(GNAME).o
XSRCS1=	$(TNAME).c $(UNAME).c $(ONAME).c $(GNAME).c
XOBJS2=	$(TNAME).o $(BNAME).o $(UNAME).o $(INAME).o $(GNAME).o
XSRCS2=	$(TNAME).c $(BNAME).c $(UNAME).c $(INAME).c $(GNAME).c

all:	macsave macstream
	touch all

macsave:	$(OBJS1) $(XOBJS1)
	$(CC) $(CFLAGS) -o macsave $(OBJS1) $(XOBJS1)

macstream:	$(OBJS2) $(XOBJS2)
	$(CC) $(CFLAGS) -o macstream $(OBJS2) $(XOBJS2)

$(TNAME).o:	$(TNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(BNAME).o:	$(BNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(UNAME).o:	$(UNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(ONAME).o:	$(ONAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

$(INAME).o:	$(INAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

$(GNAME).o:	$(GNAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

lint:
	lint $(CF) $(LFLAGS) $(SRCS1) $(XSRCS1)
	lint $(CF) $(LFLAGS) $(SRCS2) $(XSRCS2)

clean:
	-rm -f *.o

clobber:clean
	-rm -f all macsave macstream

macsave.o:	globals.h
macsave.o:	../util/patchlevel.h
macsave.o:	../fileio/wrfile.h
macsave.o:	../fileio/wrfileopt.h
macsave.o:	../util/util.h
globals.o:	globals.h
globals.o:	../fileio/machdr.h
macbinary.o:	globals.h
macbinary.o:	../fileio/machdr.h
macbinary.o:	../fileio/kind.h
macbinary.o:	../util/util.h
dir.o:	globals.h
dir.o:	../fileio/machdr.h
dir.o:	../fileio/wrfile.h
dir.o:	../util/util.h
dir.o:	../util/masks.h
mcb.o:	globals.h
mcb.o:	../fileio/machdr.h
mcb.o:	../fileio/wrfile.h
mcb.o:	../util/masks.h
mcb.o:	../util/util.h
macstream.o:	../fileio/machdr.h
macstream.o:	../fileio/rdfile.h
macstream.o:	../fileio/rdfileopt.h
macstream.o:	../util/patchlevel.h

