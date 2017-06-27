/*****************************************************************************
FILE:  match.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	5/13/2002
  ___________________________________________________________________________

match.cpp: Match beats to previous beats.
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

Match.cpp contains functions for managing template matching of beats and
managing of feature data associated with each beat type.  These
functions are called functions in classify.cpp.  Beats are matched to
previoiusly detected beats types based on how well they match point by point
in a MATCH_LENGTH region centered on FIDMARK (R-wave location).  The following
is a list of functions that are available for calling by classify.

	ResetMatch -- Resets global variables used in template matching.
	CompareBeats -- Measures the difference between two beats with
							beats scaled to produce the best match.
	CompareBeats2 -- Measures the difference between two beats without
							beat scaling.
	NewBeatType -- Start a new beat type with the present beat.
	BestMorphMatch -- Finds the beat template that best matches a new beat.
	UpdateBeatType -- Updates existing beat template and associated features
							based on a new beat.
	GetDominantType -- Returns the NORMAL beat type that has occorred most often.
	ClearLastNewType -- Removes the last new beat type from the possible beat
							types.
	DomCompare -- Compares the template for a given beat type to the template
						of the dominant normal beat type.
	DomCompare2 -- Compares a given beat template to the templat of the
						dominant normal beat type.

	PostClassify -- Classifies beats based on preceding and following beats
						and R-to-R intervals.

	ResetPostClassify -- Resets variables used for post classification.

	CheckPostClass -- Check type classification based on last eight post
						classifications.

	CheckPCClass -- Check post beat rhythm classification for the last eight
						beats.

A number of simple functions allow access to beat features while maintaining
some level of encapsulation:

	GetTypesCount -- Returns number of beat types that have been detected.
	GetBeatTypeCount -- Returns the number of beats of a given type
							  that have been detected.
	GetBeatWidth -- Returns the width estimate for a given beat type.
	SetBeatClass -- Associates a beat classification with a beat type.
	GetBeatBegin -- Returns the beginning point for a given beat type.
	GetBeatEnd -- Returns the ending point for a given beat type.

******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <wfdb/ecgcodes.h>

#include "bdac.h"
#define MATCH_LENGTH	BEAT_MS300	// Number of points used for beat matching.
#define MATCH_LIMIT	1.2			// Match limit used testing whether two
											// beat types might be combined.
#define COMBINE_LIMIT	0.8		// Limit used for deciding whether two types
											// can be combined.

#define MATCH_START	(FIDMARK-(MATCH_LENGTH/2))	// Starting point for beat matching
#define MATCH_END	(FIDMARK+(MATCH_LENGTH/2))		// End point for beat matching.
#define MAXPREV	8	// Number of preceeding beats used as beat features.
#define MAX_SHIFT	BEAT_MS40

// Local prototypes.

int NoiseCheck(int *beat) ;
double CompareBeats(int *beat1, int *beat2, int *shiftAdj) ;
double CompareBeats2(int *beat1, int *beat2, int *shiftAdj) ;
void UpdateBeat(int *aveBeat, int *newBeat, int shift) ;
void BeatCopy(int srcBeat, int destBeat) ;
int MinimumBeatVariation(int type) ;

// External prototypes.

void AnalyzeBeat(int *beat, int *onset, int *offset, int *isoLevel,
	int *beatBegin, int *beatEnd, int *amp) ;
void AdjustDomData(int oldType, int newType) ;
void CombineDomData(int oldType, int newType) ;

// Global variables.

int BeatTemplates[MAXTYPES][BEATLGTH] ;
int BeatCounts[MAXTYPES] ;
int BeatWidths[MAXTYPES] ;
int BeatClassifications[MAXTYPES] ;
int BeatBegins[MAXTYPES] ;
int BeatEnds[MAXTYPES] ;
int BeatsSinceLastMatch[MAXTYPES] ;
int BeatAmps[MAXTYPES] ;
int BeatCenters[MAXTYPES] ;
double MIs[MAXTYPES][8] ;

// Need access to these in postclas.cpp when beat types are combined
// and moved.

extern int PostClass[MAXTYPES][8] ;
extern int PCRhythm[MAXTYPES][8] ;

int TypeCount = 0 ;

/***************************************************************************
ResetMatch() resets static variables involved with template matching.
****************************************************************************/

