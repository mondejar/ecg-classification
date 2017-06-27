/* file: xform.c	G. Moody        8 December 1983
			Last revised:   3 November 2010
-------------------------------------------------------------------------------
xform: Sampling frequency, amplitude, and format conversion for WFDB records
Copyright (C) 1983-2010 George B. Moody

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
#include <stdlib.h>
#include <wfdb/wfdb.h>

/* The following definition yields dither with a triangular PDF in (-1,1). */
#define DITHER	        (((double)rand() + (double)rand())/RAND_MAX - 1.0)

char *pname, *prog_name();
double gcd();
void help();

main(argc, argv)
int argc;
char *argv[];
{
    char btstring[30], **description, **filename, *irec = NULL, *orec = NULL,
	*nrec = NULL, *script = NULL, *startp = "0:0", **units;
    double *gain, ifreq, ofreq = 0.0;
    int clip = 0, *deltav, dflag = 0, fflag = 0, gflag = 0, Hflag = 0, i,
	iframelen, j, m, Mflag = 0, mn, *msiglist, n, nann = 0, nisig,
	nminutes = 0, nosig = 0, oframelen, reopen = 0, sflag = 0,
	*siglist = NULL, spf, uflag = 0, use_irec_desc = 1, *v, *vin, *vmax,
	*vmin, *vout, vt, *vv;
    long from = 0L, it = 0L, nsamp = -1L, nsm = 0L, ot = 0L, spm, to = 0L;
    WFDB_Anninfo *ai = NULL;
    WFDB_Annotation annot;
    WFDB_Siginfo *dfin, *dfout;

    pname = prog_name(argv[0]);

    /* Interpret the command-line arguments. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator(s) */
	    /* Count the number of annotators.  Accept the next argument
	     unconditionally as an annotator name; accept additional arguments
	     until we find one beginning with `-', or the end of the argument
	     list. */
	    for (j = 0; ++i < argc && (j == 0 || *argv[i] != '-'); j++)
		;
	    if (j == 0) {
		(void)fprintf(stderr, "%s: annotator(s) must follow -a\n",
			      pname);
		exit(1);
	    }
	    /* allocate storage */
	    if ((ai = realloc(ai, (nann+j) * sizeof(WFDB_Anninfo))) == NULL) {
		(void)fprintf(stderr, "%s: insufficient memory\n", pname);
		exit(2);
	    }
	    /* fill the annotator information structures */
	    for (i -= j, j=0; i<argc && (j == 0 || *argv[i] != '-'); j++) {
		ai[nann].name = argv[i++];
		ai[nann++].stat = WFDB_READ;
	    }
	    i--;
	    break;
	  case 'c':	/* clip (limit) output (default: discard high bits) */
	    clip = 1;
	    break;
	  case 'd':
	    dflag = 1;
	    srand((long)0);	/* seed for dither RNG -- replace 0 with a
				   different value to get different dither */
	    break;
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		exit(1);
	    }
	    from = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* open the input record in `high resolution' mode */
	    Hflag = 1;
	    break;
	  case 'i':	/* input record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input record name must follow -i\n",
			pname);
		exit(1);
	    }
	    irec = argv[i];
	    break;
	  case 'M':	/* multifrequency mode (no frequency changes) */
	    Mflag = 1;
	    break;
	  case 'n':	/* new record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: new record name must follow -n\n",
			pname);
		exit(1);
	    }
	    nrec = argv[i];
	    break;
	  case 'N':	/* new record name follows;  use signal descriptions
			   from output record header file in new header file */
	    use_irec_desc = 0;
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: new record name must follow -N\n",
			pname);
		exit(1);
	    }
	    nrec = argv[i];
	    break;
	  case 'o':	/* output record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: output record name must follow -o\n",
			pname);
		exit(1);
	    }
	    orec = argv[i];
	    break;
	  case 's':	/* signal list follows */
	    sflag = 1;
	    /* count the number of output signals */
	    for (j = 0; ++i < argc && *argv[i] != '-'; j++)
		;
	    if (j == 0) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
	    /* allocate storage for the signal list */
	    if ((siglist=realloc(siglist, (nosig+j) * sizeof(int))) == NULL) {
		(void)fprintf(stderr, "%s: insufficient memory\n", pname);
		exit(2);
	    }
	    /* fill the signal list */
	    for (i -= j; i < argc && *argv[i] != '-'; )
		siglist[nosig++] = i++;
	    i--;
	    break;
	  case 'S':	/* script name follows */
	    if (++i >= argc) {
	        (void)fprintf(stderr, "%s: script name must follow -S\n",
			      pname);
		exit(1);
	    }
	    script = argv[i];
	    break;
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n", pname);
		exit(1);
	    }
	    to = i;
	    break;
	  case 'u':	/* make annotation times unique */
	    uflag = 1;
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

    /* Check that an input record was specified. */
    if (irec == NULL) {
	help();
	exit(1);
    }

    /* Check that nrec and orec differ if both were specified. */
    if (nrec && orec && (strcmp(nrec, orec) == 0)) {
	(void)fprintf(stderr,
	      "%s: arguments of -n (or -N) and -o cannot be identical\n",
		      pname);
	exit(1);
    }

    /* Determine the number of input signals.  If isigopen returns a negative
       value, quit (isigopen will have emitted an error message). */
    if ((nisig = isigopen(irec, NULL, 0)) < 0) exit(2);

    /* Determine the input sampling frequency. */
    ifreq = sampfreq(NULL);
    (void)setsampfreq(0.);

    /* If the input record contains no signals, we won't write any -- but
       we might still read and write annotations. */
    if (nisig == 0) nosig = 0;

    /* Otherwise, prepare to read and write signals. */
    else {
	/* Allocate storage for variables related to the input signals. */
	if ((dfin = malloc(nisig * sizeof(WFDB_Siginfo))) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}

	/* Open the input signals. */
	if (isigopen(irec, dfin, nisig) != nisig) exit(2);

	/* If a signal list was specified using -s, check that the specified
	   signal numbers or names are legal. */
	if (sflag) {
	    for (i = 0; i < nosig && nosig > 0; i++) {
	        char *s;

		s = argv[siglist[i]];
		if ((siglist[i] = findsig(s)) < 0) {
		    (void)fprintf(stderr,
		       "%s: warning: record %s doesn't have a signal '%s'\n",
				  pname, irec, s);
		    /* Delete illegal signal numbers from the list. */
		    for (j = i; j+1 < nosig; j++)
			siglist[j] = siglist[j+1];
		    nosig--;
		    i--;
		}
	    }
	    /* If the signal list contained no valid signal numbers, treat
	       this situation as if no signal list was specified. */
	    if (nosig == 0)
		sflag = 0;
	}

	/* If the signal list was not specified using -s, initialize it now. */
	if (sflag == 0) {
	    nosig = nisig;
	    if ((siglist = malloc(nosig * sizeof(int))) != NULL)
		for (i = 0; i < nosig; i++)
		    siglist[i] = i;
	}

	/* Allocate storage for variables related to the output signals. */
	if ((siglist == NULL) ||
	    (dfout = malloc(nosig * sizeof(WFDB_Siginfo))) == NULL ||
	    (gain = malloc(nosig * sizeof(double))) == NULL ||
	    (deltav = malloc(nosig * sizeof(int))) == NULL ||
	    (msiglist = malloc(nosig * sizeof(int))) == NULL ||
	    (v = malloc(nosig * sizeof(int))) == NULL ||
	    (vmax = malloc(nosig * sizeof(int))) == NULL ||
	    (vmin = malloc(nosig * sizeof(int))) == NULL ||
	    (vv = malloc(nosig * sizeof(int))) == NULL ||
	    (filename = malloc(nosig * sizeof(char *))) == NULL ||
	    (description = malloc(nosig * sizeof(char *))) == NULL ||
	    (units = malloc(nosig * sizeof(char *))) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
    }

    if (Hflag)
	setgvmode(WFDB_HIGHRES);
    ifreq *= spf = getspf();

    if (Mflag) {
	for (i = iframelen = 0; i < nisig; i++)
	    iframelen += dfin[i].spf;
	for (i = oframelen = 0; i < nosig; i++)
	    oframelen += dfin[siglist[i]].spf;
    }
    else {
	iframelen = nisig;
	oframelen = nosig;
    }
    if ((vin = malloc(iframelen * sizeof(int))) == NULL ||
	(vout = malloc(oframelen * sizeof(int))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }

    if (orec != NULL) {	/* an output record was specified */
	/* If a new record name was specified, check that it can be created. */
	if (nrec && newheader(nrec) < 0) exit(2);

	/* Determine the output sampling frequency. */
	if (orec) ofreq = sampfreq(orec);
	(void)setsampfreq(ifreq/spf);

	if (ifreq != ofreq) {
	    if (Mflag != 0) {
		(void)fprintf(stderr,
      "%s: -M option may not be used if sampling frequency is to be changed\n",
			      pname);
		wfdbquit();
		exit(1);
	    }
	}
    }

    /* If no output record was specified, get signal specs interactively. */
    else {
	static char answer[32], directory[32], record[WFDB_MAXRNL+2];
	static int formats[WFDB_NFMTS] = WFDB_FMT_LIST;	/* see <wfdb/wfdb.h> */
	int format;
	FILE *ttyin;
#ifndef MSDOS
	ttyin = fopen(script ? script : "/dev/tty", "r");
#else
	ttyin = fopen(script ? script : "CON", "rt");
#endif
	if (ttyin == NULL) ttyin = stdin;
	if (nrec == NULL) {
	    do {
		(void)fprintf(stderr,
		 "Choose a name for the output record (up to %d characters): ",
			      WFDB_MAXRNL);
		(void)fgets(record, WFDB_MAXRNL+2, ttyin);
		record[strlen(record)-1] = '\0';
	    } while (record[0] == '\0' || newheader(record) < 0);
	    nrec = record;
	}
	if (sflag == 0) {
	    do {
		nosig = nisig;
		(void)fprintf(stderr,
			      "Number of signals to be written (1-%d) [%d]: ",
		       nisig, nosig);
		(void)fgets(answer, sizeof(answer), ttyin);
		(void)sscanf(answer, "%d", &nosig);
		if (nosig == 0) {
		    (void)fprintf(stderr,
			"No signals will be written.  Are you sure? [n]: ");
		    (void)fgets(answer, sizeof(answer), ttyin);
		    if (*answer != 'y' && *answer != 'Y')
			nosig = -1;
		}
	    } while (nosig < 0 || nosig > nisig);
	}
	if (Mflag) {
	    ofreq = ifreq;
	    (void)fprintf(stderr,
		  "Sampling frequency (%g frames/sec) will be unchanged\n",
			  ofreq);
	}
	else do {
	    ofreq = ifreq;
	    (void)fprintf(stderr,
		  "Output sampling frequency (Hz per signal, > 0) [%g]: ",
			  ofreq);
	    (void)fgets(answer, sizeof(answer), ttyin);
	    (void)sscanf(answer, "%lf", &ofreq);
	} while (ofreq < 0);
	if (ofreq == 0) ofreq = WFDB_DEFFREQ;
	if (nosig > 0) {
	    (void)fprintf(stderr,
		"Specify the name of the directory in which the output\n");
	    (void)fprintf(stderr,
		" signals should be written, or press <return> to write\n");
	    (void)fprintf(stderr, " them in the current directory: ");
	    (void)fgets(directory, sizeof(directory)-1, ttyin);
	    if (*directory == '\n') *directory = '\0';
	    else directory[strlen(directory)-1] = '/';
	    (void)fprintf(stderr,"Any of these output formats may be used:\n");
	    (void)fprintf(stderr, "    8   8-bit first differences\n");
	    (void)fprintf(stderr,
		"   16   16-bit two's complement amplitudes (LSB first)\n");
	    (void)fprintf(stderr,
		"   61   16-bit two's complement amplitudes (MSB first)\n");
	    (void)fprintf(stderr, "   80   8-bit offset binary amplitudes\n");
	    (void)fprintf(stderr, "  160   16-bit offset binary amplitudes\n");
	    (void)fprintf(stderr,
		        "  212   2 12-bit amplitudes bit-packed in 3 bytes\n");
	    (void)fprintf(stderr,
  "  310   3 10-bit amplitudes bit-packed in 15 LS bits of each of 2 words\n");
	    (void)fprintf(stderr,
	  "  311   3 10-bit amplitudes bit-packed in 30 LS bits of 4 bytes\n");
	    (void)fprintf(stderr,
	  "   24   24-bit two's complement amplitudes (LSB first)\n");
	    (void)fprintf(stderr,
	  "   32   32-bit two's complement amplitudes (LSB first)\n");
	    do {
		format = dfin[0].fmt;
		(void)fprintf(stderr,
		 "Choose an output format (8/16/61/80/160/212/310/311/24/32) [%d]: ",
			      format);
		(void)fgets(answer, sizeof(answer), ttyin);
		(void)sscanf(answer, "%d", &format);
		for (i = 1; i < WFDB_NFMTS; i++) /* skip format[0] (= 0) */
		    if (format == formats[i]) break;
	    } while (i >= WFDB_NFMTS);
	    if (nosig > 1) {
		(void)fprintf(stderr, "Save all signals in one file? [y]: ");
		(void)fgets(answer, sizeof(answer), ttyin);
	    }
	    /* Use the input signal specifications as defaults for output. */
	    for (i = 0; i < nosig; i++)
		dfout[i] = dfin[siglist[i]];
	    if (nosig <= 1 || (answer[0] != 'n' && answer[0] != 'N')) {
		filename[0] = malloc(strlen(directory)+strlen(nrec)+10);
		if (filename[0] == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		(void)sprintf(filename[0], "%s%s.dat", directory, nrec);
		for (i = 0; i < nosig; i++) {
		    dfout[i].fname = filename[0];
		    dfout[i].group = 0;
		}
	    }
	    else {
		for (i = 0; i < nosig; i++) {
		    filename[i] = malloc(strlen(directory)+strlen(nrec)+10);
		    if (filename[i] == NULL) {
			(void)fprintf(stderr, "%s: insufficient memory\n",
				      pname);
			exit(2);
		    }
		    (void)sprintf(filename[i], "%s%s.d%d", directory, nrec, i);
		    dfout[i].fname = filename[i];
		    dfout[i].group = i;
		}
	    }
	}
	for (i = 0; i < nosig; i++) {
	    /* Make sure output signals are written in the chosen format. */
	    dfout[i].fmt = format;
	    if (Mflag == 0) dfout[i].spf = 1;
	    dfout[i].bsize = 0;	    /* no block size is defined */
	    if (!use_irec_desc) {
		char *p;

		if ((description[i] = malloc(WFDB_MAXDSL+2)) == NULL ||
		    (units[i] = malloc(WFDB_MAXUSL+2)) == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		(void)fprintf(stderr,
			      "Signal %d description (up to %d characters): ",
			      i, WFDB_MAXDSL);
		(void)fgets(description[i], WFDB_MAXDSL+2, ttyin);
		description[i][strlen(description[i])-1] = '\0';
		dfout[i].desc = description[i];
		(void)fprintf(stderr,
			      "Signal %d units (up to %d characters): ",
			      i, WFDB_MAXUSL);
		(void)fgets(units[i], WFDB_MAXUSL+2, ttyin);
		for (p = units[i]; *p; p++) {
		    if (*p == ' ' || *p == '\t') *p = '_';
		    else if (*p == '\n') { *p = '\0'; break; }
		}
		dfout[i].units = *units[i] ? units[i] : NULL;
	    }
	    (void)fprintf(stderr, " Signal %d gain (adu/%s) [%g]: ",
			  i, dfout[i].units ? dfout[i].units : "mV",
			  dfout[i].gain);
	    (void)fgets(answer, sizeof(answer), ttyin);
	    sscanf(answer, "%lf", &dfout[i].gain);
	    do {
		if (i > 0) dfout[i].adcres = dfout[i-1].adcres;
		switch (dfout[i].fmt) {
		  case 80:
		    dfout[i].adcres = 8;
		    break;
		  case 212:
		    if (dfout[i].adcres < 8 || dfout[i].adcres > 12)
			dfout[i].adcres = 12;
		    break;
		  case 310:
		    if (dfout[i].adcres < 8 || dfout[i].adcres > 10)
			dfout[i].adcres = 10;
		    break;
		  case 24:
		    if (dfout[i].adcres < 8 || dfout[i].adcres > 24)
			dfout[i].adcres = 24;
		    break;
		  case 32:
		    if (dfout[i].adcres < 8 || dfout[i].adcres > 32)
			dfout[i].adcres = 32;
		    break;
		  default:
		    if (dfout[i].adcres < 8 || dfout[i].adcres > 16)
			dfout[i].adcres = WFDB_DEFRES;
		    break;
		}
		(void)fprintf(stderr,
			" Signal %d ADC resolution in bits (8-32) [%d]: ", i,
			      dfout[i].adcres);
		(void)fgets(answer, sizeof(answer), ttyin);
		(void)sscanf(answer, "%d", &dfout[i].adcres);
	    } while (dfout[i].adcres < 8 || dfout[i].adcres > 32);
	    (void)fprintf(stderr, " Signal %d ADC zero level (adu) [%d]: ",
			  i, dfout[i].adczero);
	    (void)fgets(answer, sizeof(answer), ttyin);
	    (void)sscanf(answer, "%d", &dfout[i].adczero);
	}
    }

    /* Check the starting and ending times. */
    if (from) startp = argv[(int)from];
    from = strtim(startp);
    if (from < 0L) from = -from;
    strcpy(btstring, mstimstr(-from));
    if (to) {
	to = strtim(argv[(int)to]);
	if (to < 0L) to = -to;
	if (to > 0L && (nsamp = to - from) <= 0L) {
	    (void)fprintf(stderr,
			  "%s: 'to' time must be later than 'from' time\n",
			  pname);
	    exit(1);
	}
    }
    spm = strtim("1:0");

    /* Process the annotation file(s), if any. */
    if (nann > 0) {
	char *p0, *p1;
	int cc;
	long tt;

	/* Check that the input annotation files are readable. */
	if (annopen(irec, ai, (unsigned)nann) < 0 || iannsettime(from) < 0)
	    exit(2);

	/* Associate the output annotation files with the new record, if a
	   new record name was specified. */
	if (nrec) p0 = nrec;
	/* Otherwise, associate them with the existing output record. */
	else p0 = orec;

	/* Check that the output annotation files are writeable, without
	   closing the input annotation files. */
	if ((p1 = (char *)malloc((unsigned)strlen(p0)+2)) == NULL) {
	    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
	(void)sprintf(p1, "+%s", p0);
	for (i = 0; i < nann; i++)
	    ai[i].stat = WFDB_WRITE;
	if (annopen(p1, ai, (unsigned)nann) < 0) exit(2);

	/* Write the annotation files. */
	for (i = 0; i < nann; i++) {
	    tt = 0L;
	    cc = -129;
	    while (getann((unsigned)i, &annot) == 0 &&
		   (to == 0L || annot.time <= to)) {
		annot.time = (annot.time - from) * (ofreq*spf) / ifreq;
		/* If the -u option was specified, make sure that the corrected
		   annotation time is positive and that it is later than the
		   previous annotation time (exception: if it matches the
		   previous annotation time, but the chan field is greater than
		   that of the previous annotation, no time adjustment is
		   performed). */
		if (uflag && (annot.time < tt ||
			      (annot.time == tt && annot.chan <= cc)))
		    annot.time = ++tt;
		else tt = annot.time;
		cc = annot.chan;
		(void)putann((unsigned)i, &annot);
	    }
	}

	/* Close the annotation files. */
	for (i = nann-1; i >= 0; i--) {
	    iannclose(i);
	    oannclose(i);
	}
    }

    /* Quit if no signals are to be written. */
    if (nosig == 0) exit(0);

    /* Check that the input signals are readable. */
    (void)fprintf(stderr, "Checking input signals ...");
    if (isigsettime(from) < 0)
	exit(2);
    (void)fprintf(stderr, " done\n");

     /* If an output record was specified using `-o', check that the output
       signals are writable.  Suppress warning messages about possible
       sampling frequency differences. */
    wfdbquiet();
    if (orec && (osigopen(orec, dfout, (unsigned)nosig) != nosig)) {
	(void)fprintf(stderr, "%s: can't write output signals\n", pname);
	exit(2);
    }
    wfdbverbose();

    /* Make sure that signal file format is not 0 (putvec cannot write to
       a null record). */
    for (i = 0; i < nosig; i++) {
	if (dfout[i].fmt == 0) {
	    (void)fprintf(stderr, "%s: signal %d format cannot be 0"
			  " (changing to format 16)\n", pname, i);
	    dfout[i].fmt = 16;
	    reopen = 1;	/* record must be (re)opened using osigfopen to make
			   the change effective */
	}
    }

   /* If the `-n' option was specified, copy the signal descriptions from the
       input record header file into the WFDB_Siginfo structures for the output
       record (for eventual storage in the new output header file).  To make
       the changes effective, the output signals must be (re)opened using
       osigfopen. */
    if (nrec && use_irec_desc) {
	for (i = 0; i < nosig; i++)
	    dfout[i].desc = dfin[siglist[i]].desc;
	reopen = 1;
    }

    /* Determine relative gains and offsets of the signals.  If any output
       gain must be adjusted, the output signals must be reopened (to ensure
       that the correct gains are written to the output header). */
    for (i = 0; i < nosig; i++) {
	j = siglist[i];
	/* A signal may be rescaled by xform depending on the gains and ADC
	   resolutions specified.  The rules for doing so are as follows:
	   1.  If the output gain has not been specified, the signal is scaled
	       only if the input and output ADC resolutions differ.  In this
	       case, the scale factor (gain[]) is determined so that the
	       appropriate number of bits will be dropped from or added to each
	       signal.
	   2.  If the output gain has been specified, the signal is scaled by
	       the ratio of the output and input gains.  If the input gain is
	       not specified explicitly, the value WFDB_DEFGAIN is assumed.
	   These rules differ from those used by earlier versions of xform,
	   although the default behavior remains the same in most cases. */

	if (dfout[i].gain == 0.) {	/* output gain not specified */
	    if (dfin[j].adcres == 0) {	/* input ADC resolution not specified;
					   assume a suitable default */
		switch (dfin[j].fmt) {
		  case 80:
		    dfin[j].adcres = 8;
		    break;
		  case 212:
		    dfin[j].adcres = 12;
		    break;
		  case 310:
		    dfin[j].adcres = 10;
		    break;
		  case 24:
		    dfin[j].adcres = 24;
		    break;
		  case 32:
		    dfin[j].adcres = 32;
		    break;
		  default:
		    dfin[j].adcres = WFDB_DEFRES;
		    break;
		}
	    }
	    gain[i] = (double)(1L << dfout[i].adcres) /
		      (double)(1L << dfin[j].adcres);
	}
	else if (dfin[j].gain == 0.)	/* output gain specified, but input
					   gain not specified */
	    gain[i] = dfout[i].gain/WFDB_DEFGAIN;
	else				/* input and output gains specified */
	    gain[i] = dfout[i].gain/dfin[j].gain;

	if (gain[i] != 1.) gflag = 1;
        deltav[i] = dfout[i].adczero - gain[i] * dfin[j].adczero;
	if (dfout[i].gain != dfin[j].gain * gain[i] ||
	    dfout[i].baseline != dfin[j].baseline * gain[i]) {
	    dfout[i].gain = dfin[j].gain * gain[i];
	    dfout[i].baseline = dfin[j].baseline * gain[i];
	    reopen = 1;
	}
	/* Note that dfout[i].gain is 0 (indicating that the signal is
	   uncalibrated) if dfin[j].gain is 0.  This is a *feature*, not
	   a bug:  we don't want to create output signals that appear to
	   have been calibrated if the inputs weren't calibrated. */
    }

    /* Reopen output signals if necessary. */
    if (reopen) {
      /* Copy all strings that may be deallocated by osigfopen. */
      for (i = 0; i < nosig; i++) {
	char *p;

	if ((p = (char *)malloc((unsigned)strlen(dfout[i].fname)+1)) ==
	    (char *)NULL) {
	  (void)fprintf(stderr, "%s: out of memory\n", pname);
	  exit(2);
	}
	(void)strcpy(p, dfout[i].fname);
	dfout[i].fname = p;
	if (dfout[i].units) {
	  if ((p = (char *)malloc((unsigned)strlen(dfout[i].units)+1)) ==
	      (char *)NULL) {
	    (void)fprintf(stderr, "%s: out of memory\n", pname);
	    exit(2);
	  }
	  (void)strcpy(p, dfout[i].units);
	  dfout[i].units = p;
	}
	if (dfout[i].desc) {
	  if ((p = (char *)malloc((unsigned)strlen(dfout[i].desc)+1)) ==
	      (char *)NULL) {
	    (void)fprintf(stderr, "%s: out of memory\n", pname);
	    exit(2);
	  }
	  (void)strcpy(p, dfout[i].desc);
	  dfout[i].desc = p;
	}
      }
      if (osigfopen(dfout, (unsigned)nosig) != nosig)
	exit(2);
    }

    /* Determine the legal range of sample values for each output signal. */
    for (i = 0; i < nosig; i++) {
	vmax[i] = dfout[i].adczero + (1 << (dfout[i].adcres-1)) - 1;
	vmin[i] = dfout[i].adczero + (-1 << (dfout[i].adcres-1)) + 1;
    }

    /* If resampling is required, initialize the interpolation/decimation
       parameters. */
    if (ifreq != ofreq) {
	double f = gcd(ifreq, ofreq);

	fflag = 1;
	m = ifreq/f;
	n = ofreq/f;
	mn = m*n;
	(void)getvec(vin);
	for (i = 0; i < nosig; i++) {
	    WFDB_Sample v = vin[siglist[i]];

	    if (v != WFDB_INVALID_SAMPLE)
		vv[i] = v*gain[i] + deltav[i];
	    else
		vv[i] = WFDB_INVALID_SAMPLE;
	}
    }

    /* If in multifrequency mode, set msiglist offsets. */
    if (Mflag) {
	int j, k;

	for (i = 0; i < nosig; i++) {
	    for (j = k = 0; j < siglist[i]; j++)
		k += dfin[j].spf;
	    msiglist[i] = k;
	}
    }

    /* Process the signals. */
    if (fflag == 0 && gflag == 0) {	/* no frequency or gain changes */
      if (Mflag == 0) {			/* standard mode */
	while (getvec(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = 0; i < nosig; i++) {
		if ((vt = vin[siglist[i]]) == WFDB_INVALID_SAMPLE)
		    vout[i] = vt;
		else {
		    vout[i] = vt + deltav[i];
		    if (vout[i] > vmax[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[i]);
			if (clip) vout[i] = vmax[i];
			else vmax[i] = vout[i];
		    }
		    else if (vout[i] < vmin[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[i]);
			if (clip) vout[i] = vmin[i];
			else vmin[i] = vout[i];
		    }
		}
	    }
	    if (putvec(vout) < 0) break;
	}
      }
      else {				/* multifrequency mode */
	while (getframe(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    int j, k;

	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = j = 0; i < nosig; i++) {
	      for (k = 0; k < dfout[i].spf; j++, k++) {
		if ((vt = vin[msiglist[i] + k]) == WFDB_INVALID_SAMPLE)
		    vout[j] = vt;
		else {
		    vout[j] = vt + deltav[i];
		    if (vout[j] > vmax[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[j]);
			if (clip) vout[j] = vmax[i];
			else vmax[i] = vout[j];
		    }
		    else if (vout[j] < vmin[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[j]);
			if (clip) vout[j] = vmin[i];
			else vmin[i] = vout[j];
		    }
		}
	      }
	    }
	    if (putvec(vout) < 0) break;
	}
      }
    }
    else if (fflag == 0 && gflag != 0) {	/* change gain only */
      if (Mflag == 0) {			/* standard mode */
	while (getvec(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = 0; i < nosig; i++) {
		if ((vt = vin[siglist[i]]) == WFDB_INVALID_SAMPLE)
		    vout[i] = vt;
		else {
		    double vd = vt*gain[i] + deltav[i];

		    if (dflag) vd += DITHER;
		    if (vd >= 0) vout[i] = (int)(vd + 0.5);
		    else vout[i] = (int)(vd - 0.5);
		    if (vout[i] > vmax[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[i]);
			if (clip) vout[i] = vmax[i];
			else vmax[i] = vout[i];
		    }
		    else if (vout[i] < vmin[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[i]);
			if (clip) vout[i] = vmin[i];
			else vmin[i] = vout[i];
		    }
		}
	    }
	    if (putvec(vout) < 0) break;
	}
      }
      else {				/* multifrequency mode */
	while (getframe(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    int j, k;

	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = j = 0; i < nosig; i++) {
	      for (k = 0; k < dfout[i].spf; j++, k++) {
		if ((vt = vin[msiglist[i] + k]) == WFDB_INVALID_SAMPLE)
		    vout[j] = vt;
		else {
		    double vd = vt*gain[i] + deltav[i];

		    if (dflag) vd += DITHER;
		    if (vd >= 0) vout[j] = (int)(vd + 0.5);
		    else vout[j] = (int)(vd - 0.5);
		    if (vout[j] > vmax[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[j]);
			if (clip) vout[j] = vmax[i];
			else vmax[i] = vout[j];
		    }
		    else if (vout[j] < vmin[i]) {
			(void)fprintf(stderr, "v[%d] = %d (out of range)\n",
				      i, vout[j]);
			if (clip) vout[j] = vmin[i];
			else vmin[i] = vout[j];
		    }
		}
	      }
	    }
	    if (putvec(vout) < 0) break;
	}
      }
    }
    else if (fflag != 0 && gflag == 0) {	/* change frequency only */
	while (getvec(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = 0; i < nosig; i++) {
		if ((vt = vin[siglist[i]]) == WFDB_INVALID_SAMPLE)
		    v[i] = vt;
		else
		    v[i] = vt + deltav[i];
	    }
	    while (ot <= it) {
		double x = (ot%n == 0) ? 1.0 : (double)(ot % n)/(double)n;
		for (i = 0; i < nosig; i++) {
		    if (v[i] == WFDB_INVALID_SAMPLE ||
			vv[i] == WFDB_INVALID_SAMPLE)
			vout[i] = WFDB_INVALID_SAMPLE;
		    else {
			double vd = vv[i] + x*(v[i]-vv[i]);

			if (dflag) vd += DITHER;
			if (vd >= 0) vout[i] = (int)(vd + 0.5);
			else vout[i] = (int)(vd - 0.5);
			if (vout[i] > vmax[i]) {
			    (void)fprintf(stderr,"v[%d] = %d (out of range)\n",
					  i, vout[i]);
			    if (clip) vout[i] = vmax[i];
			    else vmax[i] = vout[i];
			}
			else if (vout[i] < vmin[i]) {
			    (void)fprintf(stderr,"v[%d] = %d (out of range)\n",
					  i, vout[i]);
			    if (clip) vout[i] = vmin[i];
			    else vmin[i] = vout[i];
			}
		    }
		}
		if (putvec(vout) < 0) { nsamp = 0L; break; }
		ot += m;
	    }
	    for (i = 0; i < nosig; i++)
		vv[i] = v[i];
	    if (it > mn) { it -= mn; ot -= mn; }
	    it += n;
	}
    }
    else if (fflag != 0 && gflag != 0) {	/* change frequency and gain */
	while (getvec(vin) >= nisig && (nsamp == -1L || nsamp-- > 0L)) {
	    if (++nsm >= spm) {
		nsm = 0L;
		(void)fprintf(stderr, ".");
		(void)fflush(stderr);
		if (++nminutes >= 60) {
		    nminutes = 0;
		    (void)fprintf(stderr, "\n");
		}
	    }
	    for (i = 0; i < nosig; i++) {
		if ((vt = vin[siglist[i]]) == WFDB_INVALID_SAMPLE)
		    v[i] = vt;
		else {
		    double vd = vt*gain[i] + deltav[i];

		    if (dflag) vd += DITHER;
		    if (vd >= 0) v[i] = (int)(vd + 0.5);
		    else v[i] = (int)(vd - 0.5);
		}
	    }
	    while (ot <= it) {
		double x = (ot%n == 0) ? 1.0 : (double)(ot % n)/(double)n;
		for (i = 0; i < nosig; i++) {
		    if (v[i] == WFDB_INVALID_SAMPLE ||
			vv[i] == WFDB_INVALID_SAMPLE)
			vout[i] = WFDB_INVALID_SAMPLE;
		    else {
			double vd =  vv[i] + x*(v[i]-vv[i]);

			if (dflag) vd += DITHER;
			if (vd >= 0) vout[i] = (int)(vd + 0.5);
			else vout[i] = (int)(vd - 0.5);
			if (vout[i] > vmax[i]) {
			    (void)fprintf(stderr,"v[%d] = %d (out of range)\n",
					  i, vout[i]);
			    if (clip) vout[i] = vmax[i];
			    else vmax[i] = vout[i];
			}
			else if (vout[i] < vmin[i]) {
			    (void)fprintf(stderr,"v[%d] = %d (out of range)\n",
					  i, vout[i]);
			    if (clip) vout[i] = vmin[i];
			    else vmin[i] = vout[i];
			}
		    }
		}
		if (putvec(vout) < 0) { nsamp = 0L; break; }
		ot += m;
	    }
	    for (i = 0; i < nosig; i++)
		vv[i] = v[i];
	    if (it > mn) { it -= mn; ot -= mn; }
	    it += n;
	}
    }
    if (nminutes > 0) (void)fprintf(stderr, "\n");

    /* Generate a new header file, if so requested. */
    if (nrec) {
	char *info, *xinfo;

	(void)setsampfreq(ofreq);
	if (btstring[0] == '[') {
	    btstring[strlen(btstring)-1] = '\0';  /* discard final ']' */
	    (void)setbasetime(btstring+1);
	}
	(void)newheader(nrec);

	/* Copy info strings from the input header file into the new one.
	   Suppress error messages from the WFDB library. */
	wfdbquiet();
	if (info = getinfo(irec))
	    do {
		(void)putinfo(info);
	    } while (info = getinfo((char *)NULL));

	/* Append additional info summarizing what xform has done. */
	if (xinfo = malloc(strlen(pname)+strlen(irec)+strlen(startp)+80)) {
	    (void)sprintf(xinfo,
			  "Produced by %s from record %s, beginning at %s",
			  pname, irec, startp);
	    (void)putinfo(xinfo);
	    for (i = 0; i < nosig; i++)
		if (siglist[i] != i) {
		    (void)sprintf(xinfo,
			      "record %s, signal %d <- record %s, signal %d",
				  nrec, i, irec, siglist[i]);
		    if (gain[i] != 1.)
			(void)sprintf(xinfo+strlen(xinfo),
				      " * %g", gain[i]);
		    if (deltav[i] != 0)
			(void)sprintf(xinfo+strlen(xinfo),
				      " %c %d", deltav[i] > 0 ? '+' : '-',
				      deltav[i] > 0 ? deltav[i] : -deltav[i]);
		    (void)putinfo(xinfo);
		}
	}
	wfdbverbose();
    }
    wfdbquit();
    exit(0);	/*NOTREACHED*/
}


