/* file: time2sec.c	G. Moody	9 July 2003
			Last revised:  14 March 2009
-------------------------------------------------------------------------------
time2sec: Convert a string in WFDB standard time format to time in seconds

Copyright (C) 2003-2009 George B. Moody

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

main(argc, argv)
int argc;
char **argv;
{
    double t;
    int milliseconds;
    long seconds;

    if (argc == 4 && strcmp(argv[1], "-r") == 0) {
	(void)isigopen(argv[2], NULL, 0);
	t = strtim(argv[3]);
	if (t < 0) t = -t;
	seconds = (long)(t/sampfreq(NULL));
	t -= (double)(seconds*sampfreq(NULL));
	milliseconds = (long)((t*1000.)/sampfreq(NULL) + 0.5);
	if (milliseconds >= 1000) { seconds++; milliseconds -= 1000; }
	printf("%ld", seconds);
	if (milliseconds)
	    printf(".%03d", milliseconds);
	printf("\n");
	//	printf("%.12g\n", t/sampfreq(NULL));
	exit(0);
    }
    else if (argc == 2 && argv[1][0] != '-') {
	setsampfreq(1000.0);
	printf("%.12g\n", strtim(argv[1])/1000.0);
	exit(0);
    }

    fprintf(stderr, "usage: %s [-r RECORD] TIME\n", argv[0]);
    fprintf(stderr, " where TIME (in hh:mm:ss format) is a time interval\n");
    fprintf(stderr, " to be converted into seconds.  Enclose TIME in\n");
    fprintf(stderr, " square brackets (e.g., [9:0:0]) to convert a time\n");
    fprintf(stderr, " of day to the elapsed time in seconds from the\n");
    fprintf(stderr, " beginning of the (optionally) specified RECORD.\n");
    exit(1);
}
