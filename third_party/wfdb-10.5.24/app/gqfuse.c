/* file: gqfuse.c		G. Moody	6 May 2012
				Last revised:  25 September 2012
-------------------------------------------------------------------------------
gqfuse: combine QRS annotation files
Copyright (C) 2012 George B. Moody

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

This program produces a QRS annotation file based on two or more input QRS
annotation files A1, A2, ....  Each one-minute segment of the output annotation
file is a copy of the corresponding segment of one of the input annotation
files.  In each segment, the program copies the input that best matches a
predicted heart rate.  If there are N inputs, the prediction is the median of
N+1 values (the previous prediction and the number of beats marked within the
current segment of each of the N input files).  Although this process allows
the input to be switched once per minute, the policy for resolving ties (within
2 beats) favors not switching if the previously chosen input is one of those
belonging to the tie.

One way to use this program is to combine input annotation files for each
available ECG signal in a record, made using a single detector such as gqrs.
Another is to combine input annotation files made using a variety of QRS
detectors.  These ideas can be combined as desired.
*/

#include <stdio.h>
#include <stddef.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

/* The `getconf' macro is used to check a line of input (already in `buf') for
   the string named by getconf's first argument.  If the string is found, the
   value following the string (and an optional `:' or `=') is converted using
   sscanf and the format specifier supplied as getconf's second argument, and
   stored in the variable named by the first argument. */
#define getconf(a, fmt)	if (p=strstr(buf,#a)){sscanf(p,#a "%*[=: \t]" fmt,&a);}

void help(void);
char *prog_name(char *p);
void *gcalloc(size_t nmemb, size_t size);
void cleanup(int status);

char *pname;			/* name of this program, used in messages */
char *record = NULL;		/* name of input record */

int icomp(const void *a, const void *b)
{
    if (*(int *)a > *(int *)b) return -1;
    if (*(int *)a < *(int *)b) return 1;
    return 0;
}

