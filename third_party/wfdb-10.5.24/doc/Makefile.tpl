# file: Makefile.tpl		G. Moody	 24 May 2000
#				Last revised:   12 March 2014
# Change the settings below as appropriate for your setup.

# Set COLORS to 'color' if you have a color printer and would like to print
# in color, or if your non-color printer accepts and prints color PostScript
# acceptably (most modern PostScript printers do, and Ghostscript also converts
# color to shades of grey acceptably.)  Set COLORS to 'grey' otherwise.
COLORS = color
#COLORS = grey

# DOSCHK is a command for checking file and directory names within the current
# directory for MS-DOS compatibility, used by 'make html'.  If you have GNU
# doschk, and you wish to perform this check, uncomment the following line:
DOSCHK = find . -print | doschk
# Otherwise, skip the check by uncommenting the next line instead:
# DOSCHK = 

# D2PARGS is a list of options for dvips.  Uncomment one of these to set the
# paper size ("a4" is most common except in the US and Canada):
# D2PARGS = -t a4
D2PARGS = -t letter

# T2DARGS is a list of options for texi2dvi.  Uncomment one of these to set the
# page size (the size of the printed area on the paper;  normally this should
# match the paper size specified in D2PARGS):
# T2DARGS = -t @afourpaper
T2DARGS = -t @letterpaper

# LN is a command that makes the file named by its first argument accessible
# via the name given in its second argument.  If your system supports symbolic
# links, uncomment the next line.
LN = ln -sf
# Otherwise uncomment the next line if your system supports hard links.
# LN = ln
# If your system doesn't support links at all, copy files instead.
# LN = cp

# If you wish to install the info (GNU hypertext documentation) files from this
# package, specify the command needed to format them from the texinfo source
# files.  If you have the GNU 'makeinfo' utility (the preferred formatter),
# uncomment the next line.
MAKEINFO = makeinfo --force --no-warn
# Otherwise, you can use GNU emacs to do the job by uncommenting the next line.
# MAKEINFO = ./makeinfo.sh

# MAN1, MAN3, MAN5, and MAN7 are the directories in which local man pages for
# section 1 (commands), section 3 (libraries), section 5 (formats), and
# section 7 (conventions and miscellany) go.  You may wish to use
# $(MANDIR)/manl for all of these; if so, uncomment the next four lines.
# MAN1 = $(MANDIR)/manl
# MAN3 = $(MANDIR)/manl
# MAN5 = $(MANDIR)/manl
# MAN7 = $(MANDIR)/manl
# Uncomment the next four lines to put the man pages in with the standard
# ones.
MAN1 = $(MANDIR)/man1
MAN3 = $(MANDIR)/man3
MAN5 = $(MANDIR)/man5
MAN7 = $(MANDIR)/man7
# If you want to put the man pages somewhere else, edit 'maninst.sh' first.

# PERL is the full pathname of your perl interpreter, needed for
# 'make wpg.html'.
PERL = /usr/bin/perl

# PSPRINT is the name of the program that prints PostScript files. If your
# printer is not a PostScript printer, see the GhostScript documentation to see
# how to do this (since the figure files are in PostScript form, it is not
# sufficient to use a non-PostScript dvi translator such as dvilj).
PSPRINT = lpr

# TROFF is the name of the program that prints UNIX troff files (needed to
# 'make wag-book' and for the covers of the guides).  Use 'groff' if you have
# GNU groff (the preferred formatter).
TROFF = groff
# Use 'ptroff' if you have Adobe TranScript software.
# TROFF = ptroff
# Consult your system administrator if you have neither 'groff' nor 'ptroff'.
# Other (untested) possibilities are 'psroff', 'ditroff', 'nroff', and 'troff'.

# TMAN is the TROFF option needed to load the 'man' macro package.  This should
# not need to be changed unless your system is non-standard;  see the file
# 'tmac.dif' for comments on a page-numbering bug in some versions of the 'man'
# package.
# TMAN = -man
# Use the alternate definition below to get consecutively numbered pages using
# GNU groff.  Omit -rD1 if the final document will be printed on only one side
# of each page.
TMAN = -rC1 -rD1 -man

