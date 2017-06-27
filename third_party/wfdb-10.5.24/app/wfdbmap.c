/* file: wfdbmap.c	G. Moody       	22 March 2009
			Last revised:	15 November 2011

-------------------------------------------------------------------------------
wfdbmap: generates a 'plt' script to make a PostScript map of a WFDB record
Copyright (C) 2009-2011 George B. Moody

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

The file signal-colors.h in this directory defines the colors used by wfdbmap.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/wfdblib.h>
#include <wfdb/ecgcodes.h>
#include <wfdb/ecgmap.h>

char *pname;
WFDB_Anninfo *ai = NULL;
WFDB_Siginfo *si = NULL;
int mflag = 0;
int spm;

main(argc, argv)
int argc;
char *argv[];
{
    char *record = NULL, *prog_name();
    int i, j, length, **map = NULL, nann = 0, nsig;
    void help();
    void map_sig(char *record, WFDB_Siginfo *si, int nsig,int **map,int length);
    void map_ann(char *record, WFDB_Anninfo *ai, int nann,int **map,int length);
    void write_map(int **map, int nsig, int nann, int length);
    void write_script(int **map, int nsig, int nann, int length);

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotators follow */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotators must follow -a\n",
			      pname);
		exit(1);
	    }
	    /* how may annotators are there? */
	    for (j = i; j < argc && *argv[j] != '-'; j++)
		;
	    nann = j - i;
	    /* allocate *ai and initialize it */
	    if (nann > 0) {
		if ((ai = malloc(nann * sizeof(WFDB_Anninfo))) == NULL) {
		    fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		for (j = 0; j < nann; j++) {
		    ai[j].name = argv[i++];
		    ai[j].stat = WFDB_READ;
		}
	    }
	    i--;
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
          case 'm':     /* print map only */
	    mflag = 1;
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
    if (record == NULL) {
	help();
	exit(1);
    }

    nsig = isigopen(record, NULL, 0);
    if (nsig > 0) {
	if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	    fprintf(stderr, "%s: insufficient memory\n", pname);
	    if (ai) { wfdbquit(); free(ai); }
	    exit(2);
	}
	if ((nsig = isigopen(record, si, nsig)) < 0)
	    nsig = 0;
    }
    spm = strtim("60");
    length = (strtim("e") + (spm-1))/spm;  /* number of minutes in record */

    if (length < 1 && nsig > 0) {
        /* calculate the length of the record from the signal file size */
        char *p;
        double fs = 0;
	FILE *ifile;
	long bpm;

        for (i = 0; i < nsig && si[i].group == 0; i++)
	  switch (si[i].fmt) {
	  case 8:
	  case 80:  fs += si[i].spf; break;
	  case 310:
	  case 311: fs += 1.33333333*si[i].spf; break;
	  case 212: fs += 1.5*si[i].spf; break;
	  default:  fs += 2*si[i].spf; break;
	  }
	bpm = fs * spm + 0.5;  /* bytes per minute */
	p = wfdbfile(si[i].fname, NULL);
	ifile = fopen(p, "r");
	fseek(ifile, 0L, SEEK_END);
	length = ftell(ifile)/bpm;
	fclose(ifile);
    }

    if (length < 1) {	/* try to get length from annotation file(s) */
        int alen;
        WFDB_Annotation annot;
	WFDB_Time t;

        for (i = 0; i < nann; i++) {
	    if (annopen(record, &ai[i], 1) < 0) continue;
	    while (getann(0, &annot) >= 0)
	        t = annot.time;
	    alen = (t + (spm-1))/spm;
	    if (alen > length) length = alen;
	}
    }

    if (length > 60000) /* 1000 hours > 41 days */
        length = 60000;		/* truncate extremely long records */

    if (length < 1) {
	fprintf(stderr, "%s: can't map record %s (length unspecified)\n",
		pname, record);
	exit(1);
    }

    if ((map = malloc((nsig + 4*nann) * sizeof(int *))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	wfdbquit();
	if (ai) free(ai);
	if (si) free(si);
	exit(2);
    }
    for (i = 0; i < nsig + 4*nann; i++)
	if ((map[i] = calloc(length, sizeof(int))) == NULL) {
	    fprintf(stderr, "%s: insufficient memory\n", pname);
	    while (--i > 0)
		if (map[i]) free(map[i]);
	    free(map);
	    wfdbquit();
	    if (ai) free(ai);
	    if (si) free(si);
	    exit(2);
	}

    if (nsig > 0) map_sig(record, si, nsig, map, length);
    if (nann > 0) map_ann(record, ai, nann, &map[nsig], length);

    if (mflag) {
	printf("T");
	for (i = 0; i < nsig; i++)
	    printf("\t%s", si[i].desc);
	for (i = 0; i < nann; i++)
	    printf("\t%s\tQRS\tEctopic\tVE", ai[i].name);
	printf("\n");
	write_map(map, nsig, nann, length);
    }
    else
	write_script(map, nsig, nann, length);
	
    for (i = 0; i < nsig + 4*nann; i++)
	free(map[i]);
    free(map);
    wfdbquit();
    if (ai) free(ai);
    if (si) free(si);

    exit(0);
}