main(int argc, char **argv)
{
    char *oaname = "gqf";
    double HR;
    FILE *config = NULL;
    int a0 = 0, a1 = 0, **count, dn, i, ibest, ichosen = 0, j, *mbuf,
	n, niann = 0, nseg, tdn;
    WFDB_Anninfo *a;
    WFDB_Annotation annot;
    WFDB_Frequency spm;
    WFDB_Time t0, tf;

    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* input annotator names */
	      for (a0 = a1 = ++i; a1 < argc && *argv[a1] != '-'; a1++, i++)
		;
	    if ((niann = a1 - a0) < 2) {
		(void)fprintf(stderr,
			      "%s: input annotator names must follow -a\n",
			      pname);
		cleanup(1);
	    }
	    break;
	  case 'c':	/* configuration file */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		     "%s: name of configuration file must follow -c\n", pname);
		cleanup(1);
	    }
	    if ((config = fopen(argv[i], "rt")) == NULL) {
		(void)fprintf(stderr,
		     "%s: can't read configuration file %s\n", pname, argv[i]);
		cleanup(1);
	    }
	    break;
	  case 'h':	/* help requested */
	    help();
	    cleanup(0);
	    break;
	  case 'o':	/* output annotator name */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: output annotator name must follow -o\n",
			      pname);
		cleanup(1);
	    }
	    oaname = argv[i];
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		cleanup(1);
	    }
	    record = argv[i];
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n", pname,
			  argv[i]);
	    cleanup(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n", pname,
			  argv[i]);
	    cleanup(1);
	}
    }

    /* Quit unless record name and at least two annotator names specified. */
    if (record == NULL || niann < 2) {
	help();
	cleanup(1);
    }

    /* Set up annotator info. */
    a = gcalloc(sizeof(WFDB_Anninfo), niann+1);
    for (i = a0, j = 0; i < a1; i++, j++) {
	a[j].name = argv[i]; a[j].stat = WFDB_READ;
    }
    a[j].name = oaname; a[j].stat = WFDB_WRITE;

    /* Read a priori physiologic parameters from the configuration file if
       available. They can be adjusted in the configuration file for pediatric,
       fetal, or animal ECGs. */
    if (config) {
	char buf[256], *p;

	/* Read the configuration file a line at a time. */
	while (fgets(buf, sizeof(buf), config)) {
	    /* Skip comments (empty lines and lines beginning with `#'). */
	    if (buf[0] == '#' || buf[0] == '\n') continue;
	    /* Set parameters.  Each `getconf' below is executed once for
	       each non-comment line in the configuration file. */
	    getconf(HR, "%lf");
	}
	fclose(config);
    }

    /* If any a priori parameters were not specified in the configuration file,
       initialize them here (using values chosen for adult human ECGs). */
    if (HR == 0.0) HR = 75;

    spm = 60.0 * sampfreq(record);  /* sample intervals per minute */
    nseg = strtim("e")/spm;	    /* number of complete 1-minute intervals */

    /* Set up buffer for median calculation. */
    mbuf = gcalloc(sizeof(int), niann + 1);
    mbuf[niann] = HR;	/* initial median is HR */

    /* Set up arrays for HR minute-by-minute time series. */
    count = gcalloc(sizeof(int *), niann + 1);
    for (i = 0; i < niann; i++)
	count[i] = gcalloc(sizeof(int), nseg+1);

    /* Pass 1: Load the HR arrays. */
    if (annopen(record, a, niann) < 0) cleanup(2);
    for (i = 0; i < niann; i++) {
	for (tf = spm, j = 0; j < nseg+1; tf += spm, j++) {
	    n = 0;
	    while (getann(i, &annot) == 0 && annot.time < tf)
		if (isqrs(annot.anntyp)) n++;
	    count[i][j] = n;
	}
    }
    wfdbquit();

    /* Pass 2: Select the input annotations to be copied, and copy them. */
    if (annopen(record, a, niann+1) < 0) cleanup(2);
    for (j = 0, t0 = 0, tf = spm; j < nseg+1; j++, t0 = tf, tf += spm) {
	for (i = 0; i < niann; i++)
	    mbuf[i] = count[i][j];
	/* Note that the last element, mbuf[niann], is the previous median. */
	qsort(mbuf, niann+1, sizeof(int), icomp);
	/* If mbuf has an even number of members, the median is the mean of
	   the two central values. */
	if (niann & 1) mbuf[niann] = (mbuf[(niann-1)/2] + mbuf[(niann+1)/2])/2;
	/* Otherwise it's the single central value. */
	else mbuf[niann] = mbuf[niann/2];
	/* Find the input that best matches the prediction (the median). */
	for (i = 0, dn = 9999; i < niann; i++) {
	    tdn = mbuf[niann] - count[i][j];
	    if (tdn < 0) tdn = -tdn;
	    if (tdn < dn) { dn = tdn; ibest = i; }
	}
	/* If the best match is not the previously chosen input, avoid switching
	   the input unless it's more than 2 beats further from the median. */
	if (ibest != ichosen) {
	    tdn = mbuf[niann] - count[ichosen][j];
	    if (tdn < 0) tdn = -tdn;
	    if (tdn < dn + 3) ibest = ichosen;
	}
	ichosen = ibest;
	/* Copy the chosen annotations for this one-minute segment. */
	n = 0;
	while (getann(ichosen, &annot) == 0 && n <= count[ichosen][j]) {
	    if (annot.time >= t0 && isqrs(annot.anntyp)) {
		putann(0, &annot);
		n++;
	    }
	}
	ungetann(ichosen, &annot);
    }
    cleanup(0);
}

/* prog_name() extracts this program's name from argv[0], for use in error and
   warning messages. */

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

/* help() prints a (very) concise summary of how to use this program.
   A more detailed summary is in the man page (gqpost.1). */

static char *help_strings[] = {
 "usage: %s -r RECORD -a ANNOTATOR ANNOTATOR ... [OPTIONS ...]\n",
 "where RECORD is the name of the record to be analyzed, two or more",
 "ANNOTATOR arguments specify annotation sets to be read and fused, and",
 "OPTIONS may include any of:",
 " -c FILE       initialize parameters from the specified configuration FILE",
 " -h            print this usage summary",
 " -o ANNOTATOR  write annotations to the specified ANNOTATOR (default: gqf)",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}

/* gcalloc() is a wrapper for calloc() that handles errors and maintains
   a list of allocated buffers for automatic on-exit deallocation via
   cleanup(). */

size_t gcn = 0, gcnmax = 0;
void **gclist = NULL;

void *gcalloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size);

    if ((p == NULL) ||
	((gcn >= gcnmax) &&
	 (gclist = realloc(gclist, (gcnmax += 32)*(sizeof(void *)))) == NULL))
	cleanup(3);	/* could not allocate requested memory */
    return (gclist[gcn++] = p);
}

void cleanup(int status)	/* close files and free allocated memory */
{
    if (status == 3)
	fprintf(stderr, "%s: insufficient memory (%d)\n", pname, status);
    if (record) wfdbquit();
    while (gcn-- > 0)
	if (gclist[gcn]) free(gclist[gcn]);
    exit(status);
}
