/* file: md2a.c		G. Moody	19 July 1983
			Last revised:   26 April 2001

-------------------------------------------------------------------------------
md2a: Convert MIT format signal file(s) to AHA DB distribution tape format
Copyright (C) 2001 George B. Moody

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

This program converts the signal file(s) of a two-signal DB record into AHA DB
distribution tape format (format 16, multiplexed signals, with a special `end
of data' marker after the last sample).  It can also convert DB records with
other numbers of signals into the same format, but AHA DB distribution tape
format is defined only for two-signal records.

This program is provided as a convenience for those who may have software that
requires files in AHA tape format as input.  It does *not* generate files in
the format currently used for distribution of the AHA database on floppy disk;
program `ad2m.c' can perform the reverse conversion, however.
*/

#include <stdio.h>
#ifndef __STDC__
#ifndef MSDOS
extern void exit();
#endif
#endif

#include <wfdb/wfdb.h>

#define EODF	0100000	/* AHA data file end-of-data marker */
#define NSAMP	525000L	/* default number of samples per signal */

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *nrec = NULL, *ofname, *record = NULL, *prog_name();
    int i, nsig, x[WFDB_MAXSIG];
    static WFDB_Siginfo s[WFDB_MAXSIG];
    void help();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'h':	/* help requested */
	    help();
	    exit(1);
	    break;
	  case 'n':	/* new record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: new record name must follow -n\n", pname);
		exit(1);
	    }
	    nrec = argv[i];
	    break;
	  case 'o':	/* output file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		      "%s: name of AHA-format output file must follow -o\n",
			      pname);
		exit(1);
	    }
	    ofname = argv[i];
	    break;
	  case 'r':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
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
    if (ofname == NULL || record == NULL) {
	help();
	exit(1);
    }

    /* Give up unless at least one input signal is readable. */
    if ((nsig = isigopen(record, s, WFDB_MAXSIG)) < 1)
	exit(2);  

    /* Check that the new record name is legal before proceeding.  Note that
       this step creates a header file in the current directory which is
       overwritten below, after the processing is completed. */
    if (nrec != NULL && newheader(nrec) < 0) exit(3);

    /* Set the output signal specifications. */
    for (i = 0; i < nsig; i++) {
	s[i].fname = ofname;
	s[i].desc = "";
	s[i].group = 0;
	s[i].fmt = 16;
    }
    
    /* Give up if the output cannot be written. */
    if (osigfopen(s, (unsigned)nsig) < nsig) exit(4);

    /* Copy the input to the output.  Reformatting is handled by putvec(). */
    while (getvec(x) == nsig && putvec(x) == nsig)
	;

    /* Write the new header file if requested. */
    if (nrec) (void)newheader(nrec);

    /* Write the end-of-data marker. */
    for (i = 0; i < nsig; i++)
	x[i] = EODF;
    (void)putvec(x);

    /* Clean up. */
    wfdbquit();
    exit(0);	/*NOTREACHED*/
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
    "usage: %s -o AHAFILE -r RECORD [ OPTIONS ... ]\n",
    "where AHAFILE is the name of the AHA-format output signal file,",
    "RECORD is the record name, and OPTIONS may include:",
    " -h         print this usage summary",
    " -n NEWREC  create a header file for the output signal file, so it",
    "             may be read as record NEWREC",
    NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