void map_sig(char *record, WFDB_Siginfo *si, int nsig, int **map, int length)
{
    if (si[0].nsamp == strtim("e") || si[0].nsamp != 0) {
	/* fixed-layout record */
        int i, t;
	for (i = 0; i < nsig; i++)
	    for (t = 0; t < length; t++)
		map[i][t] = 1;
    }
    else {	/* variable-layout record */
	char buf[256], *d, *p, *q, *r, *hfname, *shfname;
	long m, m0, mf, spm = strtim("60"), t = 0, tf;
	WFDB_FILE *ifile, *sfile;

	p = wfdbfile("hea", record);
	hfname = calloc(strlen(p) + 1, 1);
	strcpy(hfname, p);
	if ((ifile = wfdb_fopen(hfname, "r")) == NULL) {
	    fprintf(stderr, "%s: can't open %s\n", pname, hfname);
	    free(hfname);
	    return;
	}
	for (d = p + strlen(p); d > p; d--)
	    if (*(d-1) == '/') {
		*d = '\0';
		break;
	    }
	shfname = calloc(strlen(hfname) + 16, 1);
	strcpy(shfname, p);
	d = shfname + strlen(shfname);   /* d points to first char after '/' */
	wfdb_fgets(buf, sizeof(buf), ifile);  /* read and ignore two lines */
	wfdb_fgets(buf, sizeof(buf), ifile);

	m0 = 0;
	while (wfdb_fgets(buf, sizeof(buf), ifile)) {/* read a segment desc */ 
	    char *tp;

	    if (buf[0] == '~') {  /* segment is null (all signals off) */
		t += atol(buf+2);
		m0 = t/spm;
		continue;
	    }
	    for (tp = buf+1; *tp != ' '; tp++)
		; 
	    *tp = '\0';
	    tf = t + atol(tp+1);
	    if ((mf = tf/spm) > length) mf = length;
	    sprintf(d, "%s.hea", buf);
	    if (sfile = wfdb_fopen(shfname, "r")) {/* open the segment header */
		char sbuf[256];
		int i;

		wfdb_fgets(sbuf, sizeof(sbuf), sfile);/* read & ignore a line */
      
		while ((p = wfdb_fgets(sbuf, sizeof(sbuf), sfile)) &&
		       *sbuf != '#') {
		    /* signal description line */
		    for (q = sbuf, i = 0; *q; q++)
			if (*q == ' ' && ++i == 8) break;
		    q++;
		    *(q + strlen(q) - 2) = '\0';
		    for (i = 0; i < nsig; i++)
			if (strcmp(si[i].desc, q) == 0)
			    for (m = m0; m < mf; m++)
				map[i][m] = 1;
		}
		wfdb_fclose(sfile);
		t = tf;
		m0 = t/spm;
	    }
	}
	wfdb_fclose(ifile);
	free(shfname);
	free(hfname);
    }
}

int *namax;

void map_ann(char *record, WFDB_Anninfo *ai, int nann, int **map, int length)
{
    int i;

    namax = malloc(nann * sizeof(int));
    wfdbquiet();   /* suppress warnings if an annotator can't be opened */
    for (i = 0; i < nann; i++) {
	char tstring[10];
	int minutes, *na, *nq, *ne, *nv;
	WFDB_Annotation annot;
	WFDB_Time end_of_epoch;

	na = &map[4*i][0];
	namax[i] = 1;

	/* Open the annotators one at a time, and do not quit if any fail
	   to open. */
	if (annopen(record, &ai[i], 1) < 0) continue;
	minutes = end_of_epoch = 0;
	iannsettime(1L);
	while (getann(0, &annot) >= 0) {
	    if (annot.time > end_of_epoch) {
	        if (*na > namax[i]) namax[i] = *na;
		minutes = annot.time/spm;
		na = &map[4*i][minutes];
		nq = &map[4*i + 1][minutes];
		ne = &map[4*i + 2][minutes];
		nv = &map[4*i + 3][minutes];
		if (++minutes > length)
		    break;	/* stop at the end of the record */
		sprintf(tstring, "%d", 60*minutes);
		end_of_epoch = strtim(tstring);
	    }
	    switch (annot.anntyp) {
	    case PVC:
	    case FUSION:
	    case VESC:
	    case RONT:
	    case FLWAV:	(*nv)++;	/* fall through, no break! */
	    case APC:
	    case ABERR:
	    case NPC:
	    case SVPB:
	    case NESC:
	    case AESC:
	    case SVESC:	(*ne)++;	/* fall through, no break! */
	    case NORMAL:
	    case LBBB:
	    case RBBB:
	    case PACE:
	    case UNKNOWN:
	    case BBB:
	    case LEARN:
	    case PFUS:	(*nq)++;	/* fall through, no break! */
	    default:	(*na)++; break;
	    }
	}
	if (*na > namax[i]) namax[i] = *na;
    }
    return;
}

