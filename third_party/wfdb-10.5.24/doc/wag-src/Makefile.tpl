# file: Makefile.tpl		G. Moody	 24 May 2000
#				Last revised:	  8 May 2015
# Change the settings below as appropriate for your setup.

# D2PARGS is a list of options for dvips.  Uncomment one of these to set the
# paper size ("a4" is most common except in the US and Canada):
# D2PARGS = -t a4
D2PARGS = -t letter

# LN is a command that makes the file named by its first argument accessible
# via the name given in its second argument.  If your system supports symbolic
# links, uncomment the next line.
LN = ln -sf
# Otherwise uncomment the next line if your system supports hard links.
# LN = ln
# If your system doesn't support links at all, copy files instead.
# LN = cp

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

# PSPRINT is the name of the program that prints PostScript files. If your
# printer is not a PostScript printer, see the GhostScript documentation to see
# how to do this (since the figure files are in PostScript form, it is not
# sufficient to use a non-PostScript dvi translator such as dvilj).
PSPRINT = lpr

# TROFF is the name of the program that formats UNIX troff files (needed to
# 'make ag' and for the covers of the guides).  Use 'groff' if you have
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

# PDFPS converts a PDF file to a PostScript file.  Use either of these:
# PDFPS = pdf2ps
PDFPS = pdftops

# PSPDF converts a PostScript file to a PDF file.
PSPDF = ps2pdf

# It should not be necessary to modify anything below this line.
# -----------------------------------------------------------------------------

.IGNORE:

all:	wag.html wag.pdf
	cp -p wag.pdf ../wag

install:	wag.man

collect:
	$(MAKE) MANDIR=/tmp/wfdb/$(MANDIR) install

uninstall:
	../../uninstall.sh $(MAN1) *.1 ad2m.1 ahaconvert.1 ahaecg2mit.1 \
	 ann2rr.1 m2a.1 md2a.1 hrlomb.1 hrmem.1 hrplot.1 plot3d.1 cshsetwfdb.1 \
	 rr2ann.1
	../../uninstall.sh $(MAN3) *.3
	../../uninstall.sh $(MAN5) *.5
	../../uninstall.sh $(MAN7) *.7
	rm -f ../wag/*

# 'make wag-book': print a copy of the WFDB Applications Guide
wag-book:	wag.ps
	cp wag.cover wagcover
	echo $(SHORTDATE) >>wagcover
	echo .bp >>wagcover
	$(TROFF) wagcover >wagcover.ps
	$(PSPRINT) wagcover.ps
	$(PSPRINT) wag.ps

# 'make wag.html': format the WFDB Applications Guide as HTML
wag.html:
	cp -p ../misc/icons/* fixag.sh fixag.sed ../wag
	./manhtml.sh ../wag *.1 *.3 *.5 *.7
	cp -p install0.tex install.tex
	cp -p eval0.tex eval.tex
	latex2html -dir ../wag -local_icons -prefix in \
	 -up_url="wag.htm" -up_title="WFDB Applications Guide" install
	latex2html -dir ../wag -local_icons -prefix ev \
	 -up_url="wag.htm" -up_title="WFDB Applications Guide" eval
	rm -f install.tex eval.tex
	cd ../wag; rm -f index.html WARNINGS *.aux *.log *.tex
	sed "s/LONGDATE/$(LONGDATE)/" <intro.ht0 >../wag/intro.htm
	sed "s/LONGDATE/$(LONGDATE)/" <faq.ht0 >../wag/faq.htm
	cd ../wag; ./fixag.sh "$(LONGDATE)" *.html; rm -f fixag.sh images.*
	cd ../wag; rm -f .I* .ORIG_MAP *.html *.pl fixag.sed
	./maketoc-html.sh | \
	  sed "s/LONGDATE/$(LONGDATE)/" | \
	  sed "s/VERSION/$(VERSION)/" >../wag/wag.htm
	cd ../wag; ln -s wag.htm index.html

# 'make wag.man': install the man pages from the WFDB Applications Guide
wag.man:
	test -d $(MAN1) || ( mkdir -p $(MAN1); $(SETDPERMISSIONS) $(MAN1) )
	test -d $(MAN3) || ( mkdir -p $(MAN3); $(SETDPERMISSIONS) $(MAN3) )
	test -d $(MAN5) || ( mkdir -p $(MAN5); $(SETDPERMISSIONS) $(MAN5) )
	test -d $(MAN7) || ( mkdir -p $(MAN7); $(SETDPERMISSIONS) $(MAN7) )
	./maninst.sh $(MAN1) $(MAN3) $(MAN5) $(MAN7) "$(SETPERMISSIONS)"
	cd $(MAN1); $(LN) a2m.1 ad2m.1
	cd $(MAN1); $(LN) a2m.1 ahaconvert.1
	cd $(MAN1); $(LN) a2m.1 ahaecg2mit.1
	cd $(MAN1); $(LN) a2m.1 m2a.1
	cd $(MAN1); $(LN) a2m.1 md2a.1
	cd $(MAN1); $(LN) ann2rr.1 rr2ann.1
	cd $(MAN1); $(LN) edf2mit.1 mit2edf.1
	cd $(MAN1); $(LN) hrfft.1 hrlomb.1
	cd $(MAN1); $(LN) hrfft.1 hrmem.1
	cd $(MAN1); $(LN) hrfft.1 hrplot.1
	cd $(MAN1); $(LN) plot2d.1 plot3d.1
	cd $(MAN1); $(LN) pnnlist.1 pNNx.1
	cd $(MAN1); $(LN) setwfdb.1 cshsetwfdb.1
	cd $(MAN1); $(LN) wav2mit.1 mit2wav.1

# 'make wag.ps': format the WFDB Applications Guide as PostScript
wag.ps:		 wag.pdf
	$(PDFPS) wag.pdf

# 'make wag.pdf': format the WFDB Applications Guide as PDF
wag.pdf:	wag.tex
	$(MAKE) wag1.pdf  # also makes wag2.pdf, wag3.pdf, wag4.pdf
	pdftk wag[1234].pdf cat output wag.pdf	# concatenate sections
	$(SETPERMISSIONS) wag.pdf

wag1.pdf:	wag2.pdf
	$(MAKE) getpagenos maketoclines
	./maketoc-tex.sh >wag1.toc		# TOC, wag3.pdf, wag4.pdf
	sed 's/VERSION/$(VERSION)/' <wag.tex | \
	  sed 's/LONGDATE/$(LONGDATE)/' >wag1.tex
	pdflatex '\nonstopmode\input{wag1}'	# front matter

wag2.pdf:
	tbl *.1 *.3 *.5 | $(TROFF) $(TMAN) >wag2.ps	# man pages
	$(PSPDF) wag2.ps wag2.pdf

wag3.pdf:	install.tex
	sed "s/LONGDATE/$(LONGDATE)/" <install.tex | \
	  sed "s/VERSION/$(VERSION)/" >wag3.tex
	pdflatex '\nonstopmode\input{wag3}'

wag4.pdf:	eval.tex
	sed "s/LONGDATE/$(LONGDATE)/" <eval.tex | \
	  sed "s/VERSION/$(VERSION)/" >wag4.tex
	pdflatex '\nonstopmode\input{wag4}'

# 'make clean': remove intermediate and backup files
clean:
	rm -f *.aux *.dvi *.log *.ps *.toc intro.htm faq.htm wag*pdf wagcover \
	      eval.tex install.tex wag[1234].tex *~
