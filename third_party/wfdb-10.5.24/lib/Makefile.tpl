# file: Makefile.tpl		G. Moody	  24 May 2000
#				Last revised:    14 August 2014
# This section of the Makefile should not need to be changed.

INCLUDES = $(INCDIR)/wfdb/wfdb.h $(INCDIR)/wfdb/wfdblib.h \
 $(INCDIR)/wfdb/ecgcodes.h $(INCDIR)/wfdb/ecgmap.h
HFILES = wfdb.h ecgcodes.h ecgmap.h wfdblib.h
CFILES = wfdbinit.c annot.c signal.c calib.c wfdbio.c
OFILES = wfdbinit.o annot.o signal.o calib.o wfdbio.o
MFILES = Makefile Makefile.dos

# `make' or `make all':  build the WFDB library
all:	setup $(OFILES)
	$(BUILDLIB) $(OFILES) $(BUILDLIB_LDFLAGS)

# `make install':  install the WFDB library and headers
install:
	$(MAKE) clean	    # force recompilation since config may have changed
	$(MAKE) all
	$(MAKE) $(INCLUDES) $(LIBDIR)
	cp $(WFDBLIB) $(LIBDIR) 
	$(SETLPERMISSIONS) $(LIBDIR)/$(WFDBLIB)
	$(MAKE) lib-post-install 2>/dev/null

# 'make collect':  retrieve the installed WFDB library and headers
collect:
	../conf/collect.sh $(INCDIR)/wfdb wfdb.h ecgcodes.h ecgmap.h
	../conf/collect.sh $(LIBDIR) $(WFDBLIB) $(WFDBLIB_DLLNAME)

uninstall:
	../uninstall.sh $(INCDIR)/wfdb $(HFILES)
	../uninstall.sh $(INCDIR)
	../uninstall.sh $(LIBDIR) $(WFDBLIB)
	$(MAKE) lib-post-uninstall
	../uninstall.sh $(LIBDIR)

setup:
	sed "s+DBDIR+$(DBDIR)+" <wfdblib.h0 >wfdblib.h

# `make clean': remove binaries and backup files
clean:
	rm -f $(OFILES) libwfdb.* *.dll *~

# `make TAGS':  make an `emacs' TAGS file
TAGS:		$(HFILES) $(CFILES)
	@etags $(HFILES) $(CFILES)

# `make listing':  print a listing of WFDB library sources
listing:
	$(PRINT) README $(MFILES) $(HFILES) $(CFILES)

# Rule for creating installation directories
$(INCDIR) $(INCDIR)/wfdb $(INCDIR)/ecg $(LIBDIR):
	mkdir -p $@; $(SETDPERMISSIONS) $@

# Rules for installing the include files
$(INCDIR)/wfdb/wfdb.h:		$(INCDIR)/wfdb wfdb.h
	cp -p wfdb.h $(INCDIR)/wfdb; $(SETPERMISSIONS) $(INCDIR)/wfdb/wfdb.h
$(INCDIR)/wfdb/wfdblib.h:		$(INCDIR)/wfdb wfdblib.h
	cp -p wfdblib.h $(INCDIR)/wfdb; $(SETPERMISSIONS) $(INCDIR)/wfdb/wfdblib.h
$(INCDIR)/wfdb/ecgcodes.h:	$(INCDIR)/wfdb ecgcodes.h
	cp -p ecgcodes.h $(INCDIR)/wfdb
	$(SETPERMISSIONS) $(INCDIR)/wfdb/ecgcodes.h
$(INCDIR)/wfdb/ecgmap.h:		$(INCDIR)/wfdb ecgmap.h
	cp -p ecgmap.h $(INCDIR)/wfdb
	$(SETPERMISSIONS) $(INCDIR)/wfdb/ecgmap.h

# Prerequisites for the library modules
wfdbinit.o:	wfdb.h wfdblib.h wfdbinit.c
annot.o:	wfdb.h ecgcodes.h ecgmap.h wfdblib.h annot.c
signal.o:	wfdb.h wfdblib.h signal.c
calib.o:	wfdb.h wfdblib.h calib.c
wfdbio.o:	wfdb.h wfdblib.h wfdbio.c
	$(CC) $(CFLAGS) -DVERSION='"$(VERSION)"' -DCFLAGS='"-I$(INCDIR)"' \
	  -DLDFLAGS='"$(LDFLAGS)"' -c wfdbio.c
