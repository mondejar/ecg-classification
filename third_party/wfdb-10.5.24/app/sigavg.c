/* file: sigavg.c		G. Moody		25 November 2002

-------------------------------------------------------------------------------
sigavg: Calculate averages of annotated waveforms
Copyright (C) 2002 George B. Moody

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

This program is based on Example 9 in the WFDB Programmer's Guide.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

char *irec;	/* input record name */
char *pname;	/* name by which this program was invoked */
double sps;	/* sampling frequency (Hz) */
double **sum;
int dt1 = 0, dt2 = 0, flag[ACMAX+1], Hflag, nsig, vflag = 0, zflag = 0;
WFDB_Anninfo ai;
WFDB_Sample *v, *vb;
WFDB_Siginfo *s;
WFDB_Time from = 0L, to = 0L;

char *prog_name();
void help(), init(), memerr();

main(argc, argv)
int argc;
char *argv[];
{
    int i, j, nbeats = 0;
    WFDB_Annotation annot;

    init(argc, argv);	/* read and interpret command line */

    while (getann(0, &annot) == 0 && annot.time < from)
        ;
    do {
        if (flag[annot.anntyp] == 0) continue;
        isigsettime(annot.time + dt1 - 1);
        getvec(vb);
        for (j = dt1; j <= dt2  && getvec(v) > 0; j++) {
	    if (zflag)
		for (i = 0; i < nsig; i++)
		    sum[i][j-dt1] += v[i] - vb[i];
	    else
		for (i = 0; i < nsig; i++)
		    sum[i][j-dt1] += v[i];
	}
        nbeats++;
    } while (getann(0, &annot) == 0 &&
             (to == 0L || annot.time < to));
    if (nbeats < 1) {
        fprintf(stderr, "%s: no beats found\n", pname);
        exit(4);
    }
    if (vflag) {
	printf("# Average of %d beats:\n", nbeats);
	printf("#     Time\t");
	for (i = 0; i < nsig; i++)
	    printf("%10s%c", s[i].desc, (i == nsig-1) ? '\n' : '\t');
	printf("#      sec\t");
	for (i = 0; i < nsig; i++) {
	    if (s[i].units == NULL) s[i].units = "mV";
	    printf("%10s%c", s[i].units, (i == nsig-1) ? '\n' : '\t');
	}
    }
    for (j = dt1; j < dt2; j++) {
	printf("%10.5lf\t", (double)j/sps);
        for (i = 0; i < nsig; i++) {
	    double m;

	    m = sum[i][j-dt1]/nbeats;
	    if (!zflag) m -= s[i].baseline;
	    m /= s[i].gain;
            printf("%10.5lf%c", m, (i == nsig-1) ? '\n' : '\t');
	}
    }
    exit(0);
}

void init(int argc, char **argv)
{
    int i, j;

    pname = prog_name(argv[0]);
    for (i = 1; i < ACMAX; i++)
	flag[i] = isqrs(i);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    ai.stat = WFDB_READ;
	    break;
	  case 'd':
	    if (++i >= argc-1) {
		(void)fprintf(stderr, "%s: window offsets must follow -w\n",
			      pname);
		exit(1);
	    }
	    /* save arg list indices, convert argument to samples later */
	    dt1 = i;
	    dt2 = ++i;
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
	  case 'H':   /* read multifrequency records in high resolution mode */
	    Hflag = 1;
	    break;
	  case 'p':	/* annotation mnemonic(s) follow */
	    for (j = 1; j < ACMAX; j++)
		flag[j] = 0;
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
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    irec = argv[i];
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		exit(1);
	    }
	    to = i;
	    break;
	  case 'v':	/* verbose mode */
	    vflag = 1;
	    break;
	  case 'z':	/* set the baseline to zero */
	    zflag = 1;
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
    if (irec == NULL || ai.name == NULL) {
	help();
	exit(1);
    }
    if (Hflag)
	setgvmode(WFDB_HIGHRES);
    if ((nsig = isigopen(irec, NULL, 0)) <= 0)
	exit(2);
    s = malloc((size_t)nsig * sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)malloc((size_t)nsig * sizeof(WFDB_Sample));
    vb = (WFDB_Sample *)malloc((size_t)nsig * sizeof(WFDB_Sample));
    sum = (double **)malloc(nsig * sizeof(double *));
    if (s == NULL || v == NULL || vb == NULL || sum == NULL)
	memerr();
    if (wfdbinit(irec, &ai, 1, s, nsig) != nsig)
	exit(2);
    sps = sampfreq(NULL);
    dt1 = strtim(dt1 ? argv[dt1] : "-.05");
    dt2 = strtim(dt2 ? argv[dt2] : ".05");
    if (dt1 > dt2) {
	int tmp;
	tmp = dt1; dt1 = dt2; dt2 = tmp;
    }
    else if (dt1 == dt2)
	dt2 += strtim(".1");
    for (i = 0; i < nsig; i++)
        if ((sum[i]=(double *)calloc(dt2-dt1+1,sizeof(double))) == NULL) {
	    wfdbquit();
	    memerr();
        }
    if (from > 0L) {
	from = strtim(argv[from]);
	if (from < (WFDB_Time)0) from = -from;
	(void)isigsettime(from);
    }
    if (to > 0L) {
	to = strtim(argv[to]);
	if (to < (WFDB_Time)0) to = -to;
    }
}

void memerr()
{
    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
    exit(2);
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
 " -d DT1 DT2          calculate the average over a window defined by offsets",
 "                      DT1 and DT2 from the input annotations (defaults:",
 "                      DT1 = -0.05, DT2 = 0.05 (seconds from annotations)",
 " -f TIME             begin at specified time (default: beginning of RECORD)",
 " -h                  print this usage summary",
 " -H                  read multifrequency records in high resolution mode",
 " -p TYPE [TYPE ...]  include annotations of specified TYPEs only (default:",
 "                      include all QRS annotations",
 " -t TIME             stop at specified time (default: end of RECORD)",
 " -v                  verbose mode",
 " -z                  set the baseline to zero before averaging",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
