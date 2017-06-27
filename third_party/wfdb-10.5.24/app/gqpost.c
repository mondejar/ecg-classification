/* file: gqpost.c		G. Moody	16 November 2006
				Last revised:	   6 May 2012
-------------------------------------------------------------------------------
gqpost: A post-processor for gqrs
Copyright (C) 2006-2012 George B. Moody

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

The companion to this program, gqrs, is a QRS detector that is optimized for
sensitivity.  To reduce the number of false positives, gqpost can read the
output annotation file produced by gqrs and can look for interpolated beat
detections in it. Such events are identified by looking for pairs or longer
groups of inter-beat intervals that sum to values close to the predicted
interbeat interval (in other words, an interpolated event is one that occurs
between a pair of beats that occur at their expected times).  Although
true beats can be interpolated, such beats are relatively rare in comparison
with false detections that are almost always interpolated, so a high proportion
of interpolated events is false, and a simple strategy for improving positive
predictivity is to reject all interpolated events.  To avoid rejecting
(at least some of) the true interpolated beats, gqpost also looks at the size
of each interpolated event as recorded by gqrs in the annotation 'num' field,
and it rejects only those events that are smaller than a threshold value
(which may be adjusted using the -m option).

gqpost is most effective in the context of regular rhythms.  During highly
irregular rhythms, such as atrial fibrillation, the predicted time of the
next beat is highly uncertain, and gqpost rejects very few if any events in
such cases.

gqpost is very fast since it operates entirely on the input annotation
file, without reference to the raw signals.  The output of gqpost is a
complete copy of its input, except that the anntyp fields of any rejected
events occurring between the start and end times (specified on the command
line using the -f and -t options) are set to ARFCT.  gqpost can accept its
own output as input, which allows one to vary the rejection threshold if
necessary by use of multiple post-processing passes.
*/

#include <stdio.h>
#include <stddef.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>
#define PBLEN	12	/* size of predictor array */

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

main(int argc, char **argv)
{
    FILE *config = NULL;
    int dt, i, meanrr, meanrrd, minrrd, state = 0, thresh = 20;
    static double HR, RR, RRdelta;
    WFDB_Anninfo a[2];
    WFDB_Annotation annot;
    WFDB_Frequency sps;
    WFDB_Time from = (WFDB_Time)0, to = (WFDB_Time)0;

    a[0].name = "qrs"; a[0].stat = WFDB_READ;
    a[1].name = "gqp"; a[1].stat = WFDB_WRITE;
    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* input annotator name */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: input annotator name must follow -a\n",
			      pname);
		cleanup(1);
	    }
	    a[0].name = argv[i];
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
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		cleanup(1);
	    }
	    from = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    cleanup(0);
	    break;
	  case 'm':	/* threshold */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: threshold must follow -m\n", pname);
		cleanup(1);
	    }
	    thresh = 10*atof(argv[i]) + 0.5;
	    break;
	  case 'o':	/* output annotator name */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: output annotator name must follow -o\n",
			      pname);
		cleanup(1);
	    }
	    a[1].name = argv[i];
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -r\n",
			      pname);
		cleanup(1);
	    }
	    record = argv[i];
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n", pname);
		cleanup(1);
	    }
	    to = i;
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
    if (record == NULL) {
	help();
	cleanup(1);
    }

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
	    getconf(RR, "%lf");
	    getconf(RRdelta, "%lf");
	}
	fclose(config);
    }

    /* If any a priori parameters were not specified in the configuration file,
       initialize them here (using values chosen for adult human ECGs). */
    if (HR != 0.0) RR = 60.0/HR;
    if (RR == 0.0) RR = 0.8;
    if (RRdelta == 0.0) RRdelta = RR/4;

    sps = sampfreq(record);
    if (annopen(record, a, 2) < 0) cleanup(2);
    meanrr = RR * sps;
    meanrrd = RRdelta * sps;
    if ((minrrd = meanrrd/2) < 4) minrrd = 4;
    if ((dt = meanrr/80) < 1) dt = 1;
    
    while (getann(0, &annot) == 0) {
	switch (state) {
	  case 0:
	    if (annot.time < from) continue;
	    else state++;
	    /* fall through to case 1 */
  	  case 1:
	    if (to > (WFDB_Time)0 && annot.time > to) state++;
	    else {	/* reject sub-threshold interpolated events */
		static WFDB_Time tpred, tpredmin, tprev;
		static int rr, rrd;

		if (annot.time < tpredmin &&
		    isqrs(annot.anntyp) &&
		    annot.num < thresh) {   /* event is small and early */
		    WFDB_Annotation nextann;
		    if (getann(0, &nextann) == 0) {	/* peek ahead */
			if (nextann.time <= tpred)
			    annot.anntyp = ARFCT;	/* reject event */
			ungetann(0, &nextann);
		    }
		}
		if (isqrs(annot.anntyp)) {
		    rr = annot.time - tprev;
		    if ((rrd = rr - meanrr) < 0) rrd = -rrd;
		    if (rr > meanrr + dt) meanrr += dt;
		    else if (rr > meanrr - dt) meanrr = rr;
		    else meanrr -= dt;
		    if (rrd > meanrrd + dt) meanrrd += dt;
		    else if (rrd > meanrrd - dt) meanrrd = rrd;
		    else meanrrd -= dt;
		    if (meanrrd < minrrd) meanrrd = minrrd;
		    tpred = annot.time + meanrr + minrrd;
		    tpredmin = annot.time + meanrr - meanrrd;
		    tprev = annot.time;
		}
	    }
	    break;
	  case 2:
	    /* do nothing */
	    break;
	}
	putann(0, &annot);
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
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the record to be analyzed, and OPTIONS may",
 "include any of:",
 " -a ANNOTATOR read annotations from the specified ANNOTATOR (default: qrs)",
 " -c FILE     initialize parameters from the specified configuration FILE",
 " -f TIME     begin processing at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -m THRESH   set interpolated event acceptance threshold to THRESH",
 "              (default: 1)",
 " -o ANNOTATOR write annotations to the specified ANNOTATOR (default: gqp)",
 " -t TIME     stop processing at specified time",
 "If too many true beats are rejected, decrease THRESH;  if too many false",
 "detections are accepted, increase THRESH.",
 "Note that the output is a complete copy of the input (with rejected events",
 "flagged as ARFCT).  The -f and -t options only limit the interval during",
 "which events may be rejected.",
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
