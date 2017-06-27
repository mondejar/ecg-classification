# file: Makefile.tpl		G. Moody	 24 May 2000
#				Last revised:   26 August 2014
# This section of the Makefile should not need to be changed.

# 'make' or 'make all': compile the WFDB applications without installing them
all:		config.cache
	$(MAKE) WFDBROOT=`pwd`/build LIBDIR=`pwd`/build/lib install check

# 'make install': compile and install the WFDB software package
install:	config.cache
	cd lib;	     $(MAKE) clean install
	cd app;      $(MAKE) clean install
	cd convert;  $(MAKE) clean install
	cd data;     $(MAKE) clean install
	cd fortran;  $(MAKE) clean install
	cd psd;      $(MAKE) clean install
	-( cd wave;  $(MAKE) clean install )
	cd waverc;   $(MAKE) clean install
	-( cd xml;   $(MAKE) clean install )
	test -d doc && ( cd doc; $(MAKE) clean install )

# 'make collect': collect the installed files into /tmp/wfdb/
collect:
	cd lib;	     $(MAKE) collect
	cd app;      $(MAKE) collect
	cd convert;  $(MAKE) collect
	cd data;     $(MAKE) collect
	cd fortran;  $(MAKE) collect
	cd psd;      $(MAKE) collect
	-( cd wave;  $(MAKE) collect )
	cd waverc;   $(MAKE) collect
	-( cd xml;   $(MAKE) collect )
	test -d doc && ( cd doc; $(MAKE) collect )

uninstall:	config.cache
	cd app;      $(MAKE) uninstall
	cd convert;  $(MAKE) uninstall
	cd data;     $(MAKE) uninstall
	cd fortran;  $(MAKE) uninstall
	cd lib;	     $(MAKE) uninstall
	cd psd;      $(MAKE) uninstall
	cd wave;     $(MAKE) uninstall
	cd waverc;   $(MAKE) uninstall
	cd xml;      $(MAKE) uninstall
	test -d doc && ( cd doc; $(MAKE) uninstall )
	./uninstall.sh $(WFDBROOT)

