/* file: stepdet.c		G. Moody	28 February 2014

-------------------------------------------------------------------------------
stepann: detect and annotate step changes in a signal
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

This program analyzes one signal of a PhysioBank-compatible record, detecting
and annotating rising and falling step changes.

TUP is the threshold for detecting a rising step change (annotated as 'R'),
and TDOWN is the threshold for detecting a falling ('F') step change.  This
program requires that TUP > TDOWN.  Using its -m option, set TUP to a value
significantly greater than TDOWN to avoid false detections of transitions due
to noise in the signal.  Noise spikes that still cause false detections can
often be avoided by median-filtering the signal (see mfilt(1)) before using it
as input to stepdet.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *p, *record = NULL, *prog_name();
    int i, minutes = 0, nsig, signal = -1, time, tdown = 450, tup = 550, *v,
	v0, v1;
    long from = 0L, next_minute, now, spm, to = 0L;
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    static int gvmode = WFDB_LOWRES;
    static WFDB_Siginfo *s;
    void help();

    pname = prog_name(argv[0]);
    a.name = "steps"; a.stat = WFDB_WRITE;

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -a\n",
			      pname);
		exit(1);
	    }
	    a.name = argv[i];
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
	    if (++i >= argc-1) {
		(void)fprintf(stderr, "%s: TUP and TDOWN must follow -m\n",
			      pname);
		exit(1);
	    }
	    tup = atoi(argv[i++]);
	    tdown = atoi(argv[i]);
	    if (tup <= tdown) {
		(void)fprintf(stderr, "%s: TUP (%d) must be greater than TDOWN"
			      " (%d)\n", pname, tup, tdown);
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
    now = from;

    annot.subtyp = annot.chan = annot.num = 0; annot.aux = NULL;
    (void)getvec(v);
    v1 = v[signal];
    ++now;

    while (getvec(v) > 0 && (to == 0L || now <= to)) {
	v0 = v1;
	v1 = v[signal];
	if (v0 < v1 && v0 < tup && v1 >= tup) {
	    annot.anntyp = RBBB;
	    annot.time = now;
	    putann(0, &annot);
	}
	else if (v0 > v1 && v0 > tdown && v1 <= tdown) {
	    annot.anntyp = FUSION;
	    annot.time = now;
	    putann(0, &annot);
	}
	v0 = v1;
	if (++now >= next_minute) {
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
 "usage: %s -r RECORD [ OPTIONS ]\n",
 "where RECORD is the record name, and OPTIONS may include:",
 " -a ANNOTATOR  specify output ANNOTATOR name (default: steps)",
 " -f TIME       begin at specified TIME",
 " -h            show (this) help",
 " -H            analyze multifrequency input in high-resolution mode",
 " -m TUP TDOWN	 set thresholds for transitions from low to high (TUP,",
 "                default: 550) and from high to low (TDOWN, default: 450)",
 " -s SIGNAL     specify the SIGNAL to be analyzed (default: 0)",
 " -t TIME       stop at specified TIME",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
