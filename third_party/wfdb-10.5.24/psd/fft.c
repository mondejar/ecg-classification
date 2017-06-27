/* file: fft.c		G. Moody	24 February 1988
		   Last revised:	27 October 2008

-------------------------------------------------------------------------------
fft: Fast Fourier transform of real data
Copyright (C) 1988-2005 George B. Moody

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
   
The default input to this program is a text file containing a uniformly sampled
time series, presented as a column of numbers.  The default standard output is
the magnitude of the discrete Fourier transform of the input series, normalized
by the length of the FFT.

The functions `realft' and `four1' are based on those in Press, W.H., et al.,
Numerical Recipes in C: the Art of Scientific Computing (Cambridge Univ. Press,
1989;  2nd ed., 1992).
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

#define	LEN	16384		/* maximum points in FFT */
#ifdef i386
#define strcasecmp strcmp
#endif
#define PI	M_PI	/* pi to machine precision, defined in math.h */
#define TWOPI	(2.0*PI)

void four1();
void realft();

double wsum;

/* See Oppenheim & Schafer, Digital Signal Processing, p. 241 (1st ed.) */
double win_bartlett(j, n)
int j, n;
{
    double a = 2.0/(n-1), w;

    if ((w = j*a) > 1.0) w = 2.0 - w;
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.) */
double win_blackman(j, n)
int j, n;
{
    double a = 2.0*PI/(n-1), w;

    w = 0.42 - 0.5*cos(a*j) + 0.08*cos(2*a*j);
    wsum += w;
    return (w);
}

/* See Harris, F.J., "On the use of windows for harmonic analysis with the
   discrete Fourier transform", Proc. IEEE, Jan. 1978 */
double win_blackman_harris(j, n)
int j, n;
{
    double a = 2.0*PI/(n-1), w;

    w = 0.35875 - 0.48829*cos(a*j) + 0.14128*cos(2*a*j) - 0.01168*cos(3*a*j);
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.) */
double win_hamming(j, n)
int j, n;
{
    double a = 2.0*PI/(n-1), w;

    w = 0.54 - 0.46*cos(a*j);
    wsum += w;
    return (w);
}

/* See Oppenheim & Schafer, Digital Signal Processing, p. 242 (1st ed.)
   The second edition of Numerical Recipes calls this the "Hann" window. */
double win_hanning(j, n)
int j, n;
{
    double a = 2.0*PI/(n-1), w;

    w = 0.5 - 0.5*cos(a*j);
    wsum += w;
    return (w);
}

/* See Press, Flannery, Teukolsky, & Vetterling, Numerical Recipes in C,
   p. 442 (1st ed.) */
double win_parzen(j, n)
int j, n;
{
    double a = (n-1)/2.0, w;

    if ((w = (j-a)/(a+1)) > 0.0) w = 1 - w;
    else w = 1 + w;
    wsum += w;
    return (w);
}

/* See any of the above references. */
double win_square(j, n)
int j, n;
{
    wsum += 1.0;
    return (1.0);
}

/* See Press, Flannery, Teukolsky, & Vetterling, Numerical Recipes in C,
   p. 442 (1st ed.) or p. 554 (2nd ed.) */
double win_welch(j, n)
int j, n;
{
    double a = (n-1)/2.0, w;

    w = (j-a)/(a+1);
    w = 1 - w*w;
    wsum += w;
    return (w);
}

char *pname;
double rsum;
FILE *ifile;
int m, n;
int cflag;
int decimation = 1;
int iflag;
int fflag;
int len = LEN;
int nflag;
int Nflag;
int nchunks;
int pflag;
int Pflag;
int smooth = 1;
int wflag;
int zflag;
static float *c;
double freq, fstep, norm, rmean, (*window)();

