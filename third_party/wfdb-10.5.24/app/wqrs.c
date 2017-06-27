/* file: wqrs.c		Wei Zong      23 October 1998
			Last revised:  9 April 2010 (by G. Moody)
-----------------------------------------------------------------------------
wqrs: Single-lead QRS detector based on length transform
Copyright (C) 1998-2010 Wei Zong

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

You may contact the author by e-mail (wzong@mit.edu) or postal mail
(MIT Room E25-505, Cambridge, MA 02139, USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
------------------------------------------------------------------------------

This program analyzes an ECG signal, detecting QRS onsets and J-points, using
a nonlinearly-scaled ECG curve length feature.  This version has been optimized
for ECGs sampled at 125 Hz, but it can analyze ECGs sampled at any frequency
using on-the-fly resampling provided by the WFDB library.

`wqrs' can process records containing any number of signals, but it uses only
one signal for QRS detection (signal 0 by default;  this can be changed using
the `-s' option, see below).  'wqrs' has been optimized for adult human ECGs.
For other ECGs, it may be necessary to experiment with the input sampling
frequency and the time constants indicated below.

To compile this program under GNU/Linux, MacOS/X, MS-Windows, or Unix, use gcc:
	gcc -o wqrs wqrs.c -lwfdb -lm
You must have installed the WFDB library, available at	
	http://www.physionet.org/physiotools/wfdb.shtml
gcc is standard with GNU/Linux and is available for other platforms from:
	http://www.gnu.org/		(sources and Unix binaries)
	http://fink.sourceforge.net	(Mac OS/X only)
	http://www.cygwin.com/   	(MS-Windows only)
	
For a usage summary, see the help text at the end of this file.  The input
record may be in any of the formats readable by the WFDB library, and it may
be anywhere in the WFDB path (in a local directory or on a remote web or ftp
server).  The output of 'wqrs' is an annotation file named RECORD.wqrs (where
RECORD is replaced by the name of the input record).  Within the output
annotation file, the time of each NORMAL annotation marks a QRS onset;  if
the '-j' option is used, additional JPT annotations mark the J points (the
ends of the QRS complexes).

For example, to mark QRS complexes in record 100 beginning 5 minutes from the
start, ending 10 minutes and 35 seconds from the start, and using signal 1, use
the command:
    wqrs -r 100 -f 5:0 -t 10:35 -s 1
The output may be read using (for example):
    rdann -a wqrs -r 100
To evaluate the performance of this program, run it on the entire record, by:
    wqrs -r 100
and then compare its output with the reference annotations by:
    bxb -r 100 -a atr wqrs
*/

#include <stdio.h>
#include <math.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>
#include <wfdb/ecgmap.h>

#define BUFLN  16384	/* must be a power of 2, see ltsamp() */
#define EYE_CLS 0.25    /* eye-closing period is set to 0.25 sec (250 ms) */ 
#define MaxQRSw 0.13    /* maximum QRS width (130ms) */                        
#define NDP	 2.5    /* adjust threshold if no QRS found in NDP seconds */
#define PWFreqDEF 60    /* power line (mains) frequency, in Hz (default) */
#define TmDEF	 100	/* minimum threshold value (default) */

char *pname;		/* the name by which this program was invoked */
double lfsc;		/* length function scale constant */
int *ebuf;
int nsig;		/* number of input signals */
int LPn, LP2n;          /* filter parameters (dependent on sampling rate) */
int LTwindow;           /* LT window size */
int PWFreq = PWFreqDEF;	/* power line (mains) frequency, in Hz */
int sig = -1;	        /* signal number of signal to be analyzed */
int Tm = TmDEF;		/* minimum threshold value */
WFDB_Sample *lbuf = NULL;

/* ltsamp() returns a sample of the length transform of the input at time t.
   Since this program analyzes only one signal, ltsamp() does not have an
   input argument for specifying a signal number; rather, it always filters
   and returns samples from the signal designated by the global variable
   'sig'.  The caller must never "rewind" by more than BUFLN samples (the
   length of ltsamp()'s buffers). */

