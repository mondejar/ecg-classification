/* file: pscgen.c	G. Moody	26 November 1990
			Last revised:	  20 May 2002

-------------------------------------------------------------------------------
pscgen: Generate a script for pschart
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

This program reads an annotation file and generates a `pschart' script for
printing strips which (by default) are centered on each annotation.  The
argument of the -l option specifies the length of each strip, and that of
the -d option specifies the delay from the beginning of each strip to the
annotation (this delay may be negative if desired;  in this case, the
strips begin at the specified interval AFTER each annotation).  Other options
are interpreted as for `rdann'.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

main(argc, argv)	
int argc;
char *argv[];
{
    static char *record, *ts, save_aux[255];
    int delay_ann, i, j;
    long from = 0L, to = 0L, delay = 0L, length = 0L;
    static char flag[ACMAX+1];
    WFDB_Anninfo ai;
    WFDB_Annotation annot;

    if (argc < 3) {
	fprintf(stderr,	"usage: %s annotator record [options]\n", argv[0]);
	exit(1);
    }
    ai.name = argv[1]; ai.stat = WFDB_READ; record = argv[2];
    if (annopen(record, &ai, 1) < 0) exit(2);
    if ((sampfreq(record)) <= 0.) setsampfreq(WFDB_DEFFREQ);
    flag[0] = 1;
    delay = strtim("0.5");
    for (i = 3; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i] + 1)) {
	  case 'd':	/* specify delay time */
	    if (++i >= argc) {
		fprintf(stderr, "%s: delay time must follow -d\n", argv[0]);
		exit(1);
	    }
	    if (*argv[i] == '-') delay = -strtim(argv[i]+1);
	    else delay = strtim(argv[i]);
	    break;
	  case 'D':	/* specify delay in annotations */
	    if (++i >= argc) {
		fprintf(stderr, "%s: delay in annotations must follow -D\n",
			argv[0]);
		exit(1);
	    }
	    delay_ann = atoi(argv[i]);
	    break;
	  case 'f':	/* specify starting time */
	    if (++i >= argc) {
		fprintf(stderr, "%s: starting time must follow -f\n", argv[0]);
		exit(1);
	    }
	    from = strtim(argv[i]);
	    if (from < (WFDB_Time)0) from = -from;
	    break;
	  case 'l':	/* specify strip length */
	    if (++i >= argc) {
		fprintf(stderr, "%s: strip length must follow -l\n", argv[0]);
		exit(1);
	    }
	    length = strtim(argv[i]);
	    break;
	  case 't':	/* specify ending time */
	    if (++i >= argc) {
		fprintf(stderr, "%s: ending time must follow -t\n", argv[0]);
		exit(1);
	    }
	    to = strtim(argv[i]);
	    if (to < (WFDB_Time)0) to = -to;
	    break;
	}
	else {
	    flag[0] = 0;
	    if (isann(j = strann(argv[i]))) flag[j] = 1;
	}
    }
    if (from && iannsettime(from) < 0) exit(2);

    if (length <= 0L) length = strtim("1");
    if (to < 0L) to = 0L;
    while (getann(0, &annot) == 0 && (to == 0L || annot.time <= to))
	if (flag[0] || (isann(annot.anntyp) && flag[annot.anntyp])) {
	    long t0, t1;

	    if (annot.aux) strcpy(save_aux, annot.aux+1);
	    else save_aux[0] = '\0';
	    if (delay_ann) {
		for (j = 0; j < delay_ann; j++)
		    if (getann(0, &annot)) exit(0);
	    }			
	    t0 = annot.time - delay;
	    if (t0 < 0L) t0 = 0L;
	    t1 = t0 + length;
	    ts = mstimstr(t0);
	    while (*ts == ' ') ts++;
	    printf("%s %s-", record, ts);
	    ts = mstimstr(t1);
	    while (*ts == ' ') ts++;
	    printf("%s", ts);
	    if (save_aux[0]) printf(" %s", save_aux);
	    printf("\n");
	}
    exit(0);
}

