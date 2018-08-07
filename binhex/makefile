CFLAGS = -O $(CF)

SRCS  =	binhex.c dofile.c

OBJS  =	binhex.o dofile.o

LIB =	../crc/libcrc.a
TNAME =	../util/transname
BNAME = ../util/backtrans
UNAME =	../util/util
INAME = ../fileio/rdfile
GNAME =	../fileio/fileglob
XOBJS =	$(TNAME).o $(BNAME).o $(UNAME).o $(INAME).o $(GNAME).o
XSRCS =	$(TNAME).c $(BNAME).c $(UNAME).c $(INAME).c $(GNAME).c

binhex:	$(OBJS) $(XOBJS) $(LIB)
	$(CC) $(CFLAGS) -o binhex $(OBJS) $(XOBJS) $(LIB)

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

$(GNAME).o:	$(GNAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

lint:
	lint $(CF) $(LFLAGS) $(SRCS) $(XSRCS)

clean:
	-rm -f *.o

clobber:clean
	-rm -f binhex

binhex.o:	../fileio/machdr.h
binhex.o:	../fileio/rdfile.h
binhex.o:	../util/patchlevel.h
dofile.o:	../fileio/machdr.h
dofile.o:	../fileio/rdfile.h

