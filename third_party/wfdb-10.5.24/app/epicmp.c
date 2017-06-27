/* file: epicmp.c	G. Moody       3 March 1992
			Last revised:   9 May 2010

-------------------------------------------------------------------------------
epicmp: ANSI/AAMI-standard episode-by-episode annotation file comparator
Copyright (C) 1992-2010 George B. Moody

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

This program (formerly known as 'epic') implements the VF, AF, and ST episode-
by-episode comparison algorithms described in ANSI/AAMI EC38:1998, the American
National Standard for Ambulatory electrocardiographs and ANSI/AAMI EC57:1998,
the American National Standard for Testing and reporting performance results of
cardiac rhythm and ST segment measurement algorithms;  both standards are
available from AAMI, 1110 N Glebe Road, Suite 220, Arlington, VA 22201 USA
(http://www.aami.org/).  The relevant provisions of these standards are
described in file `eval.tex', and information about using this program is
contained in file `epicmp.1' (both of these files are included in the
'doc/wag-src/' directory of the WFDB Software Package).

The -f and -t options modify the comparison algorithm used by epicmp in ways
not permitted by these standards.  These options are provided for the use of
developers, who may find them useful for obtaining a more detailed
understanding of algorithm errors.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#define map1
#define map2
#define ammap
#define mamap
#define annpos
#include <wfdb/ecgmap.h>

/* Episode types */
#define AFE	0	/* atrial fibrillation */
#define VFE	1	/* ventricular flutter or fibrillation */
#define ST0E	2	/* ischemic ST in signal 0 */
#define ST1E	3	/* ischemic ST in signal 1 */
#define STE	4	/* ischemic ST in either signal */
#define AFLE	5	/* atrial flutter */

#define MAXEXCL	100	/* maximum number of intervals excluded from
			   comparison */

int aflag, sflag, s0flag, s1flag, vflag, xflag;
char *lzmstimstr(), *zmstimstr();

main(argc, argv)
int argc;
char *argv[];
{
    void epicmp(), init(), print_results(), stdc();

    /* Read and interpret command-line arguments. */
    init(argc, argv);

    if (aflag) {
	epicmp(0, AFE);		/* check AF Se */
	epicmp(1, AFE);		/* check AF +P */
	print_results(AFE);	/* print AF statistics */
    }

    if (vflag) {
	epicmp(0, VFE);		/* check VF Se */
	epicmp(1, VFE);		/* check VF +P */
	print_results(VFE);	/* print VF statistics */
    }

    if (s0flag) {
	epicmp(0, ST0E);	/* check signal 0 ischemic ST Se */
	epicmp(1, ST0E);	/* check signal 0 ischemic ST +P */
	stdc(0);	       	/* check signal 0 ST deviation measurements */
	print_results(ST0E);	/* print signal 0 ischemic ST statistics */
    }

    if (s1flag) {
	epicmp(0, ST1E);	/* check signal 1 ischemic ST Se */
	epicmp(1, ST1E);	/* check signal 1 ischemic ST +P */
	stdc(1);	       	/* check signal 1 ST deviation measurements */
	print_results(ST1E);	/* print signal 1 ischemic ST statistics */
    }

    if (sflag) {
	epicmp(0, STE);		/* check two-signal ischemic ST Se */
	epicmp(1, STE);		/* check two-signal ischemic ST +P */
	stdc(2);	       	/* check all ST deviation measurements */
	print_results(STE);	/* print two-signal ischemic ST statistics */
    }

    exit(0);	/*NOTREACHED*/
}

char *pname;		/* name by which this program was invoked */
char *record;
char *afname, *sfname, *s0fname, *s1fname, *vfname;
int Iflag, match, mismatch, nexcl, overlap_ex0, overlap_ex1;
long start_time, end_time, min_length, total_duration, total_overlap;
long ep_start[2], ep_ex0[2], ep_ex1[2], ep_end[2];
long ex_start[MAXEXCL], ex_end[MAXEXCL];
long ref_duration, ref_overlap, test_duration, test_overlap, STP, FN, PTP, FP;
WFDB_Anninfo an[2];

