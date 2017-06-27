# file: Makefile.tpl		G. Moody	  22 August 2010
#
# This section of the Makefile should not need to be changed.

CFILES = annxml.c heaxml.c xmlann.c xmlhea.c
HFILES = xmlproc.h
MFILES = Makefile
XFILES = annxml heaxml xmlann xmlhea

# General rule for compiling C sources into executable files.  This is
# redundant for most versions of `make', but at least one System V version
# needs it.
.c:
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# `make all': build applications
all:	$(XFILES)
	$(STRIP) $(XFILES)

# `make' or `make install':  build and install applications
install:	all $(BINDIR)
	$(SETXPERMISSIONS) $(XFILES)
	../install.sh $(BINDIR) $(XFILES)

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) $(XFILES)

uninstall:
	../uninstall.sh $(BINDIR) $(XFILES)

# Create directories for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)

# `make clean':  remove intermediate and backup files
clean:
	rm -f $(XFILES) *.o *~

# `make listing':  print a listing of WFDB-XML applications sources
listing:
	$(PRINT) README $(MFILES) $(CFILES) $(HFILES)

# Rules for compiling WFDB-XML applications that require non-standard options

xmlann:		xmlann.c xmlproc.h
	$(CC) $(CFLAGS) xmlann.c -o $@ $(LDFLAGS) -lexpat

xmlhea:		xmlhea.c xmlproc.h
	$(CC) $(CFLAGS) xmlhea.c -o $@ $(LDFLAGS) -lexpat
