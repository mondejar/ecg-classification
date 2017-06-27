/* file: wfdbtime.c	G. Moody	16 February 2009

-------------------------------------------------------------------------------
wfdbtime: convert to and from sample number, elapsed time, and absolute time
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

*/

#include <stdio.h>
#include <wfdb/wfdb.h>

main(int argc, char **argv)
{
    char *record = NULL, sbuf[32];
    int i;
    WFDB_Time t;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-r") == 0) {
	    if (record) wfdbquit();
	    setgvmode(WFDB_HIGHRES);
	    if (++i == argc) break;
	    record = argv[i];
	    if (isigopen(record, NULL, 0) < 0) exit(2);
	}
	else if (record) {
	    if ((t = strtim(argv[i])) < 0L) t = -t;
	    sprintf(sbuf, "s%ld", t);
	    printf("%15s\t%15s", sbuf, (t == 0L) ? "0:00.000" : mstimstr(t));
	    printf("\t%15s\n", mstimstr(-t));
	}
    }
    if (record == NULL) {
	fprintf(stderr, "usage: %s [ -r RECORD [ TIME ] ]\n", argv[0]);
	fprintf(stderr, " where RECORD is the name of the input record\n");
	fprintf(stderr, " Each TIME is a time to be converted\n");
	exit(1);
    }
    wfdbquit();
    exit(0);
}
