/* file: psfd.c		G. Moody         9 August 1988
			Last revised:	21 November 2013

-------------------------------------------------------------------------------
psfd: Produces annotated full-disclosure ECG plots on a PostScript device
Copyright (C) 1988-2013 George B. Moody

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

Note:  This program necessarily deals with several different coordinate
systems, principally the natural WFDB coordinates (time in sample intervals
or seconds, amplitude in adus or millivolts), the printer coordinates
(pixels), and the page coordinates (mostly metric, with occasional English
dimensions and printer's points added for entertainment).  If you change
anything, check your units!

PostScript devices do not readily divulge their resolution, so the "printer
coordinates" used here are a convenient fiction which correspond to the true
printer coordinates only if `dpi' (see below) has been set correctly.  Scales
will be correct even if `dpi' is incorrect, however.

The ECG signals are low-pass filtered and decimated so that the plotted
distance between successive output samples is greater than or approximately
equal to the distance between pixels (as indicated by `dpi').  The filtering
improves the legibility of the plots, and the decimation not only speeds the
output but is necessary for most PostScript devices in order to avoid running
out of memory.
*/

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

/* The ANSI C function strstr is defined here for those systems that don't
   include it in their libraries.  This includes all older (pre-ANSI) C
   libraries;  some modern non-ANSI C libraries (notably those supplied with
   SunOS 4.1) do have strstr, so we can't just make this conditional on
   __STDC__. */
#ifdef sun
#ifdef i386
#define NOSTRSTR
#endif
#endif

#ifdef NOSTRSTR
char *strstr(s1, s2)
char *s1, *s2;
{
    char *p = s1;
    int n;

    if (s1 == NULL || s2 == NULL || *s2 == '\0') return (s1);
    n = strlen(s2);
    while ((p = strchr(p, *s2)) && strncmp(p, s2, n))
	p++;
    return (p);
}
#endif

/* PROLOG is the pathname of the accompanying prolog file, `psfd.pro', which
   should normally be accessible to this program at run-time.  In most cases,
   PROLOG should be an absolute pathname (beginning from the root directory),
   so that `psfd' can be used from any directory.  If PROLOG is inaccessible,
   the built-in prolog (see dprolog[], below) is used instead.		*/
#ifndef PROLOG
#define PROLOG	"/usr/local/lib/ps/psfd.pro"
#endif

/* COPYR is the default copyright notice printed at the bottom of each page,
   with `%d' replaced by the current year obtained from the system clock. */
#define COPYR "Massachusetts Institute of Technology %d. All rights reserved."

/* Configuration parameters (changeable only by recompiling) */
#define DPI	 300.0		/* default printer resolution (dots/inch) */
#define LWMM	   0.2		/* default line width (mm) */
#define TSCALE	   2.5		/* default time scale (mm/second) */
#define VSCALE	   1.0		/* default voltage scale (mm/millivolt) */
#define TPS	   1.0		/* grid ticks per second */
#define L_SEP	   1.5		/* distance from labels to sides of grid */
#define FS_ANN	   4.5		/* font size (in PostScript points) for
				   annotations */
#define FS_LABEL   6.0		/* font size for labels */
#define FS_TITLE  10.0		/* font size for title */

#ifndef PTYPE
#define PTYPE	 "letter"	/* default page type */
#endif

char *month[12] = { "January", "February", "March", "April", "May",
     "June", "July", "August", "September", "October", "November", "December"};

char *ptype = PTYPE;		/* page type */
double p_height;		/* page height (mm) */
double p_width;			/* page width (mm) */
double s_defwidth;		/* default strip width (mm) */
double tmargin;			/* top margin (mm) */
double bmargin;			/* bottom margin (mm) */
double lmargin;			/* left margin (mm) */
double rmargin;			/* right margin (mm) */
double title_y;			/* distance from page bottom to title (mm) */
double footer_y;		/* distance from page bottom to footer (mm) */
double imargin;			/* inside margin (mm) */
double omargin;			/* outside margin (mm) */

/* User-settable parameters */
char aname[41] = "atr";		/* annotator name */
char aname2[41] = "";		/* second annotator name */
int aux_shorten = 0;		/* if non-zero, print first char of aux only */
double boff = 0.;		/* binding offset (mm) */
int Cflag = 0;			/* if non-zero, produce color output */
char *copyright;		/* copyright notice string */
char *defpagetitle;		/* default page title */
char *rdpagetitle;		/* page title based on recording date */
double dpi = DPI;		/* pixels per inch */
int eflag = 0;			/* if non-zero, do even/odd page handling */
int Eflag = 0;			/* generate EPSF */
int gflag = 0;			/* if non-zero, plot grid */
int lflag = 0;			/* if non-zero, label signals */
int Lflag = 0;			/* if non-zero, use landscape orientation */
double lwmm = LWMM;		/* line width (mm); 0 is narrowest possible */
int mflag = 0;			/* if non-zero, margins specified using -m */
int Mflag = 0;			/* annotation/marker bar mode (0: do not print
				   bars, print mnemonics at center; 1: print
				   bars across all signals, mnemonics at
				   center; 2: print bars across attached
				   signal, mnemonics at center; 3: print bars
				   across attached signal, mnemonics above
				   bars */
int Nflag = 0;			/* if non-zero, print counter values */
int numberpages = 1;		/* if zero, suppress page numbering */
int nosig = 0;			/* number of signals to be printed */
int nomax = 0;			/* largest value for nosig seen so far */
int page = 1;			/* logical page number */
int pages_written = 0;		/* number of pages written already */
char *pagetitle = NULL;		/* if not null, title for page header */
char *pname;			/* the name by which this program is invoked */
int rflag = 0;			/* if non-zero, print record names */
int sflag = 0;			/* if non-zero, a signal list was specified */
int *siglist;			/* list of signals to be printed */
int smode = 1;			/* scale mode (0: no scales; 1: mm/unit in
				   footers; 2: units/tick in footers) */
char **snstr;			/* signal names (if provided on command line) */
char *sqstr;			/* signal quality string (1 char/signal) */
double t_hideal = 7.5;		/* ideal value for t_height (see below) */
double tps = TPS;		/* grid ticks per second */
double tscale = TSCALE;		/* time scale (mm/second) */
int tsmode = 2;			/* time stamp mode (0: no time stamps; 1:
				   elapsed times only; 2: absolute times if
				   defined, elapsed times otherwise) */
int uflag = 0;			/* if non-zero, insert an extra `%!' at the
				   beginning of the output to work around a
				   bug in the Adobe TranScript package */
int vflag = 0;			/* if non-zero, echo commands */
double vscale = VSCALE;		/* voltage scale (mm/millivolt) */
int xflag = 0;			/* if non-zero, extend last strip if needed */

double l_sep = L_SEP;		/* distance from labels to sides of grid */
double fs_ann = FS_ANN;		/* font size (in PostScript points) for
				   annotations */
double fs_label = FS_LABEL;	/* font size for labels */
double fs_title = FS_TITLE;	/* font size for titles */

/* Color definitions. */
struct pscolor {
    float red, green, blue;
};
struct pscolor ac = { 0.0, 0.0, 1.0 };	/* annotations: blue */
struct pscolor gc = { 1.0, 0.5, 0.5 };	/* grid: light red */
struct pscolor lc = { 0.0, 0.0, 0.0 };	/* labels: black */
struct pscolor sc = { 0.0, 0.0, 0.3 };	/* signals: dark blue */

char *prog_name(), *timcstr();
int printstrip(), setpagedim(), setpagetitle();
void append_scale(), cont(), ejectpage(), flush_cont(), grid(), help(),
    label(), larger(), move(), newpage(), plabel(), process(), rlabel(),
    rtlabel(), setbar1(), setbar2(), setitalic(), setmargins(), setroman(),
    setrgbcolor(), setsans(), smaller(), tlabel();

