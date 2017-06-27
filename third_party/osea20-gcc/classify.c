/*****************************************************************************

FILE:  classify.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	5/13/2001
  ___________________________________________________________________________

classify.cpp: Classify a given beat.
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

	Classify.cpp contains functions for classifying beats.  The only
	function that needs to be called externally from this file is Classify().

	Functions in classify.cpp require functions in the following files:
		match.cpp
		rythmchk.cpp
		classify.cpp
		rythmchk.cpp
		analbeat.cpp
		postclas.cpp

  __________________________________________________________________________

	Revisions:
		5/13/02:
			Width constants tied to BEAT_SAMPLE_RATE in bdac.h

			Arrays added to track the classifications and RR intervals for the
			most recent 8 beats, allowing GetRunCount to become a local function.
			RR intervals and classifications are now passed to PostClassify.

			Determination of whether the dominant rhythm is regular is now made
			by examining the number of RR intervals classified as UNKNOWN in the
			last DM_BUFFER_LENGTH beats (180).  If more than 60 are UNKNOWN
			the rhythm is too irregular to give any weight to whether the beat
			was premature or not.

*******************************************************************************/

#include <wfdb/ecgcodes.h>
#include <stdlib.h>	// For abs()
#include <stdio.h>
#include "qrsdet.h"	// For base sample rate.
#include "bdac.h"
#include "match.h"
#include "rythmchk.h"
#include "analbeat.h"
#include "postclas.h"

// Detection Rule Parameters.

#define MATCH_LIMIT 1.3					// Threshold for template matching
												// without amplitude sensitivity.
#define MATCH_WITH_AMP_LIMIT	2.5	// Threshold for matching index that
												// is amplitude sensitive.
#define PVC_MATCH_WITH_AMP_LIMIT 0.9 // Amplitude sensitive limit for
												 //matching premature beats
#define BL_SHIFT_LIMIT	100			// Threshold for assuming a baseline shift.
#define NEW_TYPE_NOISE_THRESHOLD	18	// Above this noise level, do not create
												// new beat types.
#define NEW_TYPE_HF_NOISE_LIMIT 75	// Above this noise level, do not crate
												// new beat types.

#define MATCH_NOISE_THRESHOLD	0.7	// Match threshold below which noise
												// indications are ignored.

// TempClass classification rule parameters.

#define R2_DI_THRESHOLD 1.0		// Rule 2 dominant similarity index threshold
#define R3_WIDTH_THRESHOLD	BEAT_MS90		// Rule 3 width threshold.
#define R7_DI_THRESHOLD	1.2		// Rule 7 dominant similarity index threshold
#define R8_DI_THRESHOLD 1.5		// Rule 8 dominant similarity index threshold
#define R9_DI_THRESHOLD	2.0		// Rule 9 dominant similarity index threshold
#define R10_BC_LIM	3				// Rule 10 beat count limit.
#define R10_DI_THRESHOLD	2.5	// Rule 10 dominant similarity index threshold
#define R11_MIN_WIDTH BEAT_MS110			// Rule 11 minimum width threshold.
#define R11_WIDTH_BREAK	BEAT_MS140			// Rule 11 width break.
#define R11_WIDTH_DIFF1	BEAT_MS40			// Rule 11 width difference threshold 1
#define R11_WIDTH_DIFF2	BEAT_MS60			// Rule 11 width difference threshold 2
#define R11_HF_THRESHOLD	45		// Rule 11 high frequency noise threshold.
#define R11_MA_THRESHOLD	14		// Rule 11 motion artifact threshold.
#define R11_BC_LIM	1				// Rule 11 beat count limit.
#define R15_DI_THRESHOLD	3.5	// Rule 15 dominant similarity index threshold
#define R15_WIDTH_THRESHOLD BEAT_MS100	// Rule 15 width threshold.
#define R16_WIDTH_THRESHOLD BEAT_MS100	// Rule 16 width threshold.
#define R17_WIDTH_DELTA	BEAT_MS20			// Rule 17 difference threshold.
#define R18_DI_THRESHOLD	1.5	// Rule 18 dominant similarity index threshold.
#define R19_HF_THRESHOLD	75		// Rule 19 high frequency noise threshold.

