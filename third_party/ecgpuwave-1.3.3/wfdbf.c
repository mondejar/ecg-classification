/* file: wfdbf.c	G. Moody	 23 August 1995
			Last revised:    11 January 2016	wfdblib 10.5.25

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
	double precision aduphys, getbasecount, getcfreq, sampfreq
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

/* Define the C types equivalent to the corresponding Fortran types.
   The below definitions should be correct for modern systems; some
   older Fortran compilers may use different types. */
#ifndef INTEGER
#define INTEGER int
#endif
#ifndef DOUBLE_PRECISION
#define DOUBLE_PRECISION double
#endif

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

INTEGER setanninfo_(INTEGER *a, char *name, INTEGER *stat)
{
    if (0 <= *a && *a < WFDB_MAXANN*2) {
	ainfo[*a].name = fcstring(name);
	ainfo[*a].stat = *stat;
	return (0L);
    }
    else
	return (-1L);
}

INTEGER getsiginfo_(INTEGER *s, char *fname, char *desc, char *units, DOUBLE_PRECISION *gain, INTEGER *initval, INTEGER *group, INTEGER *fmt, INTEGER *spf, INTEGER *bsize, INTEGER *adcres, INTEGER *adczero, INTEGER *baseline, INTEGER *nsamp, INTEGER *cksum)
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

INTEGER setsiginfo_(INTEGER *s, char *fname, char *desc, char *units, DOUBLE_PRECISION *gain, INTEGER *initval, INTEGER *group, INTEGER *fmt, INTEGER *spf, INTEGER *bsize, INTEGER *adcres, INTEGER *adczero, INTEGER *baseline, INTEGER *nsamp, INTEGER *cksum)
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
INTEGER annopen_(char *record, INTEGER *nann)
{
    return (annopen(fcstring(record), ainfo, (unsigned int)(*nann)));
}

/* After using isigopen_ or osigopen_, use getsiginfo_ to obtain the contents
   of the signal information structures if necessary. */
