/* file: log10.c	G. Moody	19 July 1995
		   Last revised:	22 March 2006

-------------------------------------------------------------------------------
log10: common log transform of 2-column data
Copyright (C) 1995-2006 George B. Moody

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

*/

#include <stdio.h>
#include <math.h>

#ifndef MINDOUBLE
#ifdef BSD
#include <float.h>
#define MINDOUBLE DBL_MIN
#define NOVALUES_H
#endif

#ifndef NOVALUES_H
#include <values.h>
#endif
#endif

#ifndef MINDOUBLE
#define MINDOUBLE 1.0e-10
#endif

main()
{
    double x, y;

    while (scanf("%lf%lf", &x, &y) == 2) {
	if (x < MINDOUBLE) x = MINDOUBLE;
	if (y < MINDOUBLE) y = MINDOUBLE;
	printf("%lf %lf %lf %lf\n", log10(x), log10(y), x, y);
    }
}
