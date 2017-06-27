/* file wabp.c          Wei Zong       23 October 1998
   			Last revised:   9 April 2010 (by G. Moody)
-----------------------------------------------------------------------------
wabp: beat detector for arterial blood presure (ABP) signal
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

This program detects heart beats (pulse waveforms) in a continuous arterial
blood pressure (ABP) signal.  This version of wabp works best with ABP signals
sampled at 125 Hz, but it can analyze ABPs sampled at any frequency
using on-the-fly resampling provided by the WFDB library.  'wabp' has been
optimized for adult human  ABPs. For other ABPs, it may be necessary to
experiment with the input sampling frequency and the time constants indicated
below.

`wabp' can process records containing any number of signals, but it uses only
one signal for ABP pulse detection (by default, the lowest-numbered signal
labelled `ABP', `ART', or `BP';  this can be changed using the `-s' option, see
below).

To compile this program under GNU/Linux, MacOS/X, MS-Windows, or Unix, use gcc:
        gcc -o wabp wabp.c -lwfdb
You must have installed the WFDB library, available at  
        http://www.physionet.org/physiotools/wfdb.shtml
gcc is standard with GNU/Linux and is available for other platforms from:
        http://www.gnu.org/             (sources and Unix binaries)
        http://fink.sourceforge.net     (Mac OS/X only)
        http://www.cygwin.com/          (MS-Windows only)
        
For a usage summary, see the help text at the end of this file.  The input
record may be in any of the formats readable by the WFDB library, and it may
be anywhere in the WFDB path (in a local directory or on a remote web or ftp
server).  The output of 'wabp' is an annotation file named RECORD.wabp (where
RECORD is replaced by the name of the input record).  Within the output
annotation file, the time of each NORMAL annotation marks an ABP pulse wave
onset.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

#define BUFLN   4096	/* must be a power of 2, see slpsamp() */
#define EYE_CLS 0.25    /* eye-closing period is set to 0.25 sec (250 ms) */ 
#define LPERIOD 1000	/* learning period is the first LPERIOD samples */
#define SLPW    0.13    /* Slope width (130ms) */                        
#define NDP	 2.5    /* adjust threshold if no pulse found in NDP seconds */
#define TmDEF	   5	/* minimum threshold value (default) */

char *pname;		/* the name by which this program was invoked */
int *ebuf;
int nsig;		/* number of input signals */
int SLPwindow;          /* Slope window size */
int sig = -1;	        /* signal number of signal to be analyzed (initial
			   value forces search for ABP, ART, or BP signal) */
int Tm = TmDEF;		/* minimum threshold value */
WFDB_Sample *lbuf = NULL;

char *trim_whitespace(char *str)
{
    size_t len = 0;
    char *frontp = str - 1;
    char *endp = NULL;

    if (str == NULL) return NULL;
    if (str[0] == '\0') return str;
    len = strlen(str);
    endp = str + len;
    while (isspace(*(++frontp)));
    while (isspace(*(--endp)) && endp != frontp);
    if (str + len - 1 != endp)
        *(endp + 1) = '\0';
    else if (frontp != str && endp == frontp)
        *str = '\0';
    endp = str;
    if (frontp != str) {
        while (*frontp) *endp++ = *frontp++;
        *endp = '\0';
    }
    return str;
}

