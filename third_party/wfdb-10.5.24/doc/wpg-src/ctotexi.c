/* file: ctotexi.c	G. Moody	15 October 2001

-------------------------------------------------------------------------------
ctotexi: Format a C source file as texinfo
Copyright (C) 2001 George B. Moody

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

main()
{
  char ibuf[120], *p;
  int col, line = 0;

  while (fgets(ibuf, sizeof(ibuf), stdin)) {
    if (++line < 10) printf(" ");
    printf("@i{%d}  ", line);
    for (col = 0, p = ibuf; *p; p++, col++) {
      if (*p == '{' || *p == '}' || *p == '@')
	putchar('@');
      else if (*p == '\t') {
	*p = ' ';
	while (++col % 8)
	  putchar(' ');
      }
      putchar(*p);
    }
  }
}