main(argc, argv)
int argc;
char *argv[];
{
    int i;
    char *prog_name();
    double atof();
    void help();

    pname = prog_name(argv[0]);
    if (--argc < 1) {
	help();
	exit(1);
    }
    else if (strcmp(argv[argc], "-") == 0)
	ifile = stdin;		/* read data from standard input */
    else if ((ifile = fopen(argv[argc], "rt")) == NULL) {
	fprintf(stderr, "%s: can't open %s\n", pname, argv[argc]);
	exit(2);
    }
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':	/* print complex FFT (rectangular form) */
	    cflag = 1;
	    break;
	  case 'f':	/* print frequencies */
	    if (++i >= argc) {
		fprintf(stderr, "%s: sampling frequency must follow -f\n",
			pname);
		exit(1);
	    }
	    freq = atof(argv[i]);
	    fflag = 1;
	    break;
	  case 'h':	/* print help and exit */
	    help();
	    exit(0);
	    break;
	  case 'i':	/* calculate inverse FFT from `fft -c' format input */
	    if (argc > 2) {
		fprintf(stderr, "%s: no other option may be used with -i\n",
			pname);
		exit(1);
	    }
	    iflag = 1;
	    break;
	  case 'I':	/* calculate inverse FFT from `fft -p' format input */
	    if (argc > 2) {
		fprintf(stderr, "%s: no other option may be used with -I\n",
			pname);
		exit(1);
	    }
	    iflag = -1;
	    break;
	  case 'l':	/* perform up to n-point transforms */
	    if (++i >= argc) {
		fprintf(stderr, "%s: transform size must follow -l\n", pname);
		exit(1);
	    }
	    len = atoi(argv[i]);
	    break;
	  case 'n':	/* process in overlapping n-point chunks, output avg */
	    if (++i >= argc) {
		fprintf(stderr, "%s: chunk size must follow -n\n", pname);
		exit(1);
	    }
	    nflag = atoi(argv[i]);
	    break;
	  case 'N':	/* process in overlapping n-point chunks, output raw */
	    if (++i >= argc) {
		fprintf(stderr, "%s: chunk size must follow -N\n", pname);
		exit(1);
	    }
	    Nflag = atoi(argv[i]);
	    break;
	  case 'p':	/* print phases (polar form) */
	    pflag = 1;
	    break;
	  case 'P':	/* print power spectrum (squared magnitudes) */
	    Pflag = 1;
	    break;
	  case 's':	/* smooth spectrum */
	    if (++i >= argc || ((smooth=atoi(argv[i])) < 2) || smooth > 1024) {
		fprintf(stderr, "%s: smoothing parameter must follow -s\n",
			pname);
		exit(1);
	    }
	    break;
	  case 'S':	/* smooth and decimate spectrum */
	    if (++i >= argc || ((smooth=atoi(argv[i])) < 2) || smooth > 1024) {
		fprintf(stderr, "%s: smoothing parameter must follow -s\n",
			pname);
		exit(1);
	    }
	    decimation = smooth;
	    break;
	  case 'w':	/* apply windowing function to input */
	    if (++i >= argc) {
		fprintf(stderr, "%s: window type must follow -w\n",
			pname);
		exit(1);
	    }
	    if (strcasecmp(argv[i], "Bartlett") == 0)
		window = win_bartlett;
	    else if (strcasecmp(argv[i], "Blackman") == 0)
		window = win_blackman;
	    else if (strcasecmp(argv[i], "Blackman-Harris") == 0)
		window = win_blackman_harris;
	    else if (strcasecmp(argv[i], "Hamming") == 0)
		window = win_hamming;
	    /* Numerical Recipes 2nd ed. calls Hanning window "Hann window" */
	    else if (strcasecmp(argv[i], "Hann") == 0)
		window = win_hanning;
	    else if (strcasecmp(argv[i], "Hanning") == 0)
		window = win_hanning;
	    else if (strcasecmp(argv[i], "Parzen") == 0)
		window = win_parzen;
	    else if (strcasecmp(argv[i], "Square") == 0 ||
		     strcasecmp(argv[i], "Rectangular") == 0 ||
		     strcasecmp(argv[i], "Dirichlet") == 0)
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
	  case 'z':	/* zero-mean the input */
	    zflag = 1;
	    break;
	  case 'Z':	/* zero-mean and detrend the input */
	    zflag = 2;
	    break;
	  default:
	    fprintf(stderr, "%s: unrecognized option %s ignored\n",
		    pname, argv[i]);
	    break;
	}
    }
    if (cflag) {
	if (fflag) {
	    fprintf(stderr, "%s: -c and -f are incompatible\n", pname);
	    exit(1);
	}
	if (pflag) {
	    fprintf(stderr, "%s: -c and -p are incompatible\n", pname);
	    exit(1);
	}
	if (Pflag) {
	    fprintf(stderr, "%s: -c and -P are incompatible\n", pname);
	    exit(1);
	}
    }
    if (nflag & pflag & Pflag) {
	fprintf(stderr, "%s: -n, -p, and -P are incompatible\n", pname);
	exit(1);
    }
    if (Nflag) {
	if (nflag) {
	    fprintf(stderr, "%s: -n and -N are incompatible\n", pname);
	    exit(1);
	}
	else
	    nflag = Nflag;
    }
    if (smooth > 1) {
        if (cflag) {
	    fprintf(stderr, "%s: -c and -s or -S are incompatible\n", pname);
	    exit(1);
	}
        if (pflag) {
	    fprintf(stderr, "%s: -p and -s or -S are incompatible\n", pname);
	    exit(1);
	}
    }

    /* Make sure that len is a power of two. */
    if (len < 1) len = 1;
    if (len < LEN) {
	for (m = LEN; m >= len; m >>= 1)
	    ;
	m <<= 1;
    }
    else {
	for (m = LEN; m < len; m <<= 1)
	    ;
    }
    len = m;

    if ((c = (float *)calloc(len, sizeof(float))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
	
    if (iflag) {		/* calculate and print inverse FFT */
	for (n = 0, rsum = 0.; n < len && fscanf(ifile, "%f", &c[n]) == 1; n++)
	    ;
	if (n == 0) {
	    fprintf(stderr, "%s: standard input is empty\n", pname);
	    exit(2);
	}
	ifft();
	exit(0);
    }

    else {			/* calculate and print forward FFT */
	if (nflag) {		/* process input in chunks */
	    float *s, *t;
	    int nf2 = nflag/2;

	    for (m = len; m >= nflag; m >>= 1)
		;
	    m <<= 1;		/* m is now the smallest power of 2 >= nflag */
	    if ((s = (float *)calloc(sizeof(float), m)) == NULL ||
		(t = (float *)calloc(sizeof(float), m/2)) == NULL) {
		fprintf(stderr, "%s: insufficient memory\n", pname);
		exit(2);
	    }
	    for (n = 0; n < nf2 && fscanf(ifile, "%f", &t[n]) == 1; n++)
		;
	    while (1) {
		if (zflag) {
		    for (n = 0, rsum = 0.; n < nf2; n++)
			rsum += c[n] = t[n];
		    for (i=0; n<nflag && fscanf(ifile,"%f",&t[i])==1; i++, n++)
			rsum += c[n] = t[i]; /* read input, accumulate sum */
		}
		else {
		    for (n = 0, rsum = 0.; n < nf2; n++)
			c[n] = t[n];
		    for (i=0; n<nflag && fscanf(ifile,"%f",&t[i])==1; i++, n++)
			c[n] = t[i];
		    ;
		}
		if (n < nflag) break;
		for (i = n; i < m; i++)    /* re-zero the padding, if any */
		    c[i] = 0;
		fft();
		if (Nflag)
		    fft_out();
		else if (Pflag)
		    for (i = 0; i < m; i++)
			s[i] += c[i]*c[i];
		else
		    for (i = 0; i < m; i++)
			s[i] += c[i];
		nchunks++;
	    }
	    if (nchunks < 1) {
		fprintf(stderr, "%s: input series is too short\n", pname);
		exit(2);
	    }
	    if (Nflag == 0 && Pflag)
		for (i = 0; i < m; i++)
		    c[i] = sqrt(s[i])/nchunks;
	    else
		for (i = 0; i < m; i++)
		    c[i] = s[i]/nchunks;
	}

	else {
	    read_input();
	    if (n == 0) {
		fprintf(stderr, "%s: standard input is empty\n", pname);
		exit(2);
	    }
	    fft();
	}

	if (!Nflag)
	    fft_out();

	exit(0);
    }
}

/* This function detrends (subtracts a least-squares fitted line from) a
   a sequence of n uniformily spaced ordinates supplied in c. */
detrend(c, n)
float *c;
int n;
{
    int i;
    double a, b = 0.0, tsqsum = 0.0, ysum = 0.0, t;

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
}

read_input()
{
    if (zflag)
	for (n = 0, rsum = 0.; n < len && fscanf(ifile, "%f", &c[n]) == 1; n++)
	    rsum += c[n];	/* read input, accumulate sum */
    else
	for (n = 0, rsum = 0.; n < len && fscanf(ifile, "%f", &c[n]) == 1; n++)
	    ;
}

fft()		/* calculate forward FFT */
{
    int i;

    if (zflag) {		/* zero-mean the input array */
	rmean = rsum/n;
	for (i = 0; i < n; i++)
	    c[i] -= rmean;	
	if (zflag == 2)
	    detrend(c, n);
    }
    for (m = len; m >= n; m >>= 1)
	;
    m <<= 1;		/* m is now the smallest power of 2 >= n; this is the
			   length of the input series (including padding) */
    if (wflag)			/* apply the chosen windowing function */
	for (i = 0; i < m; i++)
	    c[i] *= (*window)(i, m);
    else
	wsum = m;
    norm = sqrt(2.0/(wsum*n));
    if (fflag) fstep = freq/(2.*m); /* note that fstep is actually half of
				       the frequency interval;  it is
				       multiplied by the doubled index i
				       to obtain the center frequency for
				       bin (i/2) */
    realft(c-1, m/2, 1);	/* perform the FFT;  see Numerical Recipes */
}

fft_out()	/* print the FFT */
{
    int i;

    c[m] = c[1];		/* unpack the output array */
    c[1] = c[m+1] = 0.;
    for (i = 0; i <= m; i += 2*decimation) {
	int j;
	double pow;

	if (fflag) printf("%g\t", i*fstep);
	if (cflag) printf("%g\t%g\n", c[i], c[i+1]);
	else {
	    for (j = 0, pow = 0.0; j < 2*smooth; j += 2)
	        pow += (c[i+j]*c[i+j] + c[i+j+1]*c[i+j+1])*norm*norm;
	    pow /= smooth/decimation;
	    if (Pflag) printf("%g", pow);
	    else printf("%g", sqrt(pow));
	    if (pflag) printf("\t%g", atan2(c[i+1], c[i]));
	    printf("\n");
	}
    }
}

ifft()		/* calculate and print inverse FFT */
{
    int i;

    n -= 2;
    c[1] = c[n];		/* repack IFFT input array */
    if (iflag < 0) {		/* convert polar form input to rectangular */
	for (i = 2; i < n; i += 2) {
	    float im;

	    im = c[i]*sin(c[i+1]);
	    c[i] *= cos(c[i+1]);
	    c[i+1] = im;
	}
    }
    realft(c-1, n/2, -1);
    if (iflag < 0) {
	norm = sqrt(2.0);
	for (i = 0; i < n; i++)
	    printf("%g\n", c[i]*norm);
    }
    else
	for (i = 0; i < n; i++)
	    printf("%g\n", c[i]/(n/2.0));
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
 "usage: %s [ OPTIONS ...] INPUT-FILE\n",
 " where INPUT-FILE is the name of a text file containing a time series",
 " (use `-' to read the standard input), and OPTIONS may be any of:",
 " -c       Output unnormalized complex FFT (real components in first column,",
 "          imaginary components in second column).",
 " -f FREQ  Show the center frequency for each bin in the first column.  The",
 "          FREQ argument specifies the input sampling frequency;  the center",
 "          frequencies are given in the same units.",
 " -h       Print on-line help.",
 " -i       Perform inverse FFT;  in this case, the standard input should be",
 "          in the form generated by `fft -c', and the standard output is",
 "          a series of samples.",
 " -I       Perform inverse FFT as above, but using input generated by `fft -p'.",
 " -l LEN   Perform up to LEN-point transforms.  `fft' rounds n up to the next",
 "          higher power of two unless LEN is already a power of two.  If the",
 "          input series contains fewer than LEN samples, it is padded with",
 "          zeros up to the next higher power of two.  Any additional input",
 "          samples beyond the first LEN are not read.  Default: LEN = 16384.",
 " -n NN     Process the input in overlapping chunks of N samples and output",
 "          an averaged spectrum.  If used in combination with -P, the output",
 "          is the average of the individual squared magnitudes;  otherwise,",
 "          the output is derived from the averages of the real components and",
 "          of the imaginary components taken separately.  For best results,",
 "          NN should be a power of two.",
 " -N NN    Process the input in overlapping chunks of NN samples and output a",
 "          spectrum for each chunk.  For best results, NN should be a power",
 "          of two.",
 " -p       Show the phase in radians in the last column.",
 " -P       Generate a power spectrum (print squared magnitudes).",
 " -s N     Smooth the output by applying an N-point moving average to each bin.",
 " -S N     Smooth the output by summing sets of N consecutive bins.",
 " -w WINDOW",
 "          Apply the specified WINDOW to the input data.  WINDOW may be one",
 "          of: `Bartlett', `Blackman', `Blackman-Harris', `Hamming',",
 "          `Hanning', `Parzen', `Square', and `Welch'.",
 " -z       Zero-mean the input data.",
 " -Z       Detrend and zero-mean the input data.",
 NULL
};

void help()
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

void realft(data,n,isign)
float data[];
int n,isign;
{
    int i, i1, i2, i3, i4, n2p3;
    float c1 = 0.5, c2, h1r, h1i, h2r, h2i;
    double wr, wi, wpr, wpi, wtemp, theta;
    void four1();

    theta = PI/(double) n;
    if (isign == 1) {
	c2 = -0.5;
	four1(data, n, 1);
    } 
    else {
	c2 = 0.5;
	theta = -theta;
    }
    wtemp = sin(0.5*theta);
    wpr = -2.0*wtemp*wtemp;
    wpi = sin(theta);
    wr = 1.0+wpr;
    wi = wpi;
    n2p3 = 2*n+3;
    for (i = 2; i <= n/2; i++) {
	i4 = 1 + (i3 = n2p3 - (i2 = 1 + ( i1 = i + i - 1)));
	h1r =  c1*(data[i1] + data[i3]);
	h1i =  c1*(data[i2] - data[i4]);
	h2r = -c2*(data[i2] + data[i4]);
	h2i =  c2*(data[i1] - data[i3]);
	data[i1] =  h1r + wr*h2r - wi*h2i;
	data[i2] =  h1i + wr*h2i + wi*h2r;
	data[i3] =  h1r - wr*h2r + wi*h2i;
	data[i4] = -h1i + wr*h2i + wi*h2r;
	wr = (wtemp = wr)*wpr - wi*wpi+wr;
	wi = wi*wpr + wtemp*wpi + wi;
    }
    if (isign == 1) {
	data[1] = (h1r = data[1]) + data[2];
	data[2] = h1r - data[2];
    } else {
	data[1] = c1*((h1r = data[1]) + data[2]);
	data[2] = c1*(h1r - data[2]);
	four1(data, n, -1);
    }
}

void four1(data, nn, isign)
float data[];
int nn, isign;
{
    int n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    float tempr, tempi;
    
    n = nn << 1;
    j = 1;
    for (i = 1; i < n; i += 2) {
	if (j > i) {
	    tempr = data[j];     data[j] = data[i];     data[i] = tempr;
	    tempr = data[j+1]; data[j+1] = data[i+1]; data[i+1] = tempr;
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
	istep = 2*mmax;
	theta = TWOPI/(isign*mmax);
	wtemp = sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi = sin(theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 1; m < mmax; m += 2) {
	    for (i = m; i <= n; i += istep) {
		j =i + mmax;
		tempr = wr*data[j]   - wi*data[j+1];
		tempi = wr*data[j+1] + wi*data[j];
		data[j]   = data[i]   - tempr;
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
