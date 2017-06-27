/* file: wfdb2mat.c	G. Moody	26 February 2009
			Last revised:	 8 December 2013
-------------------------------------------------------------------------------
wfdb2mat: Convert (all or part of) a WFDB signal file to Matlab .mat format
Copyright (C) 2009-2013 George B. Moody

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

This program converts the signals of any PhysioBank record (or one in any
compatible format) into a .mat file that can be read directly using any version
of Matlab, and a short text file containing information about the signals
(names, gains, baselines, units, sampling frequency, and start time/date if
known).  If the input record name is REC, the output files are RECm.mat and
RECm.hea.  The output files can also be read by any WFDB application as record
RECm.

This program does not convert annotation files; for that task, 'rdann' is
recommended.

The output .mat file contains a single matrix named 'val' containing raw
(unshifted, unscaled) samples from the selected record.  Using various options,
you can select any time interval within a record, or any subset of the signals,
which can be rearranged as desired within the rows of the matrix.  Since .mat
files are written in column-major order (i.e., all of column n precedes all of
column n+1), each vector of samples is written as a column rather than as a
row, so that the column number in the .mat file equals the sample number in the
input record (minus however many samples were skipped at the beginning of the
record, as specified using the -f option).  If this seems odd, transpose your
matrix after reading it!

This program writes version 4 MAT-file format output files, as documented in
http://www.mathworks.com/access/helpdesk/help/pdf_doc/matlab/matfile_format.pdf
The samples are written as 32-bit signed integers (mattype=20 below) in
little-endian format if the record contains any format 24 or format 32 signals,
as 8-bit unsigned integers (mattype=50) if the record contains only format 80
signals, or as 16-bit signed integers in little-endian format (mattype=30)
otherwise.

Although version 5 and newer versions of Matlab normally use a different (less
compact and more complex) format, they can read these files without difficulty.
The advantage of version 4 MAT-file format, apart from compactness and
portability, is that files in these formats are still WFDB-compatible, given
the .hea file constructed by this program.

Example:

To convert record mitdb/200, use this command:
    wfdb2mat -r mitdb/200

This works even if the input files have not been downloaded;  in this case,
wfdb2mat reads them directly from the PhysioNet server.

The output files are mitdb/200m.mat and mitdb/200m.hea.  Note that if a
subdirectory named 'mitdb' did not exist already, it would be created by
wfdb2mat.

*/

#include <stdio.h>
#include <wfdb/wfdb.h>

/* Output .mat file types (values of mattype), defined by the .mat format
   specification, see above */
