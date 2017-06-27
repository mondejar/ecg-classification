/* file: ann2rr.c		G. Moody	 16 May 1995
				Last revised:  23 February 2009
-------------------------------------------------------------------------------
ann2rr: Calculate RR intervals from an annotation file
Copyright (C) 1995-2009 George B. Moody

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
#define map1
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
    char *record = NULL, rrfstr[8], t0fstr[8], t1fstr[8], *prog_name();
    double sps, spm, sph;
    int Aflag = 0, a0, a0flag = 0, a1, a1flag = 0,
	cflag = 0, i,  j, pflag = 0, previous_annot_valid = 0, rrdigits = 3,
	rrformat = 0, t0digits = 3, t0flag = 0, t0format = 0, t1digits = 3,
	t1flag = 0, t1format = 0;
    long atol();
    static char a0f[ACMAX+1], a1f[ACMAX+1];
    static WFDB_Anninfo ai;
    static WFDB_Annotation annot;
    static WFDB_Time from, to, rr, t0, t1;
    void help();

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = a0f[0] = a1f[0] = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'A':	/* print intervals between annotations of all types */
	    Aflag = 1;
	    break;
	  case 'c':    	/* print intervals between consecutive valid
			   annotations only */
	    cflag = 1;
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
	  case 'i':	/* specify interval output format */
	    if (++i >= argc || *(argv[i]) == '-') {
		(void)fprintf(stderr,
			      "%s: interval output format must follow -i\n",
			      pname);
		exit(1);
	    }
	    rrformat = *(argv[i]);
	    sscanf(argv[i]+1, "%d", &rrdigits);
	    break;
	  case 'p':	/* annotation mnemonic(s) follow */
	    if (++i >= argc || !isann(j = strann(argv[i]))) {
		(void)fprintf(stderr,
			      "%s: annotation mnemonic(s) must follow -P\n",
			      pname);
		exit(1);
	    }
	    a1f[j] = 1;
	    /* The code above not only checks that there is a mnemonic where
	       there should be one, but also allows for the possibility that
	       there might be a (user-defined) mnemonic beginning with `-'.
	       The following lines pick up any other mnemonics, but assume
	       that arguments beginning with `-' are options, not mnemonics. */
	    while (++i < argc && argv[i][0] != '-')
		if (isann(j = strann(argv[i]))) a1f[j] = 1;
	    if (i == argc || argv[i][0] == '-') i--;
	    a1f[0] = 0;
	    break;
	  case 'P':	/* annotation mnemonic(s) follow */
	    if (++i >= argc || !isann(j = strann(argv[i]))) {
		(void)fprintf(stderr,
			      "%s: annotation mnemonic(s) must follow -p\n",
			      pname);
		exit(1);
	    }
	    a0f[j] = 1;
	    while (++i < argc && argv[i][0] != '-')
		if (isann(j = strann(argv[i]))) a0f[j] = 1;
	    if (i == argc || argv[i][0] == '-') i--;
	    a0f[0] = 0;
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
	  case 'v':	/* output times of ends of intervals */
	    t1flag = 1;
	    if (++i >= argc || argv[i][0] == '-')
		i--;
	    else {
		t1format = *(argv[i]);
		sscanf(argv[i]+1, "%d", &t1digits);
	    }
	    break;
	  case 'V':	/* output times of beginnings of intervals */
	    t0flag = 1;
	    if (++i >= argc || argv[i][0] == '-')
		i--;
	    else {
		t0format = *(argv[i]);
		sscanf(argv[i]+1, "%d", &t0digits);
	    }
	    break;
	  case 'w':	/* output annotation types following intervals */
	    a1flag = 1;
	    break;
	  case 'W':     /* output annotation types preceeding intervals */
	    a0flag = 1;
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
    if (annopen(record, &ai, 1) < 0)	/* open annotation file */
	exit(2);

    if (rrdigits < 0 || rrdigits > 15) rrdigits = 3;
    sprintf(rrfstr, "%%.%dlf", rrdigits);
    if (t0digits < 0 || t0digits > 15) t0digits = 3;
    sprintf(t0fstr, "%%.%dlf", t0digits);
    if (t1digits < 0 || t1digits > 15) t1digits = 3;
    sprintf(t1fstr, "%%.%dlf", t1digits);
    
    if (from) {
	from = strtim(argv[(int)from]);
	if (from < (WFDB_Time)0) from = -from;
    }
    if (to) {
	to = strtim(argv[(int)to]);
	if (to < (WFDB_Time)0) to = -to;
    }
    if (to > (WFDB_Time)0 && to < from) {
	WFDB_Time tt = from;

	from = to;
	to = tt;
    }
    if (from > (WFDB_Time)0 && iannsettime(from) < 0)
	exit(2);

    a0 = NOTQRS;
    t0 = from;

    while (getann(0, &annot) == 0 && (to == (WFDB_Time)0 || annot.time <= to)){
	a1 = annot.anntyp;
	t1 = annot.time;

	/* Does t1 mark a valid interval end point? */
	if (Aflag || (a1f[0] && isqrs(a1)) || a1f[a1]) {
	    /* Does t0 mark a valid interval starting point? */
	    if (cflag == 0 || previous_annot_valid == 1) {
		/* If requested, print time at beginning of interval. */
		if (t0flag) {
		    switch (t0format) {
		      case 'h': (void)printf(t0fstr, t0/sph); break;
		      case 'm': (void)printf(t0fstr, t0/spm); break;
		      case 's': (void)printf(t0fstr, t0/sps); break;
		      case 't': if (t0 == (WFDB_Time)0)
			           (void)printf("    0:00.000");
		                else
				    (void)printf("%s", mstimstr(t0));
		                break;
		      case 'T': (void)printf("%s", mstimstr(-t0)); break;
		      default:  (void)printf("%ld", t0); break;
		    }
		    (void)printf("\t");
		}

		/* If requested, print annotation at beginning of interval. */
		if (a0flag) {
		    if (a0 == NOTQRS) (void)printf("[0]\t");
		    else (void)printf("%s\t", annstr(a0));
		}

		/* Print the interval, and update t0 and a0. */
		rr = t1 - t0;
		switch (rrformat) {
	          char rrstr[20];
		  double frr;

		  case 'h': (void)sprintf(rrstr, rrfstr, rr/sph);
		      (void)printf("%s", rrstr);
		      (void)sscanf(rrstr, "%lf", &frr);
		      t1 = (long)(t0 + frr * sph + 0.5);
		      break;
		  case 'm': (void)sprintf(rrstr, rrfstr, rr/spm);
		      (void)printf("%s", rrstr);
		      (void)sscanf(rrstr, "%lf", &frr);
		      t1 = (long)(t0 + frr * spm + 0.5);
		      break;
		  case 's': (void)sprintf(rrstr, rrfstr, rr/sps);
		      (void)printf("%s", rrstr);
		      (void)sscanf(rrstr, "%lf", &frr);
		      t1 = (long)(t0 + frr * sps + 0.5);
		      break;
		  case 't': (void)printf("%s", mstimstr(t1)); break;
		  default:  (void)printf("%ld", rr); break;
		}

		/* If requested, print annotation at end of interval. */
		if (a1flag)
		    (void)printf("\t%s", annstr(a1));

		/* If requested, print time at end of interval. */
		if (t1flag) {
		    (void)printf("\t");
		    switch (t1format) {
		      case 'h': (void)printf(t1fstr, t1/sph); break;
		      case 'm': (void)printf(t1fstr, t1/spm); break;
		      case 's': (void)printf(t1fstr, t1/sps); break;
		      case 't': (void)printf("%s", mstimstr(t1)); break;
		      case 'T': (void)printf("%s", mstimstr(-t1)); break;
		      default:  (void)printf("%ld", t1); break;
		    }
		}

		(void)printf("\n");
	    }
	}
	/* Does t1 mark a valid interval starting point? */
	if (Aflag || (a0f[0] && isqrs(a1)) || a0f[a1]) {
	    a0 = a1;
	    t0 = t1;
	    previous_annot_valid = 1;
	}
	else if (cflag)
	    previous_annot_valid = 0;
    }
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
 "usage: %s -r RECORD -a ANNOTATOR [OPTIONS ...]\n",
 "where RECORD and ANNOTATOR specify the input, and OPTIONS may include:",
 " -A      print all intervals between annotations (default: print only RR",
 "          intervals; overrides -c, -p)",
 " -c      print intervals between consecutive valid annotations only",
 " -f TIME start at specified TIME",
 " -h      print this usage summary",
 " -i FMT  print intervals using format FMT (see below for values of FMT)",
 " -p TYPE [TYPE ...]  print intervals ending with annotations of specified",
 "                      TYPEs only (use mnemonics such as N or V for TYPE)",
 " -P TYPE [TYPE ...]  print intervals beginning with specified types only",
 " -t TIME stop at specified TIME",
 " -v FMT  print times of ends of intervals using format FMT (see below)",
 " -V FMT  print times of beginnings of intervals using format FMT (see below)",
 " -w      print annotations that end intervals",
 " -W      print annotations that begin intervals",
 "By default, the output contains the RR intervals only, unless one or more",
 "of -v, -V, -w, or -W are used.  Intervals and times are printed in units of",
 "sample intervals, unless a format is specified using -i, -v, or -V.",
 "Formats can be 'h' (hours), 'm' (minutes), 's' (seconds), 't' (hh:mm:ss);",
 "when used with -v or -V, format 'T' yields dates and times if available,",
 "or the format can be omitted to obtain times in sample intervals.  Formats",
 "'h', 'm', and 's' may be followed by a number between 0 and 15, specifying",
 "the number of decimal places (default: 3).  For example, to obtain intervals",
 "in seconds with 8 decimal places, use '-i s8'.",
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