main(argc, argv)
int argc;
char *argv[];
{
    char *p, *getenv();
    FILE *cfile = NULL;
    int i, j;
    struct pscolor *colorp;
    struct tm *now;
    time_t t, time();

    t = time((time_t *)NULL);    /* get current time from system clock */
    now = localtime(&t);

    pname = prog_name(argv[0]);

    /* Read the calibration file to get standard scales. */
    (void)calopen((char *)NULL);

    /* Set the default page title. */
    defpagetitle = pagetitle = (char *)malloc(26);
    (void)sprintf(defpagetitle, "Printed %d %s %d",
		  now->tm_mday, month[now->tm_mon], now->tm_year + 1900);

    /* Set the default copyright notice string. */
    copyright = (char *)malloc((unsigned)strlen(COPYR) + 3);
    (void)sprintf(copyright, COPYR, now->tm_year + 1900);

    /* Set the default page dimensions and margins. */
    (void)setpagedim();
    setmargins();
    
    /* Set other defaults (see descriptions above). */
    if ((p = getenv("DPI")) && (dpi = atof(p)) <= 0.0) dpi = DPI;
    if ((p = getenv("LWMM")) && (lwmm = atof(p)) < 0.0) lwmm = LWMM;
    if ((p = getenv("TSCALE")) && (tscale = atof(p)) <= 0.0) tscale = TSCALE;
    if ((p = getenv("VSCALE")) && (vscale = atof(p)) <= 0.0) vscale = VSCALE;
    if ((p = getenv("TPS")) && (tps = atof(p)) <= 0.0) tps = TPS;
    if (p = getenv("L_SEP")) l_sep = atof(p);
    if ((p = getenv("FS_ANN")) && (fs_ann = atof(p)) <= 0.0) fs_ann = FS_ANN;
    if ((p = getenv("FS_LABEL"))&&(fs_label=atof(p)) <= 0.0) fs_label=FS_LABEL;
    if ((p = getenv("FS_TITLE"))&&(fs_title=atof(p)) <= 0.0) fs_title=FS_TITLE;
    if (p = getenv("PTYPE")) ptype = p;
 
    /* Check for buggy TranScript software. */
    if (getenv("TRANSCRIPTBUG")) uflag = 1;

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* specify annotator */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -a\n",
			argv[0]);
		exit(1);
	    }
	    /* workaround for MS-DOS command.com bug */
	    if (strcmp(argv[i], " ") == 0) *argv[i] = '\0';
	    (void)strncpy(aname, argv[i], 40);
	    break;
	  case 'A':	/* specify second annotator */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -A\n",
			argv[0]);
		exit(1);
	    }
	    if (aname[0] == '\0') (void)strncpy(aname, argv[i], 40);
	    else (void)strncpy(aname2, argv[i], 40);
	    break;
	  case 'b':	/* specify binding offset in mm */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: binding offset (mm) must follow -b\n",
			argv[0]);
		exit(1);
	    }
	    boff = atof(argv[i]);
	    break;
	  case 'c':	/* specify alternate copyright notice */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: copyright notice string must follow -c\n",
			argv[0]);
		exit(1);
	    }
	    if (*copyright) free(copyright);
	    /* workaround for MS-DOS command.com bug */
	    if (strcmp(argv[i], " ") == 0) *argv[i] = '\0';
	    if (*argv[i]) {
		copyright = (char *)malloc((unsigned)strlen(argv[i]) + 3);
		(void)sprintf(copyright, argv[i], now->tm_year + 1900);
	    }
	    else
		copyright = "";
	    break;
	  case 'C':     /* set a color */
	    switch (argv[i][2]) {
	    case 'a':	colorp = &ac; break;
	    case 'g':	colorp = &gc; break;
	    case 'l':	colorp = &lc; break;
	    case 's':	colorp = &sc; break;
	    case '\0':  break;
	    default:
		(void)fprintf(stderr,
			      "%s: unrecognized color specification '%s'\n",
			      pname, argv[i]);
		exit(1);
	    }
	    if (argv[i][2]) {
	      if (i >= argc+3) {
	         (void)fprintf(stderr, "%s: RGB triplet must follow '%s'\n",
			       pname, argv[i]);
		 exit(1);
	      }
	      if ((colorp->red = atof(argv[++i]))<0.0 || colorp->red   > 1.0 ||
		  (colorp->green=atof(argv[++i]))<0.0 || colorp->green > 1.0 ||
		  (colorp->blue =atof(argv[++i]))<0.0 || colorp->blue  > 1.0) {
		  (void)fprintf(stderr,
			   "%s: RGB values must be between 0 (black) and 1\n",
				pname);
		  exit(1);
	      }
	    }
	    Cflag = 1;
	    break;
	  case 'd':	/* specify printer resolution in dpi */
	    if (++i >= argc || ((dpi = atof(argv[i])) <= 0)) {
		(void)fprintf(stderr,
			      "%s: printer resolution (dpi) must follow -d\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  case 'e':	/* enable special even/odd page handling */
	    eflag = 1;
	    break;
	  case 'E':	/* generate EPSF (encapsulated PostScript format) */
	    Eflag = 1;
	    break;
	  case 'g':	/* enable grid printing */
	    gflag = 1;
	    break;
	  case 'G':	/* enable alternate grid */
	    gflag = -1;
	    break;
	  case 'h':    	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* set ideal trace height in mm */
	    if (++i >= argc || ((t_hideal = atof(argv[i])) <= 0)) {
		(void)fprintf(stderr,
			      "%s: trace height (mm) must follow -H\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  case 'l':	/* enable signal labelling */
	    lflag = 1;
	    break;
	  case 'L':	/* specify landscape mode */
	    Lflag = 1;
	    (void)setpagedim();
	    if (!mflag) setmargins();
	    break;
	  case 'm':	/* specify margins */
	    if (++i >= argc - 3 || *argv[i] == '-' ||
		*argv[i+1] == '-' || *argv[i+2] == '-' || *argv[i+3] == '-') {
		(void)fprintf(stderr, "%s: margins must follow -m\n", argv[0]);
		exit(1);
	    }
	    imargin = atof(argv[i++]);
	    omargin = atof(argv[i++]);
	    tmargin = atof(argv[i++]);
	    bmargin = atof(argv[i]);
	    s_defwidth = 25.0 * (int)((p_width - (imargin + omargin))/25.0);
	    title_y = p_height - 0.6*tmargin;
	    footer_y = bmargin > 20.0 ? bmargin - 10.0 : bmargin*0.5;
	    mflag = 1;
	    break;
	  case 'M':	/* print marker bars with each annotation */
	    if (argv[i][2]) {
		Mflag = atoi(argv[i]+2);
		if (Mflag < 0 || Mflag > 3) {
		    (void)fprintf(stderr,
			  "%s: incorrect format (%d) specified after -M\n",
				  pname, Mflag);
		    exit(1);
		}
	    }
	    else
		Mflag = 1;
	    break;
	  case 'n':	/* specify first page number */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: page number must follow -n\n", argv[0]);
		exit(1);
	    }
	    page = atoi(argv[i]);
	    numberpages = (page > 0);
	    break;
	  case 'N':	/* print counter values */
	    Nflag = 1;
	    break;
	  case 'P':	/* set page size */
	    if (++i >= argc || (ptype = argv[i], setpagedim() == 0)) {
		(void)fprintf(stderr,
			      "%s: page size specification must follow -P\n",
			      argv[0]);
		exit(1);
	    }
	    if (!mflag)
		setmargins();
	    break;
	  case 'r':	/* enable/disable record name printing */
	  case 'R':
	    rflag = 1 - rflag;
	    break;
	  case 's':	/* specify signals to be printed */
	    sflag = 1;
	    /* count the number of output signals (arguments not beginning with
	       a hyphen, and not including the last argument, which must be
	       the name of the script) */
	    for (j = i+1; j < argc-1 && *argv[j] != '-'; j++)
		;
	    if (j == i+1) {
		(void)fprintf(stderr, "%s: signal list must follow -s\n",
			pname);
		exit(1);
	    }
	    /* allocate storage for the signal list, snstr, and sqstr */
	    if (j - i > nomax) {
		if ((siglist = realloc(siglist, (j-i)*sizeof(int))) == NULL ||
		    (snstr = realloc(sqstr, (j-i+1)*sizeof(char *))) == NULL ||
		    (sqstr = realloc(sqstr, (j-i+1)*sizeof(char))) == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
	    }
	    /* fill the signal list */
	    nosig = 0;
	    while (++i < j)
		snstr[nosig++] = argv[i];
	    i--;
	    break;
	  case 'S':	/* set modes for scale and time stamp printing */
	    if (i >= argc-2 ||
		*argv[++i] == '-' ||  (smode = atoi(argv[i])) > 2 ||
		*argv[++i] == '-' || (tsmode = atoi(argv[i])) > 2) {
		(void)fprintf(stderr,
			"%s: scale and time stamp modes must follow -S\n",
			argv[0]);
		exit(1);
	    }
	    break;
	  case 't':	/* specify time scale */
	    if (argv[i][2] == 'm') {
		if (++i >= argc || ((tscale = atof(argv[i])) <= 0)) {
		    (void)fprintf(stderr,
				  "%s: time scale (mm/min) must follow -tm\n",
				  argv[0]);
		    exit(1);
		}
		tscale /= 60.;
	    }
	    else if (++i >= argc || ((tscale = atof(argv[i])) <= 0)) {
		(void)fprintf(stderr,
			      "%s: time scale (mm/sec) must follow -t\n",
			      argv[0]);
		exit(1);
	    }
	    break;
	  case 'T':	/* specify page title */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: page title must follow -T\n", argv[0]);
		exit(1);
	    }
	    if (pagetitle != defpagetitle)
		free(pagetitle);
	    /* workaround for MS-DOS command.com bug */
	    if (strcmp(argv[i], " ") == 0) *argv[i] = '\0';
	    pagetitle = (char *)malloc((unsigned)strlen(argv[i])+1);
	    (void)strcpy(pagetitle, argv[i]);
	    break;
	  case 'u':	/* produce `unstructured PostScript' */
	    uflag = 1;
	    break;
	  case 'v':	/* specify voltage scale */
	    if (++i >= argc || ((vscale = atof(argv[i])) <= 0)) {
		(void)fprintf(stderr,
			      "%s: voltage scale (mm/mV) must follow -v\n",
			argv[0]);
		exit(1);
	    }
	    break;
	  case 'V':	/* enable verbose mode */
	    vflag = 1;
	    break;
	  case 'w':	/* specify line width in mm */
	    if (++i >= argc || ((lwmm = atof(argv[i])) < 0.0)) {
		(void)fprintf(stderr,
			      "%s: line width (mm) must follow -w\n",
			pname);
		exit(1);
	    }
	    break;
	  case 'x':	/* extend last strip if needed */
	    xflag = 1;
	    break;
	  case '1':	/* abbreviate aux strings to one character on output */
	    aux_shorten = 1;
	    break;
	  case '\0':	/* read commands from standard input */
	    process(cfile = stdin);
	    break;
	}
	else if ((cfile = fopen(argv[i], "r")) == NULL) {
	    (void)fprintf(stderr, "%s: can't read %s\n", argv[0], argv[i]);
	    exit(1);
	}
	else {
	    process(cfile);
	    (void)fclose(cfile);
	}
    }
    if (argc < 2 || cfile == NULL) {
	help();
	exit(1);
    }
    ejectpage();
    if (!uflag) {
	(void)printf("%%%%Trailer\n");
	(void)printf("%%%%Pages: %d\n", pages_written);
    }
    exit(0);	/*NOTREACHED*/
}

