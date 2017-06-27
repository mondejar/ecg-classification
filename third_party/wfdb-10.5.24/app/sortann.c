/* file sortann.c	G. Moody	 7 April 1997
			Last revised:	4 October 2010
-------------------------------------------------------------------------------
sortann: Rearrange annotations in canonical order
Copyright (C) 1997-2010 George B. Moody

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

It is possible to create an annotation file containing out-of-order
annotations.  This program rewrites such files with the annotations in time
order.  Any simultaneous annotations are written in 'num' and 'chan' order.

If the input contains two or more annotations with the same time, num, and chan
fields, only the last one is copied.  As a special case of this policy, if the
last such annotation has anntyp = 0 (NOTQRS), no annotation is written at that
location (thus a program that generates input for sortann can effectively
delete a previously written annotation by writing a NOTQRS annotation at the
same location).

The sorted (output) annotation file is always written to the current directory.
If the input annotation file is in the current directory, sortann replaces it
unless you specify a different output annotator name (using the -o option).
Note that the output annotation file is likely to be slightly shorter than the
input file, since more compact storage is usually possible when all annotations
are sorted.

If the input annotations are already in the correct order, no output is written
unless you have used the -o option.

If you attempt to sort a very large annotation file, sortann may run out of
memory.  If this happens, use the -f and -t options to work on the file in
sections of any convenient size, one at a time, then use mrgann to concatenate
the sections.  Note that you must specify an output annotator name (with -o)
when using the -f or -t options (to avoid replacing the entire input file with
a sorted subset of its contents).

The working memory required by sortann is approximately 10 times the size of
the annotation file.  Since annotation files are rarely as large as 1 megabyte
and available memory is rarely less than 10 megabytes, it is unlikely that
sortann will exhaust available memory, however.
*/

#include <stdio.h>
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;
struct annlistentry {
    struct annlistentry *next, *prev;
    WFDB_Annotation annotation;
} annlist;

int in_order = 1;

main(argc, argv)	
int argc;
char *argv[];
{
    static WFDB_Anninfo ai[2];
    static WFDB_Annotation annot;
    static struct annlistentry *ap;
    char *record = NULL, *prog_name();
    long from = 0L, nann = 0L, to = 0L, atol();
    int i, insert_ann();
    double sps, spm;
    void cleanup(), help();

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* (input) annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai[0].name = argv[i];
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
	  case 'o':	/* output annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -o\n",
			      pname);
		exit(1);
	    }
	    ai[1].name = argv[i];
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

    if (record == NULL || ai[0].name == NULL) {
	help();
	exit(1);
    }
    if ((from || to) && ai[1].name == NULL) {
	fprintf(stderr,	"%s: -o required when using -f or -t options\n",
		pname);
	exit(1);
    }
    if (ai[1].name && strcmp(ai[0].name, ai[1].name) == 0) {
	fprintf(stderr,
		"%s: must specify different annotator names after -a and -o\n",
		pname);
	exit(1);
    }

    /* By setting WFDBANNSORT, we ensure that wfdbquit won't invoke this program
       recursively if something goes wrong. */
    putenv("WFDBANNSORT=0");

    if ((sps = sampfreq(record)) < 0.)
	(void)setsampfreq(sps = WFDB_DEFFREQ);
    spm = 60.0*sps;

    ai[0].stat = WFDB_READ;
    ai[1].stat = WFDB_WRITE;
    if (annopen(record, &ai[0], 1) < 0)	/* open input annotation file */
	exit(2);

    if (from) {
	from = strtim(argv[(int)from]);
	if (from < 0L) from = -from;
    }
    if (to) {
	to = strtim(argv[(int)to]);
	if (to < 0L) to = -to;
    }

    /* Build a linked list of annotations in memory. */
    while (getann(0, &annot) == 0) {
	if (annot.time < from || (to > 0L && annot.time >= to))
	    continue;
	if (insert_ann(&annot) == 0) {
	    fprintf(stderr, "%s: insufficient memory (%ld annotations read); ",
		    pname, nann);
	    if (strcmp(ai[0].name, ai[1].name) == 0)
		fprintf(stderr, "no changes made\n");
	    else
		fprintf(stderr, "no output written\n");
	    wfdbquit();
	    exit(3);
	}
	nann++;
    }
    wfdbquit();

    if (in_order && ai[1].name == NULL) {
	/* If all of the annotations were in order, don't copy them unless
	   another annotator name was specified (using -o). */
	fprintf(stderr, "%s: input is already ordered -- no output written\n",
		pname);
	cleanup();
	exit(0);
    }

    if (from == 0L && to == 0L && ai[1].name == NULL)
        /* in this case, we are processing the entire input file, and it's
	   acceptable to overwrite it */
	ai[1].name = ai[0].name; 

    if (annopen(record, &ai[1], 1) < 0) /* open output annotation file */
        exit(2);
    for (ap = annlist.next; ap; ap = ap->next)
	if ((ap->annotation).anntyp != NOTQRS)
	    putann(0, &(ap->annotation));
    wfdbquit();

    cleanup();

    exit(0);	/*NOTREACHED*/
}

