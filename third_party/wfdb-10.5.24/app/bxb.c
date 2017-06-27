/* file: bxb.c		G. Moody	14 December 1987
			Last revised:	 10 August 2010

-------------------------------------------------------------------------------
bxb: ANSI/AAMI-standard beat-by-beat annotation file comparator
Copyright (C) 2010 George B. Moody

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

This program implements the beat-by-beat comparison algorithms described in
AAMI/ANSI EC38:1998, the American National Standard for ambulatory ECGs, and
in AAMI EC57:1998, the American National Standard for Testing and Reporting
Performance Results of Cardiac Rhythm and ST Segment Measurement Algorithms.
These standards are available from AAMI, 1110 N Glebe Road, Suite 220,
Arlington, VA 22201 USA (http://www.aami.org/).

The -f, -O, -t, and -w options modify the comparison algorithm used by bxb in
ways not permitted by EC38:1998 or EC57:1998.  These options are provided for
the use of developers, who may find them useful for obtaining a more detailed
understanding of algorithm errors.
*/

#include <stdio.h>
#include <math.h>	/* for declaration of sqrt() */
#include <wfdb/wfdb.h>
#define map1
#define map2
#define ammap
#define mamap
#define annpos
#include <wfdb/ecgmap.h>

#define abs(A)	((A) >= 0 ? (A) : -(A))

char *pname;		/* name by which this program was invoked */
int A, Aprime;		/* types of the current & next reference annotations */
int a, aprime;		/* types of the current & next test annotations */
int fflag = 3;		/* report format (0: none; 1: compressed; 2: line;
			   3: standard; 4: compressed with SVEB;  5: line
			   with SVEB;  6: standard with SVEB) */
int match_dt = 0;	/* match window duration in samples */
int Oflag = 0;		/* if non-zero, produce an extended annotation file */
long shut_down;		/* duration of test annotator's shutdown */
long start;		/* time of the beginning of the test period */
long end_time;		/* end of the test period (-1: end of reference annot
			   file; 0: end of either annot file) */
long huge_time = 0x7FFFFFFF;		/* largest possible time */
long T, Tprime;		/* times of the current & next reference annotations */
long t, tprime;		/* times of the current & next test annotations */

main(argc, argv)
int argc;
char *argv[];
{
    void genxcmp(), getref(), gettest(), init(), pair(), print_results();

    /* Read and interpret command-line arguments. */
    init(argc, argv);

    /* Set A and T to the type and time of the first reference annotation after
       the end of the learning period. */
    do {
	getref();
    } while (T < start);

    /* Set aprime and tprime to the type and time of the first test annotation
       after the end of the learning period, and a and t to the type and time
       of the last test annotation in the learning period. */
    do {
	gettest();
    } while (tprime < start);

    /* If an extended output annotation file was requested, produce it and
       exit. */
    if (Oflag) {
	genxcmp();
	wfdbquit();
	exit(0);
    }

    /* If t matches the first reference annotation, count it and get the next
       annotation from each file.  (Since T >= start and t < start, T-t must be
       positive.) */
    if (T-t < abs(T-tprime) && T-t <= match_dt) {
	if (A != 0 || a != 0)	/* false only if start = 0 */
	    pair(A, a);
	getref();
	gettest();
    }

    /* If there is a test annotation within an interval equal to the match 
       window following the beginning of the test period, and there is no
       match, go on to the next test annotation without counting the first
       one. */
    else {
	gettest();
	if (t-start <= match_dt && abs(T-tprime) < abs(T-t))
	    gettest();
    }

    /* Peform the comparison.  Each time through the loop below, a beat label
       pair is identified and counted (or else a non-beat annotation is
       discarded), and an annotation is read from each file from which an
       annotation was paired or discarded.  Note that only one of the four
       numbered actions is performed on each iteration.

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

    24 November 2002:  The comparison algorithm has been very slightly modified
    by the addition of an alternative criterion for accepting a match in cases
    1 and 3 below.  The original match criterion for case 1 was:
            if (T-t <= match_dt && T-t < abs(T-tprime)) ...
    and for case 3, the original match criterion was:
            if (t-T <= match_dt && t-T < abs(t-Tprime)) ...
    The alternative criteria were added to account for rare cases in which
    (for case 1) both t and tprime are in the match window, or (for case 3)
    both T and Tprime are in the match window.

    Consider case 3.  The original algorithm paired the current test and
    reference annotations only if their times matched more closely than
    those of the current test and next reference annotations.  The modified
    algorithm also checks how well the next test annotation matches the next
    reference annotation;  if this is a better match than that between the
    current test and next reference annotations, then the current annotations
    are paired.  An example may make this clear;  assume match_dt = 37 and
    the following annotation times:
         ref1 = 96	test1 = 128
	 ref2 = 160	test2 = 185
    Since test1-ref1 (32) is not less than ref2-test1 (also 32), the original
    algorithm would not match ref1 and test1, and would count ref1 as missed.
    Since ref2-test1 (32) is not less than test2-ref2 (15), the original
    algorithm would also fail to match test1 and ref2, and would count test1
    as an extra detection.  The modified algorithm finds that test2-ref2 (15)
    is less than ref2-test1 (32), and therefore excludes ref2 as a possible
    match for test1;  having done so, it then matches ref1 and test1.  Similar
    reasoning applies in case 1, in which the roles of ref and test are
    reversed.

    Note that the only situation in which these modifications will have an
    effect is when consecutive reference annotations and consecutive test
    annotations occur at intervals less than or equal to twice the match
    window.  Generally, the match window is chosen so that this is unlikely;
    using the standard window, this can only happen if both the true heart
    rate and the detected heart rate exceed 200 bpm.

    Thanks to James Pardey for reporting the problem with the original
    algorithm and for providing a test case that was helpful for developing
    and testing these modifications.
    */
    while ((end_time > 0L && (T <= end_time || t <= end_time)) ||
	   (end_time == -1L && T != huge_time) ||
	   (end_time == 0L && T != huge_time && t != huge_time)) {
	if (t < T) {	/* test annotation is earliest */
	    /* (1) If t is within the match window, and is a better match than
	       the next test annotation, pair it. */
	    if (T-t <= match_dt && 
		(T-t < abs(T-tprime) || abs(Tprime-tprime) < abs(T-tprime))) {
		pair(A, a);
		getref();
		gettest();
	    }
	    /* (2) There is no match to the test annotation, so pair it with a
	       pseudo-beat annotation and get the next one. */
	    else {
		pair(rpann(t), a);
		gettest();
	    }
	}
	else {		/* reference annotation is earliest */
	    /* (3) If T is within the match window, and is a better match than
	       the next reference annotation, pair it. */
	    if (t-T <= match_dt &&
		(t-T < abs(t-Tprime) || abs(tprime-Tprime) < abs(t-Tprime))) {
		pair(A, a);
		gettest();
		getref();
	    }
	    /* (4) There is no match to the reference annotation, so pair it
	       with a pseudo-beat annotation and get the next one. */
	    else {
		pair(A, tpann(T));
		getref();
	    }
	}
    }

    shut_down /= strtim("1");	/* convert from samples to seconds */

    /* Generate output. */
    print_results(fflag);

    wfdbquit();			/* close input files */
    exit(0);	/*NOTREACHED*/
}

