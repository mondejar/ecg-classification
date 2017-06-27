/* file rr2ann.c	G. Moody	 29 November 1999
			Last revised:	  6 December 2000
-------------------------------------------------------------------------------
rr2ann: Generate an annotation file from a text file of RR intervals
Copyright (C) 2000 George B. Moody

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

The input to this program should contain a set of RR (beat-to-beat) intervals
in text format, one per line.  The first token on each line is taken to be
an RR interval;  any additional tokens on the same line are ignored.  Empty
lines and lines beginning with '#' are also ignored.  The units of the RR
intervals are sample intervals by default;  use the '-x' option to specify
a factor by which to multiply the given intervals in order to obtain sample
intervals.  The output of this program is an annotation file in which all
beats are marked NORMAL.  The first annotation is placed at the sample that
corresponds to the end of the first interval (in other words, as if there
were a beat at sample 0, but no annotation is placed at sample 0).
If a header file does not exist for the specified record, this program will
create one;  in this case, you may use the '-F' option to specify the sampling
frequency if the default (WFDB_DEFFREQ) is incorrect.
*/

#include <stdio.h>
#ifndef __STDC__
extern void exit();
#endif

#define MAXLINELEN 1024		/* longer input lines will be silently split */

#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;

main(argc, argv)	
int argc;
char *argv[];
{
    static WFDB_Anninfo ai;
    static WFDB_Annotation annot;
    WFDB_Frequency sps = 0.0;
    double rr, t = 0.0, xfactor = 1.0;
    static char line[MAXLINELEN];
    char annstr[10], *p, *record = NULL, *prog_name();
    int i, Tflag = 0, wflag = 0;
    void help();
    extern double atof();

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'F':	/* sampling frequency in Hz */
	    if (++i >= argc || (sps = (WFDB_Frequency)atof(argv[i])) <= 0.0) {
		(void)fprintf(stderr,
			      "%s: sampling frequency (> 0) must follow -F\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'r':	/* record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 'T':	/* data are times of occurrence, not RR intervals */
	    Tflag = 1;
	    break;
	  case 'x':	/* multiplier to convert input to sample intervals */
	    if (++i >= argc || (xfactor = atof(argv[i])) <= 0.0) {
		(void)fprintf(stderr,
			      "%s: an input multiplier (> 0) must follow -x\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'w':	/* copy annotation types from second column of input */
	    wflag = 1;
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
    if (record == NULL || ai.name == NULL) {
	help();
	exit(1);
    }

    /* sampfreq is likely to fail, since the header probably doesn't exist.
       Suppress its error message. */
    wfdbquiet();
    if (sampfreq(record) <= 0.0) {
	wfdbverbose();	/* re-enable error reporting */
	if (sps == 0.0) sps = WFDB_DEFFREQ;
	setsampfreq(sps);
	if (newheader(record) < 0)
	    exit(2);
    }
    else {
      wfdbverbose();	/* re-enable error reporting */
      if (sps != 0.0)
	fprintf(stderr,
	 "%s (warning): -F option ignored (using %g for sampling frequency)\n",
		pname, sampfreq(NULL));
    }

    ai.stat = WFDB_WRITE;
    if (annopen(record, &ai, 1) < 0)	/* open annotation file */
	exit(3);
    annot.anntyp = NORMAL;

    /* Read input, write an annotation for each line beginning with a positive
       number. */
    while (fgets(line, sizeof(line), stdin) != NULL) {
	if (!wflag) { /* -w option not used, write all annotations as NORMAL */
	    if (sscanf(line, "%lf", &rr) == 0) continue;
	}
        else {	      /* -w option used, copy annotation type from input */
	    static char astring[MAXLINELEN];

	    if (sscanf(line, "%lf%s", &rr, astring) < 2) continue;
	    if ((annot.anntyp = strann(astring)) == NOTQRS) continue;
	}
	if (rr <= 0.0) continue;
	else if (Tflag) t = rr * xfactor;
	else t += rr * xfactor;
	annot.time = (WFDB_Time)(t + 0.5);
	(void)putann(0, &annot);
    }

    /* Flush buffered output. */
    wfdbquit();
    exit(0);
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
 "usage: %s -r RECORD -a ANNOTATOR [OPTIONS ...] <TEXT-FILE\n",
 "where RECORD and ANNOTATOR specify the output, and OPTIONS may include:",
 " -F FREQ     use FREQ as the sampling frequency (effective only if no"
 "               header file exists;  default: 250)",
 " -h          print this usage summary",
 " -T          TEXT-FILE contains times of occurrence, not RR intervals",
 " -x N        multiply input by N to obtain RR in sample intervals",
 "               (default: N = 1)",
 " -w          copy annotation types from second column of input",
 "TEXT-FILE should contain a column of RR intervals (or times of occurrence,",
 "if the -T option is specified), in units of sample intervals (unless the",
 "-x option is used to specify a conversion factor).   The first token on",
 "each line is taken as an RR interval, and (if the -w option is specified)",
 "the second token is taken as an annotation mnemonic (N, V, etc.);  anything",
 "else on the same line is ignored, as are empty lines, spaces and tabs at",
 "the beginning of a line non-numeric tokens and anything following them on",
 "the same line, negative intervals, and zero intervals.  The output consists",
 "of a binary annotation file (RECORD.ANNOTATOR), and (if it does not exist",
 "already) a text header file (RECORD.hea).",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
