/* file: memse.c	G. Moody	6 February 1992
			Last revised:	 13 March 2006

-------------------------------------------------------------------------------
memse: Estimate power spectrum using maximum entropy (all poles) method
Copyright (C) 1992-2006 George B. Moody

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

This program has been written to behave as much like 'fft' as possible.  The
input and output formats and many of the options are the same.  See the man
page (memse.1) for details.

This version agrees with 'fft' output (amplitude spectrum with total power
equal to the variance);  thanks to Joe Mietus.

The function 'integ' (to compute total power over a band), and the code using
'integ' to summarize power in several bands of interest for HRV analysis, was
contributed by Peter P. Domitrovich.
*/

#include <stdio.h>
#include <math.h>
#ifndef __STDC__
extern double atof();
#else
#include <stdlib.h>
#endif

#ifndef BSD
# include <string.h>
#else           /* for Berkeley UNIX only */
# include <strings.h>
# define strchr index
#endif

#define PI	M_PI	/* pi to machine precision, defined in math.h */ 

#ifdef i386
#define strcasecmp strcmp
#endif

double wsum = 0.0;

double (*window)(int j, long n);
double win_bartlett(int j, long n);
double win_blackman(int j, long n);
double win_blackman_harris(int j, long n);
double win_hamming(int j, long n);
double win_hanning(int j, long n);
double win_parzen(int j, long n);
double win_square(int j, long n);
double win_welch(int j, long n);
char *prog_name(char *p);
int detrend(double *ordinates, long n_ordinates);
void error(char *error_message);
void help(void);
int memcof(double *data, long n, long m, double *pm, double *cof);
double evlmem(double f, double *cof, long m, double pm);
long input(void);
double integ(double *cof, long poles, double pm, double lowfreq,
	     double highfreq, double tolerance);

/* See Oppenheim & Schafer, Digital Signal Processing, p. 241 (1st ed.) */
double win_bartlett(int j, long n)
{
    double a = 2.0/(n-1), w = 0.0;
    if ((w = j*a) > 1.0) w = 2.0 - w;
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.) */
double win_blackman(int j, long n)
{
    double a = 2.0*PI/(n-1), w = 0.0;

    w = 0.42 - 0.5*cos(a*j) + 0.08*cos(2*a*j);
    wsum += w;
    return (w);
}

/* See Harris, F.J., "On the use of windows for harmonic analysis with the
   discrete Fourier transform", Proc. IEEE, Jan. 1978 */
double win_blackman_harris(int j, long n)
{
    double a = 2.0*PI/(n-1), w = 0.0;

    w = 0.35875 - 0.48829*cos(a*j) + 0.14128*cos(2*a*j) - 0.01168*cos(3*a*j);
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.) */
double win_hamming(int j, long n)
{
    double a = 2.0*PI/(n-1), w = 0.0;

    w = 0.54 - 0.46*cos(a*j);
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.)
   The second edition of Numerical Recipes calls this the "Hann" window. */
double win_hanning(int j, long n)
{
    double a = 2.0*PI/(n-1), w = 0.0;

    w = 0.5 - 0.5*cos(a*j);
    wsum += w;
    return (w);
}

/* See Press, Flannery, Teukolsky, & Vetterling, Numerical Recipes in C,
   p. 442 (1st ed.) */
double win_parzen(int j, long n)
{
    double a = (n-1)/2.0, w = 0.0;

    if ((w = (j-a)/(a+1)) > 0.0) w = 1 - w;
    else w = 1 + w;
    wsum += w;
    return (w);
}

/* See any of the above references. */
double win_square(int j, long n)
{
    if (j < n)		/* to quiet the compiler */
       wsum += 1.0;
    return (1.0);
}

/* See Press, Flannery, Teukolsky, & Vetterling, Numerical Recipes in C,
   p. 442 (1st ed.) or p. 554 (2nd ed.) */
double win_welch(int j, long n)
{
    double a = (n-1)/2.0, w = 0.0;

    w = (j-a)/(a+1);
    w = 1 - w*w;
    wsum += w;
    return (w);
}

