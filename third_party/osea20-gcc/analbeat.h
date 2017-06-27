/*****************************************************************************
FILE:  analbeat.h
AUTHOR:	Patrick S. Hamilton
REVISED:	12/4/2001
  ___________________________________________________________________________
analbeat.h: Beat analysis prototype definition.
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
******************************************************************************/

// External prototypes for analbeat.cpp

void AnalyzeBeat(int *beat, int *onset, int *offset,
	int *isoLevel, int *beatBegin, int *beatEnd, int *amp) ;
