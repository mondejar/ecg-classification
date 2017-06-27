/* file: wfdbcollate.c        G. Moody        28 April 1994
			      Last revised:  14 November 2002

-------------------------------------------------------------------------------
wfdbcollate: Collate WFDB records into a multi-segment record
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

A multi-segment record is the concatenation of one or more ordinary records.  A
multi-segment record is a "virtual" record, in the sense that it has no signal
files of its own.  Its header file contains a list of the records that comprise
the multi-segment record.  A multi-segment record may have associated
annotation files, but these are independent of any annotation files that may
exist for its constituent segments.  It is permissible (though not particularly
useful) to create a multi-segment record with only one segment; it is not
permissible to use a multi-segment record as a segment within a multi-segment
record, however.

Usually, this program simply constructs an array of segment names, passing it
to the WFDB library function `setmsheader' to create a multi-segment header
file.  `wfdbcollate' can be invoked in either of two ways to do this:

 (1) wfdbcollate -i IREC1 IREC2 ... -o OREC [-a ANNOTATOR ]
     where OREC is the name of the multi-segment (output) record to be created,
     and IREC1, IREC2, etc. are the names of the (single-segment) input records
     that are to be included in OREC.  At least one IREC must be specified.

 (2) wfdbcollate OREC FIRST LAST [ -a ANNOTATOR ]
     where OREC is the name of the multi-segment (output) record to be created,
     and FIRST and LAST are numbers between 1 and 99999.  In this case, OREC
     must be 3 characters or fewer (longer names are truncated), and the names
     of the input records are derived by appending FIRST, FIRST+1, ..., LAST
     to OREC (representing FIRST, ..., as 5-digit zero-padded decimal numbers).
     Thus the command
        wfdbcollate xyz 9 12
     is equivalent to
        wfdbcollate -o xyz -i xyz00009 xyz00010 xyz00011 xyz00012

(In both forms, `-a ANNOTATOR' is optional;  if included, it specifies the
 annotator name of annotation files associated with the input records, files
 to be concatenated to form a similarly-named annotation file for OREC.  Note
 that all of the files to be concatenated must have the same annotator name.
 It is not necessary that this annotator exist for each input record, however.)

This program can also be used to split an ordinary (single-segment) record into
multiple segments.  In this mode, wfdbcollate first creates a set of segments,
then collates them into a multi-segment record.  To do this, use the form:

 (3) wfdbcollate -s IREC -o OREC [-l SEGLEN]
     where IREC is the name of an existing (ordinary) record, OREC is the name
     of the multi-segment record to be created, and SEGLEN is the length to be
     used for each segment (default: 10 minutes).  The names of the segments
     created in this way are formed from the first three characters of OREC and
     from a 5-digit zero-padded segment number, as for form (2) above.

In most cases, multi-segment records are indistinguishable from single-segment
records (from the point of view of applications built using the WFDB library,
version 9.1 or later).  Use `xform' to generate a single-segment record from a
multi-segment record if necessary (for example, to make it readable by an
application built using an earlier version of the WFDB library).  Note,
however, that older applications can generally be updated without source
changes simply by recompiling them and linking them with the current WFDB
library.  */

#include <stdio.h>
#include <wfdb/wfdb.h>

#define DEFSEGLEN	"10:0"	/* segments are 10 minutes long by default */
#define MINSEGLEN	"15"	/* segments must be at least 15 seconds long */

char *pname, *prog_name();
WFDB_Anninfo ai;
WFDB_Annotation annot;
WFDB_Sample *v;
WFDB_Siginfo *si;

int collate();
void help(), splitrecord();

main(argc, argv)
int argc;
char *argv[];
{
    collate(argc, argv);
    exit(0);
}

