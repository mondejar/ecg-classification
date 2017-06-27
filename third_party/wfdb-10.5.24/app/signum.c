/* file: signum.c	G. Moody	 9 April 2008

-------------------------------------------------------------------------------
signum: Print signal numbers for signals named as arguments
Copyright (C) 2008 George B. Moody

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

char *pname;

main(int argc, char **argv)
{
    char *record = NULL, *prog_name();
    int found, i, isiglist = 0, j, nsig, nosig = 0;
    WFDB_Siginfo *si;
    void help();

    pname = prog_name(argv[0]);

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
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
	  case 's':	/* signal list follows */
	    isiglist = i+1; /* index of 1st argument containing a signal name */
	    while (i+1 < argc && *argv[i+1] != '-') {
		i++;
		nosig++;	/* number of elements in signal list */
	    }
	    if (nosig == 0) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
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
    if (record == NULL || isiglist == 0) {
	help();
	exit(1);
    }
    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, si, -nsig)) <= 0) exit(2);
    for (i = 0; i < nosig; i++) {
	found = 0;
	for (j = 0; j < nsig; j++) {
	    if (found) printf("\t");
	    if (si[j].desc && (strcmp(argv[isiglist+i], si[j].desc) == 0)) {
		printf("%d", j);
		break;
	    }
	    found = 1;
	}
	if (!found) printf("X");
	printf("\n");
    }
    wfdbquit();
    exit(0);
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
 "where RECORD is the name of the input record, and OPTIONS may include:",
 " -h          print this usage summary",
 " -s SIGNAL [SIGNAL ...]  print signal number(s) for the specified signal(s)",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
