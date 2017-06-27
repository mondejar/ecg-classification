/* file: snip.c		G. Moody	30 July 1989
			Last revised:  1 December 2013
-------------------------------------------------------------------------------
snip: Copy an excerpt of a database record
Copyright (C) 1989-2013 George B. Moody

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
#include <wfdb/wfdblib.h>

char *pname;
int fmt = 0;	/* use specified output format if fmt > 0 */
int mflag = 0;	/* try to preserve segments if non-zero */
int sflag = 0;	/* suppress copying info if non-zero */

main(argc, argv)
int argc;
char *argv[];
{
    char *nrec = NULL, *irec = NULL, *startp = NULL, *endp = NULL;
    char **annotators, *length = NULL, *prog_name();
    int i, nann = 0, nsig = 0;
    WFDB_Time from = 0L, to = 0L, endt = 0L;
    void copy_ann(char *nrec, char *irec, WFDB_Time from, WFDB_Time to,
		  char **annotators, int nann);
    void copy_sig(char *nrec, char *irec, WFDB_Time from, WFDB_Time to,
		  int mflag);
    void copy_info(char *irec, char *startp);
    void help();

    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator(s) follow */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator(s) must follow -a\n",
			      pname);
		exit(1);
	    }
	    /* Accept the next argument unconditionally as an annotator name;
	       accept additional arguments until we find one beginning with
	       `-', or the end of the argument list. */
	    annotators = &argv[i];
	    do {
	        ++nann;
	    } while (++i < argc && *argv[i] != '-');
	    if (i < argc) i--;
	    break;
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		exit(1);
	    }
	    startp = argv[i];
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* input record name */
	  case 'r':	/* accept as equivalent to -i */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -i\n",
			      pname);
		exit(1);
	    }
	    irec = argv[i];
	    break;
	  case 'l':	/* length of snip */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: duration must follow -l\n", pname);
		exit(1);
	    }
	    length = argv[i];
	    break;
	  case 'm':  /* preserve segments of multisegment input, if possible */
	    mflag = 1;
	    break;
	  case 'n':	/* new record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: new record name must follow -n\n",
			      pname);
		exit(1);
	    }
	    nrec = argv[i];
	    break;
	  case 'O':	/* output format */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output format must follow -O\n",
			      pname);
		exit(1);
	    }
	    fmt = strtol(argv[i], NULL, 10);
	    break;
	    
	  case 's':	/* suppress copying of info */
	    sflag = 1;
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		exit(1);
	    }
	    endp = argv[i];
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
    if (irec == NULL || nrec == NULL) {
	help();
	exit(1);
    }
    if (startp == NULL) startp = "0";
    if (endp == NULL) endp = "e";
    
    /* Verify that the output record can be written. */
    if (newheader(nrec) < 0) {
	fprintf(stderr, "%s: can't create output header\n", pname);
	exit(2);
    }

    /* Determine the number of signals. */
    if ((nsig = isigopen(irec, NULL, 0)) < 0) {
	fprintf(stderr, "%s: error reading input header\n", pname);
	exit(2);
    }

    /* Evaluate the time limits. */
    if ((from = strtim(startp)) < 0L)
	from = -from;
    if (length)
	to = from + strtim(length);
    else if ((to = strtim(endp)) < 0L)
	to = -to;
    endt = strtim("e");
    if ((endt > 0L && endt < from) || (to > 0L && to < from)) {
	fprintf(stderr, "%s: improper interval selected\n", pname);
	wfdbquit();
	exit(1);
    }

    /* Copy the annotations, if any. */
    if (nann > 0)
	copy_ann(nrec, irec, from, to, annotators, nann);

    /* Copy the signals, if any. */
    if (nsig > 0)
	copy_sig(nrec, irec, from, to, mflag);

    /* Copy info strings from the input header file into the new one, unless
       suppressed with -s. */
    if (!sflag)
	copy_info(irec, startp);

    wfdbquit();
    exit(0);	/*NOTREACHED*/
}

