# file: Makefile.tpl		G. Moody	  23 May 2000
#				Last revised:	23 February 2003
# This section of the Makefile should not need to be changed.

DBFILES = 100a.atr 100s.atr 100s.dat *.hea *list wfdbcal

all:
	@echo Nothing to be made in `pwd`.

install:	$(DBDIR) $(DBDIR)/pipe $(DBDIR)/tape
	cp $(DBFILES) $(DBDIR)
	cp pipe/* $(DBDIR)/pipe
	cp tape/* $(DBDIR)/tape
	-cd $(DBDIR); $(SETPERMISSIONS) $(DBFILES)
	-cd $(DBDIR); ln -sf wfdbcal dbcal
	-cd $(DBDIR)/pipe; $(SETPERMISSIONS) *
	-cd $(DBDIR)/tape; $(SETPERMISSIONS) *

# 'make collect': retrieve the installed files
collect:
	../conf/collect.sh $(DBDIR) $(DBFILES) wfdbcal dbcal
	cd pipe; ../../conf/collect.sh $(DBDIR)/pipe *
	cd tape; ../../conf/collect.sh $(DBDIR)/tape *
	
uninstall:
	../uninstall.sh $(DBDIR) $(DBFILES) dbcal

$(DBDIR):
	mkdir $(DBDIR); $(SETDPERMISSIONS) $(DBDIR)
$(DBDIR)/pipe:
	mkdir $(DBDIR)/pipe; $(SETDPERMISSIONS) $(DBDIR)/pipe
$(DBDIR)/tape:
	mkdir $(DBDIR)/tape; $(SETDPERMISSIONS) $(DBDIR)/tape

listing:
	$(PRINT) README Makefile makefile.dos

clean:
	rm -f *~