void ResetMatch(void)
	{
	int i, j ;
	TypeCount = 0 ;
	for(i = 0; i < MAXTYPES; ++i)
		{
		BeatCounts[i] = 0 ;
		BeatClassifications[i] = UNKNOWN ;
		for(j = 0; j < 8; ++j)
			{
			MIs[i][j] = 0 ;
			}
		}
	}

/**************************************************************************
	CompareBeats() takes two beat buffers and compares how well they match
	point-by-point.  Beat2 is shifted and scaled to produce the closest
	possible match.  The metric returned is the sum of the absolute
	differences between beats divided by the amplitude of the beats.  The
	shift used for the match is returned via the pointer *shiftAdj.
***************************************************************************/

#define MATCH_START	(FIDMARK-(MATCH_LENGTH/2))
#define MATCH_END	(FIDMARK+(MATCH_LENGTH/2))

double CompareBeats(int *beat1, int *beat2, int *shiftAdj)
	{
	int i, max, min, magSum, shift ;
	long beatDiff, meanDiff, minDiff, minShift ;
	double metric, scaleFactor, tempD ;

	// Calculate the magnitude of each beat.

	max = min = beat1[MATCH_START] ;
	for(i = MATCH_START+1; i < MATCH_END; ++i)
		if(beat1[i] > max)
			max = beat1[i] ;
		else if(beat1[i] < min)
			min = beat1[i] ;

	magSum = max - min ;

	i = MATCH_START ;
	max = min = beat2[i] ;
	for(i = MATCH_START+1; i < MATCH_END; ++i)
		if(beat2[i] > max)
			max = beat2[i] ;
		else if(beat2[i] < min)
			min = beat2[i] ;

	// magSum += max - min ;
	scaleFactor = magSum ;
	scaleFactor /= max-min ;
	magSum *= 2 ;

	// Calculate the sum of the point-by-point
	// absolute differences for five possible shifts.

	for(shift = -MAX_SHIFT; shift <= MAX_SHIFT; ++shift)
		{
		for(i = FIDMARK-(MATCH_LENGTH>>1), meanDiff = 0;
			i < FIDMARK + (MATCH_LENGTH>>1); ++i)
			{
			tempD = beat2[i+shift] ;
			tempD *= scaleFactor ;
			meanDiff += beat1[i]- tempD ; // beat2[i+shift] ;
			}
		meanDiff /= MATCH_LENGTH ;

		for(i = FIDMARK-(MATCH_LENGTH>>1), beatDiff = 0;
			i < FIDMARK + (MATCH_LENGTH>>1); ++i)
			{
			tempD = beat2[i+shift] ;
			tempD *= scaleFactor ;
			beatDiff += abs(beat1[i] - meanDiff- tempD) ; // beat2[i+shift]  ) ;
			}


		if(shift == -MAX_SHIFT)
			{
			minDiff = beatDiff ;
			minShift = -MAX_SHIFT ;
			}
		else if(beatDiff < minDiff)
			{
			minDiff = beatDiff ;
			minShift = shift ;
			}
		}

	metric = minDiff ;
	*shiftAdj = minShift ;
	metric /= magSum ;

	// Metric scales inversely with match length.
	// algorithm was originally tuned with a match
	// length of 30.

	metric *= 30 ;
	metric /= MATCH_LENGTH ;
	return(metric) ;
	}

/***************************************************************************
	CompareBeats2 is nearly the same as CompareBeats above, but beat2 is
	not scaled before calculating the match metric.  The match metric is
	then the sum of the absolute differences divided by the average amplitude
	of the two beats.
****************************************************************************/

