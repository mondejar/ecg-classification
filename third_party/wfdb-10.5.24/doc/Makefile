# file: Makefile	G. Moody	24 June 1989
#			Last revised:  11 March 2002
# `make' description file for WFDB software documentation
#
# -----------------------------------------------------------------------------
# WFDB applications: programs for working with annotated signals
# Copyright (C) 2002 George B. Moody
#
# These programs are free software; you can redistribute them and/or modify
# them under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# These programs are distributed in the hope that they will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# these programs; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# You may contact the author by e-mail (george@mit.edu) or postal mail
# (MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
# please visit PhysioNet (http://www.physionet.org/).
# _____________________________________________________________________________
#
# This file is used with the UNIX `make' command to install on-line
# documentation for the WFDB Software Package, and also to format and print it.
# Before using this file for the first time, check that the site-specific
# variables below are appropriate for your system.

# The primary documentation for the WFDB Software Package consists of 3 guides:
#  * The WFDB Applications Guide.  This guide documents the application
#    programs provided in the app, convert, psd, wave (briefly), and wview
#    subdirectories of wfdb, as well as the file formats supported by the WFDB
#    Software Package. It also includes a tutorial on evaluation of arrhythmia
#    and ischemia detectors using this software and standard databases.
#  * The WFDB Programmer's Guide.  This guide documents the WFDB library.
#    It contains both tutorial and reference material, with many examples, that
#    will be useful if you wish to develop your own application programs for
#    reading, analyzing, or creating digitized, optionally annotated,
#    recordings of signals.
#  * The WAVE User's Guide.  This guide includes a comprehensive tutorial, with
#    reference material, for WAVE, an interactive waveform browser with
#    facilities for annotation editing and control of external analysis
#    programs. WAVE runs on PCs under Linux, and on SPARC-based systems under
#    SunOS or Solaris.
# The files in this directory can be used to prepare printed or on-line
# versions of these guides.

# On-line documentation
# =====================

# The most recent versions of these guides are always available on PhysioNet
# (follow the links from http://www.physionet.org/physiotools/manuals.shtml).
# Use any web browser to read them.

# If you follow the instructions for a standard installation of the WFDB
# Software Package, the man pages from this directory will be installed so they
# can be read using 'man', 'xman', etc.  The individual pages will be
# accessible to the UNIX `man' command, and to similar commands such as `xman',
# provided that MANDIR (see below) is in your MANPATH (type `man man' for
# further information).

# If you have the necessary formatting software, these man pages and a large
# amount of other documentation can be produced in a wide variety of formats
# from the sources in in this directory (see 'Makefile').  Here is a brief list
# of available formats and the commands needed to produce them:

# WFDB Applications Guide
# printed copy	'make wag-book' (requires troff, tbl, latex, dvips, a printer)
# HTML		'make wag.html' (requires rman, latex2html, and perl)
# man pages	'make wag.man'
# PDF		'make wag.pdf'  (requires troff, tbl, latex, dvips, and ps2pdf)
# PostScript	'make wag.ps'   (requires troff, tbl, latex, dvips, ps2pdf, and
#				 pdftops *)
# WFDB Programmer's Guide
# printed copy	'make wpg-book' (requires troff, texi2dvi, dvips, & a printer)
# Windows help	'make wpg.hlp'  (requires makertf and hcrtf)
# HTML		'make wpg.html' (requires texi2html and perl)
# info		'make wpg.info' (requires makeinfo or GNU emacs)
# PDF		'make wpg.pdf'  (requires texi2dvi)
# PostScript	'make wpg.ps'   (requires texi2dvi and dvips)
#
# WAVE User's Guide
# printed copy	'make wug-book' (requires troff, latex, dvips, and a printer)
# HTML		'make wug.html' (requires latex2html and perl)
# PDF		'make wug.pdf'  (requires pdflatex)
#  PostScript	'make wug.ps'   (requires latex and dvips)

# * See 'wag-src/Makefile' for information about making a PostScript version
#   of the WFDB Applications Guide without the use of ps2pdf or pdftops.

# If GNU emacs or GNU info has been installed on your system, a hypertext
# version of the WFDB Programmer's Guide may be installed by typing `make
# pg.info'.  Once the info files have been installed, provided that INFODIR is
# in your INFOPATH (see below), you can browse the guide from within emacs by
# typing C-h i to enter the info system, and selecting `WFDB library' from the
# top-level menu.  Note that `make info' adds an entry for the Guide to the
# top-level `info' file ($INFODIR/dir).  If you install a GNU emacs update,
# this entry may disappear; to restore it, simply `make pg.info' again.

# The WFDB Programmer's Guide can also be formatted as an MS-Windows help file,
# using 'makertf' and 'hcrtf'.  This requires 'makertf' (open-source, from
# http://makertf.sourceforge.net/) and 'hcrtf' (see the 'makertf' page above
# for further information).  Install both of these programs in your PATH before
# trying to generate pg.hlp.


# Printed documentation
# =====================

