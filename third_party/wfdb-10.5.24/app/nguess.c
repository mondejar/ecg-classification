/* file: nguess.c		G. Moody	9 June 1986
				Last revised:  11 June 2008
-------------------------------------------------------------------------------
nguess: Guess the times of missing normal sinus beats in an annotation file
Copyright (C) 1986-2008 George B. Moody

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

This program copies its input (a WFDB annotation file containing beat
annotations), removing annotations of events other than sinus beats, and
interpolating additional Q (unknown beat) annotations at times when sinus beats
are expected.  Intervals between sinus beats are predicted using a predictor
array as described by Paul Schluter ("The design and evaluation of a bedside
cardiac arrhythmia monitor";  Ph.D. thesis, MIT Dept. of Electrical
Engineering, 1981).  When the predictions are inconsistent with the known sinus
beats, as may occur in extreme noise or in highly irregular rhythms such as
atrial fibrillation, no interpolations are made.

It should be understood that, as the name of this program implies, the Q labels
it generates represent, at best, good guesses about the times at which sinus
beats may be expected.  Ideally, one should avoid having to make such guesses,
but some commonly-used techniques for study of heart rate variability (for
example, conventional methods for power spectral density estimation in the
frequency domain) require a uniformly sampled instantaneous heart rate signal,
such as can be obtained using 'tach' to process the output of 'nguess'.  Other
techniques, such as the Lomb periodogram method implemented by 'lomb', can
obtain frequency spectra from time series with missing and irregularly spaced
values, such as can be produced from a beat annotation file using 'ihr' without
the need to use 'nguess'.  Use 'nguess' only when necessary and do not expect
it to perform miracles; as a rule of thumb, if the number of guesses (Q
annotations) exceeds one or two percent of the number of known sinus beats (N
annotations), be exceedingly wary of the guesses and consider using techniques
such as 'lomb' that do not require the use of 'nguess'.  Also as a general
rule, 'nguess' works best when it is guessing the locations of sinus beats
obscured by noise, or those of sinus beats that were inhibited by isolated
premature ventricular beats; the underlying hypothesis of a quasi-continuous
sinus rhythm, the basis not only of 'nguess' but also of all other algorithms
for reconstructing NN interval time series, is most suspect in the context of
supraventricular ectopic beats (which may reset the SA node, thus interrupting
the sinus rhythm) and consecutive ventricular ectopic beats.

The predictor array method is based on the observation that most of the
short-term variability in normal sinus inter-beat (NN) intervals is due to
respiratory sinus arrhythmia (RSA, the quasi-periodic modulation of heart rate
by respiration, which is most notable in young, healthy subjects and decreases
with age).  Since respiration rate is (in humans and smaller mammals)
substantially slower than heart rate, it is possible to estimate the length of
the respiratory cycle in terms of some number of NN intervals.  If, for
example, heart rate is around 60 beats per minute and respiration rate is
around 10 breaths per minute, one might expect that 6 NN intervals would
correspond to one breath, and that the current interval might be particularly
well-approximated by the sixth previous interval.  Since we don't know the
ratio between heart and respiration rate a priori, we can observe how well each
of the previous PBLEN intervals predicts the current interval on average.  Thus
we have PBLEN predictors for each interval, some of which may be much better on
average than others.  At any time, we know which predictor is (locally) the
best, and we can use that predictor to make a best guess of the location of the
next sinus beat.  In subjects with significant RSA, the best predictor may be
determined by the length of the respiratory cycle; in others, the previous beat
may be a better predictor.  For our purposes, it really doesn't matter which
predictor is best, only that the mean error of the best predictor is small.  If
the next known sinus beat is at least 1.75 times as distant as the prediction,
and if the predictions are reasonably good on average, 'nguess' asserts that a
gap exists and fills it in with a Q annotation (or more than one, if the gap is
sufficiently long).
*/

#include <stdio.h>
#include <math.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>
#define PBLEN	12	/* size of predictor array */

char *pname, *prog_name();

