/*****************************************************************************

FILE:  postclas.cpp
AUTHOR:	Patrick S. Hamilton
REVISED:	5/13/2002
  ___________________________________________________________________________

postclas.cpp: Post classifier
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

This file contains functions for classifying beats based after the
following beat is detected.

	ResetPostClassify() -- Resets static variables used by
		PostClassify()
	PostClassify() -- classifies each beat based on six preceding
		beats and the following beat.
	CheckPostClass() --  classifys beat type based on the last
		eight post classifications of that beat.
	CheckPCRhythm() -- returns the classification of the RR interval
		for this type of beat based its previous eight RR intervals.

****************************************************************/

#include "bdac.h"
#include <wfdb/ecgcodes.h>

// External Prototypes.

double DomCompare(int newType, int domType) ;
int GetBeatTypeCount(int type) ;

// Records of post classifications.

int PostClass[MAXTYPES][8], PCInitCount = 0 ;
int PCRhythm[MAXTYPES][8] ;

/**********************************************************************
 Resets post classifications for beats.
**********************************************************************/

void ResetPostClassify()
	{
	int i, j ;
	for(i = 0; i < MAXTYPES; ++i)
		for(j = 0; j < 8; ++j)
			{
			PostClass[i][j] = 0 ;
			PCRhythm[i][j] = 0 ;
			}
	PCInitCount = 0 ;
	}

/***********************************************************************
	Classify the previous beat type and rhythm type based on this beat
	and the preceding beat.  This classifier is more sensitive
	to detecting premature beats followed by compensitory pauses.
************************************************************************/

void PostClassify(int *recentTypes, int domType, int *recentRRs, int width, double mi2,
	int rhythmClass)
	{
	static int lastRC, lastWidth ;
	static double lastMI2 ;
	int i, regCount, pvcCount, normRR ;
	double mi3 ;

	// If the preceeding and following beats are the same type,
	// they are generally regular, and reasonably close in shape
	// to the dominant type, consider them to be dominant.

	if((recentTypes[0] == recentTypes[2]) && (recentTypes[0] != domType)
		&& (recentTypes[0] != recentTypes[1]))
		{
		mi3 = DomCompare(recentTypes[0],domType) ;
		for(i = regCount = 0; i < 8; ++i)
			if(PCRhythm[recentTypes[0]][i] == NORMAL)
				++regCount ;
		if((mi3 < 2.0) && (regCount > 6))
			domType = recentTypes[0] ;
		}

	// Don't do anything until four beats have gone by.

	if(PCInitCount < 3)
		{
		++PCInitCount ;
		lastWidth = width ;
		lastMI2 = 0 ;
		lastRC = 0 ;
		return ;
		}

	if(recentTypes[1] < MAXTYPES)
		{

		// Find first NN interval.
		for(i = 2; (i < 7) && (recentTypes[i] != recentTypes[i+1]); ++i) ;
		if(i == 7) normRR = 0 ;
		else normRR = recentRRs[i] ;

		// Shift the previous beat classifications to make room for the
		// new classification.
		for(i = pvcCount = 0; i < 8; ++i)
			if(PostClass[recentTypes[1]][i] == PVC)
				++pvcCount ;

		for(i = 7; i > 0; --i)
			{
			PostClass[recentTypes[1]][i] = PostClass[recentTypes[1]][i-1] ;
			PCRhythm[recentTypes[1]][i] = PCRhythm[recentTypes[1]][i-1] ;
			}

		// If the beat is premature followed by a compensitory pause and the
		// previous and following beats are normal, post classify as
		// a PVC.

		if(((normRR-(normRR>>3)) >= recentRRs[1]) && ((recentRRs[0]-(recentRRs[0]>>3)) >= normRR)// && (lastMI2 > 3)
			&& (recentTypes[0] == domType) && (recentTypes[2] == domType)
				&& (recentTypes[1] != domType))
			PostClass[recentTypes[1]][0] = PVC ;

		// If previous two were classified as PVCs, and this is at least slightly
		// premature, classify as a PVC.

		else if(((normRR-(normRR>>4)) > recentRRs[1]) && ((normRR+(normRR>>4)) < recentRRs[0]) &&
			(((PostClass[recentTypes[1]][1] == PVC) && (PostClass[recentTypes[1]][2] == PVC)) ||
				(pvcCount >= 6) ) &&
			(recentTypes[0] == domType) && (recentTypes[2] == domType) && (recentTypes[1] != domType))
			PostClass[recentTypes[1]][0] = PVC ;

		// If the previous and following beats are the dominant beat type,
		// and this beat is significantly different from the dominant,
		// call it a PVC.

		else if((recentTypes[0] == domType) && (recentTypes[2] == domType) && (lastMI2 > 2.5))
			PostClass[recentTypes[1]][0] = PVC ;

		// Otherwise post classify this beat as UNKNOWN.

		else PostClass[recentTypes[1]][0] = UNKNOWN ;

		// If the beat is premature followed by a compensitory pause, post
		// classify the rhythm as PVC.

		if(((normRR-(normRR>>3)) > recentRRs[1]) && ((recentRRs[0]-(recentRRs[0]>>3)) > normRR))
			PCRhythm[recentTypes[1]][0] = PVC ;

		// Otherwise, post classify the rhythm as the same as the
		// regular rhythm classification.

		else PCRhythm[recentTypes[1]][0] = lastRC ;
		}

	lastWidth = width ;
	lastMI2 = mi2 ;
	lastRC = rhythmClass ;
	}


/*************************************************************************
	CheckPostClass checks to see if three of the last four or six of the
	last eight of a given beat type have been post classified as PVC.
*************************************************************************/

int CheckPostClass(int type)
	{
	int i, pvcs4 = 0, pvcs8 ;

	if(type == MAXTYPES)
		return(UNKNOWN) ;

	for(i = 0; i < 4; ++i)
		if(PostClass[type][i] == PVC)
			++pvcs4 ;
	for(pvcs8=pvcs4; i < 8; ++i)
		if(PostClass[type][i] == PVC)
			++pvcs8 ;

	if((pvcs4 >= 3) || (pvcs8 >= 6))
		return(PVC) ;
	else return(UNKNOWN) ;
	}

/****************************************************************************
	Check classification of previous beats' rhythms based on post beat
	classification.  If 7 of 8 previous beats were classified as NORMAL
	(regular) classify the beat type as NORMAL (regular).
	Call it a PVC if 2 of the last 8 were regular.
****************************************************************************/

int CheckPCRhythm(int type)
	{
	int i, normCount, n ;


	if(type == MAXTYPES)
		return(UNKNOWN) ;

	if(GetBeatTypeCount(type) < 9)
		n = GetBeatTypeCount(type)-1 ;
	else n = 8 ;

	for(i = normCount = 0; i < n; ++i)
		if(PCRhythm[type][i] == NORMAL)
			++normCount;
	if(normCount >= 7)
		return(NORMAL) ;
	if(((normCount == 0) && (n < 4)) ||
		((normCount <= 1) && (n >= 4) && (n < 7)) ||
		((normCount <= 2) && (n >= 7)))
		return(PVC) ;
	return(UNKNOWN) ;
	}
