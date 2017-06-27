/* file: wfdb.h		G. Moody	13 June 1983
			Last revised:    21 May 2015 		wfdblib 10.5.24
WFDB library type, constant, structure, and function interface definitions
_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1983-2013 George B. Moody

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

*/

#ifndef wfdb_WFDB_H	/* avoid multiple definitions */
#define wfdb_WFDB_H

/* WFDB library version. */
#define WFDB_MAJOR   10
#define WFDB_MINOR   5
#define WFDB_RELEASE 24
#define WFDB_NETFILES 1	/* if 1, library includes code for HTTP, FTP clients */
#define WFDB_NETFILES_LIBCURL 1

/* Determine what type of compiler is being used. */
#ifdef __STDC__		/* true for ANSI C compilers only */
#define wfdb_PROTO	/* function prototypes will be needed */
#undef _WINDOWS		/* we don't want MS-Windows API in this case */
#endif

#ifdef __cplusplus	/* true for some C++ compilers */
#define wfdb_CPP
#define wfdb_PROTO
#endif

#ifdef c_plusplus	/* true for some other C++ compilers */
#define wfdb_CPP
#define wfdb_PROTO
#endif

#ifdef _WIN32		/* true when compiling for 32-bit MS Windows */
#ifndef _WINDOWS
#define _WINDOWS
#endif
#endif

#ifdef _WINDOWS		/* true when compiling for MS Windows */
#define wfdb_PROTO
#endif

#ifndef wfdb_PROTO	/* should be true for K&R C compilers only */
#define wfdb_KRC
#define signed
#endif

/* Simple data types */
typedef int	     WFDB_Sample;   /* units are adus */
typedef long 	     WFDB_Time;	    /* units are sample intervals */
typedef long	     WFDB_Date;	    /* units are days */
typedef double	     WFDB_Frequency;/* units are Hz (samples/second/signal) */
typedef double	     WFDB_Gain;	    /* units are adus per physical unit */
typedef unsigned int WFDB_Group;    /* signal group number */
typedef unsigned int WFDB_Signal;   /* signal number */
typedef unsigned int WFDB_Annotator;/* annotator number */

/* getvec and getframe return a sample with a value of WFDB_INVALID_SAMPLE
   when the amplitude of a signal is undefined (e.g., the input is clipped or
   the signal is not available) and padding is disabled (see WFDB_GVPAD, below).
   WFDB_INVALID_SAMPLE is the minimum value for a 16-bit WFDB_Sample.  In
   a format 24 or 32 signal, this value will be near mid-range, but it should
   occur only rarely in such cases;  if this is a concern, WFDB_INVALID_SAMPLE
   can be redefined and the WFDB library can be recompiled. */
#define WFDB_INVALID_SAMPLE (-32768)

/* Array sizes
   Many older applications use the values of WFDB_MAXANN, WFDB_MAXSIG, and
   WFDB_MAXSPF to determine array sizes, but (since WFDB library version 10.2)
   there are no longer any fixed limits imposed by the WFDB library.
*/
#define WFDB_MAXANN    2   /* maximum number of input or output annotators */
#define	WFDB_MAXSIG   32   /* maximum number of input or output signals */
#define WFDB_MAXSPF    4   /* maximum number of samples per signal per frame */
#define WFDB_MAXRNL   50   /* maximum length of record name */
#define WFDB_MAXUSL   50   /* maximum length of WFDB_siginfo `.units' string */
#define WFDB_MAXDSL  100   /* maximum length of WFDB_siginfo `.desc' string */

/* wfdb_fopen mode values (WFDB_anninfo '.stat' values) */
#define WFDB_READ      0   /* standard input annotation file */
#define WFDB_WRITE     1   /* standard output annotation file */
#define WFDB_AHA_READ  2   /* AHA-format input annotation file */
#define WFDB_AHA_WRITE 3   /* AHA-format output annotation file */
#define WFDB_APPEND    4   /* for output info files */