/* Calculate the greatest common divisor of x and y.  This function uses
   Euclid's algorithm, modified so that an exact answer is not required if the
   (possibly non-integral) arguments do not have a common divisor that can be
   represented exactly. */
double gcd(x, y)
double x, y;
{
    double tol;

    if (x > y) tol = 0.001*y;
    else tol = 0.001*x;

    while (1) {
	if (x > y && x-y > tol) x -= y;
	else if (y > x && y-x > tol) y -= x;
	else return (x);
    }
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
 "usage: %s -i IREC [OPTIONS ...]\n",
 "where IREC is the name of the record to be converted, and OPTIONS may",
 " include any of:",
 " -a ANNOTATOR [ANNOTATOR ...]  copy annotations for the specified ANNOTATOR",
 "              from IREC;  two or more ANNOTATORs may follow -a",
 " -c          clip output (default: wrap around if out-of-range)",
 " -d          add dither to the input if changing sampling frequency or gain",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          open the input record in `high resolution' mode",
 " -M          process multifrequency record without changing frequencies",
 " -n NREC     create a header file, using record name NREC and signal",
 "              descriptions from IREC",
 " -N NREC     as for -n, but copy signal descriptions from OREC",
 " -o OREC     produce output signal file(s) as specified by the header file",
 "              for record OREC",
 " -s SIGNAL [SIGNAL ...]  write only the specified signal(s)",
 " -S SCRIPT   take answers to prompts from SCRIPT (a text file)",
 " -t TIME     stop at specified time",
 " -u          adjust annotation times if needed to make them unique",
 "Unless you use `-o' to specify an *existing* header that describes the",
 "desired signal files, you will be asked for output specifications.  Use",
 "`-n' to name the record to be created.  Use `-s' to select a subset of the",
 "input signals, or to re-order them in the output file;  arguments that",
 "follow `-s' are *input* signal numbers (0,1,2,...) or names.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