// Dominant monitor constants.

#define DM_BUFFER_LENGTH	180
#define IRREG_RR_LIMIT	60

// Local prototypes.

int HFNoiseCheck(int *beat) ;
int TempClass(int rhythmClass, int morphType, int beatWidth, int domWidth,
	int domType, int hfNoise, int noiseLevel, int blShift, double domIndex) ;
int DomMonitor(int morphType, int rhythmClass, int beatWidth, int rr, int reset) ;
int GetDomRhythm(void) ;
int GetRunCount(void) ;

// Local Global variables

int DomType ;
int RecentRRs[8], RecentTypes[8] ;

/***************************************************************************
*  Classify() takes a beat buffer, the previous rr interval, and the present
*  noise level estimate and returns a beat classification of NORMAL, PVC, or
*  UNKNOWN.  The UNKNOWN classification is only returned.  The beat template
*  type that the beat has been matched to is returned through the pointer
*  *beatMatch for debugging display.  Passing anything other than 0 in init
*  resets the static variables used by Classify.
****************************************************************************/

int Classify(int *newBeat,int rr, int noiseLevel, int *beatMatch, int *fidAdj,
	int init)
	{
	int rhythmClass, beatClass, i, beatWidth, blShift ;
	static int morphType, runCount = 0 ;
	double matchIndex, domIndex, mi2 ;
	int shiftAdj ;
	int domType, domWidth, onset, offset, amp ;
	int beatBegin, beatEnd, tempClass ;
	int hfNoise, isoLevel ;
	static int lastIsoLevel=0, lastRhythmClass = UNKNOWN, lastBeatWasNew = 0 ;

	// If initializing...

	if(init)
		{
		ResetRhythmChk() ;
		ResetMatch() ;
		ResetPostClassify() ;
		runCount = 0 ;
		DomMonitor(0, 0, 0, 0, 1) ;
		return(0) ;
		}

	hfNoise = HFNoiseCheck(newBeat) ;	// Check for muscle noise.
	rhythmClass = RhythmChk(rr) ;			// Check the rhythm.

	// Estimate beat features.

	AnalyzeBeat(newBeat, &onset, &offset, &isoLevel,
		&beatBegin, &beatEnd, &amp) ;

	blShift = abs(lastIsoLevel-isoLevel) ;
	lastIsoLevel = isoLevel ;

	// Make isoelectric level 0.

	for(i = 0; i < BEATLGTH; ++i)
		newBeat[i] -= isoLevel ;

	// If there was a significant baseline shift since
	// the last beat and the last beat was a new type,
	// delete the new type because it might have resulted
	// from a baseline shift.

	if( (blShift > BL_SHIFT_LIMIT)
		&& (lastBeatWasNew == 1)
		&& (lastRhythmClass == NORMAL)
		&& (rhythmClass == NORMAL) )
		ClearLastNewType() ;

	lastBeatWasNew = 0 ;

	// Find the template that best matches this beat.

	BestMorphMatch(newBeat,&morphType,&matchIndex,&mi2,&shiftAdj) ;

	// Disregard noise if the match is good. (New)

	if(matchIndex < MATCH_NOISE_THRESHOLD)
		hfNoise = noiseLevel = blShift = 0 ;

	// Apply a stricter match limit to premature beats.

	if((matchIndex < MATCH_LIMIT) && (rhythmClass == PVC) &&
		MinimumBeatVariation(morphType) && (mi2 > PVC_MATCH_WITH_AMP_LIMIT))
		{
		morphType = NewBeatType(newBeat) ;
		lastBeatWasNew = 1 ;
		}

	// Match if within standard match limits.

	else if((matchIndex < MATCH_LIMIT) && (mi2 <= MATCH_WITH_AMP_LIMIT))
		UpdateBeatType(morphType,newBeat,mi2,shiftAdj) ;

	// If the beat isn't noisy but doesn't match, start a new beat.

	else if((blShift < BL_SHIFT_LIMIT) && (noiseLevel < NEW_TYPE_NOISE_THRESHOLD)
		&& (hfNoise < NEW_TYPE_HF_NOISE_LIMIT))
		{
		morphType = NewBeatType(newBeat) ;
		lastBeatWasNew = 1 ;
		}

	// Even if it is a noisy, start new beat if it was an irregular beat.

	else if((lastRhythmClass != NORMAL) || (rhythmClass != NORMAL))
		{
		morphType = NewBeatType(newBeat) ;
		lastBeatWasNew = 1 ;
		}

	// If its noisy and regular, don't waste space starting a new beat.

	else morphType = MAXTYPES ;

	// Update recent rr and type arrays.

	for(i = 7; i > 0; --i)
		{
		RecentRRs[i] = RecentRRs[i-1] ;
		RecentTypes[i] = RecentTypes[i-1] ;
		}
	RecentRRs[0] = rr ;
	RecentTypes[0] = morphType ;

	lastRhythmClass = rhythmClass ;
	lastIsoLevel = isoLevel ;

	// Fetch beat features needed for classification.
	// Get features from average beat if it matched.

	if(morphType != MAXTYPES)
		{
		beatClass = GetBeatClass(morphType) ;
		beatWidth = GetBeatWidth(morphType) ;
		*fidAdj = GetBeatCenter(morphType)-FIDMARK ;

		// If the width seems large and there have only been a few
		// beats of this type, use the actual beat for width
		// estimate.

		if((beatWidth > offset-onset) && (GetBeatTypeCount(morphType) <= 4))
			{
			beatWidth = offset-onset ;
			*fidAdj = ((offset+onset)/2)-FIDMARK ;
			}
		}

	// If this beat didn't match get beat features directly
	// from this beat.

	else
		{
		beatWidth = offset-onset ;
		beatClass = UNKNOWN ;
		*fidAdj = ((offset+onset)/2)-FIDMARK ;
		}

	// Fetch dominant type beat features.

	DomType = domType = DomMonitor(morphType, rhythmClass, beatWidth, rr, 0) ;
	domWidth = GetBeatWidth(domType) ;

	// Compare the beat type, or actual beat to the dominant beat.

	if((morphType != domType) && (morphType != 8))
		domIndex = DomCompare(morphType,domType) ;
	else if(morphType == 8)
		domIndex = DomCompare2(newBeat,domType) ;
	else domIndex = matchIndex ;

	// Update post classificaton of the previous beat.

	PostClassify(RecentTypes, domType, RecentRRs, beatWidth, domIndex, rhythmClass) ;

	// Classify regardless of how the morphology
	// was previously classified.

	tempClass = TempClass(rhythmClass, morphType, beatWidth, domWidth,
		domType, hfNoise, noiseLevel, blShift, domIndex) ;

	// If this morphology has not been classified yet, attempt to classify
	// it.

	if((beatClass == UNKNOWN) && (morphType < MAXTYPES))
		{

		// Classify as normal if there are 6 in a row
		// or at least two in a row that meet rhythm
		// rules for normal.

		runCount = GetRunCount() ;

		// Classify a morphology as NORMAL if it is not too wide, and there
		// are three in a row.  The width criterion prevents ventricular beats
		// from being classified as normal during VTACH (MIT/BIH 205).

		if((runCount >= 3) && (domType != -1) && (beatWidth < domWidth+BEAT_MS20))
			SetBeatClass(morphType,NORMAL) ;

		// If there is no dominant type established yet, classify any type
		// with six in a row as NORMAL.

		else if((runCount >= 6) && (domType == -1))
			SetBeatClass(morphType,NORMAL) ;

		// During bigeminy, classify the premature beats as ventricular if
		// they are not too narrow.

		else if(IsBigeminy() == 1)
			{
			if((rhythmClass == PVC) && (beatWidth > BEAT_MS100))
				SetBeatClass(morphType,PVC) ;
			else if(rhythmClass == NORMAL)
				SetBeatClass(morphType,NORMAL) ;
			}
		}

	// Save morphology type of this beat for next classification.

	*beatMatch = morphType ;

	beatClass = GetBeatClass(morphType) ;
   
	// If the morphology has been previously classified.
	// use that classification.
  //	return(rhythmClass) ;

	if(beatClass != UNKNOWN)
		return(beatClass) ;

	if(CheckPostClass(morphType) == PVC)
		return(PVC) ;

	// Otherwise use the temporary classification.

	return(tempClass) ;
	}

