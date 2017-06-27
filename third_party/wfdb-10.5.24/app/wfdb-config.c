/* file: wfdb-config.c	G. Moody	27 April 2002
			Last revised:	14 August 2014	wfdblib 10.5.24

-------------------------------------------------------------------------------
wfdb-config: Print WFDB library version and linking information
Copyright (C) 2002-2009 George B. Moody

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

/* Expand command substitutions in VAR; needed if libcurl is included
   in LDFLAGS, so that we invoke the curl-config program rather than
   simply printing "`curl-config --libs`". */
static void expand_var(const char *var)
{
    char *buffer;
    SUALLOC(buffer, strlen(var) + 6, 1);
    sprintf(buffer, "echo %s", var);
    system(buffer);
    SFREE(buffer);
}

char *pname;

main(int argc, char **argv)
{
    char *filename, *prog_name(char *s);
    int i;
    void help(void);

    pname = prog_name(argv[0]);
    if (argc == 1) {
	help();
	exit(1);
    }

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "--version") == 0)
	    printf("%s\n", wfdbversion());
	else if (strcmp(argv[i], "--libs") == 0)
	    expand_var(wfdbldflags());
	else if (strcmp(argv[i], "--cflags") == 0)
	    expand_var(wfdbcflags());
	else
	    help();
    }
    exit(0);
}

char *prog_name(char *s)
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
    "usage: %s [--version] [--libs] [--cflags]\n",
    NULL
};

void help(void)
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
