/*****************************************************************************
FILE:  bdac.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	5/13/2002
  ___________________________________________________________________________

bdac.cpp: Beat Detection And Classification
Copywrite (C) 2001 Patrick S. Hamilton

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

bdac.cpp contains functions for handling Beat Detection And Classification.
The primary function calls a qrs detector.  When a beat is detected it waits
until a sufficient number of samples from the beat have occurred.  When the
beat is ready, BeatDetectAndClassify passes the beat and the timing
information on to the functions that actually classify the beat.

Functions in bdac.cpp require functions in the following files:
		qrsfilt.cpp
		qrsdet.cpp
		classify.cpp
		rythmchk.cpp
		noisechk.cpp
		analbeat.cpp
		match.cpp
		postclas.cpp

 __________________________________________________________________________

	Revisions:
		5/13/02:
			Encapsulated down sampling from input stream to beat template in
			the function DownSampleBeat.

			Constants related to time are derived from SAMPLE_RATE in qrsdet
         and BEAT_SAMPLE_RATE in bcac.h.

*******************************************************************************/
#include "qrsdet.h"	// For base SAMPLE_RATE
#include "bdac.h"

#define ECG_BUFFER_LENGTH	1000	// Should be long enough for a beat
											// plus extra space to accommodate
											// the maximum detection delay.
#define BEAT_QUE_LENGTH	10			// Length of que for beats awaiting
											// classification.  Because of
											// detection delays, Multiple beats
											// can occur before there is enough data
											// to classify the first beat in the que.

// Internal function prototypes.

void DownSampleBeat(int *beatOut, int *beatIn) ;

// External function prototypes.

int QRSDet( int datum, int init ) ;
int NoiseCheck(int datum, int delay, int RR, int beatBegin, int beatEnd) ;
int Classify(int *newBeat,int rr, int noiseLevel, int *beatMatch, int *fidAdj, int init) ;
int GetDominantType(void) ;
int GetBeatEnd(int type) ;
int GetBeatBegin(int type) ;
int gcd(int x, int y) ;

// Global Variables

int ECGBuffer[ECG_BUFFER_LENGTH], ECGBufferIndex = 0 ;  // Circular data buffer.
int BeatBuffer[BEATLGTH] ;
int BeatQue[BEAT_QUE_LENGTH], BeatQueCount = 0 ;  // Buffer of detection delays.
int RRCount = 0 ;
int InitBeatFlag = 1 ;

/******************************************************************************
	ResetBDAC() resets static variables required for beat detection and
	classification.
*******************************************************************************/

void ResetBDAC(void)
	{
	int dummy ;
	QRSDet(0,1) ;	// Reset the qrs detector
	RRCount = 0 ;
	Classify(BeatBuffer,0,0,&dummy,&dummy,1) ;
	InitBeatFlag = 1 ;
   BeatQueCount = 0 ;	// Flush the beat que.
	}