/**************************************************************************
*  HFNoiseCheck() gauges the high frequency (muscle noise) present in the
*  beat template.  The high frequency noise level is estimated by highpass
*  filtering the beat (y[n] = x[n] - 2*x[n-1] + x[n-2]), averaging the
*  highpass filtered signal over five samples, and finding the maximum of
*  this averaged highpass filtered signal.  The high frequency noise metric
*  is then taken to be the ratio of the maximum averaged highpassed signal
*  to the QRS amplitude.
**************************************************************************/

#define AVELENGTH	BEAT_MS50

int HFNoiseCheck(int *beat)
	{
	int maxNoiseAve = 0, i ;
	int sum=0, aveBuff[AVELENGTH], avePtr = 0 ;
	int qrsMax = 0, qrsMin = 0 ;

	// Determine the QRS amplitude.

	for(i = FIDMARK-BEAT_MS70; i < FIDMARK+BEAT_MS80; ++i)
		if(beat[i] > qrsMax)
			qrsMax = beat[i] ;
		else if(beat[i] < qrsMin)
			qrsMin = beat[i] ;

	for(i = 0; i < AVELENGTH; ++i)
		aveBuff[i] = 0 ;

	for(i = FIDMARK-BEAT_MS280; i < FIDMARK+BEAT_MS280; ++i)
		{
		sum -= aveBuff[avePtr] ;
		aveBuff[avePtr] = abs(beat[i] - (beat[i-BEAT_MS10]<<1) + beat[i-2*BEAT_MS10]) ;
		sum += aveBuff[avePtr] ;
		if(++avePtr == AVELENGTH)
			avePtr = 0 ;
		if((i < (FIDMARK - BEAT_MS50)) || (i > (FIDMARK + BEAT_MS110)))
			if(sum > maxNoiseAve)
				maxNoiseAve = sum ;
		}
	if((qrsMax - qrsMin)>=4)
		return((maxNoiseAve * (50/AVELENGTH))/((qrsMax-qrsMin)>>2)) ;
	else return(0) ;
	}