WFDB_Sample slpsamp(WFDB_Time t)
{
    int dy;
    static WFDB_Time tt = (WFDB_Time)-1L;

    if (lbuf == NULL) {
	lbuf = (WFDB_Sample *)malloc((unsigned)BUFLN * sizeof(WFDB_Sample));
	ebuf = (int *)malloc((unsigned)BUFLN * sizeof(int));
	if (lbuf && ebuf) {
	    for (ebuf[0] = 0, tt = 1L; tt < BUFLN; tt++)
		ebuf[tt] = ebuf[0];
	    if (t > BUFLN) tt = (WFDB_Time)(t - BUFLN);
	    else tt = (WFDB_Time)-1L;
	}
	else {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
    }
    if (t < tt - BUFLN) {
        fprintf(stderr, "%s: slpsamp buffer too short\n", pname);
	exit(2);
    }
    while (t > tt) {
	static int aet = 0, et;
        int prevVal = 0;
        int val1;
        int val2;
        val2 = sample(sig, tt - 1);
        if (sample_valid() != 1)
            val2 = prevVal;
        val1 = sample(sig, tt);
        if (sample_valid() != 1)
            val1 = val2;
        prevVal = val2;
        dy = val1 - val2;
	if (dy < 0) dy = 0;
	et = ebuf[(++tt)&(BUFLN-1)] = dy; 
	lbuf[(tt)&(BUFLN-1)] = aet += et - ebuf[(tt-SLPwindow)&(BUFLN-1)];
    }
    return (lbuf[t&(BUFLN-1)]);
}

int main(int argc, char **argv)
{ 
    char *record = NULL;	     /* input record name */
    float sps;			     /* sampling frequency, in Hz (SR) */
    float samplingInterval;          /* sampling interval, in milliseconds */
    int i, max, min, minutes = 0, onset, timer, vflag = 0;
    int dflag = 0;		     /* if non-zero, dump raw and filtered
					samples only;  do not run detector */
    int Rflag = 0;		     /* if non-zero, resample at 125 Hz  */
    int EyeClosing;                  /* eye-closing period, related to SR */
    int ExpectPeriod;                /* if no ABP pulse is detected over this period,
					the threshold is automatically reduced
					to a minimum value;  the threshold is
					restored upon a detection */
    int Ta, T0;			     /* high and low detection thresholds */
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    WFDB_Siginfo *s;
    WFDB_Time from = 0L, next_minute, spm, t, tpq, to = 0L, tt, t1;
    char *p, *prog_name();
    static int gvmode = 0;
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
	    gvmode = WFDB_HIGHRES;
	    break;
	  case 'm':	/* threshold */
	    if (++i >= argc || (Tm = atoi(argv[i])) <= 0) {
		(void)fprintf(stderr, "%s: threshold ( > 0) must follow -m\n",
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
	    sig = i;	/* remember argument until record is open */
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
    a.name = "wabp"; a.stat = WFDB_WRITE;
    if ((nsig = wfdbinit(record, &a, 1, s, nsig)) < 1) exit(2);
    if (sig >= 0) sig = findsig(argv[sig]);
    if (sig < 0 || sig >= nsig) {
	/* Identify the lowest-numbered ABP, ART, or BP signal */
	for (i = 0; i < nsig; i++)
	    if (strcmp(trim_whitespace(s[i].desc), "ABP") == 0 ||
		strcmp(s[i].desc, "ART") == 0 ||
		strcmp(s[i].desc, "BP") == 0)
		break;
	if (i == nsig) {
	    fprintf(stderr, "%s: no ABP signal specified; use -s option\n\n",
		    pname);
	    help();
	    exit(3);
	}
	sig = i;
    }
    
    if (vflag)
	fprintf(stderr, "%s: analyzing signal %d (%s)\n",
		pname, sig, s[sig].desc);
    sps = sampfreq((char *)NULL);
    if (Rflag)
    	setifreq(sps = 125.); 
    if (from > 0L) {
	if ((from = strtim(argv[from])) < 0L)
	    from = -from;
    }
    if (to > 0L) {
	if ((to = strtim(argv[to])) < 0L)
	    to = -to;
    }

    annot.subtyp = annot.num = 0;
    annot.chan = sig;
    annot.aux = NULL;
    Tm = physadu((unsigned)sig, Tm);
    samplingInterval = 1000.0/sps;
    spm = 60 * sps;
    next_minute = from + spm;
    EyeClosing = sps * EYE_CLS;   /* set eye-closing period */
    ExpectPeriod = sps * NDP;	  /* maximum expected RR interval */
    SLPwindow = sps * SLPW;       /* slope window size */

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
	printf("Minimum threshold:       %d\n", Tm);
	printf("\n------------------------------------------------------\n\n");
	printf("Processing:\n");
    }

    (void)sample(sig, 0L);
    if (dflag) {
	for (t = from; (to == 0L || t < to) && sample_valid(); t++)
	    printf("%6d\t%6d\n", sample(sig, t), slpsamp(t));
	exit(0);
    }

    /* Average the first 8 seconds of the slope  samples
       to determine the initial thresholds Ta and T0 */
    t1 = from + strtim("8");
    for (T0 = 0, t = from; t < t1 && sample_valid(); t++)
	T0 += slpsamp(t);
    T0 /= t1 - from;
    Ta = 3 * T0;

    /* Main loop */
    for (t = from; (to == 0L || t < to) && sample_valid(); t++) {
	static int learning = 1, T1;
	
	if (learning) {
	    if (t > from + LPERIOD) {
		learning = 0;
		T1 = T0;
		t = from;	/* start over */
	    }
	    else T1 = 2*T0;
	}
	
	if (slpsamp(t) > T1) {   /* found a possible ABP pulse near t */ 
	    timer = 0; 
            /* used for counting the time after previous ABP pulse */
	    max = min = slpsamp(t);
	    for (tt = t+1; tt < t + EyeClosing/2; tt++)
		if (slpsamp(tt) > max) max = slpsamp(tt);
	    for (tt = t-1; tt > t - EyeClosing/2; tt--)
		if (slpsamp(tt) < min) min = slpsamp(tt);
	    if (max > min+10) { 
		onset = max/100 + 2;
		tpq = t - 5;
		for (tt = t; tt > t - EyeClosing/2; tt--) {
		    if (slpsamp(tt) - slpsamp(tt-1) < onset) {
		      tpq = tt;  
			break;
		    }
		}

		if (!learning) {
		    /* Check that we haven't reached the end of the record. */
		    (void)sample(sig, tpq);
		    if (sample_valid() == 0) break;
		    /* Record an annotation at the ABP pulse onset */
		    annot.time = tpq;
		    annot.anntyp = NORMAL;
		    if (putann(0, &annot) < 0) { /* write the annotation */
			wfdbquit();	/* close files if an error occurred */
			exit(1);
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
	    /* Once past the learning period, decrease threshold if no pulse
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
		(void)fprintf(stderr, "\n");
		minutes = 0;
	    }
	}
    }

    (void)free(lbuf);
    (void)free(ebuf);
    wfdbquit();		        /* close WFDB files */
    fprintf(stderr, "\n");
    if (vflag) {
	printf("\n\nDone! \n\nResulting annotation file:  %s.wabp\n\n\n",
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
 " -R          resample input at 125 Hz (default: do not resample)",
" -s SIGNAL   analyze specified signal (default: first ABP, ART, or BP signal)",
 " -t TIME     stop at specified time (default: end of the record)",
 " -v          verbose mode",
 " ",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}