/* getref() and gettest() read the next beat annotations from their respective
   files. */

char *record;			/* record name */
WFDB_Anninfo an[3];
unsigned int oflag = 0;	/* if non-zero, produce an output annotation file */
long RR;		/* reference RR interval, if non-zero */
long sdonref = -1L;	/* start of reference shutdown */
long sdoffref = -1L;	/* end of reference shutdown */
long vfonref = -1L;	/* start of reference VF */
long vfoffref = -1L;	/* end of reference VF */
long psdonref = -1L;	/* start of previous reference shutdown */
long psdoffref = -1L;	/* end of previous reference shutdown */
long pvfonref = -1L;	/* start of previous reference VF */
long pvfoffref = -1L;	/* end of previous reference VF */
WFDB_Annotation ref_annot;

void getref()	/* get next reference beat annotation */
{
    static long TT;	/* time of previous reference beat annotation */
    static WFDB_Annotation annot;

    TT = T;
    T = Tprime;
    A = Aprime;

    /* T-TT is not a valid RR interval if T is the time of the first beat,
       if TT is the time of the last beat, or if a period of VF or shutdown
       occurs between TT and T. */
    if (TT == 0L || T == huge_time ||
	(TT <= vfonref && vfonref < T) ||
	(TT <= sdonref && sdonref < T) ||
	(TT <= pvfonref && pvfonref < T) ||
	(TT <= psdonref && psdonref < T))
	RR = 0L;
    else
	RR = T - TT;

    if (oflag) ref_annot = annot;

    /* Read reference annotations until a beat annotation is read, or EOF.
       If an expanded output annotation file is required, all annotations
       are treated as if they were beat annotations. */
    while (getann(0, &annot) == 0) {
	if (isqrs(annot.anntyp) || Oflag) {	/* beat annotation */
	    Tprime = annot.time;
	    Aprime = amap(annot.anntyp);
	    return;
	}
	
	/* Shutdown occurs when neither signal is readable;  the beginning of
	   shutdown is indicated by a NOISE annotation in which bits 4 and 5
	   of the subtyp field are set, and the end of shutdown is indicated
	   by a NOISE annotation with any value of `subtyp' for which at least
	   one of bits 4 and 5 is zero.  In AHA DB reference annotation files,
	   shutdown is indicated by a single shutdown annotation placed roughly
	   in the middle of the shutdown interval;  in this case, shutdown is
	   assumed to begin match_dt samples after the previous beat annotation
	   or VFOFF annotation, and is assumed to end match_dt samples before
	   the next annotation.
	*/
	else if (annot.anntyp == NOISE) {
	    if ((annot.subtyp & 0x30) == 0x30) {
		psdonref = sdonref;
		psdoffref = sdoffref;
		sdonref = annot.time;
		/* Read next annotation, which should mark end of shutdown. */
		if (getann(0, &annot) < 0) {  /* EOF before end of shutdown */
		    Tprime = sdoffref = huge_time;
		    Aprime = '*';
		    return;
		}
		if (annot.anntyp == NOISE &&
		    (annot.subtyp & 0x30) != 0x30)
		    sdoffref = annot.time;
		else {
		    if (vfoffref > T) sdonref = vfoffref + match_dt;
		    else sdonref = T + match_dt;
		    sdoffref = annot.time - match_dt;
		    if (sdonref > sdoffref) sdonref = sdoffref;
		    (void)ungetann(0, &annot);
		}
	    }
	}

	/* The beginning of ventricular fibrillation is indicated by a VFON
	   annotation, and its end by a VFOFF annotation;  any annotations
	   between VFON and VFOFF are read and ignored. */
	else if (annot.anntyp == VFON) {
	    pvfonref = vfonref;
	    pvfoffref = vfoffref;
	    vfonref = annot.time;
	    /* Read additional annotations, until end of VF or EOF. */
	    do {
		if (getann(0, &annot) < 0) {  /* EOF before end of VF */
		    Tprime = huge_time;
		    Aprime = '*';
		    return;
		}
	    } while (annot.anntyp != VFOFF);
	    vfoffref = annot.time;
	}
    }
    /* When this statement is reached, there are no more annotations in the
       reference annotation file. */
    Tprime = huge_time;
    Aprime = '*';
}

long rr;		/* test RR interval, if non-zero */
long sdontest = -1L;	/* start of test shutdown */
long sdofftest = -1L;	/* end of test shutdown */
long vfontest = -1L;	/* start of test VF */
long vfofftest = -1L;	/* end of test VF */
long psdontest = -1L;	/* start of previous test shutdown */
long psdofftest = -1L;	/* end of previous test shutdown */
long pvfontest = -1L;	/* start of previous test VF */
long pvfofftest = -1L;	/* end of previous test VF */
WFDB_Annotation test_annot;