# Individual man pages or info nodes can be printed using the facilities of
# man and emacs respectively, or (from the HTML versions) using your web
# browser's print facilities.  Complete printed copies of the guides can be
# obtained in one of these ways:
#  - The PostScript versions can be downloaded from PhysioNet for printing on
#    a PostScript printer.  If you don't have a PostScript printer, you may
#    still be able to use Ghostscript (available freely from 
#    http://www.ghostscript.com/) to print these versions on your printer
#    (most popular laser, inkjet, and dot matrix printers are supported by
#    Ghostscript).
#  - The PDF versions can be downloaded from PhysioNet for printing using
#    Acrobat reader (available freely from http://www.adobe.com/).  These
#    can also be read and printed using GhostScript (see above), but there
#    is no advantage in doing so; use the PostScript versions with GhostScript.
#  - The sources for these guides are provided in this directory;  you should
#    use these if you have made additions to the guides or wish to customize
#    them in any other way.  To format the sources, you will need groff or
#    troff, gtbl or tbl, TeX, LaTeX, texi2dvi, and dvips.  To format and print
#    the WFDB Applications Guide, type `make ag' (but read `tmac.dif'
#    first, unless you are using groff).  To print the WAVE User's Guide, type
#    `make ug'.  To print the WFDB Programmer's Guide, type `make pg'.
# _____________________________________________________________________________
# file: version.def		G. Moody	24 May 2000
#				Last revised:	12 July 2010
# Each release of the WFDB Software Package is identified by a three-part
# version number, defined below.  Be sure to leave a single space before
# and after the "=" in each of the next three lines!
MAJOR = 10
MINOR = 5
RELEASE = 24
VERSION = $(MAJOR).$(MINOR).$(RELEASE)

# RPMRELEASE can be incremented if changes are made between official
# releases.  It should be reset to 1 whenever the VERSION is changed.
RPMRELEASE = 1

# VDEFS is the set of C compiler options needed to set version number variables
# while compiling the WFDB Software Package.
VDEFS = -DWFDB_MAJOR=$(MAJOR) -DWFDB_MINOR=$(MINOR) -DWFDB_RELEASE=$(RELEASE)

# WAVEVERSION is the WAVE version number.
WAVEVERSION = 6.11
# _____________________________________________________________________________

# Definitions generated by 'configure'

PACKAGE = wfdb-10.5.24
SRCDIR = "/home/mondejar/Dropbox/ECG/code/ecg_classification/third_party/wfdb-10.5.24"
LONGDATE = 8 June 2017
SHORTDATE = JUNE 2017
YEAR = 2017
ARCH = x86_64-Linux
# _____________________________________________________________________________

# file: linux.def		G. Moody	31 May 2000
#				Last revised:  13 August 2014
# 'make' definitions for compiling the WFDB Software Package under Linux

# Choose a value for WFDBROOT to determine where the WFDB Software Package will
# be installed.  One of the following is usually a reasonable choice.
# Installing in /usr generally requires root permissions, but will be easiest
# for future software development (no special -I or -L options will be needed
# to compile software with the WFDB library, since the *.h files and the
# library will be installed in the standard directories).
WFDBROOT = /usr/local
# Installing in /usr/local usually requires root permissions.  On a multi-user
# system where it is desirable to keep the OS vendor's software separate from
# other software, this is a good choice.  Another common choice in such cases
# is /opt .
# WFDBROOT = /usr/local
# To install without root permissions, a good choice is to set WFDBROOT to the
# name of your home directory, as in the example below (change as needed).
# WFDBROOT = /home/frodo

# LC and LL are used to determine C compiler and linker options needed to
# enable NETFILES (code that allows applications linked to the WFDB library to
# read input from HTTP and FTP servers, in addition to the standard filesystem
# support available without NETFILES).  The WFDB library can use either
# libcurl (recommended) or libwww to provide NETFILES support.  To use libcurl,
# set LC to `curl-config --cflags` (note the backquotes) and LL to
# `curl-config --libs`.  Otherwise, to use libwww, set LC to
# `libwww-config --cflags` and LL to `libwww-config --libs`.  If neither
# libcurl nor libwww is available, LC and LL should be empty (and NETFILES
# will be disabled).
LC = `curl-config --cflags`
LL = `curl-config --libs`

# BINDIR specifies the directory in which the applications will be installed;
# it should be a directory in the PATH of those who will use the applications.
BINDIR = $(WFDBROOT)/bin

# DBDIR specifies the name of a directory in which to install the contents
# of the `data' directory.
DBDIR = $(WFDBROOT)/database

# INCDIR specifies the name of a directory in which to install the WFDB
# library's #include <...> files.
INCDIR = $(WFDBROOT)/include

# INFODIR is the GNU info directory (optional, needed to `make info').
INFODIR = $(WFDBROOT)/share/info

# LIBDIR specifies the name of a directory in which to install the WFDB
# library.
LIBDIR = $(WFDBROOT)/lib

# MANDIR is the root of the man page directory tree.  On most systems, this is
# something like /usr/man or /usr/local/man (type 'man man' to find out).
MANDIR = $(WFDBROOT)/share/man

# PSPDIR specifies the name of a directory in which to install the PostScript
# prolog (*.pro) files from the 'app' directory.
PSPDIR = $(WFDBROOT)/lib/ps

