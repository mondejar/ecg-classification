/* file: wfdbdesc.c		G. Moody	  June 1989
				Last revised:   6 April 2012

-------------------------------------------------------------------------------
wfdbdesc: Describe signal specifications
Copyright (C) 1989-2012 George B. Moody

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

This program is an expanded version of Example 5 in the WFDB Programmer's
Guide.

*/

#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    char *info, *p, *pname, *prog_name();
    int i, msrec = 0, nsig;
    FILE *ifile;
    WFDB_Siginfo *s;
    WFDB_Time t;

    pname = prog_name(argv[0]);
    if (argc < 2) {
        (void)fprintf(stderr, "usage: %s RECORD [-readable]\n", pname);
        exit(1);
    }
    /* Discover the number of signals defined in the header. */
    if ((nsig = isigopen(argv[1], NULL, 0)) < 0) exit(2);

    /* Allocate storage for nsig signal information structures. */
    if (nsig > 0 && (s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }

    /* If the `-readable' option is given, report only on signals which can
       be opened.  Otherwise, report on all signals named in the header file,
       without attempting to open them. */
    if (nsig > 0 && argc > 2 &&
	strncmp(argv[2], "-readable", strlen(argv[2])) == 0)
	nsig = isigopen(argv[1], s, nsig);
    else if (nsig > 0)
	nsig = isigopen(argv[1], s, -nsig);
    (void)printf("Record %s", argv[1]);
    setgvmode(WFDB_LOWRES);
    t = strtim("e");
    if (nsig > 0 && (s[0].fmt == 0 || s[0].nsamp != 0) && s[0].nsamp != t) {
 	msrec = 1;
	(void)printf(" (a %s-layout multi-segment record)\n",
		     s[0].nsamp == 0L ? "variable" : "fixed");
	if (s[0].nsamp != 0L) {
	    (void)printf("----------------------------------------------\n");
	    (void)printf("The following data apply to the entire record:\n");
	    (void)printf("----------------------------------------------");
	}
    }
    if (info = getinfo(argv[1])) {
       (void)printf("\nNotes\n=====\n");
       do {
	   puts(info);
       } while (info = getinfo((char *)NULL));
       (void)printf("=====\n");
    }
    p = mstimstr(0L);
    (void)printf("\nStarting time: %s\n", *p == '[' ? p : "not specified");
    (void)printf("Length: ");
    if (nsig < 1 || t > 0L)
	(void)printf("%s (%ld sample intervals)\n",
		     t > 0L ? mstimstr(t) : "0", t);
    else if (s[0].fmt && (ifile = fopen(s[0].fname, "rb")) &&
	     (fseek(ifile, 0L, 2) == 0)) {
	int framesize = 0;
	long nbytes = ftell(ifile) - wfdbgetstart(0);

	fclose(ifile);
	for (i = 0; i < nsig && s[i].group == 0; i++)
	    framesize += s[i].spf;
	switch (s[0].fmt) {
	  case 8:
	  case 80:
	    t = nbytes / framesize;
	    break;
	  default:
	  case 16:
	  case 61:
	  case 160:
	    t = nbytes / (2*framesize);
	    break;
	  case 212:
	    t = (2L * nbytes) / (3*framesize);
	    break;
	  case 310:
	  case 311:
	    t = (3L * nbytes) / (4*framesize);
	    break;
	}
	(void)printf("%s (%ld sample intervals) [from signal file size]\n",
		     mstimstr(t), t);
    }
    else (void)printf("not specified\n");
    (void)printf("Sampling frequency: %.12g Hz\n", sampfreq(NULL));
    (void)printf("%d signal%s\n", nsig, nsig == 1 ? "" : "s");
    if (nsig < 1) exit(2);
    if (msrec && s[0].nsamp != 0L) {
	(void)printf("----------------------------------------------\n");
	(void)printf("The following data apply to the first segment:\n");
	(void)printf("----------------------------------------------\n");
	(void)printf("Segment length: %s (%ld sample intervals)\n",
		     mstimstr(s[0].nsamp), s[0].nsamp);
    }
    for (i = 0; i < nsig; i++) {
        (void)printf("Group %d, Signal %d:\n", s[i].group, i);
        (void)printf(" File: %s\n", s[i].fmt ? s[i].fname : "[none]");
        (void)printf(" Description: %s\n", s[i].desc);
        (void)printf(" Gain: ");
        if (s[i].gain == 0.)
            (void)printf("uncalibrated; assume %g", WFDB_DEFGAIN);
        else (void)printf("%.12g", s[i].gain);
	(void)printf(" adu/%s\n", s[i].units ? s[i].units : "mV");
        (void)printf(" Initial value: %d\n", s[i].initval);
	if (s[i].fmt != 0)
	    (void)printf(" Storage format: %d", s[i].fmt);
	if (s[i].spf > 1)
	    (void)printf(" (%d samples per frame)", s[i].spf);
	if (s[i].fmt != 0 || s[i].spf > 1)
	    (void)printf("\n");
        (void)printf(" I/O: ");
        if (s[i].bsize == 0) (void)printf("can be unbuffered\n");
        else (void)printf("%d-byte blocks\n", s[i].bsize);
        (void)printf(" ADC resolution: %d bits\n", s[i].adcres);
        (void)printf(" ADC zero: %d\n", s[i].adczero);
	(void)printf(" Baseline: %d\n", s[i].baseline);
        if (s[i].nsamp > 0L)
            (void)printf(" Checksum: %d\n", s[i].cksum);
    }
    exit(0);	/*NOTREACHED*/
}

char *prog_name(s)
char *s;
{
    char *p = s + strlen(s);

#ifdef MSDOS
    while (p >= s && *p != '\\' && *p != ':') {
	if (*p == '.')
	    *p = '\0';		/* strip off extension */
	if ('A' <= *p && *p <= 'Z')
	    *p += 'a' - 'A';	/* convert to lower case */
	p--;
    }
#else
    while (p >= s && *p != '/')
	p--;
#endif
    return (p+1);
}
