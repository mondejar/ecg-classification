/* file: rdedfann.c	G. Moody	 14 March 2008
   			Last revised:	  5 March 2014

-------------------------------------------------------------------------------
rdedfann: Print annotations from an EDF+ file
Copyright (C) 2008-2014 George B. Moody

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

This program prints the annotations from an EDF+ file in the same format as
'rdann' does for WFDB-compatible annotation files.  Note, however, that the
annotation mnemonics in EDF+ files do not in general match those used in
WFDB-compatible annotation files, so that it will usually be necessary to
translate those that come from EDF+ file before the text can be converted
by 'wrann'.  For example, this command can be used to extract annotations
from 'foo.edf', change the EDF+ annotation type "QRS" to the WFDB type "N",
and then produce a WFDB-compatible annotation file 'foo.edf.qrs':
    rdedf -r foo.edf | sed "s/QRS/  N" | wrann -r foo.edf -a qrs
*/

#include <stdio.h>
#include <wfdb/wfdb.h>

char *pname;
double sfreq = 0.0;
int state;

main(int argc, char **argv)
{
    char *record = NULL, *prog_name();
    int aindex = 0, alen = 0, framelen = 0, i, nsig, s, vflag = 0, xflag = 0;
    WFDB_Sample *frame;
    WFDB_Siginfo *si;
    void help();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'F':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: sampling frequency must follow -F\n",
			      pname);
		exit(1);
	    }
	    sscanf(argv[i], "%lf", &sfreq);
	    if (sfreq <= 0.0) sfreq = 1.0;
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 'v':	/* verbose output -- include column headings */
	    vflag = 1;
	    break;
	  case 'x':	/* save EDF annotation text in aux rather than anntyp */
	    xflag = 1;
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
    if (record == NULL) {
	help();
	exit(1);
    }
    setgvmode(WFDB_HIGHRES);
    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, si, nsig)) <= 0)
	exit(2);
    for (i = framelen = 0; i < nsig; i++) {
	if (strcmp(si[i].desc, "EDF Annotations") == 0) {
	    aindex = framelen;
	    alen = si[i].spf;
	}
	framelen += si[i].spf;
    }
    if (alen == 0) {
	(void)fprintf(stderr, "%s: record %s has no EDF annotations\n",
		      pname, record);
	(void)free(si);
	wfdbquit();
	exit(3);
    }
    if ((frame = (int *)malloc((unsigned)framelen*sizeof(WFDB_Sample)))==NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	(void)free(si);
	exit(2);
    }

    if (sfreq > 0.0) {
	setgvmode(WFDB_LOWRES);
	setsampfreq(sfreq);
    }
    else 
	sfreq = sampfreq(NULL);
	

    /* Print column headers if '-v' option selected. */
    if (vflag)	
	(void)printf("      Time   Sample #  Type  Sub Chan  Num\tAux\n");

    while (getframe(frame) > 0) {
	WFDB_Sample *p;

	state = 0;
	for (i = 0, p = (frame + aindex); i < alen; i++, p++) {
	    if (*p || state) {
		proc(*p, xflag);
		proc(*p >> 8, xflag);
	    }
	    else
		break;
	}
    }

    (void)free(frame);
    (void)free(si);
    wfdbquit();
    exit(0);	/*NOTREACHED*/
}

proc(int x, int xflag)
{
    static char onset[1024], duration[1024], text[1024];
    static char *onsetp, *durationp, *textp;

    x &= 0xff;

    switch (state) {
	case 0:		/* looking for a new TAL */
	    if (x == '\0')
		break;
	    if (x != '+')
		fprintf(stderr, "%s: unexpected character '%c' in input\n",
			pname, x);
	    else {
		state = 1;
		onsetp = onset;
		durationp = duration;
		textp = text;
		*textp = 0;
	    }
	    break;
	case 1:		/* accumulate characters from onset until 024 or 025 */
	    if (x == '\025') {	/* end of onset, start of duration */
		x = 0;
		state = 2;
	    }
	    else if (x == '\024') {	/* end of onset, start of annotation */
		x = 0;
		state = 3;
	    }
	    *(onsetp++) = x;
	    break;
	case 2:   	/* accumulate characters from duration until 024 */
	    if (x == '\024') {	/* end of onset, start of annotation */
		x = 0;
		state = 3;
	    }
	    *(durationp++) = x;
	    break;
	case 3:		/* accumulate characters from annotation until 024 */
	    if (x == '\024') {	/* end of annotation */
		x = 0;
		state = 4;
	    }
	    *(textp++) = x;
	    break;
	case 4:		/* annot just ended, there may be another */
	    if (text[0]) {	/* the annotation was not empty -- output it */
		long t = (long)(atof(onset) * sfreq + 0.5);

		/* replace whitespace with '_' */
		for (textp = text; *textp; textp++)
		    if (*textp == ' ' || *textp == '\t') *textp = '_';
		printf("%s  %7ld %5s%5d%5d%5d\t%s",
		       t ? mstimstr(t) : "    0:00.000", t,
		       xflag ? "\"" : text, 0, 0, 0, xflag ? text : "");
		if (duration[0])
		    printf("%cduration: %s", xflag ? ' ' : '\t', duration);
 		printf("\n");
	    }
	    if (x) {
		text[0] = x;
		textp = text + 1;
		state = 3;
	    }
	    else		/* end of TAL */
		state = 0;
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
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the input EDF+ record, and OPTIONS may include:",
 " -F FREQ     set the sampling frequency to FREQ Hz\n",
 " -h          print this usage summary",
 " -v          print column headings",
 " -x          print EDF+ annotation text in aux (default: print in anntyp)",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
