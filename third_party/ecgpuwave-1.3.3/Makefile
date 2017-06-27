# file: Makefile	P. Laguna and G. Moody	6 February 2002
#			Last revised:	        11 January 2016
# -----------------------------------------------------------------------------
# UNIX 'make' description file for compiling ecgpuwave
# Copyright (C) 2002-2006 Pablo Laguna and George B. Moody
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place - Suite 330, Boston, MA 02111-1307, USA.
#
# You may contact the author by e-mail (george@mit.edu) or postal mail
# (MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
# please visit PhysioNet (http://www.physionet.org/).
# _____________________________________________________________________________

# This file is used with the UNIX `make' command to compile ecgpuwave.  Install
# the WFDB library (see http://www.physionet.org/physiotools/wfdb.shtml), and
# copy wfdbf.c from the 'fortran' subdirectory of the WFDB sources into this
# directory, before attempting to compile ecgpuwave.
#
# 11 January 2016 (BEM):
# Updated wfdbf.c for compatibility with modern systems (including
# x86-64), and added the -fno-automatic option to avoid undefined
# behavior when compiling with gfortran.
#
# 21 July 2013 (GBM):
# Incorporated modifications to dades.f, ecgpuwave.f, and this Makefile for
# compatibility with gfortran (thanks to Roberto Sassi!)
#
# 24 March 2010 (GBM):
# The -m32 option for F77 and CC forces compilation of 32-bit executables,
# even on a 64-bit platform, because 64-bit executables of ecgpuwave fail
# due to a mismatch in the default sizes of integer variables in Fortran and
# int variables in C.  This can probably be remedied by declaring all
# of ecgpuwave's integer variables as INTEGER*8 when compiling a 64-bit
# executable, but that would cause worse problems on 32-bit platforms.
# This has been tested using g77 and gcc on 32- and 64-bit Linux (Fedora 12).
# If your 32-bit compilers don't support the -m32 option, remove it.

CC=gcc
CFLAGS=-g -O
F77=gfortran -std=legacy -ffixed-line-length-none -fno-automatic
FFLAGS=-g -O -Wextra

# On an x86-64 system, if you need to reproduce the exact results
# produced on an IA-32 system, you can uncomment the following line:
#FFLAGS=-g -O -Wextra -mfpmath=387

WFDB_CFLAGS=`wfdb-config --cflags`
WFDB_LIBS=`wfdb-config --libs`
INCDIR=`wfdb-config --cflags | sed 's/.*-I//;s/ .*//'`

prefix=/usr/local
BINDIR=$(prefix)/bin

OFILES= ecgpuwave.o principal.o aldetqrs.o graf.o dades.o impregraf.o \
	 lgraf.o prosen.o int_qt.o punts.o l_impregraf.o wfdbf.o

all: ecgpuwave

ecgpuwave:	ecgpuwave.f $(OFILES)
	$(F77) $(FFLAGS) $(LDFLAGS) -o ecgpuwave $(OFILES) $(WFDB_LIBS)

install:	ecgpuwave
	mkdir -p $(BINDIR)
	cp ecgpuwave $(BINDIR)
	chmod 755 $(BINDIR)/ecgpuwave

check:		ecgpuwave
	./ecgpuwave -r 100s -a test
	@if cmp -s 100s.exp 100s.test; then echo Test 1 passed; \
	 else echo Test 1 Failed; fi
	bxb -r 100s -a test exp -O -f 0 -w s1 >bxb.out 2>&1
	@if cmp -s 100s.exp 100s.bxb;  then echo Test 2 passed; \
	 else echo Test 2 Failed; exit 1; fi

.f.o:
	$(F77) $(FFLAGS) -c -o $@ $<

# Use the copy of wfdbf.c that was installed with the WFDB library if it is
# newer than the copy included with the ecgpuwave sources.
wfdbf.o: wfdbf.c
	if [ $(INCDIR)/wfdb/wfdbf.c -nt wfdbf.c ]; \
	 then cp -p $(INCDIR)/wfdb/wfdbf.c .; fi
	$(CC) $(CFLAGS) -c -DFIXSTRINGS -o $@ $(WFDB_CFLAGS) wfdbf.c
	@if [ wfdbf.c -nt $(INCDIR)/wfdb/wfdbf.c ]; then \
	 echo Warning: the installed WFDB library may need to be updated.; fi

clean:
	rm -f ecgpuwave *.o *~ 100s.bxb 100s.test bxb.out fort.20 fort.21

