/*****************************************************************************

FILE:  bdac.h
AUTHOR:	Patrick S. Hamilton
REVISED:	9/25/2001
  ___________________________________________________________________________

bdac.h: Beat detection and classification parameter definitions.
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
#define BEAT_SAMPLE_RATE	100
#define BEAT_MS_PER_SAMPLE	( (double) 1000/ (double) BEAT_SAMPLE_RATE)

#define BEAT_MS10		((int) (10/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS20		((int) (20/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS40		((int) (40/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS50		((int) (50/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS60		((int) (60/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS70		((int) (70/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS80		((int) (80/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS90		((int) (90/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS100	((int) (100/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS110	((int) (110/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS130	((int) (130/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS140	((int) (140/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS150	((int) (150/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS250	((int) (250/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS280	((int) (280/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS300	((int) (300/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS350	((int) (350/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS400	((int) (400/BEAT_MS_PER_SAMPLE + 0.5))
#define BEAT_MS1000	BEAT_SAMPLE_RATE

#define BEATLGTH	BEAT_MS1000
#define MAXTYPES 8
#define FIDMARK BEAT_MS400
