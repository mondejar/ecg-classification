/* file rdann.c	    T. Baker and G. Moody	27 July 1981
		    Last revised:	         4 June 2010

-------------------------------------------------------------------------------
rdann: Print an annotation file in ASCII form
Copyright (C) 1981-2010 George B. Moody

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

Caution: If the output format of 'rdann' is modified, 'wrann' will require
modification as well!
*/

#include <stdio.h>
#ifndef BSD
#include <string.h>
#else
#include <strings.h>
#define strchr index
#endif
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
    char *record = NULL, *prog_name();
    signed char cflag = 0, chanmatch, nflag = 0, nummatch, sflag = 0, submatch;
    double sps, spm, sph;
    int eflag = 0, i, j, vflag = 0, xflag = 0;
    long afrom = 0L, anum = 0L, ato = 0L, bfrom = 0L, bnum = 0L, bto = 0L,
	from = 0L, to = 0L, atol();
    static char flag[ACMAX+1];
    static WFDB_Anninfo ai;
    WFDB_Annotation annot;
    void help();

    pname = prog_name(argv[0]);

    /* Accept old syntax. */
    if (argc >= 3 && argv[1][0] != '-') {
	ai.name = argv[1];
	record = argv[2];
	i = 3;
	if (argc > 3) { from = 3; i = 4; }
	if (argc > 4) { to = 4; i = 5; }
	if (argc <= 5) flag[0] = 1;
	else while (i < argc && argv[i][0] != '-') {
	    if (isann(j = strann(argv[i]))) flag[j] = 1;
	    i++;
	}
    }
    else
	i = flag[0] = 1;

    /* Interpret command-line options. */
    for ( ; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'c':	/* chan value follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, 
		      "%s: chan value (between -128 and 127) must follow -c\n",
			      pname);
		exit(1);
	    }
	    chanmatch = atoi(argv[i]);
	    cflag = 1;
	    break;
	  case 'e':	/* show elapsed time */
	    eflag = 1;
	    xflag = 0;
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
	  case 'n':	/* num value follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, 
		      "%s: num value (between -128 and 127) must follow -n\n",
			      pname);
		exit(1);
	    }
	    nummatch = atoi(argv[i]);
	    nflag = 1;
	    break;
	  case 'p':	/* annotation mnemonic(s) follow */
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
	  case 's':	/* subtyp value follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, 
		    "%s: subtyp value (between -128 and 127) must follow -s\n",
			      pname);
		exit(1);
	    }
	    submatch = atoi(argv[i]);
	    sflag = 1;
	    break;
	  case 't':	/* ending time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    to = i;
	    break;
	  case 'v':	/* show column headings */
	    vflag = 1;
	    break;
	  case 'x':	/* use alternate time format */
	    xflag = 1;
	    eflag = 0;
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

    if (from) {
	if (*argv[(int)from] == 'a') {
	    if ((afrom = atol(argv[(int)from]+1)) < 0L)
		afrom = from = 0L;
	    while (anum < afrom && getann(0, &annot) == 0) {
		anum++;
		if (isqrs(annot.anntyp)) bnum++;
	    }
	    if (anum < afrom) exit(2);
	}
	else if (*argv[(int)from] == 'b' || *argv[(int)from] == '#') {
	    if ((bfrom = atol(argv[(int)from]+1)) < 0L)
		bfrom = from = 0L;
	    while (bnum < bfrom && getann(0, &annot) == 0) {
		anum++;
		if (isqrs(annot.anntyp)) bnum++;
	    }
	    if (bnum < bfrom) exit(2);
	}
	else if (iannsettime(strtim(argv[(int)from])) < 0)
	    exit(2);
    }
    if (to) {
	if (*argv[(int)to] == 'a') {
	    if ((ato = atol(argv[(int)to]+1)) <  0L) ato = 0L;
	    bto = to = (WFDB_Time)0;
	}
	else if (*argv[(int)to] == 'b') {
	    if ((bto = atol(argv[(int)to]+1)) <  0L) bto = 0L;
	    ato = to = (WFDB_Time)0;
	}
	else if (*argv[(int)to] == '#') {
	    if ((bto = atol(argv[(int)to]+1)) <  1L) bto = 1L;
	    bto += bnum;
	    ato = to = (WFDB_Time)0;
	}
	else {
	    ato = bto = 0L;
	    to = strtim(argv[(int)to]);
	    if (to < (WFDB_Time)0) to = -to;
	}
    }

    if (vflag) {	/* print column headings */
      if (eflag)
	(void)printf("Elapsed time  Sample #  ");
      else if (xflag)
	(void)printf("  Seconds   Minutes     Hours  ");
      else if (strchr(mstimstr((WFDB_Time)(-1)), '/'))
	  (void)printf("      Time       Date     Sample #  ");
      else if (*(mstimstr((WFDB_Time)(-1))) == '[')
	  (void)printf("      Time     Sample #  ");
      else
	  (void)printf("      Time   Sample #  ");
      (void)printf("Type  Sub Chan  Num\tAux\n");
    }

    while (getann(0, &annot) == 0 && (to == 0L || annot.time <= to)) {
	if ((flag[0] || (isann(annot.anntyp) && flag[annot.anntyp])) &&
	    (cflag == 0 || annot.chan == chanmatch) &&
	    (nflag == 0 || annot.num == nummatch) &&
	    (sflag == 0 || annot.subtyp == submatch)) {
	    if (eflag)
		(void)printf("%s  %7ld", mstimstr(annot.time), annot.time);
	    else if (xflag)
		(void)printf("%9.3lf %9.5lf %9.7lf",
			     annot.time/sps, annot.time/spm, annot.time/sph);
	    else
		(void)printf("%s  %7ld", mstimstr(-annot.time), annot.time);
	    (void)printf("%6s%5d%5d%5d", annstr(annot.anntyp), annot.subtyp,
		annot.chan, annot.num);
	    if (annot.aux != NULL) (void)printf("\t%s", annot.aux + 1);
	    (void)printf("\n");
	}
	if (ato && ++anum >= ato) break;
	if (bto && isqrs(annot.anntyp) && ++bnum >= bto) break;
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
 " -c CHAN             print annotations with specified CHAN only",
 " -e                  * show annotation times as elapsed times",
 " -f TIME             start at specified TIME",
 " -h                  print this usage summary",
 " -n NUM              print annotations with specified NUM only",
 " -p TYPE [TYPE ...]  print annotations of specified TYPEs only",
 " -s SUBTYPE          print annotations with specified SUBTYPE only",
 " -t TIME             stop at specified TIME",
 " -v                  print column headings",
 " -x                  * use alternate time format (seconds, minutes, hours)",
 "* Only one of -e and -x can be used.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
