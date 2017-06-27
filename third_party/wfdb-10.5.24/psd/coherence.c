/* file: coherence.c		G. Moody	22 December 1993
				Last revised:   17 November 2002

-------------------------------------------------------------------------------
coherence: Coherence and cross-spectral power estimation
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

This program is based on a Fortran program by C.R. Arnold, G.C. Carter, and
J.F. Ferrie, as described in `A coherence and cross-spectral estimation
program', by G.C. Carter and J.F. Ferrie, in Programs for Digital Signal
Processing, edited by the Digital Signal Processing Committee of the IEEE
ASSP Society (New York: IEEE Press, 1979).  The functions fft842() and its
auxiliary functions r2tx(), r4tx(), and r8tx(), are based on Fortran
subroutines by G.D. Bergland and M.T. Dolan, as described by them in `Fast
Fourier transform algorithms', also included in Programs for Digital Signal
Processing.

The input to this program is a text file containing two columns of numbers,
which are taken to be the time series, with a pair of contemporaneous samples
on each line.  The output contains five columns of numbers (optionally preceded
by column headings); from left to right, these columns are frequency (in Hz),
coherence, cross-spectral power (in dB), auto-spectral power (in dB) for the
first time series, and auto-spectral power (in dB) for the second time series.

In outline, the computations are performed as follows:

1. A segment of the input file is read.  By default, each segment contains
   1024 samples from each time series;  the segment length may be set using
   the -n option.
2. The DC components and linear trends are removed from the data contained in
   the segment, and Hanning (cosine) windows are applied to these data.
3. Using fft842(), the auto- and cross-spectra for the segment are determined.
4. Running totals of the auto- and cross-spectra for each segment are updated.
5. The input file pointer is repositioned so that the next samples to be read
   are those at the middle of the current segment.  (This is the reason why
   the input cannot be obtained from a pipe, since pipes do not permit backward
   seeks.)
6. Steps 1-5 are repeated until the entire input file has been read.
7. The accumulated auto- and cross-spectra are normalized.  This step may
   optionally include scaling to account for differences in the units or
   variances of the input time series;  unless the -x option is used, no
   such scaling is applied.
8. The magnitude squared coherence is calculated as the quotient of the
   normalized, accumulated, squared cross-spectrum and the product of the
   normalized, accumulated autospectra.

Usage:
  coherence -i FILENAME [ OPTIONS ]
where FILENAME is the name of the input file, and OPTIONS may include:
  -f FREQUENCY	specify sampling frequency in Hz (default: 250)
  -n SIZE	specify number of samples per segment (default: 1024)
  -v		print column headings
  -x SX SY	specify scale factors for the two time series (defaults: 1)
*/

#include <stdio.h>
#include <math.h>
#if defined(__STDC__) || defined(_WINDOWS)
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
# include <malloc.h>
# else
extern char *calloc();
# endif
#endif

/* Function types. */
int load();
void coherence(), lremv(), fft842();

double *xx, *yy, *gxx, *gyy, *gxyre, *gxyim, *phi, *weight;
double sampfreq = 250.0;
FILE *ifile = NULL;
int npfft;	/* points per Fourier transform segment (a power of 2) */
int vflag;	/* print column headings if non-zero */