# TMS is the TROFF option needed to load the 'ms' macro package.  Use the
# following definition to get the standard 'ms' macros.
# TMS = -ms
# Use the following definition to get the GNU groff version of the 'ms' macros.
TMS = -mgs

# It should not be necessary to modify anything below this line.
# -----------------------------------------------------------------------------

.IGNORE:

all:
	@echo "Type 'make install' to install the man pages."
	@echo "Type 'make uninstall' to remove previously installed man pages."
	@echo "See 'README' for other choices."

install:
	cd wag-src; $(MAKE) wag.man

collect:
	cd wag-src; $(MAKE) collect

uninstall:
	cd wag-src; $(MAKE) uninstall

# 'make html': create HTML files, check for anything not accessible to MSDOS
html:
	cd wag-src; $(MAKE) wag.html
	cd wpg-src; $(MAKE) wpg.html
	cd wug-src; $(MAKE) wug.html
	cp -p misc/index.ht0 index.htm
	date '+%e %B %Y' >>index.htm
	cat misc/foot.ht0 >>index.htm
	$(DOSCHK)
	$(LN) index.htm index.html

# 'make tarball': create a gzip-compressed tar archive of formatted documents
tarball:
	cd wag-src; make
	cd wpg-src; make
	cd wug-src; make
	tar cfvz ../../wfdb-doc-$(VERSION).tar.gz wag wpg wug

# -----------------------------------------------------------------------------
# WFDB Applications Guide

# 'make wag-book': print a copy of the WFDB Applications Guide
wag-book:	
	cd wag-src; $(MAKE) wag

# 'make wag.html': format the WFDB Applications Guide as HTML
wag.html:
	cd wag-src; $(MAKE) wag.html

# 'make wag.man': install the man pages from the WFDB Applications Guide
wag.man:
	cd wag-src; $(MAKE) wag.man

# 'make wag.pdf': format the WFDB Applications Guide as PDF
wag.pdf:
	cd wag-src; $(MAKE) wag.pdf
	
# 'make wag.ps': format the WFDB Applications Guide as PostScript
wag.ps:
	cd wag-src; $(MAKE) wag.ps

# -----------------------------------------------------------------------------
# WFDB Programmer's Guide

# 'make wpg-book': print a copy of the WFDB Programmer's Guide
wpg-book:
	cd wpg-src; $(MAKE) wpg

# 'make wpg.hlp': format the WFDB Programmer's Guide as an MS-Windows help file
wpg.hlp:
	cd wpg-src; $(MAKE) wpg.hlp

# 'make wpg.html': format the WFDB Programmer's Guide as HTML
wpg.html:
	cd wpg-src; $(MAKE) wpg.html

# 'make wpg.info': format the WFDB Programmer's Guide as info files
wpg.info:
	cd wpg-src; $(MAKE) wpg.info

# 'make wpg.info.tar.gz': create a tarball of info files
wpg.info.tar.gz:
	cd wpg-src; $(MAKE) wpg.info.tar.gz

# 'make wpg.pdf': format the WFDB Programmer's Guide as PDF
wpg.pdf:
	cd wpg-src; $(MAKE) wpg.pdf

# 'make wpg.ps': format the WFDB Programmer's Guide as PostScript
wpg.ps:
	cd wpg-src; $(MAKE) wpg.ps

# -----------------------------------------------------------------------------
# WAVE User's Guide

# 'make wug-book': print a copy of the WAVE User's Guide
wug-book:
	cd wug-src; $(MAKE) wug

# 'make wug.html': format the WAVE User's Guide as HTML
wug.html:
	cd wug-src; $(MAKE) wug.html

# 'make wug.pdf': format the WAVE User's Guide as PDF
wug.pdf:
	cd wug-src; $(MAKE) wug.pdf

# 'make wug.ps': format the WAVE User's Guide as PostScript
wug.ps:
	cd wug-src; $(MAKE) wug.ps

# -----------------------------------------------------------------------------
# 'make clean': remove intermediate and backup files
clean:
	cd wag-src; make clean
	cd wpg-src; make clean
	cd wug-src; make clean
	rm -f index.htm index.html wag/* wpg/*.htm* wpg/*.pdf wpg/*.ps wpg/info/w* wug/* *~
