/* file: tach.c		G. Moody	 18 April 1985
			Last revised:  14 November 2002

-------------------------------------------------------------------------------
tach: Generate heart rate vs. time signal with evenly spaced samples
Copyright (C) 2002 George B. Moody

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

This program reads an annotation file and produces a uniformly sampled and
smoothed instantaneous heart rate signal.  Smoothing is accomplished by finding
the number of fractional R-R intervals within a window (with a width of two
output sample intervals) centered on the current output sample.  By default,
the output is in text form, and consists of a column of numbers, which are
samples of the instantaneous heart rate signal (in units of beats per minute).
Optionally, the output sample number, or the elapsed time in hours, minutes,
or seconds can be printed before each output sample value.  Alternatively,
`tach' can create a WFDB record containing the heart rate signal.

Studies of heart rate variability generally require special treatment of
ectopic beats.  Typically, ventricular ectopic beat annotations are removed
from the input annotation file and replaced by `phantom' beat annotations at
the expected locations of sinus beats.  The same procedure can be used to fill
in gaps resulting from other causes, such as momentary signal loss.  It is
often necessary to post-process the output of `tach' to remove impulse noise in
the heart rate signal introduced by the presence of non-compensated ectopic
beats, especially supraventricular ectopic beats.  Note that `tach' performs
none of these manipulations, although it usually attempts limited outlier
rejection (`tach' maintains an estimate of the mean absolute deviation of its
output, and replaces any output which is more than three times this amount from
the previous value with the previous value).
*/

#include <stdio.h>
#include <math.h>
#include <wfdb/wfdb.h>
#define map1
#define map2
#define ammap
#define mamap
#define annpos
#include <wfdb/ecgmap.h>

#define ABL	128	/* annotation buffer length (must be a power of 2) */
#define beat(A)	(beatab[(A)&(ABL-1)])	/* time of beat A */
#define RGAIN	100.0

char *irec, *orec, *pname, *prog_name();
double ofreq = 2.;

