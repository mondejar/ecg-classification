/* file: pschart.c	G. Moody       15 March 1988
			Last revised:   27 May 2009

-------------------------------------------------------------------------------
pschart: Produce annotated `chart recordings' on a PostScript device
Copyright (C) 1988-2009 George B. Moody

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

/* PROLOG is the pathname of the accompanying prolog file, `pschart.pro', which
   should normally be accessible to this program at run-time.  In most cases,
   PROLOG should be an absolute pathname (beginning from the root directory),
   so that `pschart' can be used from any directory. If PROLOG is inaccessible,
   the built-in prolog (see dprolog[], below) is used instead.		*/
#ifndef PROLOG
#define PROLOG	"/usr/local/lib/ps/pschart.pro"
#endif

/* COPYR is the default copyright notice printed at the bottom of each page,
   with `%d' replaced by the current year obtained from the system clock. */
#define COPYR "Massachusetts Institute of Technology %d. All rights reserved."

/* Configuration parameters (changeable only by recompiling) */
#define DPI	 300.0		/* default printer resolution (dots/inch) */
#define LWMM	   0.2		/* default line width (mm) */
#define TSCALE	  12.5		/* default time scale (mm/second) */
#define VSCALE	   5.0		/* default voltage scale (mm/millivolt) */
#define TPS	  25.0		/* grid ticks per second */
#define TPMV	  10.0		/* grid ticks per millivolt */
#define H_SEP	   5.0		/* horizontal space between strips (mm) */
#define L_SEP	   1.0		/* distance from labels to sides of grid */
#define T_SEP	   1.0		/* distance from bottom of title to top of
				   grid (mm) */
#define V_SEP	   5.0		/* vertical space between strips (mm) */
#define FS_ANN	  12.0		/* font size (in PostScript points) for
				   annotations */
#define FS_LABEL  10.0		/* font size for labels */
#define FS_TITLE  12.0		/* font size for title */

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
FILE *infofile;			/* file to print instead of title */

/* User-settable parameters */
char aname[41] = "atr";		/* annotator name */
char aname2[41] = "";		/* second annotator name */
int aux_shorten = 0;		/* if non-zero, print first char of aux only */
double boff = 0.;		/* binding offset (mm) */
char *copyright;		/* copyright notice string */
char *defpagetitle;		/* default page title */
char *footer = NULL;		/* footer string */
char *rdpagetitle;		/* page title based on recording date */
double dpi = DPI;		/* pixels per inch */
int Cflag = 0;			/* if non-zero, produce color output */
int eflag = 0;			/* if non-zero, do even/odd page handling */
int Eflag = 0;			/* generate EPSF */
int gflag = 0;			/* if non-zero, plot grid */
int Gflag = 0;			/* if non-zero, plot alternate grid */
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
int numberpages = 1;		/* if zero, suppress page numbering */
int nosig = 0;			/* number of signals to be printed */
int nomax = 0;			/* largest value for nosig seen so far */
int page = 1;			/* logical page number */
int pages_written = 0;		/* number of pages written already */
int prolog_written = 0;		/* if non-zero, prolog has been written */
char *pagetitle = NULL;		/* if not null, title for page header */
char *pname;			/* the name by which this program is invoked */
int pflag = 0;			/* if non-zero, pack strips side-by-side */
int rflag = 0;			/* if non-zero, print record names */
int Rflag = 0;			/* if non-zero, print record name in header */
int sflag = 0;			/* if non-zero, a signal list was specified */
int *siglist;			/* list of signals to be printed */
int smode = 1;			/* scale mode (0: no scales; 1: mm/unit in
				   footers; 2: units/tick in footers; 3:
				   mm/unit above strips; 4: units/tick above
				   strips; 5: mm/unit within strips; 6: units/
				   tick within strips) */
char **snstr;			/* signal names (if provided on command line) */
char *sqstr;			/* signal quality string (1 char/signal) */
double tpmv = TPMV;		/* grid ticks per millivolt */
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
int xflag = 0;                  /* if non-zero, plot only in (xvmin, xvmax) */
int xvmax;			/* high end of range if xflag set */
int xvmin;			/* low end of range if xflag set */

double h_sep = H_SEP;		/* horizontal space between strips (mm) */
double l_sep = L_SEP;		/* distance from labels to sides of grid */
double t_sep = T_SEP;		/* distance from bottom of title to top of
				   grid (mm) */
double v_sep = V_SEP;		/* vertical space between strips (mm) */
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
struct pscolor Gc = { 0.2, 0.2, 0.2 };	/* alternate grid: dark grey */
struct pscolor lc = { 0.0, 0.0, 0.0 };	/* labels: black */
struct pscolor sc = { 0.0, 0.0, 0.3 };	/* signals: dark blue */

