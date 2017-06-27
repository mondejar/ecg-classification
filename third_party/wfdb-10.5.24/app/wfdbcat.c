/* file: wfdbcat.c	G. Moody	3 March 2000
			Last revised:	22 May 2000
-------------------------------------------------------------------------------
wfdbcat: Find and concatenate WFDB files on the standard output
Copyright (C) 2000 George B. Moody

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

/* Structures used by internal WFDB library functions only */
struct netfile {
  char *url;
  char *data;
  int mode;
  long base_addr;
  long cont_len;
  long pos;
  long err;
  int fd;
};

struct WFDB_FILE {
  FILE *fp;
  struct netfile *netfp;
  int type;
};

typedef struct WFDB_FILE WFDB_FILE;

extern WFDB_FILE *wfdb_open(char *file_type, char *record, int mode);
#if WFDB_NETFILES
extern size_t wfdb_fread(void *ptr, size_t size, size_t nmemb, WFDB_FILE *fp);
extern int wfdb_fclose(WFDB_FILE *fp);
#else
#define wfdb_fread(p, s, n, wp)		fread(p, s, n, wp->fp)
#define wfdb_fclose(wp)			fclose(wp->fp)
#endif

#ifndef __STDC__
extern void exit();
#endif

#ifndef BUFSIZE
#define BUFSIZE 1024	/* bytes read at a time */
#endif

main(argc, argv)
int argc;
char *argv[];
{
    char *pname, *prog_name();
    int i = 0;
    size_t n;
    static char buf[BUFSIZE];
    WFDB_FILE *ifile;

    pname = prog_name(argv[0]);
    if (argc < 2 || (argc == 2 && strcmp(argv[1], "-h") == 0)) {
	(void)fprintf(stderr, "usage: %s FILE [FILE ...]\n", pname);
	exit(1);
    }

    for (i = 1; i < argc; i++) {
	if ((ifile = wfdb_open(argv[i], NULL, WFDB_READ)) == NULL)
	    (void)fprintf(stderr, "%s: `%s' not found\n", pname, argv[i]);
	else {
	    while ((n = wfdb_fread(buf, 1, sizeof(buf), ifile)) &&
		   (n == fwrite(buf, 1, n, stdout)))
	    	;
	    wfdb_fclose(ifile);
	}
    }

    exit(0);
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
