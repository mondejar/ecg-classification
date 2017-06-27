/* file wrann.c		G. Moody	 6 July 1983
			Last revised:	27 July 2010

-------------------------------------------------------------------------------
wrann: Translate an ASCII file in 'rdann' output format to an annotation file
Copyright (C) 1983-2010 George B. Moody

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
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;

main(argc, argv)	
int argc;
char *argv[];
{
    static WFDB_Anninfo ai;
    WFDB_Annotation annot;
    static char line[400];
    char annstr[10], *p, *record = NULL, *prog_name();
    int i, sub, ch, nm;
    long tm;
    void help();

    pname = prog_name(argv[0]);

    /* Interpret command-line options. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator must follow -a\n",
			      pname);
		exit(1);
	    }
	    ai.name = argv[i];
	    break;
	  case 'h':	/* print usage summary and quit */
	    help();
	    exit(0);
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
    if (record == NULL || ai.name == NULL) {
	help();
	exit(1);
    }

    setafreq(sampfreq(record));
    ai.stat = WFDB_WRITE;
    if (annopen(record, &ai, 1) < 0)	/* open annotation file */
	exit(2);
    while (fgets(line, sizeof(line), stdin) != NULL) {
	static char a[256], *ap;
	int auxlen;

	p = line+9;
	if (line[0] == '[')
	    while (*p != ']')
		p++;
	while (*p != ' ')
	    p++;
	*(a+1) = '\0';
	(void)sscanf(p+1, "%ld%s%d%d%d", &tm, annstr, &sub, &ch, &nm);
	annot.anntyp = strann(annstr);
	if (line[0] == '[') {
	    annot.time = -strtim(line);
	    if (annot.time < 0L) continue;
	}
	else
	    annot.time = tm;
	annot.subtyp = sub; annot.chan = ch; annot.num = nm;
	/* If annstr does not contain a recognizable mnemonic, write a NOTE
	   annotation and put annstr at the beginning of the aux field. */
	if (annot.anntyp == NOTQRS)
	    annot.anntyp = NOTE;
	else
	    *annstr = '\0';
	if (ap = strchr(p+1, '\t')) {	/* check if auxinfo at end of line */
		if (strlen(annstr) + strlen(ap) + 2 > sizeof(a))
		ap[sizeof(a)-strlen(annstr)-3] = '\0';
	    else
		ap[strlen(ap)-1] = '\0';     /* else discard trailing newline */
	}
	/* Create an aux string if needed. */
	if (*annstr || ap) {
	    if (*annstr && ap)
		sprintf(a+1, "%s %s", annstr, ap+1);
	    else if (*annstr)
		sprintf(a+1, "%s", annstr);
	    else
		sprintf(a+1, "%s", ap+1);
	    *a = strlen(a+1);
	    annot.aux = a;
	}
	else
	    annot.aux = NULL;
	(void)putann(0, &annot);
    }
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

void help()
{
    (void)fprintf(stderr, "usage: %s -r RECORD -a ANNOTATOR <TEXT-FILE\n",
		  pname);
    (void)fprintf(stderr,
		  "TEXT-FILE should be in the format produced by `rdann'.\n");
}