void gettest()	/* get next test annotation */
{
    static long tt;	/* time of previous test beat annotation */
    static WFDB_Annotation annot;

    tt = t;
    t = tprime;
    a = aprime;

    /* See comments on the similar code in getref(), above. */
    if (tt == 0L || t == huge_time ||
	(tt <= vfontest && vfontest < t) ||
	(tt <= sdontest && sdontest < t) ||
	(tt <= pvfontest && pvfontest < t) ||
	(tt <= psdontest && psdontest < t))
	rr = 0L;
    else
	rr = t - tt;

    if (oflag) test_annot = annot;

    while (getann(1, &annot) == 0) {
	if (isqrs(annot.anntyp) || Oflag) {
	    tprime = annot.time;
	    aprime = amap(annot.anntyp);
	    return;
	}
	if (annot.anntyp == NOISE) {
	    if ((annot.subtyp & 0x30) == 0x30) {
		psdontest = sdontest;
		psdofftest = sdofftest;
		sdontest = annot.time;
		if (getann(1, &annot) < 0) {
		    tprime = huge_time;
		    aprime = '*';
		    if (end_time > 0L)
			shut_down += end_time - sdontest;
		    else {
			(void)fprintf(stderr,
      "%s: unterminated shutdown starting at %s in record %s, annotator %s\n",
			      pname, timstr(sdontest), record, an[1].name);
			(void)fprintf(stderr,
			 " (not included in shutdown duration measurement)\n");
		    }
		    return;
		}
		if (annot.anntyp == NOISE &&
		    (annot.subtyp & 0x30) != 0x30)
		    sdofftest = annot.time;
		else {
		    if (vfofftest > t) sdontest = vfofftest + match_dt;
		    else sdontest = t + match_dt;
		    sdofftest = annot.time - match_dt;
		    if (sdontest > sdofftest) sdontest = sdofftest;
		    (void)ungetann(1, &annot);
		}
		/* update shutdown duration tally */
		shut_down += sdofftest - sdontest;
	    }
	}
	else if (annot.anntyp == VFON) {
	    pvfontest = vfontest;
	    pvfofftest = vfofftest;
	    vfontest = annot.time;
	    do {
		if (getann(1, &annot) < 0) {
		    tprime = huge_time;
		    aprime = '*';
		    return;
		}
	    } while (annot.anntyp != VFOFF);
	    vfofftest = annot.time;
	}
    }
    tprime = huge_time;
    aprime = '*';
}

/* Functions rpann() and tpann() return the appropriate pseudo-beat label
   for the time specified by their argument.  They should be called only
   with time arguments which match the times of the current test or reference
   beat labels, since they depend on getref() and gettest() to locate the two
   most recent VF and shutdown periods and have no information about earlier
   or later VF or shutdown periods. */
int rpann(t)
long t;
{
    if ((vfonref!=-1L && vfonref<=t && (t<=vfoffref || vfoffref==-1L)) ||
	(pvfonref!=-1L && pvfonref<=t && t<=pvfoffref))
	return ('*');	/* test beat labels during reference-marked VF are
			   not to be counted;  since `*' is not recognized by
			   pair(), returning `*' accomplishes this */
    else if ((sdonref!=-1L && sdonref<=t && (t<=sdoffref || sdoffref== -1L)) ||
	(psdonref!=-1L && psdonref<=t && t<=psdoffref))
	return ('X');	/* test beat labels during reference-marked shutdown
			   are paired with X pseudo-beat labels */
    else
	return ('O');	/* all other extra test beat labels are paired with
			   O pseudo-beat labels */
}

int tpann(t)
long t;
{
    /* no special treatment for reference beat labels during test-marked VF */
    if ((sdontest!=-1L && sdontest<=t && (t<=sdofftest || sdofftest==-1L)) ||
	(psdontest!=-1L && psdontest<= t && t <= psdofftest))
	return ('X');	/* reference beat labels during test-marked shutdown
			   are paired with X pseudo-beat labels */
    else
	return ('O');	/* all other extra reference beat labels are paired
			   with O pseudo-beat labels */
}

/* Define counters for the elements of the confusion matrix.  Static variables
   have initial values of zero.  */
static long Nn, Ns, Nv, Nf, Nq, No, Nx,
	    Sn, Ss, Sv, Sf, Sq, So, Sx,
	    Vn, Vs, Vv, Vf, Vq, Vo, Vx,
	    Fn, Fs, Fv, Ff, Fq, Fo, Fx,
	    Qn, Qs, Qv, Qf, Qq, Qo, Qx,
	    On, Os, Ov, Of, Oq,
	    Xn, Xs, Xv, Xf, Xq;

int verbose = 0;	/* if non-zero, describe all mismatches */
long nrre = 0;		/* number of RR errors tallied in ssrre */
double ssrre = 0.;	/* sum of squares of RR errors */