/************************************************************************
*  TempClass() classifies beats based on their beat features, relative
*  to the features of the dominant beat and the present noise level.
*************************************************************************/

int TempClass(int rhythmClass, int morphType,
	int beatWidth, int domWidth, int domType,
	int hfNoise, int noiseLevel, int blShift, double domIndex)
	{

	// Rule 1:  If no dominant type has been detected classify all
	// beats as UNKNOWN.

	if(domType < 0)
		return(UNKNOWN) ;

	// Rule 2:  If the dominant rhythm is normal, the dominant
	// beat type doesn't vary much, this beat is premature
	// and looks sufficiently different than the dominant beat
	// classify as PVC.

	if(MinimumBeatVariation(domType) && (rhythmClass == PVC)
		&& (domIndex > R2_DI_THRESHOLD) && (GetDomRhythm() == 1))
		return(PVC) ;

	// Rule 3:  If the beat is sufficiently narrow, classify as normal.

	if(beatWidth < R3_WIDTH_THRESHOLD)
		return(NORMAL) ;

	// Rule 5:  If the beat cannot be matched to any previously
	// detected morphology and it is not premature, consider it normal
	// (probably noisy).

	if((morphType == MAXTYPES) && (rhythmClass != PVC)) // == UNKNOWN
		return(NORMAL) ;

	// Rule 6:  If the maximum number of beat types have been stored,
	// this beat is not regular or premature and only one
	// beat of this morphology has been seen, call it normal (probably
	// noisy).

	if((GetTypesCount() == MAXTYPES) && (GetBeatTypeCount(morphType)==1)
			 && (rhythmClass == UNKNOWN))
		return(NORMAL) ;

	// Rule 7:  If this beat looks like the dominant beat and the
	// rhythm is regular, call it normal.

	if((domIndex < R7_DI_THRESHOLD) && (rhythmClass == NORMAL))
		return(NORMAL) ;

	// Rule 8:  If post classification rhythm is normal for this
	// type and its shape is close to the dominant shape, classify
	// as normal.

	if((domIndex < R8_DI_THRESHOLD) && (CheckPCRhythm(morphType) == NORMAL))
		return(NORMAL) ;

	// Rule 9:  If the beat is not premature, it looks similar to the dominant
	// beat type, and the dominant beat type is variable (noisy), classify as
	// normal.

	if((domIndex < R9_DI_THRESHOLD) && (rhythmClass != PVC) && WideBeatVariation(domType))
		return(NORMAL) ;

	// Rule 10:  If this beat is significantly different from the dominant beat
	// there have previously been matching beats, the post rhythm classification
	// of this type is PVC, and the dominant rhythm is regular, classify as PVC.

	if((domIndex > R10_DI_THRESHOLD)
		&& (GetBeatTypeCount(morphType) >= R10_BC_LIM) &&
		(CheckPCRhythm(morphType) == PVC) && (GetDomRhythm() == 1))
		return(PVC) ;

	// Rule 11: if the beat is wide, wider than the dominant beat, doesn't
	// appear to be noisy, and matches a previous type, classify it as
	// a PVC.

	if( (beatWidth >= R11_MIN_WIDTH) &&
		(((beatWidth - domWidth >= R11_WIDTH_DIFF1) && (domWidth < R11_WIDTH_BREAK)) ||
		(beatWidth - domWidth >= R11_WIDTH_DIFF2)) &&
		(hfNoise < R11_HF_THRESHOLD) && (noiseLevel < R11_MA_THRESHOLD) && (blShift < BL_SHIFT_LIMIT) &&
		(morphType < MAXTYPES) && (GetBeatTypeCount(morphType) > R11_BC_LIM))	// Rev 1.1

		return(PVC) ;

	// Rule 12:  If the dominant rhythm is regular and this beat is premature
	// then classify as PVC.

	if((rhythmClass == PVC) && (GetDomRhythm() == 1))
		return(PVC) ;

	// Rule 14:  If the beat is regular and the dominant rhythm is regular
	// call the beat normal.

	if((rhythmClass == NORMAL) && (GetDomRhythm() == 1))
		return(NORMAL) ;

	// By this point, we know that rhythm will not help us, so we
	// have to classify based on width and similarity to the dominant
	// beat type.

	// Rule 15: If the beat is wider than normal, wide on an
	// absolute scale, and significantly different from the
	// dominant beat, call it a PVC.

	if((beatWidth > domWidth) && (domIndex > R15_DI_THRESHOLD) &&
		(beatWidth >= R15_WIDTH_THRESHOLD))
		return(PVC) ;

	// Rule 16:  If the beat is sufficiently narrow, call it normal.

	if(beatWidth < R16_WIDTH_THRESHOLD)
		return(NORMAL) ;

	// Rule 17:  If the beat isn't much wider than the dominant beat
	// call it normal.

	if(beatWidth < domWidth + R17_WIDTH_DELTA)
		return(NORMAL) ;

	// If the beat is noisy but reasonably close to dominant,
	// call it normal.

	// Rule 18:  If the beat is similar to the dominant beat, call it normal.

	if(domIndex < R18_DI_THRESHOLD)
		return(NORMAL) ;

	// If it's noisy don't trust the width.

	// Rule 19:  If the beat is noisy, we can't trust our width estimate
	// and we have no useful rhythm information, so guess normal.

	if(hfNoise > R19_HF_THRESHOLD)
		return(NORMAL) ;

	// Rule 20:  By this point, we have no rhythm information, the beat
	// isn't particularly narrow, the beat isn't particulary similar to
	// the dominant beat, so guess a PVC.

	return(PVC) ;

	}


