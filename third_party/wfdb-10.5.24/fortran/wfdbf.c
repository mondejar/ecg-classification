/* file: wfdbf.c	G. Moody	 23 August 1995
			Last revised:     21 July 2013		wfdblib 10.5.19

_______________________________________________________________________________
wfdbf: Fortran wrappers for the WFDB library functions
Copyright (C) 1995-2013 George B. Moody

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

General notes on using the wrapper functions

Wrappers are provided below for all of the WFDB library functions except
setmsheader.  See the sample program (example.f, in this directory)
to see how the wrappers are used.

Include the statements
	implicit integer(a-z)
	real aduphys, getbasecount, getcfreq, sampfreq
(or the equivalent for your dialect of Fortran) in your Fortran program to
ensure that these functions (except for the four listed in the second
statement) will be understood to return integer*4 values (equivalent to C
`long' values).

Note that Fortran arrays are 1-based, and C arrays are 0-based.  Signal and
annotator numbers passed to these functions must be 0-based.

If you are using a UNIX Fortran compiler, or a Fortran-to-C translator, note
that the trailing `_' in these function names should *not* appear in your
Fortran program;  thus, for example, `annopen_' should be invoked as
`annopen'.  UNIX Fortran compilers and translators append a `_' to the
names of all external symbols referenced in Fortran source files when
generating object files.  Thus the linker can recognize that annopen_
(defined below) is the function required by a Fortran program that invokes
`annopen';  if the Fortran program were to invoke `annopen_', the linker
would search (unsuccessfully) for a function named `annopen__'.

If you are using a Fortran compiler that does not follow this convention,
you are on your own.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>
#ifndef BSD
# include <string.h>
#else		/* for Berkeley UNIX only */
# include <strings.h>
#endif

/* If a WFDB_Sample is the same size as a long (true except for MS-DOS PCs and
   a few less common systems), considerable efficiency gains are possible.
   Otherwise, define REPACK below to compile the code needed to repack arrays
   of DB_Samples into long arrays. */
/* #define REPACK */

/* If Fortran strings are terminated by spaces rather than by null characters,
   define FIXSTRINGS. */
/* #define FIXSTRINGS */

#ifdef FIXSTRINGS
/* This function leaks memory!  Ideally we would like to free(t) once *t is
   no longer needed.  If this is a concern, we might push all values assigned
   to t onto a stack and free them all on exit.  In practice we are usually
   dealing with a small number of short strings. */
char *fcstring(char *s)	/* change final space to null */
{
    char *p = s, *t;

    while (*s && *s != ' ')
	s++;
    t = calloc(1, s-p+1);
    if (s > p) strncpy(t, p, s-p);
    return (t);
}

char *cfstring(char *s)	/* change final null to space */
{
    char *p = s;

    while (*s)
	s++;
    *s = ' ';
    return (p);
}
#else
#define fcstring(s)	(s)
#define cfstring(s)	(s)
#endif

/* Static data shared by the wrapper functions.  Since Fortran does not support
   composite data types (such as C `struct' types), these are defined here, and
   functions for setting and retrieving data from these structures are provided
   below. */

static WFDB_Siginfo sinfo[WFDB_MAXSIG];
static WFDB_Calinfo cinfo;
static WFDB_Anninfo ainfo[WFDB_MAXANN*2];

/* The functions setanninfo_, setsiginfo_, and getsiginfo_ do not have direct
   equivalents in the WFDB library;  they are provided in order to permit
   Fortran programs to read and write data structures passed to and from
   several of the WFDB library functions.  Since the contents of these
   structures are directly accessible by C programs, these functions are not
   needed in the C library.
*/

long setanninfo_(long int *a, char *name, long int *stat)
{
    if (0 <= *a && *a < WFDB_MAXANN*2) {
	ainfo[*a].name = fcstring(name);
	ainfo[*a].stat = *stat;
	return (0L);
    }
    else
	return (-1L);
}