double CompareBeats2(int *beat1, int *beat2, int *shiftAdj)
	{
	int i, max, min, shift ;
	int mag1, mag2 ;
	long beatDiff, meanDiff, minDiff, minShift ;
	double metric ;

	// Calculate the magnitude of each beat.

	max = min = beat1[MATCH_START] ;
	for(i = MATCH_START+1; i < MATCH_END; ++i)
		if(beat1[i] > max)
			max = beat1[i] ;
		else if(beat1[i] < min)
			min = beat1[i] ;

	mag1 = max - min ;

	i = MATCH_START ;
	max = min = beat2[i] ;
	for(i = MATCH_START+1; i < MATCH_END; ++i)
		if(beat2[i] > max)
			max = beat2[i] ;
		else if(beat2[i] < min)
			min = beat2[i] ;

	mag2 = max-min ;

	// Calculate the sum of the point-by-point
	// absolute differences for five possible shifts.

	for(shift = -MAX_SHIFT; shift <= MAX_SHIFT; ++shift)
		{
		for(i = FIDMARK-(MATCH_LENGTH>>1), meanDiff = 0;
			i < FIDMARK + (MATCH_LENGTH>>1); ++i)
			meanDiff += beat1[i]- beat2[i+shift] ;
		meanDiff /= MATCH_LENGTH ;

		for(i = FIDMARK-(MATCH_LENGTH>>1), beatDiff = 0;
			i < FIDMARK + (MATCH_LENGTH>>1); ++i)
			beatDiff += abs(beat1[i] - meanDiff- beat2[i+shift]) ; ;

		if(shift == -MAX_SHIFT)
			{
			minDiff = beatDiff ;
			minShift = -MAX_SHIFT ;
			}
		else if(beatDiff < minDiff)
			{
			minDiff = beatDiff ;
			minShift = shift ;
			}
		}

	metric = minDiff ;
	*shiftAdj = minShift ;
	metric /= (mag1+mag2) ;

	// Metric scales inversely with match length.
	// algorithm was originally tuned with a match
	// length of 30.

	metric *= 30 ;
	metric /= MATCH_LENGTH ;

	return(metric) ;
	}

/************************************************************************
UpdateBeat() averages a new beat into an average beat template by adding
1/8th of the new beat to 7/8ths of the average beat.
*************************************************************************/

void UpdateBeat(int *aveBeat, int *newBeat, int shift)
	{
	int i ;
	long tempLong ;

	for(i = 0; i < BEATLGTH; ++i)
		{
		if((i+shift >= 0) && (i+shift < BEATLGTH))
			{
			tempLong = aveBeat[i] ;
			tempLong *= 7 ;
			tempLong += newBeat[i+shift] ;
			tempLong >>= 3 ;
			aveBeat[i] = tempLong ;
			}
		}
	}

/*******************************************************
	GetTypesCount returns the number of types that have
	been detected.
*******************************************************/

int GetTypesCount(void)
	{
	return(TypeCount) ;
	}

/********************************************************
	GetBeatTypeCount returns the number of beats of a
	a particular type have been detected.
********************************************************/

int GetBeatTypeCount(int type)
	{
	return(BeatCounts[type]) ;
	}

/*******************************************************
	GetBeatWidth returns the QRS width estimate for
	a given type of beat.
*******************************************************/
int GetBeatWidth(int type)
	{
	return(BeatWidths[type]) ;
	}

/*******************************************************
	GetBeatCenter returns the point between the onset and
	offset of a beat.
********************************************************/

int GetBeatCenter(int type)
	{
	return(BeatCenters[type]) ;
	}

/*******************************************************
	GetBeatClass returns the present classification for
	a given beat type (NORMAL, PVC, or UNKNOWN).
********************************************************/

int GetBeatClass(int type)
	{
	if(type == MAXTYPES)
		return(UNKNOWN) ;
	return(BeatClassifications[type]) ;
	}

/******************************************************
	SetBeatClass sets up a beat classifation for a
	given type.
******************************************************/

void SetBeatClass(int type, int beatClass)
	{
	BeatClassifications[type] = beatClass ;
	}

/******************************************************************************
	NewBeatType starts a new beat type by storing the new beat and its
	features as the next available beat type.
******************************************************************************/

