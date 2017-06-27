/* file: calsig.c	G. Moody	 4 March 1991
			Last revised:  7 January 2009

-------------------------------------------------------------------------------
calsig: measure gains and baselines in a WFDB record and rewrite header
Copyright (C) 1991-2009 George B. Moody

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

`calsig' rewrites the header file for a WFDB record, setting the gain and
baseline fields based on measurements it makes.  Normally, the program is
used by specifying a time interval for the measurements;  best results will
be achieved if the specified interval is restricted to one or more square-wave
calibration pulses in each signal to be calibrated, although sine-wave pulses
may be usable if the sampling frequency and/or ADC resolution is high enough.
The baseline field is set only for signals that have been identified as
DC-coupled.  `calsig' also sets the units field for any signal it
calibrates (if the units field is not already set in the header file).

By default, the program constructs a smoothed amplitude histogram for each
signal and identifies its two principal modes.  Initially, each bin of the
histogram counts the number of samples in the analysis interval for which the
amplitude has a specified value.  The histogram is smoothed by applying a
low-pass filter which replaces the contents of each bin by a weighted sum of
several bins centered on the bin of interest.  The two principal modes in the
smoothed histogram must be separated by at least one bin with a count which is
less than one-eighth the count of the larger mode.  If this criterion is not
satisfied for a given signal, `calsig' warns the user and does not adjust
the gain or baseline for the affected signal.

`calsig' has two less rigorous techniques it can use if the algorithm above
fails.  Using the `-q' option, `calsig' takes the endpoints of the specified
interval as representative of the high and low values of the calibration
pulse.  Using the `-Q' option, `calsig' searches the interval for the
maximum and minimum amplitudes and takes these as the high and low values of
the calibration pulse.  These algorithms, particularly the `-Q' algorithm, are
highly sensitive to noise and are thus not recommended except in cases in which
the default algorithm fails;  note, however, that such cases are precisely
those in which noise is likely to be a problem for the alternate algorithms.
*/

#include <stdio.h>
#include <wfdb/wfdb.h>

#define UNITSLEN	20

int isiglist = 0, ncsig = 0, nsig;
int *vhigh, *vlow;
char **units;
double *high, *low;
int *do_cal;
int *dc;		/* dc[i] is 1 if signal i is DC-coupled */
struct info_rec {
    struct info_rec *next;
    char *info_string;
} *first_rec, *current_rec, *last_rec;
char *pname, *prog_name();
void rewrite_header(), help();
WFDB_Siginfo *s;
WFDB_Sample *b;
WFDB_Gain *g;