long getsiginfo_(long int *s, char *fname, char *desc, char *units, double *gain, long int *initval, long int *group, long int *fmt, long int *spf, long int *bsize, long int *adcres, long int *adczero, long int *baseline, long int *nsamp, long int *cksum)
{
    if (0 <= *s && *s < WFDB_MAXSIG) {
	fname = sinfo[*s].fname;
	desc = sinfo[*s].desc;
	units = sinfo[*s].units;
	*gain = sinfo[*s].gain;
	*initval = sinfo[*s].initval;
	*group = sinfo[*s].group;
	*fmt = sinfo[*s].fmt;
	*spf = sinfo[*s].spf;
	*bsize = sinfo[*s].bsize;
	*adcres = sinfo[*s].adcres;
	*adczero = sinfo[*s].adczero;
	*baseline = sinfo[*s].baseline;
	*nsamp = sinfo[*s].nsamp;
	*cksum = sinfo[*s].cksum;
	return (0L);
    }
    else
	return (-1L);
}

long setsiginfo_(long int *s, char *fname, char *desc, char *units, double *gain, long int *initval, long int *group, long int *fmt, long int *spf, long int *bsize, long int *adcres, long int *adczero, long int *baseline, long int *nsamp, long int *cksum)
{
    if (0 <= *s && *s < WFDB_MAXSIG) {
	sinfo[*s].fname = fname;
	sinfo[*s].desc = desc;
	sinfo[*s].units = units;
	sinfo[*s].gain = *gain;
	sinfo[*s].initval = *initval;
	sinfo[*s].group = *group;
	sinfo[*s].fmt = *fmt;
	sinfo[*s].spf = *spf;
	sinfo[*s].bsize = *bsize;
	sinfo[*s].adcres = *adcres;
	sinfo[*s].adczero = *adczero;
	sinfo[*s].baseline = *baseline;
	sinfo[*s].nsamp = *nsamp;
	sinfo[*s].cksum = *cksum;
	return (0L);
    }
    else
	return (-1L);
}

/* Before using annopen_, set up the annotation information structures using
   setanninfo_. */
long annopen_(char *record, long int *nann)
{
    return (annopen(fcstring(record), ainfo, (unsigned int)(*nann)));
}

/* After using isigopen_ or osigopen_, use getsiginfo_ to obtain the contents
   of the signal information structures if necessary. */