/* Perform an episode-by-episode comparison. */
void epicmp(stat, type)
unsigned int stat, type;
{
    int i;
    unsigned int a, b;
    long duration, overlap, find_overlap();
    void find_episode(), find_exclusions();

    /* Find and mark any intervals to be excluded from the comparison. */
    if (xflag)
	find_exclusions(stat, type);
    else
	nexcl = 0;

    /* Return to the beginning of the annotation files. */
    if (iannsettime(0L) < 0) exit(2);

    /* If stat is 0, check sensitivity;  in this case, annotator 0 defines the
       location of episodes. */
    if (stat == 0) {
	a = 0;
	b = 1;
	ref_duration = ref_overlap = STP = FN = 0L;
    }
    /* Otherwise, check positive predictivity;  in this case, annotator 1
       defines the location of episodes. */
    else {
	a = 1;
	b = 0;
	test_duration = test_overlap = PTP = FP = 0L;
    }

    /* Initialize state variables. */
    match = mismatch = 0;
    total_duration = total_overlap = 0L;
    ep_start[0] = ep_start[1] = ep_end[0] = ep_end[1] = 0L;
    ep_ex0[0] = ep_ex0[1] = ep_ex1[0] = ep_ex1[1] = 0L;

    /* Find the first annotator a episode that ends during the test period. */
    do {
	find_episode(a, type);
    } while (0L < ep_end[a] && ep_end[a] < start_time);

    /* Quit immediately if there are no such episodes. */
    if (ep_end[a] == 0L) return;

    /* Adjust the starting time of the annotator a episode if it begins during
       the learning period. */
    if (ep_start[a] < start_time) ep_start[a] = start_time;

    /* Find the first annotator b episode. */
    find_episode(b, type);

    /* Process an annotator a episode for each iteration of this loop. */
    while (ep_end[a] && (end_time == 0L || ep_start[a] < end_time)) {
	/* For ST comparisons only, check if each extremum occurs during the
	   test period;  if not, disable checking of the extremum by setting
	   its time to 0. */
	if (type == ST0E || type == ST1E || type == STE) {
	    if (ep_ex0[a] <= start_time || ep_ex0[a] >= end_time)
		ep_ex0[a] = 0L;
	    if (ep_ex1[a] <= start_time || ep_ex1[a] >= end_time)
		ep_ex1[a] = 0L;
	}

	/* Adjust the episode ending time if it ends after the end of the
	   test period. */
	if (end_time > 0L && ep_end[a] > end_time) ep_end[a] = end_time;

	duration = ep_end[a] - ep_start[a];

	/* Adjust the episode start, end, and duration if it overlaps an
	   exclusion. */
	for (i = 0; i < nexcl; i++) {
	    if (ep_end[a] <= ex_start[i])
		break;		 /* no more overlaps possible */
	    if (ex_start[i] <= ep_start[a] && ep_start[a] <= ex_end[i]) {
		/* episode begins during an exclusion */
		if (ep_end[a] <= ex_end[i]) {
		    /* entire episode is contained in the exclusion */
		    duration = -1L;	/* ignore it */
		    break;
		}
		else {
		    /* only the beginning of the episode is excluded --
		       adjust duration and start */
		    duration -= ep_start[a] - ex_end[i];
		    ep_start[a] = ex_end[i];
		}
	    }
	    else if (ep_start[a] <= ex_start[i] && ex_start[i] <= ep_end[a]) {
		/* exclusion begins during an episode */
		if (ex_end[i] < ep_end[a])
		    /* entire exclusion is contained in the episode --
		       adjust duration but leave start and end alone */
		    duration -= ex_end[i] - ex_start[i];
		else {
		    /* the end of the episode is excluded -- adjust duration
		       and end */
		    duration -= ep_end[a] - ex_start[i];
		    ep_end[a] = ex_start[i];
		}
	    }
	}

	/* If the episode is too short, ignore it. */
	if (duration < min_length) {
	    find_episode(a, type);
	    continue;
	}

	/* Add the duration of this episode to the tally. */
	total_duration += duration;

	/* Get the duration of overlap;  update the count of matching
	   episodes if appropriate, and update the overlap tally. */
	if ((overlap = find_overlap(b, type)) > 0L) {
	    switch (type) {
	      case AFE:		/* AF */
	      case VFE:		/* VF */
		match++;
		break;
	      case ST0E:	/* ST, signal 0 */
		if (overlap_ex0 || overlap * 2L >= duration)
		    match++;
		else
		    mismatch++;
		break;
	      case ST1E:	/* ST, signal 1 */
		if (overlap_ex1 || overlap * 2L >= duration)
		    match++;
		else
		    mismatch++;
		break;
	      case STE:
		if (overlap_ex0 || overlap_ex1 || overlap * 2L >= duration)
		    match++;
		else
		    mismatch++;
		break;
	    }
	    total_overlap += overlap;
	}
	/* If there was no overlap, update the count of mismatched episodes. */
	else
	    mismatch++;

	/* Get the next episode. */
	find_episode(a, type);
    }

    if (a == 0) {
	STP = match;
	FN = mismatch;
	ref_duration = total_duration;
	ref_overlap = total_overlap;
    }
    else {
	PTP = match;
	FP = mismatch;
	test_duration = total_duration;
	test_overlap = total_overlap;
    }
}