/* WFDB_siginfo '.fmt' values
FMT_LIST is suitable as an initializer for a static array; it lists all of
the legal values for the format field in a WFDB_siginfo structure.
 fmt    meaning
   0	null signal (nothing read or written)
   8	8-bit first differences
  16	16-bit 2's complement amplitudes, low byte first
  61	16-bit 2's complement amplitudes, high byte first
  80	8-bit offset binary amplitudes
 160	16-bit offset binary amplitudes
 212	2 12-bit amplitudes bit-packed in 3 bytes
 310	3 10-bit amplitudes bit-packed in 4 bytes
 311    3 10-bit amplitudes bit-packed in 4 bytes
  24	24-bit 2's complement amplitudes, low byte first
  32	32-bit 2's complement amplitudes, low byte first
*/
#define WFDB_FMT_LIST {0, 8, 16, 61, 80, 160, 212, 310, 311, 24, 32}
#define WFDB_NFMTS	  11    /* number of items in WFDB_FMT_LIST */

/* Default signal specifications */
#define WFDB_DEFFREQ	250.0  /* default sampling frequency (Hz) */
#define WFDB_DEFGAIN	200.0  /* default value for gain (adu/physical unit) */
#define WFDB_DEFRES	12     /* default value for ADC resolution (bits) */

/* getvec operating modes */
#define WFDB_LOWRES   	0	/* return one sample per signal per frame */
#define WFDB_HIGHRES	1	/* return each sample of oversampled signals,
				   duplicating samples of other signals */
#define WFDB_GVPAD	2	/* replace invalid samples with previous valid
				   samples */

/* calinfo '.caltype' values
WFDB_AC_COUPLED and WFDB_DC_COUPLED are used in combination with the pulse
shape definitions below to characterize calibration pulses. */
#define WFDB_AC_COUPLED	0	/* AC coupled signal */
#define WFDB_DC_COUPLED	1	/* DC coupled signal */
#define WFDB_CAL_SQUARE	2	/* square wave pulse */
#define WFDB_CAL_SINE	4	/* sine wave pulse */
#define WFDB_CAL_SAWTOOTH	6	/* sawtooth pulse */
#define WFDB_CAL_UNDEF	8	/* undefined pulse shape */

/* Structure definitions */
struct WFDB_siginfo {	/* signal information structure */
    char *fname;	/* filename of signal file */
    char *desc;		/* signal description */
    char *units;	/* physical units (mV unless otherwise specified) */
    WFDB_Gain gain;	/* gain (ADC units/physical unit, 0: uncalibrated) */
    WFDB_Sample initval; 	/* initial value (that of sample number 0) */
    WFDB_Group group;	/* signal group number */
    int fmt;		/* format (8, 16, etc.) */
    int spf;		/* samples per frame (>1 for oversampled signals) */
    int bsize;		/* block size (for character special files only) */
    int adcres;		/* ADC resolution in bits */
    int adczero;	/* ADC output given 0 VDC input */
    int baseline;	/* ADC output given 0 physical units input */
    long nsamp;		/* number of samples (0: unspecified) */
    int cksum;		/* 16-bit checksum of all samples */
};

struct WFDB_calinfo {	/* calibration information structure */
    double low;		/* low level of calibration pulse in physical units */
    double high;	/* high level of calibration pulse in physical units */
    double scale;	/* customary plotting scale (physical units per cm) */
    char *sigtype;	/* signal type */
    char *units;	/* physical units */
    int caltype;	/* calibration pulse type (see definitions above) */
};

struct WFDB_anninfo {	/* annotator information structure */
    char *name;		/* annotator name */
    int stat;		/* file type/access code (READ, WRITE, etc.) */
};

struct WFDB_ann {	/* annotation structure */
    WFDB_Time time;	/* annotation time, in sample intervals from
			   the beginning of the record */
    char anntyp;	/* annotation type (< ACMAX, see <wfdb/ecgcodes.h> */
    signed char subtyp;	/* annotation subtype */
    unsigned char chan;	/* channel number */
    signed char num;	/* annotator number */
    unsigned char *aux;	/* pointer to auxiliary information */
};

struct WFDB_seginfo {	/* segment record structure */
    char recname[WFDB_MAXRNL+1];   /* segment name */
    WFDB_Time nsamp;		   /* number of samples in segment */
    WFDB_Time samp0;		   /* sample number of first sample */
};