void splitrecord(argc, argv)
int argc;
char *argv[];
{
    int framelen, i, segnumber = 0, nsig;
    static char *irecname, *orecname, segname[10], sigfname[15];
    static char irecbase[30], segbase[30];
    static WFDB_Time seglen, t, tf;

    for (i = 1; i < argc-1; i++) {
	if (argv[i][0] == '-')
	    switch (argv[i][1]) {
	      case 'a':	/* annotator name follows */
	      case 'i':	/* input record names follow */
		fprintf(stderr, "%s: -s and %s cannot be used together\n",
			pname, argv[i]);
		help();
		exit(1);
	      case 'l':	/* segment length follows */
		seglen = ++i; /* seglen will be determined below */
		break;
	      case 'o':	/* output record name follows */
		orecname = argv[++i];
		break;
	      case 's': /* create multi-segment record from ordinary record */
		irecname = argv[++i];
		break;
	      default:
		fprintf(stderr, "%s: unrecognized option `%s'\n",
			pname, argv[i]);
		help();
		exit(1);
	    }
	else {
	    fprintf(stderr, "%s: unrecognized option `%s'\n",
		    pname, argv[i]);
	    help();
	    exit(1);
	}
    }
    if (irecname == NULL || orecname == NULL) {
	help();
	exit(1);
    }
    /* Determine the number of signals in the record. */
    if ((nsig = isigopen(irecname, NULL, 0)) <= 0)
	exit(2);
    /* Allocate storage for the signal information structures. */
    if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(3);
    }
    /* Open the input signals. */
    if (isigopen(irecname, si, nsig) != nsig)
	exit(2);
    /* Allocate storage for the sample frame. */
    for (i = framelen = 0; i < nsig; i++)
	framelen += si[i].spf;
    if ((v = malloc(framelen * sizeof(WFDB_Sample))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(3);
    }
    if (newheader(orecname) < 0)
	exit(2);
    if (seglen > 0) {
	seglen = strtim(argv[seglen]);
	if (seglen < strtim(MINSEGLEN)) {
	    fprintf(stderr, "%s: segment length must be at least %s seconds\n",
		    pname, MINSEGLEN);
	    wfdbquit();
	    help();
	    exit(1);
	}
    }
    else
	t = strtim(DEFSEGLEN);
    if (seglen <= t)
	seglen = t;
    if (strlen(orecname) > 3) {
	fprintf(stderr, "%s: output record name `%s'", pname, orecname);
	orecname[3] = '\0';
	fprintf(stderr, " truncated to `%s'\n", orecname);
    }

    for (i = 0; i < nsig; i++) {
	si[i].fname = sigfname;
	si[i].group = 0;
    }

    /* Disable checksum testing, to avoid losing data at the end of the record
       if the checksums are not correct. */
    isigsettime(99L);
    isigsettime(t = tf = 0L);

    strcpy(irecbase, timstr(0L));
    if (*irecbase == '[')
	irecbase[strlen(irecbase)-1] = '\0';

    while (getframe(v) > 0) {
	if (t >= tf) {
	    if (segnumber > 0) {
		setbasetime(segbase+1);
		newheader(segname);	/* write header for current segment */
		setbasetime(irecbase+1);
		osigfopen(NULL, 0);	/* close output signal file */
	    }
	    strcpy(segbase, timstr(-t));
	    if (*segbase == '[')
		segbase[strlen(segbase)-1] = '\0';
	    sprintf(segname, "%s%05d", orecname, ++segnumber);
	    sprintf(sigfname, "%s.dat", segname);
	    if (osigfopen(si, nsig) < nsig) {
		fprintf(stderr, "%s: output incomplete\n", pname);
		exit(3);
	    }
	    tf += seglen;
	    fprintf(stderr, "\rwriting segment %s ...", segname);
	}
	putvec(v);
	t++;
    }
    if (t > 0) {
	char *cargv[4], lastsegstr[10];

	setbasetime(segbase+1);
	newheader(segname);	/* write header for final segment */
	wfdbquit();
	fprintf(stderr, " done\ncollating segments ...");

	/* build argument list for collate() */
	sprintf(lastsegstr, "%d", segnumber);
	cargv[0] = pname;
	cargv[1] = orecname;
	cargv[2] = "1";
	cargv[3] = lastsegstr;

	/* invoke main() to collate segments into a new record */
	collate(4, cargv);
	fprintf(stderr, " done\n");
    }
    else {
	fprintf(stderr, "%s: record %s is empty or unreadable\n",
		pname, irecname);
	exit(3);
    }
    exit(0);
}