int NewBeatType(int *newBeat )
	{
	int i, onset, offset, isoLevel, beatBegin, beatEnd ;
	int mcType, amp ;

	// Update count of beats since each template was matched.

	for(i = 0; i < TypeCount; ++i)
		++BeatsSinceLastMatch[i] ;

	if(TypeCount < MAXTYPES)
		{
		for(i = 0; i < BEATLGTH; ++i)
			BeatTemplates[TypeCount][i] = newBeat[i] ;

		BeatCounts[TypeCount] = 1 ;
		BeatClassifications[TypeCount] = UNKNOWN ;
		AnalyzeBeat(&BeatTemplates[TypeCount][0],&onset,&offset, &isoLevel,
			&beatBegin, &beatEnd, &amp) ;
		BeatWidths[TypeCount] = offset-onset ;
		BeatCenters[TypeCount] = (offset+onset)/2 ;
		BeatBegins[TypeCount] = beatBegin ;
		BeatEnds[TypeCount] = beatEnd ;
		BeatAmps[TypeCount] = amp ;

		BeatsSinceLastMatch[TypeCount] = 0 ;

		++TypeCount ;
		return(TypeCount-1) ;
		}

	// If we have used all the template space, replace the beat
	// that has occurred the fewest number of times.

	else
		{
		// Find the template with the fewest occurances,
		// that hasn't been matched in at least 500 beats.

		mcType = -1 ;

		if(mcType == -1)
			{
			mcType = 0 ;
			for(i = 1; i < MAXTYPES; ++i)
				if(BeatCounts[i] < BeatCounts[mcType])
					mcType = i ;
				else if(BeatCounts[i] == BeatCounts[mcType])
					{
					if(BeatsSinceLastMatch[i] > BeatsSinceLastMatch[mcType])
						mcType = i ;
					}
			}

		// Adjust dominant beat monitor data.

		AdjustDomData(mcType,MAXTYPES) ;

		// Substitute this beat.

		for(i = 0; i < BEATLGTH; ++i)
			BeatTemplates[mcType][i] = newBeat[i] ;

		BeatCounts[mcType] = 1 ;
		BeatClassifications[mcType] = UNKNOWN ;
		AnalyzeBeat(&BeatTemplates[mcType][0],&onset,&offset, &isoLevel,
			&beatBegin, &beatEnd, &amp) ;
		BeatWidths[mcType] = offset-onset ;
		BeatCenters[mcType] = (offset+onset)/2 ;
		BeatBegins[mcType] = beatBegin ;
		BeatEnds[mcType] = beatEnd ;
		BeatsSinceLastMatch[mcType] = 0 ;
      BeatAmps[mcType] = amp ;
		return(mcType) ;
		}
	}

/***************************************************************************
	BestMorphMatch tests a new beat against all available beat types and
	returns (via pointers) the existing type that best matches, the match
	metric for that type, and the shift used for that match.
***************************************************************************/

void BestMorphMatch(int *newBeat,int *matchType,double *matchIndex, double *mi2,
	int *shiftAdj)
	{
	int type, i, bestMatch, nextBest, minShift, shift, temp ;
	int bestShift2, nextShift2 ;
	double bestDiff2, nextDiff2;
	double beatDiff, minDiff, nextDiff=10000 ;

	if(TypeCount == 0)
		{
		*matchType = 0 ;
		*matchIndex = 1000 ;		// Make sure there is no match so a new beat is
		*shiftAdj = 0 ;			// created.
		return ;
		}

	// Compare the new beat to all type beat
	// types that have been saved.

	for(type = 0; type < TypeCount; ++type)
		{
		beatDiff = CompareBeats(&BeatTemplates[type][0],newBeat,&shift) ;
		if(type == 0)
			{
			bestMatch = 0 ;
			minDiff = beatDiff ;
			minShift = shift ;
			}
		else if(beatDiff < minDiff)
			{
			nextBest = bestMatch ;
			nextDiff = minDiff ;
			bestMatch = type ;
			minDiff = beatDiff ;
			minShift = shift ;
			}
		else if((TypeCount > 1) && (type == 1))
			{
			nextBest = type ;
			nextDiff = beatDiff ;
			}
		else if(beatDiff < nextDiff)
			{
			nextBest = type ;
			nextDiff = beatDiff ;
			}
		}

	// If this beat was close to two different
	// templates, see if the templates which template
	// is the best match when no scaling is used.
	// Then check whether the two close types can be combined.

	if((minDiff < MATCH_LIMIT) && (nextDiff < MATCH_LIMIT) && (TypeCount > 1))
		{
		// Compare without scaling.

		bestDiff2 = CompareBeats2(&BeatTemplates[bestMatch][0],newBeat,&bestShift2) ;
		nextDiff2 = CompareBeats2(&BeatTemplates[nextBest][0],newBeat,&nextShift2) ;
		if(nextDiff2 < bestDiff2)
			{
			temp = bestMatch ;
			bestMatch = nextBest ;
			nextBest = temp ;
			temp = minDiff ;
			minDiff = nextDiff ;
			nextDiff = temp ;
			minShift = nextShift2 ;
			*mi2 = bestDiff2 ;
			}
		else *mi2 = nextDiff2 ;

		beatDiff = CompareBeats(&BeatTemplates[bestMatch][0],&BeatTemplates[nextBest][0],&shift) ;

		if((beatDiff < COMBINE_LIMIT) &&
			((*mi2 < 1.0) || (!MinimumBeatVariation(nextBest))))
			{

			// Combine beats into bestMatch

			if(bestMatch < nextBest)
				{
				for(i = 0; i < BEATLGTH; ++i)
					{
					if((i+shift > 0) && (i + shift < BEATLGTH))
						{
						BeatTemplates[bestMatch][i] += BeatTemplates[nextBest][i+shift] ;
						BeatTemplates[bestMatch][i] >>= 1 ;
						}
					}

				if((BeatClassifications[bestMatch] == NORMAL) || (BeatClassifications[nextBest] == NORMAL))
					BeatClassifications[bestMatch] = NORMAL ;
				else if((BeatClassifications[bestMatch] == PVC) || (BeatClassifications[nextBest] == PVC))
					BeatClassifications[bestMatch] = PVC ;

				BeatCounts[bestMatch] += BeatCounts[nextBest] ;

				CombineDomData(nextBest,bestMatch) ;

				// Shift other templates over.

				for(type = nextBest; type < TypeCount-1; ++type)
					BeatCopy(type+1,type) ;

				}

			// Otherwise combine beats it nextBest.

			else
				{
				for(i = 0; i < BEATLGTH; ++i)
					{
					BeatTemplates[nextBest][i] += BeatTemplates[bestMatch][i] ;
					BeatTemplates[nextBest][i] >>= 1 ;
					}

				if((BeatClassifications[bestMatch] == NORMAL) || (BeatClassifications[nextBest] == NORMAL))
					BeatClassifications[nextBest] = NORMAL ;
				else if((BeatClassifications[bestMatch] == PVC) || (BeatClassifications[nextBest] == PVC))
					BeatClassifications[nextBest] = PVC ;

				BeatCounts[nextBest] += BeatCounts[bestMatch] ;

				CombineDomData(bestMatch,nextBest) ;

				// Shift other templates over.

				for(type = bestMatch; type < TypeCount-1; ++type)
					BeatCopy(type+1,type) ;


				bestMatch = nextBest ;
				}
			--TypeCount ;
			BeatClassifications[TypeCount] = UNKNOWN ;
			}
		}
	*mi2 = CompareBeats2(&BeatTemplates[bestMatch][0],newBeat,&bestShift2) ;
	*matchType = bestMatch ;
	*matchIndex = minDiff ;
	*shiftAdj = minShift ;
	}