long isigopen_(char *record, long int *nsig)
{
    return (isigopen(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

long osigopen_(char *record, long int *nsig)
{
    return (osigopen(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

/* Before using osigfopen_, use setsiginfo_ to set the contents of the signal
   information structures. */
long osigfopen_(long int *nsig)
{
    return (osigfopen(sinfo, (unsigned int)(*nsig)));
}

/* Before using wfdbinit_, use setanninfo_ and setsiginfo_ to set the contents
   of the annotation and signal information structures. */
long wfdbinit_(char *record, long int *nann, long int *nsig)
{
    return (wfdbinit(fcstring(record), ainfo, (unsigned int)(*nann),
		     sinfo, (unsigned int)(*nsig)));
}

long findsig_(char *signame)
{
    return (findsig(fcstring(signame)));
}

long getspf_(long int *dummy)
{
    return (getspf());
}

long setgvmode_(long int *mode)
{
    setgvmode((int)(*mode));
    return (0L);
}

long getgvmode_(long int *dummy)
{
    return (getgvmode());
}

long setifreq_(double *freq)
{
    return (setifreq((WFDB_Frequency)*freq));
}

double getifreq_(long int *dummy)
{
    return (getifreq());
}

long getvec_(long int *long_vector)
{
#ifndef REPACK
    return (getvec((WFDB_Sample *)long_vector));
#else
    WFDB_Sample v[WFDB_MAXSIG];
    int i, j;

    i = getvec(v);
    for (j = 0; j < i; j++)
	long_vector[i] = v[i];
    return (i);
#endif
}

long getframe_(long int *long_vector)
{
#ifndef REPACK
    return (getframe((WFDB_Sample *)long_vector));
#else
    WFDB_Sample v[WFDB_MAXSIG*WFDB_MAXSPF];
    int i, j, k;

    i = getframe(v);
    for (j = 0; j < i; j += k)
	for (k = 0; k < sinfo[i].spf; k++)
	    long_vector[j+k] = v[j+k];
    return (i);
#endif
}

long putvec_(long int *long_vector)
{
#ifndef REPACK
    return (putvec((WFDB_Sample *)long_vector));
#else
    WFDB_Sample v[WFDB_MAXSIG*WFDB_MAXSPF];
    int i;

    for (i = 0; i < WFDB_MAXSIG*WFDB_MAXSPF; i++)
	v[i] = long_vector[i];
    return (putvec(v));
#endif
}

long getann_(long int *annotator, long int *time, long int *anntyp, long int *subtyp, long int *chan, long int *num, char *aux)
{
    static WFDB_Annotation iann;
    int i, j;

    i = getann((WFDB_Annotator)(*annotator), &iann);
    if (i == 0) {
	*time = iann.time;
	*anntyp = iann.anntyp;
	*subtyp = iann.subtyp;
	*chan = iann.chan;
	*num = iann.num;
	aux = iann.aux;
    }
    return (i);
}

long ungetann_(long int *annotator, long int *time, long int *anntyp, long int *subtyp, long int *chan, long int *num, char *aux)
{
    static WFDB_Annotation oann;
    int i, j;

    oann.time = *time;
    oann.anntyp = *anntyp;
    oann.subtyp = *subtyp;
    oann.chan = *chan;
    oann.num = *num;
    oann.aux = aux;
    return (ungetann((WFDB_Annotator)(*annotator), &oann));
}

long putann_(long int *annotator, long int *time, long int *anntyp, long int *subtyp, long int *chan, long int *num, char *aux)
{
    static WFDB_Annotation oann;
    int i, j;
    char *p, *q = NULL;

    oann.time = *time;
    oann.anntyp = *anntyp;
    oann.subtyp = *subtyp;
    oann.chan = *chan;
    oann.num = *num;
    if (aux) {
	p = fcstring(aux);
	q = calloc(strlen(p)+2, 1);
	*q = strlen(p);
	strcpy(q+1, p);
    }
    oann.aux = q;
    i = putann((WFDB_Annotator)(*annotator), &oann);
    if (q) free(q);
    return (i);
}

long isigsettime_(long int *time)
{
    return (isigsettime((WFDB_Time)(*time)));
}

long isgsettime_(long int *group, long int *time)
{
    return (isgsettime((WFDB_Group)(*group), (WFDB_Time)(*time)));
}

long tnextvec_(long int *signal, long int *time)
{
    return (tnextvec((WFDB_Signal)(*signal), (WFDB_Time)(*time)));
} 

long iannsettime_(long int *time)
{
    return (iannsettime((WFDB_Time)(*time)));
}

long ecgstr_(long int *code, char *string)
{
    strcpy(string, ecgstr((int)(*code)));
    cfstring(string);
    return (0L);
}

long strecg_(char *string)
{
    return (strecg(fcstring(string)));
}

long setecgstr_(long int *code, char *string)
{
    return (setecgstr((int)(*code), fcstring(string)));
}

long annstr_(long int *code, char *string)
{
    strcpy(string, annstr((int)(*code)));
    cfstring(string);
    return (0L);
}

long strann_(char *string)
{
    return (strann(fcstring(string)));
}

long setannstr_(long int *code, char *string)
{
    return (setannstr((int)(*code), fcstring(string)));
}

long anndesc_(long int *code, char *string)
{
    strcpy(string, anndesc((int)(*code)));
    cfstring(string);
    return (0L);
}

long setanndesc_(long int *code, char *string)
{
    return (setanndesc((int)(*code), fcstring(string)));
}

long setafreq_(double *freq)
{
    setafreq(*freq);
    return (0L);
}

double getafreq_(long int *dummy)
{
    return (getafreq());
}

long iannclose_(long int *annotator)
{
    iannclose((WFDB_Annotator)(*annotator));
    return (0L);
}

long oannclose_(long int *annotator)
{
    oannclose((WFDB_Annotator)(*annotator));
    return (0L);
}

/* The functions below can be used in place of the macros defined in
   <wfdb/ecgmap.h>. */

long isann_(long int *anntyp)
{   
    return (wfdb_isann((int)*anntyp));
}

long isqrs_(long int *anntyp)
{   
    return (wfdb_isqrs((int)*anntyp));
}

long setisqrs_(long int *anntyp, long int *value)
{   
    wfdb_setisqrs((int)*anntyp, (int)*value);
    return (0L);
}

long map1_(long int *anntyp)
{   
    return (wfdb_map1((int)*anntyp));
}

long setmap1_(long int *anntyp, long int *value)
{   
    wfdb_setmap1((int)*anntyp, (int)*value);
    return (0L);
}

long map2_(long int *anntyp)
{   
    return (wfdb_map2((int)*anntyp));
}

long setmap2_(long int *anntyp, long int *value)
{   
    wfdb_setmap2((int)*anntyp, (int)*value);
    return (0L);
}

long ammap_(long int *anntyp)
{   
    return (wfdb_ammap((int)*anntyp));
}

long mamap_(long int *anntyp, long int *subtyp)
{   
    return (wfdb_mamap((int)*anntyp, (int)*subtyp));
}

long annpos_(long int *anntyp)
{   
    return (wfdb_annpos((int)*anntyp));
}

long setannpos_(long int *anntyp, long int *value)
{   
    wfdb_setannpos((int)*anntyp, (int)*value);
    return (0L);
}

long timstr_(long int *time, char *string)
{
    strcpy(string, timstr((WFDB_Time)(*time)));
    cfstring(string);
    return (0L);
}

long mstimstr_(long int *time, char *string)
{
    strcpy(string, mstimstr((WFDB_Time)(*time)));
    cfstring(string);
    return (0L);
}

long strtim_(char *string)
{
    return (strtim(fcstring(string)));
}

long datstr_(long int *date, char *string)
{
    strcpy(string, datstr((WFDB_Date)(*date)));
    cfstring(string);
    return (0L);
}

long strdat_(char *string)
{
    return (strdat(fcstring(string)));
}

long adumuv_(long int *signal, long int *ampl)
{
    return (adumuv((WFDB_Signal)(*signal), (WFDB_Sample)(*ampl)));
}

long muvadu_(long int *signal, long int *microvolts)
{
    return (muvadu((WFDB_Signal)(*signal), (int)(*microvolts)));
}

double aduphys_(long int *signal, long int *ampl)
{
    return (aduphys((WFDB_Signal)(*signal), (WFDB_Sample)(*ampl)));
}

long physadu_(long int *signal, double *v)
{
    return (physadu((WFDB_Signal)(*signal), *v));
}

long sample_(long int *signal, long int *t)
{
    return (sample((WFDB_Signal)(*signal), (WFDB_Time)(*t)));
}

long sample_valid_(long int *dummy)
{
    return (sample_valid());
}

long calopen_(char *calibration_filename)
{
    return (calopen(fcstring(calibration_filename)));
}

long getcal_(char *description, char *units, double *low, double *high, double *scale, long int *caltype)
{
    if (getcal(fcstring(description), fcstring(units), &cinfo) == 0) {
	*low = cinfo.low;
	*high = cinfo.high;
	*scale = cinfo.scale;
	*caltype = cinfo.caltype;
	return (0L);
    }
    else
	return (-1L);
}

long putcal_(char *description, char *units, double *low, double *high, double *scale, long int *caltype)
{
    cinfo.sigtype = fcstring(description);
    cinfo.units = fcstring(units);
    cinfo.low = *low;
    cinfo.high = *high;
    cinfo.scale = *scale;
    cinfo.caltype = *caltype;
    return (putcal(&cinfo));
}

long newcal_(char *calibration_filename)
{
    return (newcal(fcstring(calibration_filename)));
}

long flushcal_(long int *dummy)
{
    flushcal();
    return (0L);
}

long getinfo_(char *record, char *string)
{
    strcpy(string, getinfo(fcstring(record)));
    cfstring(string);
    return (0L);
}

long putinfo_(char *string)
{
    return (putinfo(fcstring(string)));
}

long setinfo_(char *record)
{
    return (setinfo(fcstring(record)));
}

long wfdb_freeinfo_(long int *dummy)
{
    wfdb_freeinfo();
    return (0L);
}

long newheader_(char *record)
{
    return (newheader(fcstring(record)));
}

/* Before using setheader_, use setsiginfo to set the contents of the signal
   information structures. */
long setheader_(char *record, long int *nsig)
{
    return (setheader(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

/* No wrappers are provided for setmsheader or getseginfo. */

long wfdbgetskew_(long int *s)
{
    return (wfdbgetskew((WFDB_Signal)(*s)));
}

long wfdbsetiskew_(long int *s, long int *skew)
{
    wfdbsetiskew((WFDB_Signal)(*s), (int)(*skew));
    return (0L);
}

long wfdbsetskew_(long int *s, long int *skew)
{
    wfdbsetskew((WFDB_Signal)(*s), (int)(*skew));
    return (0L);
}

long wfdbgetstart_(long int *s)
{
    return (wfdbgetstart((WFDB_Signal)(*s)));
}

long wfdbsetstart_(long int *s, long int *bytes)
{
    wfdbsetstart((WFDB_Signal)(*s), *bytes);
    return (0L);
}

long wfdbputprolog_(char *prolog, long int *bytes, long int *signal)
{
    return (wfdbputprolog(fcstring(prolog), (long)*bytes,(WFDB_Signal)*signal));
}

long wfdbquit_(long int *dummy)
{
    wfdbquit();
    return (0L);
}

double sampfreq_(char *record)
{
    return (sampfreq(fcstring(record)));
}

long setsampfreq_(double *frequency)
{
    return (setsampfreq((WFDB_Frequency)(*frequency)));
}

double getcfreq_(long int *dummy)
{
    return (getcfreq());
}

long setcfreq_(double *frequency)
{
    setcfreq((WFDB_Frequency)(*frequency));
    return (0L);
}

double getbasecount_(long int *dummy)
{
    return (getbasecount());
}

long setbasecount_(double *count)
{
    setbasecount(*count);
    return (0L);
}

long setbasetime_(char *string)
{
    return (setbasetime(fcstring(string)));
}

long wfdbquiet_(long int *dummy)
{
    wfdbquiet();
    return (0L);
}

long wfdbverbose_(long int *dummy)
{
    wfdbverbose();
    return (0L);
}

long wfdberror_(char *string)
{
    strcpy(string, wfdberror());
    cfstring(string);
    return (0L);
}

long setwfdb_(char *string)
{
    setwfdb(fcstring(string));
    return (0L);
}

long getwfdb_(char *string)
{
    strcpy(string, getwfdb());
    cfstring(string);
    return (0L);
}

long resetwfdb_(long int *dummy)
{
    resetwfdb();
    return (0L);
}

long setibsize_(long int *input_buffer_size)
{
    return (setibsize((int)(*input_buffer_size)));
}

long setobsize_(long int *output_buffer_size)
{
    return (setobsize((int)(*output_buffer_size)));
}

long wfdbfile_(char *file_type, char *record, char *pathname)
{
    strcpy(pathname, wfdbfile(fcstring(file_type), fcstring(record)));
    cfstring(pathname);
    return (0L);
}

long wfdbflush_(long int *dummy)
{
    wfdbflush();
    return (0L);
}

long wfdbmemerr_(long int *exit_if_error)
{
    wfdbmemerr((int)(*exit_if_error));
    return (0L);
}

long wfdbversion_(char *version)
{
    strcpy(version, wfdbversion());
    cfstring(version);
    return (0L);
}

long wfdbldflags_(char *ldflags)
{
    strcpy(ldflags, wfdbldflags());
    cfstring(ldflags);
    return (0L);
}

long wfdbcflags_(char *cflags)
{
    strcpy(cflags, wfdbcflags());
    cfstring(cflags);
    return (0L);
}

long wfdbdefwfdb_(char *defwfdb)
{
    strcpy(defwfdb, wfdbdefwfdb());
    cfstring(defwfdb);
    return (0L);
}

long wfdbdefwfdbcal_(char *defwfdbcal)
{
    strcpy(defwfdbcal, wfdbdefwfdbcal());
    cfstring(defwfdbcal);
    return (0L);
}
