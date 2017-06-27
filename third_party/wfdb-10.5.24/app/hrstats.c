/* file: hrstats.c		G. Moody	 18 March 2012
				Last revised:	  6 April 2012
-------------------------------------------------------------------------------
hrstats: Collect and summarize heart rate statistics
Copyright (C) 1985-2012 George B. Moody

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
smoothed instantaneous heart rate signal, using the IPFM model as originally
implemented in tach.c.  In this context, heart rate (HR) is defined as ten
times the number of beat-to-beat (RR) intervals (and fractional intervals)
within a 6-second HR measurement window, including intervals beginning and/or
ending with ectopic beats, and RR intervals are defined by the locations of
consecutive beat annotations in the annotation file.

The first HR window starts at the beginning of the first RR interval in the
record and ends 6 seconds later. Subsequent HR windows begin at 1-second
intervals following the first, i.e, they overlap by 5/6 (83.33%).  HR windows
that contain part or all of a very long (>3 sec) or very short (<0.2 sec)
interval are discarded.  All others are used to generate HR measurements that
are accumulated in a histogram with 1 bpm bins.

When all intervals have been processed, summary statistics calculated from the
histogram are written to the standard output and to a WFDB '.info' file,
in this format:

 <HR>: 71|73/75/81|86 +-1.8 bpm [atr]

This example is the output of 'hrstats -r mitdb/100 -a atr'.  From left to
right, it shows:
 the characteristic that is summarized [HR]
 extreme low value [in the example, 71]
 5th percentile [73]
 mean (trimmed, excluding outliers below 5th or above 95th percentiles) [75]
 95th percentile [81]
 extreme high value [86]
 sample deviation (of values included in the trimmed mean) [1.8]
 units of all statistics [bpm]
 source of data [atr annotations]

Optionally, the HR histogram can also be written to a file containing two
columns separated by a tab.  The second column contains the number of
measurements of heart rate falling within 0.5 bpm of the heart rate in the
first column.  The histogram includes only heart rates between the extreme
low and extreme high heart rates inclusive.
*/

#include <stdio.h>
#include <math.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

#define HRMAX	 500	/* maximum heart rate (bpm) */
#define TBMAX	 128	/* length of ix[] and tb[] (must be a power of 2) */

/* Note that iexcl() and tbeat() must be usable as lvalues! */
#define iexcl(A) (ix[(A)&(TBMAX-1)])	/* exclude interval A if non-zero */
#define tbeat(A) (tb[(A)&(TBMAX-1)])	/* time of beat following interval A */

char *irec, *aname, *ofname, *pname, *prog_name();
int hrhist[HRMAX+1], ix[TBMAX], tot, wexcl;
void cleanup(), help();
WFDB_Anninfo ai;
WFDB_Time tb[TBMAX], getbeat();

main(int argc, char **argv)
{
    double dt, left, right, rr, rrcnt, rrmin, rrmax, sps, t;
    int i, hr;
    long max = 0, min = 0, n;
    WFDB_Time tmaxb, tminb;

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
	    ai.name = aname = argv[i];
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'o':	/* histogram output file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: histogram output file name must follow -o\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    break;
	  case 'r':	/* input record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    irec = argv[i];
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

    if ((sps = sampfreq(irec)) <= 0.) {
	(void)setsampfreq(sps = WFDB_DEFFREQ);
	fprintf(stderr,
		"%s (warning): sampling frequency unspecified for record %s,"
		"(assuming %g Hz)\n",
		pname, irec, sps);
    }
    dt = 3 * sps;	/* window half-width */
    rrmax = 3 * sps;	/* maximum valid RR interval */
    rrmin = 0.2 * sps;	/* minimum valid RR interval */
    tbeat(-1) = -1;	/* avoid division by zero if tbeat(0) is 0 */

    ai.stat = WFDB_READ;
    if (annopen(irec, &ai, 1) < 0)	/* open annotation file */
	exit(2);

    /* Start with the left edge of the window at the time of the first beat,
       then read and process the remainder of the annotations. */
    for (tbeat(min) = getbeat(), t = tbeat(min)+dt; max-min < TBMAX; t += sps) {
	/* Reset the window limits. */
	left = t - dt; right = t + dt;

	/* read additional beats as needed to fill the window */
	while ((tmaxb = tbeat(max)) <= right) {
	    tbeat(++max) = getbeat();
	    rr = tbeat(max) - tbeat(max-1);
	    /* interval is invalid if not in bounds */
	    iexcl(max) = !(rrmin < rr && rr < rrmax);
	    /* window is invalid if it includes any invalid intervals */
	    wexcl += iexcl(max);
	}
	
	/* advance min as needed until tbeat(min) is in the window */
	while ((tminb = tbeat(min)) <= left)
	    wexcl -= iexcl(min++);
	if (wexcl == 0) { /* all intervals in the window are valid */
	    /* get the whole and fractional number of intervals in the window */
	    rrcnt = max - min + (tminb - left)  / (tminb - tbeat(min-1))
			      - (tmaxb - right) / (tmaxb - tbeat(max-1));
	    hr = (int)(10 * rrcnt + 0.5);  /* multiply beats in 6 seconds by
					      10 to get beats per minute */
	    if (hr > HRMAX) hr = HRMAX;
	    hrhist[hr]++;
	    tot++;
	}
    }
    if (max >= min + TBMAX) {
	fprintf(stderr, "%s: annotation buffer overflow\n", pname);
	cleanup();
    }
}

