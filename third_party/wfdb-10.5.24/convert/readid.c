/* file: readid.c	G. Moody	8 August 1983
			Last revised:	 9 June 2005

-------------------------------------------------------------------------------
readid: Read AHA-format ID block, write record name, file lengths on stdout
Copyright (C) 1983-2005 George B. Moody

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
#ifdef __STDC__
#include <stdlib.h>
#else
extern void exit();
#endif

g16(fp)
FILE *fp;
{
    int h, l;

    l = fgetc(fp);
    h = fgetc(fp);
    return (((h << 8) & 0xff00) | (l & 0xff));
}

main(argc, argv)
int argc;
char *argv[];
{
    int dasize, ansize;
    long dcount, acount;
    static char buf[512], name[9];

    if (argc < 3) {
	(void)fprintf(stderr, "usage: %s data-block-size annot-block-size\n",
		      argv[0]);
	exit(1);
    }
    dasize = atoi(argv[1]);
    ansize = atoi(argv[2]);
    if (dasize < 1) dasize = 1;
    if (ansize < 1) ansize = 1;
    (void)fread(buf, 1, 24, stdin);
    (void)sscanf(buf, "%s", name);
    dcount = (g16(stdin) * 512L + dasize - 1) / dasize;
    (void)fread(buf, 1, 14, stdin);
    acount = (g16(stdin) * 512L + ansize - 1) / ansize;
    (void)printf("%s %ld %ld\n", name, dcount, acount);
    exit(0);	/*NOTREACHED*/
}