WFDB_Sample ltsamp(WFDB_Time t)
{
    int dy;
    static int Yn, Yn1, Yn2;
    static WFDB_Time tt = (WFDB_Time)-1L;

    if (lbuf == NULL) {
	lbuf = (WFDB_Sample *)malloc((unsigned)BUFLN*sizeof(WFDB_Sample));
	ebuf = (int *)malloc((unsigned)BUFLN * sizeof(int));
	if (lbuf && ebuf) {
	    for (ebuf[0] = sqrt(lfsc), tt = 1L; tt < BUFLN; tt++)
		ebuf[tt] = ebuf[0];
	    if (t > BUFLN) tt = (WFDB_Time)(t - BUFLN);
	    else tt = (WFDB_Time)-1L;
	    Yn = Yn1 = Yn2 = 0;
	}
	else {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
    }
    if (t < tt - BUFLN) {
        fprintf(stderr, "%s: ltsamp buffer too short\n", pname);
	exit(2);
    }
    while (t > tt) {
	static int aet = 0, et;
	WFDB_Sample v0, v1, v2;

	Yn2 = Yn1;
	Yn1 = Yn;
	if ((v0 = sample(sig, tt)) != WFDB_INVALID_SAMPLE &&
	    (v1 = sample(sig, tt-LPn)) != WFDB_INVALID_SAMPLE &&
	    (v2 = sample(sig, tt-LP2n)) != WFDB_INVALID_SAMPLE)
	    Yn = 2*Yn1 - Yn2 + v0 - 2*v1 + v2;
	dy = (Yn - Yn1) / LP2n;		/* lowpass derivative of input */
	et = ebuf[(++tt)&(BUFLN-1)] = sqrt(lfsc +dy*dy); /* length transform */
	lbuf[(tt)&(BUFLN-1)] = aet += et - ebuf[(tt-LTwindow)&(BUFLN-1)];
	/* lbuf contains the average of the length-transformed samples over
	   the interval from tt-LTwindow+1 to tt */
    }
    return (lbuf[t&(BUFLN-1)]);
}

main(int argc, char **argv)
{ 
    char *p;
    char *record = NULL;	     /* input record name */
    float sps;			     /* sampling frequency, in Hz (SR) */
    float samplingInterval;          /* sampling interval, in milliseconds */
    int i, max, min, minutes = 0, onset, timer, vflag = 0;
    int dflag = 0;		     /* if non-zero, dump raw and filtered
					samples only;  do not run detector */
    int jflag = 0;		     /* if non-zero, annotate J-points */
    int Rflag = 0;		     /* if non-zero, resample at 120 or 150 Hz
				      */
    int EyeClosing;                  /* eye-closing period, related to SR */
    int ExpectPeriod;                /* if no QRS is detected over this period,
					the threshold is automatically reduced
					to a minimum value;  the threshold is
					restored upon a detection */
    double Ta, T0;		     /* high and low detection thresholds */
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    WFDB_Gain gain;
    WFDB_Siginfo *s;
    WFDB_Time from = 0L, next_minute, spm, t, tj, tpq, to = 0L, tt, t1;
    static int gvmode = WFDB_GVPAD | WFDB_LOWRES;
    char *prog_name();
    void help();

    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'd':	/* dump filter data */
	    dflag = 1;
	    break;
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		exit(1);
	    }
	    from = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* operate in WFDB_HIGHRES mode */
	    gvmode = WFDB_GVPAD | WFDB_HIGHRES;
	    break;
	  case 'j':	/* annotate J-points (ends of QRS complexes) */
	    jflag = 1;
	    break;
	  case 'm':	/* threshold */
	    if (++i >= argc || (Tm = atoi(argv[i])) <= 0) {
		(void)fprintf(stderr, "%s: threshold ( > 0) must follow -m\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'p':	/* specify power line (mains) frequency */
	    if (++i >= argc || (PWFreq = atoi(argv[i])) <= 0) {
		(void)fprintf(stderr,
			    "%s: power line frequency ( > 0) must follow -p\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 'R':	/* resample */
	    Rflag = 1;
	    break;
	  case 's':	/* signal */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: signal number or name must follow -s\n",
			      pname);
		exit(1);
	    }
	    sig = i;	/* remember the argument until the record is open */
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n", pname);
		exit(1);
	    }
	    to = i;
	    break;
	  case 'v':	/* verbose mode */
	    vflag = 1;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n", pname,
			  argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n", pname,
			  argv[i]);
	    exit(1);
	}
    }
    if (record == NULL) {
	help();
	exit(1);
    }

    if (gvmode == 0 && (p = getenv("WFDBGVMODE")))
	gvmode = atoi(p);
    setgvmode(gvmode|WFDB_GVPAD);

    if ((nsig = isigopen(record, NULL, 0)) < 1) exit(2);
    if ((s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, s, nsig)) < 1) exit(2);
    sps = sampfreq((char *)NULL);
    if (sps < PWFreq) {
	(void)fprintf(stderr, "%s: sampling frequency (%g Hz) is too low%s",
		      pname, sps,
		      (gvmode & WFDB_HIGHRES) ? "\n" : ", try -H option\n");
	exit(3);
    }
    if (gvmode & WFDB_HIGHRES)
	setafreq(sampfreq((char *)NULL));
    a.name = "wqrs"; a.stat = WFDB_WRITE;
    if (annopen(record, &a, 1) < 0) exit(2);
    if (sig >= 0) sig = findsig(argv[sig]);
    if (sig < 0 || sig >= nsig) sig = 0;
    if ((gain = s[sig].gain) == 0.0) gain = WFDB_DEFGAIN;
    if (Rflag) {
    	if (PWFreq == 60.0) setifreq(sps = 120.);
    	else setifreq(sps = 150.);
    }
    if (from > 0L) {
	if ((from = strtim(argv[from])) < 0L)
	from = -from;
    }
    if (to > 0L) {
	if ((to = strtim(argv[to])) < 0L)
	    to = -to;
    }
    else
	to = strtim("e");

    annot.subtyp = annot.num = 0;
    annot.chan = sig;
    annot.aux = NULL;
    Tm = muvadu((unsigned)sig, Tm);
    samplingInterval = 1000.0/sps;
    lfsc = 1.25*gain*gain/sps;	/* length function scale constant */
    spm = 60 * sps;
    next_minute = from + spm;
    LPn = sps/PWFreq; 		/* The LP filter will have a notch at the
				    power line (mains) frequency */
    if (LPn > 8)  LPn = 8;	/* avoid filtering too agressively */
    LP2n = 2 * LPn;
    EyeClosing = sps * EYE_CLS; /* set eye-closing period */
    ExpectPeriod = sps * NDP;	/* maximum expected RR interval */
    LTwindow = sps * MaxQRSw;   /* length transform window size */

    (void)sample(sig, 0L);
    if (dflag) {
	for (t = from; t < to || (to == 0L && sample_valid()); t++)
	    printf("%6d\t%6d\n", sample(sig, t), ltsamp(t));
	exit(0);
    }

    if (vflag) {
	printf("\n------------------------------------------------------\n");
	printf("Record Name:             %s\n", record);
	printf("Total Signals:           %d  (", nsig);
	for (i = 0; i < nsig - 1; i++)
	    printf("%d, ", i);
	printf("%d)\n", nsig - 1);
	printf("Sampling Frequency:      %.1f Hz\n", sps);
	printf("Sampling Interval:       %.3f ms\n", samplingInterval);
	printf("Signal channel used for detection:    %d\n", sig);
	printf("Eye-closing period:      %d samples (%.0f ms)\n",
	       EyeClosing, EyeClosing*samplingInterval);
	printf("Minimum threshold:       %d A/D units (%d microvolts)\n",
	       Tm, adumuv(sig, Tm));
	printf("Power line frequency:    %d Hz\n", PWFreq);
	printf("\n------------------------------------------------------\n\n");
	printf("Processing:\n");
    }

    /* Average the first 8 seconds of the length-transformed samples
       to determine the initial thresholds Ta and T0. The number of samples
       in the average is limited to half of the ltsamp buffer if the sampling
       frequency exceeds about 2 KHz. */
    if ((t1 = strtim("8")) > BUFLN*0.9)
	t1 = BUFLN/2;
    t1 += from;
    for (T0 = 0, t = from; t < t1 && sample_valid(); t++)
	T0 += ltsamp(t);
    T0 /= t1 - from;
    Ta = 3 * T0;

    /* Main loop */
    for (t = from; t < to || (to == 0L && sample_valid()); t++) {
	static int learning = 1, T1;
	
	if (learning) {
	    if (t > t1) {
		learning = 0;
		T1 = T0;
		t = from;	/* start over */
	    }
	    else
		T1 = 2*T0;
	}
	
	/* Compare a length-transformed sample against T1. */
	if (ltsamp(t) > T1) {	/* found a possible QRS near t */
	    timer = 0; /* used for counting the time after previous QRS */
	    max = min = ltsamp(t);
	    for (tt = t+1; tt < t + EyeClosing/2; tt++)
		if (ltsamp(tt) > max) max = ltsamp(tt);
	    for (tt = t-1; tt > t - EyeClosing/2; tt--)
		if (ltsamp(tt) < min) min = ltsamp(tt);
	    if (max > min+10) { /* There is a QRS near tt */
		/* Find the QRS onset (PQ junction) */
		onset = max/100 + 2;
		tpq = t - 5;
		for (tt = t; tt > t - EyeClosing/2; tt--) {
		    if (ltsamp(tt)   - ltsamp(tt-1) < onset &&
			ltsamp(tt-1) - ltsamp(tt-2) < onset &&
			ltsamp(tt-2) - ltsamp(tt-3) < onset &&
			ltsamp(tt-3) - ltsamp(tt-4) < onset) {
			tpq = tt - LP2n;	/* account for phase shift */
			break;
		    }
		}

		if (!learning) {
		    /* Check that we haven't reached the end of the record. */
		    (void)sample(sig, tpq);
		    if (sample_valid() == 0) break;
		    /* Record an annotation at the QRS onset */
		    annot.time = tpq;
		    annot.anntyp = NORMAL;
		    if (putann(0, &annot) < 0) { /* write the annotation */
			wfdbquit();	/* close files if an error occurred */
			exit(1);
		    }
		    if (jflag) {
			/* Find the end of the QRS */
			for (tt = t, tj = t + 5; tt < t + EyeClosing/2; tt++) {
			    if (ltsamp(tt) > max - (max/10)) {
				tj = tt;
				break;
			    }
			}
			(void)sample(sig, tj);
			if (sample_valid() == 0) break;
			/* Record an annotation at the J-point */
			annot.time = tj;
			annot.anntyp = JPT;
			if (putann(0, &annot) < 0) {
			    wfdbquit();
			    exit(1);
			}
		    }
		}

		/* Adjust thresholds */
		Ta += (max - Ta)/10;
		T1 = Ta / 3;


		/* Lock out further detections during the eye-closing period */
		t += EyeClosing;
	    }
	}
	else if (!learning) {
	    /* Once past the learning period, decrease threshold if no QRS
	       was detected recently. */
	    if (++timer > ExpectPeriod && Ta > Tm) {
		Ta--;
		T1 = Ta / 3;
	    }      
	}

	/* Keep track of progress by printing a dot for each minute analyzed */
	if (t >= next_minute) {
	    next_minute += spm;
	    (void)fprintf(stderr, ".");
	    (void)fflush(stderr);
	    if (++minutes >= 60) {
		(void)fprintf(stderr, " %s\n", timstr(t));
		minutes = 0;
	    }
	}
    }
    if (minutes) (void)fprintf(stderr, " %s\n", timstr(t));

    (void)free(lbuf);
    (void)free(ebuf);
    wfdbquit();		        /* close WFDB files */
    fprintf(stderr, "\n");
    if (vflag) {
	printf("\n\nDone! \n\nResulting annotation file:  %s.wqrs\n\n\n",
	       record);
    }
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
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the record to be analyzed, and OPTIONS may",
 "include any of:",
 " -d          dump unfiltered and filtered samples on standard output;",
 "             do not annotate",
 " -f TIME     begin at specified time (default: beginning of the record)",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -j          find and annotate J-points (QRS ends) as well as QRS onsets",
 " -m THRESH   set detector threshold to THRESH (default: 100)", /* TmDEF */
 " -p FREQ     specify power line (mains) frequency (default: 60)",
 								/* PWFreqDEF */
 " -R          resample input at 120 or 150 Hz, depending on power line",
 "             frequency (default: do not resample)",
 " -s SIGNAL   analyze specified signal (default: 0)",
 " -t TIME     stop at specified time (default: end of the record)",
 " -v          verbose mode",
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