char *pname = NULL;
FILE *ifile = NULL;
double *data = NULL, *wk1 = NULL, *wk2 = NULL;
long nmax = 512L;	/* Initial buffer size (must be a power of 2).
			   Note that input() will increase this value as
			   necessary by repeated doubling, depending on
			   the length of the input series. */
double pm = 0.0;
double *cof = NULL;
double *wkm = NULL;
int fflag = 0;
long len = 0;
long nout = 0;
long poles = 0;
int wflag = 0;
int zflag = 0;
double freq = 0.0;

void band_power(double f0, double f1)
{
    double f;
    static int first_band = 1;

    if (f0 < 0.0) f0 = 0.0;
    if (f1 < 0.0) f1 = 0.0;
    if (f0 > f1) { f = f0; f0 = f1; f1 = f; }
    if (f0 == f1) return;
    if (first_band) {
	printf("\nModel order = %ld\n", poles);
	printf("     Band (Hz)\t\t  Power\n");
	first_band = 0;
    }
    printf("%lf - %lf\t%lf\n", f0, f1,
	   2 * integ(cof, poles, pm, f0, f1, 0.0000001));
}

int main(int argc, char **argv)
{
    int i = 0, pflag = 0, sflag = 0, fi, fi0 = -1, fi1 = -1;
    double df = 0.0;
    double f = 0.0, f0, f1, p = 0.0;

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'b':	/* print power in specified bands */
	    if (++i > argc-1 || *argv[i] == '-')
		error("at least two frequencies must follow -b");
	    fi0 = i;
	    while (++i < argc && *argv[i] != '-')
		;
	    fi1 = i--;
	    break;
	  case 'f':	/* print frequencies */
	    if (++i >= argc)
                error("sampling frequency must follow -f");
	    freq = atof(argv[i]);
	    fflag = 1;
	    break;
	  case 'h':	/* print help and exit */
	    help();
	    exit(0);
	    break;
	  case 'l':	/* handle up to n-point input series (obsolete) */
	    fprintf(stderr,
	    "%s: -l option is obsolete (%s can handle inputs of any length)\n",
		    pname, pname);
	    ++i;
	    break;
	  case 'n':	/* print n equally-spaced output values */
	    if (++i >= argc)
                error("output length must follow -n");
	    nout = atoi(argv[i]);
	    break;
	  case 'o':	/* specify the model order (number of poles) */
	    if (++i >= argc)
                error("order (number of poles) must follow -o");
	    poles = atoi(argv[i]);
	    break;
	  case 'P':     /* print power spectrum (squared magnitudes) */
	    pflag = 1;
	    break;
	  case 's':	/* summarize power in HRV bands of interest */
	    sflag = 1;
	    break;
	  case 'w':	/* apply windowing function to input */
	    if (++i >= argc)
                error("window type must follow -w");
	    if (strcasecmp(argv[i], "Bartlett") == 0)
		window = win_bartlett;
	    else if (strcasecmp(argv[i], "Blackman") == 0)
		window = win_blackman;
	    else if (strcasecmp(argv[i], "Blackman-Harris") == 0)
		window = win_blackman_harris;
	    else if (strcasecmp(argv[i], "Hamming") == 0)
		window = win_hamming;
	    else if (strcasecmp(argv[i], "Hanning") == 0 ||
		     strcasecmp(argv[i], "Hann") == 0)
		window = win_hanning;
	    else if (strcasecmp(argv[i], "Parzen") == 0)
		window = win_parzen;
	    else if (strcasecmp(argv[i], "Dirichlet") == 0 ||
		     strcasecmp(argv[i], "Rectangular") == 0 ||
		     strcasecmp(argv[i], "Square") == 0)
		window = win_square;
	    else if (strcasecmp(argv[i], "Welch") == 0)
		window = win_welch;
	    else {
		fprintf(stderr, "%s: unrecognized window type %s\n",
			pname, argv[i]);
		exit(1);
	    }
	    wflag = 1;
	    break;
	  case 'z':			/* zero-mean the input */
	    zflag = 1;
	    break;
	  case 'Z':			/* zero-mean and detrend the input */
	    zflag = 2;
	    break;
	  case '\0':			/* read data from standard input */
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

    /* Read the input series. */
    len = input( );

    /* Check the model order. */
    if (poles > len) poles = len;
    if ((double)poles*poles > len)
	fprintf(stderr, "%s: the model order (number of poles) may be too high\n", pname);

    /* Set the model order to a reasonable value if it is unspecified. */
    if (poles == 0) {
	poles = (int)(sqrt((double)len) + 0.5);
	fprintf(stderr, "%s: using a model order of %ld\n", pname, poles);
    }

    /* Allocate arrays for coefficients. */
    if (((cof = (double *)malloc((unsigned)poles*sizeof(double))) == NULL) ||
	((wkm = (double *)malloc((unsigned)poles*sizeof(double))) == NULL)) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(1);
    }

    /* Zero-mean, detrend, and/or window the input series as required. */
    if (zflag) {
	double rmean = 0, rsum = 0;

	for (i = 0; i < len; i++)
	    rsum += data[i];
	rmean = rsum/len;
	for (i = 0; i < len; i++)
	    data[i] -= rmean;
	if (zflag == 2)
	    detrend(data, len);
    }
    if (wflag)
	for (i = 0; i < len; i++)
	    data[i] *= (*window)(i, len);

    /* Calculate coefficients for MEM spectral estimation. */
    memcof(data, len, poles, &pm, cof);

    /* If the number of output points was not specified, choose the largest
      power of 2 less than len, plus 1 (so that the number of output points
      matches that produced by an FFT). */
    if (nout == 0) {
	while (nmax >= len)
	    nmax /= 2;
	nout = nmax + 1;
    }

    /* Print outputs. */
    if (nout > 1)
       for (i = 0, df = 0.5/((double)(nout-1)); i < nout; i++) {
	   f = i*df*freq;
	   p = evlmem(i*df, cof, poles, pm)/((double)(nout-1));
	   if (fflag) printf("%lf\t", i*df*freq);
	   if (pflag)
	       printf("%lf\n",
		      evlmem(i*df, cof, poles, pm)/((double)(nout-1)));
	   else
	       printf("%lf\n",
		      sqrt(evlmem(i*df, cof, poles, pm)/((double)(nout-1))));
       }
    else {
	free(cof);
	free(data);
	free(wk1);
	free(wk2);
	free(wkm);
	error("no output produced");
    }

    if (sflag) {
	band_power(0.0, 0.5);
	band_power(0.0, 0.0033);
	band_power(0.0033, 0.04);
	band_power(0.04, 0.15);
	band_power(0.15, 0.4);
	band_power(0.4, 0.5);
    }
    for (fi = fi0; fi < fi1-1; ) {
	f0 = atof(argv[fi++]);
	f1 = atof(argv[fi++]);
	band_power(f0, f1);
    }

    free(cof);
    free(data);
    free(wk1);
    free(wk2);
    free(wkm);

    return 0;
}

