/* file ihr.c		G. Moody      12 November 1992
			Last revised:   5 March 2004

-------------------------------------------------------------------------------
ihr: Generate instantaneous heart rate data from annotation file
Copyright (C) 1992-2004 George B. Moody

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
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>
#define map2
#define ammap
#define mamap
#define annpos
#include <wfdb/ecgmap.h>

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *record = NULL, *prog_name();
    double dmhr,  ihr, ihrlast, mhr = 70.0, sph, spm, sps, tol = 10.0,
	atof(), fabs();
    int i, j, lastann = NOTQRS, last2ann = NOTQRS, tformat = 1, vflag = 1,
	xflag = 0, lastint = 1, thisint = 0;
    long from = 0L, to = 0L, lasttime = -9999L;
    static char flag[ACMAX+1];
    static WFDB_Anninfo ai;
    WFDB_Annotation annot;
    void help();

    pname = prog_name(argv[0]);
    flag[0] = 1;

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'd':	/* tolerance (max HR change beat to beat, in bpm) */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: tolerance must follow -d\n",
			      pname);
		exit(1);
	    }
	    tol = atof(argv[i]);
	    break;
	  case 'f':	/* starting time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: starting time must follow -f\n",
			      pname);
		exit(1);
	    }
	    from = i;		/* to be converted to sample intervals below */
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* include intervals bounded by any QRS annotations */
	    for (j = 0; j <= ACMAX; j++)
		flag[j] = isqrs(j);
	    break;
	  case 'p':	/* include intervals bounded by specific annotations
			   only; annotation mnemonic(s) follow */
	    if (++i >= argc || !isann(j = strann(argv[i]))) {
		(void)fprintf(stderr,
			      "%s: annotation mnemonic(s) must follow -p\n",
			      pname);
		exit(1);
	    }
	    flag[j] = 1;
	    /* The code above not only checks that there is a mnemonic where
	       there should be one, but also allows for the possibility that
	       there might be a (user-defined) mnemonic beginning with `-'.
	       The following lines pick up any other mnemonics, but assume
	       that arguments beginning with `-' are options, not mnemonics. */
	    while (++i < argc && argv[i][0] != '-')
		if (isann(j = strann(argv[i]))) flag[j] = 1;
	    if (i == argc || argv[i][0] == '-') i--;
	    flag[0] = 0;
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
	  case 'V':	/* output times of ends of intervals */
	      vflag = -1;
	      /* no 'break': fall through case 'v' */
	  case 'v':	/* output times of beginnings of intervals */
	    switch (*(argv[i]+2)) {
	      case 'h': tformat = 3; break;	/* use hours */
	      case 'm': tformat = 2; break;	/* use minutes */
	      case 's': tformat = 1; break;	/* use seconds */
	      default:  tformat = 0; break;	/* use sample intervals */
	    }
	    break;
	  case 'x':	/* exclude intervals following those adjacent to
			   excluded beats */
	    xflag = 1;
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
    if (record == NULL || ai.name == NULL) {
	help();
	exit(1);
    }

    if ((sps = sampfreq(record)) < 0.)
	(void)setsampfreq(sps = WFDB_DEFFREQ);
    spm = 60.0*sps;
    sph = 60.0*spm;

    ai.stat = WFDB_READ;
    if (annopen(record, &ai, 1) < 0) /* open annotation file */
	exit(2);

    if (from && iannsettime(strtim(argv[(int)from])) < 0) exit(2);
    if (to) {
        to = strtim(argv[(int)to]);
	if (to < (WFDB_Time)0) to = -to;
    }

    if (flag[0])    /* neither -i nor -p used -- include only normal beats */
	for (j = 0; j <= ACMAX; j++)
	    flag[j] = (map1(j) == NORMAL);

    while (getann(0, &annot) == 0 && (to == 0L || annot.time <= to)) {
	if (flag[annot.anntyp]) {
	    ihr = sps*60./(annot.time - lasttime);
	    dmhr = (ihr - mhr)/10.;
	    /* The next two lines of code were added in March 2004.  They limit
	       the magnitude of dmhr (the increment to be applied to the
	       predictor mhr) in order to limit the influence on mhr of any
	       single observation.  This helps to keep mhr reasonably close to
	       the recent mean heart rate even when the input contains gross
	       QRS detection errors.  Given error-free input and a reasonable
	       value for tol, these lines have no significant effect. */
	    if (dmhr > tol) dmhr = tol;
	    else if (dmhr < -tol) dmhr = -tol;
	    mhr += dmhr;
	    if (flag[lastann] && fabs(ihr-ihrlast)<tol && fabs(ihr-mhr)<tol) {
		if (flag[last2ann] || !xflag) {
		    long tt = (vflag > 0) ? lasttime : annot.time;
		    switch (tformat) {
		      case 0: (void)printf("%ld\t", tt); break;
		      default:
		      case 1: (void)printf("%.3lf\t", tt/sps); break;
		      case 2: (void)printf("%.5lf\t", tt/spm); break;
		      case 3: (void)printf("%.7lf\t", tt/sph); break;
		    }
		    if (xflag) (void)printf("%g\n", ihr);
		    else (void)printf("%g\t%d\n", ihr, lastint);
		    thisint = 0;
		}
	    }
	    ihrlast = ihr;
	}
	else if (!isqrs(annot.anntyp)) continue;
	last2ann = lastann;
	lastann = annot.anntyp;
	lasttime = annot.time;
	lastint = thisint;
	thisint = 1;
    }
    exit(0);			/*NOTREACHED*/
}

char *prog_name(s)
char *s;
{
    char *p = s + strlen(s);

#ifdef MSDOS
    while (p >= s && *p != '\\' && *p != ':') {
	if (*p == '.')
	    *p = '\0';			/* strip off extension */
	if ('A' <= *p && *p <= 'Z')
	    *p += 'a' - 'A';		/* convert to lower case */
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
 " -d TOL   reject beat-to-beat HR changes > TOL bpm (default: TOL = 10)",
 " -f TIME  start at specified TIME",
 " -h       print this usage summary",
 " -i       include intervals bounded by any QRS annotations",
 " -p TYPE [ TYPE ... ]  include intervals bounded by annotations of listed",
 "                        TYPEs only",
 " -t TIME  stop at specified TIME",
 " -x       exclude intervals adjacent to abnormal beats",
 "Each line of output contains data derived from a single interbeat interval:",
 "  * Elapsed time (in seconds) from the beginning of the record to the",
 "    beginning of the interval (may be modified by -v or -V options below)",
 "  * Instantaneous heart rate (in beats per minute)",
 "  * Interval type (1 if the interval was bounded by normal beats, otherwise",
 "     0 (this column does not appear in the output if the -x option is used)",
 "Use one of the following options to modify the format of the first column:",
 " -v       print times of beginnings of intervals as sample numbers",
 " -vh      same as -v, but print times in hours",
 " -vm      same as -v, but print times in minutes",
 " -vs      same as -v, but print times in seconds [default]",
 " -V       print times of ends of intervals as sample numbers",
 " -Vh      same as -V, but print times in hours",
 " -Vm      same as -V, but print times in minutes",
 " -Vs      same as -V, but print times in seconds",
    NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