int lflag;
long min_length;

void find_episode(annotator, type)
unsigned int annotator, type;
{
    int stat, stcount = 0;
    long tt;
    static WFDB_Annotation annot;

    ep_start[annotator] = ep_ex0[annotator] = ep_ex1[annotator] =
	ep_end[annotator] = 0L;

    /* Find beginning of episode. */
    while ((stat = getann(annotator, &annot)) == 0) {
	if (type == AFE) {	/* atrial fibrillation */
	    if (annot.anntyp==RHYTHM && strncmp(annot.aux+1,"(AFIB",5) == 0) {
		ep_start[annotator] = annot.time;
		break;
	    }
	}
	else if (type == VFE) {	/* VF */
	    if (annot.anntyp == VFON) {
		ep_start[annotator] = annot.time;
		break;
	    }
	}
	else if (type == ST0E) {/* ST episode, signal 0 */
	    if (annot.anntyp == STCH && strncmp(annot.aux+1,"(ST0",4) == 0) {
		ep_start[annotator] = annot.time;
		break;
	    }
	}
	else if (type == ST1E) {/* ST episode, signal 1 */
	    if (annot.anntyp == STCH && strncmp(annot.aux+1,"(ST1",4) == 0) {
		ep_start[annotator] = annot.time;
		break;
	    }
	}
	else if (type == STE) {	/* ST episode, either signal */
	    if (annot.anntyp == STCH && strncmp(annot.aux+1,"(ST",3) == 0) {
		ep_start[annotator] = annot.time;
		stcount = 1;
		break;
	    }
	}
	else if (type == AFLE) {/* atrial flutter */
	    if (annot.anntyp==RHYTHM && strncmp(annot.aux+1,"(AFL",4) == 0) {
		ep_start[annotator] = annot.time;
		break;
	    }
	}
    }

    /* Quit if at end of annotation file. */
    if (stat != 0) return;
    tt = annot.time;

    /* Find end of episode. */
    while ((stat = getann(annotator, &annot)) == 0) {
	tt = annot.time;
	if (type == AFE) {	/* atrial fibrillation */
	    if (annot.anntyp==RHYTHM && strncmp(annot.aux+1,"(AFIB",5) != 0) {
		ep_end[annotator] = annot.time;
		break;
	    }
	}
	else if (type == VFE) {	/* VF */
	    if (annot.anntyp == VFOFF) {
		ep_end[annotator] = annot.time;
		break;
	    }
	}
	else if (type == ST0E) {/* ST episode, signal 0 */
	    if (annot.anntyp == STCH) {
		if (strncmp(annot.aux+1, "ST0", 3) == 0) {
		    ep_end[annotator] = annot.time;
		    break;
		}
		else if (strncmp(annot.aux+1, "AST0", 4) == 0)
		    ep_ex0[annotator] = annot.time;
	    }
	}
	else if (type == ST1E) {/* ST episode, signal 1 */
	    if (annot.anntyp == STCH) {
		if (strncmp(annot.aux+1, "ST1", 3) == 0) {
		    ep_end[annotator] = annot.time;
		    break;
		}
		else if (strncmp(annot.aux+1, "AST1", 4) == 0)
		    ep_ex1[annotator] = annot.time;
	    }
	}
	else if (type == STE) {	/* ST episode, either signal */
	    if (annot.anntyp == STCH) {
		if (strncmp(annot.aux+1, "(ST", 3) == 0)
		    stcount++;
		else if (strncmp(annot.aux+1, "AST0", 4) == 0)
		    ep_ex0[annotator] = annot.time;
		else if (strncmp(annot.aux+1, "AST1", 4) == 0)
		    ep_ex1[annotator] = annot.time;
		else if (strncmp(annot.aux+1, "ST", 2) == 0 && --stcount==0) {
		    ep_end[annotator] = annot.time;
		    break;
		}
	    }
	}
	else if (type == AFLE) {/* atrial flutter */
	    if (annot.anntyp==RHYTHM && strncmp(annot.aux+1,"(AFL",4) != 0) {
		ep_end[annotator] = annot.time;
		break;
	    }
	}
    }

    /* If end of annotation file was reached, episode did not terminate --
       assume that it ended at the greater of tt (the time of the previous
       annotation) and end_time. */
    if (stat != 0)
	ep_end[annotator] = (end_time > tt) ? end_time : tt;
}

