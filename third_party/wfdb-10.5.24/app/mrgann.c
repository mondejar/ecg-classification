/* file mrgann.c		G. Moody	28 May 1995
				Last revised:  30 April 1999

-------------------------------------------------------------------------------
mrgann: Merge annotation files by segments
Copyright (C) 1999 George B. Moody

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

This program reads two annotation files and creates a third.  Command-line
arguments divide the annotation files into segments.  Within each segment,
the annotations copied to the output annotation file may be any of the
following:
 - none (selected by -m0)
 - all annotations from the first input file (selected by -m1)
 - all annotations from the second input file (selected by -m2)
 - all annotations from both input files (default; selected by -m3)

Optionally, mrgann can remap the `chan' field in each annotation from one or
both input files to a value that can be specified separately (using -c or -C)
for each input annotation file.  This feature may be useful, for example, to
merge annotations for independent signals, where there may be occasional
simultaneous input annotations.  By remapping only the `chan' fields from one
input file, it is possible in two or more passes to merge three or more
annotation files in this way.

When in `-m3' mode, if simultaneous annotations with the same `chan' field
(after any remapping has been done) are present, only the annotation from the
first annotator is copied, and a warning message is written to the standard
output.  */

#include <stdio.h>
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>

/* mode definitions */
#define UNINITIALIZED	(-1)
#define DISCARD_ALL	0
#define COPY_0		1
#define COPY_1		2
#define MERGE		3

char *pname, *record = NULL;
static int ateof[2], map0 = -1, map1 = -1, vflag;
static WFDB_Anninfo ai[3];
static WFDB_Annotation annot[2];
void help(), mergeann();

main(argc, argv)	
int argc;
char *argv[];
{
    char *prog_name();
    WFDB_Time tf = (WFDB_Time)(-1);
    int i, mode = UNINITIALIZED, next_mode = MERGE;

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':	/* map0 follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		     "%s: `chan' mapping for first annotator must follow -c\n",
			      pname);
		exit(1);
	    }
	    map0 = atoi(argv[i]);
	    if (map0 < -1 || map0 > 255) map0 = -1;
	    break;
	  case 'C':	/* map1 follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		    "%s: `chan' mapping for second annotator must follow -C\n",
			      pname);
		exit(1);
	    }
	    map1 = atoi(argv[i]);
	    if (map1 < -1 || map1 > 255) map1 = -1;
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* input annotators follow */
	    if (++i >= argc-1) {
		(void)fprintf(stderr, "%s: input annotators must follow -i\n",
			      pname);
		exit(1);
	    }
	    ai[0].name = argv[i]; ai[0].stat = WFDB_READ;
	    ai[1].name = argv[++i]; ai[1].stat = WFDB_READ;
	    break;
	  case 'm':	/* time to switch modes follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: time to change modes must follow %s\n",
			      pname, argv[i-1]);
		exit(1);
	    }
	    tf = strtim(argv[i]);
	    if (tf < (WFDB_Time)0) tf = -tf;
	    if (argv[i][0] == 'e') tf = (WFDB_Time)(-1);
	    if (mode == UNINITIALIZED) {
		init();
		mode = MERGE;
	    }
	    mergeann(mode, tf);
	    switch (*(argv[i-1]+2)) {
	      case '0': mode = DISCARD_ALL; break;
	      case '1': mode = COPY_0; break;
	      case '2': mode = COPY_1; break;
	      case '3': mode = MERGE; break;
	      default:
		fprintf(stderr,
			"%s: unrecognized mode `%c' -> 3\n",
			pname, *(argv[i]+2));
		mode = MERGE;
		break;
	    }
	    break;
	  case 'o':	/* output annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output annotator must follow -o\n",
			      pname);
		exit(1);
	    }
	    ai[2].name = argv[i]; ai[2].stat = WFDB_WRITE;
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
	  case 'v':	/* verbose mode */
	    vflag = 1;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s (ignored)\n",
			  pname, argv[i]);
	    break;
	}
	else
	    (void)fprintf(stderr, "%s: unrecognized argument %s (ignored)\n",
			  pname, argv[i]);
    }

    if (mode == UNINITIALIZED) {
	init();
	mode = MERGE;
    }
    mergeann(mode, (WFDB_Time)(-1));

    wfdbquit();
    exit(0);	/*NOTREACHED*/
}