void pair(ref, test)	/* count a beat label pair */
int ref, test;			/* reference and test annotation types */
{
    switch (ref) {
	case 'N': switch (test) {
		case 'N': Nn++; break;
		case 'S': Ns++; break;
		case 'V': Nv++; break;
		case 'F': Nf++; break;
		case 'Q': Nq++; break;
		case 'O': No++; break;
		case 'X': Nx++; break;
	    } break;
	case 'S': switch (test) {
		case 'N': Sn++; break;
		case 'S': Ss++; break;
		case 'V': Sv++; break;
		case 'F': Sf++; break;
		case 'Q': Sq++; break;
		case 'O': So++; break;
		case 'X': Sx++; break;
	    } break;
	case 'V': switch (test) {
		case 'N': Vn++; break;
		case 'S': Vs++; break;
		case 'V': Vv++; break;
		case 'F': Vf++; break;
		case 'Q': Vq++; break;
		case 'O': Vo++; break;
		case 'X': Vx++; break;
	    } break;
	case 'F': switch (test) {
		case 'N': Fn++; break;
		case 'S': Fs++; break;
		case 'V': Fv++; break;
		case 'F': Ff++; break;
		case 'Q': Fq++; break;
		case 'O': Fo++; break;
		case 'X': Fx++; break;
	    } break;
	case 'Q': switch (test) {
		case 'N': Qn++; break;
		case 'S': Qs++; break;
		case 'V': Qv++; break;
		case 'F': Qf++; break;
		case 'Q': Qq++; break;
		case 'O': Qo++; break;
		case 'X': Qx++; break;
	    } break;
	case 'O': switch (test) {
		case 'N': On++; break;
		case 'S': Os++; break;
		case 'V': Ov++; break;
		case 'F': Of++; break;
		case 'Q': Oq++; break;
	    } break;
	case 'X': switch (test) {
		case 'N': Xn++; break;
		case 'S': Xs++; break;
		case 'V': Xv++; break;
		case 'F': Xf++; break;
		case 'Q': Xq++; break;
	    } break;
    }

    /* Compute the RR interval error and update the sum of squared errors. */
    if (RR > 0L && rr > 0L) {
	double rre = RR - rr;

	ssrre += rre*rre;
	nrre++;
    }

    if (oflag) {
	if (ref == test) (void)putann(0, &test_annot);
	else {
	    WFDB_Annotation out_annot;
	    char auxp[3];

	    auxp[0] = 2; auxp[1] = ref; auxp[2] = test - 'A' + 'a';
	    if (test == 'O' || test == 'X')
		out_annot.time = T;
	    else
		out_annot.time = t;
	    out_annot.anntyp = NOTE;
	    out_annot.subtyp = out_annot.chan = out_annot.num = 0;
	    out_annot.aux = auxp;
	    (void)putann(0, &out_annot);
	}
    }
    if (verbose && ref != test) {
	if (ref == 'O' || ref == 'X')
	    (void)fprintf(stderr, "%c(%ld)/%c(%ld)\n", ref, t, test, t);
	else if (test == 'O' || test == 'X')
	    (void)fprintf(stderr, "%c(%ld)/%c(%ld)\n", ref, T, test, T);
	else
	    (void)fprintf(stderr, "%c(%ld)/%c(%ld)\n", ref, T, test, t);
    }
}

int amap(a)		/* map MIT annotation code into AAMI test label */
int a;
{
    switch (a) {
	case NORMAL:
	case LBBB:
	case RBBB:
	case BBB:	return ('N');
	case NPC:
	case APC:
	case SVPB:
	case ABERR:
	case NESC:
	case AESC:
	case SVESC:	return (fflag > 3 ? 'S' : 'N');
	case PVC:
	case RONT:
	case VESC:	return ('V');
	case FUSION:	return ('F');
	case UNKNOWN:	return ('Q');

/* The AAMI RP excludes records containing paced beats from its reporting
   requirements.  To permit this program to be used with such records,
   beats which are either paced (type PACE) or fusions of paced and normal
   beats (type PFUS) are treated in the same way as unknown beats. */
	case PACE:
	case PFUS:	return ('Q');

/* LEARN annotations should appear only in the `test' annotation file, and
   only during the learning period;  if they appear elsewhere, they are
   treated in the same way as unknown beats. */
	case LEARN:	return ('Q');

/* Other annotations (including NOISE and VFON/VFOFF) are treated as non-beat
   annotations. */
	default:	return ('O');
    }
}

FILE *ofile, *sfile;	/* files for beat-by-beat and shutdown reports */

/* `pstat' prints a statistic described by s, defined as the quotient of a and
   b expressed in percentage units.  Undefined values are indicated by `-'. */

void pstat(s, f, a, b)
char *s, *f;
long a, b;
{
    if (fflag == 1 || fflag == 3 || fflag == 4 || fflag == 6) {
	(void)fprintf(ofile, "%s: ", s);
	if (b <= 0) (void)fprintf(ofile, "     - ");
	else {
	    (void)fprintf(ofile, f, (100.*a)/b);
	    (void)fprintf(ofile, "%%");
	}
	(void)fprintf(ofile, " (%ld/%ld)\n", a, b);
    }
    else if (b <= 0) (void)fprintf(ofile, "      -");
    else { (void)fprintf(ofile, " "); (void)fprintf(ofile, f, (100.*a)/b); }
}

/* `sstat' prints a statistic as for `pstat', but the output goes to sfile. */

void sstat(s, f, a, b)
char *s, *f;
long a, b;
{
    if (fflag == 1 || fflag == 3 || fflag == 4 || fflag == 6) {
	(void)fprintf(sfile, "%s: ", s);
	if (b <= 0) (void)fprintf(sfile, "     - ");
	else {
	    (void)fprintf(sfile, f, (100.*a)/b);
	    (void)fprintf(sfile, "%%");
	}
	(void)fprintf(sfile, " (%ld/%ld)\n", a, b);
    }
    else if (b <= 0) (void)fprintf(sfile, "      -");
    else { (void)fprintf(sfile, " "); (void)fprintf(sfile, f, (100.*a)/b); }
}