/****************************************************************************
*  DomMonitor, monitors which beat morphology is considered to be dominant.
*  The dominant morphology is the beat morphology that has been most frequently
*  classified as normal over the course of the last 120 beats.  The dominant
*  beat rhythm is classified as regular if at least 3/4 of the dominant beats
*  have been classified as regular.
*******************************************************************************/

#define DM_BUFFER_LENGTH	180

int NewDom, DomRhythm ;
int DMBeatTypes[DM_BUFFER_LENGTH], DMBeatClasses[DM_BUFFER_LENGTH] ;
int DMBeatRhythms[DM_BUFFER_LENGTH] ;
int DMNormCounts[8], DMBeatCounts[8], DMIrregCount = 0 ;

int DomMonitor(int morphType, int rhythmClass, int beatWidth, int rr, int reset)
	{
	static int brIndex = 0 ;
	int i, oldType, runCount, dom, max ;

	// Fetch the type of the beat before the last beat.

	i = brIndex - 2 ;
	if(i < 0)
		i += DM_BUFFER_LENGTH ;
	oldType = DMBeatTypes[i] ;

	// If reset flag is set, reset beat type counts and
	// beat information buffers.

	if(reset != 0)
		{
		for(i = 0; i < DM_BUFFER_LENGTH; ++i)
			{
			DMBeatTypes[i] = -1 ;
			DMBeatClasses[i] = 0 ;
			}

		for(i = 0; i < 8; ++i)
			{
			DMNormCounts[i] = 0 ;
			DMBeatCounts[i] = 0 ;
			}
		DMIrregCount = 0 ;
		return(0) ;
		}

	// Once we have wrapped around, subtract old beat types from
	// the beat counts.

	if((DMBeatTypes[brIndex] != -1) && (DMBeatTypes[brIndex] != MAXTYPES))
		{
		--DMBeatCounts[DMBeatTypes[brIndex]] ;
		DMNormCounts[DMBeatTypes[brIndex]] -= DMBeatClasses[brIndex] ;
		if(DMBeatRhythms[brIndex] == UNKNOWN)
			--DMIrregCount ;
		}

	// If this is a morphology that has been detected before, decide
	// (for the purposes of selecting the dominant normal beattype)
	// whether it is normal or not and update the approporiate counts.

	if(morphType != 8)
		{

		// Update the buffers of previous beats and increment the
		// count for this beat type.

		DMBeatTypes[brIndex] = morphType ;
		++DMBeatCounts[morphType] ;
		DMBeatRhythms[brIndex] = rhythmClass ;

		// If the rhythm appears regular, update the regular rhythm
		// count.

		if(rhythmClass == UNKNOWN)
			++DMIrregCount ;

		// Check to see how many beats of this type have occurred in
		// a row (stop counting at six).

		i = brIndex - 1 ;
		if(i < 0) i += DM_BUFFER_LENGTH ;
		for(runCount = 0; (DMBeatTypes[i] == morphType) && (runCount < 6); ++runCount)
			if(--i < 0) i += DM_BUFFER_LENGTH ;

		// If the rhythm is regular, the beat width is less than 130 ms, and
		// there have been at least two in a row, consider the beat to be
		// normal.

		if((rhythmClass == NORMAL) && (beatWidth < BEAT_MS130) && (runCount >= 1))
			{
			DMBeatClasses[brIndex] = 1 ;
			++DMNormCounts[morphType] ;
			}

		// If the last beat was within the normal P-R interval for this beat,
		// and the one before that was this beat type, assume the last beat
		// was noise and this beat is normal.

		else if(rr < ((FIDMARK-GetBeatBegin(morphType))*SAMPLE_RATE/BEAT_SAMPLE_RATE)
			&& (oldType == morphType))
			{
			DMBeatClasses[brIndex] = 1 ;
			++DMNormCounts[morphType] ;
			}

		// Otherwise assume that this is not a normal beat.

		else DMBeatClasses[brIndex] = 0 ;
		}

	// If the beat does not match any of the beat types, store
	// an indication that the beat does not match.

	else
		{
		DMBeatClasses[brIndex] = 0 ;
		DMBeatTypes[brIndex] = -1 ;
		}

	// Increment the index to the beginning of the circular buffers.

	if(++brIndex == DM_BUFFER_LENGTH)
		brIndex = 0 ;

	// Determine which beat type has the most beats that seem
	// normal.

	dom = 0 ;
	for(i = 1; i < 8; ++i)
		if(DMNormCounts[i] > DMNormCounts[dom])
			dom = i ;

	max = 0 ;
	for(i = 1; i < 8; ++i)
		if(DMBeatCounts[i] > DMBeatCounts[max])
			max = i ;

	// If there are no normal looking beats, fall back on which beat
	// has occurred most frequently since classification began.

	if((DMNormCounts[dom] == 0) || (DMBeatCounts[max]/DMBeatCounts[dom] >= 2))			// == 0
		dom = GetDominantType() ;

	// If at least half of the most frequently occuring normal
	// type do not seem normal, fall back on choosing the most frequently
	// occurring type since classification began.

	else if(DMBeatCounts[dom]/DMNormCounts[dom] >= 2)
		dom = GetDominantType() ;

	// If there is any beat type that has been classfied as normal,
	// but at least 10 don't seem normal, reclassify it to UNKNOWN.

	for(i = 0; i < 8; ++i)
		if((DMBeatCounts[i] > 10) && (DMNormCounts[i] == 0) && (i != dom)
			&& (GetBeatClass(i) == NORMAL))
			SetBeatClass(i,UNKNOWN) ;

	// Save the dominant type in a global variable so that it is
	// accessable for debugging.

	NewDom = dom ;
	return(dom) ;
	}