/* This function finds and marks (in the ex_start and ex_end arrays) any
   intervals that are to be excluded from the comparison.  The only case
   in which such intervals are currently defined is that of atrial fibrillation
   positive predictivity comparisons, from which intervals of reference-marked
   atrial flutter are excluded. */

void find_exclusions(stat, type)
unsigned int stat, type;
{
    nexcl = 0;
    if (stat == 1 && type == AFE) {
	if (iannsettime(0L) < 0) exit(2);

	/* Find the first exclusion that ends after the beginning of the test
	   period. */
	do {
	    find_episode(0, AFLE);
	} while (0L < ep_end[0] && ep_end[0] < start_time);

	/* If there is no such exclusion, return immediately. */
	if (ep_end[0] == 0L) return;

	/* Mark one exclusion on each iteration of this loop. */
	while (0L < ep_end[0] && (end_time == 0L || ep_start[0] < end_time)) {
	    if (nexcl > MAXEXCL) {
		(void)fprintf(stderr,
	    "%s: (warning) too many exclusions in record %s\n", pname, record);
		(void)fprintf(stderr,
" Use the -f and -t options to process this record in segments, or recompile");
		(void)fprintf(stderr,
			      "%s with a larger value of MAXEXCL.\n", pname);
		return;
	    }
	    ex_start[nexcl] = ep_start[0];
	    ex_end[nexcl++] = ep_end[0];
	    find_episode(0, AFLE);
	}
    }
}

/* This function determines the duration of any episodes of the specified
   type according to the specified annotator within a window defined by
   ep_start[1-annotator] and ep_end[1-annotator].  It also sets the flags
   overlap_ex0 and overlap_ex1 if the period of overlap includes the times
   ep_ex0[1-annotator] and ep_ex1[1-annotator]. */

