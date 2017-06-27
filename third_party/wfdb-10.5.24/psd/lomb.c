/* file: lomb.c		G. Moody	12 February 1992
			Last revised:	  27 July 2010
-------------------------------------------------------------------------------
lomb: Lomb periodogram of real data
Copyright (C) 1992-2010 George B. Moody

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

The default input to this program is a text file containing a sampled time
series, presented as two columns of numbers (the sample times and the sample
values).  The intervals between consecutive samples need not be uniform (in
fact, this is the most significant advantage of the Lomb periodogram over other
methods of power spectral density estimation).  This program writes the Lomb
periodogram (the power spectral density estimate derived from the input time
series) on the standard output.

The original version of this program was based on the algorithm described in
Press, W.H, and Rybicki, G.B., Astrophysical J. 338:277-280 (1989).  It has
been rewritten using the versions of this algorithm presented in Numerical
Recipes in C (Press, Teukolsky, Vetterling, and Flannery;  Cambridge U. Press,
1992).

This version agrees with 'fft' output (amplitude spectrum up to the Nyquist
frequency with total power equal to the variance);  thanks to Joe Mietus.
*/

#include <stdio.h>
#include <math.h>
#ifdef __STDC__
#include <stdlib.h>
#else
extern void exit();
#endif

#ifndef BSD
# include <string.h>
#else           /* for Berkeley UNIX only */
# include <strings.h>
# define strchr index
#endif

static long lmaxarg1, lmaxarg2;
#define LMAX(a,b) (lmaxarg1 = (a),lmaxarg2 = (b), (lmaxarg1 > lmaxarg2 ? \
						   lmaxarg1 : lmaxarg2))
static long lminarg1, lminarg2;
#define LMIN(a,b) (lminarg1 = (a),lminarg2 = (b), (lminarg1 < lminarg2 ? \
						   lminarg1 : lminarg2))
#define MOD(a,b)	while (a >= b) a -= b
#define MACC 4
#define SIGN(a,b) ((b) > 0.0 ? fabs(a) : -fabs(a))
static float sqrarg;
#define SQR(a) ((sqrarg = (a)) == 0.0 ? 0.0 : sqrarg*sqrarg)
#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

char *pname;
float pwr;

void fasper(x, y, n, ofac, hifac, wk1, wk2, nwk, nout, jmax, prob)
float x[], y[];
unsigned long n;
float ofac, hifac, wk1[], wk2[];
unsigned long nwk, *nout, *jmax;
float *prob;
{
    void avevar(), realft(), spread(), error();
    unsigned long j, k, ndim, nfreq, nfreqt;
    float ave, ck, ckk, cterm, cwt, den, df, effm, expy, fac, fndim, hc2wt,
          hs2wt, hypo, pmax, sterm, swt, var, xdif, xmax, xmin;

    *nout = 0.5*ofac*hifac*n;
    nfreqt = ofac*hifac*n*MACC;
    nfreq = 64;
    while (nfreq < nfreqt) nfreq <<= 1;
    ndim = nfreq << 1;
    if (ndim > nwk)
	error("workspaces too small\n");
    avevar(y, n, &ave, &var);
    xmax = xmin = x[1];
    for (j = 2; j <= n; j++) {
	if (x[j] < xmin) xmin = x[j];
	if (x[j] > xmax) xmax = x[j];
    }
    xdif = xmax - xmin;
    for (j = 1; j <= ndim; j++) wk1[j] = wk2[j] = 0.0;
    fac = ndim/(xdif*ofac);
    fndim = ndim;
    for (j = 1; j <= n; j++) {
	ck = (x[j] - xmin)*fac;
	MOD(ck, fndim);
	ckk = 2.0*(ck++);
	MOD(ckk, fndim);
	++ckk;
	spread(y[j] - ave, wk1, ndim, ck, MACC);
	spread(1.0, wk2, ndim, ckk, MACC);
    }
    realft(wk1, ndim, 1);
    realft(wk2, ndim, 1);
    df = 1.0/(xdif*ofac);
    pmax = -1.0;
    for (k = 3, j = 1; j <= (*nout); j++, k += 2) {
	hypo = sqrt(wk2[k]*wk2[k] + wk2[k+1]*wk2[k+1]);
	hc2wt = 0.5*wk2[k]/hypo;
	hs2wt = 0.5*wk2[k+1]/hypo;
	cwt = sqrt(0.5+hc2wt);
	swt = SIGN(sqrt(0.5-hc2wt), hs2wt);
	den = 0.5*n + hc2wt*wk2[k] + hs2wt*wk2[k+1];
	cterm = SQR(cwt*wk1[k] + swt*wk1[k+1])/den;
	sterm = SQR(cwt*wk1[k+1] - swt*wk1[k])/(n - den);
	wk1[j] = j*df;
	wk2[j] = (cterm+sterm)/(2.0*var);
	if (wk2[j] > pmax) pmax = wk2[(*jmax = j)];
    }
    expy = exp(-pmax);
    effm = 2.0*(*nout)/ofac;
    *prob = effm*expy;
    if (*prob > 0.01) *prob = 1.0 - pow(1.0 - expy, effm);
}

