/* file: gqrs.c			G. Moody		16 November 2006
				Last revised:		  21 July 2013   
-------------------------------------------------------------------------------
gqrs: A QRS detector
Copyright (C) 2006-2013 George B. Moody

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
*/

#include <stdio.h>
#include <stddef.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

#define BUFLN 32768	/* must be a power of 2, see qf() */
#define NPEAKS 64	/* number of peaks buffered (per signal) */

/* The `getconf' macro is used by gqrs_init() (below) to check a line of input
   (already in `buf', defined in gqrs_init) for the string named by getconf's
   first argument.  If the string is found, the value following the string
   (and an optional `:' or `=') is converted using sscanf and the format
   specifier supplied as getconf's second argument, and stored in the variable
   named by the first argument. */
#define getconf(a, fmt)	if (p=strstr(buf,#a)){sscanf(p,#a "%*[=: \t]" fmt,&a);}

/* The 'peak' structure contains information about a local maximum in the
   filtered signal (qfv).  Peaks are stored in circular buffers of peak
   structures, one buffer per input signal.  The time of a flat-topped peak
   is the time of the first sample that has the maximum value.  A peak is
   secondary if there is a larger peak within its neighborhood (time +- rrmin),
   of if it has been identified as a T-wave associated with a previous primary
   peak.  A peak is primary if it is largest in its neighborhood, or if the
   only larger peaks are secondary. */
struct peak {
    struct peak *prev, *next; /* pointers to neighbors (in time) */
    WFDB_Time time;	/* time of local maximum of qfv */
    int amp;	 /* value of qfv at time of peak */
    short type;  /* 1: primary, 2: secondary, 0: not determined */
} *peaks, *cpeak;

/* Prototypes of functions defined below.  The definitions of these functions
   follow that of main(), in the order shown below. */
WFDB_Sample sm(WFDB_Time t);
void qf(void);
void addpeak(WFDB_Time t, int peak_amplitude);
int peaktype(struct peak *p);
void gqrs_init(WFDB_Time from, WFDB_Time to);
void rewind_gqrs(void);
struct peak *find_missing(struct peak *previous_peak, struct peak *next_peak);
void gqrs(WFDB_Time from, WFDB_Time to);
void help(void);
char *prog_name(char *p);
void *gcalloc(size_t nmemb, size_t size);
void cleanup(int status);

char auxbuf[1+255+1];		/* 'aux' string buffer for annotations */
char *pname;			/* name of this program, used in messages */
char *record = NULL;		/* name of input record */
FILE *config = NULL;		/* configuration file, if any */
int countdown = -1;		/* if > 0, ticks remaining (see gqrs()) */
int debug;			/* if non-zero, generate debugging output */
int minutes = 0;		/* minutes elapsed in the current hour */
int nsig;			/* number of signals */
int pthr;			/* peak-detection threshold */
int qthr;			/* QRS-detection threshold */
int pthmin, qthmin;		/* minimum values for pthr and qthr */
int rrdev;			/* mean absolute deviation of RR from rrmean */
int rrinc;			/* maximum incremental change in rrmean */
int rrmean;			/* mean RR interval, in sample intervals */
int rrmax;			/* maximum likely RR interval */
int rrmin;			/* minimum RR interval, in sample intervals */
int rtmax;			/* maximum RT interval, in sample intervals */
int rtmin;			/* minimum RT interval, in sample intervals */
int rtmean;			/* mean RT interval, in sample intervals */
int tamean;			/* mean T-wave amplitude in qfv */
double thresh = 1.0;		/* normalized detection threshold */
long v1;			/* integral of dv in qf() */
long v1norm;			/* normalization for v1 */
WFDB_Annotation annot;		/* most recently written annotation */
WFDB_Sample *v;			/* latest sample of each input signal */
WFDB_Siginfo *si;		/* characteristics of each input signal */
WFDB_Signal sig;		/* signal number of signal to be analyzed */
WFDB_Time next_minute;		/* sample number of start of next minute */
WFDB_Time spm;			/* sample intervals per minute */
WFDB_Time sps;			/* sample intervals per second */
WFDB_Time t;			/* time of the current sample being analyzed */
WFDB_Time t0;			/* time of the start of the analysis period */
WFDB_Time tf;			/* time of the end of the analysis period */
WFDB_Time tf_learn;		/* time of the end of the learning period */