init()
{
    if (record == NULL || ai[0].name == NULL ||
	ai[1].name == NULL || ai[2].name == NULL) {
	help();
	exit(1);
    }
    if (sampfreq(record) < 0.)
	(void)setsampfreq(WFDB_DEFFREQ);
    if (annopen(record, ai, 3) < 0)
	exit(2);
    ateof[0] = getann(0, &annot[0]);
    ateof[1] = getann(1, &annot[1]);
}

void mergeann(mode, tf)
int mode;
WFDB_Time tf;
{
    switch (mode) {
      case DISCARD_ALL:
	while (!ateof[0] && (tf < 0L || annot[0].time < tf))
	    ateof[0] = getann(0, &annot[0]);
	while (!ateof[1] && (tf < 0L || annot[1].time < tf))
	    ateof[1] = getann(1, &annot[1]);
	break;
      case COPY_0:
	while (!ateof[0] && (tf < 0L || annot[0].time < tf)) {
	    if (map0 >= 0) annot[0].chan = map0;
	    putann(0, &annot[0]);
	    ateof[0] = getann(0, &annot[0]);
	}
	while (!ateof[1] && (tf < 0L || annot[1].time < tf))
	    ateof[1] = getann(1, &annot[1]);
	break;
      case COPY_1:
	while (!ateof[0] && (tf < 0L || annot[0].time < tf))
	    ateof[0] = getann(0, &annot[0]);
	while (!ateof[1] && (tf < 0L || annot[1].time < tf)) {
	    if (map1 >= 0) annot[1].chan = map1;
	    putann(0, &annot[1]);
	    ateof[1] = getann(1, &annot[1]);
	}
	break;
      case MERGE:
	while (!ateof[0] && !ateof[1] &&
	       (tf < 0L || annot[0].time < tf || annot[1].time < tf)) {
	    if (annot[0].time < annot[1].time) {
		if (map0 >= 0) annot[0].chan = map0;
		putann(0, &annot[0]);
		ateof[0] = getann(0, &annot[0]);
	    }
	    else if (annot[0].time > annot[1].time) {
		if (map1 >= 0) annot[1].chan = map1;
		putann(0, &annot[1]);
		ateof[1] = getann(1, &annot[1]);
	    }
	    else {
		if (map0 >= 0) annot[0].chan = map0;
		if (map1 >= 0) annot[1].chan = map1;
		if (annot[0].chan < annot[1].chan) {
		    putann(0, &annot[0]);
		    putann(0, &annot[1]);
		}
		else if (annot[0].chan > annot[1].chan) {
		    putann(0, &annot[1]);
		    putann(0, &annot[0]);
		}
		else {
		    putann(0, &annot[0]);
		    if (vflag && annot[0].anntyp != annot[1].anntyp)
			fprintf(stderr, "%s: %s written, %s discarded\n",
				mstimstr(annot[0].time),
				annstr(annot[0].anntyp),
				annstr(annot[1].anntyp));
		}
		ateof[0] = getann(0, &annot[0]);
		ateof[1] = getann(1, &annot[1]);
	    }
	}
	while (!ateof[0] && ateof[1] && (tf < 0L || annot[0].time < tf)) {
	    if (map0 >= 0) annot[0].chan = map0;
	    putann(0, &annot[0]);
	    ateof[0] = getann(0, &annot[0]);
	}
	while (ateof[0] && !ateof[1] && (tf < 0L || annot[1].time < tf)) {
	    if (map1 >= 0) annot[1].chan = map1;
	    putann(0, &annot[1]);
	    ateof[1] = getann(1, &annot[1]);
	}
	break;
    }
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
 "usage: %s -r RECORD -i ANNOTATOR1 ANNOTATOR2 -o ANNOTATOR3 [OPTIONS ...]\n",
 "where RECORD, ANNOTATOR1, and ANNOTATOR2 specify the input, ANNOTATOR3",
 "specifies an output annotation file for RECORD, and OPTIONS may include:",
 " -h       print this usage summary",
 " -mX TIME change mode to X at specified TIME, where X is one of:",
 "   0      discard all annotations beginning at TIME",
 "   1      copy ANNOTATOR1 annotations and discard ANNOTATOR2 annotations",
 "   2      copy ANNOTATOR2 annotations and discard ANNOTATOR1 annotations",
 "   3      merge ANNOTATOR1 and ANNOTATOR2 annotations (default)",
 " -v       verbose mode (warn about simultaneous annotations)",
 " -c N     map `chan' fields of ANNOTATOR1 annotations to N (-1 <= N <= 255)",
 " -C N     map `chan' fields of ANNOTATOR2 annotations to N (-1 <= N <= 255)",
 "Specifying N as -1 disables `chan' mapping (default).",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