void spread(y, yy, n, x, m)
float y, yy[];
unsigned long n;
float x;
int m;
{
    int ihi, ilo, ix, j, nden;
    static int nfac[11] = { 0, 1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880 };
    float fac;
    void error();

    if (m > 10)
	error("factorial table too small");
    ix = (int)x;
    if (x == (float)ix) yy[ix] += y;
    else {
	ilo = LMIN(LMAX((long)(x - 0.5*m + 1.0), 1), n - m + 1);
	ihi = ilo + m - 1;
	nden = nfac[m];
	fac = x - ilo;
	for (j = ilo + 1; j <= ihi; j++) fac *= (x - j);
	yy[ihi] += y*fac/(nden*(x - ihi));
	for (j = ihi-1; j >= ilo; j--) {
	    nden = (nden/(j + 1 - ilo))*(j - ihi);
	    yy[j] += y*fac/(nden*(x - j));
	}
    }
}

void avevar(data, n, ave, var)
float data[];
unsigned long n;
float *ave, *var;
{
    unsigned long j;
    float s, ep;

    for (*ave = 0.0, j = 1; j <= n; j++) *ave += data[j];
    *ave /= n;
    *var = ep = 0.0;
    for (j = 1; j <= n; j++) {
	s = data[j] - (*ave);
	ep += s;
	*var += s*s;
    }
    *var = (*var - ep*ep/n)/(n-1);
    pwr = *var;
}

void realft(data, n, isign)
float data[];
unsigned long n;
int isign;
{
    void four1();
    unsigned long i, i1, i2, i3, i4, np3;
    float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
    double wr, wi, wpr, wpi, wtemp, theta;

    theta = 3.141592653589793/(double)(n>>1);
    if (isign == 1) {
	c2 = -0.5;
	four1(data, n>>1, 1);
    } else {
	c2 = 0.5;
	theta = -theta;
    }
    wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0 + wpr;
    wi = wpi;
    np3 = n+3;
    for (i = 2; i <= (n>>2); i++) {
	i4 = 1 + (i3 = np3 - (i2 = 1 + (i1 = i + i - 1)));
	h1r =  c1*(data[i1] + data[i3]);
	h1i =  c1*(data[i2] - data[i4]);
	h2r = -c2*(data[i2] + data[i4]);
	h2i =  c2*(data[i1] - data[i3]);
	data[i1] =  h1r + wr*h2r - wi*h2i;
	data[i2] =  h1i + wr*h2i + wi*h2r;
	data[i3] =  h1r - wr*h2r + wi*h2i;
	data[i4] = -h1i +wr*h2i + wi*h2r;
	wr = (wtemp = wr)*wpr - wi*wpi + wr;
	wi = wi*wpr + wtemp*wpi + wi;
    }
    if (isign == 1) {
	data[1] = (h1r = data[1]) + data[2];
	data[2] = h1r - data[2];
    } else {
	data[1] = c1*((h1r = data[1]) + data[2]);
	data[2] = c1*(h1r - data[2]);
	four1(data, n>>1, -1);
    }
}