long find_overlap(annotator, type)
unsigned int annotator, type;
{
    long overlap = 0L, o_start, o_end;

    overlap_ex0 = overlap_ex1 = 0;

    /* Return immediately if there are no more episodes. */
    if (ep_end[annotator] == 0L) return (overlap);

    /* If the current episode ended before the beginning of the window, get
       another. */
    while (ep_end[annotator] < ep_start[1-annotator]) {
	find_episode(annotator, type);
	if (ep_start[annotator] == 0L) return (overlap);
    }

    /* If the current episode starts after the end of the window, return
       immediately. */
    if (ep_start[annotator] > ep_end[1-annotator]) return (overlap);

    /* There is at least some overlap -- determine its extent. */
    do {
	/* Ignore any episodes shorter than the minimum length if using -I */
	if (Iflag && ep_end[annotator] - ep_start[annotator] < min_length) {
	    find_episode(annotator, type);
	    continue;
	}

	/* Ignore any portion of the current episode that precedes the
	   beginning of the window. */
	if (ep_start[annotator] < ep_start[1-annotator])
	    o_start = ep_start[1-annotator];
	else
	    o_start = ep_start[annotator];

	/* Ignore any portion of the current episode that follows the end of
	   the window. */
	if (ep_end[annotator] > ep_end[1-annotator])
	    o_end = ep_end[1-annotator];
	else
	    o_end = ep_end[annotator];

	/* Tally the overlap. */
	overlap += o_end - o_start;

	/* If the definition of exclusions is ever changed, it may be necessary
	   to check explicitly here to see if the interval from o_start to
	   o_end includes any periods that are to be excluded from the
	   comparison; in such a case, the duration of such periods must be
	   subtracted from the overlap tally.  Given the current definition of
	   exclusions, this case should never happen. */

	/* Check if overlap includes extrema. */
	if (o_start <= ep_ex0[1-annotator] && ep_ex0[1-annotator] <= o_end)
	    overlap_ex0 = 1;
	if (o_start <= ep_ex1[1-annotator] && ep_ex1[1-annotator] <= o_end)
	    overlap_ex1 = 1;

	/* If the current episode ends after the end of the window, stop. */
	if (ep_end[annotator] >= ep_end[1-annotator])
	    break;
	/* Otherwise, get the next episode and see if it also falls in the
	   window. */
	find_episode(annotator, type);
    } while (0L < ep_start[annotator] &&
	     ep_start[annotator] < ep_end[1-annotator]);
    return (overlap);
}	

FILE *ofile;
long tref;		/* time of the most recent reference ST extremum */
int sigref, stref;	/* signal number and ST deviation for the most recent
			   reference ST extremum */

/* This function finds the next reference ST extremum annotation and sets the
   variables tref, sigref, and stref appropriately. */

int find_reference_extremum(mode)
int mode;	/* 0: signal 0 only, 1: signal 1 only, 2: both signals */
{
    WFDB_Annotation refann;

    while (getann(0, &refann) == 0) {
	if (refann.anntyp == STCH && refann.aux != NULL &&
	    strncmp(refann.aux+1, "AST", 3) == 0) {
	    switch (*(refann.aux+4)) {
	      case '0':	/* signal 0 extremum */
		if (mode != 1) {
		    tref = refann.time;
		    sigref = 0;
		    stref = atoi(refann.aux+5);
		    return (1);
		}
		break;
	      case '1':	/* signal 1 extremum */
		if (mode != 0) {
		    tref = refann.time;
		    sigref = 1;
		    stref = atoi(refann.aux+5);
		    return (1);
		}
		break;
	      case '+':	/* extremum in single-lead record */
	      case '-':
		if (mode != 1) {
		    tref = refann.time;
		    sigref = 0;
		    stref = atoi(refann.aux+4);
		    return (1);
		}
		break;
	    }
	}
    }
    return (0);
}

char *sd0fname, *sd1fname, *sdfname;

/* This function compares ST measurements.  Since reference measurements are
   only available at the extremum of each episode, stdc finds the test
   measurement that is nearest in time to each reference measurement.
   Each pair of measurements is recorded in the output file. */

