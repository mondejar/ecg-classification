/* file: mxm.c		G. Moody	20 March 1992
			Last revised:  14 November 2002

-------------------------------------------------------------------------------
mxm: ANSI/AAMI-standard measurement-by-measurement annotation file comparator
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

This program implements the measurement-by-measurement comparison algorithm
described in AAMI/ANSI EC38-1994, the American National Standard for ambulatory
ECGs, for evaluating heart rate measurements. (`mxm' can be used to evaluate
other types of measurements, such as HRV or RRV measurements, using the same
comparison algorithm, without modification.) Information about using this
program is contained in file `mxm.1'.

Note: the calculations of both normalized and unnormalized RMS error performed
by versions of this program dated earlier than 15 May 1996 were incorrect, and
depended on the number of measurements made (in general, the erroneous values
became smaller as the number of measurements increased).  For this reason,
values obtained using earlier versions of this program are not directly
comparable with those obtained using this version, although comparisons among
values obtained using earlier versions are valid for purposes of ranking.
Thanks to Denny Dow for pointing out the problem!
*/

#include <stdio.h>
#include <math.h>	/* for declaration of sqrt() */
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;		/* name by which this program was invoked */
double A, Anext;	/* measurements from the current & next reference
			   annotation */
double a;		/* measurement from the current test annotation */
int fflag;		/* output format (0: standard, 1: line format) */
int mtype = 0;		/* measurement subtype to be compared */
long start;		/* time of the beginning of the test period */
long end_time;		/* end of the test period (-1: end of reference annot
			   file; 0: end of either annot file) */
long huge_time = 0x7FFFFFFF;		/* largest possible time */
long T, Tnext = -1L;	/* times of the current & next reference annotations */
long t;			/* time of the current test annotation */

main(argc, argv)
int argc;
char *argv[];
{
    void getref(), gettest(), init(), pair(), print_results();

    /* Read and interpret command-line arguments. */
    init(argc, argv);

    /* Set A and T to the measurement and time of the first test annotation
       after the end of the learning period. */
    do {
	gettest();
    } while (t < start);

    /* Peform the comparison.  Each time through the loop below, a test
       annotation is read and the matching reference annotation is paired with
       it.

       The complex loop termination condition is dependent on end_time, which
       is not changed during execution of the loop.  There are three ways the
       loop termination condition can be satisfied:
       - If the length of the comparison is known, either because it was
         specified using the `-t' option or because the header file specifies
	 the record length, the loop ends when both T and t are greater than
	 end_time.  This is the usual case.
       - If the length of the comparison is unknown (end_time = -1), the loop
         ends when EOF is reached in the reference annotation file (T =
	 huge_time).
       - If the option `-t 0' was specified (end_time = 0), the loop ends when
         EOF is first reached in either annotation file (T or t = huge_time).
    */
    while ((end_time > 0L && (Tnext <= end_time || t <= end_time)) ||
	   (end_time == -1L && Tnext != huge_time) ||
	   (end_time == 0L && Tnext != huge_time && t != huge_time)) {

	/* Set Anext and Tnext to the measurement and time of the first
	   reference annotation at or after t, and A and T to the measurement
	   and time of the previous reference annotation. */
	while (Tnext < t) {
	    T = Tnext;
	    A = Anext;
	    getref();
	}

	/* We now have T < t <= Tnext.  Pair a with either A or Anext,
	   depending on which of T and Tnext is nearer to t. */
	if (t - T < Tnext - t)
	    pair(A, a);
	else
	    pair(Anext, a);

	/* Get the next test annotation. */
	gettest();
    }

    /* Generate output. */
    print_results(fflag);

    wfdbquit();			/* close input files */
    exit(0);	/*NOTREACHED*/
}

long nrmeas = 0L;

void getref()	/* get next reference MEASURE annotation with subtyp = mtype */
{
    static WFDB_Annotation annot;

    while (getann(0, &annot) == 0) {
	if (annot.anntyp == MEASURE && annot.subtyp == mtype) {
	    Tnext = annot.time;
	    if (annot.aux)
		Anext = atof(annot.aux+1);
	    nrmeas++;
	    return;
	}
    }	

    /* When this statement is reached, there are no more annotations in the
       reference annotation file. */
    Tnext = huge_time;
}