# CC is the name of your C compiler.
CC = gcc

# CCDEFS is the set of C compiler options needed to set preprocessor variables
# while compiling the WFDB Software Package.  CCDEFS should always include
# VDEFS (the version definitions).  Under Linux, add -fno-stack-protector
# when compiling the original (SWIG-based) WFDB Toolkit for Matlab.
CCDEFS = $(VDEFS)

# MFLAGS is the set of architecture-dependent (-m*) compiler options, which
# is usually empty.  See the gcc manual for information about gcc's -m options.
MFLAGS =

# CFLAGS is the set of C compiler options.  CFLAGS should always include
# CCDEFS.
CFLAGS = $(MFLAGS) -g -O $(CCDEFS) $(LC) -I$(INCDIR)

# RPATHFLAGS is a set of C compiler options that cause the program to
# search for shared libraries in a non-standard location at run-time.
# These options are not needed or useful when using a static library.
RPATHFLAGS = -Wl,--enable-new-dtags,-rpath,$(LIBDIR)

# LDFLAGS is appended to the C compiler command line to specify loading the
# WFDB library.
LDFLAGS = -L$(LIBDIR) -lwfdb $(RPATHFLAGS)

# WFDBLIB is the name of the standard WFDB library.  In order to access it via
# `-lwfdb', WFDBLIB should be `libwfdb.a'.
WFDBLIB = libwfdb.a

# BUILDLIB is the command that creates the static WFDB library once its
# components have been compiled separately;  the list of *.o files that
# make up the library will be appended to BUILDLIB.
BUILDLIB = $(AR) $(ARFLAGS) $(WFDBLIB)

# PRINT is the name of the program used to produce listings (including any
# options for the desired formatting).
PRINT = lpr

# SETPERMISSIONS is the command needed to make the installed files accessible
# to those who will use them.  The value given below makes them readable by
# everyone, and writeable by the owner only.  (If you perform the installation
# as `root', `root' is the owner of the installed files.)
SETPERMISSIONS = chmod 644

# SETDPERMISSIONS is similarly used to make directories created during the
# installation accessible.
SETDPERMISSIONS = chmod 755

# SETLPERMISSIONS is the command needed to make the WFDB library usable by
# programs linked to it.
SETLPERMISSIONS = chmod 644

# SETXPERMISSIONS is the command needed to make the applications accessible.
SETXPERMISSIONS = chmod 755

# STRIP is the command used to compact the compiled binaries by removing their
# symbol tables.
STRIP = strip
# To retain the symbol tables for debugging, comment out the previous line, and
# uncomment the next line.
# STRIP = :

# ...........................................................................
# This section of definitions is used only when compiling WAVE, which is
# possible only if the XView and Xlib libraries and include files have been
# installed.

# OPENWINHOME specifies the root directory of the OpenWindows hierarchy.
# This is usually /usr/openwin.
OPENWINHOME = /usr/openwin

# OWINCDIR is the directory in which the `xview' directory containing XView
# *.h files is found.
OWINCDIR = $(OPENWINHOME)/include

# OWLIBDIR is the directory in which the XView library is found.
OWLIBDIR = $(OPENWINHOME)/lib

# XHOME specifies the root directory of the X11 hierarchy.
# This is usually /usr/X11R6 (or /usr/X11).
XHOME = /usr/X11R6

# XINCDIR is the directory in which the 'X11' directory containing X11 *.h
# files is found.  This is usually /usr/X11R6/include, although there is often
# a link connecting /usr/include/X11 to this directory.
XINCDIR = $(XHOME)/include

# XLIBDIR is the directory in which the X11 libraries are found.
XLIBDIR = $(XHOME)/lib

# WCFLAGS is the set of C compiler options to use when compiling WAVE.
WCFLAGS = $(CFLAGS) -I$(OWINCDIR) -I$(XINCDIR) -fno-stack-protector

# HELPOBJ can be set to "help.o" if you wish to recompile the XView spot help
# functions in "wave/help.c" (recommended under Linux).
HELPOBJ = help.o
# Otherwise, use the version in libxview by uncommenting the next line:
# HELPOBJ =

# WLDFLAGS is the set of loader options appended to the C compiler command line
# to specify loading the WFDB, XView, and Xlib libraries.
WLDFLAGS = $(LDFLAGS) -L$(OWLIBDIR) -L$(XLIBDIR) -lxview -lolgx -lX11
# Users of Red Hat Linux 5.0 or 5.1 (not later versions), use this instead:
# WLDFLAGS = $(LDFLAGS) -L$(XLIBDIR) $(OWLIBDIR)/libxview.a \
#  $(OWLIBDIR)/libolgx.a -lX11
# This setting avoids incompatibilities with the shared (dynamic) libraries.
# ...........................................................................

# `make' (with no target specified) will be equivalent to `make all'.
make-all:	all

# `make lib-post-install' should be run after installing the WFDB library.
lib-post-install:
	ranlib $(LIBDIR)/$(WFDBLIB)

lib-post-uninstall:
	echo "Nothing to be done for lib-post-uninstall"
# _____________________________________________________________________________
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