INTEGER isigopen_(char *record, INTEGER *nsig)
{
    return (isigopen(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

INTEGER osigopen_(char *record, INTEGER *nsig)
{
    return (osigopen(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

/* Before using osigfopen_, use setsiginfo_ to set the contents of the signal
   information structures. */
INTEGER osigfopen_(INTEGER *nsig)
{
    return (osigfopen(sinfo, (unsigned int)(*nsig)));
}

/* Before using wfdbinit_, use setanninfo_ and setsiginfo_ to set the contents
   of the annotation and signal information structures. */
INTEGER wfdbinit_(char *record, INTEGER *nann, INTEGER *nsig)
{
    return (wfdbinit(fcstring(record), ainfo, (unsigned int)(*nann),
		     sinfo, (unsigned int)(*nsig)));
}

INTEGER findsig_(char *signame)
{
    return (findsig(fcstring(signame)));
}

INTEGER getspf_(INTEGER *dummy)
{
    return (getspf());
}

INTEGER setgvmode_(INTEGER *mode)
{
    setgvmode((int)(*mode));
    return (0L);
}

INTEGER getgvmode_(INTEGER *dummy)
{
    return (getgvmode());
}

INTEGER setifreq_(DOUBLE_PRECISION *freq)
{
    return (setifreq((WFDB_Frequency)*freq));
}

DOUBLE_PRECISION getifreq_(INTEGER *dummy)
{
    return (getifreq());
}

INTEGER getvec_(INTEGER *long_vector)
{
    if (sizeof(WFDB_Sample) == sizeof(INTEGER)) {
	return (getvec((WFDB_Sample *)long_vector));
    }
    else {
	WFDB_Sample v[WFDB_MAXSIG];
	int i, j;

	i = getvec(v);
	for (j = 0; j < i; j++)
	    long_vector[i] = v[i];
	return (i);
    }
}

INTEGER getframe_(INTEGER *long_vector)
{
    if (sizeof(WFDB_Sample) == sizeof(INTEGER)) {
	return (getframe((WFDB_Sample *)long_vector));
    }
    else {
	WFDB_Sample v[WFDB_MAXSIG*WFDB_MAXSPF];
	int i, j, k;

	i = getframe(v);
	for (j = 0; j < i; j += k)
	    for (k = 0; k < sinfo[i].spf; k++)
		long_vector[j+k] = v[j+k];
	return (i);
    }
}

INTEGER putvec_(INTEGER *long_vector)
{
    if (sizeof(WFDB_Sample) == sizeof(INTEGER)) {
	return (putvec((WFDB_Sample *)long_vector));
    }
    else {
	WFDB_Sample v[WFDB_MAXSIG*WFDB_MAXSPF];
	int i;

	for (i = 0; i < WFDB_MAXSIG*WFDB_MAXSPF; i++)
	    v[i] = long_vector[i];
	return (putvec(v));
    }
}

INTEGER getann_(INTEGER *annotator, INTEGER *time, INTEGER *anntyp, INTEGER *subtyp, INTEGER *chan, INTEGER *num, char *aux)
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

INTEGER ungetann_(INTEGER *annotator, INTEGER *time, INTEGER *anntyp, INTEGER *subtyp, INTEGER *chan, INTEGER *num, char *aux)
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

INTEGER putann_(INTEGER *annotator, INTEGER *time, INTEGER *anntyp, INTEGER *subtyp, INTEGER *chan, INTEGER *num, char *aux)
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

INTEGER isigsettime_(INTEGER *time)
{
    return (isigsettime((WFDB_Time)(*time)));
}

INTEGER isgsettime_(INTEGER *group, INTEGER *time)
{
    return (isgsettime((WFDB_Group)(*group), (WFDB_Time)(*time)));
}

INTEGER tnextvec_(INTEGER *signal, INTEGER *time)
{
    return (tnextvec((WFDB_Signal)(*signal), (WFDB_Time)(*time)));
} 

INTEGER iannsettime_(INTEGER *time)
{
    return (iannsettime((WFDB_Time)(*time)));
}

INTEGER ecgstr_(INTEGER *code, char *string)
{
    strcpy(string, ecgstr((int)(*code)));
    cfstring(string);
    return (0L);
}

INTEGER strecg_(char *string)
{
    return (strecg(fcstring(string)));
}

INTEGER setecgstr_(INTEGER *code, char *string)
{
    return (setecgstr((int)(*code), fcstring(string)));
}

INTEGER annstr_(INTEGER *code, char *string)
{
    strcpy(string, annstr((int)(*code)));
    cfstring(string);
    return (0L);
}

INTEGER strann_(char *string)
{
    return (strann(fcstring(string)));
}

INTEGER setannstr_(INTEGER *code, char *string)
{
    return (setannstr((int)(*code), fcstring(string)));
}

INTEGER anndesc_(INTEGER *code, char *string)
{
    strcpy(string, anndesc((int)(*code)));
    cfstring(string);
    return (0L);
}

INTEGER setanndesc_(INTEGER *code, char *string)
{
    return (setanndesc((int)(*code), fcstring(string)));
}

INTEGER setafreq_(DOUBLE_PRECISION *freq)
{
    setafreq(*freq);
    return (0L);
}

DOUBLE_PRECISION getafreq_(INTEGER *dummy)
{
    return (getafreq());
}

INTEGER iannclose_(INTEGER *annotator)
{
    iannclose((WFDB_Annotator)(*annotator));
    return (0L);
}

INTEGER oannclose_(INTEGER *annotator)
{
    oannclose((WFDB_Annotator)(*annotator));
    return (0L);
}

/* The functions below can be used in place of the macros defined in
   <wfdb/ecgmap.h>. */

INTEGER isann_(INTEGER *anntyp)
{   
    return (wfdb_isann((int)*anntyp));
}

INTEGER isqrs_(INTEGER *anntyp)
{   
    return (wfdb_isqrs((int)*anntyp));
}

INTEGER setisqrs_(INTEGER *anntyp, INTEGER *value)
{   
    wfdb_setisqrs((int)*anntyp, (int)*value);
    return (0L);
}

INTEGER map1_(INTEGER *anntyp)
{   
    return (wfdb_map1((int)*anntyp));
}

INTEGER setmap1_(INTEGER *anntyp, INTEGER *value)
{   
    wfdb_setmap1((int)*anntyp, (int)*value);
    return (0L);
}

INTEGER map2_(INTEGER *anntyp)
{   
    return (wfdb_map2((int)*anntyp));
}

INTEGER setmap2_(INTEGER *anntyp, INTEGER *value)
{   
    wfdb_setmap2((int)*anntyp, (int)*value);
    return (0L);
}

INTEGER ammap_(INTEGER *anntyp)
{   
    return (wfdb_ammap((int)*anntyp));
}

INTEGER mamap_(INTEGER *anntyp, INTEGER *subtyp)
{   
    return (wfdb_mamap((int)*anntyp, (int)*subtyp));
}

INTEGER annpos_(INTEGER *anntyp)
{   
    return (wfdb_annpos((int)*anntyp));
}

INTEGER setannpos_(INTEGER *anntyp, INTEGER *value)
{   
    wfdb_setannpos((int)*anntyp, (int)*value);
    return (0L);
}

INTEGER timstr_(INTEGER *time, char *string)
{
    strcpy(string, timstr((WFDB_Time)(*time)));
    cfstring(string);
    return (0L);
}

INTEGER mstimstr_(INTEGER *time, char *string)
{
    strcpy(string, mstimstr((WFDB_Time)(*time)));
    cfstring(string);
    return (0L);
}

INTEGER strtim_(char *string)
{
    return (strtim(fcstring(string)));
}

INTEGER datstr_(INTEGER *date, char *string)
{
    strcpy(string, datstr((WFDB_Date)(*date)));
    cfstring(string);
    return (0L);
}

INTEGER strdat_(char *string)
{
    return (strdat(fcstring(string)));
}

INTEGER adumuv_(INTEGER *signal, INTEGER *ampl)
{
    return (adumuv((WFDB_Signal)(*signal), (WFDB_Sample)(*ampl)));
}

INTEGER muvadu_(INTEGER *signal, INTEGER *microvolts)
{
    return (muvadu((WFDB_Signal)(*signal), (int)(*microvolts)));
}

DOUBLE_PRECISION aduphys_(INTEGER *signal, INTEGER *ampl)
{
    return (aduphys((WFDB_Signal)(*signal), (WFDB_Sample)(*ampl)));
}

INTEGER physadu_(INTEGER *signal, DOUBLE_PRECISION *v)
{
    return (physadu((WFDB_Signal)(*signal), *v));
}

INTEGER sample_(INTEGER *signal, INTEGER *t)
{
    return (sample((WFDB_Signal)(*signal), (WFDB_Time)(*t)));
}

INTEGER sample_valid_(INTEGER *dummy)
{
    return (sample_valid());
}

INTEGER calopen_(char *calibration_filename)
{
    return (calopen(fcstring(calibration_filename)));
}

INTEGER getcal_(char *description, char *units, DOUBLE_PRECISION *low, DOUBLE_PRECISION *high, DOUBLE_PRECISION *scale, INTEGER *caltype)
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

INTEGER putcal_(char *description, char *units, DOUBLE_PRECISION *low, DOUBLE_PRECISION *high, DOUBLE_PRECISION *scale, INTEGER *caltype)
{
    cinfo.sigtype = fcstring(description);
    cinfo.units = fcstring(units);
    cinfo.low = *low;
    cinfo.high = *high;
    cinfo.scale = *scale;
    cinfo.caltype = *caltype;
    return (putcal(&cinfo));
}

INTEGER newcal_(char *calibration_filename)
{
    return (newcal(fcstring(calibration_filename)));
}

INTEGER flushcal_(INTEGER *dummy)
{
    flushcal();
    return (0L);
}

INTEGER getinfo_(char *record, char *string)
{
    strcpy(string, getinfo(fcstring(record)));
    cfstring(string);
    return (0L);
}

INTEGER putinfo_(char *string)
{
    return (putinfo(fcstring(string)));
}

INTEGER setinfo_(char *record)
{
    return (setinfo(fcstring(record)));
}

INTEGER wfdb_freeinfo_(INTEGER *dummy)
{
    wfdb_freeinfo();
    return (0L);
}

INTEGER newheader_(char *record)
{
    return (newheader(fcstring(record)));
}

/* Before using setheader_, use setsiginfo to set the contents of the signal
   information structures. */
INTEGER setheader_(char *record, INTEGER *nsig)
{
    return (setheader(fcstring(record), sinfo, (unsigned int)(*nsig)));
}

/* No wrappers are provided for setmsheader or getseginfo. */

INTEGER wfdbgetskew_(INTEGER *s)
{
    return (wfdbgetskew((WFDB_Signal)(*s)));
}

INTEGER wfdbsetiskew_(INTEGER *s, INTEGER *skew)
{
    wfdbsetiskew((WFDB_Signal)(*s), (int)(*skew));
    return (0L);
}

INTEGER wfdbsetskew_(INTEGER *s, INTEGER *skew)
{
    wfdbsetskew((WFDB_Signal)(*s), (int)(*skew));
    return (0L);
}

INTEGER wfdbgetstart_(INTEGER *s)
{
    return (wfdbgetstart((WFDB_Signal)(*s)));
}

INTEGER wfdbsetstart_(INTEGER *s, INTEGER *bytes)
{
    wfdbsetstart((WFDB_Signal)(*s), *bytes);
    return (0L);
}

INTEGER wfdbputprolog_(char *prolog, INTEGER *bytes, INTEGER *signal)
{
    return (wfdbputprolog(fcstring(prolog), (long)*bytes,(WFDB_Signal)*signal));
}

INTEGER wfdbquit_(INTEGER *dummy)
{
    wfdbquit();
    return (0L);
}

DOUBLE_PRECISION sampfreq_(char *record)
{
    return (sampfreq(fcstring(record)));
}

INTEGER setsampfreq_(DOUBLE_PRECISION *frequency)
{
    return (setsampfreq((WFDB_Frequency)(*frequency)));
}

DOUBLE_PRECISION getcfreq_(INTEGER *dummy)
{
    return (getcfreq());
}

INTEGER setcfreq_(DOUBLE_PRECISION *frequency)
{
    setcfreq((WFDB_Frequency)(*frequency));
    return (0L);
}

DOUBLE_PRECISION getbasecount_(INTEGER *dummy)
{
    return (getbasecount());
}

INTEGER setbasecount_(DOUBLE_PRECISION *count)
{
    setbasecount(*count);
    return (0L);
}

INTEGER setbasetime_(char *string)
{
    return (setbasetime(fcstring(string)));
}

INTEGER wfdbquiet_(INTEGER *dummy)
{
    wfdbquiet();
    return (0L);
}

INTEGER wfdbverbose_(INTEGER *dummy)
{
    wfdbverbose();
    return (0L);
}

INTEGER wfdberror_(char *string)
{
    strcpy(string, wfdberror());
    cfstring(string);
    return (0L);
}

INTEGER setwfdb_(char *string)
{
    setwfdb(fcstring(string));
    return (0L);
}

INTEGER getwfdb_(char *string)
{
    strcpy(string, getwfdb());
    cfstring(string);
    return (0L);
}

INTEGER resetwfdb_(INTEGER *dummy)
{
    resetwfdb();
    return (0L);
}

INTEGER setibsize_(INTEGER *input_buffer_size)
{
    return (setibsize((int)(*input_buffer_size)));
}

INTEGER setobsize_(INTEGER *output_buffer_size)
{
    return (setobsize((int)(*output_buffer_size)));
}

INTEGER wfdbfile_(char *file_type, char *record, char *pathname)
{
    strcpy(pathname, wfdbfile(fcstring(file_type), fcstring(record)));
    cfstring(pathname);
    return (0L);
}

INTEGER wfdbflush_(INTEGER *dummy)
{
    wfdbflush();
    return (0L);
}

INTEGER wfdbmemerr_(INTEGER *exit_if_error)
{
    wfdbmemerr((int)(*exit_if_error));
    return (0L);
}

INTEGER wfdbversion_(char *version)
{
    strcpy(version, wfdbversion());
    cfstring(version);
    return (0L);
}

INTEGER wfdbldflags_(char *ldflags)
{
    strcpy(ldflags, wfdbldflags());
    cfstring(ldflags);
    return (0L);
}

INTEGER wfdbcflags_(char *cflags)
{
    strcpy(cflags, wfdbcflags());
    cfstring(cflags);
    return (0L);
}

INTEGER wfdbdefwfdb_(char *defwfdb)
{
    strcpy(defwfdb, wfdbdefwfdb());
    cfstring(defwfdb);
    return (0L);
}

INTEGER wfdbdefwfdbcal_(char *defwfdbcal)
{
    strcpy(defwfdbcal, wfdbdefwfdbcal());
    cfstring(defwfdbcal);
    return (0L);
}
