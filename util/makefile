CFLAGS=	-O $(CF)

all:	util.o transname.o backtrans.o
	touch all

util.o:	util.c

transname.o:	transname.c

backtrans.o:	backtrans.c

clean:
	-rm -f util.o
	-rm -f transname.o
	-rm -f backtrans.o
	-rm -f all

util.o:	../fileio/fileglob.h
util.o:	masks.h
util.o:	util.h
backtrans.o:	masks.h