/* Calculate coefficients for MEM spectral estimation.  See Numerical Recipes,
   pp. 447-451. */
int memcof(double *data, long n, long m, double *pm, double *cof)
{
    int i = 0, j = 0, k = 0;
    double denom = 0.0, num = 0.0, p = 0.0;

    for (j = 0; j < n; j++)
	p += data[j]*data[j];
    *pm = p/n;
    wk1[0] = data[0];
    wk2[n-2] = data[n-1];
    for (j = 1; j < n-1; j++) {
	wk1[j] = data[j];
	wk2[j-1] = data[j];
    }
    for (k = 0; k < m; k++) {
	for (j = 0, num = denom = 0.0; j < n-k-1; j++) {
	    num += wk1[j]*wk2[j];
	    denom += wk1[j]*wk1[j] + wk2[j]*wk2[j];
	}
	cof[k] = 2.0*num/denom;
	*pm *= 1.0 - cof[k]*cof[k];
	if (k)
	    for (i = 0; i < k; i++)
		cof[i] = wkm[i] - cof[k]*wkm[k-i-1];
	if (k != m-1) {
	    for (i = 0; i <= k; i++)
		wkm[i] = cof[i];
	    for (j = 0; j < n-k-2; j++) {
		wk1[j] -= wkm[k]*wk2[j];
		wk2[j] = wk2[j+1] - wkm[k]*wk1[j+1];
	    }
	}
    }
    return 0;
}