long ntmeas = 0L;

void gettest()	/* get next test MEASURE annotation with subtyp = mtype */
{
    static WFDB_Annotation annot;

    while (getann(1, &annot) == 0) {
	if (annot.anntyp == MEASURE && annot.subtyp == mtype) {
	    t = annot.time;
	    if (annot.aux)
		a = atof(annot.aux+1);
	    ntmeas++;
	    return;
	}
    }	

    /* When this statement is reached, there are no more annotations in the
       test annotation file. */
    t = huge_time;
}

double errsum = 0.0, refsum = 0.0;

void pair(ref, test)	/* count a measurement pair */
double ref, test;		/* reference and test measurements */
{

    refsum += ref;
    errsum += (ref-test)*(ref-test);
}

FILE *ofile;			/* output file */
char *ofname = "-";		/* filename for reports */
char *record;			/* record name */
int uflag;			/* if non-zero, don't normalize */
WFDB_Anninfo an[2];

/* Read and interpret command-line arguments. */
void init(argc, argv)
int argc;
char *argv[];
{
    int i;
    char *prog_name();
    void help();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator names follow */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
		     "%s: reference and test annotator names must follow -a\n",
		       pname);
		exit(1);
	    }
	    an[0].name = argv[i];
	    an[1].name = argv[++i];
	    break;
	  case 'f':	/* start time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: start time must follow -f\n", pname);
		exit(1);
	    }
	    start = i;	/* save arg index, convert to samples later, when
			   record has been opened and sampling frequency is
			   known */
	    break;
	  case 'h':	/* print usage summary */
	    help();
	    exit(1);
	    break;
	  case 'l':	/* line-format output */
	  case 'L':	/* line-format output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -l\n",
			      pname);
		exit(1);
	    }
	    fflag = 1;
	    ofname = argv[i];
	    break;
	  case 'm':	/* specify measurement type */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: measurement number must follow -m\n",
			      pname);
		exit(1);
	    }
	    mtype = atoi(argv[i]);
	    if (mtype < -128 || mtype > 127) {
		(void)fprintf(stderr,
		      "%s: measurement number must be between -128 and 127\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'r':	/* record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* standard-format output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -s\n",
			      pname);
		exit(1);
	    }
	    fflag = 0;
	    ofname = argv[i];
	    break;
	  case 't':	/* end time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end_time = i;
	    break;
	  case 'u':	/* don't normalize */
	    uflag = 1;
	    break;
	  default:
	    (void)fprintf(stderr,
			  "%s: unrecognized option %s\n", pname, argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr,
			  "%s: unrecognized argument %s\n",pname,argv[i]);
	    exit(1);
	}
    }

    if (!record || !an[0].name) {
	help();
	exit(1);
    }

    if (start != 0L || end_time != 0L)
	(void)fprintf(stderr,"%s: (warning) nonstandard comparison selected\n",
		pname);

    if (sampfreq(record) <= 0) {
	(void)fprintf(stderr,
		      "%s: (warning) %g Hz sampling frequency assumed\n",
		pname, WFDB_DEFFREQ);
	(void)setsampfreq(WFDB_DEFFREQ);
    }

    /* Set the times of the start and end of the test period. */
    if (start) {
	start = strtim(argv[(int)start]);
	if (start < (WFDB_Time)0) start = -start;
    }
    else
	start = strtim("5:0");			/* 5 minutes */
    if (end_time) {
	end_time = strtim(argv[(int)end_time]);
	if (end_time < (WFDB_Time)0) end_time = -end_time;
    }
    else if ((end_time = strtim("e")) == 0L)
	end_time = -1L;		/* record length unavailable -- go to end of
				   reference annotation file */
    if (end_time > 0L && end_time < start) {
	(void)fprintf(stderr, "%s: improper interval specified\n", pname);
	exit(1);
    }

    an[0].stat = an[1].stat = WFDB_READ;
    if (annopen(record, an, 2) < 0) exit(2);
}