void stdc(mode)
int mode;	/* 0: signal 0 only, 1: signal 1 only, 2: both signals */
{
    char *ofname;
    WFDB_Annotation testann;
    static int pst0, pst1, stat, st0, st1, sttest;
    static long pttest;

    /* Return to the beginning of the annotation files. */
    if (iannsettime(0L) < 0) exit(2);

    switch (mode) {
      case 0: ofname = sd0fname; break;
      case 1: ofname = sd1fname; break;
      case 2: ofname = sdfname; break;
    }

    /* Open the output file. */
    if (strcmp(ofname, "-")) {
	if ((ofile = fopen(ofname, "r")) == NULL) {
	    if ((ofile = fopen(ofname, "w")) == NULL) {
		(void)fprintf(stderr, "%s: can't create %s\n",
			      pname, ofname);
		exit(3);
	    }
	    (void)fprintf(ofile, "(ST measurements)\n");
	    (void)fprintf(ofile, "Record     Time  Signal  Reference  Test\n");
	}
	else {
	    (void)fclose(ofile);
	    if ((ofile = fopen(ofname, "a")) == NULL) {
		(void)fprintf(stderr, "%s: can't modify %s\n",
			      pname, ofname);
		exit(3);
	    }
	}
    }
    else ofile = stdout;

    /* Check one reference ST measurement on each iteration. */
    while (find_reference_extremum(mode)) {		/* outer loop */
	/* Find the first test beat annotation after tref. */
	while ((stat = getann(1, &testann)) == 0) {	/* inner loop */
	    /* Ignore non-beat annotations. */
	    if (!isqrs(testann.anntyp)) continue;
	    pst0 = st0;
	    pst1 = st1;
	    /* Read test ST measurements (assumed to be the first two
	       numbers in the aux field of the test beat annotation).
	       If measurements are absent, assume they were unchanged
	       since the previous beat annotation (initially zero). */
	    if (testann.aux != NULL)
		(void)sscanf(testann.aux+1, "%d%d", &st0, &st1);
	    if (testann.time > tref)
		break;
	    pttest = testann.time; /* This statement was moved from the end of
				      the outer loop in version 10.5.2. */
	}
	if (stat != 0 || tref - pttest >= testann.time - tref)
	    sttest = sigref ? st1 : st0;   /* current annotation is closer */
	else
	    sttest = sigref ? pst1 : pst0; /* previous annotation was closer */
	(void)fprintf(ofile, "%6s %s %7d %10d %5d", record,
		      timstr(tref), sigref, stref, sttest);
	if (stref - sttest > 100 || sttest - stref > 100)
	    (void)fprintf(ofile, " *");
	(void)fprintf(ofile, "\n");
    }

    if (ofile != stdout)
	(void)fclose(ofile);
}

/* `pstat' prints a statistic described by s, defined as the quotient of a and
   b expressed in percentage units.  Undefined values are indicated by `-'. */

void pstat(s, a, b)
char *s;
long a, b;
{
    if (!lflag) {
	(void)fprintf(ofile, "%s:", s);
	if (b <= 0) (void)fprintf(ofile, "   - ");
	else (void)fprintf(ofile, " %3d%%", (int)((100.*a)/b + 0.5));
	(void)fprintf(ofile, " (%ld/%ld)\n", a, b);
    }
    else if (b <= 0) (void)fprintf(ofile, "   -");
    else (void)fprintf(ofile, " %3d", (int)((100.*a)/b + 0.5));
}


/* `tstat' prints a statistic as above, but prints the numerator and
   denominator as times. */

void tstat(s, a, b)
char *s;
long a, b;
{
    if (!lflag) {
	(void)fprintf(ofile, "%s:", s);
	if (b <= 0) (void)fprintf(ofile, "   - ");
	else (void)fprintf(ofile, " %3d%%", (int)((100.*a)/b + 0.5));
	(void)fprintf(ofile, " (%s", lzmstimstr(a));
	(void)fprintf(ofile, "/%s)\n", lzmstimstr(b));
    }
    else if (b <= 0) (void)fprintf(ofile, "   -");
    else (void)fprintf(ofile, " %3d", (int)((100.*a)/b + 0.5));
}

char *lzmstimstr(t)
long t;
{
    char *p = zmstimstr(t);

    while (*p == ' ')
	p++;
    return (p);
}

char *zmstimstr(t)
long t;
{
    return (t ? mstimstr(t) : "       0.000");
}

