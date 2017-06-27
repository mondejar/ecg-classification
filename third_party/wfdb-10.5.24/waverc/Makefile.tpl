# file: Makefile.tpl		G. Moody		24 May 2000
#				Last revised:		11 May 2006
# Change the settings below as appropriate for your setup.

# `make all' creates wavescript and wave-remote without installing them.
all:	wavescript wave-remote

# `make install' installs `url_view', `wavescript', and `wave-remote'.  See
# the WAVE User's Guide for instructions on setting up `wavescript' as a helper
# application for  your Web browser.
install:	$(BINDIR) wave-remote
	rm -f wavescript wavescript.exe
	$(MAKE) wavescript	# make sure wavescript has the correct BINDIR
	$(STRIP) wavescript wave-remote
	$(SETXPERMISSIONS) url_view wavescript wave-remote
	../install.sh $(BINDIR) url_view wavescript wave-remote

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) url_view wavescript wave-remote

uninstall:
	../uninstall.sh $(BINDIR) url_view wavescript wave-remote

# `wavescript' reads commands from a named file and passes them to WAVE.
wavescript:	wavescript.c
	$(CC) -o wavescript -DBINDIR=$(BINDIR) -O wavescript.c

# `wave-remote' passes its command-line arguments as commands to WAVE.
wave-remote:	wave-remote.c
	$(CC) -o wave-remote -O wave-remote.c

# `wave-remote-test' looks like WAVE to `wavescript' and `wave-remote', and
# can be used to verify their proper operation.  Start `wave-remote-test'
# before starting `wavescript' or `wave-remote';  the commands these programs
# send to WAVE should appear on the standard output of `wave-remote-test.
wave-remote-test:	wave-remote-test.c
	$(CC) -o wave-remote-test -O wave-remote-test.c

# `make clean':  remove intermediate and backup files
clean:
	rm -f wavescript wave-remote wave-remote-test *~

# Create directory for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)