/* Parameters set from the WFDB header file */
int nisig;		/* number of signals in current record */
int nimax;		/* largest value for nisig seen so far */
double sps;		/* sampling frequency (samples/second/signal) */
WFDB_Siginfo *s;	/* signal parameters, including gain */
int *uncal;		/* if non-zero, signal is uncalibrated */

/* Arrays indexed by signal # (allocated by process(), used by printstrip()) */
int *accept, *buflen, *v, *vbase, **vbuf, *vmax, *vmin, *vn;
long *vs, *vsum;

/* Derived parameters */
int decf;		/* decimation factor (input samples/output sample) */
double dpmm;		/* pixels per millimeter */
double dppt;		/* pixels per PostScript "printer's point" (PostScript
			   "printer's points" are 1/72 inch;  true printer's
			   points are 1/72.27 inch) */
double dpsi;		/* pixels per sample interval */
long nisamp;		/* number of input samples/signal/strip */
int nosamp;		/* number of output samples/signal/strip */
double sdur;		/* strip duration in seconds */
double nticks;		/* number of ticks on grid (see grid() below) */

int aflag = 0;	       	/* if non-zero, plot annotations */
unsigned int nann = 0;	/* number of annotation files to be plotted (0-2) */
int pright = 1;		/* if non-zero, print time, etc., in right margin */
char *scales;		/* string which describes time and voltage scales */
char record[80];	/* current record name */
int usflag = 0;		/* if non-zero, uncalibrated signals were plotted */

#define mm(A)	((int)((A)*dpmm))    /* convert millimeters to pixels */

/* The rhpage() macro is true for all right-hand pages (normally the odd-
   numbered pages).  It is true for even-numbered pages if even/odd page
   handling is disabled. */
#define rhpage()	((eflag == 0) || (page & 1))