/* Composite data types */
typedef struct WFDB_siginfo WFDB_Siginfo;
typedef struct WFDB_calinfo WFDB_Calinfo;
typedef struct WFDB_anninfo WFDB_Anninfo;
typedef struct WFDB_ann WFDB_Annotation;
typedef struct WFDB_seginfo WFDB_Seginfo;

/* Dynamic memory allocation macros. */
#define MEMERR(P, N, S) \
    { wfdb_error("WFDB: can't allocate (%ld*%ld) bytes for %s\n", \
	     (size_t)N, (size_t)S, #P);			      \
      if (wfdb_me_fatal()) exit(1); }
#define SFREE(P) { if (P) { free (P); P = 0; } }
#define SUALLOC(P, N, S) { if (!(P = calloc((N), (S)))) MEMERR(P, (N), (S)); }
#define SALLOC(P, N, S) { SFREE(P); SUALLOC(P, (N), (S)) }
#define SREALLOC(P, N, S) { if (!(P = realloc(P, (N)*(S)))) MEMERR(P,(N),(S)); }
#define SSTRCPY(P, Q) { if (Q) { \
	 SALLOC(P, (size_t)strlen(Q)+1,1); strcpy(P, Q); } }

/* Function types */
#ifndef _WINDLL	/* for everything *except* MS Windows applications */
typedef char *FSTRING;
typedef const char *FCONSTSTRING;
typedef WFDB_Date FDATE;
typedef double FDOUBLE;
typedef WFDB_Frequency FFREQUENCY;
typedef int FINT;
typedef long FLONGINT;
typedef WFDB_Sample FSAMPLE;
typedef WFDB_Time FSITIME;
typedef void FVOID;
#else		
#ifndef _WIN32	/* for 16-bit MS Windows applications using the WFDB DLL */
  /* typedefs don't work properly with _far or _pascal -- must use #defines */
#define FSTRING char _far * _pascal
#define FCONSTSTRING const char _far * _pascal
#define FDATE WFDB_Date _far _pascal
#define FDOUBLE double _far _pascal
#define FFREQUENCY WFDB_Frequency _far _pascal
#define FINT int _far _pascal
#define FLONGINT long _far _pascal
#define FSAMPLE WFDB_Sample _far _pascal
#define FSITIME WFDB_Time _far _pascal
#define FVOID void _far _pascal
#else		/* for 32-bit MS Windows applications using the WFDB DLL */
#ifndef CALLBACK
#define CALLBACK __stdcall	/* from windef.h */
#endif
#define FSTRING __declspec (dllexport) char * CALLBACK
#define FCONSTSTRING __declspec (dllexport) const char * CALLBACK
#define FDATE __declspec (dllexport) WFDB_Date CALLBACK
#define FDOUBLE __declspec (dllexport) double CALLBACK
#define FFREQUENCY __declspec (dllexport) WFDB_Frequency CALLBACK
#define FINT __declspec (dllexport) int CALLBACK
#define FLONGINT __declspec (dllexport) long CALLBACK
#define FSAMPLE __declspec (dllexport) WFDB_Sample CALLBACK
#define FSITIME __declspec (dllexport) WFDB_Time CALLBACK
#define FVOID __declspec (dllexport) void CALLBACK
#endif
#endif