/***************************************************************************
	UpdateBeatType updates the beat template and features of a given beat type
	using a new beat.
***************************************************************************/

void UpdateBeatType(int matchType,int *newBeat, double mi2,
	 int shiftAdj)
	{
	int i,onset,offset, isoLevel, beatBegin, beatEnd ;
	int amp ;

	// Update beats since templates were matched.

	for(i = 0; i < TypeCount; ++i)
		{
		if(i != matchType)
			++BeatsSinceLastMatch[i] ;
		else BeatsSinceLastMatch[i] = 0 ;
		}

	// If this is only the second beat, average it with the existing
	// template.

	if(BeatCounts[matchType] == 1)
		for(i = 0; i < BEATLGTH; ++i)
			{
			if((i+shiftAdj >= 0) && (i+shiftAdj < BEATLGTH))
				BeatTemplates[matchType][i] = (BeatTemplates[matchType][i] + newBeat[i+shiftAdj])>>1 ;
			}

	// Otherwise do a normal update.

	else
		UpdateBeat(&BeatTemplates[matchType][0], newBeat, shiftAdj) ;

	// Determine beat features for the new average beat.

	AnalyzeBeat(&BeatTemplates[matchType][0],&onset,&offset,&isoLevel,
		&beatBegin, &beatEnd, &amp) ;

	BeatWidths[matchType] = offset-onset ;
	BeatCenters[matchType] = (offset+onset)/2 ;
	BeatBegins[matchType] = beatBegin ;
	BeatEnds[matchType] = beatEnd ;
	BeatAmps[matchType] = amp ;

	++BeatCounts[matchType] ;

	for(i = MAXPREV-1; i > 0; --i)
		MIs[matchType][i] = MIs[matchType][i-1] ;
	MIs[matchType][0] = mi2 ;

	}


/****************************************************************************
	GetDominantType returns the NORMAL beat type that has occurred most
	frequently.
****************************************************************************/