void four1(data,nn,isign)
float data[];
unsigned long nn;
int isign;
{
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;

    n = nn << 1;
    j = 1;
    for (i = 1; i < n; i += 2) {
	if (j > i) {
	    SWAP(data[j], data[i]);
	    SWAP(data[j+1], data[i+1]);
	}
	m = n >> 1;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }
    mmax = 2;
    while (n > mmax) {
	istep = mmax << 1;
	theta = isign*(6.28318530717959/mmax);
	wtemp = sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi = sin(theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 1; m < mmax; m += 2) {
	    for (i = m;i <= n;i += istep) {
		j = i + mmax;
		tempr = wr*data[j] - wi*data[j+1];
		tempi = wr*data[j+1] + wi*data[j];
		data[j] = data[i] - tempr;
		data[j+1] = data[i+1] - tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr = (wtemp = wr)*wpr - wi*wpi + wr;
	    wi = wi*wpr + wtemp*wpi + wi;
	}
	mmax = istep;
    }
}

FILE *ifile;
float *x, *y, *wk1, *wk2;
long nmax = 512L;	/* Initial buffer size (must be a power of 2).
			   Note that input() will increase this value as
			   necessary by repeated doubling, depending on
			   the length of the input series. */

main(argc, argv)
int argc;
char *argv[];
{
    char *prog_name();
    float prob;
    int aflag = 1, i, sflag = 0, zflag = 0;
    unsigned long n, nout, jmax, maxout, input();
    void help(), zeromean();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'h':	    /* print help and exit */
	    help();
	    exit(0);
	    break;
	  case 's':	    /* smooth output */
	    sflag = 1;
	    break;
	  case 'P':	    /* output powers instead of amplitudes */
	    aflag = 0;
	    break;
	  case 'z':	    /* zero-mean the input */
	    zflag = 1;
	    break;
	  case '\0':	    /* read data from standard input */
	    ifile = stdin;
	    break;
	  default:
	    fprintf(stderr, "%s: unrecognized option %s ignored\n",
		    pname, argv[i]);
	    break;
	}
	else if (i == argc-1) {	/* last argument: input file name */
	    if ((ifile = fopen(argv[i], "rt")) == NULL) {
		fprintf(stderr, "%s: can't open %s\n", pname, argv[i]);
		exit(2);
	    }
	}
    }
    if (ifile == NULL) {
	help();
	exit(1);
    }

    /* Read input. */
    n = input();

    /* Zero-mean the input if requested. */
    if (zflag) zeromean(n);

    /* Compute the Lomb periodogram. */
    fasper(x-1, y-1, n, 4.0, 2.0, wk1-1, wk2-1, 64*nmax, &nout, &jmax, &prob);

    /* Write the results.  Output only up to Nyquist frequency, so that the
       results are directly comparable to those obtained using conventional
       methods.  The normalization is by half the number of output samples; the
       sum of the outputs is (approximately) the mean square of the inputs.

       Note that the Nyquist frequency is not well-defined for an irregularly
       sampled series.  Here we use half of the mean sampling frequency, but
       the Lomb periodogram can return (less reliable) estimates of frequency
       content for frequencies up to half of the maximum sampling frequency in
       the input.  */

    maxout = nout/2;
    if (sflag) {        /* smoothed */

        pwr /= 4;

        if (aflag)      /* smoothed amplitudes */
	    for (n = 0; n < maxout; n += 4)
	        printf("%g\t%g\n", wk1[n],
		      sqrt((wk2[n]+wk2[n+1]+wk2[n+2]+wk2[n+3])/(nout/(8.0*pwr))));

        else            /* smoothed powers */
	    for (n = 0; n < maxout; n += 4)
	        printf("%g\t%g\n", wk1[n],
		      (wk2[n]+wk2[n+1]+wk2[n+2]+wk2[n+3])/(nout/(8.0*pwr)));

    }
    else {    	        /* oversampled */

        if (aflag)      /* amplitudes */
	    for (n = 0; n < maxout; n++)
	        printf("%g\t%g\n", wk1[n], sqrt(wk2[n]/(nout/(2.0*pwr))));

        else            /* powers */
	    for (n = 0; n < maxout; n++)
	        printf("%g\t%g\n", wk1[n], wk2[n]/(nout/(2.0*pwr)));

    }

    free(x);
    free(y);
    free(wk1);
    free(wk2);

    exit(0);
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

