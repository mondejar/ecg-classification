/*****************************************************************************
FILE:  qrsdet2.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	7/08/2002
  ___________________________________________________________________________

qrsdet2.cpp: A QRS detector.
Copywrite (C) 2002 Patrick S. Hamilton

This file is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This software is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (pat@eplimited.edu) or postal mail
(Patrick Hamilton, E.P. Limited, 35 Medford St., Suite 204 Somerville,
MA 02143 USA).  For updates to this software, please visit our website
(http://www.eplimited.com).
  __________________________________________________________________________

This file contains functions for detecting QRS complexes in an ECG.  The
QRS detector requires filter functions in qrsfilt.cpp and parameter
definitions in qrsdet.h.  QRSDet is the only function that needs to be
visable outside of these files.

Syntax:
	int QRSDet(int ecgSample, int init) ;

Description:
	QRSDet() implements a modified version of the QRS detection
	algorithm described in:

	Hamilton, Tompkins, W. J., "Quantitative investigation of QRS
	detection rules using the MIT/BIH arrhythmia database",
	IEEE Trans. Biomed. Eng., BME-33, pp. 1158-1165, 1987.

	Consecutive ECG samples are passed to QRSDet.  QRSDet was
	designed for a 200 Hz sample rate.  QRSDet contains a number
	of static variables that it uses to adapt to different ECG
	signals.  These variables can be reset by passing any value
	not equal to 0 in init.

	Note: QRSDet() requires filters in QRSFilt.cpp

Returns:
	When a QRS complex is detected QRSDet returns the detection delay.

****************************************************************/

/* For memmove. */
#ifdef __STDC__
#include <string.h>
#else
#include <mem.h>
#endif

#include <math.h>
#include "qrsdet.h"

#define PRE_BLANK	MS195
#define MIN_PEAK_AMP	7 // Prevents detections of peaks smaller than 150 uV.

// External Prototypes.

int QRSFilter(int datum, int init) ;
int deriv1( int x0, int init ) ;

// Local Prototypes.

int Peak( int datum, int init ) ;
int mean(int *array, int datnum) ;
int thresh(int qmean, int nmean) ;
int BLSCheck(int *dBuf,int dbPtr,int *maxder) ;

double TH = .3125 ;

int DDBuffer[DER_DELAY], DDPtr ;	/* Buffer holding derivative data. */
int Dly  = 0 ;

const int MEMMOVELEN = 7*sizeof(int);