char *ofname = "-", *sfname;	/* filenames for reports */

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
	  case 'c':	/* condensed output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -c\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 1;
	    break;
	  case 'C':	/* condensed output with SVEB statistics */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -C\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 4;
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
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: two output file names must follow -l\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    sfname = argv[++i];
	    fflag = 2;
	    break;
	  case 'L':	/* line-format output, with SVEB statistics */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: two output file names must follow -L\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    sfname = argv[++i];
	    fflag = 5;
	    break;
	  case 'o':	/* generate output annotation file */
	    oflag = 1;
	    break;
	  case 'O':	/* generate expanded output annotation file */
	    oflag = 1;
	    Oflag = 1;
	    fflag = 0;
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
	    ofname = argv[i];
	    fflag = 3;
	    break;
	  case 'S':	/* standard-format output, with SVEB statistics */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -S\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 6;
	    break;
	  case 't':	/* end time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end_time = i;
	    break;
	  case 'v':	/* verbose mode */
	    verbose = 1;
	    break;
	  case 'w':	/* match window follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: match window must follow -w\n", pname);
		exit(1);
	    }
	    match_dt = i;
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

    if (start != 0L || end_time != 0L || match_dt != 0)
	(void)fprintf(stderr,"%s: (warning) nonstandard comparison selected\n",
		pname);

    if (sampfreq(record) <= 0) {
	(void)fprintf(stderr,
		      "%s: (warning) %g Hz sampling frequency assumed\n",
		pname, WFDB_DEFFREQ);
	(void)setsampfreq(WFDB_DEFFREQ);
    }

    /* Set the match window and the times of the start and end of the test
       period.  Initialize the shutdown tally to 1/2 second so that it will be
       properly rounded to the nearest second at the end. */
    if (match_dt)
	match_dt = (int)strtim(argv[match_dt]);
    else
	match_dt = (int)strtim(".15");		/* 150 milliseconds */
    if (start) {
	start = strtim(argv[(int)start]);
	/* If the header file defines a base time (absolute time of day),
	   the start and end times can be supplied in the form '[hh:mm:ss]',
	   and strtim returns a negative value (to signal that the user
	   specified the time in this way).  In this case, the magnitude of
	   the returned value is the elapsed time in sample intervals.  We
	   don't care how the user entered the time here, so we throw away
	   the sign information and keep the elapsed time. */
	if (start < (WFDB_Time)0)
	    start = -start;
    }
    else
	start = strtim("5:0");			/* 5 minutes */
    if (end_time) {
	end_time = strtim(argv[(int)end_time]);
	/* See the comments about strtim in the previous block (above). */
	if (end_time < (WFDB_Time)0) end_time = -end_time;
    }
    else if ((end_time = strtim("e")) == 0L)
	end_time = -1L;		/* record length unavailable -- go to end of
				   reference annotation file */
    if (end_time > 0L && end_time < start) {
	(void)fprintf(stderr, "%s: improper interval specified\n", pname);
	exit(1);
    }
    shut_down = strtim(".5");	/* 1/2 second */

    an[0].stat = an[1].stat = WFDB_READ;
    if (oflag) {
	an[2].name = "bxb";
	an[2].stat = WFDB_WRITE;
    }
    if (annopen(record, an, 2 + oflag) < 0) exit(2);
}

