CFLAGS = -O $(CF)

SRCS =	macunpack.c \
	globals.c \
	macbinary.c \
	dir.c \
	mcb.c \
	bin.c \
	jdw.c \
	stf.c \
	lzc.c \
	pit.c \
	sit.c \
	dia.c \
	cpt.c \
	zma.c \
	lzh.c \
	dd.c \
	de_huffman.c \
	de_compress.c \
	de_lzah.c \
	de_lzh.c \
	crc.c \
	bits_be.c

OBJS =	macunpack.o \
	globals.o \
	macbinary.o \
	dir.o \
	mcb.o \
	bin.o \
	jdw.o \
	stf.o \
	lzc.o \
	pit.o \
	sit.o \
	dia.o \
	cpt.o \
	zma.o \
	lzh.o \
	dd.o \
	de_huffman.o \
	de_compress.o \
	de_lzah.o \
	de_lzh.o \
	crc.o \
	bits_be.o

LIB =	../crc/libcrc.a
TNAME =	../util/transname
UNAME =	../util/util
ONAME =	../fileio/wrfile
GNAME =	../fileio/fileglob
XOBJS =	$(TNAME).o $(UNAME).o $(ONAME).o $(GNAME).o
XSRCS =	$(TNAME).c $(UNAME).c $(ONAME).c $(GNAME).c
CRCS =	../crc/arc.c ../crc/binhex.c ../crc/zip.c

macunpack:	$(OBJS) $(LIB) $(XOBJS)
	$(CC) $(CFLAGS) -o macunpack $(OBJS) $(XOBJS) $(LIB)

$(LIB):	../crc/makecrc.c
	(cd ../crc; make CC=$(CC) CF="$(CF)" )

$(TNAME).o:	$(TNAME).c
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
	-rm -f macunpack

macunpack.o:	macunpack.h
macunpack.o:	globals.h
macunpack.o:	../util/patchlevel.h
macunpack.o:	../fileio/wrfile.h
macunpack.o:	../fileio/wrfileopt.h
macunpack.o:	../fileio/kind.h
macunpack.o:	../util/util.h
globals.o:	globals.h
globals.o:	../fileio/machdr.h
globals.o:	../fileio/wrfile.h
globals.o:	../fileio/kind.h
macbinary.o:	macunpack.h
macbinary.o:	globals.h
macbinary.o:	zmahdr.h
macbinary.o:	../fileio/machdr.h
macbinary.o:	../fileio/wrfile.h
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
bin.o:	macunpack.h
bin.o:	globals.h
bin.o:	../fileio/machdr.h
bin.o:	../fileio/wrfile.h
bin.o:	../fileio/kind.h
bin.o:	../util/util.h
bin.o:	../util/masks.h
jdw.o:	macunpack.h
jdw.o:	jdw.h
jdw.o:	globals.h
jdw.o:	huffman.h
jdw.o:	../fileio/wrfile.h
jdw.o:	../fileio/machdr.h
jdw.o:	../util/util.h
jdw.o:	../util/masks.h
stf.o:	macunpack.h
stf.o:	stf.h
stf.o:	globals.h
stf.o:	huffman.h
stf.o:	../util/curtime.h
stf.o:	../fileio/wrfile.h
stf.o:	../fileio/machdr.h
stf.o:	../util/util.h
lzc.o:	macunpack.h
lzc.o:	globals.h
lzc.o:	lzc.h
lzc.o:	../util/util.h
lzc.o:	../fileio/machdr.h
lzc.o:	../fileio/wrfile.h
lzc.o:	../util/masks.h
pit.o:	macunpack.h
pit.o:	../fileio/fileglob.h
pit.o:	../fileio/wrfile.h
pit.o:	../fileio/kind.h
pit.o:	globals.h
pit.o:	pit.h
pit.o:	../fileio/machdr.h
pit.o:	crc.h
pit.o:	../util/masks.h
pit.o:	../util/util.h
pit.o:	huffman.h
sit.o:	macunpack.h
sit.o:	globals.h
sit.o:	sit.h
sit.o:	crc.h
sit.o:	../util/util.h
sit.o:	../fileio/machdr.h
sit.o:	../fileio/wrfile.h
sit.o:	../fileio/kind.h
sit.o:	../util/masks.h
sit.o:	huffman.h
dia.o:	macunpack.h
dia.o:	globals.h
dia.o:	dia.h
dia.o:	../util/curtime.h
dia.o:	../util/masks.h
dia.o:	../fileio/machdr.h
dia.o:	../fileio/wrfile.h
dia.o:	../fileio/kind.h
dia.o:	../util/util.h
cpt.o:	macunpack.h
cpt.o:	globals.h
cpt.o:	cpt.h
cpt.o:	crc.h
cpt.o:	../util/util.h
cpt.o:	../fileio/machdr.h
cpt.o:	../fileio/wrfile.h
cpt.o:	../fileio/kind.h
cpt.o:	../util/masks.h
cpt.o:	huffman.h
zma.o:	macunpack.h
zma.o:	globals.h
zma.o:	zma.h
zma.o:	crc.h
zma.o:	../fileio/machdr.h
zma.o:	../fileio/wrfile.h
zma.o:	../fileio/kind.h
zma.o:	../util/masks.h
zma.o:	../util/util.h
lzh.o:	macunpack.h
lzh.o:	globals.h
lzh.o:	lzh.h
lzh.o:	crc.h
lzh.o:	../fileio/wrfile.h
lzh.o:	../fileio/machdr.h
lzh.o:	../util/masks.h
lzh.o:	../util/util.h
lzh.o:	bits_be.h
dd.o:	macunpack.h
dd.o:	globals.h
dd.o:	dd.h
dd.o:	crc.h
dd.o:	../fileio/machdr.h
dd.o:	../fileio/wrfile.h
dd.o:	../fileio/fileglob.h
dd.o:	../util/masks.h
dd.o:	../util/util.h
de_huffman.o:	macunpack.h
de_huffman.o:	globals.h
de_huffman.o:	../util/masks.h
de_huffman.o:	huffman.h
de_huffman.o:	../fileio/wrfile.h
de_huffman.o:	../util/util.h
de_compress.o:	macunpack.h
de_compress.o:	globals.h
de_compress.o:	../fileio/wrfile.h
de_lzah.o:	macunpack.h
de_lzah.o:	globals.h
de_lzah.o:	../util/masks.h
de_lzah.o:	../fileio/wrfile.h
de_lzh.o:	macunpack.h
de_lzh.o:	globals.h
de_lzh.o:	../util/masks.h
de_lzh.o:	../fileio/wrfile.h
de_lzh.o:	bits_be.h
bits_be.o:	../util/masks.h
bits_be.o:	bits_be.h