/*****************************************************************************
Syntax:
	int BeatDetectAndClassify(int ecgSample, int *beatType, *beatMatch) ;
Description:
	BeatDetectAndClassify() implements a beat detector and classifier.
	ECG samples are passed into BeatDetectAndClassify() one sample at a
	time.  BeatDetectAndClassify has been designed for a sample rate of
	200 Hz.  When a beat has been detected and classified the detection
	delay is returned and the beat classification is returned through the
	pointer *beatType.  For use in debugging, the number of the template
   that the beat was matched to is returned in via *beatMatch.
Returns
	BeatDetectAndClassify() returns 0 if no new beat has been detected and
	classified.  If a beat has been classified, BeatDetectAndClassify returns
	the number of samples since the approximate location of the R-wave.
****************************************************************************/
int BeatDetectAndClassify(int ecgSample, int *beatType, int *beatMatch)
	{
	int detectDelay, rr, i, j ;
	int noiseEst = 0, beatBegin, beatEnd ;
	int domType ;
	int fidAdj ;
	int tempBeat[(SAMPLE_RATE/BEAT_SAMPLE_RATE)*BEATLGTH] ;

	// Store new sample in the circular buffer.

	ECGBuffer[ECGBufferIndex] = ecgSample ;
	if(++ECGBufferIndex == ECG_BUFFER_LENGTH)
		ECGBufferIndex = 0 ;

	// Increment RRInterval count.

	++RRCount ;

	// Increment detection delays for any beats in the que.

	for(i = 0; i < BeatQueCount; ++i)
		++BeatQue[i] ;

	// Run the sample through the QRS detector.

	detectDelay = QRSDet(ecgSample,0) ;
	if(detectDelay != 0)
		{
		BeatQue[BeatQueCount] = detectDelay ;
		++BeatQueCount ;
		}

	// Return if no beat is ready for classification.

	if((BeatQue[0] < (BEATLGTH-FIDMARK)*(SAMPLE_RATE/BEAT_SAMPLE_RATE))
		|| (BeatQueCount == 0))
		{
		NoiseCheck(ecgSample,0,rr, beatBegin, beatEnd) ;	// Update noise check buffer
		return 0 ;
		}

	// Otherwise classify the beat at the head of the que.

	rr = RRCount - BeatQue[0] ;	// Calculate the R-to-R interval
	detectDelay = RRCount = BeatQue[0] ;

	// Estimate low frequency noise in the beat.
	// Might want to move this into classify().

	domType = GetDominantType() ;
	if(domType == -1)
		{
		beatBegin = MS250 ;
		beatEnd = MS300 ;
		}
	else
		{
		beatBegin = (SAMPLE_RATE/BEAT_SAMPLE_RATE)*(FIDMARK-GetBeatBegin(domType)) ;
		beatEnd = (SAMPLE_RATE/BEAT_SAMPLE_RATE)*(GetBeatEnd(domType)-FIDMARK) ;
		}
	noiseEst = NoiseCheck(ecgSample,detectDelay,rr,beatBegin,beatEnd) ;

	// Copy the beat from the circular buffer to the beat buffer
	// and reduce the sample rate by averageing pairs of data
	// points.

	j = ECGBufferIndex - detectDelay - (SAMPLE_RATE/BEAT_SAMPLE_RATE)*FIDMARK ;
	if(j < 0) j += ECG_BUFFER_LENGTH ;

	for(i = 0; i < (SAMPLE_RATE/BEAT_SAMPLE_RATE)*BEATLGTH; ++i)
		{
		tempBeat[i] = ECGBuffer[j] ;
		if(++j == ECG_BUFFER_LENGTH)
			j = 0 ;
		}

	DownSampleBeat(BeatBuffer,tempBeat) ;

	// Update the QUE.

	for(i = 0; i < BeatQueCount-1; ++i)
		BeatQue[i] = BeatQue[i+1] ;
	--BeatQueCount ;


	// Skip the first beat.

	if(InitBeatFlag)
		{
		InitBeatFlag = 0 ;
		*beatType = 13 ;
		*beatMatch = 0 ;
		fidAdj = 0 ;
		}
	// Classify all other beats.
	else
		{
		*beatType = Classify(BeatBuffer,rr,noiseEst,beatMatch,&fidAdj,0) ;
		fidAdj *= SAMPLE_RATE/BEAT_SAMPLE_RATE ;
      }

	// Ignore detection if the classifier decides that this
	// was the trailing edge of a PVC.

	if(*beatType == 100)
		{
		RRCount += rr ;
		return(0) ;
		}

	// Limit the fiducial mark adjustment in case of problems with
	// beat onset and offset estimation.

	if(fidAdj > MS80)
		fidAdj = MS80 ;
	else if(fidAdj < -MS80)
		fidAdj = -MS80 ;

	return(detectDelay-fidAdj) ;
	}

void DownSampleBeat(int *beatOut, int *beatIn)
	{
	int i ;

	for(i = 0; i < BEATLGTH; ++i)
		beatOut[i] = (beatIn[i<<1]+beatIn[(i<<1)+1])>>1 ;
	}