/* Current operating mode of the detector */
enum { LEARNING, RUNNING, CLEANUP } state = LEARNING;

int dt, dt2, dt3, dt4, smdt;	/* time intervals set by gqrs_init() depending
				   on the sampling frequency and used by
				   filter functions sm() and qf();  units
				   are sample intervals */
WFDB_Time smt = 0, smt0;	/* current and starting time for sm() */
long *qfv, *smv;		/* filter buffers allocated by gqrs_init() */

/* The q() and s() macros can be used for fast lookup of filter outputs. */

#define q(T) (qfv[(T)&(BUFLN-1)])
#define s(T) (smv[(T)&(BUFLN-1)])

main(int argc, char **argv)
{
    char *p;
    int gvmode = 0, i, isiglist = 0, j, nisig;
    WFDB_Anninfo a;

    pname = prog_name(argv[0]);
    a.name = "qrs"; a.stat = WFDB_WRITE;

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':	/* configuration file */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		     "%s: name of configuration file must follow -c\n", pname);
		cleanup(1);
	    }
	    if ((config = fopen(argv[i], "rt")) == NULL) {
		(void)fprintf(stderr,
		     "%s: can't read configuration file %s\n", pname, argv[i]);
		cleanup(1);
	    }
	    break;
	  case 'd':	/* record debugging info in the annotation file */
	    debug = 1;
	    break;
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		cleanup(1);
	    }
	    t0 = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    cleanup(0);
	    break;
	  case 'H':	/* operate in WFDB_HIGHRES mode */
	    gvmode = WFDB_HIGHRES;
	    break;
	  case 'm':	/* threshold */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: threshold must follow -m\n", pname);
		cleanup(1);
	    }
	    thresh = atof(argv[i]);
	    break;
	  case 'o':	/* write output annotations as specified annotator */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: annotator name must follow -o\n",
			      pname);
		cleanup(1);
	    }
	    a.name = argv[i];
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		cleanup(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal name or number follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: signal name or # must follow -s\n",
			pname);
		cleanup(1);
	    }
	    sig = i;
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		cleanup(1);
	    }
	    tf = i;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n", pname,
			  argv[i]);
	    cleanup(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n", pname,
			  argv[i]);
	    cleanup(1);
	}
    }

    if (record == NULL) {
	help();
	cleanup(1);
    }

    if (gvmode == 0 && (p = getenv("WFDBGVMODE")))
	gvmode = atoi(p);
    setgvmode(gvmode|WFDB_GVPAD);

    if ((nsig = isigopen(record, NULL, 0)) < 1) cleanup(2);
    si = (WFDB_Siginfo *)gcalloc((size_t)nsig, sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)gcalloc((size_t)nsig, sizeof(WFDB_Sample));
    if ((nsig = wfdbinit(record, &a, 1, si, nsig)) < 1) cleanup(2);
    if (sampfreq((char *)NULL) < 50.) {
	(void)fprintf(stderr, "%s: sampling frequency (%g Hz) is too low%s",
		      pname, sampfreq((char *)NULL),
		      gvmode & WFDB_HIGHRES ? "\n" : ", try -H option\n");
	cleanup(3);
    }
    if (t0 > 0L && (t0 = strtim(argv[t0])) < 0L)
	    t0 = -t0;
    if (tf > 0L) {
	if ((tf = strtim(argv[tf])) < 0L)
	    tf = -tf;
	tf += sps;    /* make sure that the last beat before 'to' is marked */
    }
    else
	tf = strtim("e");
    t = t0;
    sps = strtim("1");
    spm = strtim("1:0");
    next_minute = t + spm;

    if (sig > 0) {
	int i = findsig(argv[sig]);
	if (i < 0) {
	    (void)fprintf(stderr,
			  "%s: (warning) no signal %s in record %s\n",
			  pname, argv[sig], record);
	    cleanup(4);
	}
	sig = i;
    }

    /* Record the argument list in a NOTE annotation. */
    for (i = j = 0, p = auxbuf+1; i < argc; i++) {
	int len;
	j += len = strlen(argv[i]);
	if (j < 255) {
	    strncpy(p, argv[i], len);
	    p += len;
	    *p++ = ' ';
	    j++;
	}
	else {
	    j -= len;
	    break;
	}
    }	
    *(--p) = '\0';
    auxbuf[0] = j;
    annot.subtyp = annot.chan = annot.num = 0;
    annot.aux = auxbuf;
    annot.anntyp = NOTE;
    annot.time = (WFDB_Time)0;
    putann(0, &annot);

    gqrs_init(t0, tf);	/* initialize variables for gqrs() */
    if (spm >= BUFLN) {
	if (tf - t0 > BUFLN) tf_learn = t0 + BUFLN - dt4;
	else tf_learn = tf - dt4;
    }
    else {
	if (tf - t0 > spm) tf_learn = t0 + spm - dt4;
	else tf_learn = tf - dt4;
    }

    state = LEARNING;
    gqrs(t0, tf_learn);

    rewind_gqrs();
    state = RUNNING;
    t = t0 - dt4;
    gqrs(t0, tf);		/* run the detector and collect output */

    printf(" %s\n", timstr(strtim("i")));
    cleanup(0);
}

