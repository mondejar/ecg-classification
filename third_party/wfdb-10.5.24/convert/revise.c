/* file: revise.c	G. Moody	8 February 1991
			Last revised:   7 September 1999

-------------------------------------------------------------------------------
revise: Convert obsolete-format header files into new ones
Copyright (C) 1999 George B. Moody

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
#ifndef __STDC__
extern double atof();
extern void exit();
#endif
#ifndef BSD
#include <string.h>
#else
#include <strings.h>
#endif

/* netfile and WFDB_FILE definitions copied from ../lib/libwfdb.h */
struct netfile {
  char *url;
  char *data;
  int mode;
  long base_addr;
  long cont_len;
  long pos;
  long err;
  int fd;
};

struct WFDB_FILE {
  FILE *fp;
  struct netfile *netfp;
  int type;
};

typedef struct WFDB_FILE WFDB_FILE;

WFDB_FILE *iheader;
WFDB_FILE *wfdb_open();
WFDB_Siginfo si[WFDB_MAXSIG];
WFDB_Time nsamples;

main(argc, argv)
int argc;
char *argv[];
{
    char *record;
    int nsig;

    if (argc < 2) {
	(void)fprintf(stderr, "usage: %s record\n", argv[0]);
	(void)fprintf(stderr,
   " This program converts obsolete-format header files into new ones.\n");
	exit(1);
    }
    record = argv[1];
    if (strncmp(record, "header.", 7) == 0) record += 7;
    if ((nsig = readoldheader(record, si)) < 0)
	exit(2);
    (void)setheader(record, si, (unsigned)nsig);
    exit(0);	/*NOTREACHED*/
}

int readoldheader(record, siarray)
char *record;
WFDB_Siginfo *siarray;
{
    char linebuf[256], *p;
    WFDB_Frequency f;
    WFDB_Signal s;
    WFDB_Time ns;
    unsigned int i;
    static char sep[] = " \t\n";
#ifndef atof
    double atof();
#endif
#ifndef atol
    long atol();
#endif

    /* Try to open the header file. */
    if ((iheader = wfdb_open("header", record, WFDB_READ)) == NULL) {
	wfdb_error("init: can't open header for record %s\n", record);
	return (-1);
    }

    /* Get the first token (the record name) from the first non-empty,
       non-comment line. */
    do {
	if (fgets(linebuf, 256, iheader->fp) == NULL) {
	    wfdb_error("init: can't find record name in record %s header\n",
		     record);
	    return (-2);
	}
    } while ((p = strtok(linebuf, sep)) == NULL || *p == '#');
    if (iheader->fp != stdin && strcmp(p, record) != 0) {
	wfdb_error("init: record name in record %s header is incorrect\n",
		 record);
	return (-2);
    }

    /* Identify which type of header file is being read by trying to get
       another token from the line which contains the record name.  (Old-style
       headers have only one token on the first line, but new-style headers
       have two or more.) */
    if (p = strtok((char *)NULL, sep)) {
	(void)fprintf(stderr,
	   "The header for record %s appears not to be an old-style header.\n",
		record);
	return (-2);
    }

    else {
	/* We come here if there were no other tokens on the line which
	   contained the record name.  The file appears to be an old-style
	   header file. */

	WFDB_Group g, ng;
	int mpx;

	/* Determine the number of signal groups. */
	if (fgets(linebuf, 256, iheader->fp) == NULL ||
	    (p = strtok(linebuf, sep)) == NULL) {
	    wfdb_error("init: incorrect format in record %s header\n", record);
	    return (-2);
	}
	ng = atoi(p);

	/* Now get information for each signal group. */
	for (g = s = 0; g < ng; g++) {

	    /* Set the group number for the first signal in the group. */
	    siarray[s].group = g;

	    /* Get the signal file name. */
	    if (fgets(linebuf, 256, iheader->fp) == NULL ||
		(p = strtok(linebuf, sep)) == NULL ||
		(siarray[s].fname = (char *)malloc((unsigned)(strlen(p) + 1)))
		 == NULL) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    (void)strcpy(siarray[s].fname, p);

	    /* Determine the gain. */
	    if (fgets(linebuf, 256, iheader->fp) == NULL ||
		(p = strtok(linebuf, sep)) == NULL) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    siarray[s].gain = atof(p);

	    /* Determine the sampling frequency. */
	    if (fgets(linebuf, 256, iheader->fp) == NULL ||
		(p = strtok(linebuf, sep)) == NULL ||
		(f = atof(p)) <= 0.) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    (void)setsampfreq(f);

	    /* Determine the format, block size, and number of signals in
	       this group.  In old-style headers these three quantities are
	       encoded into two fields. */
	    if (fgets(linebuf, 256, iheader->fp) == NULL ||
		(p = strtok(linebuf, sep)) == NULL) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    i = (unsigned)atoi(p);
	    mpx = (p = strtok((char *)NULL, sep)) ? atoi(p) : 1;
	    if (mpx == 0) mpx = 1;
	    if (i != 8 && i != 16) {
		siarray[s].bsize = i;
		if (mpx < 0) {
		    mpx = -mpx;
		    siarray[s].fmt = 8;
		}
		else
		    siarray[s].fmt = 16;
	    }
	    else {
		siarray[s].fmt = i;
		siarray[s].bsize = 0;
	    }

	    /* Determine the ADC resolution. */
	    siarray[s].adcres = (p = strtok((char *)NULL, sep)) ?
		                atoi(p) : WFDB_DEFRES;

	    /* Determine the ADC zero. */
	    siarray[s].adczero = (p = strtok((char *)NULL, sep)) ? atoi(p) : 0;

	    /* Determine the initial value. */
	    if (fgets(linebuf, 256, iheader->fp) == NULL ||
		(p = strtok(linebuf, sep)) == NULL) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    siarray[s].initval = atoi(p);

	    /* Determine the number of samples. */
	    ns = (p = strtok((char *)NULL, sep)) ? atol(p) : 0L;
	    if (ns < 0L) {
		wfdb_error("init: incorrect format in record %s header\n",
			 record);
		return (-2);
	    }
	    if (nsamples == (WFDB_Time)0L) nsamples = ns;
	    else if (ns > (WFDB_Time)0L && ns != nsamples) {
		wfdb_error("warning (init):\n");
		wfdb_error(" record %s durations are inconsistent\n", record);
		/* nsamples must match the shortest record duration. */
		if (nsamples > ns)
		    nsamples = ns;
	    }

	    /* Determine the checksum. */
	    if (p = strtok((char *)NULL, sep)) {
		siarray[s].cksum = atoi(p);
		siarray[s].nsamp = ns;
	    }
	    else {
		siarray[s].cksum = 0;
		siarray[s].nsamp = (WFDB_Time)0L;
	    }

	    /* If this signal group contains more than one signal, make
	       additional copies of the siginfo structure for the other
	       signals. */
	    while (++s < WFDB_MAXSIG && --mpx > 0)
		siarray[s] = siarray[s-1];

	    if (s == WFDB_MAXSIG && (mpx > 0 || g < ng - 1)) {
		wfdb_error("init: too many signals in record %s\n", record);
		return (-2);
	    }
	}

	/* Now generate signal descriptions for each signal. */
	for (i = 0; i < s; i++) {
	    if ((siarray[i].desc=(char *)malloc((unsigned)(strlen(record)+20)))
		== NULL) {
		wfdb_error("init: insufficient memory\n");
		return (-2);
	    }
	    else
		(void)sprintf(siarray[i].desc,
			      "record %s, signal %d", record, i);
	}
    }

    return (s);			/* return number of available signals */
}