/* Specify C linkage for C++ compilers. */
#ifdef wfdb_CPP
extern "C" {
#endif

/* Define function prototypes for ANSI C compilers and C++ compilers */
#ifdef wfdb_PROTO
extern FINT annopen(char *record, WFDB_Anninfo *aiarray,
		    unsigned int nann);
extern FINT isigopen(char *record, WFDB_Siginfo *siarray, int nsig);
extern FINT osigopen(char *record, WFDB_Siginfo *siarray,
		     unsigned int nsig);
extern FINT osigfopen(WFDB_Siginfo *siarray, unsigned int nsig);
extern FINT wfdbinit(char *record,
		     WFDB_Anninfo *aiarray, unsigned int nann,
		     WFDB_Siginfo *siarray, unsigned int nsig);
extern FINT findsig(char *signame);
extern FINT getspf(void);
extern FVOID setgvmode(int mode);
extern FINT getgvmode(void);
extern FINT setifreq(WFDB_Frequency freq);
extern FFREQUENCY getifreq(void);
extern FINT getvec(WFDB_Sample *vector);
extern FINT getframe(WFDB_Sample *vector);
extern FINT putvec(WFDB_Sample *vector);
extern FINT getann(WFDB_Annotator a, WFDB_Annotation *annot);
extern FINT ungetann(WFDB_Annotator a, WFDB_Annotation *annot);
extern FINT putann(WFDB_Annotator a, WFDB_Annotation *annot);
extern FINT isigsettime(WFDB_Time t);
extern FINT isgsettime(WFDB_Group g, WFDB_Time t);
extern FSITIME tnextvec(WFDB_Signal s, WFDB_Time t);
extern FINT iannsettime(WFDB_Time t);
extern FSTRING ecgstr(int annotation_code);
extern FINT strecg(char *annotation_mnemonic_string);
extern FINT setecgstr(int annotation_code, char *annotation_mnemonic_string);
extern FSTRING annstr(int annotation_code);
extern FINT strann(char *annotation_mnemonic_string);
extern FINT setannstr(int annotation_code, char *annotation_mnemonic_string);
extern FSTRING anndesc(int annotation_code);
extern FINT setanndesc(int annotation_code, char *annotation_description);
extern FVOID setafreq(WFDB_Frequency f);
extern FFREQUENCY getafreq(void);
extern FVOID iannclose(WFDB_Annotator a);
extern FVOID oannclose(WFDB_Annotator a);
extern FINT wfdb_isann(int code);
extern FINT wfdb_isqrs(int code);
extern FINT wfdb_setisqrs(int code, int newval);
extern FINT wfdb_map1(int code);
extern FINT wfdb_setmap1(int code, int newval);
extern FINT wfdb_map2(int code);
extern FINT wfdb_setmap2(int code, int newval);
extern FINT wfdb_ammap(int code);
extern FINT wfdb_mamap(int code, int subtype);
extern FINT wfdb_annpos(int code);
extern FINT wfdb_setannpos(int code, int newval);
extern FSTRING timstr(WFDB_Time t);
extern FSTRING mstimstr(WFDB_Time t);
extern FSITIME strtim(char *time_string);
extern FSTRING datstr(WFDB_Date d);
extern FDATE strdat(char *date_string);
extern FINT adumuv(WFDB_Signal s, WFDB_Sample a);
extern FSAMPLE muvadu(WFDB_Signal s, int microvolts);
extern FDOUBLE aduphys(WFDB_Signal s, WFDB_Sample a);
extern FSAMPLE physadu(WFDB_Signal s, double v);
extern FSAMPLE sample(WFDB_Signal s, WFDB_Time t);
extern FINT sample_valid(void);
extern FINT calopen(char *calibration_filename);
extern FINT getcal(char *description, char *units, WFDB_Calinfo *cal);
extern FINT putcal(WFDB_Calinfo *cal);
extern FINT newcal(char *calibration_filename);
extern FVOID flushcal(void);
extern FSTRING getinfo(char *record);
extern FINT putinfo(char *info);
extern FINT setinfo(char *record);
extern FVOID wfdb_freeinfo(void);
extern FINT newheader(char *record);
extern FINT setheader(char *record, WFDB_Siginfo *siarray, unsigned int nsig);
extern FINT setmsheader(char *record, char **segnames, unsigned int nsegments);
extern FINT getseginfo(WFDB_Seginfo **segments);
extern FINT wfdbgetskew(WFDB_Signal s);
extern FVOID wfdbsetiskew(WFDB_Signal s, int skew);
extern FVOID wfdbsetskew(WFDB_Signal s, int skew);
extern FLONGINT wfdbgetstart(WFDB_Signal s);
extern FVOID wfdbsetstart(WFDB_Signal s, long bytes);
extern FINT wfdbputprolog(char *prolog, long bytes, WFDB_Signal s);
extern FVOID wfdbquit(void);
extern FFREQUENCY sampfreq(char *record);
extern FINT setsampfreq(WFDB_Frequency sampling_frequency);
extern FFREQUENCY getcfreq(void);
extern FVOID setcfreq(WFDB_Frequency counter_frequency);
extern FDOUBLE getbasecount(void);
extern FVOID setbasecount(double count);
extern FINT setbasetime(char *time_string);
extern FVOID wfdbquiet(void);
extern FVOID wfdbverbose(void);
extern FSTRING wfdberror(void);
extern FVOID setwfdb(char *database_path_string);
extern FSTRING getwfdb(void);
extern FVOID resetwfdb(void);
extern FINT setibsize(int input_buffer_size);
extern FINT setobsize(int output_buffer_size);
extern FSTRING wfdbfile(char *file_type, char *record);
extern FVOID wfdbflush(void);
extern FVOID wfdbmemerr(int exit_on_error);
extern FCONSTSTRING wfdbversion(void);
extern FCONSTSTRING wfdbldflags(void);
extern FCONSTSTRING wfdbcflags(void);
extern FCONSTSTRING wfdbdefwfdb(void);
extern FCONSTSTRING wfdbdefwfdbcal(void);
#endif

#ifdef wfdb_CPP
}
#endif