char *prog_name();
int printstrip(), setpagedim(), setpagetitle();
void append_scale(), cont(), ejectpage(), flush_cont(), grid(), Grid(), help(),
    label(), larger(), move(), newpage(), plabel(), process(), rlabel(),
    rtlabel(), setbar1(), setbar2(), setcourier(), setitalic(), setmargins(),
    setrgbcolor(), setroman(), smaller(), tlabel();

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
    if ((p = getenv("TPMV")) && (tpmv = atof(p)) <= 0.0) tpmv = TPMV;
    if (p = getenv("H_SEP")) h_sep = atof(p);
    if (p = getenv("L_SEP")) l_sep = atof(p);
    if (p = getenv("T_SEP")) t_sep = atof(p);
    if (p = getenv("V_SEP")) v_sep = atof(p);
    if ((p = getenv("FS_ANN")) && (fs_ann = atof(p)) <= 0.0) fs_ann = FS_ANN;
    if ((p = getenv("FS_LABEL"))&&(fs_label=atof(p)) <= 0.0) fs_label=FS_LABEL;
    if ((p = getenv("FS_TITLE"))&&(fs_title=atof(p)) <= 0.0) fs_title=FS_TITLE;
    if (p = getenv("PTYPE")) ptype = p;
    if (p = getenv("FOOTER")) footer = p;

    /* Check for buggy TranScript software. */
    if (getenv("TRANSCRIPTBUG")) uflag = 1;

    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* specify annotator */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -a\n",
			pname);
		exit(1);
	    }
	    /* workaround for MS-DOS command.com bug */
	    if (strcmp(argv[i], " ") == 0) *argv[i] = '\0';
	    (void)strncpy(aname, argv[i], 40);
	    break;
	  case 'A':	/* specify second annotator */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: annotator name must follow -A\n",
			pname);
		exit(1);
	    }
	    if (aname[0] == '\0') (void)strncpy(aname, argv[i], 40);
	    else (void)strncpy(aname2, argv[i], 40);
	    break;
	  case 'b':	/* specify binding offset in mm */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: binding offset (mm) must follow -b\n",
			pname);
		exit(1);
	    }
	    boff = atof(argv[i]);
	    break;
	  case 'c':	/* specify alternate copyright notice */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: copyright notice string must follow -c\n",
			pname);
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
	    case 'G':	colorp = &Gc; break;
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
			pname);
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
	  case 'G':	/* enable alternate grid printing */
	    Gflag = 1;
	    break;
	  case 'h':    	/* print usage summary and quit */
	    help();
	    exit(0);
	    break;
	  case 'H':	/* use getvec's high resolution mode */
	    setgvmode(WFDB_HIGHRES);
	    break;
	  case 'i':	/* print contents of file in title area */
	    if (++i >= argc || ((infofile = fopen(argv[i], "rt")) == NULL)) {
		(void)fprintf(stderr,
		      "%s: the name of a readable file must follow -i\n",
			      pname);
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
	    else {
		s_defwidth = 25.0 * (int)((p_width - (imargin+omargin))/25.0);
		title_y = p_height - 0.6*tmargin;
		footer_y = 0.5*bmargin;
	    }
	    break;
	  case 'm':	/* specify margins */
	    if (++i >= argc - 3 || *argv[i] == '-' ||
		*argv[i+1] == '-' || *argv[i+2] == '-' || *argv[i+3] == '-') {
		(void)fprintf(stderr, "%s: margins must follow -m\n", pname);
		exit(1);
	    }
	    imargin = atof(argv[i++]);
	    omargin = atof(argv[i++]);
	    tmargin = atof(argv[i++]);
	    bmargin = atof(argv[i]);
	    s_defwidth = 25.0 * (int)((p_width - (imargin + omargin))/25.0);
	    title_y = p_height - 0.6*tmargin;
	    footer_y = 0.5*bmargin;
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
			      "%s: page number must follow -n\n", pname);
		exit(1);
	    }
	    page = atoi(argv[i]);
	    numberpages = (page > 0);
	    break;
	  case 'p':	/* enable printing strips side-by-side */
	    pflag = 1;
	    break;
	  case 'P':	/* set page size */
	    if (++i >= argc || (ptype = argv[i], setpagedim() == 0)) {
		(void)fprintf(stderr,
			      "%s: page size specification must follow -P\n",
			pname);
		exit(1);
	    }
	    if (!mflag)
		setmargins();
	    break;
	  case 'r':	/* enable record name printing */
	    rflag = 1;
	    break;
	  case 'R':	/* enable record name printing in header */
	    Rflag = 1;
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
		*argv[++i] == '-' ||  (smode = atoi(argv[i])) > 6 ||
		*argv[++i] == '-' || (tsmode = atoi(argv[i])) > 2) {
		(void)fprintf(stderr,
			"%s: scale and time stamp modes must follow -S\n",
			pname);
		exit(1);
	    }
	    break;
	  case 't':	/* specify time scale */
	    if (++i >= argc || ((tscale = atof(argv[i])) <= 0)) {
		(void)fprintf(stderr,
			      "%s: time scale (mm/sec) must follow -t\n",
			pname);
		exit(1);
	    }
	    break;
	  case 'T':	/* specify page title */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: page title must follow -T\n", pname);
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
			pname);
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
	  case 'x':     /* specify range of raw sample values to be plotted */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: sample range must follow -x\n",
			pname);
		exit(1);
	    }
	    xflag = 1;
   	    xvmin = atoi(argv[i++]);
	    xvmax = atoi(argv[i]);
	    break;
	  case '1':	/* abbreviate aux strings to one character on output */
	    aux_shorten = 1;
	    break;
	  case '\0':	/* read commands from standard input */
	    process(cfile = stdin);
	    break;
	}
	else if ((cfile = fopen(argv[i], "r")) == NULL) {
	    (void)fprintf(stderr, "%s: can't read %s\n", pname, argv[i]);
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
int *accept, *buflen, *v, *vbase, **vbuf, *vmax, *vmin;
long *vsum;

/* Derived parameters */
double dpmm;		/* pixels per millimeter */
double dppt;		/* pixels per PostScript "printer's point" (PostScript
			   "printer's points" are 1/72 inch;  true printer's
			   points are 1/72.27 inch) */
double dpsi;		/* pixels per sample interval */
int nsamp;		/* number of samples/signal/strip */
double sdur;		/* strip duration in seconds */
double t_tick;		/* distance (mm) between time ticks on grid */
double v_tick;		/* distance (mm) between voltage ticks on grid */

int aflag = 0;	       	/* if non-zero, plot annotations */
unsigned int nann = 0;	/* number of annotation files to be plotted (0-2) */
char *scales;		/* string which describes time and voltage scales */
char record[80];	/* current record name */
int usflag = 0;		/* if non-zero, uncalibrated signals were plotted */

/* The rhpage() macro is true for all right-hand pages (normally the odd-
   numbered pages).  It is true for even-numbered pages if even/odd page
   handling is disabled. */
#define rhpage()	((eflag == 0) || (page & 1))

/* Read and execute commands from a file. */
void process(cfile)
FILE *cfile;
{
    char *tokptr, *strtok();
    int i;
    long t0, t1, tt;
    static char combuf[256];
    static char *title, *tstring, *tstring2;
    static WFDB_Anninfo af[2] = { { aname, WFDB_READ },
				  { aname2, WFDB_READ } };

    if (scales == NULL && (scales = (char *)malloc(40)) == NULL) return;
    if (smode & 1) {
	(void)sprintf(scales, "%g mm/sec", tscale);
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
	(void)sprintf(scales, "Grid intervals: %g sec", 5.0/tps);
    dpmm = dpi/25.4; dppt = dpi/72.;
    sdur = s_defwidth/tscale;
    t_tick = tscale / tps;
    v_tick = vscale / tpmv;
    lmargin = rhpage() ? imargin + boff : omargin - boff;
    rmargin = rhpage() ? omargin - boff : imargin + boff;
    while (fgets(combuf, 256, cfile)) {	/* read a command */
	if (*combuf == '\n') {
	    if (prolog_written) ejectpage();
	}
	else {
	    tokptr = strtok(combuf, " \t");
	    if (tokptr) (void)strncpy(record, tokptr, sizeof(record)-1);
	    tstring = strtok((char *)NULL, " \t\n");
	    title = strtok((char *)NULL, "\n");
	    if (tokptr == NULL || tstring == NULL ||
		(nisig = isigopen(record, NULL, 0)) < 0) continue;
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
	    nsamp = (int)(sps*sdur);
	    if (aname2[0]) nann = 2;
	    else if (aname[0]) nann = 1;
	    else aflag = nann = 0;
	    if (nann)
		aflag = (annopen(record, af, nann) == 0);
	    tstring = strtok(tstring, "-");
	    tstring2 = strtok((char *)NULL, "-");
	    if ((t0 = strtim(tstring)) < 0L) t0 = -t0;
	    if (tstring2) {
		if ((t1 = strtim(tstring2)) < 0L) t1 = -t1;
		if (t1 != 0L && t1 < t0) { tt = t0; t0 = t1; t1 = t0; }
	    }
	    /* If no stop time is specified, plot a standard-length strip. */
	    else t1 = t0 + nsamp;
	    while (t1 == 0L || t0 < t1) {
		if ((tt = t0 + nsamp) > t1 && t1 > 0L) tt = t1;
		if (printstrip(t0, tt, record, title) == 0) break;
		t0 += nsamp;
	    }
	    wfdbquit();
	}
    }
}

static double __mt;	/* temporary variable for adu macro */
#define adu(A)	((__mt=(A)*dpadu), (int)(__mt>=0 ? __mt+0.5 : __mt-0.5))
#define mm(A)	((int)((A)*dpmm))    /* convert millimeters to pixels */
#define si(A)	((int)((A)*dpsi))    /* convert sample intervals to pixels */
#define pt(A)	((int)((A)*dppt))    /* convert PostScript points to pixels */

double dpadu;	/* pixels per adu.  This quantity must be recalculated for each
		   signal;  all signals are plotted to the same scale in mm/mV,
		   even though gains (in adu/mV) may vary among signals of the
		   same record. */

/* Position and dimensions (mm) of the strip to be printed.  The position is
   measured from the lower left corner of the page. */
double s_top = -9999., s_bot, s_left = -9999., s_right, s_height, s_width;
double t_height;	/* height (mm) of space alloted per trace */

/* Print a strip beginning at sample t0, ending at sample t1-1.  This function
   returns 2 if completely successful, 1 if the signals were plotted but not
   the annotations, or 0 if nothing was printed. */

int printstrip(t0, t1, record, title)
long t0, t1;
char *record, *title;
{
    char *ts;
    double curr_s_top;
    int i, j;
    int jmax, tm_y, *vp, x0, y0, ya[2];

    /* Allocate buffers for the samples to be plotted, and initialize the
       range variables. */
    for (i = 0; i < nosig; i++) {
	accept[i] = 0;
	vmax[i] = vmin[i] = WFDB_INVALID_SAMPLE; vsum[i] = 0L;
	if (nsamp > buflen[i]) {
	    if ((vbuf[i] = realloc(vbuf[i], nsamp * sizeof(int))) == NULL) {
		buflen[i] = 0;
		(void)fprintf(stderr, "insufficient memory\n");
		return (0);
	    }
	    buflen[i] = nsamp;
	}
    }

    if ((jmax = (int)(t1 - t0)) > nsamp) jmax = nsamp;
    if (nosig > 0) {
	/* Fill the buffers. */
	if (isigsettime(t0) < 0) return (0);
	for (j = 0; j < jmax && getvec(v) >= 0; j++) {
	    for (i = 0; i < nosig; i++) {
		int vtmp = v[siglist[i]];

		if (xflag && (vtmp > xvmax || vtmp < xvmin))
		    vtmp = WFDB_INVALID_SAMPLE;
		vbuf[i][j] = vtmp;
		if (vtmp != WFDB_INVALID_SAMPLE) {
		    if (vtmp > vmax[i] || vmax[i] == WFDB_INVALID_SAMPLE)
			vmax[i] = vtmp;
		    else if (vtmp < vmin[i] || vmin[i] == WFDB_INVALID_SAMPLE)
			vmin[i] = vtmp;
		    vsum[i] += vtmp;
		    accept[i]++;
		}
	    }
	}
	if (j == 0) return (0);
	jmax = j;

	/* Calculate the midranges. */
	for (i = 0; i < nosig; i++) {
	    int vb, vm, vs;
	    double w;

	    if (accept[i]) {
		vs = vsum[i]/accept[i];
		vb = (vmax[i] + vmin[i])/2;
		if (vb > vs && vmax[i] != vs)
		    w = (vb - vs)/(vmax[i] - vs);
		else if (vb < vs && vmin[i] != vs)
		    w = (vs - vb)/(vs - vmin[i]);
		else w = 0.0;
		vbase[i] = vs + ((double)vb-vs)*w;
	    }
	    else	/* no valid samples in this trace */
		vbase[i] = 0;
	}
	t1 = t0 + jmax;
    
	/* Determine the width and height of the strip.  Allow the greater of 4
           mV or 20 mm per trace if possible.  If n strips won't fit on the
           page (even if the allowance per trace is decreased moderately), the
           allowance is adjusted so that n-1 strips fill the page (for n = 2,
           3, and 4). */
	s_width = jmax * tscale / sps;
	if (vscale >= 5.) s_height = 4. * vscale * nosig;
	else s_height = 20. * nosig;
	if (1.5 * (s_height+v_sep) > p_height - (tmargin+bmargin))
	    s_height = p_height - (tmargin+bmargin+v_sep);
	else if (2.5 * (s_height+v_sep) > p_height - (tmargin+bmargin))
	    s_height = (p_height - (tmargin+bmargin+2*v_sep))/2; 
	else if (3.5 * (s_height+v_sep) > p_height - (tmargin+bmargin))
	    s_height = (p_height - (tmargin+bmargin+3*v_sep))/3;
	if (nosig == 0) t_height = s_height;
	else t_height = s_height / nosig;

	/* Decide where to put the strip.  Usually, this is directly below the
           previous strip, and s_top will have been set correctly after the
           previous strip was printed.  If the strip won't fit in the available
           space, though, go to the top of the next page. */
	s_bot = s_top - s_height;
	if (s_bot < bmargin) {
	    setpagetitle(t0);
	    ejectpage();
	    newpage();
	    s_top = p_height - (tmargin + v_sep);
	    s_bot = s_top - s_height;
	    if (pflag) s_left = lmargin;
	}
	/* If side-by-side printing is enabled, s_left will have been set
	   correctly after the previous strip was printed. */
	if (!pflag) s_left = lmargin + (s_defwidth - s_width)/2;
	s_right = s_left + s_width;

	/* Draw the grid and print the title and time markers. */
	if (gflag) {
	    setrgbcolor(&gc);
	    if (Gflag)
		grid(mm(s_left-10.0), mm(s_bot), mm(s_right), mm(s_top),
		     t_tick, v_tick);
	    else
		grid(mm(s_left), mm(s_bot), mm(s_right), mm(s_top),
		     t_tick, v_tick);
	}
	if (Gflag) {
	    setrgbcolor(&Gc);
	    Grid(mm(s_left), mm(s_bot), mm(s_right), mm(s_top), t_tick,v_tick);
	}
	setrgbcolor(&lc);
	setroman(fs_title);
	move(mm(s_left), mm(s_top + t_sep));
	if (rflag) { label("Record "); label(record); label("    "); }
	if (title) label(title);
	/* If side-by-side strip printing is enabled, print the time markers
	   within the strip at the upper corners; in this case, if the strip is
	   less than 30 mm wide, only the starting time marker is printed.
	   Otherwise, if signal labels are to be printed, and nosig is odd,
	   print the time markers at the bottom of the strip, so they won't
	   interfere with the labels.  Otherwise, print time markers midway
	   between top and bottom. */
	if (pflag) tm_y = mm(s_top) - pt(fs_title);
	else if (lflag && (nosig & 1)) tm_y = mm(s_bot);
	else tm_y = mm(s_top + s_bot)/2 - pt(fs_title/2.);
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
	    if (!pflag) {
		move(mm(s_left - l_sep), tm_y);
		rlabel(ts);
	    }
	    else {
		move(mm(s_left + l_sep), tm_y);
		label(ts);
	    }
	    if (vflag)
		(void)fprintf(stderr, "%s %s-", record, ts);
	    ts = mstimstr(t1);
	    while (*ts == ' ') ts++;
	    if (strcmp(ts + strlen(ts)-4, ".000") == 0)
		*(ts + strlen(ts)-4) = '\0';
	    if (!pflag) {
		move(mm(s_right + l_sep), tm_y);
		label(ts);
	    }
	    else if (s_width >= 30.) {
		move(mm(s_right - l_sep), tm_y);
		rlabel(ts);
	    }
	    if (vflag)
		(void)fprintf(stderr, "%s %s\n",
			      ts, (title != NULL) ? title : "");
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
	    if (!pflag) {
		move(mm(s_left - l_sep), tm_y);
		rlabel(ts);
	    }
	    else {
		move(mm(s_left + l_sep), tm_y);
		label(ts);
	    }
	    if (vflag)
		(void)fprintf(stderr, "%s %s-", record, ts);
	    ts = mstimstr(-t1);
	    while (*ts == ' ') ts++;
	    for (p = ts; *p != '.'; p++)
		;
	    if (strncmp(p, ".000", 4))
		p += 4;
	    if (*ts == '[')
		*p++ = ']';
	    *p = '\0';
	    if (!pflag) {
		move(mm(s_right + l_sep), tm_y);
		label(ts);
	    }
	    else if (s_width >= 30.) {
		move(mm(s_right - l_sep), tm_y);
		rlabel(ts);
	    }
	    if (vflag)
		(void)fprintf(stderr, "%s %s\n",
			      ts, (title != NULL) ? title : "");
	}

	/* Draw the signals. */
	x0 = mm(s_left);
	if (lflag) {
	    setitalic(fs_label);
	    /* If side-by-side printing and signal label printing are both
	       enabled, print the signal labels in the left margin only. */
	    if (pflag && s_left == lmargin)
		for (i = 0; i < nosig; i++) {
		    int sig = siglist[i];
		    char *d, *t;

		    d = s[sig].desc;
		    y0 = mm(s_top - (i + 0.5)*t_height);
		    move(mm(s_left - l_sep), y0 - pt(fs_label/2.));
		    if (strncmp(d,"record ",7) == 0 &&
			(t = strchr(d,',')) != NULL)
			d = t+1;
		    rlabel(d);
		    if (s[sig].gain == 0.) rlabel("* ");
		}
	}
	for (i = 0; i < nosig; i++) {
	    int last_sample_valid,  sig = siglist[i];
	    static WFDB_Calinfo ci;

	    setrgbcolor(&lc);
	    y0 = mm(s_top - (i + 0.5)*t_height);
	    if (s[sig].gain == 0.) {
		s[sig].gain = WFDB_DEFGAIN;
		usflag = uncal[sig] = 1;
	    }
	    if (lflag && !pflag) {
		char *d = s[sig].desc, *t;

		if (strncmp(d, "record ", 7) == 0 &&
		    (t = strchr(d, ',')) != NULL)
		    d = t+1;
		move(mm(s_left - l_sep), y0 - pt(fs_label * 0.3));
		rlabel(d);
		if (uncal[sig]) rlabel("* ");
		move(mm(s_right + l_sep), y0 - pt(fs_label * 0.3));
		label(d);
		if (uncal[sig]) label(" *");
	    }
	    dpadu = dpmm * vscale / s[sig].gain;
	    if (getcal(s[sig].desc, s[sig].units, &ci) == 0 &&
		ci.scale != 0.0) {
		dpadu /= ci.scale;
		append_scale(ci.sigtype, ci.units, ci.scale);
	    }
	    /* Handle the common case of mV-dimensioned signals that are not
	       in the WFDB calibration database. */
	    else if (s[sig].units == NULL)
		append_scale("record", "mV", 1.0);
	    /* Adjust vbase for calibrated DC-coupled signals. */
	    if (ci.scale != 0.0 && (ci.caltype & 1) && !uncal[sig]) {
		int n, yzero = physadu((unsigned int)sig, 0.0), ytick;
		double ptick = 5.0*ci.scale/tpmv;

		ytick = physadu((unsigned int)sig, ptick) - yzero;
		if (ytick == 0) ytick = 1;
		if (vbase[i] >= yzero)
		    n = (int)((vbase[i] - yzero)/ytick);
		else
		    n = -(int)((yzero - vbase[i])/ytick);
		vbase[i] = yzero + ytick * n;
		if (lflag && !pflag) {
		    char lbuf[20];
		    int yt, yi = pt(fs_label * 0.3);

		    setroman(fs_label * 0.75);
		    (void)sprintf(lbuf, "%g", ptick * (n-1));
		    yt = y0 - adu(ytick);
		    move(mm(s_left - 1.5*l_sep), yt - yi); rlabel(lbuf);
		    move(mm(s_left - l_sep), yt); cont(mm(s_left), yt);
		    move(mm(s_right + 1.5*l_sep), yt - yi); label(lbuf);
		    move(mm(s_right + l_sep), yt); cont(mm(s_right), yt);
		    (void)sprintf(lbuf, "%g", ptick * (n+1));
		    yt = y0 + adu(ytick);
		    move(mm(s_left - 1.5*l_sep), yt - yi); rlabel(lbuf);
		    move(mm(s_left - l_sep), yt); cont(mm(s_left), yt);
		    move(mm(s_right + 1.5*l_sep), yt - yi); label(lbuf);
		    move(mm(s_right + l_sep), yt); cont(mm(s_right), yt);
		    setitalic(fs_label);
		}
	    }
	    y0 -= adu(vbase[i]);
	    setrgbcolor(&sc);
	    for (j = last_sample_valid = 0, vp = vbuf[i]; j < jmax; j++, vp++){
		if (*vp == WFDB_INVALID_SAMPLE)
		    last_sample_valid = 0;
		else if (last_sample_valid)
			cont(x0 + si(j), y0 + adu(*vp));
		else {
		    move(x0 + si(j), y0 + adu(*vp));
		    last_sample_valid = 1;
		}
	    }
	}
    }
    if (lflag) setroman(fs_label);
    else flush_cont();

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
       that on the right is used in other cases. */
    if (nosig < 2) ya[0] = mm(s_top - 5.0) - pt(fs_ann/2.);
    else ya[0] = mm(s_top - t_height) - pt(fs_ann/2.);
    if (nosig < 3) ya[1] = mm(s_bot + 5.0) - pt(fs_ann/2.);
    else ya[1] = mm(s_top - 2*t_height) - pt(fs_ann/2.);
    if (Mflag) setbar1(mm(s_top), mm(s_bot));

    /* Set s_top (and s_left, if side-by-side strip printing is enabled)
       for the next strip. */
    curr_s_top = s_top;
    if (pflag) {
	s_left = s_right + h_sep;
	/* If there is insufficient room on the current line to print at
	   least one second at the current time scale, go to the next line. */
	if (s_left + tscale > lmargin + s_defwidth) {
	    if (lflag) {
		setrgbcolor(&lc);
		setitalic(fs_label);
		for (i = 0; i < nosig; i++) {
		    char *d = s[siglist[i]].desc, *t;

		    y0 = mm(s_top - (i + 0.5)*t_height);
		    move(mm(s_right + l_sep), y0 - pt(fs_label * 0.4));
		    if (strncmp(d,"record ",7)==0 && (t=strchr(d,',')) != NULL)
			d = t+1;
		    label(d);
		    if (uncal[siglist[i]]) label(" *");
		}
		setroman(fs_label);
	    }
	    s_left = lmargin;
	    s_top -= (s_height + v_sep);
	}
    }
    else
	s_top -= (s_height + v_sep);	/* set s_top for the next strip */

    if (aflag) {
	static WFDB_Annotation annot;
	int x, y, c;
	unsigned int ia;


	if (iannsettime(t0) < 0 && nann == 1) return (1);
	setrgbcolor(&ac);
	setroman(fs_ann);
	for (ia = 0; ia < nann; ia++) {
	    if (Mflag <= 2)
		(void)printf("%d Ay\n", y = ya[ia]);
	    else
		c = -1;
	    while (getann(ia, &annot) >= 0 && annot.time < t1) {
	      if (annot.time == 0) continue;
		if (Mflag >= 2 && annot.chan != c) {
		    int i, j;
		    i = c = annot.chan;
		    if (i < 0 || i >= nisig) i = 0;
		    for (j = 0; j < nosig; j++) {
			if (siglist[j] == i) {
			    y = mm(curr_s_top - (j + 0.5)*t_height + 10.);
			    if (Mflag > 2)
				(void)printf("%d Ay\n", y);
			    break;
			}
		    }
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
		    (void)printf("%d A\n", x0 + si(x) );
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
    if (smode == 3 || smode == 4) {
	double a = s_width;

	setrgbcolor(&lc);
	/* Estimate the space available for printing the scales above the
	   strip (1 printer's em in 8-point type is about 1 mm). */
	if (rflag) a -= 9. + (double)strlen(record);
	if (title) a -= (double)strlen(title);
	if (a >= (double)strlen(scales) + 10.) {
	    move(mm(s_left + s_width), mm(s_top + t_sep)); rlabel(scales);
	}
    }
    else if ((smode == 5 || smode == 6) &&
	     s_width >= (double)strlen(scales) + 10.) {
	setrgbcolor(&lc);
	move(mm(s_right - l_sep), mm(s_bot + t_sep));
	rlabel(scales);
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
    else if (strcmp(ptype, "rletter") == 0) {   /* rotated letter (11x8.5) */
	p_width = 271.61;
	p_height = 208.62;
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
    tmargin = 20.0;
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
    if ((sctbuf = (char *)malloc((unsigned)strlen(units)+strlen(desc)+20)) ==
	NULL)
	return;
    /* Include desc in the scale string only if it is not "record". */
    if (strcmp(desc, "record")) {
	if (smode & 1)
	    (void)sprintf(sctbuf, ", %g mm/%s (%s)",
			  vscale/scale, units, desc);
	else
	    (void)sprintf(sctbuf, ", %g %s (%s)", 5.0*scale/tpmv, units, desc);
    }
    else {
	if (smode & 1)
	    (void)sprintf(sctbuf, ", %g mm/%s", vscale/scale, units);
	else
	    (void)sprintf(sctbuf, ", %g %s", 5.0*scale/tpmv, units);
    }
    if ((scp = (char *)malloc((unsigned)strlen(scales)+strlen(sctbuf)+1)) ==
	NULL)
	return;
    (void)sprintf(scp, "%s%s", scales, sctbuf);
    free(scales);
    free(sctbuf);
    scales = scp;
}

/* Print a grid.  The lower left corner is at (x0, y0), and the upper right
   corner is at (x1, y1);  these are printer coordinates.  The tick intervals
   (xtick and ytick) are in millimeters (converted to printer coordinates by
   the PostScript `grid' procedure to minimize roundoff error). */   

void grid(x0, y0, x1, y1, xtick, ytick)
int x0, y0, x1, y1;
double xtick, ytick;
{
    (void)printf("%d %d %d %d %g %g grid\n", x0, y0, x1, y1, xtick, ytick);
}

/* As above, but print the alternate grid. */
void Grid(x0, y0, x1, y1, xtick, ytick)
int x0, y0, x1, y1;
double xtick, ytick;
{
    (void)printf("%d %d %d %d %g %g Grid\n", x0, y0, x1, y1, xtick, ytick);
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
	    (void)printf("%%%%Title: Chart Recording\n");
	    (void)printf("%%%%Pages: (atend)\n");
	    (void)printf(
	       "%%%%DocumentFonts: Times-Roman Times-Italic Courier Symbol\n");
	    if (Lflag == 0)
		(void)printf("%%%%BoundingBox: %d %d %d %d\n",
		   (int)(lmargin*72.0/25.4 - 72.0),
		   (int)(footer_y*72.0/25.4 - 8.0),
		   (int)((p_width - rmargin)*72.0/25.4 + 44.0),
		   (int)(title_y*72.0/25.4 + 8.0));
	    else
		(void)printf("%%%%BoundingBox: %d %d %d %d\n",
		   (int)(footer_y*72.0/25.4 - 8.0),
		   (int)(lmargin*72.0/25.4 - 72.0),
		   (int)(title_y*72.0/25.4 + 8.0),
		   (int)((p_width - rmargin)*72.0/25.4 + 44.0));
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
    prolog_written = 1;
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

/* If anything has been printed on the current page, print the header as
   appropriate and eject the page. */
void ejectpage()
{
    flush_cont();
    if (s_top != -9999.) {
	setrgbcolor(&lc);
	setroman(fs_title);
	if (Rflag) {
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
	if (infofile) {
	    static char tstr[256];
	    double yt = mm(title_y) + mm(30.0);

	    if (Rflag) yt -= mm(10.0);
	    setcourier(fs_label);
	    while (fgets(tstr, sizeof(tstr), infofile)) {
		move(mm(lmargin), (int)(yt -= pt(fs_label * 1.1)));
		label(tstr);
	    }
	    (void)fclose(infofile);
	    infofile = NULL;
	}
	else if (*pagetitle) {
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
	if (lflag && usflag) {
	    setroman(fs_label*0.75);
	    move(mm(p_width - rmargin), mm(bmargin) - pt(fs_label*0.75));
	    rlabel("* uncalibrated signal");
	}
	if (smode == 1 || smode == 2) {
	    setroman(fs_label*0.75);
	    if (scales) {
		/* 1 em at 6 points is about 1 mm.  Decide whether to
		   try to fit the scales on the same line as the copyright
		   notice. */
		if ((int)(strlen(scales) + strlen(copyright)) <
		    p_width - rmargin - lmargin)
		    move(mm(p_width - rmargin), mm(footer_y));
		else
		    move(mm(p_width - rmargin),
			 mm(footer_y) + pt(fs_label*0.75));
		rlabel(scales);
		if (footer) {
		    move(mm(lmargin), mm(footer_y));
		    label(footer);
		}
		/* reset scales for next page */
		if (smode == 1) (void)sprintf(scales, "%g mm/sec", tscale);
		else (void)sprintf(scales, "Grid intervals: %g sec", 5.0/tps);
	    }
	}
	(void)printf("endpschart\n");
	(void)fflush(stdout);
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

	ya = y - mm(3.);
	yb = y - mm(t_height);
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

void setcourier(size)
double size;
{
    flush_cont();
    fsize = size; style = 'C';
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
   slightly more than 2 characters on average per vector, and execution within
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

#if DEBUG
    printf("cont(%4d,%4d)\n", x, y);
#else
    dx = x - xc; dy = y - yc;
    xc = x; yc = y;
    if (0 <= dx && dx <= 94 && -47 <= dy && dy <= 47) {
	c = 126 - dx;
	if (c == '(' || c == ')' || c == '\\')
	    *contp++ = '\\';
	*contp++ = c;
	c = 79 + dy;
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
#endif
}

void flush_cont()
{
#if DEBUG
    printf("flush_cont()\n");
#else
    if (contp > contbuf) {
	*contp = '\0';
	(void)printf("(%s) z\n", contbuf);
	contp = contbuf;
    }
#endif
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
 " -g        print grids",
 " -G	     print alternate grids",
 " -h        print this usage summary",
 " -H        use high-resolution mode for multi-frequency records",
 " -i FILE   print (as text) contents of FILE in title area of first page",
 " -l        print signal labels",
 " -L        use landscape orientation",
 " -m IN OUT TOP BOTTOM   set margins in mm",
 " -M        print annotation marker bars across all signals (same as -M1)",
 " -Mn       (NOTE: no space after M) set marker bar and annotation format,",
 "   where n is 0 (no bars), 1 (bars across all signals), 2 (bars across",
 "   attached signal, annotations at center), or 3 (bars across attached",
 "   signal, annotations above bars) (default: 0)",
 " -n N      set first page number (default: 1)",
 " -p        pack short strips side-by-side",
 " -P PAGESIZE  set page dimensions (default: letter)",	  /* ** PTYPE ** */
 " -r        include record names in strip titles",
 " -R        include record names in page headers",
 " -s SIGNAL-LIST  print listed signals only",
 " -S SCALE-MODE TIMESTAMP-MODE",
 "    SCALE-MODE 0: none; 1, 2: footer; 3, 4: above strips; 5, 6: in strips",
 "         1, 3, or 5 for mm/unit; 2, 4, or 6 for units/tick (default: 1)",
 "    TIMESTAMP-MODE 0: none, 1: elapsed, 2: absolute or elapsed (default: 2)",
 " -t N      set time scale to N mm/sec (default: 12.5)",    /* ** TSCALE ** */
 " -T STR    set page title",
 " -u        generate `unstructured' PostScript",
 " -v N      set voltage scale to N mm/mV (default: 5)",     /* ** VSCALE ** */
 " -V        verbose mode",
 " -w N      set line width to N mm (default: 0.2; 0 is narrowest possible)",
 " -x MIN MAX  plot samples only if raw values are between MIN and MAX",
 " -1        print only first character of comment annotation strings",
 " -         read script from standard input",
 "Script line format:",
 " RECORD TIME TITLE",
 "  TIME may include start and end separated by `-'.",
 "  TITLE is optional.  Empty lines force page breaks.",
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
below.  For a commented version of the code below, see `pschart.pro'. */

static char *dprolog[] = {
" %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
" % If this message appears in your printout, you may be using a buggy version %",
" % of Adobe TranScript. Try using pschart with the -u option as a workaround. %",
" %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%",
"save 100 dict begin /pschart exch def",
"/dpi 300 def",
"/mm {72 mul 25.4 div}def",
"/lwmm 0.2 def",
"/lw lwmm mm def",
"/tm matrix currentmatrix def",
"/gm matrix currentmatrix def",
"/I {/Times-Italic findfont exch scalefont setfont}def",
"/R {/Times-Roman findfont exch scalefont setfont}def",
"/C {/Courier findfont exch scalefont setfont}def",

"/grid { newpath 0 setlinecap",
" /dy1 exch dpi 25.4 div mul lw sub def /dy2 dy1 lw add 5 mul def",
" /dx1 exch dpi 25.4 div mul lw sub def /dx2 dx1 lw add 5 mul def",
" /y1 exch def /x1 exch def /y0 exch def /x0 exch def",
" dpi 100 idiv setlinewidth x0 y0 moveto x1 y0 lineto x1 y1 lineto x0 y1 lineto",
" closepath stroke lw setlinewidth [lw dx1] 0 setdash",
" y0 dy2 add dy2 y1 {newpath dup x0 exch moveto x1 exch lineto stroke}for",
" [lw dy1] 0 setdash",
" x0 dx2 add dx2 x1 {newpath dup y0 moveto y1 lineto stroke }for",
" [] 0 setdash",
"}bind def",

"/Grid { newpath 0 setlinecap",
" /dy1 exch dpi 25.4 div mul lw sub def /dy2 dy1 lw add 5 mul def",
" /dx1 exch dpi 25.4 div mul lw sub def /dx2 dx1 lw add 5 mul def",
" /y1 exch def /x1 exch def /y0 exch def /x0 exch def",
" dpi 100 idiv setlinewidth x0 y0 moveto x1 y0 lineto x1 y1 lineto x0 y1 lineto",
" closepath stroke lw setlinewidth",
" y0 dy2 add dy2 y1 {newpath dup x0 exch moveto x1 exch lineto stroke}for",
" x0 dx2 add dx2 x1 {newpath dup y0 moveto y1 lineto stroke }for",
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

"/newpage {/dpi exch def tm setmatrix newpath [] 0 setdash",
" 1 setlinecap /lw lwmm mm def mark } def",

"/ss {72 dpi div dup scale /gm matrix currentmatrix def lw setlinewidth} def",

"/t {tm setmatrix show gm setmatrix}def",

"/b {tm setmatrix dup stringwidth exch neg exch rmoveto currentpoint 3 -1 roll",
" show moveto gm setmatrix}def",

"/m {newpath moveto}def",

"/N {rlineto currentpoint stroke moveto}bind def",

"/z {{counttomark 2 ge{79 sub rlineto}{126 exch sub}ifelse}forall",
" currentpoint stroke moveto}bind def",

"/ay 0 def",
"/Ay {/ay exch def}def",
"/ya 0 def",
"/yb 0 def",
"/yc 0 def",
"/yd 0 def",
"/sb {/yd exch def /yc exch def /yb exch def /ya exch def}def",
"/Sb {/yb exch def /ya exch def /yc yb def /yd yb def}def",
"/mb { dup ya newpath moveto dup yb lineto dup yc moveto yd lineto",
"stroke}bind def",
"/a {ya yb ne {dup mb}if ay m t}bind def",
"/A {ya yb ne {dup mb}if ay m (\\267) t}bind def",
"/endpschart {cleartomark showpage pschart end restore}def",
NULL
};

void sendprolog()
{
    char buf[100], *prolog;
    int i;
    FILE *fp;

    if ((prolog = getenv("PSCHARTPRO")) == NULL) prolog = PROLOG;
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