WFDB_Time getbeat()
{
    int stat;
    WFDB_Annotation annot;

    while ((stat = getann(0, &annot)) >= 0 && !isqrs(annot.anntyp))
	;
    if (stat < 0)
	cleanup();
    return (annot.time);
}

void cleanup()
{
    char hrbuf[80];
    double mean, sampdev, ssum, sum, target;
    int extreme_high, extreme_low, i, imean, max95, min5, n, outliers;

    wfdbquit();

    /* derive statistics from histograms */
    for (i = 0; i <= HRMAX; i++)
	if (hrhist[i]) break;
    if (i > HRMAX) {
	fprintf(stderr, "%s: no HR data for record %s, annotator %s\n",
		pname, irec, aname);
	exit(1);
    }
    extreme_low = i;

    for (i = HRMAX; i >= extreme_low; i--)
	if (hrhist[i]) break;
    extreme_high = i;

    /* if requested, write histogram to specified file */
    if (ofname) {
	FILE *ofile;

	if ((ofile = fopen(ofname, "wt")) == NULL) {
	    fprintf(stderr, "%s: can't write %s\n", pname, ofname);
	}
	else {
	    for (i = extreme_low; i <= extreme_high; i++)
		fprintf(ofile, "%d\t%d\n", i, hrhist[i]);
	    fclose(ofile);
	}
    }

    for (i = extreme_low, n = 0, target = 0.05 * tot; n <= target; i++)
	n += hrhist[i];
    /* Adjust the count in the 5th percentile bin so that exactly 5% of
       the lowest values are excluded from the mean. */
    hrhist[--i] = n - target;
    min5 = i;	/* 5th percentile */

    for (i = extreme_high, n = 0; n <= target; i--)
	n += hrhist[i];
    /* Adjust the count in the 95th percentile bin so that exactly 5% of
       the highest values are excluded from the mean. */
    hrhist[++i] = n - target;
    max95 = i;	/* 95th percentile */

    for (i = min5, n = sum = 0; i <= max95; i++) {
	n += hrhist[i];
	sum += hrhist[i]*i;
    }
    mean = sum / n;
    imean = (int)(mean + 0.5);

    for (i = min5, ssum = 0; i <= max95; i++)
	ssum += hrhist[i] * (i - mean) * (i - mean);
    sampdev = sqrt(ssum / n);

    /* write statistics to standard output */
    sprintf(hrbuf, " <HR>: %d|%d/%d/%d|%d +-%.2g bpm [%s]",
	   extreme_low, min5, imean, max95, extreme_high, sampdev, aname);
    printf("%s\n", hrbuf);
    setinfo(irec);
    putinfo(hrbuf);
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
 "usage: %s -r RECORD -a ANNOTATOR [OPTIONS ...]\n",
 "where RECORD and ANNOTATOR specify the input; OPTIONS may include any of:",
" -h            print this usage summary",
" -o HISTFILE   save histogram in HISTFILE",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