#ifdef wfdb_KRC	/* declare only function return types for K&R C compilers */
extern FINT annopen(), isigopen(), osigopen(), wfdbinit(), findsig(), getspf(),
    setifreq(), getvec(), getframe(), getgvmode(), putvec(), getann(),
    ungetann(), putann(), isigsettime(), isgsettime(), iannsettime(), strecg(),
    setecgstr(), strann(), setannstr(), setanndesc(), wfdb_isann(),
    wfdb_isqrs(), wfdb_setisqrs(), wfdb_map1(), wfdb_setmap1(), wfdb_map2(),
    wfdb_setmap2(), wfdb_ammap(), wfdb_mamap(), wfdb_annpos(), wfdb_setannpos(),
    adumuv(), newheader(), setheader(), setmsheader(), getseginfo(),
    wfdbputprolog(), setsampfreq(), setbasetime(), putinfo(), setinfo(),
    setibsize(), setobsize(), calopen(), getcal(), putcal(), newcal(),
    wfdbgetskew(), sample_valid();
extern FLONGINT wfdbgetstart();
extern FSAMPLE muvadu(), physadu(), sample();
extern FSTRING ecgstr(), annstr(), anndesc(), timstr(), mstimstr(),
    datstr(), getwfdb(), getinfo(), wfdberror(), wfdbfile();
extern FSITIME strtim(), tnextvec();
extern FDATE strdat();
extern FVOID setafreq(), setgvmode(), wfdb_freeinfo(), wfdbquit(), wfdbquiet(),
    wfdbverbose(), setdb(), wfdbflush(), setcfreq(), setbasecount(), flushcal(),
    wfdbsetiskew(), wfdbsetskew(), wfdbsetstart(), wfdbmemerr();
extern FFREQUENCY getafreq(), getifreq(), sampfreq(), getcfreq();
extern FDOUBLE aduphys(), getbasecount();
#endif

/* Remove local preprocessor definitions. */
#ifdef wfdb_PROTO
#undef wfdb_PROTO
#endif

#ifdef wfdb_CPP
#undef wfdb_CPP
#endif

#ifdef wfdb_KRC
#undef wfdb_KRC
#undef signed
#endif

/* Include some useful function declarations.  This section includes standard
   header files if available and provides alternative declarations for those
   platforms that need them.

  The ANSI/ISO C standard requires conforming compilers to predefine __STDC__.
  Non-conforming compilers for MS-Windows may or may not predefine _WINDOWS;
  if you use such a compiler, you may need to define _WINDOWS manually. */
#if defined(__STDC__) || defined(_WINDOWS)
# include <stdlib.h>
#else
extern char *getenv();
extern void exit();
typedef long time_t;
# ifdef HAVE_MALLOC_H
# include <malloc.h>
# else
extern char *malloc(), *calloc(), *realloc();
# endif
# ifdef ISPRINTF
extern int sprintf();
# else
#  ifndef MSDOS
extern char *sprintf();
#  endif
# endif
#endif

#ifndef BSD
# include <string.h>
#else		/* for Berkeley UNIX only */
# include <strings.h>
# define strchr index
#endif

#endif
