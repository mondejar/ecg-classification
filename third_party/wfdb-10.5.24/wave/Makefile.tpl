# file: Makefile.tpl		G. Moody	31 May 2000
#				Last revised:   21 February 2009
# Change the settings below as appropriate for your setup.

# Choose directories in which to install WAVE and its ancillary files by
# editing the variables below.  You will need write permission in all of them
# in order to install WAVE successfully, and WAVE users will need read
# permission in all of them.  If the directories don't exist already, they
# will be created with appropriate permissions by the installation procedure.

# This section of site-dependent variables specifies the locations in your
# file system where the WAVE software and data files will be installed.
# You may choose a different set of locations if you prefer, but documentation
# included in this package generally assumes that you have used the defaults
# given here.  You will need write permission in all of the directories named
# in this section, and users of the software will need read permission in all
# of these directories.  Generally, you will need `root' permissions in order
# to install the software in the standard places.

# HELPDIR specifies the directory in which the on-line help files are kept.
# The installation procedure creates a subdirectory, `wave', in HELPDIR, and
# installs several files there.
HELPDIR = $(WFDBROOT)/help

# MENUDIR specifies the directory in which the default analysis menu file is
# kept.
MENUDIR = $(WFDBROOT)/lib

# RESDIR specifies the directory in which X11 client resource files are kept.
RESDIR = $(WFDBROOT)/lib/X11/app-defaults

# It should not be necessary to modify anything below this line.
# -----------------------------------------------------------------------------

HFILES = wave.h bitmaps.h xvwave.h
CFILES = wave.c init.c mainpan.c modepan.c helppan.c logpan.c annpan.c edit.c \
 grid.c sig.c annot.c analyze.c scope.c search.c xvwave.c help.c
OFILES = wave.o init.o mainpan.o modepan.o helppan.o logpan.o annpan.o edit.o \
 grid.o sig.o annot.o analyze.o scope.o search.o xvwave.o $(HELPOBJ)
HELPFILES = analysis.hlp buttons.hlp editing.hlp intro.hlp log.hlp news.hlp \
 printing.hlp resource.hlp
OTHERFILES = wave.hl0 wave.info demo.txt Wave.res wavemenu.def Makefile

all:	wave wave.hlp news.hlp

# `make install':  compile and install WAVE and its help files
install:  $(BINDIR) $(HELPDIR)/wave $(MENUDIR) $(RESDIR) wave.hlp news.hlp
	rm -f wave.o analyze.o xvwave.o
	$(MAKE) wave  # make sure all compiled-in paths are up-to-date
	$(STRIP) wave; $(SETXPERMISSIONS) wave;	../install.sh $(BINDIR) wave
	cp $(HELPFILES) wave.hlp wave.info demo.txt $(HELPDIR)/wave
	cd $(HELPDIR)/wave; $(SETPERMISSIONS) $(HELPFILES) wave.info demo.txt
	-cp wavemenu.def $(MENUDIR) && \
	 $(SETPERMISSIONS) $(MENUDIR)/wavemenu.def
	-cp Wave.res $(RESDIR)/Wave && $(SETPERMISSIONS) $(RESDIR)/Wave

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) wave
	../conf/collect.sh $(HELPDIR)/wave $(HELPFILES) wave.info demo.txt
	../conf/collect.sh $(MENUDIR) wavemenu.def
	../conf/collect.sh $(RESDIR) Wave

uninstall:
	../uninstall.sh $(BINDIR) wave
	../uninstall.sh $(HELPDIR)/wave $(HELPFILES) wave.hlp wave.info \
	  demo.txt
	rmdir $(HELPDIR) || echo "(Ignored)"
	../uninstall.sh $(MENUDIR) wavemenu.def
	../uninstall.sh $(RESDIR) Wave
	../uninstall.sh $(LIBDIR)/X11
	../uninstall.sh $(LIBDIR)

wave:		$(OFILES)
	$(CC) $(WCFLAGS) -o wave $(OFILES) $(WLDFLAGS)