int QRSDet( int datum, int init )
	{
	static int det_thresh, qpkcnt = 0 ;
	static int qrsbuf[8], noise[8], rrbuf[8] ;
	static int rsetBuff[8], rsetCount = 0 ;
	static int nmean, qmean, rrmean ;
	static int count, sbpeak = 0, sbloc, sbcount = MS1500 ;
	static int maxder, lastmax ;
	static int initBlank, initMax ;
	static int preBlankCnt, tempPeak ;
	
	int fdatum, QrsDelay = 0 ;
	int i, newPeak, aPeak ;

/*	Initialize all buffers to 0 on the first call.	*/

	if( init )
		{
		for(i = 0; i < 8; ++i)
			{
			noise[i] = 0 ;	/* Initialize noise buffer */
			rrbuf[i] = MS1000 ;/* and R-to-R interval buffer. */
			}

		qpkcnt = maxder = lastmax = count = sbpeak = 0 ;
		initBlank = initMax = preBlankCnt = DDPtr = 0 ;
		sbcount = MS1500 ;
		QRSFilter(0,1) ;	/* initialize filters. */
		Peak(0,1) ;
		}

	fdatum = QRSFilter(datum,0) ;	/* Filter data. */


	/* Wait until normal detector is ready before calling early detections. */

	aPeak = Peak(fdatum,0) ;
	if(aPeak < MIN_PEAK_AMP)
		aPeak = 0 ;

	// Hold any peak that is detected for 200 ms
	// in case a bigger one comes along.  There
	// can only be one QRS complex in any 200 ms window.

	newPeak = 0 ;
	if(aPeak && !preBlankCnt)			// If there has been no peak for 200 ms
		{										// save this one and start counting.
		tempPeak = aPeak ;
		preBlankCnt = PRE_BLANK ;			// MS200
		}

	else if(!aPeak && preBlankCnt)	// If we have held onto a peak for
		{										// 200 ms pass it on for evaluation.
		if(--preBlankCnt == 0)
			newPeak = tempPeak ;
		}

	else if(aPeak)							// If we were holding a peak, but
		{										// this ones bigger, save it and
		if(aPeak > tempPeak)				// start counting to 200 ms again.
			{
			tempPeak = aPeak ;
			preBlankCnt = PRE_BLANK ; // MS200
			}
		else if(--preBlankCnt == 0)
			newPeak = tempPeak ;
		}

	/* Save derivative of raw signal for T-wave and baseline
	   shift discrimination. */
	
	DDBuffer[DDPtr] = deriv1( datum, 0 ) ;
	if(++DDPtr == DER_DELAY)
		DDPtr = 0 ;

	/* Initialize the qrs peak buffer with the first eight 	*/
	/* local maximum peaks detected.						*/

	if( qpkcnt < 8 )
		{
		++count ;
		if(newPeak > 0) count = WINDOW_WIDTH ;
		if(++initBlank == MS1000)
			{
			initBlank = 0 ;
			qrsbuf[qpkcnt] = initMax ;
			initMax = 0 ;
			++qpkcnt ;
			if(qpkcnt == 8)
				{
				qmean = mean( qrsbuf, 8 ) ;
				nmean = 0 ;
				rrmean = MS1000 ;
				sbcount = MS1500+MS150 ;
				det_thresh = thresh(qmean,nmean) ;
				}
			}
		if( newPeak > initMax )
			initMax = newPeak ;
		}

	else	/* Else test for a qrs. */
		{
		++count ;
		if(newPeak > 0)
			{
			
			
			/* Check for maximum derivative and matching minima and maxima
			   for T-wave and baseline shift rejection.  Only consider this
			   peak if it doesn't seem to be a base line shift. */
			   
			if(!BLSCheck(DDBuffer, DDPtr, &maxder))
				{


				// Classify the beat as a QRS complex
				// if the peak is larger than the detection threshold.

				if(newPeak > det_thresh)
					{
					memmove(&qrsbuf[1], qrsbuf, MEMMOVELEN) ;
					qrsbuf[0] = newPeak ;
					qmean = mean(qrsbuf,8) ;
					det_thresh = thresh(qmean,nmean) ;
					memmove(&rrbuf[1], rrbuf, MEMMOVELEN) ;
					rrbuf[0] = count - WINDOW_WIDTH ;
					rrmean = mean(rrbuf,8) ;
					sbcount = rrmean + (rrmean >> 1) + WINDOW_WIDTH ;
					count = WINDOW_WIDTH ;

					sbpeak = 0 ;

					lastmax = maxder ;
					maxder = 0 ;
					QrsDelay =  WINDOW_WIDTH + FILTER_DELAY ;
					initBlank = initMax = rsetCount = 0 ;
					}

				// If a peak isn't a QRS update noise buffer and estimate.
				// Store the peak for possible search back.


				else
					{
					memmove(&noise[1],noise,MEMMOVELEN) ;
					noise[0] = newPeak ;
					nmean = mean(noise,8) ;
					det_thresh = thresh(qmean,nmean) ;

					// Don't include early peaks (which might be T-waves)
					// in the search back process.  A T-wave can mask
					// a small following QRS.

					if((newPeak > sbpeak) && ((count-WINDOW_WIDTH) >= MS360))
						{
						sbpeak = newPeak ;
						sbloc = count  - WINDOW_WIDTH ;
						}
					}
				}
			}
		
		/* Test for search back condition.  If a QRS is found in  */
		/* search back update the QRS buffer and det_thresh.      */

		if((count > sbcount) && (sbpeak > (det_thresh >> 1)))
			{
			memmove(&qrsbuf[1],qrsbuf,MEMMOVELEN) ;
			qrsbuf[0] = sbpeak ;
			qmean = mean(qrsbuf,8) ;
			det_thresh = thresh(qmean,nmean) ;
			memmove(&rrbuf[1],rrbuf,MEMMOVELEN) ;
			rrbuf[0] = sbloc ;
			rrmean = mean(rrbuf,8) ;
			sbcount = rrmean + (rrmean >> 1) + WINDOW_WIDTH ;
			QrsDelay = count = count - sbloc ;
			QrsDelay += FILTER_DELAY ;
			sbpeak = 0 ;
			lastmax = maxder ;
			maxder = 0 ;

			initBlank = initMax = rsetCount = 0 ;
			}
		}

	// In the background estimate threshold to replace adaptive threshold
	// if eight seconds elapses without a QRS detection.

	if( qpkcnt == 8 )
		{
		if(++initBlank == MS1000)
			{
			initBlank = 0 ;
			rsetBuff[rsetCount] = initMax ;
			initMax = 0 ;
			++rsetCount ;

			// Reset threshold if it has been 8 seconds without
			// a detection.

			if(rsetCount == 8)
				{
				for(i = 0; i < 8; ++i)
					{
					qrsbuf[i] = rsetBuff[i] ;
					noise[i] = 0 ;
					}
				qmean = mean( rsetBuff, 8 ) ;
				nmean = 0 ;
				rrmean = MS1000 ;
				sbcount = MS1500+MS150 ;
				det_thresh = thresh(qmean,nmean) ;
				initBlank = initMax = rsetCount = 0 ;
				}
			}
		if( newPeak > initMax )
			initMax = newPeak ;
		}

	return(QrsDelay) ;
	}