int collate(argc, argv)
int argc;
char *argv[];
{
    char **irecname, *orecname = NULL, ofname[20];
    int first, last, i, nsegments = 0, nsig, segment;

    pname = prog_name(argv[0]);

    if (argc < 2) {
	help();
	exit(1);
    }
    if (*argv[1] == '-') {
	irecname = (char **)malloc(argc * sizeof(char *));
	/* The array is slightly bigger than necessary. */
	i = 1;
	while (i < argc-1 && *argv[i] == '-') {
	    switch (argv[i][1]) {
	      case 'a':	/* annotator name follows */
		ai.name = argv[++i];
		break;
	      case 'i':	/* input record names follow */
		while (i < argc && *argv[++i] != '-')
		    irecname[nsegments++] = argv[i];
		break;
	      case 'l':
		fprintf(stderr, "%s: -s must precede -l\n", pname);
		help();
		exit(1);
		break;
	      case 'o':	/* output record name follows */
		orecname = argv[++i];
		i++;
		break;
	      case 's': /* create multi-segment record from ordinary record */
	        splitrecord(argc, argv);
		break;
	      default:
		fprintf(stderr, "%s: unrecognized option `%s'\n",
			pname, argv[i]);
		help();
		exit(1);
	    }
	}
    }
    else {
	orecname = argv[1];
	if (strlen(orecname) > 3) {
	    fprintf(stderr, "%s: output record name `%s'", pname, orecname);
	    orecname[3] = '\0';
	    fprintf(stderr, " truncated to `%s'\n", orecname);
	}
	if ((first = atoi(argv[2])) < 1) first = 1;
	if ((last = atoi(argv[3])) < first) last = 999;
	if (argc > 5 && strcmp(argv[4], "-a") == 0)
	    ai.name = argv[5];
	nsegments = last-first+1;
	irecname = (char **)malloc(sizeof(char *) * nsegments);
	for (i = first; i <= last; i++) {
	    irecname[i-first] = (char *)malloc(9);
	    sprintf(irecname[i-first], "%s%05d", orecname, i);
	}
    }

    if (orecname == NULL || nsegments < 1) {
	help();
	exit(1);
    }
    if (newheader(orecname) < 0)
	exit(2);

    setmsheader(orecname, irecname, nsegments);
    wfdbquit();

    if (ai.name) {
	char buf[20];
	long t0 = 0L;

	ai.stat = WFDB_WRITE;
	if (annopen(orecname, &ai, 1) < 0)
	    exit(3);
	ai.stat = WFDB_READ;
	/* Determine the number of signals in the first segment of the
	   record.  (We assume this is constant for all segments.) */
	if ((nsig = isigopen(irecname[0], NULL, 0)) <= 0)
	    exit(2);
	/* Allocate storage for the signal information structures. */
	if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	    fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(3);
	}
	/* Assume that segments are of the default length unless the
	   header tells us otherwise (after invoking isigopen below). */
	si[0].nsamp = strtim(DEFSEGLEN);
	for (i = 0; i < nsegments; i++) {
	    sprintf(buf, "+%s", irecname[i]);
	    if (annopen(buf, &ai, 1) == 0) {
		while (getann(0, &annot) == 0) {
		    annot.time += t0;
		    putann(0, &annot);
		}
		iannclose(0);	/* close input file, leave output file open */
	    }
	    /* Fill the signal info structures without opening the signals. */
	    (void)isigopen(irecname[i], si, -nsig);
	    /* If isigopen fails, the previous contents of si are undisturbed.
	       In this case, we need to guess the length of the segment.  A
	       good guess is that it is the same length as the previous segment
	       -- so we can use si[0].nsamp irrespective of the success of
	       isigopen! */
	    t0 += si[0].nsamp;
	}
	wfdbquit();
    }

    return (0);
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

void help()
{
    fprintf(stderr,
	    "usage: %s -i IREC [ IREC ... ] -o OREC [ -a ANNOTATOR ]\n",
	    pname);
    fprintf(stderr,
   " or:   %s OREC first-segment-number last-segment-number [-a ANNOTATOR ]\n",
	    pname);
    fprintf(stderr,
   " or:   %s -s IREC -o OREC [ -l SEGLEN ]\n", pname);
    fprintf(stderr,
   "Use either of the first two forms to create a multi-segment record\n");
    fprintf(stderr,
   "from an existing set of ordinary records (segments).\n");
    fprintf(stderr,
   "Use the last form to split a single ordinary record into segments.\n");
}
