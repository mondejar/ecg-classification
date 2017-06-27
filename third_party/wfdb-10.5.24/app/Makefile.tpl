# file: Makefile.tpl		G. Moody	  23 May 2000
#				Last revised:	28 February 2014
# This section of the Makefile should not need to be changed.

CFILES = ann2rr.c bxb.c calsig.c ecgeval.c epicmp.c fir.c gqfuse.c gqpost.c \
 gqrs.c hrstats.c ihr.c mfilt.c mrgann.c mxm.c nguess.c nst.c plotstm.c \
 pscgen.c pschart.c psfd.c rdann.c rdsamp.c rr2ann.c rxr.c sampfreq.c sigamp.c \
 sigavg.c signame.c signum.c skewedit.c snip.c sortann.c sqrs.c sqrs125.c \
 stepdet.c sumann.c sumstats.c tach.c time2sec.c wabp.c wfdb-config.c \
 wfdbcat.c wfdbcollate.c wfdbdesc.c wfdbmap.c wfdbsignals.c wfdbtime.c \
 wfdbwhich.c wqrs.c wrann.c wrsamp.c xform.c
CFFILES = gqrs.conf
HFILES = signal-colors.h
XFILES = ann2rr bxb calsig ecgeval epicmp fir gqfuse gqpost \
 gqrs hrstats ihr mfilt mrgann mxm nguess nst plotstm \
 pscgen pschart psfd rdann rdsamp rr2ann rxr sampfreq sigamp \
 sigavg signame signum skewedit snip sortann sqrs sqrs125 \
 stepdet sumann sumstats tach time2sec wabp wfdb-config \
 wfdbcat wfdbcollate wfdbdesc wfdbmap wfdbsignals wfdbtime \
 wfdbwhich wqrs wrann wrsamp xform
SCRIPTS = cshsetwfdb setwfdb pnwlogin
PSFILES = pschart.pro psfd.pro 12lead.pro
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
install:	all $(BINDIR) $(PSPDIR) scripts
	rm -f pschart psfd pschart.exe psfd.exe
	$(MAKE) pschart psfd	# be sure compiled-in paths are up-to-date
	$(STRIP) pschart psfd
	$(SETXPERMISSIONS) $(XFILES)
	../install.sh $(BINDIR) $(XFILES)
	cp $(PSFILES) $(PSPDIR)
	cd $(PSPDIR); $(SETPERMISSIONS) $(PSFILES)

# 'make collect': retrieve the installed applications
collect:
	../conf/collect.sh $(BINDIR) $(XFILES) $(SCRIPTS)
	../conf/collect.sh $(PSPDIR) $(PSFILES)

# `make scripts': install customized scripts for setting WFDB path
scripts:
	sed s+/usr/local/database+$(DBDIR)+g <setwfdb >$(BINDIR)/setwfdb
	sed s+/usr/local/database+$(DBDIR)+g <cshsetwfdb >$(BINDIR)/cshsetwfdb
	sed s+/usr/local/database+$(DBDIR)+g <pnwlogin >$(BINDIR)/pnwlogin
	cd $(BINDIR); $(SETPERMISSIONS) *setwfdb; $(SETXPERMISSIONS) pnwlogin

uninstall:
	../uninstall.sh $(PSPDIR) $(PSFILES)
	../uninstall.sh $(BINDIR) $(XFILES) $(SCRIPTS)
	../uninstall.sh $(LIBDIR)

# Create directories for installation if necessary.
$(BINDIR):
	mkdir -p $(BINDIR); $(SETDPERMISSIONS) $(BINDIR)
$(PSPDIR):
	mkdir -p $(PSPDIR); $(SETDPERMISSIONS) $(PSPDIR)

# `make clean':  remove intermediate and backup files
clean:
	rm -f $(XFILES) *.o *~

# `make listing':  print a listing of WFDB applications sources
listing:
	$(PRINT) README $(MFILES) $(CFILES) $(HFILES) $(CFFILES) $(PSFILES)

# Rules for compiling applications that require non-standard options

bxb:		bxb.c
	$(CC) $(CFLAGS) bxb.c -o $@ $(LDFLAGS) -lm
hrstats:	hrstats.c
	$(CC) $(CFLAGS) hrstats.c -o $@ $(LDFLAGS) -lm
mxm:		mxm.c
	$(CC) $(CFLAGS) mxm.c -o $@ $(LDFLAGS) -lm
nguess:		nguess.c
	$(CC) $(CFLAGS) nguess.c -o $@ $(LDFLAGS) -lm
nst:		nst.c
	$(CC) $(CFLAGS) nst.c -o $@ $(LDFLAGS) -lm
plotstm:	plotstm.c
	$(CC) $(CFLAGS) plotstm.c -o $@
pschart:
	$(CC) $(CFLAGS) -DPROLOG=\"$(PSPDIR)/pschart.pro\" pschart.c -o $@ \
          $(LDFLAGS)
psfd:
	$(CC) $(CFLAGS) -DPROLOG=\"$(PSPDIR)/psfd.pro\" psfd.c -o $@ $(LDFLAGS)
sigamp:		sigamp.c
	$(CC) $(CFLAGS) sigamp.c -o $@ $(LDFLAGS) -lm
wfdbmap:      wfdbmap.c signal-colors.h
	$(CC) $(CFLAGS) wfdbmap.c -o $@ $(LDFLAGS)
wqrs:		wqrs.c
	$(CC) $(CFLAGS) wqrs.c -o $@ $(LDFLAGS) -lm