#define MAT8	50	/* 8 bits per sample */
#define MAT16	30	/* 16 bits per sample */
#define	MAT32	20	/* 32 bits per sample */

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char *matname, *orec, *p, *q, *record = NULL, *search = NULL, *prog_name();
    static char prolog[24];
    int highres = 0, i, isiglist, mattype, nisig, nosig = 0, pflag = 0,
	s, sfname = 0, *sig = NULL, stat = 0, vflag = 0, wfdbtype;
    WFDB_Frequency freq;
    WFDB_Sample *vi, *vo;
    WFDB_Siginfo *si, *so;
    WFDB_Time from = 0L, maxl = 0L, t, to = 0L;
    void help();

    pname = prog_name(argv[0]);
    for (i = 1 ; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'f':	/* starting time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -f\n", pname);
		exit(1);
	    }
	    from = i;
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* select high-resolution mode */
	    highres = 1;
	    break;
	  case 'l':	/* maximum length of output follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: max output length must follow -l\n",
			      pname);
		exit(1);
	    }
	    maxl = i;
	    break;
	  case 'r':	/* record name */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: record name must follow -r\n",
			      pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal list follows */
	    isiglist = i+1; /* index of first argument containing a signal # */
	    while (i+1 < argc && *argv[i+1] != '-') {
		i++;
		nosig++;	/* number of elements in signal list */
	    }
	    if (nosig == 0) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
	    break;
	  case 'S':	/* search for valid sample of specified signal */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: signal name or number must follow -S\n",
			      pname);
		exit(1);
	    }
	    search = argv[i];
	    break;
	      
	  case 't':	/* end time */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: time must follow -t\n",pname);
		exit(1);
	    }
	    to = i;
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n", pname,
			  argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n", pname,
			  argv[i]);
	    exit(1);
	}
    }
    if (record == NULL) {
	help();
	exit(1);
    }
    if ((nisig = isigopen(record, NULL, 0)) <= 0) exit(2);
    SUALLOC(si, nisig, sizeof(WFDB_Siginfo));
    SUALLOC(vi, nisig, sizeof(WFDB_Sample));
    if ((nisig = isigopen(record, si, nisig)) <= 0)
	exit(2);
    for (i = 0; i < nisig; i++)
	if (si[i].gain == 0.0) si[i].gain = WFDB_DEFGAIN;
    if (highres)
        setgvmode(WFDB_HIGHRES);
    freq = sampfreq(NULL);
    if (from > 0L && (from = strtim(argv[from])) < 0L)
	from = -from;
    if (isigsettime(from) < 0)
	exit(2);
    if (to > 0L && (to = strtim(argv[to])) < 0L)
	to = -to;

    if (nosig) {	      /* convert samples only from specified signals */
	SUALLOC(so, nosig, sizeof(WFDB_Siginfo));
	SUALLOC(vo, nosig, sizeof(WFDB_Sample));
	SUALLOC(sig, nosig, sizeof(int));
	for (i = 0; i < nosig; i++) {
	    if ((s = findsig(argv[isiglist+i])) < 0) {
		(void)fprintf(stderr, "%s: can't read signal '%s'\n", pname,
			      argv[isiglist+i]);
		exit(2);
	    }
	    sig[i] = s;
	}
    }
    else {			/* convert samples from all signals */
	nosig = nisig;
	SUALLOC(so, nosig, sizeof(WFDB_Siginfo));
	SUALLOC(vo, nosig, sizeof(WFDB_Sample));
	SUALLOC(sig, nosig, sizeof(int));
	for (i = 0; i < nosig; i++)
	    sig[i] = i;
    }

    /* Reset 'from' if a search was requested. */
    if (search &&
	((s = findsig(search)) < 0 || (from = tnextvec(s, from)) < 0)) {
	(void)fprintf(stderr, "%s: can't read signal '%s'\n", pname, search);
	exit(2);
    }
    /* Reset 'to' if a duration limit was specified. */
    if (maxl > 0L && (maxl = strtim(argv[maxl])) < 0L)
	maxl = -maxl;
    if (maxl && (to == 0L || to > from + maxl))
	to = from + maxl;

    t = strtim("e"); /* the end of the record */
    if (to == 0L || to > t)
	to = t;

    /* Quit if there will be no output. */
    if (to <= from) {
	wfdbquit();
	fprintf(stderr, "%s: no output\n", pname);
	fprintf(stderr, " Use the -f and -t options to specify time limits\n"
		"(or type '%s -h' for help)\n", pname);
	exit(1);
    }

    /* Generate the names for the output .mat file and the output record. */
    p = record + strlen(record) - 1;	/* *p = final char of record name */
    if (*p == '/')	/* short form name ('rec/' rather than 'rec/rec') */
	sfname = 1;
    while (--p > record)
	if (*p == '/') { p++; break; }  /* omit path components from orec */
    SUALLOC(orec, strlen(p)+2, sizeof(char));
    strncpy(orec, p, strlen(p) - sfname);
    /* If the input record is an EDF file, it will have a '.' in its name.
       The output record will not be an EDF file, so it may not have a '.'
       in its name.  Replace any '.' with '_' here. */
    for (p = orec; *p; p++)
	if (*p == '.') *p = '_';
    *p = 'm';	/* append 'm' to the output record name */
    SUALLOC(matname, strlen(record)+6, sizeof(char));
    sprintf(matname, "%s.mat", orec);

    /* Determine if we can write 8-bit unsigned samples, or if 16 or 32 bits are
       needed per sample. */
    mattype = MAT8;
    wfdbtype = 80;
    for (i = 0; i < nosig; i++)
	switch (si[sig[i]].fmt) {
	  case 24:
	  case 32: mattype = MAT32; wfdbtype = 32; break;
	  case 80: break;
	default: if (mattype != MAT32) mattype = MAT16; wfdbtype = 16; break;
	}
    for (i = 0; i < nosig; i++) {
	so[i] = si[sig[i]];
	so[i].fname = matname;
	so[i].group = 0;
	so[i].spf = 1;
	so[i].fmt = wfdbtype;
	/* handle possibly missing units strings */
	if (so[i].units == NULL) {
	    /* in a .hea file, missing units can be assumed to be millivolts */
	    SSTRCPY(so[i].units, "mV");
	}
	else if (strlen(so[i].units) == 0) {
	    /* this can happen only in an EDF file; in this case, the signal
	       is dimensionless (nd => "no dimension") */
	    SSTRCPY(so[i].units, "nd");
	}
    }

    /* Create an empty .mat file. */
    if (osigfopen(so, nosig) != nosig) {
	wfdbquit();		/* failed to open output, quit */
	exit(1);
    }
    
    /* Fill in the .mat file's prolog and write it. (Elements of prolog[]
       not set explicitly below are always zero.) */
    prolog[ 0] = mattype & 0xff;				/* format */
    prolog[ 1] = (mattype >> 8) & 0xff;
    prolog[ 4] = nosig & 0xff;			/* number of rows */
    prolog[ 5] = (nosig >> 8) & 0xff;
    prolog[ 6] = (nosig >> 16) & 0xff;
    prolog[ 7] = (nosig >> 24) & 0xff;
    prolog[ 8] = (to - from) & 0xff;		/* number of columns */
    prolog[ 9] = ((to - from) >> 8) & 0xff;
    prolog[10] = ((to - from) >> 16) & 0xff;
    prolog[11] = ((to - from) >> 24) & 0xff;
    prolog[16] = 4;		/* strlen("val") + 1 */
    sprintf(prolog+20, "val");
    wfdbputprolog((char *)prolog, 24, 0);

    /* Copy the selected data into the .mat file. */
    for (t = from; t < to && stat >= 0; t++) {
	stat = getvec(vi);
	for (i = 0; i < nosig; i++)
	    vo[i] = vi[sig[i]];
	if (putvec(vo) != nosig)
	    break;
    }

    /* If the input ended prematurely, pad the matrix with invalid samples. */
    if (t != to) {
	fprintf(stderr, "%s (warning): final %ld columns are invalid\n",
		pname, to - t);
	for (i = 0; i < nosig; i++)
	    vo[i] = WFDB_INVALID_SAMPLE;
	while (t++ < to)
	    putvec(vo);
    }

    /* Create the new header file. */
    p = mstimstr(-from);
    if (p && *p == '[')
	setbasetime(p+1);
    newheader(orec);

    /* Copy info from the old record, if any */
    if (p = getinfo(NULL))
	do {
	    (void)putinfo(p);
	} while (p = getinfo((char *)NULL));
    /* Append additional info summarizing what wfdb2mat has done. */
    SUALLOC(p, strlen(record)+80, 1);
    (void)sprintf(p, "Creator: %s", pname);
    (void)putinfo(p);
    (void)sprintf(p, "Source: record %s", record);
    q = mstimstr(0);
    while (*q == ' ') q++;
    if (from != 0 || *q == '[') {
	strcat(p, "  Start: ");
	strcat(p, q);
    }
    (void)putinfo(p);

    /* Summarize the contents of the .mat file. */
    printf("%s\n", p);
    printf("val has %d row%s (signal%s) and %ld column%s (sample%s/signal)\n",
	   nosig, nosig == 1 ? "" : "s", nosig == 1 ? "" : "s",
	   to-from, to == from+1 ? "" : "s", to == from+1 ? "" : "s");
    printf("Duration: %s\n", timstr(to-from));
    printf("Sampling frequency: %.12g Hz  Sampling interval: %.10g sec\n",
	   freq, 1/freq);
    printf("Row\tSignal\tGain\tBase\tUnits\n");
    for (i = 0; i < nosig; i++)
	printf("%d\t%s\t%.12g\t%d\t%s\n", i+1, so[i].desc, so[i].gain,
	       so[i].baseline, so[i].units);
    printf("\nTo convert from raw units to the physical units shown\n"
	   "above, subtract 'base' and divide by 'gain'.\n");

    SFREE(p);
    wfdbquit();

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

static char *help_strings[] = {
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the input record, and OPTIONS may include:",
 " -f TIME     begin at specified time",
 " -h          print this usage summary",
 " -H          read multifrequency signals in high resolution mode",
 " -l INTERVAL truncate output after the specified time interval (hh:mm:ss)",
 " -s SIGNAL   [SIGNAL ...]  convert only the specified signal(s)",
 " -S SIGNAL   search for a valid sample of the specified SIGNAL at or after",
 "		the time specified with -f, and begin converting then",
 " -t TIME     stop at specified time",
"Outputs are written to the current directory as RECORD.mat and RECORDm.hea.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