main(argc,argv)    /* form tachometer signal from annotator file */
int argc;
char *argv[];
{
    double sps, decf, dx, k=1., left, rdmax=40., right, rrcnt, rrate=80., x;
    int i, Oflag = 0, vflag = 0, Vflag = 0;
    long beatab[ABL], lastn, max, min, maxbt, minbt, n, start = 0L, end = 0L;
#ifndef atol
    long atol();
#endif
    WFDB_Anninfo ai;
    WFDB_Annotation annot;
    WFDB_Siginfo si;
    void cleanup(), help();

    pname = prog_name(argv[0]);

    /* Accept old syntax. */
    if (argc >= 3 && argv[1][0] != '-') {
	ai.name = argv[1];
	irec = argv[2];
	i = 3;
    }
    else
	i = 1;

    /* Interpret command-line options. */
    for ( ; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'f':	/* starting time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: starting time must follow -f\n",
			      pname);
		exit(1);
	    }
	    start = i;	/* to be converted to sample intervals below */
	    break;
	  case 'F':	/* output sampling frequency follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: output sampling frequency must follow -F\n",
			pname);
		exit(1);
	    }
	    if ((ofreq = atof(argv[i])) <= 0.) {
		(void)fprintf(stderr,
			"%s: improper output frequency `%s' specified\n",
			pname, argv[i]);
		exit(1);
	    }
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* expected rate follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: expected rate must follow -i\n",
			pname);
		exit(1);
	    }
	    rrate = atoi(argv[i]);
	    break;
	  case 'l':	/* duration of interval follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: duration of interval must follow -l\n",
			pname);
		exit(1);
	    }
	    end = -i;
	    break;
	  case 'n':	/* number of output samples follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: number of output samples must follow -n\n",
			pname);
		exit(1);
	    }
	    ofreq = -i;	/* save index, figure out sampling frequency below */
	    break;
	  case 'o':	/* output record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: output record name must follow -o\n",
			pname);
		exit(1);
	    }
	    orec = argv[i];
	    break;
	  case 'O':	/* disable outlier rejection */
	    Oflag = 1;
	    break;
	  case 'r':	/* input record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    irec = argv[i];
	    break;
	  case 's':	/* smoothing constant follows */
	    if ((++i >= argc) || ((k = atof(argv[i])) <= 0.)) {
		(void)fprintf(stderr,
		       "%s: smoothing constant (> 0) must follow -s\n", pname);
		exit(1);
	    }
	    break;
	  case 't':	/* ending time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end = i;
	    break;
	  case 'v':	/* verbose mode, print output sample numbers */
	    vflag = 1;
	    break;
	  case 'V':	/* alternate verbose mode, print output sample times */
	    Vflag = 1;
	    if (argv[i][2] == 'm') Vflag = 2;
	    else if (argv[i][2] == 'h') Vflag = 3;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n",
			  pname, argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n",
			  pname, argv[i]);
	    exit(1);
	}
    }
    if (irec == NULL || ai.name == NULL) {
	help();
	exit(1);
    }

    if ((sps = sampfreq(irec)) <= 0.)  /* get input sampling frequency */
	(void)setsampfreq(sps = WFDB_DEFFREQ);

    ai.stat = WFDB_READ;
    if (annopen(irec, &ai, 1) < 0)	/* open annotation file */
	exit(2);

    if (start && (start = strtim(argv[(int)start])) < 0L)
	start = -start;
    if (end) {
	if (end < 0) {	/* duration was specified */
	    if ((end = strtim(argv[(int)(-end)])) <= 0L) {
		(void)fprintf(stderr,
			      "%s: improper interval specified\n", pname);
		exit(1);
	    }
	    end += start;
	}
	else {		/* ending time was specified */
	    if ((end = strtim(argv[(int)end])) < 0L)
		end = -end;
	    if (end > 0L && end <= start) {
		(void)fprintf(stderr, "%s: improper interval specified\n",
			      pname);
		exit(1);
	    }
	}
    }
    else if (isigopen(irec, &si, -1) > 0) /* check header for record length */
	end = si.nsamp;

    if (ofreq < 0) {
	if (end-start <= 0L) {
	    (void)fprintf(stderr,
			  "%s: proper interval must be specified before -n\n",
			  pname);
	    exit(1);
	}
	ofreq = atof(argv[(int)(-ofreq)])*sps/(end-start);
    }

    if (iannsettime(start) < 0) exit(3);

    decf = sps/ofreq;  	/* effective decimation factor */
    n = start / decf;
    lastn = end / decf;

    if (orec) {
	static WFDB_Siginfo so;
	static char s[20];

	(void)sprintf(s, "%s.dat", orec);
	so.fname = s;
	so.desc = "HR";
	so.units = "bpm";
	so.gain = RGAIN;
	so.fmt = so.adcres = 16;
	if (osigfopen(&so, 1) < 1) exit(2);
    }

    /* Read the first two beat annotations in the interval to be processed. */
    while (getann(0, &annot) >= 0 && !isqrs(annot.anntyp))
	;
    beat(min = 0) = annot.time;
    while (getann(0, &annot) >= 0 && !isqrs(annot.anntyp))
	;
    beat(max = 1) = annot.time;

    /* Process the remainder of the annotations. */
    while (lastn == 0L || n < lastn) {
	left = decf * (n - k); right = decf * (n + k);	/* window limits */
	while ((maxbt = beat(max)) < right) { /* read beats to right edge */
	    if ((++max - min) > ABL) {  /* buffer full */
		(void)fprintf(stderr,
			      "%s: annotation buffer overflow\n", pname);
		cleanup();
		exit(6);
	    }
	    do {
		if (getann(0, &annot) < 0) { /* quit at end of record */
		    cleanup();
		    exit(0);
		}
	    } while (!isqrs(annot.anntyp));
	    beat(max) = annot.time;
	}
	while ((minbt = beat(min)) <= left)
	    min++;	/* advance to first beat past left edge */
	if (min > 0L)	/* get number of RR intervals in window */
	    rrcnt = max - min + (minbt - left) / (minbt - beat(min-1))
			      - (maxbt - right) / (maxbt - beat(max-1));
	else		/* handle samples between start and first beat */
	    rrcnt = (beat(0) - start > beat(1) - beat(0)) ?
			(2. * decf * k) / (beat(0) - start) :
			(2. * decf * k) / (beat(1) - beat(0));
	x = ofreq * 30. * rrcnt / k;  /* beats per minute */
	if (!Oflag) {	/* attempt to detect and reject outliers */
	    if ((dx = x - rrate) < 0.) dx = -dx;
	    if (dx > 3.*rdmax) x = rrate;
	    rdmax += (dx - rdmax)/20.;
	    if (rdmax < 1.) rdmax = 1.;
	}
	rrate = x;
	if (orec) {
	    int v = (int)(rrate*RGAIN + 0.5);

	    if (putvec(&v) < 1) break;
	}
	else {
	    switch (Vflag) {
	      case 1: (void)printf("%g\t", n/ofreq); break;
	      case 2: (void)printf("%g\t", n/(ofreq*60.0)); break;
	      case 3: (void)printf("%g\t", n/(ofreq*3600.0)); break;
	    }
	    if (vflag) (void)printf("%ld\t", n);
	    (void)printf("%g\n", rrate);
	}
	n++;
    }
    cleanup();
    exit(0);	/*NOTREACHED*/
}

void cleanup()
{
    if (orec) {
	(void)setsampfreq(ofreq);
	(void)newheader(orec);
    }
    wfdbquit();
}

char *prog_name(s)
char *s;
{
    char *p = s + strlen(s);

#ifdef MSDOS
    while (p >= s && *p != '\\' && *p != ':') {
	if (*p == '.')
	    *p = '\0';		/* strip off extension */
	if ('A' <= *p && *p <= 'Z')
	    *p += 'a' - 'A';	/* convert to lower case */
	p--;
    }
#else
    while (p >= s && *p != '/')
	p--;
#endif
    return (p+1);
}

static char *help_strings[] = {
 "usage: %s -r RECORD -a ANNOTATOR [OPTIONS ...]\n",
 "where RECORD and ANNOTATOR specify the input; OPTIONS may include any of:",
" -f TIME       begin at specified time",
" -F FREQUENCY  produce output at the specified sampling FREQUENCY in Hz",
"                (default: 2)",
" -h            print this usage summary",
" -i RATE       set expected RATE in bpm (default: 80)",
" -l DURATION   produce output for the specified DURATION (time interval)",
" -n SAMPLES    produce the specified number of samples, evenly spaced",
"                throughout the interval specified by previous -f and -t or",
"                -l options",
" -o RECORD     generate signal and header files for the specified output",
"                RECORD (which should not be the same as the input record)",
" -O            disable outlier rejection",
" -s SMOOTHING  set smoothing constant (default: 1)",
" -t TIME       stop at specified time",
" -v            print output sample number before each output sample value",
" -Vs (or -V)   print elapsed time in seconds before each output sample value",
" -Vm           print elapsed time in minutes before each output sample value",
" -Vh           print elapsed time in hours before each output sample value",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