main(argc, argv)
int argc;
char *argv[];
{
    char *cfname=NULL, *p, *record = NULL, *t0p = NULL, *t1p = NULL, *getenv();
    int do_skip = 1,  **h, **ho, i, multi_seg = 0, n = 0, *o, qflag = 0,
	Qflag = 0, *v, vflag = 0, *vmax, *vmin;
    long nsamp, t, t0 = 0L, t1;

    /* Read and interpret command-line arguments. */
    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: file name must follow -c\n", pname);
		exit(1);
	    }
	    cfname = argv[i];
	    break;
	  case 'f':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: starting time must follow -f\n",
			      pname);
		exit(1);
	    }
	    t0p = argv[i];
	    break;
	  case 'h':    	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'q':	/* use the quick and dirty method */
	    qflag = 1;
	    break;
	  case 'Q':	/* use the alternate quick and dirty method */
	    Qflag = 1;
	    break;
	  case 'r':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal list follows */
	    isiglist = i+1; /* index of first argument containing a signal # */
	    while (i+1 < argc && *argv[i+1] != '-') {
		i++;
		ncsig++;	/* number of elements in signal list */
	    }
	    if (ncsig == 0) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
	    break;
	  case 't':
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: ending time must follow -t\n",
			      pname);
		exit(1);
	    }
	    t1p = argv[i];
	    break;
	  case 'v':
	    vflag = 1;
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

    /* Check that a record name was provided. */
    if (record == NULL) {
	help();
	exit(1);
    }

    /* Open the record. */
    if ((nsig = isigopen(record, NULL, 0)) < 1) exit(2);
    if ((s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(b = malloc(nsig * sizeof(WFDB_Sample))) == NULL ||
	(g = malloc(nsig * sizeof(WFDB_Gain))) == NULL ||
	(units = malloc(nsig * sizeof(char **))) == NULL ||
	(high = malloc(nsig * sizeof(double))) == NULL ||
	(low = malloc(nsig * sizeof(double))) == NULL ||
	(do_cal = malloc(nsig * sizeof(int))) == NULL ||
	(dc = malloc(nsig * sizeof(int))) == NULL ||
	(vhigh = malloc(nsig * sizeof(int))) == NULL ||
	(vlow = malloc(nsig * sizeof(int))) == NULL ||
	(h = malloc(nsig * sizeof(int *))) == NULL ||
	(ho = malloc(nsig * sizeof(int *))) == NULL ||
	(o = malloc(nsig * sizeof(int))) == NULL ||
	(v = malloc(nsig * sizeof(int))) == NULL ||
	(vmax = malloc(nsig * sizeof(int))) == NULL ||
	(vmin = malloc(nsig * sizeof(int))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    for (i = 0; i < nsig; i++)
	if ((units[i] = malloc((UNITSLEN+1) * sizeof(char))) == NULL) {
	    fprintf(stderr, "%s: insufficient memory\n", pname);
	    exit(2);
	}
    if (isigopen(record, s, nsig) != nsig) exit(2);

    if (strtim("e") != s[0].nsamp) multi_seg = 1;

    /* If a signal list was provided, validate it;  otherwise, generate one. */
    if (isiglist > 0) {
	for (i = 0; i < nsig; i++)
	    do_cal[i] = 0;
	for (i = 0; i < ncsig; i++) {
	    n = findsig(argv[isiglist+i]);
	    if (0 <= n && n < nsig)
		do_cal[i] = 1;
	}
    }
    else {
	for (i = 0; i < nsig; i++)
	    do_cal[i] = 1;
	ncsig = nsig;
    }
    if (ncsig < 1) {
	(void)fprintf(stderr, "%s: no signals to be calibrated\n", pname);
	exit(2);
    }

    /* Find the interval to be read. */
    if (t0p) {
	t0 = strtim(t0p);
	if (t0 < 0L) t0 = -t0;
    }
    if (t1p) {
	t1 = strtim(t1p);
	if (t1 < 0L) t1 = -t1;
	if (t1 <= t0) {
	    (void)fprintf(stderr, "%s: improper interval specified\n", pname);
	    exit(1);
	}
	nsamp = t1 - t0;
    }
    else
	nsamp =  strtim("1");

    /* Check if any of the signals to be calibrated is stored in difference
       format.  If so, avoid using isigsettime(). */
    for (i = 0; i < nsig; i++) {
	if (do_cal[i] && (s[i].fmt == 8)) {
	    do_skip = 0; break;
	}
    }

    /* Go to the first sample to be used for measurement. */
    if (do_skip) {
	if (isigsettime(t0) < 0) exit(2);
    }
    else {
	for (t = 0L; t < t0; t++)
	    if (getvec(v) < nsig) exit(2);
    }

    /* Get calibration specifications, interactively if necessary. */
    if (cfname == NULL) cfname = getenv("WFDBCAL");
    if (cfname) (void)calopen(cfname);
    for (i = 0; i < nsig; i++) {
	WFDB_Calinfo ci;

	if (do_cal[i]) {
	    char buf[21];

	    *units[i] = '\0';
	    low[i] = high[i] = 0;
	    dc[i] = -1;

	    /* Get a calibration record from the list if possible. */
	    if (getcal(s[i].desc, (char *)NULL, &ci) == 0) {
		if (!vflag) {
		    low[i] = ci.low;
		    high[i] = ci.high;
		}
		dc[i] = ci.caltype & WFDB_DC_COUPLED;
		(void)strncpy(units[i], ci.units, UNITSLEN);
	    }

	    /* If necessary, get the physical units. */
	    if (*units[i] == '\0') {
		do {
		    (void)fprintf(stderr,
				  "Signal %d units (up to %d characters): ",
				  i, UNITSLEN);
		    (void)fgets(units[i], UNITSLEN, stdin);
		} while (*units[i] == '\n');
		units[i][strlen(units[i])-1] = '\0';	/* strip off \n */
	    }

	    /* Make sure that the units string contains no whitespace. */
	    for (p = units[i]; *p; p++)
		if (*p == ' ' || *p == '\t') *p = '_';

	    /* Determine if signal is DC- or AC-coupled. */
	    while (dc[i] == -1) {
		(void)fprintf(stderr, "Is signal %d DC-coupled? (y/n): ", i);
		(void)fgets(buf, 20, stdin);
		if (*buf == 'y' || *buf == 'Y' || *buf == '\n')
		    dc[i] = 1;
		else if (*buf == 'n' || *buf == 'N')
		    dc[i] = 0;
	    }

	    /* If necessary, get the calibration pulse specifications. */
	    while (low[i] == high[i]) {
		if (dc[i] == 1) {
		    (void)fprintf(stderr,
				  "Signal %d calibration pulse limits\n", i);
		    (void)fprintf(stderr, "  Low value (in %s): ", units[i]);
		    (void)fgets(buf, 20, stdin);
		    (void)sscanf(buf, "%lf", &low[i]);
		    (void)fprintf(stderr, " High value (in %s): ", units[i]);
		    (void)fgets(buf, 20, stdin);
		    (void)sscanf(buf, "%lf", &high[i]);
		    if (low[i] == high[i])
		     (void)fprintf(stderr,
				   "Low and high values must be unequal!\n");
		}
		else {
		    low[i] = 0.0;
		    (void)fprintf(stderr, "Signal %d calibration pulse\n", i);
		    (void)fprintf(stderr, "  Amplitude (in %s): ", units[i]);
		    (void)fgets(buf, 20, stdin);
		    (void)sscanf(buf, "%lf", &high[i]);
		    if (low[i] == high[i])
		     (void)fprintf(stderr,
				   "Pulse must have a non-zero amplitude!\n");
		}
	    }
	}
    }

    /* Use the quick-and-dirty method if requested.  This method takes the
       endpoints as representing the high and low values of the calibration
       pulse. */
    if (qflag) {
	int vtemp;

	if (getvec(vlow) < nsig) exit(2);
	if (do_skip) {
	    if (isigsettime(t1) < 0) exit(2);
	}
	else
	    while (++t <= t1)
		if (getvec(v) < nsig) exit(2);
	if (getvec(vhigh) < nsig) exit(2);
	for (i = 0; i < nsig; i++)
	    if (do_cal[i]) {
		if (vlow[i] > vhigh[i]) {
		    vtemp = vlow[i];
		    vlow[i] = vhigh[i];
		    vhigh[i] = vtemp;
		}
		g[i] = ((double)vhigh[i] - vlow[i])/(high[i] - low[i]);
		if (dc[i])
		    b[i] = vhigh[i] - (high[i] * g[i]);
	    }
    }

    /* Use the alternate quick-and-dirty method if requested.  This method
       takes the maximum and minimum sample values as representing the high
       and low values of the calibration pulse. */
    else if (Qflag) {
	if (getvec(vlow) < nsig) exit(2);
	for (i = 0; i < nsig; i++)
	    vhigh[i] = vlow[i];
	for (t = t0+1; t < t1; t++) {
	    if (getvec(v) < nsig) exit(2);
	    for (i = 0; i < nsig; i++) {
		if (v[i] > vhigh[i]) vhigh[i] = v[i];
		else if (v[i] < vlow[i]) vlow[i] = v[i];
	    }
	}
	for (i = 0; i < nsig; i++)
	    if (do_cal[i]) {
		g[i] = ((double)vhigh[i] - vlow[i])/(high[i] - low[i]);
		if (dc[i])
		    b[i] = vhigh[i] - (high[i] * g[i]);
	    }
    }

    /* Otherwise, use the standard method.  This method uses an amplitude
       histogram of the samples, locates the two principal modes, and takes
       the amplitudes corresponding to these modes as the high and low
       values of the calibration pulse. */
    else {

	/* Allocate memory for the amplitude histograms. */
	for (i = 0; i < nsig; i++)
	    if (do_cal[i]) {
		if (s[i].adcres < 1) s[i].adcres = WFDB_DEFRES;
		if ((h[i] = (int *)calloc((unsigned)(1<<s[i].adcres),
					  sizeof(int))) == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n",
				  pname);
		    exit(3);
		}
		ho[i] = h[i] + (o[i] = (1 << (s[i].adcres-1)) - s[i].adczero);
		vmax[i] = s[i].adczero + (1 << (s[i].adcres-1)) - 1;
		vmin[i] = s[i].adczero - (1 << (s[i].adcres-1));
	    }

	/* Read the data. */
	while (nsamp-- > 0L) {
	    if (getvec(v) < nsig) exit(2);
	    for (i = 0; i < nsig; i++)
		if (do_cal[i]) {
		    if (v[i] < vmin[i] || v[i] > vmax[i]) {
			(void)fprintf(stderr,
			"%s: `%s' contains incorrect ADC data for signal %d\n",
				      pname, wfdbfile(s[i].fname, NULL), i);
			(void)fprintf(stderr,
	  " (specified range: %d to %d, but a sample value of %d was found)\n",
				      vmin[i], vmax[i], v[i]);
			exit(2);
		    }
		    else
			ho[i][v[i]]++;
		}
	}
	    
	/* Smooth the histograms, find the two principal modes, and determine
	   the gain and baseline. */
	for (i = 0; i < nsig; i++) {
	    if (do_cal[i]) {
		int dhs, hmax = -1, hmax2 = -1, hs, hthr, *hp = h[i], j, jj,
		    jmax = -1, jmax2 = -1, k, n=(1<<s[i].adcres), *r=NULL, wl;

		/* Smoothing is done using a triangular window function.  The
		   length of the window is wl (the larger of 8 and n/256, where
		   n is the number of bins in the histogram). */
		if ((wl = n >> 8) < 8) wl = 8;
		if ((r = (int *)malloc((unsigned)(wl+1)*sizeof(int))) == NULL){
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(3);
		}
	        for (jj = 0; jj < wl+1; jj++)
		    r[jj] = (jj >= (wl/2)+1) ? hp[jj-(wl/2)-1] : 0;
		/* hs is the smoothed histogram value for bin j, and dhs is the
		   first difference between the previous and current values of
		   hs;  the initializations are for j = -1 (with r[k] terms
		   omitted for 0<=k<=wl/2, since these are all 0 initially).
		   ** Watch out for integer overflow on 16-bit machines,
		   especially if the ADC resolution is high. ** */
		for (jj = wl/2-1, hs = 0, dhs = 0; jj > 0; jj--) {
		    hs += (wl/2-jj)*r[wl/2+jj];
		    dhs += r[wl/2+jj];
		}
		for (j = 0; j < n; j++) {
		    /* hs is computed by double summation from the previous hs,
		       the previous dhs, and the second difference, r[0] -
		       2r[wl/2] + r[wl]. */
		    hp[j] = hs += dhs += r[0] - 2*r[wl/2] + r[wl];
		    /* While smoothing the histogram, find the primary mode. */
		    if (hp[j] > hmax) { hmax = hp[j]; jmax = j; }
		    for (k = 0; k < wl; k++)
			r[k] = r[k+1];
		    r[wl] = (j+wl/2+1 < n) ? hp[j+wl/2+1] : 0;
		}

		/* Set the threshold to one-eighth of the primary mode. */
		hthr = hmax >> 3;

		/* Search past the primary mode for a sub-threshold bin. */
		for (j = jmax+1; j < n; j++)
		    if (hp[j] <= hthr) break;
		/* If a sub-threshold bin was found, search for a secondary
		   mode.  If the unweighted value of a bin exceeds the
		   threshold, the value is weighted in proportion to the
		   distance from the primary mode.  The secondary mode is
		   taken to be the bin with the largest weighted value;  this
		   criterion favors widely separated modes over closely spaced
		   modes. */
		if (j < n)
		    for ( ; j < n; j++)
			if (hp[j] > hthr && hp[j]*(j-jmax) > hmax2) {
			    hmax2 = hp[j]*(j-jmax);
			    jmax2 = j;
			}
		/* Search on the other side of the primary mode for a sub-
		   threshold bin. */
		for (j = jmax-1; j >= 0; j--)
		    if (hp[j] <= hthr) break;
		/* If successful, search for a secondary mode using the
		   criteria described above. */
		if (j >= 0)
		    for ( ; j >= 0; j--)
			if (hp[j] > hthr && hp[j]*(jmax-j) > hmax2) {
			    hmax2 = hp[j]*(jmax-j);
			    jmax2 = j;
			}
		if (jmax2 < 0)
		    (void)fprintf(stderr,
			    "%s: warning: cannot calibrate signal %d\n",
			    pname, i);
		else {
		    if (jmax2 > jmax) {
			int jtmp = jmax2;

			jmax2 = jmax;
			jmax = jtmp;
		    }
		    g[i] = ((double) jmax - jmax2)/(high[i] - low[i]);
		    if (dc[i])
			b[i] = jmax2 - (low[i]*g[i]) - o[i];
		}
		(void)free((char *)r);
	    }
	    if (do_cal[i] && h[i]) (void)free((char *)h[i]);
	}
    }
    if (multi_seg) {
	char buf[256], *hfname;
	FILE *hfile;
	int nseg;

	if ((hfname = wfdbfile("header", record)) == NULL) {
	    fprintf(stderr, "%s: can't find header for record %s\n",
		    pname, record);
	    exit(3);
	}
	if ((hfile = fopen(hfname, "rt")) == NULL) {
	    fprintf(stderr, "%s: can't read `%s'\n", pname, hfname);
	    exit(4);
	}
	fgets(buf, 256, hfile);
	for (p = buf; *p; p++)
	    if (*p==' ' || *p == '/' || *p == '\t' || *p == '\n' || *p == '\r')
	    break;
	if (*p != '/') {    	/* oops! this shouldn't happen */
	    fprintf(stderr, "%s: missing `/' in line 0 of `%s'\n",
		    pname, hfname);
	    fclose(hfile);
	    exit(5);
	}
	nseg = atoi(p+1);
	for (i = 0; i < nseg; i++) {
	    fgets(buf, 256, hfile);
	    for (p = buf; *p && *p != ' '; p++)
		;
	    *p = '\0';
	    (void)isigopen(buf, s, -nsig);
	    rewrite_header(buf);
	    wfdbquit();
	}
	fclose(hfile);
    }
    else {
	rewrite_header(record);
	wfdbquit();
    }
    exit(0);	/*NOTREACHED*/
}

void rewrite_header(record)
char *record;
{
    char *p;
    int i;

    /* Copy any `info' strings from the original header. */
    if (p = getinfo(record)) {
	do {
#ifndef lint
	    if ((current_rec =
		 (struct info_rec *)malloc(sizeof(struct info_rec))) == NULL ||
		(current_rec->info_string =
		 (char *)malloc((unsigned)(strlen(p)+1))) == NULL) {
		(void)fprintf(stderr, "%s: insufficient memory\n", pname);
		exit(3);
	    }
#endif
	    (void)strcpy(current_rec->info_string, p);
	    current_rec->next = NULL;
	    if (last_rec == NULL) 
		first_rec= last_rec = current_rec;
	    else {
		last_rec->next = current_rec;
		last_rec = current_rec;
	    }
	} while (p = getinfo((char *)NULL));
    }

    /* Preserve skews and byte offsets recorded in the existing header, and
       set the gains, baselines, units, and adcres fields. */
    for (i = 0; i < nsig; i++) {
	wfdbsetskew(i, wfdbgetskew(i));
	wfdbsetstart(i, wfdbgetstart(i));
	if (do_cal[i]) {
	    s[i].gain = g[i];
	    if (dc[i]) s[i].baseline = b[i];
	    s[i].units = *units[i] ? units[i] : NULL;
	    if (s[i].adcres < 1) s[i].adcres = WFDB_DEFRES;
	}
    }

    /* Write out the modified header file. */
    (void)setheader(record, s, (unsigned)nsig);

    /* Write any `info' strings out to the new header. */
    if (current_rec = first_rec) {
	do {
	    (void)putinfo(current_rec->info_string);
	    p = (char *)current_rec;
	    current_rec = current_rec->next;
	    (void)free(p);
	} while (current_rec);
	current_rec = first_rec = last_rec = NULL;
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
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the record to be calibrated, and OPTIONS are:",
 " -c FILE   obtain calibration pulse specifications from the specified FILE",
 "            (default: obtain this information from the file specified by",
 "            the environment variable WFDBCAL, or interactively)",
 " -f TIME   start at the specified TIME (default: beginning of record)",
 " -h        print this usage summary",
 " -q        make a quick-and-dirty estimate from the starting and ending",
 "            samples (as specified by -f and -t)",
 " -Q	     make an alternative quick-and-dirty estimate from the range of",
 "            the samples in the interval specified by -f and -t",
 " -s SIGNAL [SIGNAL ...]  calibrate the specified SIGNALs only (the first",
 "            signal is `0';  default: calibrate all signals)",
 " -t TIME   stop at the specified TIME (default: 1 second after the start)",
 " -v        ask for cal pulse limits (default: use limits in WFDBCAL file)",
 "If the `-s' option is used, only the specified signals are calibrated, and",
 "gains and baselines for any other signals are not changed.  Thus, if",
 "calibration pulses are not simultaneously available in all signals, run",
 "this program repeatedly with different time intervals and signal lists.",
 NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
