# file: Makefile.tpl		G. Moody	  24 May 2000
#				Last revised:	  11 May 2006
# This section of the Makefile should not need to be changed.

# Programs to be compiled.
XFILES = coherence fft log10 lomb memse

# Shell scripts to be installed.
SCRIPTS = hrfft hrlomb hrmem hrplot plot2d plot3d

# `make all': build applications
all:	$(XFILES)
	$(STRIP) $(XFILES)

# `make' or `make install':  build and install applications
install:	$(BINDIR) all scripts
	$(SETXPERMISSIONS) $(XFILES)
	../install.sh $(BINDIR) $(XFILES)

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) $(XFILES) $(SCRIPTS)

# `make scripts': customize and install scripts
scripts:
	sed s+BINDIR+$(BINDIR)+g <hrfft >$(BINDIR)/hrfft
	sed s+BINDIR+$(BINDIR)+g <hrlomb >$(BINDIR)/hrlomb
	sed s+BINDIR+$(BINDIR)+g <hrmem >$(BINDIR)/hrmem
	sed s+BINDIR+$(BINDIR)+g <hrplot >$(BINDIR)/hrplot
	cp plot2d plot3d $(BINDIR)
	cd $(BINDIR); $(SETXPERMISSIONS) $(SCRIPTS)

uninstall:
	../uninstall.sh $(BINDIR) $(XFILES) $(SCRIPTS)

coherence:	coherence.c
	$(CC) -o coherence -O coherence.c -lm

fft:		fft.c
	$(CC) -o fft -O fft.c -lm

log10:		log10.c
	$(CC) $(CCDEFS) -o log10 -O log10.c -lm

lomb:		lomb.c
	$(CC) -o lomb -O lomb.c -lm

memse:		memse.c
	$(CC) -o memse -O memse.c -lm

# `make clean': remove intermediate and backup files.
clean:
	rm -f *.o *~ $(XFILES)

# Create directory for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)