/* sm() implements a trapezoidal low pass (smoothing) filter (with a gain of
   4*smdt) applied to input signal sig before the QRS matched filter qf().
   Before attempting to 'rewind' by more than BUFLN-smdt samples, reset smt
   and smt0.
 */

WFDB_Sample sm(WFDB_Time t)
{
    while (t > smt) {
	int i;

	if (++smt > smt0) {	/* fast update by summing first differences */
	    s(smt) = s(smt-1) +
		sample(sig, smt+smdt) + sample(sig, smt+smdt-1) -
		sample(sig, smt-smdt) - sample(sig, smt-smdt-1);
	}
	else { 			/* get initial value by full convolution */
	    int j, v;

	    for (j = 1, v = sample(sig, smt); j < smdt; j++)
		v += sample(sig, smt+j) + sample(sig, smt-j);
		s(smt) = (v << 1) + sample(sig, smt+j) + sample(sig, smt-j)
		    - si[sig].adczero * (smdt << 2); /* FIXME: needed? */
	}
    }
    return (s(t));
}

void qf()	/* evaluate the QRS detector filter for the next sample */
{
    long dv, dv1, dv2, v0;

    dv2 = sm(t+dt4);/* do this first, to ensure that all of the other
		       smoothed values needed below are in the buffer */
    dv2 -= s(t-dt4);
    dv1 = s(t+dt)  - s(t-dt);
    dv  = (dv1 << 1);
    dv -= s(t+dt2) - s(t-dt2);
    dv <<= 1;
    dv += dv1;
    dv -= s(t+dt3) - s(t-dt3);
    dv <<= 1;
    dv += dv2;
    v1 += dv;
    v0 = v1 / v1norm;  /* scaling is needed to avoid overflow */
    q(t) = v0 * v0;
}

void addpeak(WFDB_Time t, int peak_amplitude)
{
    struct peak *p = cpeak->next;

    p->time = t;
    p->amp = peak_amplitude;
    p->type = 0;
    cpeak = p;
    (p->next)->amp = 0;
}

/* peaktype() returns 1 if p is the most prominent peak in its neighborhood, 2
   otherwise.  The neighborhood consists of all other peaks within rrmin.
   Normally, "most prominent" is equivalent to "largest in amplitude", but this
   is not always true.  For example, consider three consecutive peaks a, b, c
   such that a and b share a neighborhood, b and c share a neighborhood, but a
   and c do not; and suppose that amp(a) > amp(b) > amp(c).  In this case, if
   there are no other peaks, a is the most prominent peak in the (a, b)
   neighborhood.  Since b is thus identified as a non-prominent peak, c becomes
   the most prominent peak in the (b, c) neighborhood.  This is necessary to
   permit detection of low-amplitude beats that closely precede or follow beats
   with large secondary peaks (as, for example, in R-on-T PVCs).
*/

