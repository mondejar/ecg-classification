/* file: calib.c	G. Moody	  4 July 1991
			Last revised:  18 November 2013		wfdblib 10.5.21
WFDB library functions for signal calibration

_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1991-2013 George B. Moody

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

This file contains definitions of the following WFDB library functions:
 calopen	(initializes calibration list from file)
 getcal		(gets WFDB_Calinfo structure matching signal specification)
 putcal		(inserts WFDB_Calinfo structure in calibration list)
 newcal		(writes calibration list to specified file)
 flushcal	(empties calibration list)

(These functions are new in WFDB library version 6.0.)

Beginning with version 6.1, calibration files are written by newcal() with
\r\n line separators (version 6.0 used \n only).  These are not compatible
with calopen() version 6.0, although calibration files written by newcal()
version 6.0 can be read successfully by all versions of calopen().  This change
was made so that calibration files can be more easily viewed and edited under
MS-DOS.

Beginning with version 8.3, calopen() uses the default calibration file name
(DEFWFDBCAL, defined in wfdblib.h) if passed a NULL argument and if the WFDBCAL
environment variable is not set.
*/

#include "wfdblib.h"

/* Calibration list entries each contain a WFDB_Calinfo record (crec) and
   a forward pointer (next). */
static struct cle {
    double low;		/* low level of calibration pulse in physical units */
    double high;	/* high level of calibration pulse in physical units */
    double scale;	/* customary plotting scale (physical units per cm) */
    char *sigtype;	/* signal type */
    char *units;	/* physical units */
    int caltype;	/* calibration pulse type (see definitions above) */
    struct cle *next;
} *first_cle, *this_cle, *prev_cle;

/* Function calopen reads the specified calibration file;  if called with
   a NULL argument, it takes the value of the environment variable WFDBCAL
   to be the name of the calibration file.  If the calibration file name
   does not begin with "+", calopen empties the calibration list first;
   otherwise, the "+" is discarded before attempting to open the file,
   which may be in any directory in the WFDB path.  If the file can be read,
   its contents are appended to the calibration list. */
FINT calopen(char *cfname)
{
    WFDB_FILE *cfile;
    char buf[128], *p1, *p2, *p3, *p4, *p5, *p6;

    /* If no calibration file is specified, return immediately. */
    if (cfname == NULL && (cfname = getenv("WFDBCAL")) == NULL &&
	(cfname = DEFWFDBCAL) == NULL)
	return (0);

    if (*cfname == '+')		/* don't empty the calibration list */
	cfname++;		/* discard the '+' prefix */
    else flushcal();		/* empty the calibration list */

    /* Quit if file can't be found or opened. */
    if ((cfile = wfdb_open(cfname, (char *)NULL, WFDB_READ)) == NULL) {
	wfdb_error("calopen: can't read calibration file %s\n", cfname);
	return (-2);
    }

    /* Read a line of the calibration file on each iteration.  See wfdbcal(5)
       for a description of the format. */
    while (wfdb_fgets(buf, 127, cfile)) {
	/* ignore leading whitespace */
	for (p1 = buf; *p1 == ' ' || *p1 == '\t' || *p1 == '\r'; p1++)
	    ;
	/* skip comments, empty lines, and improperly formatted lines */
	if (*p1 == '#' ||
	    (p1 = strtok(p1, "\t")) == NULL ||		    /* signal type */
	    (p2 = strtok((char *)NULL, " \t")) == NULL ||   /* low, or '-' */
	    (p3 = strtok((char *)NULL, " \t")) == NULL ||   /* high, or '-' */
	    (p4 = strtok((char *)NULL, " \t")) == NULL ||   /* pulse type */
	    (p5 = strtok((char *)NULL, " \t")) == NULL ||   /* scale */
	    (p6 = strtok((char *)NULL, " \t\r\n")) == NULL) /* units */
	    continue;

	/* This line appears to be a correctly formatted entry.  Allocate
	   memory for a calibration list entry. */
	SUALLOC(this_cle, 1, (sizeof(struct cle)));

	/* Fill in the fields of the new calibration list entry. */
	SSTRCPY(this_cle->sigtype, p1);
	if (strcmp(p2, "-") == 0) {
	    this_cle->caltype = WFDB_AC_COUPLED;
	    this_cle->low = 0.0;
	}
	else {
	    this_cle->caltype = WFDB_DC_COUPLED;
	    this_cle->low = strtod(p2, NULL);
	}
	if (strcmp(p3, "-") == 0)
	    this_cle->high = this_cle->low = 0.0;
	else
	    this_cle->high = strtod(p3, NULL);
	if (strcmp(p4, "square") == 0)
	    this_cle->caltype |= WFDB_CAL_SQUARE;
	else if (strcmp(p4, "sine") == 0)
	    this_cle->caltype |= WFDB_CAL_SINE;
	else if (strcmp(p4, "sawtooth") == 0)
	    this_cle->caltype |= WFDB_CAL_SAWTOOTH;
	/* otherwise pulse shape is undefined */
	this_cle->scale = strtod(p5, NULL);
	SSTRCPY(this_cle->units, p6);
	this_cle->next = NULL;

	/* Append the new entry to the end of the list. */
	if (first_cle) {
	    prev_cle->next = this_cle;
	    prev_cle = this_cle;
	}
	else
	    first_cle = prev_cle = this_cle;
    }

    (void)wfdb_fclose(cfile);
    return (0);
}

