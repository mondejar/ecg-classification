/* file: sigamp.c	G. Moody	30 November 1991
			Last revised:	13 November 2009

-------------------------------------------------------------------------------
sigamp: Measure signal amplitudes
Copyright (C) 1991-2009 George B. Moody

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
#include <math.h>
#include <wfdb/wfdb.h>
#define isqrs
#define map2
#define ammap
#define mamap
#define annpos
#include <wfdb/ecgmap.h>

#define NMAX 300
#define _STRING(X) #X
#define STRING(X) _STRING(X)

/* values for timeunits */
#define SECONDS     1
#define MINUTES     2
#define HOURS       3
#define TIMSTR      4
#define MSTIMSTR    5
#define SHORTTIMSTR 6
#define HHMMSS	    7
#define SAMPLES     8

char *pname;
int namp;
int nsig;
int pflag;		/* if non-zero, print physical units */
int qflag;		/* if non-zero, suppress output of trimmed means */
int timeunits = SECONDS;
int vflag;		/* if non-zero, print individual measurements */
int *v0, *vmax, *vmin, *vv;
double **amp, *vmean, *vsum;
long dt1, dt2, dtw;
WFDB_Anninfo ai;
WFDB_Frequency sfreq;
WFDB_Siginfo *si;
WFDB_Time from = 0L, to = 0L, t;