int peaktype(struct peak *p)
{
    if (p->type)
	return (p->type);
    else {
	int a = p->amp;
	struct peak *pp;
	WFDB_Time t0 = p->time - rrmin, t1 = p->time + rrmin;

	if (t0 < 0) t0 = 0;
	for (pp = p->prev; t0 < pp->time && pp->time < (pp->next)->time;
	     pp = pp->prev) {
	    if (pp->amp == 0) break;
	    if (a < pp->amp && peaktype(pp) == 1)
		return (p->type = 2);
	}
	for (pp = p->next; pp->time < t1 && pp->time > (pp->prev)->time;
	     pp = pp->next) {
	    if (pp->amp == 0) break;
	    if (a < pp->amp && peaktype(pp) == 1)
		return (p->type = 2);
	}
	return (p->type = 1);
    }
}

/* rewind_gqrs resets the sample pointers and annotation fields to their
   initial values. */
void rewind_gqrs(void)
{
    int i;
    struct peak *p;

    countdown = -1;
    (void)sample(0,t);
    annot.time = (WFDB_Time)0;
    annot.anntyp = NORMAL;
    annot.subtyp = annot.chan = annot.num = 0;
    annot.aux = NULL;
    for (i = 0, p = peaks; i < NPEAKS; i++, p++)
	p->type = p->amp = p->time = 0;
}

/* gqrs_init() is intended to be called once only, to initialize variables for
   the QRS detection function gqrs(). */

void gqrs_init(WFDB_Time from, WFDB_Time to)
{
    int i, dv;
    static double HR, RR, RRdelta, RRmin, RRmax, QS, QT, RTmin, RTmax,
	QRSa, QRSamin;
    
    /* Allocate workspace.  The allocator, gcalloc, is defined below;  it
       includes an error handler and on-exit deallocation so that this
       code can be kept simpler. */
    qfv = (long *)gcalloc((size_t)BUFLN, sizeof(long));
    smv = (long *)gcalloc((size_t)BUFLN, sizeof(long));
    peaks = (struct peak *)gcalloc((size_t)NPEAKS, sizeof(struct peak));

    /* Gather peak structures into circular buffers */
    for (i = 0; i < NPEAKS; i++) {
	peaks[i].next = &peaks[i+1];
	peaks[i].prev = &peaks[i-1];
    }
    peaks[0].prev = &peaks[NPEAKS-1];
    cpeak = peaks[NPEAKS-1].next = &peaks[0];

    /* Read a priori physiologic parameters from the configuration file if
       available. They can be adjusted in the configuration file for pediatric,
       fetal, or animal ECGs. */
    if (config) {
	char buf[256], *p;

	/* Read the configuration file a line at a time. */
	while (fgets(buf, sizeof(buf), config)) {
	    /* Skip comments (empty lines and lines beginning with `#'). */
	    if (buf[0] == '#' || buf[0] == '\n') continue;
	    /* Set parameters.  Each `getconf' below is executed once for
	       each non-comment line in the configuration file. */
	    getconf(HR, "%lf");
	    getconf(RR, "%lf");
	    getconf(RRdelta, "%lf");
	    getconf(RRmin, "%lf");
	    getconf(RRmax, "%lf");
	    getconf(QS, "%lf");
	    getconf(QT, "%lf");
	    getconf(RTmin, "%lf");
	    getconf(RTmax, "%lf");
	    getconf(QRSa, "%lf");
	    getconf(QRSamin, "%lf");
	}
	fclose(config);
    }

    /* If any a priori parameters were not specified in the configuration file,
       initialize them here (using values chosen for adult human ECGs). */
    if (HR != 0.0) RR = 60.0/HR;
    if (RR == 0.0) RR = 0.8;
    if (RRdelta == 0.0) RRdelta = RR/4;
    if (RRmin == 0.0) RRmin = RR/4;
    if (RRmax == 0.0) RRmax = 3*RR;
    if (QS == 0.0) QS = 0.07;
    if (QT == 0.0) QT = 5*QS;
    if (RTmin == 0.0) RTmin = 3*QS;
    if (RTmax == 0.0) RTmax = 5*QS;
    if (QRSa == 0.0) QRSa = 750;
    if (QRSamin == 0.0) QRSamin = QRSa/5;

    /* Initialize gqrs's adaptive parameters based on the a priori parameters.
       During its learning period, gqrs will adjust them based on the observed
       input; after the learning period, gqrs continues to adjust these
       parameters, but with slower rates of change than during the learning
       period. */
    rrmean = RR * sps;
    rrdev = RRdelta * sps;
    rrmin = RRmin * sps;
    rrmax = RRmax * sps;
    if ((rrinc = rrmean/40) < 1) rrinc = 1;    
    if ((dt = QS * sps / 4) < 1) {
	dt = 1;
	fprintf(stderr, "%s (warning): sampling rate may be too low\n", pname);
    }
    rtmin = RTmin * sps;	/* minimum RTpeak interval */
    rtmean = 0.75 * QT * sps;	/* expected RTpeak interval, about 75% of QT */
    rtmax = RTmax * sps;	/* maximum RTpeak interval */

    dv = muvadu(sig, (int)(QRSamin));
    pthr = (thresh * dv * dv) / 6;
    qthr = pthr << 1;
    pthmin = pthr >> 2;
    qthmin = (pthmin << 2)/3;
    tamean = qthr;	/* initial value for mean T-wave amplitude */

    /* Filter constants and thresholds. */
    dt2 = 2*dt;
    dt3 = 3*dt;
    dt4 = 4*dt;
    smdt = dt;
    v1norm = smdt * dt * 64;
    smt = t0;
    smt0 = t0 + smdt;
    t = t0 - dt4;
    for (i = 0; i < BUFLN; i++)
	qfv[i] = smv[i] = 0;
}