/* Function getcal attempts to find a calibration record which matches the
   supplied desc and units strings.  If the sigtype field of the record
   is a prefix of or an exact match for the desc string, and if the units
   field of the record is an exact match for the units string, the record
   matches.  If either desc or units is NULL, it is ignored for the purpose
   of finding a match.  If a match is found, it is copied into the caller's
   WFDB_Calinfo structure.  The caller must not write over the storage
   addressed by the desc and units fields. */
FINT getcal(char *desc, char *units, WFDB_Calinfo *cal)
{
    for (this_cle = first_cle; this_cle; this_cle = this_cle->next) {
	if ((desc == NULL || strncmp(desc, this_cle->sigtype,
				     strlen(this_cle->sigtype)) == 0) &&
	    (units == NULL || strcmp(units, this_cle->units) == 0)) {
	    cal->low = this_cle->low;
	    cal->high = this_cle->high;
	    cal->scale = this_cle->scale;
	    cal->sigtype = this_cle->sigtype;
	    cal->units = this_cle->units;
	    cal->caltype = this_cle->caltype;
	    return (0);
	}
    }
    return (-1);
}

/* Function putcal appends the caller's WFDB_Calinfo structure to the end of
   the calibration list. */
FINT putcal(WFDB_Calinfo *cal)
{
    SUALLOC(this_cle, 1, sizeof(struct cle));
    SSTRCPY(this_cle->sigtype, cal->sigtype);
    this_cle->caltype = cal->caltype;
    this_cle->low = cal->low;
    this_cle->high = cal->high;
    this_cle->scale = cal->scale;
    SSTRCPY(this_cle->units, cal->units);
    this_cle->next = NULL;

    if (first_cle) {
	prev_cle->next = this_cle;
	prev_cle = this_cle;
    }
    else
	first_cle = prev_cle = this_cle;
    return (0);
}

/* Function newcal generates a calibration file based on the contents of the
   calibration list. */
FINT newcal(char *cfname)
{
    WFDB_FILE *cfile;

    if (wfdb_checkname(cfname, "calibration file") < 0)
	return (-1);

    if ((cfile = wfdb_open(cfname, (char *)NULL, WFDB_WRITE)) == NULL) {
	wfdb_error("newcal: can't create calibration file %s\n", cfname);
	return (-1);
    }

    for (this_cle = first_cle; this_cle; this_cle = this_cle->next) {
	char *pulsetype;

	(void)wfdb_fprintf(cfile, "%s\t", this_cle->sigtype);
	if (this_cle->caltype & WFDB_DC_COUPLED)
	    (void)wfdb_fprintf(cfile, "%g ", this_cle->low);
	else
	    (void)wfdb_fprintf(cfile, "- ");
	if (this_cle->high != this_cle->low)
	    (void)wfdb_fprintf(cfile, "%g ", this_cle->high);
	else
	    (void)wfdb_fprintf(cfile, "- ");
	switch (this_cle->caltype & (~WFDB_DC_COUPLED)) {
	  case WFDB_CAL_SQUARE:	pulsetype = "square"; break;
	  case WFDB_CAL_SINE:	pulsetype = "sine"; break;
	  case WFDB_CAL_SAWTOOTH:	pulsetype = "sawtooth"; break;
	  default:		pulsetype = "undefined"; break;
	}
	(void)wfdb_fprintf(cfile, "%s %g %s\r\n",
		      pulsetype,
		      this_cle->scale,
		      this_cle->units);
    }
    (void)wfdb_fclose(cfile);
    return (0);
}

/* Function flushcal empties the calibration list. */
FVOID flushcal(void)
{
    while (first_cle) {
	SFREE(first_cle->sigtype);
	SFREE(first_cle->units);
	this_cle = first_cle->next;
	SFREE(first_cle);
	first_cle = this_cle;
    }
}

	
