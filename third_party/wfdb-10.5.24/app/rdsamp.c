/* file: rdsamp.c	G. Moody	 23 June 1983
			Last revised:	6 October 2010

-------------------------------------------------------------------------------
rdsamp: Print an arbitrary number of samples from each signal
Copyright (C) 1983-2010 George B. Moody

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
#include <wfdb/wfdb.h>

/* values for timeunits */
#define SECONDS     1
#define MINUTES     2
#define HOURS       3
#define TIMSTR      4
#define MSTIMSTR    5
#define SHORTTIMSTR 6
#define HHMMSS	    7
#define SAMPLES     8

#define WFDBXMLPROLOG  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
 "<?xml-stylesheet type=\"text/xsl\"" \
 " href=\"wfdb.xsl\"?>\n" \
 "<!DOCTYPE wfdbsampleset PUBLIC \"-//PhysioNet//DTD WFDB 1.0//EN\"" \
 " \"http://physionet.org/physiobank/database/XML/wfdb.dtd\">\n"

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *record = NULL, *search = NULL, *escapify(), *prog_name();
    char *invalid, *snfmt, *tfmt, *tnfmt, *tufmt, *vfmt, speriod[16], tustr[16];
    int cflag = 0, highres = 0, i, isiglist, nsig, nosig = 0, pflag = 0, s,
	*sig = NULL, timeunits = SECONDS, vflag = 0, xflag = 0;
    WFDB_Frequency freq;
    WFDB_Sample *v;
    WFDB_Siginfo *si;
    WFDB_Time from = 0L, maxl = 0L, to = 0L;
    void help();

    pname = prog_name(argv[0]);
    for (i = 1 ; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':	/* output in CSV format */
	    cflag = 1;
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
	  case 'H':	/* select high-resolution mode */
	    highres = 1;
	    break;
	  case 'l':	/* maximum length of output follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: max output length must follow -l\n",
			      pname);
		exit(1);
	    }
	    maxl = i;
	    break;
	  case 'P':	/* output in high-precision physical units */
	    ++pflag;	/* (fall through to case 'p') */
	  case 'p':	/* output in physical units specified */
	    ++pflag;
	    if (*(argv[i]+2) == 'd') timeunits = TIMSTR;
	    else if (*(argv[i]+2) == 'e') timeunits = HHMMSS;
	    else if (*(argv[i]+2) == 'h') timeunits = HOURS;
	    else if (*(argv[i]+2) == 'm') timeunits = MINUTES;
	    else if (*(argv[i]+2) == 'S') timeunits = SAMPLES;
	    else timeunits = SECONDS;
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal list follows */
	    isiglist = i+1; /* index of first argument containing a signal # */
	    while (i+1 < argc && *argv[i+1] != '-') {
		i++;
		nosig++;	/* number of elements in signal list */
	    }
	    if (nosig == 0) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
	    break;
	  case 'S':	/* search for valid sample of specified signal */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: signal name or number must follow -S\n",
			      pname);
		exit(1);
	    }
	    search = argv[i];
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		exit(1);
	    }
	    to = i;
	    break;
	  case 'v':	/* verbose output -- include column headings */
	    vflag = 1;
	    break;
	  case 'X':	/* output in WFDB-XML format */
	    xflag = cflag = vflag = 1; /* format is CSV inside an XML wrapper */
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
    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((v = malloc(nsig * sizeof(WFDB_Sample))) == NULL ||
	(si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, si, nsig)) <= 0)
	exit(2);
    for (i = 0; i < nsig; i++)
	if (si[i].gain == 0.0) si[i].gain = WFDB_DEFGAIN;
    if (highres)
        setgvmode(WFDB_HIGHRES);
    freq = sampfreq(NULL);
    if (from > 0L && (from = strtim(argv[from])) < 0L)
	from = -from;
    if (isigsettime(from) < 0)
	exit(2);
    if (to > 0L && (to = strtim(argv[to])) < 0L)
	to = -to;
    if (nosig) {		/* print samples only from specified signals */
	if ((sig = (int *)malloc((unsigned)nosig*sizeof(int))) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
	for (i = 0; i < nosig; i++) {
	    if ((s = findsig(argv[isiglist+i])) < 0) {
		(void)fprintf(stderr, "%s: can't read signal '%s'\n", pname,
			      argv[isiglist+i]);
		exit(2);
	    }
	    sig[i] = s;
	}
	nsig = nosig;
    }
    else {			/* print samples from all signals */
	if ((sig = (int *)malloc((unsigned)nsig*sizeof(int))) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
	for (i = 0; i < nsig; i++)
	    sig[i] = i;
    }

    /* Reset 'from' if a search was requested. */
    if (search &&
	((s = findsig(search)) < 0 || (from = tnextvec(s, from)) < 0)) {
	(void)fprintf(stderr, "%s: can't read signal '%s'\n", pname, search);
	exit(2);
    }

    /* Reset 'to' if a duration limit was specified. */
    if (maxl > 0L && (maxl = strtim(argv[maxl])) < 0L)
	maxl = -maxl;
    if (maxl && (to == 0L || to > from + maxl))
	to = from + maxl;

    /* Adjust timeunits if starting time or date is undefined. */
    if (timeunits == TIMSTR) {
	char *p = timstr(0L);
	if (*p != '[') timeunits = HHMMSS;
	else if (strlen(p) < 16) timeunits = SHORTTIMSTR;
	else if (freq > 1.0) timeunits = MSTIMSTR;
    }
    if (timeunits == HOURS) freq *= 3600.;
    else if (timeunits == MINUTES) freq *= 60.;

    /* Set formats for output. */
    if (cflag) {  /* CSV output selected */
	snfmt = ",'%s'";
	if (pflag) { 	/* output in physical units */
	    switch (timeunits) {
	      case SAMPLES:     tnfmt = "'sample interval'";
		                sprintf(tustr, "'%g sec'", 1./freq);
				tufmt = tustr; break;
	      case SHORTTIMSTR: tnfmt = "'Time'";
		                tufmt = "'hh:mm:ss.mmm'"; break;
	      case TIMSTR:      tnfmt = "'Time and date'";
		                tufmt = "'hh:mm:ss dd/mm/yyyy'"; break;
	      case MSTIMSTR:    tnfmt = "'Time and date'";
		                tufmt = "'hh:mm:ss.mmm dd/mm/yyyy'"; break;
	      case HHMMSS:      tnfmt = "'Elapsed time'";
		                tufmt = "'hh:mm:ss.mmm'"; break;
	      case HOURS:       tnfmt = "'Elapsed time'";
		                tufmt = "'hours'"; break;
	      case MINUTES:     tnfmt = "'Elapsed time'";
		                tufmt = "'minutes'"; break;
	      default:
	      case SECONDS:     tnfmt = "'Elapsed time'";
		                tufmt = "'seconds'"; break;
	    }
	    invalid = ",-";
	    if (pflag > 1)	/* output in high-precision physical units */
		vfmt = ",%.8lf";
	    else
		vfmt = ",%.3lf";
	}
	else {	/* output in raw units */
	    tnfmt = "'sample #'";
	    tfmt = "%ld";
	    vfmt = ",%d";
	}
    }
    else {	/* output in tab-separated columns selected */
	if (pflag) {	/* output in physical units */
	    switch (timeunits) {
	      case SAMPLES:     tnfmt = "sample interval";
		                sprintf(speriod, "(%g", 1./freq);
                                speriod[10] = '\0';
		                sprintf(tustr, "%10s sec)", speriod);
				tufmt = tustr; break;
	      case SHORTTIMSTR: tnfmt = "     Time";
		                tufmt = "(hh:mm:ss.mmm)"; break;
	      case TIMSTR:	tnfmt = "   Time      Date    ";
		                tufmt = "(hh:mm:ss dd/mm/yyyy)"; break;
	      case MSTIMSTR:    tnfmt = "      Time      Date    ";
		                tufmt = "(hh:mm:ss.mmm dd/mm/yyyy)"; break;
	      case HHMMSS:      tnfmt = "   Elapsed time";
		                tufmt = "   hh:mm:ss.mmm"; break;
	      case HOURS:       tnfmt = "   Elapsed time";
		                tufmt = "        (hours)"; break;
	      case MINUTES:     tnfmt = "   Elapsed time";
		                tufmt = "      (minutes)"; break;
	      default:
	      case SECONDS:     tnfmt = "   Elapsed time";
		                tufmt = "      (seconds)"; break;
	    }
	    if (pflag > 1) {	/* output in high-precision physical units */
		snfmt = "\t%15s";
		invalid = "\t              -";
		vfmt = "\t%15.8lf";
	    }
	    else {
		snfmt = "\t%7s";
		invalid = "\t      -";
		vfmt = "\t%7.3lf";
	    }
	}
	else {	/* output in raw units */
	    snfmt = "\t%7s";
	    tnfmt = "       sample #";
	    tfmt = "%15ld";
	    vfmt = "\t%7d";
	}
    }

    /* Print WFDB-XML prolog if '-x' option selected. */
    if (xflag) {
	printf(WFDBXMLPROLOG);
	printf("<wfdbsampleset>\n"
	       "<samplingfrequency>%g</samplingfrequency>\n"
	       "<signals>%d</signals>\n<description>",
	       freq, nsig);
    }
    /* Print column headers if '-v' option selected. */
    if (vflag) {
	char *p, *t;
	int j, l;

	(void)printf("%s", tnfmt);

	for (i = 0; i < nsig; i++) {
	    /* Check if a default signal description was provided by looking
	       for the string ", signal " in the desc field.  If so, replace it
	       with a shorter string. */
	    p = si[sig[i]].desc;
	    if (strstr(p, ", signal ")) {
		char *t;
		if (t = malloc(10*sizeof(char))) {
		    (void)sprintf(t, "sig %d", sig[i]);
		    p = t;
		}
	    }
	    if (cflag == 0) {
		l = strlen(p);
		if (pflag > 1) {
		    if (l > 15) p += l - 15;
		}
		else {
		    if (l > 7) p+= l - 7;
		}
	    }
	    else
		p = escapify(p);
	    (void)printf(snfmt, p);
	}
	if (xflag) (void)printf("</description>");
	(void)printf("\n");
    }

    /* Print data in physical units if '-p' option selected. */
    if (pflag) {
	char *p;


	/* Print units as a second line of column headers if '-v' selected. */
	if (vflag) {
	    char s[12];

	    if (xflag) (void)printf("<units>");
	    (void)printf("%s", tufmt);

	    for (i = 0; i < nsig; i++) {
		p = si[sig[i]].units;
		if (p == NULL) p = "mV";
		if (cflag == 0) {
		    char ustring[16];
		    int len;
	
		    len = strlen(p);
		    if (pflag > 1) { if (len > 13) len = 13; }
		    else if (len > 5) len = 5;
		    ustring[0] = '(';
		    strncpy(ustring+1, p, len);
		    ustring[len+1] = '\0';
		    (void)printf(pflag > 1 ? "\t%14s)" : "\t%6s)", ustring);    
		}
		else {
		    p = escapify(p);
		    (void)printf(",'%s'", p);
		}
	    }
	    if (xflag) (void)printf("</units>");
	    (void)printf("\n");
	}

	if (xflag) (void)printf("<samplevectors>\n", nsig+1);
	while ((to == 0L || from < to) && getvec(v) >= 0) {
	    if (cflag == 0) {
	      switch (timeunits) {
	        case TIMSTR:   (void)printf("%s", timstr(-from)); break;
	        case SHORTTIMSTR:
	        case MSTIMSTR: (void)printf("%s", mstimstr(-from)); break;
	        case HHMMSS:   (void)printf("%15s", from == 0L ?
					    "0:00.000" : mstimstr(from)); break;
	        case SAMPLES:  (void)printf("%15ld", from); break;
	        default:
	        case SECONDS:  (void)printf("%15.3lf",(double)from/freq); break;
	        case MINUTES:  (void)printf("%15.5lf",(double)from/freq); break;
	        case HOURS:    (void)printf("%15.7lf",(double)from/freq); break;
	      }
	    }
	    else {
	      switch (timeunits) {
	        case TIMSTR:
		    for (p = timstr(-from); *p == ' '; p++)
			;
		    (void)printf("'%s'", p); break;
	        case SHORTTIMSTR:
	        case MSTIMSTR:
		    for (p = mstimstr(-from); *p == ' '; p++)
			;
		    (void)printf("'%s'", p); break;
	        case HHMMSS:
		    if (from == 0L) printf("'0:00.000'");
		    else {
			for (p = mstimstr(from); *p == ' '; p++)
			    ;
			(void)printf("'%s'", p); break;
		    }
		    break;
	        case SAMPLES:  (void)printf("%ld", from); break;
	        default:
	        case SECONDS:  (void)printf("%.3lf",(double)from/freq); break;
	        case MINUTES:  (void)printf("%.5lf",(double)from/freq); break;
	        case HOURS:    (void)printf("%.7lf",(double)from/freq); break;
	      }
	    }

	    from++;
	    for (i = 0; i < nsig; i++) {
		if (v[sig[i]] != WFDB_INVALID_SAMPLE)
		    (void)printf(vfmt,
		     ((double)v[sig[i]] - si[sig[i]].baseline)/si[sig[i]].gain);
		else
		    (void)printf("%s", invalid);
	    }
	    (void)printf("\n");
	}
    }

    else {	/* output in raw units */
	if (xflag) (void)printf("<samplevectors>\n", nsig+1);
	while ((to == 0L || from < to) && getvec(v) >= 0) {
	    (void)printf(tfmt, from++);
	    for (i = 0; i < nsig; i++)
		(void)printf(vfmt, v[sig[i]]);
	    (void)printf("\n");
	}
    }

    if (xflag)		/* print trailer if WFDB-XML output was selected */
	printf("</samplevectors>\n</wfdbsampleset>\n");

    exit(0);
}

char *escapify(char *s)
{
    char *p = s, *q = s, *r;
    int c = 0;

    while (*p) {
	if (*p == '\'' || *p == '\\')
	    c++;
	p++;
    }
    if (c > 0 && (p = r = calloc(p-s+c, sizeof(char))) != NULL) {
	while (*q) {
	    if (*q == '\'' || *q == '\\')
		*q++ = '\\';
	    *p++ = *q;
	}
	q = r;
    }
    return (q);
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
 "where RECORD is the name of the input record, and OPTIONS may include:",
 " -c          use CSV (comma-separated value) output format",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -l INTERVAL truncate output after the specified time interval (hh:mm:ss)",
 " -p          print times and samples in physical units (default: raw units)",
 " -P          same as -p, but with greater precision",
 "              -p and -P may be followed by a character to choose a time",
 "              format;  choices are:",
 "  -pd (or -Pd)  print time of day and date if known",
 "  -pe (or -Pe)  print elapsed time as <hours>:<minutes>:<seconds>",
 "  -ph (or -Ph)  print elapsed time in hours",
 "  -pm (or -Pm)  print elapsed time in minutes",
 "  -ps (or -Ps)  print elapsed time in seconds",
 "  -pS (or -PS)  print elapsed time in sample intervals",
 " -s SIGNAL [SIGNAL ...]  print only the specified signal(s)",
 " -S SIGNAL   search for a valid sample of the specified SIGNAL at or after",
 "		the time specified with -f, and begin printing then",
 " -t TIME     stop at specified time",
 " -v          print column headings",
 " -X          output in WFDB-XML format",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