/* find_missing() is invoked by gqrs() whenever it is suspected that a
   low-amplitude beat may have been missed between two consecutive detected
   beats at r and p.  The primary peak closest to the expected time of
   the missing beat, if any, is returned as the suggested missing beat. */

struct peak *find_missing(struct peak *r, struct peak *p)
{
    int rrerr, rrtmp, minrrerr;
    struct peak *q, *s = NULL;

    if (r == NULL || p == NULL) return (NULL);
    minrrerr = p->time - r->time;
    for (q = r->next; q->time < p->time; q = q->next) {
	if (peaktype(q) == 1) {
	    rrtmp = q->time - r->time;
	    rrerr = rrtmp - rrmean;
	    if (rrerr < 0) rrerr = -rrerr;
	    if (rrerr < minrrerr) {
		minrrerr = rrerr;
		s = q;
	    }
	}
    }
    return (s);
}

/* gqrs() is the main QRS detection function.  It attempts to find all
   beats between the time limits specified by its arguments, and to label
   them using annotations of type NORMAL (gqrs() does not attempt to
   differentiate normal and ectopic beats). */

void gqrs(WFDB_Time from, WFDB_Time to)
{
    int c, i, qamp, q0, q1 = 0, q2 = 0, rr, rrd, rt, rtd, rtdmin;
    struct peak *p, *q = NULL, *r = NULL, *tw;
    WFDB_Time last_peak = from, last_qrs = from;

    while (t <= to + sps) {
	if (countdown < 0 && sample_valid())
	    qf();
	else if (countdown < 0) {
	    countdown = strtim("1");
	    state = CLEANUP;
	}
	else if (countdown-- <= 0)
	    break;

	q0 = q(t); q1 = q(t-1); q2 = q(t-2);
	if (q1 > pthr && q2 < q1 && q1 >= q0 && t > dt4) {
	    addpeak(t-1, q1);
	    last_peak = t-1;
	    for (p = cpeak->next; p->time < t - rtmax; p = p->next) {
		if (p->time >= annot.time + rrmin && peaktype(p) == 1) {
		    if (p->amp > qthr) {
			rr = p->time - annot.time;
			if (rr > rrmean + 2 * rrdev &&
			    rr > 2 * (rrmean - rrdev) &&
			    (q = find_missing(r, p))) {
			    p = q;
			    rr = p->time - annot.time;
			    annot.subtyp = 1;
			}
			if ((rrd = rr - rrmean) < 0) rrd = -rrd;
			rrdev += (rrd - rrdev) >> 3;
			if (rrd > rrinc) rrd = rrinc;
			if (rr > rrmean) rrmean += rrd;
			else rrmean -= rrd;
			if (p->amp > qthr * 4) qthr++;
			else if (p->amp < qthr) qthr--;
			if (qthr > pthr * 20) qthr = pthr * 20;
			last_qrs = p->time;
			if (state == RUNNING) {
			    int qsize;

			    annot.time = p->time - dt2;
			    annot.anntyp = NORMAL;
			    annot.chan = sig;
			    qsize = p->amp * 10.0 / qthr;
			    if (qsize > 127) qsize = 127;
			    annot.num = qsize;
			    putann(0, &annot);
			    annot.time += dt2;
			}
			/* look for this beat's T-wave */
			tw = NULL; rtdmin = rtmean;
			for (q = p->next; q->time > annot.time; q = q->next) {
			    rt = q->time - annot.time - dt2;
			    if (rt < rtmin) continue;
			    if (rt > rtmax) break;
			    if ((rtd = rt - rtmean) < 0) rtd = -rtd;
			    if (rtd < rtdmin) {
				rtdmin = rtd;
				tw = q;
			    }
			}
			if (tw) {
			    static WFDB_Annotation tann;

			    tann.time = tw->time - dt2;
			    if (debug && state == RUNNING) {
				tann.anntyp = TWAVE;
				tann.chan = sig+1;
				tann.num = rtdmin;
				tann.subtyp = (tann.time > annot.time + rtmean);
				tann.aux = NULL;
				putann(0, &tann);
			    }
			    rt = tann.time - annot.time;
			    if ((rtmean += (rt - rtmean) >> 4) > rtmax)
				rtmean = rtmax;
			    else if (rtmean < rtmin)
				rtmean = rrmin;
			    tw->type = 2;	/* mark T-wave as secondary */
			}
			r = p; q = NULL; qamp = 0; annot.subtyp = 0;
		    }
		    else if (t - last_qrs > rrmax && qthr > qthmin)
			qthr -= (qthr >> 4);
		}
	    }
	}
	else if (t - last_peak > rrmax && pthr > pthmin)
	    pthr -= (pthr >> 4);

	if (++t >= next_minute) {
	    next_minute += spm;
	    (void)fprintf(stderr, ".");
	    (void)fflush(stderr);
	    if (++minutes >= 60) {
		(void)fprintf(stderr, " %s\n", timstr(-t));
		minutes = 0;
	    }
	}
    }

    if (state == LEARNING)
	return;
    
    /* Mark the last beat or two. */
    for (p = cpeak->next; p->time < (p->next)->time; p = p->next) {
	//	if (to > 0 && p->time > to - sps)
	//    break;
	if (p->time >= annot.time + rrmin && p->time < tf && peaktype(p) == 1) {
	    annot.anntyp = NORMAL;
	    annot.chan = sig;
	    annot.time = p->time;
	    putann(0, &annot);
	}
    }
}