/* Read and execute commands from a file. */
void process(cfile)
FILE *cfile;
{
    char *strtok();
    int i;
    long t0, t1, tt;
    static char combuf[256];
    static char *rstring, *tstring, *tstring2;
    static WFDB_Anninfo af[2] = { { aname, WFDB_READ },
				  { aname2,WFDB_READ } };

    if (scales == NULL && (scales = (char *)malloc(40)) == NULL) return;
    if (smode & 1) {
	if (gflag >= 0)
	    (void)sprintf(scales, "%g mm/sec", tscale);
	else
	    (void)sprintf(scales, "%g mm/min", tscale*60.0);
	if (Eflag) {
	    static int warned;

	    if (!warned) {
		(void)fprintf(stderr,
		    "Warning: printed scales will not be correct if\n");
		(void)fprintf(stderr,
		    "%s output is resized by an embedding document!\n", pname);
		(void)fprintf(stderr,
		    "You may wish to rerun %s with option `-S %d %d'\n",
			pname, smode+1, tsmode);
		warned = 1;
	    }
	}
    }
    else
	(void)sprintf(scales, "Grid intervals: %g sec", 1.0/tps);
    dpmm = dpi/25.4; dppt = dpi/72.;
    sdur = s_defwidth/tscale;
    lmargin = rhpage() ? imargin + boff : omargin - boff;
    rmargin = rhpage() ? omargin - boff : imargin + boff;
    while (fgets(combuf, 256, cfile)) {	/* read a command */
	if (vflag) (void)fprintf(stderr, "%s", combuf);
	if (*combuf == '\n') {
	    if (pages_written > 0) ejectpage();
	}
	else {
	    rstring = strtok(combuf, " \t\n");
	    tstring = strtok((char *)NULL, " \t\n");
	    if (rstring == NULL)
		continue;
	    (void)strcpy(record, rstring);
	    if ((nisig = isigopen(record, NULL, 0)) < 1)
	        continue;
	    if (nisig > nimax &&
		(s = realloc(s, nisig * sizeof(WFDB_Siginfo))) == NULL) {
		(void)fprintf(stderr, "%s: insufficient memory\n", pname);
		exit(2);
	    }
	    if (isigopen(record, s, nisig) != nisig)
		continue;
	    if (!sflag) {
		if ((siglist == NULL || nisig > nosig) &&
		    ((siglist=realloc(siglist,nisig*sizeof(int))) == NULL)) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		for (i = 0; i < nisig; i++)
		    siglist[i] = i;
		nosig = nisig;
	    }
	    else {
		for (i = 0; i < nosig; i++) {
		    if ((siglist[i] = findsig(snstr[i])) < 0) {
			(void)fprintf(stderr,
				      "record %s doesn't have a signal '%s'\n",
				      record, snstr[i]);
			wfdbquit();
			return;
		    }
		}
	    }
	    if (nisig > nimax) {
		if ((uncal = realloc(uncal, nisig * sizeof(int))) == NULL ||
		    (v = realloc(v, nisig * sizeof(int))) == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		nimax = nisig;
	    }
	    for (i = 0; i < nisig; i++)
		uncal[i] = 0;
	    if (nosig > nomax) {
		if ((accept = realloc(accept, nosig * sizeof(int))) == NULL ||
		    (buflen = realloc(buflen, nosig * sizeof(int))) == NULL ||
		    (vbase = realloc(vbase, nosig * sizeof(int))) == NULL ||
		    (vmax = realloc(vmax, nosig * sizeof(int))) == NULL ||
		    (vmin = realloc(vmin, nosig * sizeof(int))) == NULL ||
		    (vn = realloc(vn, nosig * sizeof(int))) == NULL ||
		    (vs = realloc(vs, nosig * sizeof(long))) == NULL ||
		    (vsum = realloc(vsum, nosig * sizeof(long))) == NULL ||
		    (vbuf = realloc(vbuf, nosig * sizeof(int *))) == NULL) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		if (!sflag &&
		    ((siglist = realloc(siglist,nosig*sizeof(int))) == NULL ||
		     (sqstr =realloc(sqstr,(nosig+1)*sizeof(char))) == NULL)) {
		    (void)fprintf(stderr, "%s: insufficient memory\n", pname);
		    exit(2);
		}
		while (nomax < nosig) {
		    vbuf[nomax] = NULL;
		    buflen[nomax++] = 0;
		}
	    }
	    (void)setpagetitle(0L);
	    if ((sps = sampfreq((char *)NULL)) <= 0.) sps = WFDB_DEFFREQ;
	    dpsi = dpmm * tscale / sps;
 	    nisamp = (int)(sps*sdur);
	    if ((decf = (int)(nisamp/mm(s_defwidth) + 0.5)) < 1) decf = 1;
	    nosamp = nisamp/decf;
	    if (aname2[0]) nann = 2;
	    else if (aname[0]) nann = 1;
	    else aflag = nann = 0;
	    if (nann)
		aflag = (annopen(record, af, nann) == 0);
	    if (tstring == NULL) t0 = t1 = 0L;
	    else {
		tstring = strtok(tstring, "-");
		tstring2 = strtok((char *)NULL, "-");
		if ((t0 = strtim(tstring)) < 0L) t0 = -t0;
		if (tstring2) {
		    if ((t1 = strtim(tstring2)) < 0L) t1 = -t1;
		    if (t1 != 0L && t1 < t0) { tt = t0; t0 = t1; t1 = t0; }
		}
		/* If no stop time is specified, plot to the end of the record.
		   Note that strtim("e") is either the time of the end of the
		   record, or zero (if the number of samples is not specified
		   in the header file). */
		else t1 = strtim("e");
	    }
	    while (t1 == 0L || t0 < t1) {
		if ((tt = t0 + nisamp) > t1 && t1 > 0L) tt = t1;
		if (xflag && 0L < t1 - tt && t1 - tt <= nisamp/10) {
		    pright = 0;
		    nisamp = t1 - t0;
		    nosamp = nisamp/decf;
		    (void)printstrip(t0, t1);
		    pright = 1;
		    break;
		}
		if (printstrip(t0, tt) == 0) break;
		if (((t0 += nisamp) < t1 || t1 == 0L) && vflag)
		    (void)fprintf(stderr, " %s\n", timstr(t0));
	    }
	    wfdbquit();
	}
    }
}

static double __mt;	/* temporary variable for adu macro */
#define adu(A)	((__mt=(A)*dpadu), (int)(__mt>=0 ? __mt+0.5 : __mt-0.5))
#define si(A)	((int)((A)*dpsi))    /* convert sample intervals to pixels */
#define pt(A)	((int)((A)*dppt))    /* convert PostScript points to pixels */

double dpadu;	/* pixels per adu.  This quantity must be recalculated for each
		   signal;  all signals are plotted to the same scale in mm/mV,
		   even though gains (in adu/mV) may vary among signals of the
		   same record. */

/* Convert time argument to counter value and format as a string. */
char *timcstr(t)
long t;
{
    static char cstring[10];
    static double basecount, cfreq = -1.;

    if (cfreq < 0) {
	basecount = getbasecount();
	if ((cfreq = getcfreq()) <= 0.) cfreq = 1.0;
	cfreq = sps/cfreq;
    }
    if (t < 0L) t = -t;
    (void)sprintf(cstring, "%g", t/cfreq + basecount);
    return (cstring);
}

/* Position and dimensions (mm) of the strip to be printed.  The position is
   measured from the lower left corner of the page. */
double s_top = -9999., s_bot, s_left, s_right, s_height, s_width;
double t_height;	/* height (mm) of space actually alloted per trace */

/* Print a strip beginning at sample t0, ending at sample t1-1.  This function
   returns 2 if completely successful, 1 if the signals were plotted but not
   the annotations, or 0 if nothing was printed. */

int printstrip(t0, t1)
long t0, t1;
{
    char *ts;
    double curr_s_top;
    int i, k;
    long j, jmax;
    int nstrips, tm_y, tt, ttmax, *vp, x0, y0, ya[2];

    /* Allocate buffers for the samples to be plotted, and initialize the
       range and filter variables. */
    for (i = 0; i < nosig; i++) {
	accept[i] = vn[i] = 0;
	vmax[i] = vmin[i] = WFDB_INVALID_SAMPLE; vs[i] = 0L; vsum[i] = 0L;
	if (nosamp > buflen[i]) {
	    if ((vbuf[i] = realloc(vbuf[i], nosamp * sizeof(int))) == NULL) {
		buflen[i] = 0;
		(void)fprintf(stderr, "insufficient memory\n");
		return (0);
	    }
	    buflen[i] = nosamp;
	}
    }

    if ((jmax = t1 - t0) > nisamp) jmax = nisamp;
    if (nosig > 0) {
	/* Fill the buffers. */
	if (isigsettime(t0) < 0) return (0);

	for (j = 0L, k = 1, tt = 0; j < jmax && getvec(v) >= 0; j++) {
	    for (i = 0; i < nosig; i++) {
		int vtmp = v[siglist[i]];

		/* add up all valid samples in each group of decf samples */
		if (vtmp != WFDB_INVALID_SAMPLE) {
		    vs[i] += vtmp;
		    vn[i]++;
		}
		if (k >= decf) {
		    /* average the valid samples in each group */
		    if (vn[i] > 0) {
			vsum[i] += vbuf[i][tt] = vtmp = vs[i]/vn[i];
			if (vtmp > vmax[i] || vmax[i] == WFDB_INVALID_SAMPLE)
			    vmax[i] = vtmp;
			else if (vtmp<vmin[i] || vmin[i]==WFDB_INVALID_SAMPLE)
			    vmin[i] = vtmp;
			vn[i] = vs[i] = 0;
			accept[i]++;
		    }
		    else
			vbuf[i][tt] = WFDB_INVALID_SAMPLE;
		}
	    }
	    if (++k > decf) {
		k = 1;
		tt++;
	    }
	}
	if (j == 0) return (0);
	jmax = j;


	if (j == 0L) return (0);
	if (j == jmax - 1) j++;
	/* reached end of record exactly at end of strip -- adjust j */
	t1 = t0 + j;
	ttmax = tt;
	if (nticks < j/sps) nticks = j/sps;
	
	/* Calculate the midranges. */
	for (i = 0; i < nosig; i++) {
	    int sig = siglist[i], vb, vm, vsm;
	    double w;
	    
	    if (accept[i]) {
		vsm = vsum[i]/accept[i];
		vb = (vmax[i] + vmin[i])/2;
		if (vb > vsm && vmax[i] != vsm)
		    w = (vb - vsm)/(vmax[i] - vsm);
		else if (vb < vsm && vmin[i] != vsm)
		    w = (vsm - vb)/(vsm - vmin[i]);
		else w = 0.0;
		vbase[i] = vsm + ((double)vb-vsm)*w;
	    }
	    else	/* no valid samples in this trace */
		vbase[i] = 0;
	}

	/* Determine the width and height of the strip. */
	s_width = j*tscale/sps;
	/* The calculation of nstrips allots roughly t_hideal (7.5 by default)
	   mm per trace.  The formula is derived from the desired results: 30
	   for nosig = 1, 15 for nosig = 2, 10 for nosig = 3, 8 for nosig = 4,
	   6 for nosig = 5, etc., given the default paper size and margins. */
	nstrips = (nosig<=16) ?
	    (int)((p_height-tmargin-bmargin)/(t_hideal*nosig)+0.45) : 1;
	s_height = (p_height - (tmargin+bmargin)) / nstrips;
	if (nosig > 2 || nann > 1) s_height -= s_height / (nosig+1);
	t_height = s_height / nosig;
	
	/* Decide where to put the strip.  Usually, this is directly below the
	   previous strip, and s_top will have been set correctly after the
	   previous strip was printed.  If the strip won't fit in the available
	   space, though, go to the top of the next page. */
	if (s_top - s_height < bmargin - 2.0) {
	    setpagetitle(t0);
	    ejectpage();
	    newpage();
	    s_top = p_height - tmargin;
	    nticks = j/sps;
	}
	s_bot = s_top - s_height;
	s_left = lmargin;
	s_right = s_left + s_width;
	
	/* If signal labels are to be printed, and nosig is odd, print the time
	   markers at the bottom of the strip (so they won't interfere with the
	   signal labels).  Otherwise, print them midway between top and
	   bottom. */
	setrgbcolor(&lc);
	setroman(fs_title * 0.8);
	if (lflag && (nosig & 1)) tm_y = mm(s_bot) - pt(fs_title * 0.2);
	else tm_y = mm(s_top + s_bot)/2 - pt(fs_title * 0.4);
	if (tsmode == 1) {	/* print elapsed times only */
	    if (t0) {
		ts = mstimstr(t0);
		while (*ts == ' ') ts++;
		/* Print milliseconds only if t0 is not an exact multiple of 1
		   second. */
		if (strcmp(ts + strlen(ts)-4, ".000") == 0)
		    *(ts + strlen(ts)-4) = '\0';
	    }
	    else
		ts = "0:00";
	    move(mm(s_left - l_sep), tm_y);
	    if (Nflag) {
		rlabel(")");
		rlabel(timcstr(t0));
		rlabel(" (");
	    }
	    rlabel(ts);
	    if (pright) {
		ts = mstimstr(t1);
		while (*ts == ' ') ts++;
		if (strcmp(ts + strlen(ts)-4, ".000") == 0)
		    *(ts + strlen(ts)-4) = '\0';
		move(mm(s_right + l_sep), tm_y); label(ts);
	    }
	}
	else if (tsmode == 2) { /* print absolute times (but not date) if
				   defined, elapsed times otherwise */
	    char *p;
	    
	    ts = mstimstr(-t0);
	    while (*ts == ' ') ts++;
	    for (p = ts; *p != '.'; p++)
		;
	    if (strncmp(p, ".000", 4))
		p += 4;
	    if (*ts == '[')
		*p++ = ']';
	    *p = '\0';
	    move(mm(s_left - l_sep), tm_y);
	    if (Nflag) {
		rlabel(")");
		rlabel(timcstr(t0));
		rlabel(" (");
	    }
	    rlabel(ts);
	    if (pright) {
		ts = mstimstr(-t1);
		while (*ts == ' ') ts++;
		for (p = ts; *p != '.'; p++)
		    ;
		if (strncmp(p, ".000", 4))
		    p += 4;
		if (*ts == '[')
		    *p++ = ']';
		*p = '\0';
		move(mm(s_right + l_sep), tm_y); label(ts);
	    }
	}
	
	/* Draw the signals. */
	x0 = mm(s_left);
	if (lflag) setitalic(fs_label);
	for (i = 0; i < nosig; i++) {
	    int last_sample_valid = 0, sig = siglist[i];
	    static WFDB_Calinfo ci;
	    
	    setrgbcolor(&lc);
	    y0 = mm(s_top - (i + 0.5)*t_height);
	    if (s[sig].gain == 0.) {
		s[sig].gain = WFDB_DEFGAIN;
		usflag = uncal[sig] = 1;
	    }
	    if (lflag) {
		char *d = s[sig].desc, *t;
		
		if (strncmp(d, "record ", 7) == 0 &&
		    (t = strchr(d, ',')) != NULL)
		    d = t+1;
		move(mm(s_left - l_sep), y0 - pt(fs_label/2.));
		rlabel(d);
		if (uncal[sig]) rlabel("* ");
		if (pright) {
		    move(mm(s_right + l_sep), y0 - pt(fs_label/2.));
		    label(d);
		    if (uncal[sig]) label(" *");
		}
	    }
	    dpadu = dpmm * vscale / s[sig].gain;
	    if (getcal(s[sig].desc, s[sig].units, &ci) == 0 &&
		ci.scale != 0.0) {
		dpadu /= ci.scale;
		append_scale(ci.sigtype, ci.units, ci.scale);
	    }
	    /* Handle the common case of mV-dimensioned signals that are not in
	       the WFDB calibration database. */
	    else if (s[sig].units == NULL)
		append_scale("record", "mV", 1.0);
	    y0 -= adu(vbase[i]);
	    setrgbcolor(&sc);
	    for (tt = last_sample_valid=0, vp=vbuf[i]; tt < ttmax; tt++, vp++){
		if (*vp == WFDB_INVALID_SAMPLE)
		    last_sample_valid = 0;
		else if (last_sample_valid)
			cont(x0 + si(decf*tt), y0 + adu(*vp));
		else {
		    move(x0 + si(decf*tt), y0 + adu(*vp));
		    last_sample_valid = 1;
		}
	    }
	}
    }
    flush_cont();

    /* Print the annotations;  ya[0] and ya[1] are the baseline ordinates (in
       pixels) of annotation labels for annotators 0 and 1 respectively.
       There are several possibilities for positioning the annotations relative
       to the signals, as indicated schematically below:

		       annotator 0	signal 0
		       signal 0		annotator 0
		       [annotator 1]	signal 1
		       			[annotator 1]
					[signal 2]
					[signal 3]
					     .
					     .
					     .

       The scheme on the left is used for records with only one signal, and
       that on the right is used in other cases.  If two sets of annotations
       are to be printed for a record with one or two signals, or if there
       are three or more signals, additional space is allocated between the
       strips to separate them.  */
    if (nosig < 2) ya[0] = mm(s_top) - pt(fs_ann*2.);
    else ya[0] = mm(s_top - t_height) - pt(fs_ann/2.);
    if (Mflag) setbar1(mm(s_top), mm(s_bot));

    curr_s_top = s_top;
    if (nann > 1 && nosig < 3) {
	ya[1] = mm(s_bot) - pt(fs_ann/2.);
	s_bot -= t_height;
	s_top -= (s_height + t_height);
    }
    else {
	ya[1] = mm(s_top - 2*t_height) - pt(fs_ann/2.);
	if (nosig > 2) {
	    s_bot -= t_height;
	    s_top -= (s_height + t_height);
	}
	else
	    s_top -= s_height;
    }
    if (aflag) {
	static WFDB_Annotation annot;
	int x, y, c;
	unsigned int ia;

	if (iannsettime(t0) < 0 && nann == 1) return (1);
	setrgbcolor(&ac);
	setsans(fs_ann);
	for (ia = 0; ia < nann; ia++) {
	    if (Mflag <= 2)
		(void)printf("%d Ay\n", y = ya[ia]);
	    else
		c = -1;
	    while (getann(ia, &annot) >= 0 && annot.time < t1) {
		if (Mflag >= 2 && annot.chan != c) {
		    int i;
		    i = c = annot.chan;
		    if (i < 0 || i >= nosig) i = 0;
		    y = mm(curr_s_top - (i + 0.5)*t_height + 2.);
		    if (Mflag > 2)
			(void)printf("%d Ay\n", y);
		}
		if (Mflag) setbar2(y);   
		x = (int)(annot.time - t0);
		switch (annot.anntyp) {
		  case NOISE:
		    move(x0 + si(x), ya[ia] + pt(fs_ann * 1.1));
		    if (annot.subtyp == -1) { label("U"); break; }
		    /* The existing scheme is good for up to 4 signals;  it can
		       be easily extended to 8 or 12 signals using the chan and
		       num fields, or to an arbitrary number of signals using
		       `aux'. */
		    for (i = 0; i < nosig; i++) {
			int j = siglist[i];

			if (j > 3)
			    sqstr[i] = '*';	/* quality of input signal j
						   is undefined */
			else if (annot.subtyp & (0x10 << j))
			    sqstr[i] = 'u';	/* signal j is unreadable */
			else if (annot.subtyp & (0x01 << j))
			    sqstr[i] = 'n';	/* signal j is noisy */
			else
			    sqstr[i] = 'c';	/* signal j is clean */
		    }
		    sqstr[i] = '\0';
		    label(sqstr);
		    break;
		  case STCH:
		  case TCH:
		  case NOTE:
		    if (annot.time == 0L)
			break;		/* don't show modification records */
		    if (annot.aux == NULL || *annot.aux == 0)
			(void)printf("(%s) %d a\n",
				     annstr(annot.anntyp), x0+si(x));
		    else {
			move(x0 + si(x), y + pt(fs_ann * 1.1));
			if (aux_shorten && *annot.aux > 1)
			    *(annot.aux+2) = '\0'; /* print first char only */
			label(annot.aux+1);
		    }
		    break;
		  case RHYTHM:
		    if (annot.aux == NULL || *annot.aux == 0)
			(void)printf("(%s) %d a\n",
				     annstr(annot.anntyp), x0+si(x));
		    else {
			move(x0 + si(x), ya[ia] - pt(fs_ann * 1.1));
			label(annot.aux+1);
		    }
		    break;
		  case NORMAL:
		    (void)printf("%d A\n", x0 + si(x));
		    break;
                  case WFON:
		    (void)printf("(\\%s) %d a\n",
				 annstr(annot.anntyp), x0 + si(x));
		    break;
                  case WFOFF:
		    (void)printf("(\\%s) %d a\n",
				 annstr(annot.anntyp), x0 + si(x));
		    break;
		  default:
		    (void)printf("(%s) %d a\n",
				 annstr(annot.anntyp), x0 + si(x));
		    break;
		}
	    }
	}
    }
    return (2);
}

/* setpagedim sets p_height and p_width based on ptype.  Note that the values
   are in millimeters and refer to the imageable area (centered on the page),
   not to the physical size of the paper.  Except for `lwletter', the defined
   page sizes are those used by the Sun SPARCprinter. */
int setpagedim()
{
    if (strcmp(ptype, "A4") == 0) {
	p_width = 201.85;
	p_height = 289.31;
    }
    else if (strcmp(ptype, "A5") == 0) {
	p_width = 140.21;
	p_height = 201.85;
    }
    else if (strcmp(ptype, "B4") == 0) {
	p_width = 249.26;
	p_height = 356.28;
    }
    else if (strcmp(ptype, "B5") == 0) {
	p_width = 173.40;
	p_height = 249.26;
    }
    else if (strcmp(ptype, "legal") == 0) {
	p_width = 208.62;
	p_height = 348.15;
    }
    else if (strcmp(ptype, "legal13") == 0) {
	p_width = 208.62;
	p_height = 322.41;
    }
    else if (strcmp(ptype, "letter") == 0) {   /* SPARCprinter letter size */
	p_width = 208.62;
	p_height = 271.61;
    }
    else if (strcmp(ptype, "lwletter") == 0) { /* Apple LaserWriter letter */
	p_width = 203.20;
	p_height = 277.37;
    }
    else {
	p_width = p_height = 0.;
	(void)sscanf(ptype, "%lfx%lf", &p_width, &p_height);
	if (p_width < 20.0 || p_height < 20.0)
	    return (0);
    }
    if (Lflag && p_height > p_width) {	/* landscape: switch height & width */
	double temp;

	temp = p_height;
	p_height = p_width;
	p_width = temp;
    }
    return (1);
}

/* setpagetitle() sets a page title based on its time argument. */
int setpagetitle(t)
WFDB_Time t;
{
    int hours, minutes, seconds, days, months, year = -1;
    char *s;

    if (t > 0L) t = -t;
    s = timstr(t);
    if (rdpagetitle) free(rdpagetitle);
    rdpagetitle = NULL;
    if (*s != '[') return (0);
    (void)sscanf(s, "[%d:%d:%d %d/%d/%d]",
	   &hours, &minutes, &seconds, &days, &months, &year);
    if (year == -1 || (rdpagetitle = (char *)malloc(27)) == NULL) return (0);
    (void)sprintf(rdpagetitle, "Recorded %d %s %d", days,month[months-1],year);
    return (1);
}

/* setmargins() determines the default margins and the strip width from the
   page size. */
void setmargins()
{
    s_defwidth = 25.0 * ((int)((p_width - 50.0)/25.0));
    tmargin = (p_height >=  275.0) ? 25.0 : 20.0;
    bmargin = 25.0;
    imargin = omargin = (p_width - s_defwidth)/2.0;
    title_y = p_height - 0.6*tmargin;
    footer_y = 0.5*bmargin;
}

void append_scale(desc, units, scale)
char *desc;
char *units;
double scale;
{
    char *sctbuf, *scp;

    /* Do nothing if desc, units, or the scale string is null, if scale is 0,
       or if desc is already included in the scale string. */
    if (desc == NULL || units == NULL || scales == NULL || scale == 0. ||
	strstr(scales, desc))
	return;

    /* If desc is the default ("record ..."), do nothing if units is already
       in the scale string. */
    if (strcmp(desc, "record") == 0 && strstr(scales, units)) return;

    /* Otherwise append scale information to the end of the scale string. */
    if ((sctbuf = (char *)malloc((unsigned)(strlen(units)+strlen(desc)+20))) ==
	 NULL)
	return;
    /* Include desc in the scale string only if it is not "record". */
    if (strcmp(desc, "record")) {
	if (smode & 1)
	    (void)sprintf(sctbuf, ", %g mm/%s (%s)",
			  vscale/scale, units, desc);
	else
	    (void)sprintf(sctbuf, ", %g %s (%s)",
		    tscale/vscale*VSCALE/TSCALE*1.0*scale/tps, units, desc);
    }
    else {
	if (smode & 1)
	    (void)sprintf(sctbuf, ", %g mm/%s", vscale/scale, units);
	else
	    (void)sprintf(sctbuf, ", %g %s", 
		    tscale/vscale*VSCALE/TSCALE*1.0*scale/tps, units);
    }
    if ((scp = (char *)malloc((unsigned)(strlen(scales)+strlen(sctbuf)+1))) ==
	NULL)
	return;
    (void)sprintf(scp, "%s%s", scales, sctbuf);
    free(scales);
    free(sctbuf);
    scales = scp;
}

/* Print a grid.  The lower left corner is at (x0, y0), and the upper right
   corner is at (x1, y1);  these are printer coordinates.  The last argument
   (nticks) specifies the number of sections into which the grid is to divide
   the x-axis;  including the ticks at both ends of the axis, there will be
   (nticks + 1) ticks printed if nticks is an integer. */

void grid(x0, y0, x1, y1, nticks)
int x0, y0, x1, y1;
double nticks;
{
    if (gflag < 0)
	nticks /= 60.;	/* change tick interval from 1 second to 1 minute if
			   the alternate grid was chosen */
    (void)printf("%d %d %d %d %g grid\n", x0, y0, x1, y1, nticks);
}

/* Start a new page by invoking the PostScript `newpage' procedure.
   This is a little kludgy -- it should be necessary to send the prolog
   only once, but (with some PostScript printers, at least) pages get
   lost occasionally if this is done.  Sending the prolog at the beginning
   of each page seems to fix this problem completely, with a relatively
   low overhead.
*/
void newpage()
{
    char *getenv();
    double cox, coy, pnx, pny;
    void sendprolog();

    lmargin = rhpage() ? imargin + boff : omargin - boff;
    rmargin = rhpage() ? omargin - boff : imargin + boff;
    if (uflag)
	(void)printf("%%!\n");	/* keep TranScript happy */
    else {
	if (pages_written == 0) {
	    (void)printf("%%!PS-Adobe-1.0\n");
	    (void)printf("%%%%Creator: %s\n", pname);
	    (void)printf("%%%%Title: Full Disclosure\n");
	    (void)printf("%%%%Pages: (atend)\n");
	    (void)printf(
	     "%%%%DocumentFonts: Times-Roman Times-Italic Helvetica Symbol\n");
	    if (Lflag == 0)
		(void)printf("%%%%BoundingBox: %d %d %d %d\n",
		   (int)(lmargin*72.0/25.4 - 36.0),
		   (int)(footer_y*72.0/25.4 - 8.0),
		   (int)((p_width - rmargin)*72.0/25.4 + 8.0),
		   (int)(title_y*72.0/25.4 + 8.0));
	    else
		(void)printf("%%%%BoundingBox: %d %d %d %d\n",
		   (int)(footer_y*72.0/25.4 - 8.0),
		   (int)(lmargin*72.0/25.4 - 36.0),
		   (int)(title_y*72.0/25.4 + 8.0),
		   (int)((p_width - rmargin)*72.0/25.4 + 8.0));
	    (void)printf("%%%%EndComments\n");
	    (void)printf("%%%%EndProlog\n");
	}
	(void)printf("%%%%Page: %d %d\n", page, pages_written+1);
    }
    if (!Eflag) {
	(void)printf("matrix defaultmatrix setmatrix newpath clippath\n");
	(void)printf("pathbbox newpath pop pop translate\n");
    }
    sendprolog();
    if (Lflag) {
	(void)printf("tm setmatrix 90 rotate 0 %g mm translate\n", -p_height);
	(void)printf("/tm matrix currentmatrix def\n");
	(void)printf("/gm matrix currentmatrix def\n");
    }
    if (lwmm != LWMM)
	(void)printf("/lwmm %f def\n", lwmm);
    (void)printf("%g newpage\n", dpi);
    setrgbcolor(&lc);
    if (numberpages) {
	pnx = p_width/2 + (rhpage() ? boff : -boff);
	pny = title_y;
	(void)printf("%d %g %g prpn\n", page, pnx, pny);
    }
    if (*copyright) {
	cox = lmargin;
	coy = footer_y;
	(void)printf("( %s) %g %g prco\n", copyright, cox, coy);
    }
    (void)printf("ss\n");
    if (vflag)
	(void)fprintf(stderr, " --- Page %d ---\n", page);
}

/* If anything has been printed on the current page, add the record name,
   title, grid, and scales as appropriate and eject the page.  (Note that
   if more than one title or record name has been used on the current page,
   only the most recent values are printed, in keeping with normal conventions
   for page headings.) */
void ejectpage()
{
    flush_cont();
    if (s_top != -9999.) {
	if (rflag || *pagetitle) {
	    setrgbcolor(&lc);
	    setroman(fs_title);
	}
	if (rflag) {
	    if (rhpage()) {
		move(mm(p_width - rmargin), mm(title_y));
		rtlabel(record);
		rtlabel("Record ");
	    }
	    else {
		move(mm(lmargin), mm(title_y));
		tlabel("Record ");
		tlabel(record);
	    }
	}
	if (*pagetitle) {
	    if (rhpage()) {
		move(mm(lmargin), mm(title_y));
		if (pagetitle == defpagetitle && rdpagetitle != NULL)
		    tlabel(rdpagetitle);
		else
		    tlabel(pagetitle);
	    }
	    else {
		move(mm(p_width - rmargin), mm(title_y));
		if (pagetitle == defpagetitle && rdpagetitle != NULL)
		    rtlabel(rdpagetitle);
		else
		    rtlabel(pagetitle);
	    }
	}
	if (gflag && nticks > 0.) {
	    if (nticks > sdur) nticks = sdur;
	    s_right = s_left + nticks*tscale;
	    setrgbcolor(&gc);
	    grid(mm(s_left),mm(s_bot),mm(s_right),mm(p_height-tmargin),nticks);
	}
	setrgbcolor(&lc);
	if (lflag && usflag) {
	    setroman(fs_label);
	    move(mm(p_width - rmargin), mm(bmargin) - pt(fs_label * 1.25));
	    rlabel("* uncalibrated signal");
	}
	if (smode == 1 || smode == 2) {
	    setroman(fs_label);
	    if (scales) {
		/* 1 em at 6 points is about 1 mm.  Decide whether to
		   try to fit the scales on the same line as the copyright
		   notice. */
		if ((strlen(scales) + strlen(copyright)) * fs_label/6.0 <
		    p_width - rmargin - lmargin)
		    move(mm(p_width - rmargin), mm(footer_y));
		else
		    move(mm(p_width - rmargin),
			 mm(footer_y) + pt(fs_label * 1.25));
		rlabel(scales);
		/* reset scales for next page */
		if (smode == 1)
		    (void)sprintf(scales, "%g mm/sec", tscale);
		else
		    (void)sprintf(scales, "Grid intervals: %g sec", 1.0/tps);
	    }
	}
	(void)printf("endpsfd\n");
	(void)fflush(stdout);
	nticks = 0.;
	s_top = -9999.;
	usflag = 0;
	page++;
	pages_written++;
    }
}

void setrgbcolor(color)
struct pscolor *color;
{
  static struct pscolor currentcolor = { -1.0, -1.0, -1.0 };

  if (Cflag == 0) {
    color->red = 0;
    color->green = 0;
    color->blue = 0;
  }
  if (color->red == currentcolor.red &&
    color->green == currentcolor.green &&
    color->blue == currentcolor.blue)
    return;
  if (color->red < 0.0 || color->red > 1.0 ||
    color->green < 0.0 || color->green > 1.0 ||
    color->blue < 0.0 || color->blue > 1.0)
    return;
  if (Cflag == 0)
    printf("0 setgray\n");
  else
    printf("%g %g %g setrgbcolor\n", color->red, color->green, color->blue);
  currentcolor = *color;
}

static int bya, byd;
void setbar1(ya, yd)
int ya, yd;
{
    bya = ya; byd = yd;
}

void setbar2(y)
int y;
{   
    if (Mflag < 2)	/* draw bars across all signals */
	(void)printf("%d %d %d %d sb\n", bya, y+mm(5.), y-mm(3.), byd);
    else {		/* draw bars across one signal only */
	int ya, yb;

	ya = y - mm(1.);
	yb = y - mm(t_height - 2.);
       (void)printf("%d %d Sb\n", ya, yb);
    }
}

/* Text-printing functions. */

/* Low-level text-printing function. */
void plabel(s, t)
char *s;
int t;
{
    flush_cont();
    (void)putchar('(');
    while (*s) {
	if (*s == '(' || *s == ')' || *s == '\\')
	    (void)putchar('\\');
	(void)putchar(*s);
	s++;
    }
    (void)printf(")%c\n", t);
}

/* Print a string beginning at the current point. */
void label(s)
char *s;
{
    plabel(s, 't');
}

/* Print a string ending at the current point. */
void rlabel(s)
char *s;
{
    plabel(s, 'b');
}

/* Print a string using small caps for the lower-case letters, beginning at
   the current point. */
void tlabel(s)
char *s;
{
    int big = 1;
    static char c[2];

    while (*s) {
	if (islower(*s)) {
	    c[0] = toupper(*s);
	    if (big) { smaller(); big = 0; }
	}
	else {
	    c[0] = *s;
	    if (!big) { larger(); big = 1; }
	}
	label(c);
	s++;
    }
    if (!big) larger();
}

/* As above, but ending at the current point. */
void rtlabel(s)
char *s;
{
    int big = 1;
    static char c[2], *p;

    p = s + strlen(s) - 1;
    while (p >= s) {
	if (islower(*p)) {
	    c[0] = toupper(*p);
	    if (big) { smaller(); big = 0; }
	}
	else {
	    c[0] = *p;
	    if (!big) { larger(); big = 1; }
	}
	rlabel(c);
	p--;
    }
    if (!big) larger();
}

/* Font-changing functions. */

char style;		/* type style: I (italic), R (roman), S (sans serif) */
double fsize;		/* font size in printer's points */

void setitalic(size)
double size;
{
    flush_cont();
    fsize = size; style = 'I';
    (void)printf("%g %c\n", fsize, style); 
}

void setroman(size)
double size;
{
    flush_cont();
    fsize = size; style = 'R';
    (void)printf("%g %c\n", fsize, style); 
}

void setsans(size)
double size;
{
    flush_cont();
    fsize = size; style = 'S';
    (void)printf("%g %c\n", fsize, style); 
}

void smaller()	/* change to a font 20% smaller in the current style */
{
    flush_cont();
    fsize *= 0.8;
    (void)printf("%g %c\n", fsize, style);
}

void larger()	/* change to a font 25% larger in the current style */
{
    flush_cont();
    fsize *= 1.25;
    (void)printf("%g %c\n", fsize, style);
}

/* Line-drawing functions. */

int xc, yc;	/* current move/cont "pen position" (pixels) */

/* (Re)set the current point without drawing a line.  The arguments are in
   pixels. */
void move(x, y)
int x, y;
{
    flush_cont();
    (void)printf("%d %d m\n", x, y);
    xc = x; yc = y;
}

/* Draw a line from the current point to (x, y), and make (x, y) the new
   current point.  The arguments are in pixels.

   This implementation of `cont' is optimized for the current application, in
   which many short vectors with monotonically increasing x components are
   connected end-to-end to plot a signal.  Simpler methods typically require
   transmission of 8 to 12 characters per vector to the PostScript printer, and
   printing speed is limited by transmission time.  This algorithm generates
   slightly more than 1 character on average per vector, and execution within
   the printer is compute-bound (for most PostScript printers).

   WARNING:  Since the output of `cont' is buffered, `flush_cont' must be
   invoked before sending any other output to the printer.

   FURTHER WARNING:  The output of pschart's `cont' function is similar to that
   generated by psfd's `cont' function, but the optimizations are slightly
   (i.e., incompatibly) different.  Don't mix up the prolog files!	*/

static char contbuf[256], *contp = contbuf;

void cont(x, y)
int x, y;
{
    char c;
    int dx, dy;

    dx = x - xc; dy = y - yc;
    xc = x; yc = y;
    if (0 <= dx && dx <= 1 && -23 <= dy && dy <= 23) {
	c = 79 + 2*dy + dx;
	if (c == '(' || c == ')' || c == '\\')
	    *contp++ = '\\';
	*contp++ = c;
	if (contp - contbuf >= 250)
	    flush_cont();
    }
    else {
	flush_cont();
	(void)printf("%d %d N\n", dx, dy);
    }
}

void flush_cont()
{
    if (contp > contbuf) {
	(void)printf("(%s) z\n", contbuf);
	while (--contp > contbuf)
	    *contp = '\0';
    }
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
 "usage: %s [ OPTIONS ] SCRIPT-FILE\n",
 "Options are:",
 " -a ANN    specify first annotator (default: atr)",
 " -A ANN    specify second annotator (default: none)",
 " -b N      set binding offset in mm (default: 0)",
 " -c STR    set copyright notice",
 " -C        produce charts in color (default: black and white)",
 " -CX RED GREEN BLUE  specify a color, where 'X' is 'a' (annotations),",
 "   'g' (grid), 'l' (labels), or 's' (signals); and RED, GREEN, BLUE are",
 "   between 0 and 1 inclusive (example: '-Cs 0 .5 1' causes the signals to",
 "   be drawn in blue-green on a color-capable device)",
 " -d N      specify printer resolution in dpi (default: 300)",	/* ** DPI ** */
 " -e        even/odd processing for two-sided printing",
 " -E        generate EPSF",
 " -g        print grids with 1-second tick intervals",
 " -G        print grids with 1-minute tick intervals",
 " -h        print this usage summary",
 " -H N      set trace height in mm (default: 7.5)",
 " -l        print signal labels",
 " -L        use landscape orientation",
 " -m IN OUT TOP BOTTOM   set margins in mm",
 " -M        print annotation marker bars across all signals (same as -M1)",
 " -Mn       (NOTE: no space after M) set marker bar and annotation format,",
 "   where n is 0 (no bars), 1 (bars across all signals), 2 (bars across",
 "   attached signal, annotations at center), or 3 (bars across attached",
 "   signal, annotations above bars) (default: 0)",
 " -n N      set first page number (default: 1)",
 " -N        print counter values",
 " -p        pack short strips side-by-side",
 " -P PAGESIZE  set page dimensions (default: letter)",	  /* ** DEF_PTYPE ** */
 " -R        include record names in page headers",
 " -s SIGNAL-LIST  print listed signals only",
 " -S SCALE-MODE TIMESTAMP-MODE",
 "    SCALE-MODE 0: none; 1: mm/unit; 2: units/tick (default: 1)",
 "    TIMESTAMP-MODE 0: none, 1: elapsed, 2: absolute or elapsed (default: 2)",
 " -t N      set time scale to N mm/sec (default: 2.5)",     /* ** TSCALE ** */
 " -tm N     set time scale to N mm/min",
 " -T STR    set page title",
 " -u        generate `unstructured' PostScript",
 " -v N      set voltage scale to N mm/mV (default: 1)",     /* ** VSCALE ** */
 " -V        verbose mode",
 " -w N      set line width to N mm (default: 0.2; 0 is narrowest possible)",
 " -x        extend last strip up to 10% if necessary",
 " -1        print only first character of comment annotation strings",
 " -         read script from standard input",
 "Script line format:",
 " RECORD TIME",
 "  TIME may include start and end separated by `-'.  Anything following TIME",
 " is ignored.  Empty lines force page breaks.",
 NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++) {
	(void)fprintf(stderr, "%s\n", help_strings[i]);
	if (i % 23 == 0) {
	    char b[5];
	    (void)fprintf(stderr, "--More--");
	    (void)fgets(b, 5, stdin);
	    (void)fprintf(stderr, "\033[A\033[2K");  /* erase "--More--";
						      assumes ANSI terminal */
	}
    }
}

