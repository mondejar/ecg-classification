/* file: ad2m.c		G. Moody	26 August 1983
			Last revised:   20 December 2007

-------------------------------------------------------------------------------
ad2m: Convert an AHA format signal file to MIT format
Copyright (C) 1983-2007 George B. Moody

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

This program converts AHA DB signal files in either the original tape format
or the `compressed' CD-ROM/diskette file format to MIT format.  It also
generates header files.
*/

#include <stdio.h>
#ifdef __STDC__
#define RB	"rb"
#else
#ifdef MSDOS
#define RB	"rb"
#else
#define RB	"r"
extern void exit();
#endif
#endif

#include <wfdb/wfdb.h>

#define EODF	0100000	/* AHA data file end-of-data marker */
#define FNLEN	12	/* maximum length for signal file name */

char *pname;

main(argc, argv)
int argc;
char *argv[];
{
    char dfname[FNLEN], *ifname = NULL, *p, *record = NULL, *prog_name();
    int cflag = 0, i, v[2], getcvec();
    long t = 0L, start_time = 0L, end_time = 0L;
    static WFDB_Siginfo dfarray[2];
    void help();

    pname = prog_name(argv[0]);
    (void)setsampfreq(250.);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'c':
	    cflag = 1;
	    break;
	  case 'f':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: start time must follow -f\n", pname);
		exit(1);
	    }
	    start_time = strtim(argv[i]);
	    break;
	  case 'h':	/* help requested */
	    help();
	    exit(1);
	    break;
	  case 'i':	/* input file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		      "%s: name of AHA-format input file must follow -i\n",
			      pname);
		exit(1);
	    }
	    ifname = argv[i];
	    break;
	  case 'r':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 't':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			"%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end_time = strtim(argv[i]);
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
    if (ifname == NULL || (0L < end_time && end_time <= start_time)) {
	help();
	exit(1);
    }
    if (cflag == 0 && strlen(ifname) > 4) {
	char *p = ifname + strlen(ifname) - 4;

	if (strcmp(p, ".CMP") == 0 || strcmp(p, ".cmp") == 0)
	    cflag = 1;
    }

    /* Attach the input file to the standard input. */
    if (strcmp(ifname, "-") != 0 && freopen(ifname, RB, stdin) == NULL) {
	(void)fprintf(stderr, "%s: can't read input file `%s'\n", pname,
		      ifname);
	exit(2);
    }

    /* If no output record name was specified, use the basename of the input
       file (prog_name strips off any path info from ifname, and any extension
       is stripped off in the 'for' loop. */
    if (record == NULL) {
	record = prog_name(ifname);
	for (p = record+1; *p; p++)
	    if (*p == '.') {
		*p = '\0';
		break;
	    }
    }

    if (end_time == 0L && strlen(record) == 4) {      /* assume AHA DB input */
	if (record[1] == '2' || record[1] == '3')     /* short format record */
	    end_time = strtim("35:0");
	else if (record[1] == '0' || record[1] == '1') /* long format record */
	    end_time = strtim("3:0:0");
    }

    if (end_time == 0L) {
	(void)fprintf(stderr,
               "%s: warning: end time not specified, output may be truncated\n",
		      pname);
	(void)fprintf(stderr,
		      "               rerun with -t TIME to override\n");
	end_time = start_time + strtim("100:0:0");
    }

    (void)sprintf(dfname, "%s.dat", record);
    dfarray[0].fname = dfarray[1].fname = dfname;
    dfarray[0].desc = dfarray[1].desc = "";
    dfarray[0].gain = dfarray[1].gain = 400;
    dfarray[0].fmt = dfarray[1].fmt = 212;
    dfarray[0].adcres = dfarray[1].adcres = 12;
    if (osigfopen(dfarray, 2) < 0) exit(3);

    if (!cflag) {	/* process tape-format input file */
	if (isigopen("16", dfarray, 2) < 2)
	    exit(2);
	if (start_time > 0L && isigsettime(t = start_time) < 0)
	    exit(2);
	while (getvec(v) == 2 && v[0] != EODF && putvec(v)>0 && ++t < end_time)
	    ;	/* continue to soft EOF, hard error, or specified time */

	(void)newheader(record);	/* write header with correct signal
					   length and checksums */
	while (getvec(v) >= 0)
	    ;	/* continue to hard EOF (to position tape properly if reading
		   from tape) */
    }

    else {		/* process compressed-format input file */
	while (t < start_time && getcvec(v) == 2)
	    ++t;
	while (getcvec(v) == 2 && putvec(v) > 0 && ++t < end_time)
	    ;	/* continue to hard EOF, hard error, or specified time */

	(void)newheader(record);	/* write header with correct signal
					   length and checksums */
    }

    wfdbquit();
    exit(0);	/*NOTREACHED*/
}

int getcvec(v)
int *v;
{
    int x, y;
    static int v0, v1;

    if ((x = getchar()) == EOF)
	return (0);
    if ((y = getchar()) == EOF) {
	(void)fprintf(stderr, "unexpected EOF in input file\n");
	return (0);
    }

    if ((x & 0x80) == 0) {
	v0 += (x & 0x7f);
	if (x & 0x40) v0 -= 128;
	v1 += y;
	if (y & 0x80) v1 -= 256;
    }
    else {
	if (x & 0x40) x |= ~(0x3f);
	else x &= 0x7f;
	v0 += (x << 8) + y;
	if ((x = getchar()) == EOF || (y = getchar()) == EOF) {
	    (void)fprintf(stderr, "unexpected EOF in input file\n");
	    return (0);
	}
	if (x & 0x80) x |= ~(0xff);
	v1 += (x << 8) + y;
    }

    v[0] = v0; v[1] = v1;
    return (2);
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
    "usage: %s -i AHAFILE [ OPTIONS ... ]\n",
    "where AHAFILE is the name of the AHA-format input signal file,",
    "and OPTIONS may include:",
    " -c         process a compressed input file (default if AHAFILE ends",
    "             with .CMP or .cmp;  otherwise old AHA tape format assumed)",
    " -f TIME    begin at specified TIME (default: 0, i.e., the beginning",
    "             of the input file)",
    " -h         print this usage summary",
    " -r RECORD  specify the name of the output RECORD (default: use the,"
    "            input file name, less path info and extension, as RECORD)",
    " -t TIME    stop at specified TIME (default: 35 minutes after the",
    "             starting time if RECORD=x2xx or x3xx, 3 hours if",
    "             RECORD=x0xx or x1xx, or the end of the input file)",
    NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
