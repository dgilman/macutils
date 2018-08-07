SHELL =	/bin/sh
BINDIR =	/ufs/dik/tmpbin
# Use the following flags on the CF macro definition as needed.
#
# -DBSD if you are on a BSD system
#
# -DTYPES_H if your system has /usr/include/sys/types.h
#
# -DDIRENT_H if your system has /usr/include/dirent.h
#
# -DTERMIOS_H if your system has /usr/include/sys/termios.h
#
# -DNODOT if you do not want to create files with an initial period
#
# -DLATIN1 if your system supports LATIN-1 and you want to use it
#
# Note you can use at most one of the following four!
#
# -DNOMKDIR if your system does not have the mkdir system call
#
# -DAUFS if you want to use an AUFS file system
#
# -DAUFSPLUS if you use CAP 6.0 and want to use times on files
#
# -DAPPLEDOUBLE if you want to be able to use an AppleDouble file system
#
CF =	-DBSD -DTYPES_H -DDIRENT_H -DTERMIOS_H -DNODOT -DAPPLEDOUBLE

all:
	(cd crc; make CF='$(CF)')
	(cd util; make CF='$(CF)')
	(cd fileio; make CF='$(CF)')
	(cd macunpack; make CF='$(CF)')
	(cd hexbin; make CF='$(CF)')
	(cd mixed; make CF='$(CF)')
	(cd binhex; make CF='$(CF)')
	(cd comm; make CF='$(CF)')

clean:
	(cd crc; make clean)
	(cd util; make clean)
	(cd fileio; make clean)
	(cd macunpack; make clean)
	(cd hexbin; make clean)
	(cd mixed; make clean)
	(cd binhex; make clean)
	(cd comm; make clean)

clobber:
	(cd crc; make clean)
	(cd util; make clean)
	(cd fileio; make clean)
	(cd macunpack; make clobber)
	(cd hexbin; make clobber)
	(cd mixed; make clobber)
	(cd binhex; make clobber)
	(cd comm; make clobber)

lint:
	(cd macunpack; make CF='$(CF)' lint)
	(cd hexbin; make CF='$(CF)' lint)
	(cd mixed; make CF='$(CF)' lint)
	(cd binhex; make CF='$(CF)' lint)
	(cd comm; make CF='$(CF)' lint)

install:
	cp macunpack/macunpack $(BINDIR)/.
	cp hexbin/hexbin $(BINDIR)/.
	cp mixed/macsave $(BINDIR)/.
	cp mixed/macstream $(BINDIR)/.
	cp binhex/binhex $(BINDIR)/.
	cp comm/tomac $(BINDIR)/.
	cp comm/frommac $(BINDIR)/.

distr:
	shar -a README makefile crc util fileio macunpack hexbin mixed binhex  \
		comm doc man >macutil.shar