# 'make clean': remove binaries, other cruft from source directories
clean:
	cd app;      $(MAKE) clean
	cd checkpkg; $(MAKE) clean
	cd convert;  $(MAKE) clean
	cd data;     $(MAKE) clean
	cd examples; $(MAKE) clean
	cd fortran;  $(MAKE) clean
	cd lib;      $(MAKE) clean
	cd psd;      $(MAKE) clean
	cd wave;     $(MAKE) clean
	cd waverc;   $(MAKE) clean
	cd xml;	     $(MAKE) clean
	test -d doc && ( cd doc; $(MAKE) clean )
	cd conf; rm -f *~ prompt site.def site-slib.def
	rm -f *~ config.cache */*.exe $(PACKAGE)-*.spec
	rm -rf build

# 'make config.cache': check configuration
config.cache:
	exec ./configure
	@echo "(Ignore any error that may appear on the next line.)"
	@false	# force an immediate exit from `make'

conf/prompt:
	echo -n >echo.out
	-test -s echo.out && ln -sf prompt-c conf/prompt
	-test -s echo.out || ln -sf prompt-n conf/prompt
	rm echo.out

# 'make test' or 'make test-all': compile the WFDB applications without
# installing them (installs the dynamically-linked WFDB library and includes
# into subdirectories of $(HOME)/wfdb-test)
test test-all: $(HOME)/wfdb-test/include $(HOME)/wfdb-test/lib
	$(MAKE) WFDBROOT=$(HOME)/wfdb-test all

# 'make test-install': compile and install the WFDB software package into
# subdirectories of $(HOME)/wfdb-test
test-install: $(TESTDIRS)
	$(MAKE) WFDBROOT=$(HOME)/wfdb-test install

# 'make check': test currently installed version of the WFDB software package
check:		config.cache conf/prompt
	cd checkpkg; $(MAKE) all

# Create directories for test installation if necessary.
TESTDIRS = $(HOME)/wfdb-test/bin $(HOME)/wfdb-test/database \
 $(HOME)/wfdb-test/help $(HOME)/wfdb-test/include $(HOME)/wfdb-test/lib

$(HOME)/wfdb-test:
	mkdir -p $(HOME)/wfdb-test; $(SETDPERMISSIONS) $(HOME)/wfdb-test
$(HOME)/wfdb-test/bin:		$(HOME)/wfdb-test
	mkdir -p $(HOME)/wfdb-test/bin; \
	 $(SETDPERMISSIONS) $(HOME)/wfdb-test/bin
$(HOME)/wfdb-test/database:	$(HOME)/wfdb-test
	mkdir -p $(HOME)/wfdb-test/database; \
	 $(SETDPERMISSIONS) $(HOME)/wfdb-test/database
$(HOME)/wfdb-test/help:		$(HOME)/wfdb-test
	mkdir -p $(HOME)/wfdb-test/help; \
	 $(SETDPERMISSIONS) $(HOME)/wfdb-test/help
$(HOME)/wfdb-test/include:	$(HOME)/wfdb-test
	mkdir -p $(HOME)/wfdb-test/include; \
	 $(SETDPERMISSIONS) $(HOME)/wfdb-test/include
$(HOME)/wfdb-test/lib:		$(HOME)/wfdb-test
	mkdir -p $(HOME)/wfdb-test/lib; \
	 $(SETDPERMISSIONS) $(HOME)/wfdb-test/lib

# 'make tarballs': clean up the source directories, run ./configure with
# default settings, then make a pair of gzipped tar source archives of the WFDB
# software package (with and without the documentation), and check that the
# MANIFEST (list of files in the package) is correct.
tarballs:	clean
	./configure
	$(MAKE) clean
	rm -f ../$(PACKAGE)-MANIFEST ../$(PACKAGE).tar.gz \
	  ../$(PACKAGE)-no-docs.tar.gz
	cd lib; $(SETPERMISSIONS) *.h
	cd ..; export COPYFILE_DISABLE=true; \
	  tar --create --file $(PACKAGE).tar.gz --verbose --gzip \
          '--exclude=.git*' $(PACKAGE) 2>&1 | \
	  sed "s+^a ++" | sed s+${PACKAGE}/++ | \
          tee $(PACKAGE)-MANIFEST
	cd ..; tar --create --file $(PACKAGE)-no-docs.tar.gz \
	  --verbose --gzip \
          '--exclude=$(PACKAGE)/*doc' \
	  '--exclude=.git*' $(PACKAGE)
	./check-manifest $(PACKAGE)

# 'make bin-tarball': make a gzipped tar archive of the WFDB software package
# binaries and other installed files
bin-tarball:	install collect
	rm -rf /tmp/$(PACKAGE)-$(ARCH)
	mv /tmp/wfdb /tmp/$(PACKAGE)-$(ARCH)
	cd /tmp; tar cfvz $(PACKAGE)-$(ARCH).tar.gz \
	 $(PACKAGE)-$(ARCH)
	mv /tmp/$(PACKAGE)-$(ARCH).tar.gz ..
	rm -rf /tmp/$(PACKAGE)-$(ARCH)

# 'make doc-tarball': make a gzipped tar archive of formatted documents
# (requires many freely-available utilities that are not part of this
# package;  see doc/Makefile.top for details)
doc-tarball:
	cd doc; $(MAKE) tarball

# 'make rpms': make source and binary RPMs
RPMROOT=$(HOME)/rpmbuild

rpms:		tarballs
	mkdir -p $(RPMROOT)/BUILD $(RPMROOT)/RPMS $(RPMROOT)/SOURCES \
	 $(RPMROOT)/SPECS $(RPMROOT)/SRPMS
	cp -p ../$(PACKAGE).tar.gz $(RPMROOT)/SOURCES
	sed s/VERSION/$(VERSION)/g <wfdb.spec | \
	 sed s/MAJOR/$(MAJOR)/g | sed s/MINOR/$(MINOR)/g | \
	 sed s/RPMRELEASE/$(RPMRELEASE)/ >$(PACKAGE)-$(RPMRELEASE).spec
	cp -p $(PACKAGE)-$(RPMRELEASE).spec $(RPMROOT)/SPECS
	cd; if [ -e .rpmmacros ]; then cp -p .rpmmacros ..rpmmacros; fi
	cp conf/rpm.mc $(HOME)/.rpmmacros
	if [ -x /usr/bin/rpmbuild ]; \
	 then rpmbuild -ba $(PACKAGE)-$(RPMRELEASE).spec; \
	 else echo "rpmbuild not found in /usr/bin; attempting to use rpm"; \
	  rpm -ba $(PACKAGE)-$(RPMRELEASE).spec; fi
	mv $(RPMROOT)/RPMS/*/wfdb*-$(VERSION)-$(RPMRELEASE).*.rpm ..
	mv $(RPMROOT)/SRPMS/$(PACKAGE)-$(RPMRELEASE).src.rpm ..
	rm -f $(PACKAGE)-$(RPMRELEASE).spec $(HOME)/.rpmmacros
	cd; if [ -e ..rpmmacros ]; then mv ..rpmmacros .rpmmacros; fi
	@echo "Remember to sign the RPMs by"
	@echo "   cd ..; rpm --addsign wfdb*$(VERSION)*rpm"