void print_results(type)
int type;
{
    char *ofname;

    switch (type) {
      case AFE:	ofname = afname; break;
      case VFE:	ofname = vfname; break;
      case ST0E:ofname = s0fname; break;
      case ST1E:ofname = s1fname; break;
      case STE:	ofname = sfname; break;
    }

    /* Open output file.  If line-format output was selected, write column
       headings only if the file must be created from scratch. */
    if (strcmp(ofname, "-")) {
	if ((ofile = fopen(ofname, "r")) == NULL) {
	    if ((ofile = fopen(ofname, "w")) == NULL) {
		(void)fprintf(stderr, "%s: can't create %s\n",
			      pname, ofname);
		exit(3);
	    }
	    if (lflag) {
		switch (type) {
		  case AFE:
		    (void)fprintf(ofile, "(AF detection)\n");
		    break;
		  case VFE:
		    (void)fprintf(ofile, "(VF detection)\n");
		    break;
		  case ST0E:
		    (void)fprintf(ofile,"(Ischemic ST detection, signal 0)\n");
		    break;
		  case ST1E:
		    (void)fprintf(ofile,"(Ischemic ST detection, signal 1)\n");
		    break;
		  case STE:
		    (void)fprintf(ofile,
				  "(Ischemic ST detection, both signals)\n");
		    break;
		}
		(void)fprintf(ofile, "Record  TPs   FN  TPp   FP");
		(void)fprintf(ofile,
			  "  ESe E+P DSe D+P  Ref duration  Test duration \n");
	    }
	}
	else {
	    (void)fclose(ofile);
	    if ((ofile = fopen(ofname, "a")) == NULL) {
		(void)fprintf(stderr, "%s: can't modify %s\n",
			      pname, ofname);
		exit(3);
	    }
	}
    }
    else ofile = stdout;

    if (lflag)
	(void)fprintf(ofile, "%6s %4ld %4ld %4ld %4ld ",
		      record, STP, FN, PTP, FP);
    else {
	switch (type) {
	  case AFE:
	    (void)fprintf(ofile,
		    "AF episode-by-episode comparison results for record %s\n",
		    record);
	    break;
	  case VFE:
	    (void)fprintf(ofile,
		    "VF episode-by-episode comparison results for record %s\n",
		    record);
	    break;
	  case ST0E:
	    (void)fprintf(ofile,
          "ST episode-by-episode comparison results for record %s, signal 0\n",
		    record);
	    break;
	  case ST1E:
	    (void)fprintf(ofile,
          "ST episode-by-episode comparison results for record %s, signal 1\n",
		    record);
	    break;
	  case STE:
	    (void)fprintf(ofile,
      "ST episode-by-episode comparison results for record %s, both signals\n",
		    record);
	    break;
	  default:
	    break;
	}
	(void)fprintf(ofile, "Reference annotator: %s\n", an[0].name);
	(void)fprintf(ofile, "     Test annotator: %s\n\n", an[1].name);
    }
    pstat("           Episode sensitivity", STP, STP + FN);
    pstat(" Episode positive predictivity", PTP, PTP + FP);
    tstat("          Duration sensitivity", ref_overlap, ref_duration);
    tstat("Duration positive predictivity", test_overlap, test_duration);
    if (!lflag)
	(void)fprintf(ofile, "Duration of reference episodes:");
    (void)fprintf(ofile, "  %s", zmstimstr(ref_duration));
    if (!lflag)
	(void)fprintf(ofile, "\n     Duration of test episodes:");
    else
	(void)fprintf(ofile, " ");
    (void)fprintf(ofile, "  %s\n", zmstimstr(test_duration));

    if (ofile != stdout)
	(void)fclose(ofile);
}

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
	  case 'A':	/* perform AF comparison */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -A\n",
			pname);
		exit(1);
	    }
	    afname = argv[i];
	    aflag = 1;
	    break;
	  case 'f':	/* start time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: start time must follow -f\n", pname);
		exit(1);
	    }
	    start_time = i;	/* save arg index, convert to samples later,
				   when record has been opened and sampling
				   frequency is known */
	    break;
	  case 'h':	/* print usage summary */
	    help();
	    exit(1);
	    break;
	  case 'I':	/* ignore short intervals when calculating overlap */
	      Iflag = 1;	/* fall through to -i */
	  case 'i':	/* minimum episode length follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: minimum episode length must follow %s\n", pname,
			      argv[i-1]);
		exit(1);
	    }
	    min_length = i;
	    break;
	  case 'l':	/* line-format output */
	  case 'L':		/* treat -L and -l the same way, for
				   consistency with `bxb' and `rxr' usage */
	    lflag = 1;
	    break;
	  case 'r':	/* record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 'S':	/* perform ST comparison */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: two output file names must follow %s\n",
			      pname, argv[i-1]);
		exit(1);
	    }
	    switch (argv[i-1][2]) {
	      case '0':
		s0fname = argv[i++];
		sd0fname = argv[i];
		s0flag = 1;
		break;
	      case '1':
		s1fname = argv[i++];
		sd1fname = argv[i];
		s1flag = 1;
		break;
	      case '\0':
		sfname = argv[i++];
		sdfname = argv[i];
		sflag = 1;
		break;
	      default:
		(void)fprintf(stderr,
			  "%s: unrecognized option %s\n", pname, argv[i-1]);
		exit(1);
	    }
	    break;
	  case 't':	/* end time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end_time = i;
	    break;
	  case 'V':	/* perform VF comparison */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -V\n",
			pname);
		exit(1);
	    }
	    vfname = argv[i];
	    vflag = 1;
	    break;
	  case 'x':	/* exclude AFL from AFIB comparison, as in EC38:1998) */
	    xflag = 1;
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

    if (start_time != 0L || end_time != 0L || min_length != 0)
	(void)fprintf(stderr,"%s: (warning) nonstandard comparison selected\n",
		pname);

    if (sampfreq(record) <= 0) {
	(void)fprintf(stderr,
		      "%s: (warning) %g Hz sampling frequency assumed\n",
		pname, WFDB_DEFFREQ);
	(void)setsampfreq(WFDB_DEFFREQ);
    }

    /* Set the minimum episode length and the times of the start and end of
       the test period. */
    if (min_length)
	min_length = strtim(argv[min_length]);
    if (start_time) {
	start_time = strtim(argv[(int)start_time]);
	if (start_time < (WFDB_Time)0) start_time = -start_time;
    }
    else
	start_time = strtim("5:0");			/* 5 minutes */
    if (end_time) {
	end_time = strtim(argv[(int)end_time]);
	if (end_time < (WFDB_Time)0) end_time = -end_time;
    }
    else if ((end_time = strtim("e")) == 0L)
	end_time = -1L;		/* record length unavailable -- go to end of
				   reference annotation file */
    if (end_time > 0L && end_time < start_time) {
	(void)fprintf(stderr, "%s: improper interval specified\n", pname);
	exit(1);
    }
    an[0].stat = an[1].stat = WFDB_READ;

    /* Open annotation files. */
    if (annopen(record, an, 2) < 0) exit(2);
}