main(argc, argv)
int argc;
char *argv[];
{
    int i, pps = 1024;
    double sfx = 1.0, sfy = 1.0, atof();

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'f':	/* sampling frequency (Hz) follows */
	    if (++i >= argc || (sampfreq = atof(argv[i])) <= 0.0) {
		(void)fprintf(stderr,
			      "%s: sampling frequency (Hz) must follow -f\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  case 'i':	/* input file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: input file name must follow -i\n",
			      argv[0]);
		exit(1);
	    }
	    if (strcmp(argv[i], "-") == 0)
		ifile = stdin;
	    else if ((ifile = fopen(argv[i], "r")) == NULL) {
		(void)fprintf(stderr, "%s: can't open input file %s\n",
			      argv[0], argv[i]);
		exit(1);
	    }
	    break;
	  case 'n':	/* number of points per segment follows */
	    if (++i >= argc || (pps = atoi(argv[i])) < 2 || pps > 32768) {
		(void)fprintf(stderr,
     "%s: number of points per segment (between 2 and 32768) must follow -n\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  case 'v':	/* print column headings */
	    vflag = 1;
	    break;
	  case 'x':	/* scale factors follow */
	    if (++i >= argc || (sfx = atof(argv[i])) == 0.0 ||
		++i >= argc || (sfy = atof(argv[i])) == 0.0) {
		(void)fprintf(stderr, "%s: scale factors must follow -x\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n",
			  argv[0], argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n",
			  argv[0],argv[i]);
	    exit(1);
	}
    }
    if (ifile == NULL) {
	(void)fprintf(stderr, "usage: %s -i FILENAME [ OPTIONS ]\n", argv[0]);
	(void)fprintf(stderr,
" where FILENAME is the name of the file containing the samples of the two\n");
	(void)fprintf(stderr,
" time series arranged in two columns (use `-' for standard input, which\n");
	(void)fprintf(stderr,
" may not come from a pipe), and\n");
	(void)fprintf(stderr, " OPTIONS may include:\n");
	(void)fprintf(stderr,
"  -f FREQ    specify sampling frequency in Hz (default: 250)\n");
	(void)fprintf(stderr,
"  -n SIZE    specify number of samples per segment (default: 1024)\n");
	(void)fprintf(stderr,
"  -v         print column headings\n");
	(void)fprintf(stderr,
"  -x SX SY   specify scale factors for the two time series (defaults: 1)\n");
	(void)fprintf(stderr,
" The standard output contains five columns: frequency (Hz), coherence,\n");
	(void)fprintf(stderr,
" and power cross- and auto-spectral densities (dB).\n");
	exit(1);
    }
    
    /* Number of FFT inputs (a power of 2 no less than nnn). */
    for (npfft = 2; npfft < pps; npfft <<= 1)
	;

    if ((xx = (double *)calloc(npfft, sizeof(double))) == NULL ||
	(yy = (double *)calloc(npfft, sizeof(double))) == NULL ||
	(gxx = (double *)calloc(npfft/2 + 1, sizeof(double))) == NULL ||
	(gyy = (double *)calloc(npfft/2 + 1, sizeof(double))) == NULL ||
	(gxyre = (double *)calloc(npfft/2 + 1, sizeof(double))) == NULL ||
	(gxyim = (double *)calloc(npfft/2 + 1, sizeof(double))) == NULL ||
	(phi = (double *)calloc(npfft/2 + 1, sizeof(double))) == NULL ||
	(weight = (double *)calloc(npfft, sizeof(double))) == NULL) {
	(void)fprintf(stderr,
		      "%s: insufficient memory (try again using -n %d)\n",
		      argv[0], npfft/2);
	exit(1);
    }
    coherence(pps, sfx, sfy);
    exit(0);
}

void coherence(nnn, sfx, sfy)
int nnn;	/* number of points per segment */
double sfx, sfy;/* scale factors for input data */
{
    double df, dt, sf, temp1, temp2, temp3, temp4;
    int i, nloaded, nd2, nffts;

    /* Number of FFT outputs (half of the number of inputs). */
    nd2 = npfft/2;

    /* Compute Hanning window. */
    for (i = 0; i < nnn; i++)
	weight[i] = 0.5*(1 - cos(2.0*M_PI*i/(nnn - 1)));

    /* Read a pair of segments, and compute and sum their spectra. */
    for (nffts = 0; (nloaded = load(xx, yy, nnn)) > 0; nffts++) {
	/* Detrend and zero-mean xx[] and yy[]. */
	lremv(xx, nloaded);
	lremv(yy, nloaded);

	/* Apply Hanning window. */
	for (i = 0; i < nloaded; i++) {
	    xx[i] *= weight[i];
	    yy[i] *= weight[i];
	}

	/* Compute forward FFT. */
	fft842(0, npfft, xx, yy);

	/* Compute auto- and cross-spectra. */
	gxx[0] += 4.0 * xx[0] * xx[0];
	gyy[0] += 4.0 * yy[0] * yy[0];
	gxyre[0] += 2.0 * xx[0] * yy[0];
	gxyim[0] = 0.0;
	for (i = 1; i < nd2; i++) {
	    double xi = xx[i], xj = xx[npfft-i],
	           yi = yy[i], yj = yy[npfft-i];

	    gxx[i] += (xi+xj)*(xi+xj) + (yi-yj)*(yi-yj);
	    gyy[i] += (yi+yj)*(yi+yj) + (xi-xj)*(xi-xj);
	    gxyre[i] += xi*yj + xj*yi;
	    gxyim[i] += xj*xj + yj*yj - xi*xi - yi*yi;
	}
    }
    if (nffts == 0) return;

    /* Sample interval (seconds). */
    dt = 1.0/sampfreq;

    /* Frequency interval (Hz). */
    df = 1.0/(dt*npfft);

    /* Normalize estimates. */
    temp1 = sfx * dt / (4.0 * nnn * nffts);
    temp2 = sfy * dt / (4.0 * nnn * nffts);
    sf = sqrt(fabs(sfx*sfy));
    temp3 = sf  * dt / (2.0 * nnn * nffts);
    temp4 = sf  * dt / (4.0 * nnn * nffts);
    if (vflag)
	(void)printf(
		"Freq (Hz)  Coherence    gxy (dB)    gxx (dB)    gyy (dB)\n");
    for (i = 0; i < nd2; i++) {
	gxx[i] *= temp1;
	gyy[i] *= temp2;
	gxyre[i] *= temp3;
	gxyim[i] *= temp4;
	/* Compute and print magnitude squared coherence (dimensionless), and
	   cross- and auto-spectra (in dB). */
	phi[i] = gxyre[i]*gxyre[i] + gxyim[i]*gxyim[i];
	if (gxx[i] == 0.0 || gyy[i] == 0.0) xx[i] = 1.0;
	else xx[i] = phi[i] / (gxx[i]*gyy[i]);
	(void)printf("%9.4lf  %9.4lf  %10.4lf  %10.4lf  %10.4lf\n",
		     df*i, xx[i],
		     (phi[i] > 1.0e-10 ? 5.0*log10(phi[i]) : -50.0),
		     (gxx[i] > 1.0e-10 ? 10.0*log10(gxx[i]) : -100.0),
		     (gyy[i] > 1.0e-10 ? 10.0*log10(gyy[i]) : -100.0));
    }
}

/* This function loads the data arrays. */
int load(xx, yy, nnn)
double *xx, *yy;/* arrays to be filled */
int nnn;	/* number of values to be loaded into each array (<= npfft) */
{
    int i, nloaded, nd2 = nnn/2;
    static long pos;

    (void)fseek(ifile, pos, 0);
    for (i = 0; i < nnn; i++) {
	if (i == nd2) pos = ftell(ifile);
	if (fscanf(ifile, "%lf%lf", xx+i, yy+i) != 2) break;
    }
    nloaded = i;
    if (i < npfft) {
	if (i < nd2) pos = ftell(ifile);
	for ( ; i < npfft; i++)
	    *(xx+i) = *(yy+i) = 0.0;
    }
    return (nloaded);
}

/* This function computes and removes the DC component and the slope of an
   array. */
void lremv(xx, nnn)
double *xx;	/* input data array */
int nnn;	/* number of values in data array */
{
    int i;
    double fln;
    double dc;		/* DC component of data */
    double slope;	/* slope of data */

    dc = slope = 0.0;
    for (i = 0; i < nnn; i++) {
	dc += xx[i];
	slope += xx[i]*(i+1);
    }
    dc /= (double)nnn;
    slope *= 12.0/(nnn*(nnn*(double)nnn-1.0));
    slope -= 6.0*dc/(nnn-1.0);
    fln = dc - 0.5*(nnn+1.0)*slope;
    for (i = 0; i < nnn; i++)
	xx[i] -= (i+1)*slope + fln;
}

void r2tx(nthpo, cr0, cr1, ci0, ci1)
int nthpo;
double *cr0, *cr1, *ci0, *ci1;
{
    int i;
    double temp;

    for (i = 0; i < nthpo; i += 2) {
	temp = cr0[i] + cr1[i];	cr1[i] = cr0[i] - cr1[i]; cr0[i] = temp;
	temp = ci0[i] + ci1[i];	ci1[i] = ci0[i] - ci1[i]; ci0[i] = temp;
    }
}

void r4tx(nthpo, cr0, cr1, cr2, cr3, ci0, ci1, ci2, ci3)
int nthpo;
double *cr0, *cr1, *cr2, *cr3, *ci0, *ci1, *ci2, *ci3;
{
    int i;
    double i1, i2, i3, i4, r1, r2, r3, r4;

    for (i = 0; i < nthpo; i += 4) {
	r1 = cr0[i] + cr2[i];
	r2 = cr0[i] - cr2[i];
	r3 = cr1[i] + cr3[i];
	r4 = cr1[i] - cr3[i];
	i1 = ci0[i] + ci2[i];
	i2 = ci0[i] - ci2[i];
        i3 = ci1[i] + ci3[i];
	i4 = ci1[i] - ci3[i];
	cr0[i] = r1 + r3;
	ci0[i] = i1 + i3;
	cr1[i] = r1 - r3;
	ci1[i] = i1 - i3;
	cr2[i] = r2 - i4;
	ci2[i] = i2 + r4;
	cr3[i] = r2 + i4;
	ci3[i] = i2 - r4;
    }
}

void r8tx(nx, nthpo, length, cr0, cr1, cr2, cr3, cr4, cr5, cr6, cr7, ci0, ci1,
	  ci2, ci3, ci4, ci5, ci6, ci7)
int nx, nthpo, length;
double *cr0, *cr1, *cr2, *cr3, *cr4, *cr5, *cr6, *cr7;
double *ci0, *ci1, *ci2, *ci3, *ci4, *ci5, *ci6, *ci7;
{
    double scale = 2.0*M_PI/length, arg, tr, ti;
    double c1, c2, c3, c4, c5, c6, c7;
    double s1, s2, s3, s4, s5, s6, s7;
    double ar0, ar1, ar2, ar3, ar4, ar5, ar6, ar7;
    double ai0, ai1, ai2, ai3, ai4, ai5, ai6, ai7;
    double br0, br1, br2, br3, br4, br5, br6, br7;
    double bi0, bi1, bi2, bi3, bi4, bi5, bi6, bi7;
    int j, k;

    for (j = 0; j < nx; j++) {
	arg = j*scale;
	c1 = cos(arg);
	s1 = sin(arg);
	c2 = c1*c1 - s1*s1;
	s2 = 2.0*c1*s1;
	c3 = c1*c2 - s1*s2;
	s3 = c2*s1 + s2*c1;
	c4 = c2*c2 - s2*s2;
	s4 = 2.0*c2*s2;
	c5 = c2*c3 - s2*s3;
	s5 = c3*s2 + s3*c2;
	c6 = c3*c3 - s3*s3;
	s6 = 2.0*c3*s3;
	c7 = c3*c4 - s3*s4;
	s7 = c4*s3 + s4*c3;
	for (k = j; k < nthpo; k += length) {
	    ar0 = cr0[k] + cr4[k];	ar4 = cr0[k] - cr4[k];
	    ar1 = cr1[k] + cr5[k];	ar5 = cr1[k] - cr5[k];
	    ar2 = cr2[k] + cr6[k];	ar6 = cr2[k] - cr6[k];
	    ar3 = cr3[k] + cr7[k];	ar7 = cr3[k] - cr7[k];

	    ai0 = ci0[k] + ci4[k];	ai4 = ci0[k] - ci4[k];
	    ai1 = ci1[k] + ci5[k];	ai5 = ci1[k] - ci5[k];
	    ai2 = ci2[k] + ci6[k];	ai6 = ci2[k] - ci6[k];
	    ai3 = ci3[k] + ci7[k];	ai7 = ci3[k] - ci7[k];

	    br0 = ar0 + ar2;		br2 = ar0 - ar2;
	    br1 = ar1 + ar3;		br3 = ar1 - ar3;

	    br4 = ar4 - ai6;		br6 = ar4 + ai6;
	    br5 = ar5 - ai7;		br7 = ar5 + ai7;

	    bi0 = ai0 + ai2;		bi2 = ai0 - ai2;
	    bi1 = ai1 + ai3;		bi3 = ai1 - ai3;

	    bi4 = ai4 + ar6;		bi6 = ai4 - ar6;
	    bi5 = ai5 + ar7;		bi7 = ai5 - ar7;

	    cr0[k] = br0 + br1;
	    ci0[k] = bi0 + bi1;
	    if (j > 0) {
		cr1[k] = c4*(br0-br1) - s4*(bi0-bi1);
		ci1[k] = c4*(bi0-bi1) + s4*(br0-br1);
		cr2[k] = c2*(br2-bi3) - s2*(bi2+br3);
		ci2[k] = c2*(bi2+br3) + s2*(br2-bi3);
		cr3[k] = c6*(br2+bi3) - s6*(bi2-br3);
		ci3[k] = c6*(bi2-br3) + s6*(br2+bi3);
		tr = M_SQRT1_2*(br5-bi5);
		ti = M_SQRT1_2*(br5+bi5);
		cr4[k] = c1*(br4+tr) - s1*(bi4+ti);
		ci4[k] = c1*(bi4+ti) + s1*(br4+tr);
		cr5[k] = c5*(br4-tr) - s5*(bi4-ti);
		ci5[k] = c5*(bi4-ti) + s5*(br4-tr);
		tr = -M_SQRT1_2*(br7+bi7);
		ti =  M_SQRT1_2*(br7-bi7);
		cr6[k] = c3*(br6+tr) - s3*(bi6+ti);
		ci6[k] = c3*(bi6+ti) + s3*(br6+tr);
		cr7[k] = c7*(br6-tr) - s7*(bi6-ti);
		ci7[k] = c7*(bi6-ti) + s7*(br6-tr);
	    }
	    else {
		cr1[k] = br0 - br1;
		ci1[k] = bi0 - bi1;
		cr2[k] = br2 - bi3;
		ci2[k] = bi2 + br3;
		cr3[k] = br2 + bi3;
		ci3[k] = bi2 - br3;
		tr = M_SQRT1_2*(br5 - bi5);
		ti = M_SQRT1_2*(br5 + bi5);
		cr4[k] = br4 + tr;
		ci4[k] = bi4 + ti;
		cr5[k] = br4 - tr;
		ci5[k] = bi4 - ti;
		tr = -M_SQRT1_2*(br7 + bi7);
		ti =  M_SQRT1_2*(br7 - bi7);
		cr6[k] = br6 + tr;
		ci6[k] = bi6 + ti;
		cr7[k] = br6 - tr;
		ci7[k] = bi6 - ti;
	    }
	}
    }
}

void fft842(in, n, x, y)
int in;		/* 0: forward FFT; non-zero: inverse FFT */
int n;		/* number of points */
double *x, *y;	/* arrays of points */
{
    double temp;
    int i, j, ij, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14,
        ji, l[15], nt, nx, n2pow, n8pow;

    for (n2pow = nt = 1; n2pow <= 15 && n > nt; n2pow++)
	nt <<= 1;
    n2pow--;
    if (n != nt) {
	(void)fprintf(stderr, "fft842: %d is not a power of 2\n", n);
	exit(2);
    }
    n8pow = n2pow/3;
    if (in == 0) {
	for (i = 0; i < n; i++)
	    y[i] = -y[i];
    }

    /* Do radix 8 passes, if any. */
    for (i = 1; i <= n8pow; i++) {
	nx = 1 << (n2pow - 3*i);
	r8tx(nx, n, 8*nx,
	     x, x+nx, x+2*nx, x+3*nx, x+4*nx, x+5*nx, x+6*nx, x+7*nx,
	     y, y+nx, y+2*nx, y+3*nx, y+4*nx, y+5*nx, y+6*nx, y+7*nx);
    }

    /* Do final radix 2 or radix 4 pass. */
    switch (n2pow - 3*n8pow) {
      case 0:	break;
      case 1:	r2tx(n, x, x+1, y, y+1); break;
      case 2:	r4tx(n, x, x+1, x+2, x+3, y, y+1, y+2, y+3); break;
    }

    for (j = 0; j < 15; j++) {
	if (j <= n2pow) l[j] = 1 << (n2pow - j);
	else l[j] = 1;
    }	
    ij = 0;
    for (j1 = 0; j1 < l[14]; j1++)
     for (j2 = j1; j2 < l[13]; j2 += l[14])
      for (j3 = j2; j3 < l[12]; j3 += l[13])
       for (j4 = j3; j4 < l[11]; j4 += l[12])
	for (j5 = j4; j5 < l[10]; j5 += l[11])
	 for (j6 = j5; j6 < l[9]; j6 += l[10])
	  for (j7 = j6; j7 < l[8]; j7 += l[9])
	   for (j8 = j7; j8 < l[7]; j8 += l[8])
	    for (j9 = j8; j9 < l[6]; j9 += l[7])
	     for (j10 = j9; j10 < l[5]; j10 += l[6])
	      for (j11 = j10; j11 < l[4]; j11 += l[5])
	       for (j12 = j11; j12 < l[3]; j12 += l[4])
		for (j13 = j12; j13 < l[2]; j13 += l[3])
		 for (j14 = j13; j14 < l[1]; j14 += l[2])
		  for (ji = j14; ji < l[0]; ji += l[1]) {
		      if (ij < ji) {
			  temp = x[ij]; x[ij] = x[ji]; x[ji] = temp;
			  temp = y[ij]; y[ij] = y[ji]; y[ji] = temp;
		      }
		      ij++;
		  }
    if (in == 0) {
	for (i = 0; i < n; i++)
	    y[i] = -y[i];
    }
    else {
	for (i = 0; i < n; i++) {
	    x[i] /= (double)n;
	    y[i] /= (double)n;
	}
    }
}
