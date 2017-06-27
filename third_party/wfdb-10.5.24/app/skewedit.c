/* file: skewedit.c	G. Moody	10 August 1994
			Last revised:  14 November 2002

-------------------------------------------------------------------------------
skewedit: Edit skew fields of header file(s)
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

*/

#include <stdio.h>
#include <wfdb/wfdb.h>

static int nskews, *skew;

main(argc, argv)
int argc;
char *argv[];
{
    char buf[256], *hfname, *p;
    FILE *hfile;
    int i;
    void skewedit();

    if (argc < 3) {
	fprintf(stderr, "usage: %s record skew0 [skew1 ... skewN]\n", argv[0]);
	fprintf(stderr,
" If `record' contains multiple segments, the headers for each segment are\n");
	fprintf(stderr," rewritten.\n");
	fprintf(stderr,
" The `skew0', `skew1', etc. arguments are skews (in samples) for each signal,\n");
	fprintf(stderr,
	    " beginning with signal 0.\n");
	exit(1);
    }

    nskews = argc-2;
    if ((skew = malloc(nskews * sizeof(int))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", argv[0]);
	exit(2);
    }
    for (i = 2; i < argc; i++) {
	if ((skew[i-2] = atoi(argv[i])) < 0) {
	    fprintf(stderr, "%s: skew cannot be less than zero\n", argv[0]);
	    exit(1);
	}
    }

    if ((hfname = wfdbfile("header", argv[1])) == NULL) {
	fprintf(stderr, "%s: can't find header for record %s\n", argv[0],
		argv[1]);
	exit(2);
    }

    if ((hfile = fopen(hfname, "rt")) == NULL) {
	fprintf(stderr, "%s: can't read `%s'\n", argv[0], hfname);
	exit(2);
    }

    fgets(buf, 256, hfile);
    for (p = buf; *p; p++)
	if (*p == ' ' || *p == '/' || *p == '\t' || *p == '\n' || *p == '\r')
	    break;
    if (*p == '/') {		/* this is a multi-segment record */
	char *shfname;
	FILE *shfile;
	int nseg = atoi(p+1);

	for (i = 0; i < nseg; i++) {
	    fgets(buf, 256, hfile);
	    for (p = buf; *p && *p != ' '; p++)
		;
	    *p = '\0';
	    if ((shfname = wfdbfile("header", buf)) == NULL) {
		fprintf(stderr, "%s: can't find header for segment %s\n",
			argv[0], buf);
		continue;
	    }
	    if ((shfile = fopen(shfname, "rt")) == NULL) {
		fprintf(stderr, "%s: can't read `%s'\n", argv[0], shfname);
		continue;
	    }
	    skewedit(buf, shfile);
	}
    }
    else {
	rewind(hfile);
	skewedit(argv[1], hfile);
    }
    exit(0);
}

void skewedit(record, hfile)
char *record;
FILE *hfile;
{
    char buf[256], tmpfname[20], hfname[20], *p;
    int i = 0;
    FILE *tfile;

    sprintf(tmpfname, "%s.tmp", record);
    if ((tfile = fopen(tmpfname, "wt")) == NULL) {
	fprintf(stderr, "can't create temporary file `%s'\n", tmpfname);
	return;
    }
    do {
	fgets(buf, 256, hfile);
	fputs(buf, tfile);
    } while (strncmp(buf, record, strlen(record)));
    while (fgets(buf, 256, hfile)) {
	if (buf[0] == '#' || buf[0] == '\n') {
	    fprintf(tfile, "%s", buf);
	    continue;
	}
	for (p = buf; *p != ' '; p++)
	    ;
	for (p = p+1; *p != ' ' && *p != ':'; p++)
	    ;
	if (*p == ':') {
	    *p++ = '\0';
	    while (*p++ != ' ')
		;
	}
	else
	    *p++ = '\0';
	if (i < nskews && skew[i])
	    fprintf(tfile, "%s:%d %s", buf, skew[i], p);
	else
	    fprintf(tfile, "%s %s", buf, p);
	i++;
    }
    fclose(tfile);
    fclose(hfile);
    sprintf(hfname, "%s.hea", record);
    if (rename(tmpfname, hfname))
	fprintf(stderr, "can't rename `%s' as `%s'\n", tmpfname, hfname);
}
