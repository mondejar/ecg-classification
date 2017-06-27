/* file: soelim.c	G. Moody	25 January 1995
			Last revised:	 29 June 2005
-------------------------------------------------------------------------------
soelim: expands .so requests in [nt]roff source files
Copyright (C) 1995-2005 George B. Moody

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

usage: soelim [ file ... ]

This version of `soelim' emulates the SunOS/Solaris program of the same name.
It concatenates the files named in its argument list on the standard output,
replacing any lines beginning with `.so' by the contents of the files named on
those lines.  Included files may be nested to a depth of MAXLEVEL (this
restriction exists only to avoid infinite recursion that might otherwise result
from improperly structured inputs).  `soelim' copies the standard input if no
files are named, or if the token `-' appears in the argument list.
*/

#include <stdio.h>
#if defined(__STDC__) || defined(_WINDOWS)
#include <stdlib.h>
#else
extern void exit();
#endif
#ifndef BSD
# include <string.h>
#else		/* for Berkeley UNIX only */
# include <strings.h>
#endif

#define MAXLEVEL 100

void soelim();

main(argc, argv)
int argc;
char *argv[];
{
    int i;

    if (argc < 1)
	soelim("-");
    else 
	for (i = 1; i < argc; i++)
	    soelim(argv[i]);
    exit(0);
}

void soelim(filename)
char *filename;
{	
    FILE *ifile;
    char buf[256], *p, *q;
    long line = 1;
    int atbol = 1;
    static int level;

    if (filename == NULL || *filename == '\0') {
	fprintf(stderr, "soelim: empty `.so' statement (ignored)\n");
	return;
    }
    else if (level > MAXLEVEL) {
	fprintf(stderr, "soelim: files are nested too deeply\n");
	return;
    }
    else if (strcmp(filename, "-") == 0)
	ifile = stdin;
    else if ((ifile = fopen(filename, "rt")) == NULL) {
	fprintf(stderr, "soelim: can't open `%s'\n", filename);
	return;
    }
    level++;
    while (fgets(buf, sizeof(buf), ifile)) {
	p = buf + strlen(buf) - 1;	/* p points to the last char in buf */
	if (atbol && strncmp(buf, ".so ", 4) == 0) {
	    if (*p == '\n')
		soelim(strtok(buf+4, " \t\n"));  /* copy the named file */
	/* We assume that very long lines beginning with .so are corrupted. */
	    else {
		buf[20] = '\0';
		fprintf(stderr, "%s (%ld): bogus filename `%s...'\n",
			filename, line, buf);
		fprintf(stderr, " (flushing remainder of input line)\n");
		while (fgets(buf, sizeof(buf), ifile))
		    if (buf[strlen(buf) - 1] == '\n')
			break;
	    }
	}
	else {
	    fputs(buf, stdout);
	    if (*p == '\n') {	/* count as a complete line */
		line++;
		atbol = 1;
	    }
	    else
		atbol = 0;
	}
    }
    fclose(ifile);
    level--;
}