static char *help_strings[] = {
 "usage: %s -r RECORD -a REF TEST [OPTIONS ...]\n",
 "where RECORD is the record name;  REF is reference annotator name;  TEST is",
 "the test annotator name; and OPTIONS may include any of:",
 " -A FILE        append AF reports to FILE",
 " -f TIME        begin comparison at specified TIME (default: 5 minutes",
 "                 after beginning of record)",
 " -h             print this usage summary",
 " -i TIME        exclude episodes shorter than TIME from episode statistics",
 " -I TIME	  exclude episodes shorter than TIME from all statistics",
 " -l             write reports in line format (default: matrix format)",
 " -L             same as -l",
 " -S FILE FILE2  append ST reports for both signals to FILE, and ST",
 "                 measurements to FILE2",
 " -S0 FILE FILE2 append ST reports for signal 0 to FILE, and ST measurements",
 "                 to FILE2",
 " -S1 FILE FILE2 append ST reports for signal 1 to FILE, and ST measurements",
 "                 to FILE2",
 " -t TIME        stop comparison at specified TIME (default: end of record",
 "                 if defined, end of reference annotation file otherwise;",
 "                 if TIME is 0, the comparison ends when the end of either",
 "                 annotation file is reached)",
 " -V FILE        append VF reports to FILE",
 " -x		  exclude reference AFL from AFIB +P comparison (EC38:1998)",
 "                 default: no exclusions (EC38:2007, EC57)",
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