main(int argc, char **argv)
{
    char *p, *record = NULL, *prog_name(char *s);
    int gvmode = 0, i, j, jlow, jhigh, nmax = NMAX,
	ampcmp(), getptp(WFDB_Time t), getrms(WFDB_Time t);
    void help(void), printamp(WFDB_Time t);

    /* Interpret command-line arguments. */
    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch(*(argv[i]+1)) {
	  case 'a':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'd':
	    if (++i >= argc-1) {
		(void)fprintf(stderr, "%s: DT1 and DT2 must follow -d\n",
			      pname);
		exit(1);
	    }
	    /* save arg list index, convert arguments to samples later (after
	       record has been opened and sampling frequency is known) */
	    dt1 = i++;
	    break;
	  case 'f':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: start time must follow -f\n",
			      pname);
		exit(1);
	    }
	    /* save arg list index, convert argument to samples later */
	    from = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* operate in WFDB_HIGHRES mode */
	    gvmode = WFDB_HIGHRES;
	    break;
	  case 'n':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: number of measurements must follow -n\n",
			      pname);
		exit(1);
	    }
	    if ((nmax = atoi(argv[i])) < 1) nmax = NMAX;
	    break;
	  case 'p':
	    pflag = 1;
	    if (*(argv[i]+2) == 'd') timeunits = TIMSTR;
	    else if (*(argv[i]+2) == 'e') timeunits = HHMMSS;
	    else if (*(argv[i]+2) == 'h') timeunits = HOURS;
	    else if (*(argv[i]+2) == 'm') timeunits = MINUTES;
	    else if (*(argv[i]+2) == 'S') timeunits = SAMPLES;
	    else timeunits = SECONDS;
	    break;
          case 'q':
	    qflag = 1;
	    break;
	  case 'r':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 't':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: stop time must follow -t\n",
			      pname);
		exit(1);
	    }
	    /* save arg list index, convert argument to samples later */
	    to = i;
	    break;
	  case 'v':
	    vflag = 1;
	    break;
	  case 'w':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: window size must follow -w\n",
			      pname);
		exit(1);
	    }
	    /* save arg list index, convert argument to samples later */
	    dtw = i;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n",
			  pname, argv[i]);
	    exit(1);
	    break;
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n",
			  pname, argv[i]);
	    exit(1);
	    break;
	}
    }
    if (record == NULL) {
	help();
	exit(1);
    }

    /* Finish initialization. */
    if (gvmode == 0 && (p = getenv("WFDBGVMODE")))
	gvmode = atoi(p);
    setgvmode(gvmode|WFDB_GVPAD);

    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(v0 = malloc(nsig * sizeof(int))) == NULL ||
	(vmax = malloc(nsig * sizeof(int))) == NULL ||
	(vmin = malloc(nsig * sizeof(int))) == NULL ||
	(vv = malloc(nsig * sizeof(int))) == NULL ||
	(amp = malloc(nsig * sizeof(double *))) == NULL ||
	(vmean = malloc(nsig * sizeof(double))) == NULL ||
	(vsum = malloc(nsig * sizeof(double))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(3);
    }
    if (isigopen(record, si, nsig) != nsig) exit(2);
    for (i = 0; i < nsig; i++) {
	if (si[i].gain == 0.0) si[i].gain = WFDB_DEFGAIN;
	if ((amp[i] = malloc(nmax * sizeof(double))) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(3);
	}
    }
    if ((sfreq = sampfreq(NULL)) <= 0.0)
	sfreq = WFDB_DEFFREQ;

    /* Adjust timeunits if starting time is undefined. */
    if (timeunits == TIMSTR || timeunits == HHMMSS) {
	char *p = timstr(0L);
	if (*p != '[') timeunits = HHMMSS;
    }
    if (timeunits == HOURS) sfreq *= 3600.;
    else if (timeunits == MINUTES) sfreq *= 60.;

    if (from > 0L) from = strtim(argv[(int)from]);
    if (to > 0L) to = strtim(argv[(int)to]);
    if (from < 0L) from = -from;
    if (to < 0L) to = -to;
    if (from >= to) to = strtim("e");

    /* Collect amplitudes, print them if qflag or vflag set. */
    if (ai.name) {	/* windows are relative to annotations */
	WFDB_Annotation annot;

	if (dtw > 0L) {
	    (void)fprintf(stderr,
			  "%s: -a and -w options cannot be used together;\n",
			  pname);
	    (void)fprintf(stderr, "  -w option is being ignored\n");
	}
	if (annopen(record, &ai, 1) < 0) exit(2);
	if (dt1 == 0L) dt1 = -(dt2 = strtim("0.05"));
	else {
	    char *p = argv[(int)dt1+1];

	    if (*p == '-') dt2 = -strtim(p+1);
	    else dt2 = strtim(p);
	    p = argv[(int)dt1];
	    if (*p == '-') dt1 = strtim(p+1);
	    else dt1 = -strtim(p);
	    if (dt1 > dt2) {
		long temp = dt1;
		
		dt1 = dt2; dt2 = temp;
	    }
	}
	if (dt1 == dt2) dt2++;
	if (iannsettime(from) < 0) exit(2);
	while (getann(0, &annot) == 0 && namp < nmax &&
	       (to == 0L || annot.time < to)) {
	    if (map1(annot.anntyp) == NORMAL) {
		if (getptp(annot.time) < 0) break;
		if (vflag || qflag) printamp(annot.time);
		namp++;
	    }
	}
    }
    else {		/* windows are consecutive nonoverlapping segments */
	if (dt1 > 0L) {
	    (void)fprintf(stderr,
			  "%s: -d option must be used with -a;\n", pname);
	    (void)fprintf(stderr, "  -d option is being ignored\n");
	}
	if (dtw == 0L || (dtw = strtim(argv[(int)dtw])) <= 0L)
	    dtw = strtim("1");
	if (from > 0L && isigsettime(from) < 0L) exit(2);
	for (t = from; namp < nmax && (to == 0L || t < to); namp++, t += dtw) {
	    if (getrms(t) < 0) break;
	    if (vflag || qflag) printamp(t);
	}	
    }

    /* Calculate trimmed means of collected amplitudes unless -q option set. */
    if (qflag == 0) {
	jlow = namp/20;
	jhigh = namp - jlow;
	if (vflag)
	    (void)printf("Trimmed mean");
	for (i = 0; i < nsig; i++) {
	    double a;

	    qsort((char*)amp[i], namp, sizeof(double), ampcmp);
	    for (a = 0.0, j = jlow; j < jhigh; j++)
		a += amp[i][j];
	    a /= jhigh - jlow;
	    (void)printf("\t%g", pflag ? a/si[i].gain : a);
	}
	(void)printf("\n");
    }
    exit(0);
}

