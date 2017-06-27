# This example works with either f77 (g77) or gfortran.
example:	example.f wfdbf.c
	$(F77) -o example -DFIXSTRINGS example.f wfdbf.c -lwfdb

wfdbf.o:	wfdbf.c
	$(CC) -g -O -DFIXSTRINGS wfdbf.c


# If you have `f2c', but not `f77', use `make example-alt' instead of `make'.
example-alt:	example.f wfdbf.c
	f2c example.f
	$(CC) -o example example.c wfdbf.c -lf2c -lm -lwfdb


# 'make install' copies the wrapper sources into the directory where the
# WFDB headers are also installed.
install:
	../install.sh $(INCDIR)/wfdb wfdbf.c
	$(SETPERMISSIONS) $(INCDIR)/wfdb/wfdbf.c

collect:
	../conf/collect.sh $(INCDIR)/wfdb wfdbf.c

uninstall:
	../uninstall.sh $(INCDIR)/wfdb wfdbf.c

clean:
	rm -f example example.c *.o *~
