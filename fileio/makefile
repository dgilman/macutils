CFLAGS=	-O $(CF)

all:	wrfile.o rdfile.o fileglob.o
	touch all

wrfile.o:	wrfile.c

rdfile.o:	rdfile.c

clean:
	-rm -f wrfile.o
	-rm -f rdfile.o
	-rm -f fileglob.o
	-rm -f all

wrfile.o:	machdr.h
wrfile.o:	wrfile.h
wrfile.o:	wrfileopt.h
wrfile.o:	fileglob.h
wrfile.o:	aufs.h
wrfile.o:	appledouble.h
wrfile.o:	../util/util.h
wrfile.o:	../util/curtime.h
rdfile.o:	machdr.h
rdfile.o:	rdfile.h
rdfile.o:	rdfileopt.h
rdfile.o:	../util/util.h
rdfile.o:	../util/curtime.h
rdfile.o:	../util/masks.h
rdfile.o:	aufs.h
rdfile.o:	appledouble.h
fileglob.o:	fileglob.h

