/* file: wfdbsignals.c	G. Moody	  June 1989
			Last revised:   15 November 2011

-------------------------------------------------------------------------------
wfdbdesc: List signals in a WFDB record
Copyright (C) 1989-2011 George B. Moody

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

This program is a drastically pruned version of wfdbdesc.

*/

#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    char *info, *p, *pname, *prog_name();
    int i, msrec = 0, nsig;
    FILE *ifile;
    WFDB_Siginfo *s;
    WFDB_Time t;

    pname = prog_name(argv[0]);
    if (argc < 2) {
        (void)fprintf(stderr, "usage: %s RECORD [-readable]\n", pname);
        exit(1);
    }
    /* Discover the number of signals defined in the header. */
    if ((nsig = isigopen(argv[1], NULL, 0)) < 0) exit(2);

    /* Allocate storage for nsig signal information structures. */
    if (nsig > 0 && (s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }

    /* If the `-readable' option is given, report only on signals which can
       be opened.  Otherwise, report on all signals named in the header file,
       without attempting to open them. */
    if (nsig > 0 && argc > 2 &&
	strncmp(argv[2], "-readable", strlen(argv[2])) == 0)
	nsig = isigopen(argv[1], s, nsig);
    else if (nsig > 0)
	nsig = isigopen(argv[1], s, -nsig);

    for (i = 0; i < nsig; i++)
        (void)printf("%s\n", s[i].desc);
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