/* Evaluate power spectral estimate at f (0 <= f < = 0.5, where 1 is the
   sampling frequency), given MEM coefficients in cof[0 ... m-1] and pm
   (see Numerical Recipes, pp. 451-452). */
double evlmem(double f, double *cof, long m, double pm)
{
    int i = 0;
    double sumr = 1.0, sumi = 0.0;
    double wr = 1.0, wi = 0.0, wpr = 0.0, wpi = 0.0, wt = 0.0, theta = 0.0;

    theta = 2.0*PI*f;
    wpr = cos(theta);
    wpi = sin(theta);
    for (i = 0; i < m; i++) {
	wt = wr;
	sumr -= cof[i]*(wr = wr*wpr - wi*wpi);
	sumi -= cof[i]*(wi = wi*wpr + wt*wpi);
    }
    return (pm/(sumr*sumr+sumi*sumi));
}


/* This function detrends (subtracts a least-squares fitted line from)
   a sequence of n uniformly spaced ordinates supplied in c. */
int detrend(double *c, long n)
{
    int i = 0;
    double a = 0.0, b = 0.0, tsqsum = 0.0, ysum = 0.0, t = 0.0;

    for (i = 0; i < n; i++)
	ysum += c[i];
    for (i = 0; i < n; i++) {
	t = i - n/2 + 0.5;
	tsqsum += t*t;
	b += t*c[i];
    }
    b /= tsqsum;
    a = ysum/n - b*(n-1)/2.0;
    for (i = 0; i < n; i++)
	c[i] -= a + b*i;
    if (b < -0.04 || b > 0.04)
	fprintf(stderr,
		"%s: (warning) possibly significant trend in input series\n",
		pname);
    return 0;
}

char *prog_name(char *s)
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

void error(char *s)
{
    fprintf(stderr, "%s: %s\n", pname, s);
    exit(1);
}

static char *help_strings[] = {
"usage: %s [ OPTIONS ...] INPUT-FILE\n",
" where INPUT-FILE is the name of a text file containing a time series",
" (use `-' to read the standard input), and OPTIONS may be any of:",
" -b LF HF Print the power in the frequency band defined by LF and HF.  More",
"          than one band may be specified following a single -b option.",
" -f FREQ  Show the center frequency for each bin in the first column.  The",
"          FREQ argument specifies the input sampling frequency;  the center",
"          frequencies are given in the same units.",
" -h       Print on-line help.",
" -n N     Print N equally-spaced output values; default: N = half the number",
"          of input samples.",
" -o P     Specify the model order (number of poles); default: P = the square",
"          root of the number of input samples.",
" -P       Generate a power spectrum (print squared magnitudes).",
" -s       Print a summary of power in bands of interest for HRV analysis.",
" -w WINDOW",
"          Apply the specified WINDOW to the input data.  WINDOW may be one",
"          of: `Bartlett', `Blackman', `Blackman-Harris', `Hamming',",
"          `Hanning', `Parzen', `Square', and `Welch'.",
" -z       Zero-mean the input data.",
" -Z       Detrend and zero-mean the input data.",
NULL
};

void help(void)
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++) {
	(void)fprintf(stderr, "%s\n", help_strings[i]);
	if (i % 23 == 0) {
	    char b[5];
	    (void)fprintf(stderr, "--More--");
	    (void)fgets(b, 5, stdin);
	    (void)fprintf(stderr, "\033[A\033[2K"); /* erase "--More--";
						       assumes ANSI terminal */
	}
    }
}

/* Read input data, allocating and filling x[] and y[].  The return value is
   the number of points read.

   This function allows the input buffers to grow as large as necessary, up to
   the available memory (assuming that a long int is large enough to address
   any memory location). */