# `make help':  show help text
help:
	@echo "*************************************************************"
	@echo "To print the WAVE manual, type 'make manual'."
	@echo "If you have a PostScript printer, you may also wish to print"
	@echo "the WAVE User's Guide, by typing 'make guide'."
	@echo "*************************************************************"
	@echo
	@../conf/prompt "Press <Return> to view the manual on-screen: "
	@read x
	@echo
	@soelim wave.hl0 | more
	@echo
	@echo "*************************************************************"
	@echo "To print the WAVE manual, type 'make manual'."
	@echo "If you have a PostScript printer, you may also wish to print"
	@echo "the WAVE User's Guide, by typing 'make guide'."
	@echo "*************************************************************"

wave-static:	$(OFILES)
	$(CC) $(WCFLAGS) -o wave-static $(OFILES) -static $(LDFLAGS)

soelim:		soelim.c
	$(CC) -o soelim -O soelim.c

wave.hlp:	soelim wave.hl0 $(HELPFILES)
	./soelim wave.hl0 >wave.hlp

news.hlp:
	sed s/WAVEVERSION/$(WAVEVERSION)/ <wave.prf | sed "s/WHEN/`date`/" | \
	 sed "s%HELPDIR%$(HELPDIR)%" >news.hlp

# `make manual': print the on-line manual
manual:
	./soelim wave.hl0 | $(PRINT)

# `make guide': print the WAVE User's Guide
guide:
	cd ../doc; make wug-book

# `make TAGS':  make an `emacs' TAGS file
TAGS:		$(HFILES) $(CFILES)
	@etags $(HFILES) $(CFILES)

# `make clean':  remove intermediate and backup files
clean:
	rm -f soelim wave wave-static *.o *~ wave.hlp news.hlp

# `make listing':  print a listing of WAVE sources
listing:	wave.hlp news.hlp
	$(PRINT) README REGCARD $(HFILES) $(CFILES) $(HELPFILES) $(OTHERFILES)

# Dependencies and special rules for compilation of the modules of `wave'
wave.o:
	$(CC) -c $(WCFLAGS) -DHELPDIR=\"$(HELPDIR)\" wave.c
init.o:		wave.h xvwave.h init.c
	$(CC) -c $(WCFLAGS) init.c
mainpan.o:	wave.h xvwave.h mainpan.c Makefile
	$(CC) -c $(WCFLAGS) -DWAVEVERSION=\"$(WAVEVERSION)\" mainpan.c
modepan.o:	wave.h xvwave.h modepan.c
	$(CC) -c $(WCFLAGS) modepan.c
helppan.o:	wave.h xvwave.h helppan.c
	$(CC) -c $(WCFLAGS) -DWAVEVERSION=\"$(WAVEVERSION)\" helppan.c
logpan.o:	wave.h xvwave.h logpan.c
	$(CC) -c $(WCFLAGS) logpan.c
annpan.o:	wave.h xvwave.h annpan.c
	$(CC) -c $(WCFLAGS) annpan.c
edit.o:		wave.h xvwave.h edit.c
	$(CC) -c $(WCFLAGS) edit.c
grid.o:		wave.h xvwave.h grid.c
	$(CC) -c $(WCFLAGS) grid.c
search.o:	wave.h xvwave.h search.c
	$(CC) -c $(WCFLAGS) search.c
sig.o:		wave.h xvwave.h sig.c
	$(CC) -c $(WCFLAGS) sig.c
annot.o:	wave.h xvwave.h annot.c
	$(CC) -c $(WCFLAGS) -DWAVEVERSION=\"$(WAVEVERSION)\" annot.c
analyze.o:    	analyze.c
	$(CC) -c $(WCFLAGS) -DMENUDIR=\"$(MENUDIR)\" analyze.c
scope.o:	wave.h xvwave.h scope.c
	$(CC) -c $(WCFLAGS) scope.c
xvwave.o:     	xvwave.c
	$(CC) -c $(WCFLAGS) -DRESDIR=\"$(RESDIR)\" xvwave.c
help.o:		help.c
	$(CC) -c $(WCFLAGS) -w help.c

# Create directories for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)
$(HELPDIR):
	mkdir -p $(HELPDIR); $(SETDPERMISSIONS) $(HELPDIR)
$(HELPDIR)/wave:
	mkdir -p $(HELPDIR)/wave; $(SETDPERMISSIONS) $(HELPDIR)/wave
$(MENUDIR):
	mkdir -p $(MENUDIR); $(SETDPERMISSIONS) $(MENUDIR)
$(RESDIR):
	mkdir -p $(RESDIR); $(SETDPERMISSIONS) $(RESDIR)