int getrms(WFDB_Time t)
{
    int i, v;
    long tt;

    if (getvec(vv) < nsig) return (-1);

    for (i = 0; i < nsig; i++) {
	vmean[i] = v0[i] = vv[i];
	vsum[i] = 0.0;
    }

    for (tt = 1; tt < dtw; tt++) {
	if (getvec(vv) < nsig) return (-1);
	for (i = 0; i < nsig; i++)
	    vmean[i] += vv[i];
    }

    if (isigsettime(t) < 0 || getvec(vv) < nsig) return (-1);

    /* The quantity vv[i] - v0[i] is normally zero, but may be non-zero if
       the signals are stored in difference format (since isigsettime may
       introduce a DC error in this case).  In the calculation of vmean[i]
       below, this quantity is used to correct any such error. */
    for (i = 0; i < nsig; i++) {
	vmean[i] = vmean[i]/dtw + vv[i] - v0[i];
	v = vv[i] - vmean[i];
	vsum[i] += (double)v*v;
    }

    for (tt = 1; tt < dtw; tt++) {
	if (getvec(vv) < nsig) return (-1);
	for (i = 0; i < nsig; i++) {
	    v = vv[i] - vmean[i];
	    vsum[i] += (double)v*v;
	}
    }

    for (i = 0; i < nsig; i++)
	amp[i][namp] = sqrt(vsum[i]/dtw);

    return (0);
}

int getptp(WFDB_Time t)
{
    int i;
    long tt;

    if (isigsettime(t + dt1) < 0) return (-1);

    if (getvec(vmin) < nsig) return (-1);

    for (i = 0; i < nsig; i++)
	vmax[i] = vmin[i];

    for (tt = dt1; tt < dt2; tt++) {
	if (getvec(vv) < nsig) return (-1);
	for (i = 0; i < nsig; i++) {
	    if (vv[i] < vmin[i]) vmin[i] = vv[i];
	    else if (vv[i] > vmax[i]) vmax[i] = vv[i];
	}
    }

    for (i = 0; i < nsig; i++)
	amp[i][namp] = vmax[i] - vmin[i];

    return (0);
}

void printamp(WFDB_Time t)
{
    int i;

    switch (timeunits) {
      case TIMSTR:  printf("%s",mstimstr(-t)); break;
      case HHMMSS:    printf("%s", mstimstr(t)); break;
      case SAMPLES:   printf("%ld", t); break;
      default:	    printf("%lf", t/sfreq); break;
    }
    for (i = 0; i < nsig; i++)
	(void)printf("\t%g", pflag ? amp[i][namp]/si[i].gain :
		     amp[i][namp]);
    (void)printf("\n");
}

int ampcmp(double *p1, double *p2)
{
    if (*p1 > *p2) return (1);
    else if (*p1 == *p2) return (0);
    else return (-1);
}

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

static char *help_strings[] = {
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where OPTIONS may include any of:",
 " -a ANN      specify annotator, measure amplitudes near QRS annotations",
 " -d DT1 DT2  set measurement window relative to QRS annotations",
 "              defaults: DT1 = 0.05 (seconds before annotation);",
 "              DT2 = 0.05 (seconds after annotation)",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -n NMAX     make up to NMAX measurements per signal (default: NMAX = "
     STRING(NMAX) ")",
 " -p          print results in physical units (default: ADC units)",
 "              -p may be followed by a character to choose a time format:",
 "  -pd         print time of day and date if known",
 "  -pe         print elapsed time as <hours>:<minutes>:<seconds>",
 "  -ph         print elapsed time in hours",
 "  -pm         print elapsed time in minutes",
 "  -ps         print elapsed time in seconds (default)",
 "  -pS         print elapsed time in sample intervals",
 " -q          quick mode: print individual measurements only",
 " -t TIME     stop at specified time",
 " -v          verbose mode: print individual measurements and trimmed means",
 " -w DTW      set RMS amplitude measurement window",
 "              default: DTW = 1 (second)",
NULL
};

void help(void)
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