int GetNewDominantType(void)
	{
	return(NewDom) ;
	}

int GetDomRhythm(void)
	{
	if(DMIrregCount > IRREG_RR_LIMIT)
		return(0) ;
	else return(1) ;
	}


void AdjustDomData(int oldType, int newType)
	{
	int i ;

	for(i = 0; i < DM_BUFFER_LENGTH; ++i)
		{
		if(DMBeatTypes[i] == oldType)
			DMBeatTypes[i] = newType ;
		}

	if(newType != MAXTYPES)
		{
		DMNormCounts[newType] = DMNormCounts[oldType] ;
		DMBeatCounts[newType] = DMBeatCounts[oldType] ;
		}

	DMNormCounts[oldType] = DMBeatCounts[oldType] = 0 ;

	}

void CombineDomData(int oldType, int newType)
	{
	int i ;

	for(i = 0; i < DM_BUFFER_LENGTH; ++i)
		{
		if(DMBeatTypes[i] == oldType)
			DMBeatTypes[i] = newType ;
		}

	if(newType != MAXTYPES)
		{
		DMNormCounts[newType] += DMNormCounts[oldType] ;
		DMBeatCounts[newType] += DMBeatCounts[oldType] ;
		}

	DMNormCounts[oldType] = DMBeatCounts[oldType] = 0 ;

	}

/***********************************************************************
	GetRunCount() checks how many of the present beat type have occurred
	in a row.
***********************************************************************/

GetRunCount()
	{
	int i ;
	for(i = 1; (i < 8) && (RecentTypes[0] == RecentTypes[i]); ++i) ;
	return(i) ;
	}

