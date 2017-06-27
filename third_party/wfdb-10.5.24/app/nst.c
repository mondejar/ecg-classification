/* file: nst.c		G. Moody	8 December 1983
			Last revised:    6 October 2009
-------------------------------------------------------------------------------
nst: Noise stress test
Copyright (C) 1983-2009 George B. Moody

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
#include <math.h>	/* declarations of pow(), sqrt() */
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

#ifdef NOMKSTEMP
#define mkstemp mktemp
#endif

/* Define the WFDB path component separator (OS-dependent). */
#ifdef MSDOS		/* for MS-DOS, OS/2, etc. */
# define PSEP	';'
#else
#ifdef MAC		/* for Macintosh */
# define PSEP	';'
#else			/* for UNIX, etc. */
# define PSEP	':'
#endif
#endif

char *pname, *prog_name();
double *gn;
int format = 16, nisig, nnsig, *nse, *vin, *vout, *z, *zz;
WFDB_Siginfo *si;

main(argc, argv)
int argc;
char *argv[];
{
    static char answer[20], buf[256], *wfdbp, irec[10], nrec[21], nnrec[20],
        orec[10], refaname[10], *protocol, tfname[20], *p, *s;
    double *anoise, *asig, *g, nsf = 0.0, nsr, *r, snr = -999999.0, ssf = 0.0;
    FILE *ifile;
    int i;
    long t, t0, tf, dt;
    WFDB_Anninfo ai;
    static WFDB_Annotation annot;
    void help(), nst();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* reference annotator name follows */
	    if (i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: reference annotator name must follow -a\n",
			      pname);
		exit(1);
	    }
	    (void)strncpy(refaname, argv[++i], 10);
	    break;
	  case 'F':	/* format follows */
	    if (i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: signal file format must follow -F\n",
			      pname);
		exit(1);
	    }
	    format = atoi(argv[++i]);
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* input record names follow */
	    if (i >= argc-2) {
		(void)fprintf(stderr,
		       "%s: input ECG and noise record names must follow -i\n",
			      pname);
		exit(1);
	    }
	    (void)strncpy(irec, argv[++i], 10);
	    (void)strncpy(nrec+1, argv[++i], 10);
	    nrec[0] = '+';
	    break;
	  case 'o':	/* output record name follows */
	    if (i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: output record name must follow -o\n",
			      pname);
		exit(1);
	    }
	    (void)strncpy(orec, argv[++i], 10);
	    break;
	  case 'p':	/* protocol annotator name follows */
	    if (i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: protocol annotator name must follow -p\n",
			      pname);
		exit(1);
	    }
	    protocol = argv[++i];
	    break;
	  case 's':	/* signal-to-noise ratio follows */
	    if (i >= argc-1) {
		(void)fprintf(stderr,
			  "%s: signal-to-noise ratio (in dB) must follow -s\n",
			      pname);
		exit(1);
	    }
	    snr = atof(argv[++i]);
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

    /* Make sure that the WFDB path begins with a '.' (current directory)
       component or an empty component (otherwise, nst may not be able to find
       the protocol file it generates). */
    wfdbp = getwfdb();
    if (*wfdbp != PSEP &&
	!(*wfdbp == '.' && ((*wfdbp+1) == PSEP || *(wfdbp+1) == ' '))) {
	char *nwfdbp;

	if ((nwfdbp = (char *)malloc(strlen(wfdbp+2))) == NULL) {
	    fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(1);
	}
	(void)sprintf(nwfdbp, "%c%s", PSEP, wfdbp);
	setwfdb(nwfdbp);
    }

    /* Generate a temporary file name for use below. */
    (void)strcpy(tfname, "nsXXXXXX");
    (void)mkstemp(tfname);

    /* Set the reference annotator name if it was not specified with -a. */
    if (refaname[0] == '\0')
	(void)strcpy(refaname, "atr");

    /* Get input record names if they were not specified with -i. */
    if (irec[0] == '\0' || nrec[0] == '\0') {
	(void)fprintf(stderr,
	   "Enter the name of the existing ECG record (or `?' for help): ");
	(void)fgets(irec, 10, stdin);
	irec[strlen(irec)-1] = '\0';
	if (strcmp(irec, "?") == 0) {
	    help();
	    exit(1);
	}
	ai.name = refaname; ai.stat = WFDB_READ;
	wfdbquiet();
	while (annopen(irec, &ai, 1) < 0) {
	    (void)fprintf(stderr,
			  "Can't read annotator `%s' for record `%s'\n",
			  ai.name, irec);
	    (void)fprintf(stderr,
			  "Enter the name of the reference annotator: ");
	    (void)fgets(refaname, 10, stdin);
	    refaname[strlen(refaname)-1] = '\0';
	}
	wfdbquit();
	wfdbverbose();
	(void)fprintf(stderr, "Enter the name of the existing noise record: ");
	nrec[0] = '+';
	(void)fgets(nrec+1, 10, stdin);
	nrec[strlen(nrec)-1] = '\0';
    }

    /* Get the output record name if it was not specified using -o. */
    if (orec[0] == '\0') {
	(void)fprintf(stderr, "Enter the name of the record to be created: ");
	(void)fgets(orec, 10, stdin);
	orec[strlen(orec)-1] = '\0';
    }

    /* Count the input signals. */
    if ((nisig = isigopen(irec, NULL, 0)) < 1) exit(2);
    wfdbquiet();   /* suppress warning if sampling frequencies don't match */
    if ((nnsig = isigopen(nrec, NULL, 0)) < 1) {
	(void)fprintf(stderr, "%s: can't read record %s\n", pname, nrec+1);
	exit(2);
    }
    wfdbverbose();

    /* Allocate storage. */
    if ((si = malloc((nisig+nnsig) * sizeof(WFDB_Siginfo))) == NULL ||
	(anoise = malloc(nnsig * sizeof(double))) == NULL ||
	(asig = malloc(nisig * sizeof(double))) == NULL ||
	(r = malloc(nisig * sizeof(double))) == NULL ||
	(g = malloc(nisig * sizeof(double))) == NULL ||
	(gn = malloc(nisig * sizeof(double))) == NULL ||
	(nse = malloc(nisig * sizeof(int))) == NULL ||
	(vin = malloc(nisig * sizeof(int))) == NULL ||
	(vout = malloc(nisig * sizeof(int))) == NULL ||
	(z = calloc(nisig, sizeof(int))) == NULL ||
	(zz = calloc(nisig, sizeof(int))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    for (i = 0; i < nisig; i++)
	g[i] = 0.;

    /* If the protocol was unspecified, generate one. */
    if (protocol == (char*)NULL) {
	/* Get the SNR if it was not specified using -s. */
	if (snr == -999999.0) {
	    (void)fprintf(stderr,
			  "Enter the desired signal-to-noise ratio in dB: ");
	    (void)fgets(answer, 10, stdin);
	    (void)sscanf(answer, "%lf", &snr);
	}
	/* Convert SNR in dB to nsr (RMS noise / p-p signal amplitude ratio) */
	nsr = pow(10., snr/-20.)*sqrt(2.0)/4.0;
	(void)sprintf(buf, "sigamp -r %s >%s", nrec+1, tfname);
	(void)fprintf(stderr,
		      "Checking RMS amplitude of record `%s' ...", nrec+1);
	if (system(buf) != 0) exit(1);
	(void)fprintf(stderr, " done\n");
	if ((ifile = fopen(tfname, "r")) == NULL ||
	    fscanf(ifile, "%lf", &anoise[0]) < 1 || anoise[0] == 0.0) {
	    (void)fprintf(stderr,
			  "%s: can't read temporary file %s\n", pname, tfname);
	    exit(1);
	}
	for (i = 1; fscanf(ifile, "%lf", &anoise[i]) == 1; i++) {
	    if (anoise[i] == 0.0) {
		(void)fprintf(stderr,
		 "%s: noise signal %d amplitude is zero -- can't normalize\n",
			      pname, i);
		exit(2);
	    }
	}
	(void)unlink(tfname);
	(void)sprintf(buf, "sigamp -a %s -r %s >%s", refaname, irec, tfname);
	(void)fprintf(stderr,
		      "Checking normal QRS amplitude of record `%s' ...",
		      irec);
	if (system(buf) != 0) exit(1);
	(void)fprintf(stderr, " done\n");
	if ((ifile = fopen(tfname, "r")) == NULL ||
	    fscanf(ifile, "%lf", &asig[0]) < 1 || asig[0] == 0.0) {
	    (void)fprintf(stderr, "%s: can't read temporary file %s\n", pname,
			  tfname);
	    exit(1);
	}
	for (i = 1; fscanf(ifile, "%lf", &asig[i]) == 1; i++) {
	    if (asig[i] == 0.0) {
		(void)fprintf(stderr,
		    "%s: ECG signal %d amplitude is zero -- can't normalize\n",
			      pname, i);
		exit(2);
	    }
	}
	(void)fclose(ifile);
	(void)unlink(tfname);

	/* Determine gains for noisy segments. */
	for (i = 0; i < nisig; i++) {
	    r[i] = nsr*asig[i]/anoise[i % nnsig];
	    (void)fprintf(stderr,
	       "[rec %s, sig %d] = [rec %s, sig %d] + %g * [rec %s, sig %d]\n",
			  orec, i, irec, i, r[i], nrec+1, i % nnsig);
	}

	/* Determine the length of the test. */
	if (sampfreq(irec) <= 0.) {
	    (void)setsampfreq(WFDB_DEFFREQ);
	    (void)fprintf(stderr, "Enter length of test [30:00]: ");
	    (void)fgets(buf, 20, stdin);
	    if (sscanf(buf, "%s", answer) == 0)
		(void)strcpy(answer, "30:0");
	}
	else
	    (void)strncpy(answer, mstimstr(strtim("e")), sizeof(answer));
	t0 = strtim("5:0");
	dt = strtim("2:0");
	tf = strtim(answer);
	if (tf <= 0L) tf = strtim("30:0");
	(void)setsampfreq(0.);

	(void)fprintf(stderr, "Generating protocol annotation file ...");
#ifdef MSDOS
	for (p = tfname; *p; p++)
	    if (*p == '.') {
		*p = '\0';
		break;
	    }
#endif
	ai.name = protocol = tfname;
	ai.stat = WFDB_WRITE;
	if (annopen(nrec+1, &ai, 1) < 0)
	    exit(3);
	annot.anntyp = NOTE;
	annot.aux = buf;
	for (t = t0; t < tf; t += dt) {
	    buf[0] = '\0';
	    for (i = 0; i < nisig; i++) {
		/* toggle g[i] from r[i] to 0 or vice versa */
		g[i] = r[i] - g[i];
		(void)sprintf(buf+strlen(buf), " %g", g[i]);
	    }
	    /* replace initial space with byte count */
	    buf[0] = strlen(buf+1);
	    annot.time = t;
	    (void)putann(0, &annot);
	}
	buf[0] = '\0';
	for (i = 0; i < nisig; i++)
	    (void)sprintf(buf+strlen(buf), " 0");
	buf[0] = strlen(buf+1);
	annot.time = tf;
	(void)putann(0, &annot);
	wfdbquit();
	(void)fprintf(stderr, " done\n");
	(void)fprintf(stderr, "The protocol annotation file is `%s'.\n",
		      wfdbfile(tfname, nrec+1));
    }

    /* Check sampling frequencies of input records. */
    wfdbquiet();
    if ((ssf = sampfreq(irec)) <= 0.) ssf = WFDB_DEFFREQ;
    (void)setsampfreq(0.);
    if ((nsf = sampfreq(nrec+1)) <= 0.) nsf = WFDB_DEFFREQ;
    (void)setsampfreq(0.);
    if (0.9*ssf > nsf || nsf > 1.1*ssf) {
	(void)fprintf(stderr,
	"Sampling frequencies of records `%s' and `%s' differ significantly\n",
		      irec, nrec+1);
	p = nnrec;
	s = nrec+1;
	while (*s != '\0' && *s != '_')
	    *p++ = *s++;
	(void)sprintf(p, "_%d", (int)ssf);
	nsf = sampfreq(nnrec);
	(void)setsampfreq(0.);
	if (0.9*ssf <= nsf && nsf <= 1.1*ssf) {
	    (void)fprintf(stderr, " ... substituting record `%s' for `%s'\n",
			  nnrec, nrec+1);
	    ai.name = protocol; ai.stat = WFDB_WRITE;
	    if (annopen(nnrec, &ai, 1) < 0) exit(2);
	    ai.stat = WFDB_READ;
	    if (annopen(nrec, &ai, 1) < 0) exit(2);
	    while (getann(0, &annot) == 0)
		(void)putann(0, &annot);
	    wfdbquit();
	    (void)unlink(wfdbfile(protocol, nrec+1));
	    (void)strcpy(nrec+1, nnrec);
	}
	else {
	    if (isigopen(nrec+1, si, -nnsig) != nnsig) {
		(void)fprintf(stderr, "\n%s: can't read record %s\n", pname,
			      nrec+1);
		exit(2);
	    }
	    (void)setsampfreq(ssf);
	    (void)sprintf(buf, "%s.dat", nnrec);
	    for (i = 0; i < nnsig; i++)
		si[i].fname = buf;
	    (void)setheader(tfname, si, (unsigned)nnsig);
	    wfdbquit();
	    (void)sprintf(buf, "xform -i %s -o %s -N %s -a %s",
			  nrec+1, tfname, nnrec, protocol);
	    (void)fprintf(stderr, " ... resampling record `%s' ...", nrec+1);
	    if (system(buf) != 0) exit(1);
	    (void)unlink(wfdbfile(protocol, nrec+1));
	    (void)unlink(wfdbfile("header", tfname));
	    (void)fprintf(stderr,
			  "Resampled noise record `%s' has been generated.\n",
			  nnrec);
	    (void)strcpy(nrec+1, nnrec);
	}
    }
    wfdbverbose();

    (void)fprintf(stderr, "Generating record `%s' ...", orec);
    nst(irec, nrec, protocol, orec, snr);
    (void)fprintf(stderr, " done\n");

    ai.name = refaname; ai.stat = WFDB_READ;
    if (annopen(irec, &ai, 1) < 0) exit(2);
    ai.stat = WFDB_WRITE;
    (void)sprintf(buf, "+%s", orec);
    if (annopen(buf, &ai, 1) < 0) exit(2);
    (void)fprintf(stderr,
		  "Copying reference annotations for record `%s' to `%s' ...",
		  irec, orec);
    while (getann(0, &annot) == 0)
	(void)putann(0, &annot);
    wfdbquit();
    (void)fprintf(stderr, " done\n");

    exit(0);	/*NOTREACHED*/
}

void nst(irec, nrec, protocol, orec, snr)
char *irec, *nrec, *protocol, *orec;
double snr;
{
    char buf[80], ofname[20], *p;
    int errct = 0, i;
    long nlen, nend, t = 0L, dt, next_tick;
    WFDB_Annotation annot;
    WFDB_Anninfo ai;

    /* Open ECG signals. */
    if (isigopen(irec, si, nisig) != nisig) exit(2);

    /* Open the noise record. */
    wfdbquiet();
    if (isigopen(nrec, si+nisig, nnsig) != nnsig) {
	(void)fprintf(stderr, "%s: can't read record %s\n", pname, nrec+1);
	exit(2);
    }
    wfdbverbose();

    /* Open the protocol annotation file. */
    ai.name = protocol;  ai.stat = WFDB_READ;
    if (annopen(nrec, &ai, 1) < 0) exit(2);

    /* Open the output signal file. */
    (void)sprintf(ofname, "%s.dat", orec);  /* name for output signal file */
    for (i = 0; i < nisig; i++) {
	si[i].fname = ofname;
	si[i].group = 0;
	zz[i] = si[i].adczero;		/* any offset will be removed */
	si[i].adczero = 0;
	si[i].fmt = format;
	gn[i] = 0.0;
	nse[i] = nisig + (i % nnsig);	/* signal number of noise signal to be
					   added to ECG signal i */
    }
    if (osigfopen(si, (unsigned)nisig) < nisig) exit(2);

    if ((nend = strtim("e")) <= 0L)
	nlen = nend = strtim("30:0");
    else
	nlen = nend;
    next_tick = dt = strtim("1:0");

    /* Output section.  On each iteration of the outer loop, an annotation
       from the protocol annotation file is read.  The time of that annotation
       indicates the time at which the program's state variables must be 
       changed next. */
    while (errct == 0 & getann(0, &annot) >= 0) {
	/* Skip any protocol annotations that aren't NOTE annotations, or
	   that don't have `aux' fields. */
	while (annot.anntyp != NOTE || annot.aux == NULL)
	    if (getann(0, &annot) < 0) goto cleanup;

	/* The inner loop is executed once per sample until the time specified
	   by the protocol annotation is reached. */
	while (t < annot.time && errct == 0) {
	    if (++t > next_tick) {
		next_tick += dt;
		(void)fprintf(stderr, ".");
	    }
	    /* If the end of the noise record has been reached, the input
	       pointer(s) for the noise signals are reset so that the next
	       noise samples to be read will be those from the beginning of the
	       record. */
	    if (--nlen <= 0L) {
		nlen = nend;
		for (i = nse[0]; i < nse[0]+nnsig; i++)
		    if (si[i].group != si[i-1].group &&
			isgsettime(si[i].group, 0L) < 0)
			errct++;
		if (getvec(vin) < 0)
		    errct++;
		/* Adjust offsets to avoid discontinuities. */
		for (i = 0; i < nisig; i++)
		    if (vin[i] != WFDB_INVALID_SAMPLE &&
			vin[nse[i]] != WFDB_INVALID_SAMPLE &&
			vout[i] != WFDB_INVALID_SAMPLE)
			z[i] = vin[i] + gn[i]*vin[nse[i]] - vout[i];
	    }
	    else
		if (getvec(vin) < 0)
		    errct++;
	    for (i = 0; i < nisig; i++) {
		if (vin[i] != WFDB_INVALID_SAMPLE &&
		    vin[nse[i]] != WFDB_INVALID_SAMPLE &&
		    vout[i] != WFDB_INVALID_SAMPLE) {
		    vin[i] -= zz[i];	/* remove any offset first */
		    vout[i] = vin[i] + gn[i]*vin[nse[i]] - z[i];
		}
		else
		    vout[i] = WFDB_INVALID_SAMPLE;
	    }
	    if (putvec(vout) < 0)
		errct++;
	}

	/* When the sample specified by the protocol annotation has been
	   reached, reset the state variables.  The `aux' string contains
	   noise gains. */
	for (i = 0, p = annot.aux+1; i < nisig && *p; i++) {
	    (void)sscanf(p, "%lf", &gn[i]);
	    z[i] = vin[i] + gn[i]*vin[nse[i]] - vout[i];
	    while (*p && *(p++) != ' ')
		;
	}
    }

    /* Cleanup section. */
  cleanup:
    (void)newheader(orec);
    if (snr == -999999.0)
	(void)sprintf(buf,
	       " Created by `%s' from records %s and %s, using protocol %s",
		pname, irec, nrec+1, protocol);
    else
	(void)sprintf(buf,
		" Created by `%s' from records %s and %s (SNR = %g dB)",
		pname, irec, nrec+1, snr);
    (void)putinfo(buf);
    wfdbquit();
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
 "usage: %s [OPTIONS ...]\n",
 "where OPTIONS may include any of:",
 " -a ANNOTATOR  copy annotations for the specified ANNOTATOR from SREC to",
 "                OREC (default: atr);  unless -p is used, normal QRS",
 "                annotations from this annotator are used in determining",
 "                signal amplitudes (hence noise scale factors)",
 " -F N          write output signals in format N (default: 16)",
 " -h            print this usage summary",
 " -i SREC NREC  read signals from record SREC, and noise from record NREC",
 " -o OREC       generate output record OREC",
 " -p PROTOCOL   use the specified PROTOCOL (in an annotation file with",
 "                annotator name PROTOCOL and record name NREC);  if this",
 "                option is omitted, a protocol annotation file is generated",
 "                using scale factors that may be set using -s",
 " -s SNR        set scale factors for noise such that the signal-to-noise",
 "                ratio during noisy segments is SNR (in dB);  this option",
 "                is ignored if a protocol is specified using -p",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
