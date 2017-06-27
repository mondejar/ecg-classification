C file: example.f		G. Moody	23 August 1995
C                               Last revised:  23 February 2006
C
C -----------------------------------------------------------------------------
C Sample program illustrating use of Fortran wrappers for the WFDB library
C Copyright (C) 1995-2006 George B. Moody
C
C This program is free software; you can redistribute it and/or modify it under
C the terms of the GNU General Public License as published by the Free Software
C Foundation; either version 2 of the License, or (at your option) any later
C version.
C
C This program is distributed in the hope that it will be useful, but WITHOUT
C ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
C FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
C details.
C
C You should have received a copy of the GNU General Public License along with
C this program; if not, write to the Free Software Foundation, Inc., 59 Temple
C Place - Suite 330, Boston, MA 02111-1307, USA.
C
C You may contact the author by e-mail (george@mit.edu) or postal mail
C (MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
C please visit PhysioNet (http://www.physionet.org/).
C _____________________________________________________________________________
C
C This program is a slightly elaborated version of the C example shown in
C section 1.1 of the WFDB Programmer's Guide.  It uses the WFDB library to open
C record 100s (a sample record, provided in the `data' directory at the same
C level as this one).  The program prints the number of signals, the sampling
C frequency, and the first ten samples from each signal.
C
C To compile this program on a UNIX system, type:
C    f77 example.f wfdbf.c -lwfdb
C To run the resulting executable file, type:
C    a.out
C Compare the output with that shown in section 1.4 of the WFDB Programmer's
C Guide.
C
C If your Fortran compiler is a very old one, you might need to replace the
C tabs at the beginnings of the lines below with spaces.

C The next two lines specify the data types returned by the wrappers.
	implicit integer(a-z)
	real aduphys, getbasecount, getcfreq, sampfreq

	integer i, v(32), g
	real f

C Open up to 32 signals from record 100s.  (There are only 2 signals in this
C record, however.)  Note how we force the string argument to end with an
C explicit NULL (CHAR(0));  this is necessary for any strings passed to C
C functions.
	i = isigopen("100s"//CHAR(0), 32)
	write (6,1) i
 1	format("Number of signals in record 100s = ", i2)

C Check out the sampling frequency of record 100s.  The returned value is
C the sampling frequency in Hz, represented as a C double (Fortran real) value.
	f = sampfreq("100s"//CHAR(0))
	write (6,2) f
 2	format("Sampling frequency = ", f6.2)

C Read the first 10 samples from each signal.  The value returned from getvec
C is the number of signals;  the samples themselves are filled into the v
C array by getvec:  v(1) contains a sample for signal 0, v(2) for signal 1.
	do i = 1, 10
	 g = getvec(v)
	 write (6,3) v(1), v(2)
 3	format("v(1) = ", i4, "    v(2) = ", i4)
	end do
	end
