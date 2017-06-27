# file: Makefile.tpl		G. Moody	  24 May 2000
#				Last revised:	   8 May 2015
# Change the settings below as appropriate for your setup.

# Set COLORS to 'color' if you have a color printer and would like to print
# in color, or if your non-color printer accepts and prints color PostScript
# acceptably (most modern PostScript printers do, and Ghostscript also converts
# color to shades of grey acceptably.)  Set COLORS to 'grey' otherwise.
COLORS = color
#COLORS = grey

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

# PSPRINT is the name of the program that prints PostScript files. If your
# printer is not a PostScript printer, see the GhostScript documentation to see
# how to do this (since the figure files are in PostScript form, it is not
# sufficient to use a non-PostScript dvi translator such as dvilj).
PSPRINT = lpr

# TROFF is the name of the program that prints UNIX troff files (needed to
# print the cover of the guide).  Use 'groff' if you have GNU groff (the
# preferred formatter).
TROFF = groff
# Use 'ptroff' if you have Adobe TranScript software.
# TROFF = ptroff
# Consult your system administrator if you have neither 'groff' nor 'ptroff'.
# Other (untested) possibilities are 'psroff', 'ditroff', 'nroff', and 'troff'.

# It should not be necessary to modify anything below this line.
# -----------------------------------------------------------------------------

.IGNORE:

all:	wug.html wug.pdf
	cp -p wug.pdf ../wug

install:
	@echo Nothing to install in wug-src.

uninstall:
	rm -f ../wug/*

# 'make wug-book': print a copy of the WAVE User's Guide
wug-book:	wug.ps
	cp wug.cover wugcover
	echo $(SHORTDATE) >>wugcover
	echo .bp >>wugcover
	$(TROFF) wugcover >wugcover.ps
	$(PSPRINT) wugcover.ps
	$(PSPRINT) wug.ps

# 'make wug.html': format the WAVE User's Guide as HTML
#   'wug.aux' is listed as a prerequisite because the figure numbers are
#   recorded there.  It doesn't matter if it was created by latex or pdflatex.
#   Note that the file 'wug.html' created at the end of this process is empty;
#   it is created only so that 'make' can easily determine if the real HTML
#   files (in ../wug/) are up-to-date.
wug.html:	wug.tex wug.aux
	cp -p ../misc/icons/* wave/png/* ../../examples/stdev.c \
	 wave/misc/example.xws ../wug
	wave/scripts/wugfigures -color	# get a set of figures
	latex2html -dir ../wug -local_icons \
	 -up_url="../manuals.shtml" -up_title="Books about PhysioToolkit" wug
	cp wave/scripts/dossify-html wave/scripts/fixlinks ../wug
	cd ../wug; ./dossify-html *.html
	cd ../wug; rm -f dossify-html fixlinks *.html *.orig
	cd ../wug; rm -f .ID_MAP .IMG_PARAMS .ORIG_MAP images.*
	mv ../wug/*.pl .
	wave/scripts/fixwug.sh ../wug
	cd ../wug; ln -s wug.htm index.html
	wave/scripts/fixinfo >../../wave/wave.info
	touch wug.html

# 'make wug.pdf': format the WAVE User's Guide as PDF
wug.pdf:	wug.tex
	wave/scripts/wugfigures -pdf
	convert -scale 50\% wave/ppm/print-setup-window.ppm.gz \
	  print-setup-window.pdf
	convert -scale 50\% wave/ppm/title-with-parens.ppm.gz \
	  title-with-parens.pdf
	# epstopdf chokes on the two files above for some reason.
	# convert does not do a great job on them, but the results are
	# at least recognizable!
	rm -f wug.aux wug.idx wug.ind wug.toc
	pdflatex '\nonstopmode\input{wug}'
	makeindex wug.idx
	pdflatex '\nonstopmode\input{wug}'
	makeindex wug.idx
	pdflatex '\nonstopmode\input{wug}'

# 'make wug.ps': format the WAVE User's Guide as PostScript
wug.ps:		wug.tex
	wave/scripts/wugfigures -$(COLORS)	# get a set of figures
	rm -f wug.aux wug.idx wug.ind wug.toc
	latex '\nonstopmode\input{wug}'
	makeindex wug.idx
	latex '\nonstopmode\input{wug}'
	makeindex wug.idx
	latex '\nonstopmode\input{wug}'
	dvips $(D2PARGS) -o wug.ps wug.dvi

# 'wug.aux' is created by 'latex wug' or 'pdflatex wug' (which make slightly
# different versions of 'wug.aux').  It is a separate target because it is
# needed by 'make wug.html' (to obtain the figure numbers).  Either version
# of 'wug.aux' is acceptable for 'make wug.html'.
wug.aux:	wug.tex
	$(MAKE) wug.ps

wug.tex:	wug0.tex
	sed 's/WVRSN/$(WAVEVERSION)/' <wug0.tex | \
	 sed 's/LONGDATE/$(LONGDATE)/' >wug.tex

# 'make clean': remove intermediate and backup files
clean:
	wave/scripts/wugfigures -clean	# remove figures from this directory
	rm -rf internals.pl labels.pl wug.aux wug.dvi wug.html wug.idx \
	 wug.ilg wug.ind wug.log wug.out wug.pdf wug.ps wug.toc wug.tex \
         .xvpics wugcover* *~