/* prog_name() extracts this program's name from argv[0], for use in error and
   warning messages. */

char *prog_name(char *s)
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

/* help() prints a (very) concise summary of how to use this program.
   A more detailed summary is in the man page (gqrs.1). */

static char *help_strings[] = {
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the record to be analyzed, and OPTIONS may",
 "include any of:",
 " -c FILE     initialize parameters from the specified configuration FILE",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -m THRESH   set detector threshold to THRESH (default: 1.00)",
 " -o ANN      save annotations as annotator ANN (default: qrs)", 
 " -s SIGNAL   analyze specified SIGNAL (default: 0)",
 "                (Note: SIGNAL may be specified by number or name.)",
 " -t TIME     stop at specified time",
 "If too many beats are missed, decrease THRESH;  if there are too many extra",
 "detections, increase THRESH.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}

/* gcalloc() is a wrapper for calloc() that handles errors and maintains
   a list of allocated buffers for automatic on-exit deallocation via
   gfreeall(). */

size_t gcn = 0, gcnmax = 0;
void **gclist = NULL;

void *gcalloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size), **q = NULL;

    if ((p == NULL) ||
	((gcn >= gcnmax) &&
	 (q = realloc(gclist, (gcnmax += 32)*(sizeof(void *)))) == NULL)) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	cleanup(3);
    }
    if (q) gclist = q;
    return (gclist[gcn++] = p);
}

void gfreeall()			/* free memory allocated using gcalloc() */
{
   while (gcn-- > 0)
	if (gclist[gcn]) free(gclist[gcn]);
    free(gclist);
}

void cleanup(int status)	/* close files and free allocated memory */
{
    if (record) wfdbquit();
    gfreeall();
    exit(status);
}