void error(s)
char *s;
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

static char *help_strings[] = {
"usage: %s [ OPTIONS ...] INPUT-FILE\n",
" where INPUT-FILE is the name of a text file (use '-' to read the standard",
" input) containing a sampled time series, presented as two columns of",
" numbers (the sample times and the sample values).  The intervals between",
" consecutive samples need not be uniform.  The standard output is the Lomb",
" periodogram (a frequency spectrum derived from the time series, in two"
" columns (frequency and amplitude), normalized such that the total power",
" between zero and the Nyquist frequency (based on the mean sampling"
" frequency) is equal to the variance of the input.  If the units of sample",
" times are seconds, the units of the frequencies are Hz.",
" OPTIONS may be any of:",
" -h       Print on-line help.",
" -P       Generate a power spectrum (print squared amplitudes).",
" -s       Produce smoothed output.",
" -z       Zero-mean the input data.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}

/* Read input data, allocating and filling x[] and y[].  The return value is
   the number of points read.

   This function allows the input buffers to grow as large as necessary, up to
   the available memory (assuming that a long int is large enough to address
   any memory location). */

unsigned long input()
{
    unsigned long npts = 0L;

    if ((x = (float *)malloc(nmax * sizeof(float))) == NULL ||
	(y = (float *)malloc(nmax * sizeof(float))) == NULL ||
	(wk1 = (float *)malloc(64 * nmax * sizeof(float))) == NULL ||
	(wk2 = (float *)malloc(64 * nmax * sizeof(float))) == NULL) {
	if (x) (void)free(x);
	fclose(ifile);
	error("insufficient memory");
    }

    while (fscanf(ifile, "%f%f", &x[npts], &y[npts]) == 2) {
        if (++npts >= nmax) {	/* double the size of the input buffers */
	    float *xt, *yt, *w1t, *w2t;
	    unsigned long nmaxt = nmax << 1;

	    if (64 * nmaxt * sizeof(float) < nmax) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %lu\n",
		      pname, npts);
	        break;
	    }
	    if ((xt = (float *)realloc(x, nmaxt * sizeof(float))) == NULL) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %lu\n",
		      pname, npts);
	        break;
	    }
	    x = xt;
	    if ((yt = (float *)realloc(y, nmaxt * sizeof(float))) == NULL) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %lu\n",
		      pname, npts);
	        break;
	    }
	    y = yt;
	    if ((w1t = (float *)realloc(wk1,64*nmaxt*sizeof(float))) == NULL){
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %lu\n",
		      pname, npts);
	        break;
	    }
	    wk1 = w1t;
	    if ((w2t = (float *)realloc(wk2,64*nmaxt*sizeof(float))) == NULL){
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %lu\n",
		      pname, npts);
	        break;
	    }
	    wk2 = w2t;
	    nmax = nmaxt;
	}
    }

    fclose(ifile);
    if (npts < 1) error("no data read");
    return (npts);
}

/* This function calculates the mean of all sample values and subtracts it
   from each sample value, so that the mean of the adjusted samples is zero. */

void zeromean(n)
unsigned long n;
{
    unsigned long i;
    double ysum = 0.0;

    for (i = 0; i < n; i++)
	ysum += y[i];
    ysum /= n;
    for (i = 0; i < n; i++)
	y[i] -= ysum;
}