long input( )
{
    long npts = 0L;

    if (((data = (double *)malloc(nmax * sizeof(double))) == NULL) ||
	((wk1 = (double *)malloc(64 * nmax * sizeof(double))) == NULL) ||
	((wk2 = (double *)malloc(64 * nmax * sizeof(double))) == NULL)) {
	if (data) (void)free(data);
	if (wk1) (void)free(wk1);
	fclose(ifile);
        error("insufficient memory");
    }

    while (fscanf(ifile, "%lf", &data[npts]) == 1) {
        if (++npts >= nmax) {	/* double the size of the input buffers */
	    long nmaxt = nmax << 1;
	    double *datat = NULL, *w1t = NULL, *w2t = NULL;

	    if ((long)(nmaxt * sizeof(double)) < nmax) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %ld\n",
			pname, npts);
	        break;
	    }
	    if ((datat = (double *)realloc(data,nmaxt*sizeof(double)))==NULL) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %ld\n",
			pname, npts);
	        break;
	    }
	    data = datat;
	    if ((w1t = (double *)realloc(wk1,64*nmaxt*sizeof(double)))==NULL) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %ld\n",
			pname, npts);
	        break;
	    }
	    wk1 = w1t;
	    if ((w2t = (double *)realloc(wk2,64*nmaxt*sizeof(double)))==NULL) {
		fprintf(stderr,
		      "%s: insufficient memory, truncating input at row %ld\n",
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

/* Function 'integ' was contributed by Peter P. Domitrovich, who translated it
   from a FORTRAN version by an unknown author from a book written in Chinese.
   This code is designed to integrate functions with sharp peaks. */
double integ(double *aa, long m, double ee, double a, double b, double epsilon)
{
  double f[3][31], fm[3][31], e[3][31], krtn[31];
  double sum = 0.0, t = 1.0, absa = 1.0, est = 1.0, f1 = 0.0, fa = 0.0,
      fb = 0.0, fp = 0.0, x = a, da = b - a, dx = 0.0, sx = 0.0, fm1 = 0.0,
      e1 = 0.0, s = 0.0;
  long l = 0, k = 0;

  fa = evlmem(a, aa, m, ee);
  fb = evlmem(b, aa, m, ee);
  fp = 4.0 * evlmem(0.5 * (a + b), aa, m, ee);
  while (1) {
      k = 1;
      l++;
      t *= 1.7;
      dx = da / 3.0;
      sx = dx / 6.0;
      fm1 = 4.0 * evlmem(x + 0.5 * dx, aa, m, ee);
      f1 = evlmem(x + dx, aa, m, ee);
      fm[1][l] = fp;
      f[1][l] = evlmem(x + 2.0 * dx, aa, m, ee);
      fm[2][l] = 4.0 * evlmem (x + 2.5 * dx, aa, m, ee);
      f[2][l] = fb;
      e1 = sx * (fa + fm1 + f1);
      e[1][l] = sx * (f1 + fp + f[1][l]);
      e[2][l] = sx * (f[1][l] + fm[2][l] + fb);
      s = e1 + e[1][l] + e[2][l];
      absa = absa - fabs(est) + fabs(e1) + fabs(e[1][l]) + fabs(e[2][l]);
      if (fabs (est - 1.0) < 1.0e-06) {
	  est = e1;
	  fp = fm1;
	  fb = f1;
	  da = dx;
	  krtn[l] = k;
	  continue;
      }
      if (t * fabs (est - s) <= epsilon * absa) {
	  sum += s;
	  do {
	      l--;
	      t /= 1.7;
	      k = krtn[l];
	      dx *= 3.0;
	      if (k == 3 && l - 1 <= 0)
		  return sum;
	  } while (k == 3 && l - 1 > 0);
	  est = e[k][l];
	  fp = fm[k][l];
	  fa = fb;
	  fb = f[k][l];
	  k++;
	  x += da;
	  da = dx;
	  krtn[l] = k;
	  continue;
	}
      if (l < 30) {
	  est = e1;
	  fp = fm1;
	  fb = f1;
	  da = dx;
	  krtn[l] = k;
	  continue;
      }
      sum += 5.0;
      do {
	  l--;
	  t /= 1.7;
	  k = krtn[l];
	  dx *= 3.0;
	  if (k == 3 && l - 1 <= 0)
	      return sum;
      } while (k == 3 && l - 1 > 0);
      est = e[k][l];
      fp = fm[k][l];
      fa = fb;
      fb = f[k][l];
      k++;
      x += da;
      da = dx;
      krtn[l] = k;
  }
}

