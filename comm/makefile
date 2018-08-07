CFLAGS = -O $(CF)

SRCS1 =	tomac.c xm_to.c ym_to.c zm_to.c tty.c globals.c
SRCS2 =	frommac.c xm_from.c ym_from.c zm_from.c tty.c globals.c

OBJS1 =	tomac.o xm_to.o ym_to.o zm_to.o tty.o globals.o
OBJS2 =	frommac.o xm_from.o ym_from.o zm_from.o tty.o globals.o

LIB =	../crc/libcrc.a
TNAME =	../util/transname
BNAME = ../util/backtrans
UNAME =	../util/util
INAME = ../fileio/rdfile
ONAME = ../fileio/wrfile
GNAME =	../fileio/fileglob
XOBJS1 =	$(TNAME).o $(BNAME).o $(UNAME).o $(INAME).o $(GNAME).o
XOBJS2 =	$(TNAME).o $(UNAME).o $(ONAME).o $(GNAME).o
XSRCS1 =	$(TNAME).c $(BNAME).c $(UNAME).c $(INAME).c $(GNAME).c
XSRCS2 =	$(TNAME).c $(UNAME).c $(ONAME).c $(GNAME).c

all:	tomac frommac
	touch all

tomac:	$(OBJS1) $(XOBJS1)
	$(CC) $(CFLAGS) -o tomac $(OBJS1) $(XOBJS1)

frommac:	$(OBJS2) $(XOBJS2)
	$(CC) $(CFLAGS) -o frommac $(OBJS2) $(XOBJS2)

$(LIB):	../crc/makecrc.c
	(cd ../crc; make CC=$(CC) CF="$(CF)" )

$(TNAME).o:	$(TNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(BNAME).o:	$(BNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(UNAME).o:	$(UNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(INAME).o:	$(INAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

$(ONAME).o:	$(ONAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

$(GNAME).o:	$(GNAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

lint:
	lint $(CF) $(LFLAGS) $(SRCS1) $(XSRCS)

clean:
	-rm -f *.o

clobber:clean
	-rm -f all tomac frommac

tomac.o:	comm.h
tomac.o:	../fileio/machdr.h
tomac.o:	../fileio/rdfile.h
tomac.o:	../util/patchlevel.h
tomac.o:	globals.h
xm_to.o:	comm.h
xm_to.o:	../fileio/machdr.h
xm_to.o:	../fileio/rdfile.h
xm_to.o:	../util/masks.h
xm_to.o:	globals.h
xm_to.o:	protocol.h
ym_to.o:	comm.h
zm_to.o:	comm.h
frommac.o:	comm.h
frommac.o:	../util/patchlevel.h
frommac.o:	../fileio/machdr.h
frommac.o:	globals.h
frommac.o:	../fileio/fileglob.h
frommac.o:	../fileio/wrfile.h
xm_from.o:	comm.h
xm_from.o:	../fileio/machdr.h
xm_from.o:	../fileio/wrfile.h
xm_from.o:	../util/masks.h
xm_from.o:	globals.h
xm_from.o:	protocol.h
ym_from.o:	comm.h
zm_from.o:	comm.h
globals.o:	globals.h
tty.o:	../util/masks.h
tty.o:	protocol.h
tty.o:	globals.h

