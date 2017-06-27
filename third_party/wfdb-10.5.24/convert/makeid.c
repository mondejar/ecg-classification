/* file: makeid.c	G. Moody	19 July 1983
			Last revised:    5 June 2003

-------------------------------------------------------------------------------
makeid: Make an AHA-format ID block for a database record
Copyright (C) 2003 George B. Moody

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

void p16(x, fp)		/* write a 16-bit integer in PDP-11 format */
unsigned int x;
FILE *fp;
{
    (void)fputc((char)x, fp);
    (void)fputc((char)(x >> 8), fp);
}

void p32(x, fp)		/* write a 32-bit integer in PDP-11 format */
long x;
FILE *fp;
{
    p16((unsigned int)(x >> 16), fp);
    p16((unsigned int)x, fp);
}

main(argc, argv)
int argc;
char *argv[];
{
    long ftell(), nsamp, first, last;
    int blocks, len, nann;
    static char nulls[464];
    WFDB_Anninfo afarray[1];
    WFDB_Siginfo dfarray[2];
    WFDB_Annotation annot;

    if (argc < 3) {
	(void)fprintf(stderr, "usage: %s annotator record\n", argv[0]);
	exit(1);
    }
    if ((len = strlen(argv[2])) > 8) {
	(void)fprintf(stderr, "record name must be 8 characters or less\n");
	exit(1);
    }
    afarray[0].name = argv[1]; afarray[0].stat = WFDB_READ;
    if (wfdbinit(argv[2], afarray, 1, dfarray, 2) < 1)
	exit(2);
    nsamp = dfarray[0].nsamp;
    (void)getann(0, &annot);
    first = annot.time;
    nann = 1;
    while (getann(0, &annot) >= 0) {
	last = annot.time;
	nann++;
    }
    wfdbquit();

    /* Bytes 1-8: record ID (null-padded) */
    (void)printf("%s", argv[2]);
    if (len < 8)
	(void)fwrite(nulls, 1, 8-len, stdout);
    (void)fprintf(stderr, "%s\n", argv[2]);

    /* Bytes 9-10: number of annotations */
    p16((unsigned)nann, stdout);
    (void)fprintf(stderr, "%d annotations\n", nann);

    /* Bytes 11-16: unused */
    (void)fwrite(nulls, 1, 6, stdout);

    /* Bytes 17-20: time of first sample in the annotated segment of the
       record, relative to the beginning of the record */
    p32(0L, stdout);
    (void)fprintf(stderr, "Annotated segment begins at sample 0\n");

    /* Bytes 21-24: time of last sample in the annotated segment of the record,
       relative to the beginning of the record */
    p32(nsamp, stdout);
    (void)fprintf(stderr, "Annotated segment ends at sample %ld\n", nsamp);

    /* Bytes 25-26: number of 512-byte logical blocks of sample data
       (nsamp * 2 bytes per sample * 2 channels / 512, rounded upward) */
    blocks = (nsamp + 127) >> 7;
    p16((unsigned)blocks, stdout);
    (void)fprintf(stderr, "%d blocks of sample data\n", blocks);

    /* Bytes 27-32: unused */
    (void)fwrite(nulls, 1, 6, stdout);

    /* Bytes 33-36: TOC of first annotation, relative to the beginning of the
       annotated segment */
    p32(first, stdout);
    (void)fprintf(stderr, "First annotation is at sample %ld\n", first);

    /* Bytes 37-40: TOC of last annotation, relative to the beginning of the
       annotated segment */
    p32(last, stdout);
    (void)fprintf(stderr, "Last annotation is at sample %ld\n", last);

    /* Bytes 41-42: number of 512-byte logical blocks of annotations
       (nann * 16 bytes per annotation / 512, rounded upward) */
    blocks = (nann + 31) >> 5;
    p16((unsigned)blocks, stdout);
    (void)fprintf(stderr, "%d blocks of annotations\n", blocks);

    /* Bytes 43-48: unused */
    (void)fwrite(nulls, 1, 6, stdout);

    /* Bytes 49-512: may contain information such as a copyright notice */
    (void)fwrite(nulls, 1, 464, stdout);
    exit(0);	/*NOTREACHED*/
}