int GetDominantType(void)
	{
	int maxCount = 0, maxType = -1 ;
	int type, totalCount ;

	for(type = 0; type < MAXTYPES; ++type)
		{
		if((BeatClassifications[type] == NORMAL) && (BeatCounts[type] > maxCount))
			{
			maxType = type ;
			maxCount = BeatCounts[type] ;
			}
		}

	// If no normals are found and at least 300 beats have occurred, just use
	// the most frequently occurring beat.

	if(maxType == -1)
		{
		for(type = 0, totalCount = 0; type < TypeCount; ++type)
			totalCount += BeatCounts[type] ;
		if(totalCount > 300)
			for(type = 0; type < TypeCount; ++type)
				if(BeatCounts[type] > maxCount)
					{
					maxType = type ;
					maxCount = BeatCounts[type] ;
					}
		}

	return(maxType) ;
	}


/***********************************************************************
	ClearLastNewType removes the last new type that was initiated
************************************************************************/

void ClearLastNewType(void)
	{
	if(TypeCount != 0)
		--TypeCount ;
	}

/****************************************************************
	GetBeatBegin returns the offset from the R-wave for the
	beginning of the beat (P-wave onset if a P-wave is found).
*****************************************************************/

int GetBeatBegin(int type)
	{
	return(BeatBegins[type]) ;
	}

/****************************************************************
	GetBeatEnd returns the offset from the R-wave for the end of
	a beat (T-wave offset).
*****************************************************************/

int GetBeatEnd(int type)
	{
	return(BeatEnds[type]) ;
	}

int GetBeatAmp(int type)
	{
	return(BeatAmps[type]) ;
	}


/************************************************************************
	DomCompare2 and DomCompare return similarity indexes between a given
	beat and the dominant normal type or a given type and the dominant
	normal type.
************************************************************************/

double DomCompare2(int *newBeat, int domType)
	{
	int shift ;
	return(CompareBeats2(&BeatTemplates[domType][0],newBeat,&shift)) ;
	}

double DomCompare(int newType, int domType)
	{
	int shift ;
	return(CompareBeats2(&BeatTemplates[domType][0],&BeatTemplates[newType][0],
		&shift)) ;
	}

/*************************************************************************
BeatCopy copies beat data from a source beat to a destination beat.
*************************************************************************/

void BeatCopy(int srcBeat, int destBeat)
	{
	int i ;

	// Copy template.

	for(i = 0; i < BEATLGTH; ++i)
		BeatTemplates[destBeat][i] = BeatTemplates[srcBeat][i] ;

	// Move feature information.

	BeatCounts[destBeat] = BeatCounts[srcBeat] ;
	BeatWidths[destBeat] = BeatWidths[srcBeat] ;
	BeatCenters[destBeat] = BeatCenters[srcBeat] ;
	for(i = 0; i < MAXPREV; ++i)
		{
		PostClass[destBeat][i] = PostClass[srcBeat][i] ;
		PCRhythm[destBeat][i] = PCRhythm[srcBeat][i] ;
		}

	BeatClassifications[destBeat] = BeatClassifications[srcBeat] ;
	BeatBegins[destBeat] = BeatBegins[srcBeat] ;
	BeatEnds[destBeat] = BeatBegins[srcBeat] ;
	BeatsSinceLastMatch[destBeat] = BeatsSinceLastMatch[srcBeat];
	BeatAmps[destBeat] = BeatAmps[srcBeat] ;

	// Adjust data in dominant beat monitor.

	AdjustDomData(srcBeat,destBeat) ;
	}

/********************************************************************
	Minimum beat variation returns a 1 if the previous eight beats
	have all had similarity indexes less than 0.5.
*********************************************************************/

int MinimumBeatVariation(int type)
	{
	int i ;
	for(i = 0; i < MAXTYPES; ++i)
		if(MIs[type][i] > 0.5)
			i = MAXTYPES+2 ;
	if(i == MAXTYPES)
		return(1) ;
	else return(0) ;
	}

/**********************************************************************
	WideBeatVariation returns true if the average similarity index
	for a given beat type to its template is greater than WIDE_VAR_LIMIT.
***********************************************************************/

#define WIDE_VAR_LIMIT	0.50

int WideBeatVariation(int type)
	{
	int i, n ;
	double aveMI ;

	n = BeatCounts[type] ;
	if(n > 8)
		n = 8 ;

	for(i = 0, aveMI = 0; i <n; ++i)
		aveMI += MIs[type][i] ;

	aveMI /= n ;
	if(aveMI > WIDE_VAR_LIMIT)
		return(1) ;
	else return(0) ;
	}