void print_results(fflag)
int fflag;
{
    if (nrmeas == 0L && ntmeas == 0L) {
	(void)fprintf(stderr,
		      "%s: no measurements of type %d in either input file\n",
		      pname, mtype);
	exit(2);
    }

    if (nrmeas == 0L) {
	(void)fprintf(stderr,
		      "%s: no measurements of type %d for annotator %s\n",
		      pname, mtype, an[0].name);
	exit(2);
    }

    if (ntmeas == 0L) {
	(void)fprintf(stderr,
		      "%s: no measurements of type %d for annotator %s\n",
		      pname, mtype, an[1].name);
	exit(2);
    }

    if (refsum == 0.0 && uflag == 0) {
	(void)fprintf(stderr,
		      "%s: reference measurements have zero mean\n", pname);
	exit(3);
    }

    /* Open output file.  If line-format output was selected, write column
       headings only if the file must be created from scratch. */
    if (strcmp(ofname, "-")) {
	if ((ofile = fopen(ofname, "r")) == NULL) {
	    if ((ofile = fopen(ofname, "w")) == NULL) {
		(void)fprintf(stderr, "%s: can't create %s\n", pname, ofname);
		exit(3);
	    }
	    if (fflag == 1) {
		(void)fprintf(ofile,
			      "(Measurement errors)\nMeasurement %d\n", mtype);
		(void)fprintf(ofile,
		    "Record\tRMS error (%%)\tMean reference measurement\n");
	    }
	}
	else {
	    (void)fclose(ofile);
	    if ((ofile = fopen(ofname, "a")) == NULL) {
		(void)fprintf(stderr, "%s: can't modify %s\n", pname, ofname);
		exit(3);
	    }
	}
    }
    else ofile = stdout;

    if (fflag == 0) {
	(void)fprintf(ofile,
	       "Measurement-by-measurement comparison results for record %s\n",
		      record);
	(void)fprintf(ofile, "Measurement number %d\n", mtype);
	(void)fprintf(ofile, "Reference annotator: %s\n", an[0].name);
	(void)fprintf(ofile, "     Test annotator: %s\n\n", an[1].name);
	if (!uflag)
	    (void)fprintf(ofile,
			  "     Normalized RMS error: %7.4lf%% (n = %ld)\n",
			  100.0*sqrt(errsum*ntmeas/(refsum*refsum)), ntmeas);
	else
	    (void)fprintf(ofile,
			  "   Unnormalized RMS error: %7.4lf (n = %ld)\n",
			  sqrt(errsum/ntmeas), ntmeas);
	(void)fprintf(ofile,
		          "Mean reference measurement: %g\n", refsum/ntmeas);
    }
    else if (!uflag)
	(void)fprintf(ofile, "%6s\t%7.4lf\t\t%g\n",
		      record,
		      100.0*sqrt(errsum*ntmeas/(refsum*refsum)),
		      refsum/ntmeas);
    else
	(void)fprintf(ofile, "%6s\t%7.4lf\t\t%g (unnormalized)\n",
		      record,
		      sqrt(errsum/ntmeas),
		      refsum/ntmeas);
}

static char *help_strings[] = {
 "usage: %s -r RECORD -a REF TEST [OPTIONS ...]\n",
 "where RECORD is the record name;  REF is reference annotator name;  TEST is",
 "the test annotator name; and OPTIONS may include any of:",
 " -f TIME  begin comparison at specified TIME (default: 5 minutes",
 "           after beginning of record)",
 " -h       print this usage summary",
 " -l FILE  append line-format reports to FILE",
 " -L FILE  same as -l FILE",
 " -m TYPE  specify measurement type to be compared (-128 to 127; default: 0)",
 " -s FILE  append standard-format reports to FILE",
 " -t TIME  stop comparison at specified TIME (default: end of record",
 "           if defined, end of reference annotation file otherwise;",
 "           if TIME is 0, the comparison ends when the end of either",
 "           annotation file is reached)",
 " -u       calculate unnormalized RMS measurement error",
 NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
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
