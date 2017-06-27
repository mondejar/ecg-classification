/* file: stdev.c        G. Moody        19 August 1996
			Last revised:	17 November 2002
-------------------------------------------------------------------------------
stdev: sample application for use with WAVE
Copyright (C) 2002 George B. Moody

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

This program measures ST deviations given QRS annotations and a pair of
`(' and `)' annotations around the first QRS annotation.  Its output
contains two columns of numbers:  the elapsed time in minutes, and the
measured ST deviation in microvolts.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

main(int argc, char **argv)
{
    char *from_string = "0", *record = NULL, *to_string = "e";
    WFDB_Siginfo *si;
    WFDB_Anninfo ai;
    WFDB_Annotation annot;
    int i, s = 0;        /* analyze signal 0 unless otherwise specified */
    double spm;
    WFDB_Sample dv, *v0, *v1;
    WFDB_Time dt = 0L, dt0 = 0L, dt1 = 0L, epoch, ms80, tfrom = 0L, tpq = 0L,
        tqrs = 0L, tto = 0L;

    ai.name = NULL; ai.stat = WFDB_READ;
    /* Read all of the command-line arguments. */
    for (i = 1; i < argc-1; i++) {
        if (argv[i][0] == '-') switch (argv[i][1]) {
        case 'a':        /* annotator name follows */
            ai.name = argv[++i];
            break;
        case 'f':        /* start time follows */
            from_string = argv[++i];
            break;
        case 'r':        /* record name follows */
            record = argv[++i];
            break;
        case 's':        /* signal number follows */
            s = atoi(argv[++i]);
            break;
        case 't':        /* end time follows */
            to_string = argv[++i];
            break;
        default:
            fprintf(stderr, "%s: unrecognized option %s\n",
                    argv[0], argv[i]);
        }
        else {
            fprintf(stderr, "%s: unrecognized argument %s\n",
                    argv[0], argv[i]);
        }
    }
    if (record == NULL || ai.name == NULL) {
        /* complain unless both RECORD and ANNOTATOR were specified */
        fprintf(stderr, "usage: %s -r RECORD -a ANNOTATOR [ OPTIONS ]\n",
                argv[0]);
        fprintf(stderr, "OPTIONS may include:\n");
        fprintf(stderr, "-f START\n");
        fprintf(stderr, "-s SIGNAL\n");
        fprintf(stderr, "-t END\n");
        exit(1);
    }
    if (s < 0 || (i = isigopen(record, NULL, 0)) <= s) {
        fprintf(stderr, "%s: invalid signal number %d\n",
                argv[0], s);
        exit(2);
    }
    if ((si = (WFDB_Siginfo *)malloc(i * sizeof(WFDB_Siginfo))) == NULL ||
	(v0 = (WFDB_Sample *)malloc(i * sizeof(WFDB_Sample))) == NULL ||
	(v1 = (WFDB_Sample *)malloc(i * sizeof(WFDB_Sample))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", argv[0]);
	exit(3);
    }
    if (isigopen(record, si, i) <= s)
	exit(2);

    if (annopen(record, &ai, 1) < 0) {
        fprintf(stderr, "%s: can't read annotations\n", argv[0]);
        exit(3);
    }

    ms80 = strtim("0.08");        /* 80 milliseconds, in sample intervals */
    spm  = strtim("1:0");        /* 1 minute, in sample intervals */
    if ((tfrom = strtim(from_string)) < 0L)        /* start time */
        tfrom = -tfrom;                /* convert absolute to elapsed time */
    epoch = tfrom + spm;
    if ((tto = strtim(to_string)) < 0L)                /* end time */
        tto = -tto;
    while (getann(0, &annot) == 0) {        /* read an annotation */
        if (annot.time > epoch) {
            fprintf(stderr, ".");        /* show progress by minutes read */
            epoch += spm;
        }
        if (annot.anntyp == WFON)        /* found a `(', save its TOC */
            tpq = annot.time;
        else if (annot.anntyp == WFOFF && tqrs > 0L) {
                                    /* found a valid `)', update dt1 and dt */
            dt1 = ms80 + annot.time - tqrs;     /* samples from QRS to J+80 */
            if (dt0 > 0L) dt = dt0 + dt1;        /* samples from PQ to J+80 */
        }
        else if (isqrs(annot.anntyp)) {        /* found a beat label */
            tqrs = annot.time;
            if (tpq > 0L) { /* previous `(' went with this beat, update dt0 */
                dt0 = tqrs - tpq;                /* samples from PQ to QRS */
                tpq = 0L;
                if (dt1 > 0L) dt = dt0 + dt1;
            }
            if (tqrs >= tfrom && dt0 > 0L && dt1 > 0L) {
                if (tto > 0L && /* if tto is zero, continue to end of record */
                    tqrs > tto)
                    break;        /* nothing further to be done */
                /* tqrs, dt0, and dt1 are all valid: make a measurement */
                if (isigsettime(tqrs - dt0) < 0 ||
                    getvec(v0) <= s) {
                    fprintf(stderr,
                            "%s: can't read signal %d at time %s\n",
                            argv[0], s,
                            mstimstr(tqrs - dt0));
                    exit(4);
                }
                for (i = 1; i <= dt; i++)
                    if (getvec(v1) <= s) {
                        fprintf(stderr,
                                "%s: can't read signal %d at time %s\n",
                                argv[0], s,
                                mstimstr(tqrs - dt0 + i));
                    exit(4);
                    }
                dv = v1[s] - v0[s];        /* ST deviation, in ADC units */
                printf("%g\t%d\n",        /* print a line of output */
                       tqrs/spm,        /* elapsed time in minutes */
                       adumuv(s, dv));  /* ST deviation in microvolts */
            }
        }
        else        /* found some other annotation: ignore it */
            continue;
    }
    if (dt == 0) {        /* we didn't make any measurements -- complain */
        fprintf(stderr, "\n%s: no measurements made\n", argv[0]);
        exit(5);
    }
    exit(0);
}