void print_results(fflag)
int fflag;
{
    long QTP, QFN, QFP, STP, SFN, SFP, VTP, VFN, VTN, VFP;

    /* Open output files.  If line-format output was selected, write column
       headings only if the files must be created from scratch. */
    if (strcmp(ofname, "-")) {
	if ((ofile = fopen(ofname, "r")) == NULL) {
	    if ((ofile = fopen(ofname, "w")) == NULL) {
		(void)fprintf(stderr, "%s: can't create %s\n", pname, ofname);
		exit(3);
	    }
	    if (fflag == 2) {
		(void)fprintf(ofile,
			      "Record Nn' Vn' Fn' On'  Nv   Vv  Fv' Ov' No'");
		(void)fprintf(ofile,
			      " Vo' Fo'  Q Se   Q +P   V Se   V +P  V FPR\n");
	    }
	    else if (fflag == 5) {
		(void)fprintf(ofile,
			      "Record Nn' Sn' Vn' Fn' On'  Ns  Ss  Vs  Fs'");
		(void)fprintf(ofile,
			      " Os' Nv  Sv   Vv  Fv' Ov' No' So' Vo' Fo'");
		(void)fprintf(ofile,
		      "  Q Se   Q +P   V Se   V +P   S Se   S +P RR err\n");
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
    if (fflag == 2 || fflag == 5) {
	if (strcmp(sfname, "-")) {
	    if ((sfile = fopen(sfname, "r")) == NULL) {
		if ((sfile = fopen(sfname, "w")) == NULL) {
		    (void)fprintf(stderr,
				  "%s: can't create %s\n", pname, sfname);
		    exit(3);
		}
		if (fflag == 2) {
		    (void)fprintf(sfile,
			    "Record Nx   Vx   Fx   Qx  %% beats  %% N    ");
		    (void)fprintf(sfile, "%% V    %% F   Total Shutdown\n");
		    (void)fprintf(sfile,
			    "                           missed missed ");
		    (void)fprintf(sfile, "missed missed      Time\n");
		}
		else {
		    (void)fprintf(sfile,
			  "Record Nx   Sx   Vx   Fx   Qx  %% beats  %% N    ");
		    (void)fprintf(sfile,
				  "%% S    %% V    %% F   Total Shutdown\n");
		    (void)fprintf(sfile,
			    "                                missed missed ");
		    (void)fprintf(sfile, "missed missed missed      Time\n");
		}
	    }
	    else {
		(void)fclose(sfile);
		if ((sfile = fopen(sfname, "a")) == NULL) {
		    (void)fprintf(stderr,
				  "%s: can't modify %s\n", pname, sfname);
		    exit(3);
		}
	    }
	}
	else sfile = stdout;
    }
    else sfile = stdout;

    if (fflag == 1 || fflag == 3 || fflag == 4 || fflag == 6) {
	(void)fprintf(ofile, "Beat-by-beat comparison results for record %s\n",
		      record);
	(void)fprintf(ofile, "Reference annotator: %s\n", an[0].name);
	(void)fprintf(ofile, "     Test annotator: %s\n\n", an[1].name);
    }

    switch (fflag) {
      case 1:	/* print condensed format summary tables */
	(void)fprintf(ofile, "         Algorithm\n");
	(void)fprintf(ofile, "      n+f+q    v  o+x\n");
	(void)fprintf(ofile, "     ________________\n");
	(void)fprintf(ofile, "  N  | %4ld %4ld %4ld\n",
		      Nn+Ns+Nf+Nq + Sn+Ss+Sf+Sq, Nv + Sv, No+Nx + So+Sx);
	(void)fprintf(ofile, "  V  | %4ld %4ld %4ld\n",
		      Vn+Vs+Vf+Vq, Vv, Vo+Vx);
	(void)fprintf(ofile, " F+Q | %4ld %4ld %4ld\n",
		      Fn+Fs+Ff+Fq + Qn+Qs+Qf+Qq, Fv + Qv, Fo+Fx + Qo+Qx);
	(void)fprintf(ofile, " O+X | %4ld %4ld\n\n",
		      On+Os+Of+Oq + Xn+Xs+Xf+Xq, Ov+Xv);
	break;
      case 2:	/* print line-format output */
	(void)fprintf(ofile,
	      "%4s %5ld %3ld %3ld %3ld %3ld %4ld %3ld %3ld %3ld %3ld %3ld",
		      record,
		      Nn+Ns+Nf+Nq + Sn+Ss+Sf+Sq,
		      Vn+Vs+Vf+Vq,
		      Fn+Fs+Ff+Fq + Qn+Qs+Qf+Qq,
		      On+Os+Of+Oq + Xn+Xs+Xf+Xq,
		      Nv+Sv, Vv, Fv+Qv, Ov+Xv,
		      No+Nx+So+Sx, Vo+Vx, Fo+Fx+Qo+Qx);
	(void)fprintf(sfile, "%4s %4ld %4ld %4ld %4ld  ",
		      record, Nx+Sx, Vx, Fx, Qx);
	break;
      case 3:	/* print standard format summary tables */
	(void)fprintf(ofile, "               Algorithm\n");
	(void)fprintf(ofile, "        n    v    f    q    o    x\n");
	(void)fprintf(ofile, "   _______________________________\n");
	(void)fprintf(ofile, " N | %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Nn+Ns+Sn+Ss, Nv+Sv, Nf+Sf, Nq+Sq, No+So, Nx+Sx);
	(void)fprintf(ofile, " V | %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Vn+Vs, Vv, Vf, Vq, Vo, Vx);
	(void)fprintf(ofile, " F | %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Fn+Fs, Fv, Ff, Fq, Fo, Fx);
	(void)fprintf(ofile, " Q | %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Qn+Qs, Qv, Qf, Qq, Qo, Qx);
	(void)fprintf(ofile, " O | %4ld %4ld %4ld %4ld\n",
		      On+Os, Ov, Of, Oq);
	(void)fprintf(ofile, " X | %4ld %4ld %4ld %4ld\n\n",
		      Xn+Xs, Xv, Xf, Xq);
	break;
      case 4:	/* print condensed format summary tables, with SVEBs */
	(void)fprintf(ofile, "         Algorithm\n");
	(void)fprintf(ofile, "      n+f+q    s    v  o+x\n");
	(void)fprintf(ofile, "     _____________________\n");
	(void)fprintf(ofile, "  N  | %4ld %4ld %4ld %4ld\n",
		      Nn+Nf+Nq, Ns, Nv, No+Nx);
	(void)fprintf(ofile, "  S  | %4ld %4ld %4ld %4ld\n",
		      Sn+Sf+Sq, Ss, Sv, So+Sx);
	(void)fprintf(ofile, "  V  | %4ld %4ld %4ld %4ld\n",
		      Vn+Vf+Vq, Vs, Vv, Vo+Vx);
	(void)fprintf(ofile, " F+Q | %4ld %4ld %4ld %4ld\n",
		      Fn+Ff+Fq+Qn+Qf+Qq, Fs+Qs, Fv+Qv, Fo+Fx+Qo+Qx);
	(void)fprintf(ofile, " O+X | %4ld %4ld %4ld\n\n",
		      On+Of+Oq+Xn+Xf+Xq, Os+Xs, Ov+Xv);
	break;
      case 5:	/* print line-format output, with SVEBs */
	(void)fprintf(ofile,
		      "%4s %5ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld",
		      record,
		      Nn+Nf+Nq,
		      Sn+Sf+Sq,
		      Vn+Vf+Vq,
		      Fn+Ff+Fq + Qn+Qf+Qq,
		      On+Of+Oq + Xn+Xf+Xq,
		      Ns, Ss, Vs, Fs+Qs, Os+Xs);
	(void)fprintf(ofile,
		      " %3ld %3ld %4ld %3ld %3ld %3ld %3ld %3ld %3ld",
		      Nv, Sv, Vv, Fv+Qv, Ov+Xv,
		      No+Nx, So+Sx, Vo+Vx, Fo+Fx+Qo+Qx);
	(void)fprintf(sfile,
		      "%4s %4ld %4ld %4ld %4ld %4ld  ",
		      record, Nx, Sx, Vx, Fx, Qx);
	break;
      case 6:	/* print standard format summary tables, with SVEBs */
      default:
	(void)fprintf(ofile, "               Algorithm\n");
	(void)fprintf(ofile, "        n    s    v    f    q    o    x\n");
	(void)fprintf(ofile, "   ____________________________________\n");
	(void)fprintf(ofile, " N | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Nn, Ns, Nv, Nf, Nq, No, Nx);
	(void)fprintf(ofile, " S | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Sn, Ss, Sv, Sf, Sq, So, Sx);
	(void)fprintf(ofile, " V | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Vn, Vs, Vv, Vf, Vq, Vo, Vx);
	(void)fprintf(ofile, " F | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Fn, Fs, Fv, Ff, Fq, Fo, Fx);
	(void)fprintf(ofile, " Q | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		      Qn, Qs, Qv, Qf, Qq, Qo, Qx);
	(void)fprintf(ofile, " O | %4ld %4ld %4ld %4ld %4ld\n",
		      On, Os, Ov, Of, Oq);
	(void)fprintf(ofile, " X | %4ld %4ld %4ld %4ld %4ld\n\n",
		      Xn, Xs, Xv, Xf, Xq);
	break;
    }
    QTP = Nn+Ns+Nv+Nf+Nq + Sn+Ss+Sv+Sf+Sq + Vn+Vs+Vv+Vf+Vq + Fn+Fs+Fv+Ff+Fq +
	Qn+Qs+Qv+Qf+Qq;
    QFN = No+Nx + So+Sx + Vo+Vx + Fo+Fx + Qo+Qx;
    QFP = On+Os+Ov+Of+Oq + Xn+Xs+Xv+Xf+Xq;
    VTP = Vv;
    VFN = Vn + Vs + Vf + Vq + Vo + Vx;
    VTN = Nn+Ns+Nf+Nq + Sn+Ss+Sf+Sq + Fn+Fs+Ff+Fq + Qn+Qs+Qf+Qq + On+Os+Of+Oq +
	Xn+Xs+Xf+Xq;
    VFP = Nv + Sv + Ov + Xv;
    STP = Ss;
    SFN = Sn + Sv + Sf + Sq + So + Sx;
    SFP = Ns + Vs + Fs + Os + Xs;
    pstat("           QRS sensitivity", "%6.2f", QTP, QTP + QFN);
    pstat(" QRS positive predictivity", "%6.2f", QTP, QTP + QFP);
    pstat("           VEB sensitivity", "%6.2f", VTP, VTP + VFN);
    pstat(" VEB positive predictivity", "%6.2f", VTP, VTP + VFP);
    if (fflag < 4)
	pstat("   VEB false positive rate", "%6.3f", VFP, VTN + VFP);
    else {
	pstat("          SVEB sensitivity", "%6.2f", STP, STP + SFN);
	pstat("SVEB positive predictivity", "%6.2f", STP, STP + SFP);
    }
    if (fflag == 4 || fflag == 6) {
	(void)fprintf(ofile, "     RMS RR interval error: ");
	if (nrre)
	    (void)fprintf(ofile, "%6.2f ms",
			  sqrt(ssrre/nrre)*1000./strtim("1"));
	else
	    (void)fprintf(ofile, "     -");
    }
    else if (fflag == 5) {
	if (nrre)
	    (void)fprintf(ofile, " %6.2f", sqrt(ssrre/nrre)*1000./strtim("1"));
	else
	    (void)fprintf(ofile, "     -");
    }
    (void)fprintf(ofile, "\n");
    sstat("\n  Beats missed in shutdown", "%6.2f", Nx+Vx+Fx+Qx, QTP + QFN);
    sstat("      N missed in shutdown", "%6.2f", Nx, Nn+Ns+Nv+Nf+Nq+No+Nx);
    if (fflag >= 4)
	sstat("      S missed in shutdown", "%6.2f", Sx, Sn+Ss+Sv+Sf+Sq+So+Sx);
    sstat("      V missed in shutdown", "%6.2f", Vx, Vn+Vs+Vv+Vf+Vq+Vo+Vx);
    sstat("      F missed in shutdown", "%6.2f", Fx, Fn+Fs+Fv+Ff+Fq+Fo+Fx);
    if (fflag == 1 || fflag == 3 || fflag == 4 || fflag == 6)
	(void)fprintf(sfile, "       Total shutdown time: ");
    if (fflag != 2 && fflag != 5)
        (void)fprintf(sfile, "%5ld seconds\n", shut_down);
    else 
        (void)fprintf(sfile, "%5ld seconds %ld %ld %ld %ld %ld\n", shut_down,
		      Nn+Ns+Nv+Nf+Nq+No+Nx, Sn+Ss+Sv+Sf+Sq+So+Sx,
		      Vn+Vs+Vv+Vf+Vq+Vo+Vx, Fn+Fs+Fv+Ff+Fq+Fo+Fx,
		      Qn+Qs+Qv+Qf+Qq+Qo+Qx);
}

static char *help_strings[] = {
 "usage: %s -r RECORD -a REF TEST [OPTIONS ...]\n",
 "where RECORD is the record name;  REF is reference annotator name;  TEST is",
 "the test annotator name; and OPTIONS may include any of:",
 " -c FILE        append condensed reports (AAMI RP Table 6 format) to FILE",
 " -C FILE        as for -c, but report SVEB statistics also",
 " -f TIME        begin comparison at specified TIME (default: 5 minutes",
 "                 after beginning of record)",
 " -h             print this usage summary",
 " -l FILE1 FILE2 append line-format reports (AAMI RP Tables 7 and 8 format)",
 "                 to FILE1 and FILE2 respectively",
 " -L FILE1 FILE2 as for -l, but report SVEB statistics also",
 " -o             generate an output annotation file",
 " -O             generate an expanded output annotation file only",
 " -s FILE        append standard reports (AAMI RP Table 3 format) to FILE",
 " -S FILE        as for -s, but report SVEB statistics also",
 " -t TIME        stop comparison at specified TIME (default: end of record",
 "                 if defined, end of reference annotation file otherwise;",
 "                 if TIME is 0, the comparison ends when the end of either",
 "                 annotation file is reached)",
 " -v             verbose mode:  list all beat label discrepancies",
 " -w TIME        set match window (default: 0.15 seconds)",
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

/* Function `genxcmp' is used only when generating an expanded output
   annotation file. */

void genxcmp()
{
    int alen;
    static char mstring[550], *p, nullaux[1];

    if (ref_annot.anntyp == 0) getref();
    if (test_annot.anntyp == 0) gettest();
    if (!ref_annot.aux) ref_annot.aux = nullaux;
    if (!test_annot.aux) test_annot.aux = nullaux;
    while ((end_time > 0L && (T <= end_time || t <= end_time)) ||
	   (end_time == -1L && T != huge_time) ||
	   (end_time == 0L && T != huge_time && t != huge_time)) {
	for (p = mstring+1; p < mstring + *mstring; p++)
	    *p = '\0';
	if (t < T) {
	    /* Test annotation precedes reference annotation. */
	    if (T-t <= match_dt &&
		     (T-t < abs(T-tprime) || aprime == 'O')) {
		/* Annotation times match within the window. */
		if (test_annot.anntyp != ref_annot.anntyp ||
		    test_annot.subtyp != ref_annot.subtyp ||
		    test_annot.chan   != ref_annot.chan   ||
		    test_annot.num    != ref_annot.num    ||
		    strcmp(test_annot.aux, ref_annot.aux)) {
		    /* Annotation types, subtypes, ..., don't match. */
		    p = mstring+1;
		    (void)strcat(p, annstr(ref_annot.anntyp));
		    p += strlen(p);
		    if (test_annot.subtyp != ref_annot.subtyp ||
			test_annot.chan   != ref_annot.chan   ||
			test_annot.num    != ref_annot.num) {
			(void)sprintf(p, "[%d,%d,%d]", ref_annot.subtyp,
				ref_annot.chan, ref_annot.num);
			p += strlen(p);
		    }
		    if (*ref_annot.aux) {
			(void)sprintf(p, " (%s)", ref_annot.aux+1);
			p += strlen(p);
		    }
		    (void)sprintf(p, "/%s", annstr(test_annot.anntyp));
		    p += strlen(p);
		    if (*test_annot.aux)
			(void)sprintf(p, " (%s)", test_annot.aux+1);
		    alen = strlen(mstring+1);
		    if (alen > 254) {
			(void)fprintf(stderr,
				      "aux string truncated at %s (%ld)\n",
				timstr(test_annot.time), test_annot.time);
			alen = 254;
			mstring[alen+1] = '\0';
		    }
		    mstring[0] = alen;
		    test_annot.aux = mstring;
		    test_annot.anntyp = NOTE;
		}
		(void)putann(0, &test_annot);
		getref();
		gettest();
		if (!ref_annot.aux) ref_annot.aux = nullaux;
		if (!test_annot.aux) test_annot.aux = nullaux;
	    }
	    else {
		/* No matching reference annotation. */
		p = mstring+1;
		(void)sprintf(p, "%c/%s",
			   rpann(test_annot.time), annstr(test_annot.anntyp));
		p += strlen(p);
		if (*test_annot.aux)
		    (void)sprintf(p, " (%s)", test_annot.aux+1);
		alen = strlen(mstring+1);
		if (alen > 254) {
		    (void)fprintf(stderr, "aux string truncated at %s (%ld)\n",
			    timstr(test_annot.time), test_annot.time);
		    alen = 254;
		    mstring[alen+1] = '\0';
		}
		mstring[0] = alen;
		test_annot.aux = mstring;
		test_annot.anntyp = NOTE;
		(void)putann(0, &test_annot);
		gettest();
		if (!test_annot.aux) test_annot.aux = nullaux;
	    }
	}
	else {
	    /* Reference annotation precedes test annotation. */
	    if (t-T <= match_dt &&
		     (t-T < abs(t-Tprime) || Aprime == 'O')) {
		/* Annotation times match within the window. */
		if (test_annot.anntyp != ref_annot.anntyp ||
		    test_annot.subtyp != ref_annot.subtyp ||
		    test_annot.chan   != ref_annot.chan   ||
		    test_annot.num    != ref_annot.num    ||
		    strcmp(test_annot.aux, ref_annot.aux)) {
		    /* Annotation types, subtypes, ..., don't match. */
		    p = mstring+1;
		    (void)strcat(p, annstr(ref_annot.anntyp));
		    p += strlen(p);
		    if (test_annot.subtyp != ref_annot.subtyp ||
			test_annot.chan   != ref_annot.chan   ||
			test_annot.num    != ref_annot.num) {
			(void)sprintf(p, "[%d,%d,%d]", ref_annot.subtyp,
				ref_annot.chan, ref_annot.num);
			p += strlen(p);
		    }
		    if (*ref_annot.aux) {
			(void)sprintf(p, " (%s)", ref_annot.aux+1);
			p += strlen(p);
		    }
		    (void)sprintf(p, "/%s", annstr(test_annot.anntyp));
		    p += strlen(p);
		    if (*test_annot.aux)
			(void)sprintf(p, " (%s)", test_annot.aux+1);
		    alen = strlen(mstring+1);
		    if (alen > 254) {
			(void)fprintf(stderr,
				      "aux string truncated at %s (%ld)\n",
				timstr(test_annot.time), test_annot.time);
			alen = 254;
			mstring[alen+1] = '\0';
		    }
		    mstring[0] = alen;
		    test_annot.aux = mstring;
		    test_annot.anntyp = NOTE;
		}
		(void)putann(0, &test_annot);
		getref();
		gettest();
		if (!ref_annot.aux) ref_annot.aux = nullaux;
		if (!test_annot.aux) test_annot.aux = nullaux;
	    }
	    else {
		/* No matching test annotation. */
		p = mstring+1;
		(void)sprintf(p, "%s", annstr(ref_annot.anntyp));
		p += strlen(p);
		if (*ref_annot.aux) {
		    (void)sprintf(p, " (%s)", ref_annot.aux+1);
		    p += strlen(p);
		}
		(void)sprintf(p, "/%c", tpann(ref_annot.time));
		alen = strlen(mstring+1);
		if (alen > 254) {
		    (void)fprintf(stderr, "aux string truncated at %s (%ld)\n",
			    timstr(ref_annot.time), ref_annot.time);
		    alen = 254;
		    mstring[alen+1] = '\0';
		}
		mstring[0] = alen;
		ref_annot.aux = mstring;
		ref_annot.anntyp = NOTE;
		(void)putann(0, &ref_annot);
		getref();
		if (!ref_annot.aux) ref_annot.aux = nullaux;
	    }
	}
    }
}
