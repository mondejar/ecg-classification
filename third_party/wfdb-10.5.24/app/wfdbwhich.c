/* file: wfdbwhich.c	G. Moody	20 June 1990
			Last revised:	 5 May 1999

-------------------------------------------------------------------------------
wfdbwhich: Find a WFDB file and print its pathname
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

*/

#include <stdio.h>
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    char *filename, *pname, *prog_name();
    int status = 0;

    pname = prog_name(argv[0]);
    if (argc == 2 && strcmp(argv[1], "-h") == 0)
	argc = 0;	/* give usage instructions and quit */
    switch (argc) {
      case 2:
	if (filename = wfdbfile(argv[1], (char *)NULL))
	    (void)printf("%s\n", filename);
	else {
	    (void)fprintf(stderr, "`%s' not found\n", argv[1]);
	    (void)fprintf(stderr, "(WFDB path = %s)\n", getwfdb());
	    status = 2;
	}
	break;
      case 3:
	wfdbquiet();
	(void)isigopen(argv[2], NULL, 0);
	if (filename = wfdbfile(argv[1], argv[2]))
	    (void)printf("%s\n", filename);
	else {
	    (void)fprintf(stderr,
			  "No file of type `%s' found for record `%s'\n",
			  argv[1], argv[2]);
	    (void)fprintf(stderr, "(WFDB path = %s)\n", getwfdb());
	    status = 2;
	}
	break;
      default:
	if (argc != 4 || strcmp(argv[1], "-r") != 0) {
	    (void)fprintf(stderr, "usage: %s [-r RECORD ] FILENAME\n", pname);
	    (void)fprintf(stderr, " -or-  %s FILE-TYPE RECORD\n", pname);
	    status = 1;
	}
	else {
	    (void)isigopen(argv[2], NULL, 0);
	    if (filename = wfdbfile(argv[3], (char *)NULL))
		(void)printf("%s\n", filename);
	    else {
		(void)fprintf(stderr, "`%s' not found\n", argv[3]);
		(void)fprintf(stderr, "(WFDB path = %s)\n", getwfdb());
		status = 2;
	    }
	}
	break;
    }
    exit(status);
    /*NOTREACHED*/
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