static struct annlistentry *lastp = &annlist;
int insert_ann(pa)
WFDB_Annotation *pa;
{
    char *p;
    static struct annlistentry *ap, *newp = NULL;
    void copybytes();

    if (pa->aux) p = (char *)malloc(*(pa->aux)+2);
    newp = (struct annlistentry *)malloc(sizeof(struct annlistentry));
    if (newp == NULL) return (0);	/* insufficient memory -- quit */
    newp->annotation = *pa;
    if (pa->aux) {
	copybytes(p, pa->aux, *(pa->aux)+2);
	(newp->annotation).aux = p;
    }
    if (lastp == &annlist ||
	pa->time > (lastp->annotation).time ||
	(pa->time == (lastp->annotation).time &&
	 pa->num > (lastp->annotation).num) ||
	(pa->time == (lastp->annotation).time &&
	 pa->num == (lastp->annotation).num &&
	 pa->chan > (lastp->annotation).chan)) {
	/* this annotation is in order -- add to end of list */
	newp->prev = lastp;
	lastp->next = newp;
	lastp = newp;
	lastp->next = NULL;
    }
    else {
	in_order = 0;	/* set flag to indicate input was out-of-order */
	/* Find the correct location in the list for this annotation.  The
	   search is linear, beginning at the end of the list, which works
	   well for nearly-ordered input. */
	ap = lastp;
	while (ap) {
	    if (pa->time > (ap->annotation).time ||
		(pa->time == (ap->annotation).time &&
		 pa->num > (ap->annotation).num) ||
		(pa->time == (ap->annotation).time &&
		 pa->num == (ap->annotation).num &&
		 pa->chan >= (ap->annotation).chan)) {
		break;
	    }
	    ap = ap->prev;
	}
	if (ap == NULL) {
	    /* insert newp at the beginning of the list */
	    newp->prev = NULL;
	    newp->next = annlist.next;
	    (newp->next)->prev = annlist.next = newp;
	}
	else if (pa->time == (ap->annotation).time &&
		 pa->num == (ap->annotation).num &&
		 pa->chan == (ap->annotation).chan) {  /* replace ap by newp */
	    if (newp->prev = ap->prev) (newp->prev)->next = newp;
	    if (newp->next = ap->next) (newp->next)->prev = newp;
	    else lastp = newp;
	    if ((ap->annotation).aux) free((ap->annotation).aux);
	    if (ap != &annlist) free(ap);
	}
	else {	/* insert newp immediately after ap */
	    newp->prev = ap;
	    newp->next = ap->next;
	    (newp->next)->prev = ap->next = newp;
	}
    }
    return (1);	/* success */
}

void cleanup()	/* free the memory used for the annotation list */
{
    while (lastp != NULL && lastp != &annlist) {
	if ((lastp->annotation).aux) free((lastp->annotation).aux);
	if (lastp = lastp->prev) free(lastp->next);
    }
}

/* This function emulates memcpy (which is not universally available). */
void copybytes(dest, src, n)
char *dest, *src;
int n;
{
    while (n-- > 0)
	*dest++ = *src++;
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
 " -f TIME    start at specified TIME",
 " -h         print this usage summary",
 " -o OUTANN  write output as annotator OUTANN (required with -f or -t)",
 " -t TIME    stop at specified TIME",
 "The sorted output replaces the input if -o is omitted, the input is in",
 "the current directory, and the input is not already sorted.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
