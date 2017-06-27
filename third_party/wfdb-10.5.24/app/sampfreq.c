/* file: sampfreq.c	G. Moody	7 June 1998
			Last revised:  29 July 2012

-------------------------------------------------------------------------------
sampfreq: Print the sampling frequency of a record
Copyright (C) 1998-2012 George B. Moody

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
#include <wfdb/wfdb.h>

char *pname;

main(argc, argv)
int argc;
char **argv;
{
    void help();

    pname = argv[0];
    switch (argc) {
      case 1:
	  help();
	  exit(1);
	  break;
      case 2:
	  printf("%g\n", sampfreq(argv[1]));
	  exit(0);
	  break;
      case 3:
	  if (strcmp(argv[1], "-a") == 0 ||
	      strcmp(argv[1], "-H") == 0) {
	      int i, nsig;
	      double freq;
	      WFDB_Siginfo *si;

	      if ((nsig = isigopen(argv[2], NULL, 0)) > 0) {
		  SUALLOC(si, nsig, sizeof(WFDB_Siginfo));
		  isigopen(argv[2], si, nsig);
	      }
	      if (argv[1][1] == 'H') {
		  setgvmode(WFDB_HIGHRES);
		  printf("%g\n", sampfreq(NULL));
		  exit(0);
	      }
	      else {
		  setgvmode(WFDB_LOWRES);
		  freq = sampfreq(NULL);
		  for (i = 0; i < nsig; i++)
		      printf("%s\t%g\n", si[i].desc, si[i].spf * freq);
	      }
	  }
    }
}

void help()
{
    fprintf(stderr, "usage: %s [OPTION] RECORD\n", pname);
    fprintf(stderr,
	    " where RECORD is the name of the input record\n"
	    " and OPTION may be either of:\n"
	    "  -a   list all signals and their sampling frequencies\n"
	    "  -h   show this help\n"
    "  -H   show the maximum sampling frequency of a multifrequency record)\n"
    " If no OPTION is used, show the base sampling frequency (frame rate).\n");
}