main(argc, argv)
int argc;
char *argv[];
{
    char *record = NULL;
    static double alpha = 0.75, bestpe, pe[PBLEN], rrmax, sps, worstpe;
    static int b, best, i, n, worst;
    static long rr[PBLEN], rrsum;
    static WFDB_Anninfo an[2];
    static WFDB_Annotation in_ann, out_ann;
    static WFDB_Time from, from0, next, to;
    void help();

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    an[0].name = argv[i];
	    break;
	  case 'f':	/* starting time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: starting time must follow -f\n",
			      pname);
		exit(1);
	    }
	    from = i;   /* to be converted to sample intervals below */
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'm':  /* multiple of predicted interval needed for inserting Q */
	    if (++i >= argc || (alpha = atof(argv[i]) - 1.0) <= 0) {
		(void)fprintf(stderr, "%s: multiplier (> 1) must follow -m\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'o':	/* output annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output annotator must follow -o\n",
			      pname);
		exit(1);
	    }
	    an[1].name = argv[i];
	    break;
	  case 'r':	/* input record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: input record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 't':	/* ending time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    to = i;
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

    /* Check that required arguments are present. */
    if (record == NULL || an[0].name == NULL) {
	help();
	exit(1);
    }
    an[0].stat = WFDB_READ;
    if (an[1].name == NULL) an[1].name = "nguess";
    an[1].stat = WFDB_WRITE;

    /* Open the input and output annotation files. */
    if (annopen(record, an, 2) < 0)
	exit(2);
    wfdbquiet();
    if ((sps = sampfreq(record)) < 0) {
	fprintf(stderr,
		"%s: can't read header for record %s, assuming %g Hz\n",
		pname, record, sps = WFDB_DEFFREQ);
	setsampfreq(sps);
    }
    wfdbverbose();
    
    if (from) {
	from = strtim(argv[(int)from]);
	if (from < (WFDB_Time)0) from = -from;
    }
    if (to) {
	to = strtim(argv[(int)to]);
	if (to < (WFDB_Time)0) to = -to;
    }
    if (to < from && to > 0L) {
	WFDB_Time tt = from;

	from = to;
	to = tt;
    }

    rrmax = sps/2.0;

    /* Begin processing a minute before the specified start time, if possible.
       No output is produced until the specified start time, but the extra
       minute gives the program a chance to `learn' the rhythm. */
    if ((from0 = from - strtim("1:0")) < 0L)
	from0 = 0L;
    if (from0 > 0L && iannsettime(from0) < 0) {
	wfdbquit();
	exit(0);
    }

    /* Get the first normal beat annotation after from0. */
    getnormal(&in_ann);

    out_ann.time = in_ann.time;

    /* Once past `from', emit an output annotation for each iteration of the
       loop below. */
    while (to <= 0L || out_ann.time < to) {
	/* Get (in in_ann) the normal beat annotation that occurs nearest the
	   prediction, and get the following normal beat annotation in
	   in_ann.  Once the predictor array is full, the prediction is
	   updated on each iteration, and the following lines discard
	   interpolated beats and false detections.  The assumption is that
	   the true sinus rate never doubles instantaneously. */
	while (map2(in_ann.anntyp) != NORMAL ||
		in_ann.time < next)	/* get input annot if needed */
	    getnormal(&in_ann);

	/* From the previous RR intervals, choose the best predictor. */
	for (i = n, bestpe = 99999., worst = 0.; i > 0; i--) {
	    double error;

	    /* Put an upper bound on the prediction error, to limit the
	       influence of a single observation on pe[i]. */
	    if ((error = fabs(rr[i] - rr[0])) > sps/2) error = sps/2;
	    if (rr[i] > 0 &&
		(pe[i] += (error - pe[i])/20.) <= bestpe) {
		bestpe = pe[i];
		best = i; 
	    }		/* find best predicting interval */
	    if (pe[i] >= worstpe) {
		worstpe = pe[i];
		worst = i;
	    }
	    rr[i] = rr[i-1];	/* shift RR buffer */
	}

	/* Define the current RR interval as the difference between the time
	   of the current normal beat and the time of the most recently
	   written output (beat) annotation. */
	rr[0] = in_ann.time - out_ann.time;

	/* If the predictor array is not yet full, just copy the input
	   annotation to the output (if it falls after `from'). */
	if (n < PBLEN-1) {
	    n++;
	    out_ann.anntyp = in_ann.anntyp;
	    in_ann.anntyp = NOTQRS;
	    if ((out_ann.time += rr[0]) > from)
		putann(0, &out_ann);
	    continue;
	}

	/* If the interval is long enough, search for a gap (corresponding
	   to a non-conducted P-wave, or, more likely, an undetected sinus
	   beat). */
	i = 0;	/* expected number of RR intervals contained within rr[0] */
	if (rr[0] > rrmax) {
	    b = best;	/* index of best predicting interval */
	    rrsum = 0;	/* sum of i predicted intervals */
	    while (rr[0] > rrsum + rr[b]*alpha) {
		rrsum += rr[b];
		if (--b <= 0) b = best;  /* get next prediction index */
		i++;
	    }
	}
	if (i > 1) {	/* gap found -- prepare to insert an annotation */
	    rr[0] = (long)(0.5 + ((double)rr[0]*rr[best])/(double)rrsum);
	    out_ann.anntyp = UNKNOWN;
	}
	else {		/* no gap -- prepare to copy the input annotation */
	    out_ann.anntyp = in_ann.anntyp;
	    in_ann.anntyp = NOTQRS;
	}

	if ((out_ann.time += rr[0]) > from)
	    putann(0, &out_ann);	/* emit an output annotation */

	/* If rr[0] doesn't match the best predicting interval reasonably well,
	   adjust it to a weighted sum of the predicted and observed intervals.
	   The weighting heavily favors the prediction, on the assumption
	   that the observation is in error (or is unrepresentative of sinus
	   rhythm). */
	if (fabs(rr[best] - rr[0]) > rr[best]/10)
	    rr[0] = (9*rr[best] + rr[0])/10;

	/* Predict the time of the next sinus beat, if the best and worst
	   predictions are in reasonable agreement. */
	if (fabs(rr[best] - rr[worst]) < rr[best]/8)
	    next = out_ann.time + 
	         (rr[best-1] < sps) ? (19*rr[best-1])/20 : 19*sps/20;
    }
}

getnormal(ap)
WFDB_Annotation *ap;
{
    do {
	if (getann(0, ap) < 0) {
	    wfdbquit();
	    exit(0);
	}
    } while (map2(ap->anntyp) != NORMAL);
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
 "where RECORD and ANNOTATOR specify the input, and OPTIONS may include:",
 " -f TIME    start at specified TIME",
 " -h         print this usage summary",
 " -m M       insert a Q if RR > M * prediction (M > 1; default: M = 1.75)",
 " -o OANN    write output as annotator OANN (default: nguess)",
 " -t TIME    stop at specified TIME",
 "The output contains copies of all N annotations, with additional Q",
 "annotations inserted at the inferred locations of missing N (sinus) beats.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++) {
	(void)fprintf(stderr, "%s\n", help_strings[i]);
	if (i % 23 == 0) {
	    char b[5];
	    (void)fprintf(stderr, "--More--");
	    (void)fgets(b, 5, stdin);
	    (void)fprintf(stderr, "\033[A\033[2K");  /* erase "--More--";
						      assumes ANSI terminal */
	}
    }
}