void write_map(int **map, int nsig, int nann, int length)
{
    int i, imax = nsig + 4*nann - 1, t;

    for (t = 0; t < length; t++) {
	if (mflag == 0) printf("%d\t0\t", t); /* x0, y0 for histogram plots */
	printf("%d\t", t+1);
	for (i = 0; i < imax; i++)
	    printf("%d\t", map[i][t]);
	printf("%d\n", map[i][t]);
    }
}

#include "signal-colors.h"

char *color(char *type)
{
    int i;

    for (i = 0; ctab[i].name != NULL; i++)
	if (strcmp(type, ctab[i].name) == 0) return (ctab[i].color);
    return ("grey");
}

void write_script(int **map, int nsig, int nann, int length)
{
  char *p, tstring[2][30];
    double ylow, ymax;
    int i, j;
    WFDB_Time tf = strtim("e");

    strcpy(tstring[0], timstr(0));
    for (p = tstring[0]; *p == ' '; p++)
        ;
    if (tf) strcpy(tstring[1], timstr(-tf));
    else sprintf(tstring[1], "%d:%02d:00", length/60, length%60);

    ymax = 2*(nsig + 2*nann + 1);
    ylow = 0.01 * ymax;

    printf("#! /bin/sh\n"
	   "cat >map.txt <<EOF\n");
    write_map(map, nsig, nann, length);
    printf("EOF\n\n");

    printf("MAP=map.txt\n"
	   "FDEF=\"-sf l Fh,P8\"\n"
	   "LDEF=\"-setxy -l %g 0 RB\"\n"
	   "WDEF=\"-W .048 .1 .982 1\"\n"
	   "XDEF=\"-X 0 %d\"\n"
	   "OPT=\"$FDEF $XDEF $WDEF $LDEF\"\n",
	   -0.005*length, length);
    
    printf("lwplt $WDEF $XDEF -Y 0 %g -stxy -sf l Fh-b,P12", ymax);
    printf(" -l 0 %g LT \"%s\"", ylow, p);
    printf(" -l %g %g CT \"time\"", 0.5*length, ylow);
    printf(" -l %d %g RT \"%s\"\n\n", length, ylow, tstring[1]);

    for (i = 0; i < nann; i++) {
	j = nsig + i;
	printf("lwplt $MAP -p\"0,1,2,%dh(C%s)\" -Y %g %g $OPT %s\n",
	       j+3*i+3, "grey", 0.5*namax[i]*(2*(j+i+2) - ymax),
	       0.5*namax[i]*2*(j+i+2), ai[i].name);
	printf("lwplt $MAP -p\"0,1,2,%dh(C%s)\" -Y %g %g $OPT %s\n",
	       j+3*i+4, "black", 0.5*namax[i]*(2*(j+i+2) - ymax),
	       0.5*namax[i]*2*(j+i+2), ai[i].name);
	printf("lwplt $MAP -p\"0,1,2,%dh(C%s)\" -Y %g %g $OPT %s\n",
	       j+3*i+5, "blue", 0.5*namax[i]*(2*(j+i+2) - ymax),
	       0.5*namax[i]*2*(j+i+2), ai[i].name);
	printf("lwplt $MAP -p\"0,1,2,%dh(C%s)\" -Y %g %g $OPT %s\n",
	       j+3*i+6, "red", 0.5*namax[i]*(2*(j+i+2) - ymax),
	       0.5*namax[i]*2*(j+i+2), ai[i].name);
    }

    for (i = 0; i < nsig; i++)
	printf("lwplt $MAP -p\"0,1,2,%dh(C%s)\" -Y %g %g $OPT \"%s\"\n",
	       i+3, color(si[i].desc), 2.*(i+1) - ymax,
	       2.*(i+1), si[i].desc);
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
 " -h       print this usage summary",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
