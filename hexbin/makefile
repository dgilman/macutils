CFLAGS=	-O $(CF)

SRCS =	hexbin.c \
	dl.c \
	hecx.c \
	hqx.c \
	mu.c \
	buffer.c \
	crc.c \
	readline.c \
	printhdr.c \
	globals.c

OBJS =	hexbin.o \
	dl.o \
	hecx.o \
	hqx.o \
	mu.o \
	buffer.o \
	crc.o \
	readline.o \
	printhdr.o \
	globals.o

LIB =	../crc/libcrc.a
TNAME =	../util/transname
BNAME =	../util/backtrans
UNAME =	../util/util
ONAME =	../fileio/wrfile
GNAME =	../fileio/fileglob
XOBJS =	$(TNAME).o $(BNAME).o $(UNAME).o $(ONAME).o $(GNAME).o
XSRCS =	$(TNAME).c $(BNAME).c $(UNAME).c $(ONAME).c $(GNAME).c
CRCS =	../crc/binhex.c

hexbin:	$(OBJS) $(LIB) $(XOBJS)
	$(CC) $(CFLAGS) -o hexbin $(OBJS) $(XOBJS) $(LIB)

$(LIB):	../crc/makecrc.c
	(cd ../crc; make CC=$(CC) CF="$(CF)" )

$(TNAME).o:	$(TNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(BNAME).o:	$(BNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(UNAME).o:	$(UNAME).c
	(cd ../util; make CC=$(CC) CF="$(CF)" )

$(ONAME).o:	$(ONAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

$(GNAME).o:	$(GNAME).c
	(cd ../fileio; make CC=$(CC) CF="$(CF)" )

lint:
	lint $(CF) $(LFLAGS) $(SRCS) $(XSRCS) $(CRCS)

clean:
	-rm -f *.o

clobber:clean
	-rm -f hexbin

hexbin.o:	globals.h
hexbin.o:	crc.h
hexbin.o:	readline.h
hexbin.o:	../util/masks.h
hexbin.o:	../util/util.h
hexbin.o:	../util/patchlevel.h
hexbin.o:	../fileio/wrfile.h
hexbin.o:	../fileio/wrfileopt.h
hexbin.o:	../fileio/machdr.h
hexbin.o:	../fileio/kind.h
hexbin.o:	../util/curtime.h
hexbin.o:	hexbin.h
dl.o:	hexbin.h
dl.o:	globals.h
dl.o:	crc.h
dl.o:	readline.h
dl.o:	../fileio/machdr.h
dl.o:	../fileio/wrfile.h
dl.o:	../util/util.h
dl.o:	buffer.h
dl.o:	printhdr.h
hecx.o:	hexbin.h
hecx.o:	globals.h
hecx.o:	crc.h
hecx.o:	readline.h
hecx.o:	../util/masks.h
hecx.o:	../util/util.h
hecx.o:	../fileio/machdr.h
hecx.o:	../fileio/wrfile.h
hecx.o:	buffer.h
hecx.o:	printhdr.h
hqx.o:	hexbin.h
hqx.o:	globals.h
hqx.o:	readline.h
hqx.o:	crc.h
hqx.o:	buffer.h
hqx.o:	../fileio/machdr.h
hqx.o:	../fileio/wrfile.h
hqx.o:	../util/util.h
hqx.o:	printhdr.h
mu.o:	hexbin.h
mu.o:	globals.h
mu.o:	readline.h
mu.o:	../util/masks.h
mu.o:	../util/util.h
mu.o:	../fileio/machdr.h
mu.o:	../fileio/wrfile.h
mu.o:	buffer.h
mu.o:	printhdr.h
buffer.o:	globals.h
buffer.o:	../util/util.h
buffer.o:	buffer.h
buffer.o:	../fileio/wrfile.h
crc.o:	hexbin.h
crc.o:	crc.h
crc.o:	../util/masks.h
crc.o:	globals.h
readline.o:	readline.h
readline.o:	globals.h
printhdr.o:	printhdr.h
printhdr.o:	globals.h
globals.o:	globals.h
globals.o:	../fileio/machdr.h
globals.o:	../fileio/wrfile.h
globals.o:	../fileio/kind.h