/**************************************************************
* peak() takes a datum as input and returns a peak height
* when the signal returns to half its peak height, or 
**************************************************************/

int Peak( int datum, int init )
	{
	static int max = 0, timeSinceMax = 0, lastDatum ;
	int pk = 0 ;

	if(init)
		max = timeSinceMax = 0 ;
		
	if(timeSinceMax > 0)
		++timeSinceMax ;

	if((datum > lastDatum) && (datum > max))
		{
		max = datum ;
		if(max > 2)
			timeSinceMax = 1 ;
		}

	else if(datum < (max >> 1))
		{
		pk = max ;
		max = 0 ;
		timeSinceMax = 0 ;
		Dly = 0 ;
		}

	else if(timeSinceMax > MS95)
		{
		pk = max ;
		max = 0 ;
		timeSinceMax = 0 ;
		Dly = 3 ;
		}
	lastDatum = datum ;
	return(pk) ;
	}

/********************************************************************
mean returns the mean of an array of integers.  It uses a slow
sort algorithm, but these arrays are small, so it hardly matters.
********************************************************************/

int mean(int *array, int datnum)
	{
	long sum ;
	int i ;

	for(i = 0, sum = 0; i < datnum; ++i)
		sum += array[i] ;
	sum /= datnum ;
	return(sum) ;
	}

/****************************************************************************
 thresh() calculates the detection threshold from the qrs mean and noise
 mean estimates.
****************************************************************************/

int thresh(int qmean, int nmean)
	{
	int thrsh, dmed ;
	double temp ;
	dmed = qmean - nmean ;
/*	thrsh = nmean + (dmed>>2) + (dmed>>3) + (dmed>>4); */
	temp = dmed ;
	temp *= TH ;
	dmed = temp ;
	thrsh = nmean + dmed ; /* dmed * THRESHOLD */
	return(thrsh) ;
	}

/***********************************************************************
	BLSCheck() reviews data to see if a baseline shift has occurred.
	This is done by looking for both positive and negative slopes of
	roughly the same magnitude in a 220 ms window.
***********************************************************************/

int BLSCheck(int *dBuf,int dbPtr,int *maxder)
	{
	int max, min, maxt, mint, t, x ;
	max = min = 0 ;

	for(t = 0; t < MS220; ++t)
		{
		x = dBuf[dbPtr] ;
		if(x > max)
			{
			maxt = t ;
			max = x ;
			}
		else if(x < min)
			{
			mint = t ;
			min = x;
			}
		if(++dbPtr == DER_DELAY)
			dbPtr = 0 ;
		}

	*maxder = max ;
	min = -min ;
	
	/* Possible beat if a maximum and minimum pair are found
		where the interval between them is less than 150 ms. */
	   
	if((max > (min>>3)) && (min > (max>>3)) &&
		(abs(maxt - mint) < MS150))
		return(0) ;
		
	else
		return(1) ;
	}