void copy_ann(char *nrec, char *irec, WFDB_Time from, WFDB_Time to,
	      char **annotators, int nann)
{
    char *orec;
    int i;
    WFDB_Anninfo *ai, *ao;
    WFDB_Annotation annot;

    /* Allocate data structures for nann annotators. */
    if ((ai = malloc((nann) * sizeof(WFDB_Anninfo))) == NULL ||
	(ao = malloc((nann) * sizeof(WFDB_Anninfo))) == NULL ||
	(orec = malloc((strlen(nrec)+2) * sizeof(char))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    for (i = 0; i < nann; i++) {
	ai[i].name = malloc((strlen(annotators[i]) + 1) * sizeof(char));
	strcpy(ai[i].name, annotators[i]);
	ai[i].stat = WFDB_READ;
	ao[i].name = malloc((strlen(annotators[i]) + 1) * sizeof(char));
	strcpy(ao[i].name, annotators[i]);
	ao[i].stat = WFDB_WRITE;
    }

    /* Open the input annotators. */
    if (annopen(irec, ai, nann) < 0) {
	fprintf(stderr, "%s: error opening input annotators\n", pname);
	exit(2);
    }

    /* Open the output annotators. */
    (void)sprintf(orec, "+%s", nrec);
    if (annopen(orec, ao, nann) < 0) {
	fprintf(stderr, "%s: error opening output annotators\n", pname);
	exit(2);
    }

    /* Copy the selected segment. */
    for (i = 0; i < nann; i++) {
	while (getann((unsigned)i, &annot) == 0) {
	    if (annot.time >= from && (to == 0L || annot.time < to)) {
		annot.time -= from;
		if (putann((unsigned)i, &annot) < 0) break;
	    }
	}
    }
    wfdbquit();
    free(orec);
    free(ai);
}

void copy_sig(char *nrec, char *irec, WFDB_Time from, WFDB_Time to, int recurse)
{
    char *ofname, *p, tstring[24];
    int i, j, nsig, maxseg, maxres;
    long nsamp;
    WFDB_Sample *v;
    WFDB_Siginfo *si;
    WFDB_Time t, tf;
    WFDB_Seginfo *seginfo;

    wfdbquit();
    if ((nsig = isigopen(irec, NULL, 0)) < 0) {
	fprintf(stderr, "%s: error opening header for %s\n", pname, irec);
	exit(2);
    }

    /* Allocate data structures for nsig signals. */
    if ((v = malloc(nsig * sizeof(WFDB_Sample))) == NULL ||
	(si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(ofname = malloc((strlen(nrec)+5) * sizeof(char))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }

    /* Open the input signals. */
    if (isigopen(irec, si, (unsigned)nsig) != nsig) {
	fprintf(stderr, "%s: error opening signals for %s\n", pname, irec);
	exit(2);
    }

    p = mstimstr(-from);
    if (*p == '[') {
	strncpy(tstring, mstimstr(-from)+1, 23);
	tstring[23] = 0;
    }
    else tstring[0] = '\0';

    if (recurse && (maxseg = getseginfo(&seginfo)) > 0) {
	/* irec has multiple segments */
	char *ohfname, *orseg;
	int first_seg, last_seg, nseg;
	WFDB_Frequency sfreq = sampfreq(NULL);
	FILE *ohfile;
	WFDB_Seginfo *tmp;

	SUALLOC(tmp, maxseg, sizeof(WFDB_Seginfo));
	memcpy(tmp, seginfo, maxseg * sizeof(WFDB_Seginfo));
	seginfo = tmp;

	ohfname = malloc((strlen(nrec)+5) * sizeof(char));
	sprintf(ohfname, "%s.hea", nrec);
	ohfile = fopen(ohfname, "wb");
	if (ohfile == NULL) {
	    fprintf(stderr, "%s: can't create master header\n", pname);
	    exit(2);
	}

	if (seginfo[0].nsamp == 0) { /* irec has variable layout */
	    char *olrecname;

	    SUALLOC(olrecname, strlen(nrec) + 8, sizeof(char));
	    sprintf(olrecname, "%s_layout", nrec);

	    for (i = 0; i < nsig; i++) {
		si[i].fname = "~";
		si[i].fmt = 0;
		si[i].nsamp = 0;
		si[i].cksum = 0;
	    }

	    if (tstring[0]) {
		static char tstring_copy[24];
		strcpy(tstring_copy, tstring);
		setbasetime(tstring_copy);
	    }

	    if (0 > setheader(olrecname, si, nsig)) {
		fprintf(stderr, "%s: can't create layout header\n", pname);
		exit(2);
	    }

	    if (sflag == 0) {
		char *info;

		wfdbquiet();  /* Suppress errors from the WFDB library. */
		if (info = getinfo(irec))
		    do {
			putinfo(info);
		    } while (info = getinfo((char *)NULL));
		wfdbverbose();
		sflag = 1;
	    }
	}

	/* Figure out which input segments contain data to be copied */
	for (i = first_seg = 0, last_seg = maxseg - 1; i < maxseg; i++) {
	    t = seginfo[i].samp0;
	    tf = seginfo[i].samp0 + seginfo[i].nsamp;
	    if (t <= from && from < tf) first_seg = i;
	    if (t <= to && to <= tf) { last_seg = i; break; }
	}
	nseg = last_seg - first_seg + 1;
	if (seginfo[0].nsamp == 0) nseg++;
	orseg = malloc((strlen(nrec)+6) * sizeof(char));

	tf = seginfo[last_seg].samp0 + seginfo[last_seg].nsamp;
	if (to > tf)
	    to = tf;

	/* Start writing the master output header. */
	fprintf(ohfile, "%s/%d %d %.12g %ld", nrec, nseg, nsig, sfreq, to-from);
	if (tstring[0]) fprintf(ohfile, " %s", tstring);
	fprintf(ohfile, "\r\n");

	if (seginfo[0].nsamp == 0) {
	    fprintf(ohfile, "%s_layout 0\r\n", nrec);
	    nseg--;
	}

	for (i = 0, j = first_seg; j <= last_seg; j++) {
	    long len;
	    WFDB_Time start;

	    t = seginfo[j].samp0;
	    tf = seginfo[j].samp0 + seginfo[j].nsamp;
	    if (to < tf) /* this is the last segment with data to copy */
		tf = to;
	    if (from > t) {
		start = from - t; len = tf - from;
	    }
	    else {
		start = 0L; len = tf - t;
	    }
	    if (!strcmp(seginfo[j].recname, "~"))
		sprintf(orseg, "~");
	    else {
		sprintf(orseg, "%s_%04d", nrec, ++i);
		copy_sig(orseg, seginfo[j].recname, start, start + len, 0);
	    }
	    fprintf(ohfile, "%s %ld\r\n", orseg, len);
	}
	fclose(ohfile);
	return;
    }

    if (fmt <= 0) { /* no -O option, choose format based on ADC resolution */
	maxres = si[0].adcres;
	fmt = si[0].fmt;
	for (i = 1; i < nsig; i++) {
	    if (si[i].adcres > maxres)
		maxres = si[i].adcres;
	    if (si[i].fmt != fmt)
		fmt = 0;
	}
	if (fmt == 0) {
	    if (maxres > 24)
		fmt = 32;
	    else if (maxres > 16)
		fmt = 24;
	    else if (maxres > 12)
		fmt = 16;
	    else if (maxres > 10)
		fmt = 212;
	    else if (maxres > 8)
		fmt = 310;
	    else
		fmt = 80;
	}
    }

    /* Open the output signals. */
    (void)sprintf(ofname, "%s.dat", nrec);
    for (i = 0; i < nsig; i++) {
	si[i].fname = ofname;
	si[i].group = 0;
	si[i].fmt = fmt;
    }
    if (osigfopen(si, (unsigned)nsig) != nsig) exit(2);

    /* Copy the selected segment. */
    if (isigsettime(from) < 0) exit(2);
    wfdbquiet();
    nsamp = (to == 0L) ? -1L : to - from;
    while (nsamp == -1L || nsamp-- > 0L) {
	int stat = getvec(v);

	if (stat > 0 || nsamp == 0L) putvec(v);
	else break;
    }

    /* Clean up. */
    wfdbverbose();
    free(ofname);
    free(si);
    free(v);
    if (tstring[0]) setbasetime(tstring);
    (void)newheader(nrec);
}

void copy_info(char *irec, char *startp)
{
    char *info, *xinfo;

    wfdbquiet();	/* Suppress error messages from the WFDB library. */
    if (info = getinfo(irec))
	do {
	    (void)putinfo(info);
	} while (info = getinfo((char *)NULL));
    /* Append additional info summarizing what snip has done. */
    if (xinfo =
	malloc((strlen(pname)+strlen(irec)+strlen(startp)+50)* sizeof(char))) {
	(void)sprintf(xinfo,
		      "Produced by %s from record %s, beginning at %s",
		      pname, irec, startp);
	(void)putinfo(xinfo);
	free(xinfo);
    }
    wfdbverbose();
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
 "usage: %s -i IREC -n NREC [OPTIONS ...]\n",
 "where IREC is the name of the record to be converted, NREC is the name to",
 "be given to the record to be generated, and OPTIONS may include any of:",
 " -a ANNOTATOR [ANNOTATOR ...]  copy annotations for the specified ANNOTATOR",
 "              from IREC;  two or more ANNOTATORs may follow -a",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -l DURATION snip only DURATION (hh:mm:ss or sNNNN; overrides -t)",    
 " -m          preserve segments of multsegment input, if possible",
 " -O FORMAT   save output in the specified FORMAT (default: use best fit",
 "              to ADC resolution)",
 " -s          suppress output of info strings in header",
 " -t TIME     stop at specified time",
 "To change the sampling frequency, zero level, or gain, to select a",
 "subset of signals, or to re-order signals, use `xform'.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