/* This PostScript code is the default prolog.  If PSCHARTPRO names a readable
file, the contents of that file are transmitted as a prolog instead of the code
below.  For a commented version of the code below, see `psfd.pro'. */

static char *dprolog[] = {
" %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
" % If this message appears in your printout, you may be using a buggy version %",
" % of Adobe TranScript. Try using psfd with the -u option as a workaround.    %",
" %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
"save 100 dict begin /psfd exch def",
"/dpi 300 def",
"/mm {72 mul 25.4 div}def",
"/lwmm 0.2 def",
"/lw lwmm mm def",
"/tm matrix currentmatrix def",
"/gm matrix currentmatrix def",
"/I {/Times-Italic findfont exch scalefont setfont}def",
"/R {/Times-Roman findfont exch scalefont setfont}def",
"/S {/Helvetica findfont exch scalefont setfont}def",

"/grid { /nt exch def /y1 exch def /x1 exch def /y0 exch def /x0 exch def",
" /dx x1 x0 sub nt div def",
" /dy1 dpi 12.7 div def /dy2 dpi 25.4 div def",
" x0 y0 moveto x1 y0 lineto x0 y1 moveto x1 y1 lineto stroke",
" 0 1 nt cvi { dup 5 mod 0 eq {/dy dy1 def} {/dy dy2 def} ifelse",
"  dx mul x0 add /xx exch def newpath",
"  xx y0 moveto xx y0 dy sub lineto",
"  xx y1 moveto xx y1 dy add lineto stroke }for",
"}bind def",

"/prpn { mm exch mm exch moveto 10 R /pn exch def /str 10 string def",
" pn str cvs stringwidth exch -.5 mul exch rmoveto",
" (- ) stringwidth exch neg exch rmoveto",
" (- ) show",
" pn str cvs show",
" ( -) show } def",

"/prco { mm exch mm exch moveto",
" 6 R (Copyright ) show",
" /Symbol findfont 6 scalefont setfont (\\323) show",
" 6 R show } def",

"/newpage {/dpi exch def tm setmatrix newpath [] 0 setdash 0 setgray",
" 1 setlinecap /lw lwmm mm def mark } def",

"/ss {72 dpi div dup scale /gm matrix currentmatrix def lw setlinewidth} def",

"/t {tm setmatrix show gm setmatrix}def",

"/b {tm setmatrix dup stringwidth exch neg exch rmoveto currentpoint 3 -1 roll",
" show moveto gm setmatrix}def",

"/m {newpath moveto}def",

"/N {rlineto currentpoint stroke moveto}bind def",

"/z {{33 sub dup 1 and exch 2 idiv 23 sub rlineto}forall",
" currentpoint stroke moveto}bind def",

"/ay 0 def",

"/Ay {/ay exch def}def",

"/ya 0 def",
"/yb 0 def",
"/yc 0 def",
"/yd 0 def",
"/ye 0 def",

"/sb {/yd exch def /yc exch def /yb exch def /ya exch def",
" /ye dpi 50.8 div lw sub def}def",

"/Sb {/yb exch def /ya exch def /yc yb def /yd yb def",
" /ye dpi 50.8 div lw sub def}def",

"/mb { dup ya newpath moveto dup yb lineto dup yc moveto yd lineto",
" [lw ye] 0 setdash stroke [] 0 setdash}bind def",

"/a {ya yb ne {dup mb}if ay m t}bind def",

"/A {ya yb ne {dup mb}if ay m (\\267) t}bind def",

"/endpsfd {cleartomark showpage psfd end restore}def",
NULL
};

void sendprolog()
{
    char buf[100], *prolog;
    int i;
    FILE *fp;

    if ((prolog = getenv("PSFDPRO")) == NULL) prolog = PROLOG;
    if ((fp = fopen(prolog, "r")) == NULL) {
	for (i = 0; dprolog[i] != NULL; i++)
	    (void)printf("%s\n", dprolog[i]);
	return;
    }

    /* Copy all except empty and comment lines from the prolog to stdout. */
    while (fgets(buf, 100, fp))
	if (*buf != '%' && *buf != '\n') (void)fputs(buf, stdout);
    (void)fclose(fp);
}
