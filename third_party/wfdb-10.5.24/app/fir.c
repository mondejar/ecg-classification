/* file: fir.c		G. Moody	5 January 1987
			Last revised:  25 February 2006

-------------------------------------------------------------------------------
fir: General-purpose FIR filter for database records
Copyright (C) 1987-2006 George B. Moody

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

char *pname;	/* name by which this program was invoked */
char *nrec;	/* name of record to be created */
double *c;	/* pointer to array of filter coefficients */
int flen;	/* number of coefficients (filter length) */
int nsig;	/* number of signals to be filtered */
int ri, ro;	/* rectify input/output if non-zero */
int **vin;	/* pointers to input vectors */
int *vout;	/* pointer to output vector */
long nsamp;	/* number of samples to be processed */

char *prog_name();
void help(), init(), memerr();

main(argc, argv)
int argc;
char *argv[];
{
    int f, i = 0, j, s;
    void init();

    init(argc, argv);	/* read and interpret command line */
    while ((nsamp == -1L || nsamp-- > 0L) && getvec(vin[i]) >= 0) {
	if (ri) {
	    for (s = 0; s < nsig; s++)
		if (vin[i][s] < 0) vin[i][s] = -vin[i][s];
	}
	if (++i >= flen)
	    i = 0;
	for (s = 0, j = i; s < nsig; s++)
	    for (f = vout[s] = 0; f < flen; f++) {
		vout[s] += c[f]*vin[j][s];
		if (++j >= flen) j = 0;
	    }
	if (ro) {
	    for (s = 0; s < nsig; s++)
		if (vout[s] < 0) vout[s] = -vout[s];
	}
	if (putvec(vout) < 0) break;
    }
    if (nrec) (void)newheader(nrec);
    wfdbquit();
    exit(0);	/*NOTREACHED*/
}

void init(argc, argv)
int argc;
char *argv[];
{
    char *irec = "16", *orec = "16", *p;
    double *tc = NULL, atof();
    int i, n = 128, s;
    long from = 0L, shift = 0L, to = 0L;
    static int gvmode = 0;
    static WFDB_Siginfo *chin, *chout;
    FILE *ifile;

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':	/* filter coefficients follow */
	    flen = argc - (++i);
	    if ((c=tc=(double *)calloc((unsigned)flen,sizeof(double))) == NULL)
		memerr();
	    while (i < argc)
		*tc++ = atof(argv[i++]);
	    break;
	  case 'C':	/* filter coefficients are in a file */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: file name must follow -C\n", pname);
		exit(1);
	    }
	    if ((ifile = fopen(argv[i], "r")) == NULL) {
		(void)fprintf(stderr, "%s: can't open %s\n", pname, argv[i]);
		exit(1);
	    }
	    if ((c = (double *)calloc((unsigned)n,sizeof(double))) == NULL)
		memerr();
	    while (fscanf(ifile, "%lf", &c[flen]) == 1)
		if (++flen >= n) {
		    n += 128;
		    c=(double *)realloc((char *)c,(unsigned)n*sizeof(double));
		    if (c == NULL)
			memerr();
		}
	    (void)fclose(ifile);
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
	  case 'i':	/* input record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -i\n",
			      pname);
		exit(1);
	    }
	    irec = argv[i];
	    break;
	  case 'n':	/* new record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -n\n",
			      pname);
		exit(1);
	    }
	    if (strlen(nrec = argv[i]) > WFDB_MAXRNL) {
		(void)fprintf(stderr, "%s: new record name too long\n", pname);
		exit(1);
	    }
	    break;
	  case 'o':	/* output record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -o\n",
			      pname);
		exit(1);
	    }
	    orec = argv[i];
	    break;
	  case 'r':	/* rectify */
	    if (*(argv[i]+2) == 'i') ri = 1;
	    else if (*(argv[i]+2) == 'o') ro = 1;
	    else {
		(void)fprintf(stderr,  "%s: unrecognized option %s\n",
			      pname, argv[i]);
		exit(1);
	    }
	    break;
	  case 's':	/* phase shift */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -s\n", pname);
		exit(1);
	    }
	    shift = i;
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
    if (flen < 1) {
	help();
	exit(1);
    }

    if (gvmode == 0 && (p = getenv("WFDBGVMODE")))
	gvmode = atoi(p);
    setgvmode(gvmode|WFDB_GVPAD);

    if ((nsig = isigopen(irec, NULL, 0)) <= 0)
	exit(2);
    if ((chin = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(chout = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	memerr();
    }
    if (isigopen(irec, chin, nsig) != nsig)
	exit(2);
    if (nrec) {
	static char ofname[WFDB_MAXRNL+5];

	sprintf(ofname, "%s.dat", nrec);
	for (i = 0; i < nsig; i++) {
	    chout[i] = chin[i];
	    chout[i].fname = ofname;
	    chout[i].group = 0;
	}
	if ((osigfopen(chout, nsig) < nsig)) {
	    wfdbquit();
	    exit(1);
	}
    }
    else if (osigopen(orec, chout, (unsigned)nsig) <= 0) {
	wfdbquit();
	exit(2);
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
    nsamp = (to > 0L) ? to - from : -1L;
    if ((vout = (int *)calloc((unsigned)nsig, sizeof(int))) == NULL ||
	(vin = (int **)calloc((unsigned)flen, sizeof(int *))) == NULL)
	memerr();
    for (i = 0; i < flen; i++)
	if ((vin[i] = (int *)calloc((unsigned)nsig, sizeof(int))) == NULL)
	    memerr();
    if (shift > 0) {
	shift = strtim(argv[shift]);
	i = flen - (shift % flen);
	while (shift-- > 0) {
	    if (i >= flen) i = 0;
	    (void)getvec(vin[i]);
	    if (ri) {
		for (s = 0; s < nsig; s++)
		    if (vin[i][s] < 0) vin[i][s] = -vin[i][s];
	    }
	    i++;
	}
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
 "usage: %s [OPTIONS ...]\n",
 "where OPTIONS may include any of:",
 " -c A1 [A2 ...]  filter using coefficients A1, A2, ... (must be the last",
 "              option; -c marks the beginning of the coefficient list)",
 " -C file     filter using coefficients read from the specified FILE",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -i IREC     read signals from record IREC (default: 16)",
 " -n NREC     create a header file, using record name NREC and signal",
 "              descriptions from IREC",
 " -o OREC     produce output signal file(s) as specified by the header file",
 "              for record OREC (default: 16)",
 " -ri         rectify the input signals before filtering",
 " -ro         rectify the filtered output",
 " -s TIME     correct for phase shift by reading from the input for the",
 "              specified TIME before starting to produce output samples",
 " -t TIME     stop at specified time",
 "One of `-c' or `-C' *must* be used to specify filter coefficients.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
