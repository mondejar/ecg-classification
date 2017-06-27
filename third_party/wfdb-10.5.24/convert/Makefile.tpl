# file: Makefile.tpl		G. Moody	24 May 2000
#				Last revised:	5 March 2014
# This section of the Makefile should not need to be changed.

CFILES = a2m.c ad2m.c ahaecg2mit.c m2a.c md2a.c readid.c makeid.c edf2mit.c \
 mit2edf.c parsescp.c rdedfann.c wav2mit.c mit2wav.c wfdb2mat.c revise.c
XFILES = a2m ad2m ahaecg2mit m2a md2a readid makeid edf2mit \
 mit2edf parsescp rdedfann wav2mit mit2wav wfdb2mat revise
SCRIPTS = ahaconvert
MFILES = Makefile

# General rule for compiling C sources into executable files.  This is
# redundant for most versions of `make', but at least one System V version
# needs it.
.c:
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# `make all': build applications
all:	$(XFILES)
	$(STRIP) $(XFILES)

# `make' or `make install':  build and install applications
install:	$(BINDIR) all $(SCRIPTS)
	$(SETXPERMISSIONS) $(XFILES) $(SCRIPTS)
	../install.sh $(BINDIR) $(XFILES) $(SCRIPTS)

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) $(XFILES) $(SCRIPTS)

uninstall:
	../uninstall.sh $(BINDIR) $(XFILES) $(SCRIPTS)

# `make clean':  remove intermediate and backup files
clean:
	rm -f $(XFILES) *.o *~

# `make listing': print a listing of WFDB format-conversion application sources
listing:
	$(PRINT) README $(MFILES) $(CFILES)

# Create directory for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)
