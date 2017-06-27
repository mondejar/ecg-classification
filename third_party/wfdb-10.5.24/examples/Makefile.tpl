# file: Makefile.tpl		G. Moody	  23 May 2000
#				Last revised:	26 October 2002
# This section of the Makefile should not need to be changed.

CFILES = example1.c example2.c example3.c example4.c example5.c example6.c \
 example7.c example8.c example9.c example10.c exannstr.c exgetann.c \
 exgetvec.c exputvec.c pgain.c psamples.c psamplex.c refhr.c stdev.c \
 wfdbversion.c
XFILES = example1 example2 example3 example4 example5 example6 \
 example7 example8 example9 example10 exannstr exgetann \
 exgetvec exputvec pgain psamples psamplex refhr stdev \
 wfdbversion
MFILES = Makefile Makefile.dos

# General rule for compiling C sources into executable files.  This is
# redundant for most versions of `make', but at least one System V version
# needs it.
.c:
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# `make' or `make all':  compile the examples
all:	$(XFILES)

# `make install':  compile but do no more
install:	all

uninstall:
	echo "Nothing to be done for uninstall"

# `make listing':  print a listing of sources
listing:
	$(PRINT) README $(MFILES) $(CFILES)

# `make clean':  remove executable, intermediate and backup files
clean:
	rm -f $(XFILES) *.o *~ core
