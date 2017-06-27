/* file: sqrs.c		G. Moody	27 October 1990
			Last revised:    9 April 2010

-------------------------------------------------------------------------------
sqrs: Single-channel QRS detector
Copyright (C) 1990-2010 George B. Moody

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

The detector algorithm is based on example 10 in the WFDB Programmer's
Guide, which in turn is based on a Pascal program written by W.A.H. Engelse
and C. Zeelenberg, "A single scan algorithm for QRS-detection and feature
extraction", Computers in Cardiology 6:37-42 (1979).  `sqrs' does not include
the feature extraction capability of the Pascal program.  The output of `sqrs'
is an annotation file (with annotator name `qrs') in which all detected beats
are labelled normal;  the annotation file may also contain `artifact'
annotations at locations which `sqrs' believes are noise-corrupted.

`sqrs' can process records containing any number of signals, but it uses only
one signal for QRS detection (signal 0 by default;  this can be changed using
the `-s' option, see below).  'sqrs' has been optimized for adult human ECGs.
For other ECGs, it may be necessary to experiment with the input sampling
frequency and the time constants indicated below.

This program is provided as an example only, and is not intended for any
clinical application.  At the time the algorithm was originally published,
its performance was typical of state-of-the-art QRS detectors.  Recent designs,
particularly those that can analyze two or more input signals, may exhibit
significantly better performance.

Usage:
    sqrs -r RECORD [ OPTIONS ]
  where RECORD is the record name, and OPTIONS may include:
    -f TIME		to specify the starting TIME (default: the beginning of
			the record)
    -m THRESHOLD	to specify the detection THRESHOLD (default: 500);
			use higher values to reduce false detections, or lower
			values to reduce the number of missed beats
    -s SIGNAL		to specify the SIGNAL to be used for QRS detection
			(default: 0)
    -t TIME		to specify the ending TIME (default: the end of the
			record)

For example, to mark QRS complexes in record 100 beginning 5 minutes from the
start, ending 10 minutes and 35 seconds from the start, and using signal 1, use
the command:
    sqrs -r 100 -f 5:0 -t 10:35 -s 1
The output may be read using (for example):
    rdann -a qrs -r 100
To evaluate the performance of this program, run it on the entire record, by:
    sqrs -r 100
and then compare its output with the reference annotations by:
    bxb -r 100 -a atr qrs
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

#define abs(A)	((A) >= 0 ? (A) : -(A))

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *p, *record = NULL, *prog_name();
    int filter, i, minutes = 0, nsig, time = 0,
        slopecrit, sign, maxslope = 0, nslope = 0,
        qtime, maxtime, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9,
        ms160, ms200, s2, scmax, scmin = 500, signal = -1, *v;
    long from = 0L, next_minute, now, spm, to = 0L;
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    static int gvmode = WFDB_LOWRES;
    static WFDB_Siginfo *s;
    void help();

    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
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
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: threshold must follow -m\n", pname);
		exit(1);
	    }
	    scmin = atoi(argv[i]);
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: signal number or name must follow -s\n",
			      pname);
		exit(1);
	    }
	    signal = i;
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		exit(1);
	    }
	    to = i;
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
    if ((s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(v = malloc(nsig * sizeof(WFDB_Sample))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, s, nsig)) < 1) exit(2);
    if (sampfreq((char *)NULL) < 50.) {
	(void)fprintf(stderr, "%s: sampling frequency (%g Hz) is too low%s",
		      pname, sampfreq((char *)NULL),
		      gvmode & WFDB_HIGHRES ? "\n" : ", try -H option\n");
	exit(3);
    }
    if (sampfreq((char *)NULL) < 240. || sampfreq((char *)NULL) > 260.) 
	setifreq(250.);

    a.name = "qrs"; a.stat = WFDB_WRITE;
    if (annopen(record, &a, 1) < 0) exit(2);

    if (from > 0L) {
	if ((from = strtim(argv[from])) < 0L)
	    from = -from;
	if (isigsettime(from) < 0)
	    exit(2);
    }
    if (to > 0L) {
	if ((to = strtim(argv[to])) < 0L)
	    to = -to;
    }
    spm = strtim("1:0");
    next_minute = from + spm;
    if (signal >= 0) signal = findsig(argv[signal]);
    if (signal < 0 || signal >= nsig) signal = 0;
    scmin = muvadu((unsigned)signal, scmin);
    if (scmin < 1) scmin = muvadu((unsigned)signal, 500);
    slopecrit = scmax = 10 * scmin;
    now = from;

    /* These time constants may need adjustment for pediatric or small
       mammal ECGs. */
    ms160 = strtim("0.16"); ms200 = strtim("0.2"); s2 = strtim("2");

    annot.subtyp = annot.chan = annot.num = 0; annot.aux = NULL;
    (void)getvec(v);
    t9 = t8 = t7 = t6 = t5 = t4 = t3 = t2 = t1 = v[signal];

    do {
        filter = (t0 = v[signal]) + 4*t1 + 6*t2 + 4*t3 + t4
                - t5         - 4*t6 - 6*t7 - 4*t8 - t9;
        if (time % s2 == 0) {
            if (nslope == 0) {
                slopecrit -= slopecrit >> 4;
                if (slopecrit < scmin) slopecrit = scmin;
            }
            else if (nslope >= 5) {
                slopecrit += slopecrit >> 4;
                if (slopecrit > scmax) slopecrit = scmax;
            }
        }
        if (nslope == 0 && abs(filter) > slopecrit) {
            nslope = 1; maxtime = ms160;
            sign = (filter > 0) ? 1 : -1;
            qtime = time;
        }
        if (nslope != 0) {
            if (filter * sign < -slopecrit) {
                sign = -sign;
                maxtime = (++nslope > 4) ? ms200 : ms160;
            }
            else if (filter * sign > slopecrit &&
                     abs(filter) > maxslope)
                maxslope = abs(filter);
            if (maxtime-- < 0) {
                if (2 <= nslope && nslope <= 4) {
                    slopecrit += ((maxslope>>2) - slopecrit) >> 3;
                    if (slopecrit < scmin) slopecrit = scmin;
                    else if (slopecrit > scmax) slopecrit = scmax;
                    annot.time = now - (time - qtime) - 4;
                    annot.anntyp = NORMAL; (void)putann(0, &annot);
                    time = 0;
                }
                else if (nslope >= 5) {
                    annot.time = now - (time - qtime) - 4;
                    annot.anntyp = ARFCT; (void)putann(0, &annot);
                }
                nslope = 0;
            }
        }
        t9 = t8; t8 = t7; t7 = t6; t6 = t5; t5 = t4;
        t4 = t3; t3 = t2; t2 = t1; t1 = t0; time++; now++;
	if (now >= next_minute) {
	    next_minute += spm;
	    (void)fprintf(stderr, ".");
	    (void)fflush(stderr);
	    if (++minutes >= 60) {
		(void)fprintf(stderr, " %s\n", timstr(now));
		minutes = 0;
	    }
	}
    } while (getvec(v) > 0 && (to == 0L || now <= to));
    if (minutes) (void)fprintf(stderr, " %s\n", timstr(now));
    wfdbquit();
    exit(0);	/*NOTREACHED*/
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
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -m THRESH   set detector threshold to THRESH (default: 500)",
 " -s SIGNAL   analyze specified signal (default: 0)",
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
