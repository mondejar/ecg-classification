/*****************************************************************************
FILE:  qrsfilt.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	5/13/2002
  ___________________________________________________________________________

qrsfilt.cpp filter functions to aid beat detecton in electrocardiograms.
Copywrite (C) 2000 Patrick S. Hamilton

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

	This file includes QRSFilt() and associated filtering files used for QRS
	detection.  Only QRSFilt() and deriv1() are called by the QRS detector
	other functions can be hidden.
	Revisions:
		5/13: Filter implementations have been modified to allow simplified
			modification for different sample rates.
*******************************************************************************/
#include <math.h>
#include "qrsdet.h"
// Local Prototypes.
int lpfilt( int datum ,int init) ;
int hpfilt( int datum, int init ) ;
int deriv1( int x0, int init ) ;
int deriv2( int x0, int init ) ;
int mvwint(int datum, int init) ;
/******************************************************************************
* Syntax:
*	int QRSFilter(int datum, int init) ;
* Description:
*	QRSFilter() takes samples of an ECG signal as input and returns a sample of
*	a signal that is an estimate of the local energy in the QRS bandwidth.  In
*	other words, the signal has a lump in it whenever a QRS complex, or QRS
*	complex like artifact occurs.  The filters were originally designed for data
*  sampled at 200 samples per second, but they work nearly as well at sample
*	frequencies from 150 to 250 samples per second.
*
*	The filter buffers and static variables are reset if a value other than
*	0 is passed to QRSFilter through init.
*******************************************************************************/
int QRSFilter(int datum,int init)
	{
	int fdatum ;
	if(init)
		{
		hpfilt( 0, 1 ) ;		// Initialize filters.
		lpfilt( 0, 1 ) ;
		mvwint( 0, 1 ) ;
		deriv1( 0, 1 ) ;
		deriv2( 0, 1 ) ;
		}
	fdatum = lpfilt( datum, 0 ) ;		// Low pass filter data.
	fdatum = hpfilt( fdatum, 0 ) ;	// High pass filter data.
	fdatum = deriv2( fdatum, 0 ) ;	// Take the derivative.
	fdatum = abs(fdatum) ;				// Take the absolute value.
	fdatum = mvwint( fdatum, 0 ) ;	// Average over an 80 ms window .
	return(fdatum) ;
	}

/*************************************************************************
*  lpfilt() implements the digital filter represented by the difference
*  equation:
*
* 	y[n] = 2*y[n-1] - y[n-2] + x[n] - 2*x[t-24 ms] + x[t-48 ms]
*
*	Note that the filter delay is (LPBUFFER_LGTH/2)-1
*
**************************************************************************/
int lpfilt( int datum ,int init)
	{
	static long y1 = 0, y2 = 0 ;
	int data[LPBUFFER_LGTH], ptr = 0 ;
	long y0 ;
	int output, halfPtr ;
	if(init)
		{
		for(ptr = 0; ptr < LPBUFFER_LGTH; ++ptr)
			data[ptr] = 0 ;
		y1 = y2 = 0 ;
		ptr = 0 ;
		}
	halfPtr = ptr-(LPBUFFER_LGTH/2) ;	// Use halfPtr to index
	if(halfPtr < 0)							// to x[n-6].
		halfPtr += LPBUFFER_LGTH ;
	y0 = (y1 << 1) - y2 + datum - (data[halfPtr] << 1) + data[ptr] ;
	y2 = y1;
	y1 = y0;
	output = y0 / ((LPBUFFER_LGTH*LPBUFFER_LGTH)/4);
	data[ptr] = datum ;			// Stick most recent sample into
	if(++ptr == LPBUFFER_LGTH)	// the circular buffer and update
		ptr = 0 ;					// the buffer pointer.
	return(output) ;
	}

/******************************************************************************
*  hpfilt() implements the high pass filter represented by the following
*  difference equation:
*
*	y[n] = y[n-1] + x[n] - x[n-128 ms]
*	z[n] = x[n-64 ms] - y[n] ;
*
*  Filter delay is (HPBUFFER_LGTH-1)/2
******************************************************************************/
int hpfilt( int datum, int init )
	{
	static long y=0 ;
	int data[HPBUFFER_LGTH], ptr = 0 ;
	int z, halfPtr ;
	if(init)
		{
		for(ptr = 0; ptr < HPBUFFER_LGTH; ++ptr)
			data[ptr] = 0 ;
		ptr = 0 ;
		y = 0 ;
		}
	y += datum - data[ptr];
	halfPtr = ptr-(HPBUFFER_LGTH/2) ;
	if(halfPtr < 0)
		halfPtr += HPBUFFER_LGTH ;
	z = data[halfPtr] - (y / HPBUFFER_LGTH);
	data[ptr] = datum ;
	if(++ptr == HPBUFFER_LGTH)
		ptr = 0 ;
	return( z );
	}
/*****************************************************************************
*  deriv1 and deriv2 implement derivative approximations represented by
*  the difference equation:
*
*	y[n] = x[n] - x[n - 10ms]
*
*  Filter delay is DERIV_LENGTH/2
*****************************************************************************/
int deriv1(int x, int init)
	{
	int derBuff[DERIV_LENGTH], derI = 0 ;
	int y ;
	if(init != 0)
		{
		for(derI = 0; derI < DERIV_LENGTH; ++derI)
			derBuff[derI] = 0 ;
		derI = 0 ;
		return(0) ;
		}
	y = x - derBuff[derI] ;
	derBuff[derI] = x ;
	if(++derI == DERIV_LENGTH)
		derI = 0 ;
	return(y) ;
	}
int deriv2(int x, int init)
	{
	int derBuff[DERIV_LENGTH], derI = 0 ;
	int y ;
	if(init != 0)
		{
		for(derI = 0; derI < DERIV_LENGTH; ++derI)
			derBuff[derI] = 0 ;
		derI = 0 ;
		return(0) ;
		}
	y = x - derBuff[derI] ;
	derBuff[derI] = x ;
	if(++derI == DERIV_LENGTH)
		derI = 0 ;
	return(y) ;
	}


/*****************************************************************************
* mvwint() implements a moving window integrator.  Actually, mvwint() averages
* the signal values over the last WINDOW_WIDTH samples.
*****************************************************************************/
int mvwint(int datum, int init)
	{
	static long sum = 0 ;
	int data[WINDOW_WIDTH], ptr = 0 ;
	int output;
	if(init)
		{
		for(ptr = 0; ptr < WINDOW_WIDTH ; ++ptr)
			data[ptr] = 0 ;
		sum = 0 ;
		ptr = 0 ;
		}
	sum += datum ;
	sum -= data[ptr] ;
	data[ptr] = datum ;
	if(++ptr == WINDOW_WIDTH)
		ptr = 0 ;
	if((sum / WINDOW_WIDTH) > 32000)
		output = 32000 ;
	else
		output = sum / WINDOW_WIDTH ;
	return(output) ;
	}
