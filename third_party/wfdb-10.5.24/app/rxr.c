/* file: rxr.c		G. Moody	16 August 1989
			Last revised:    7 August 2009

-------------------------------------------------------------------------------
rxr: ANSI/AAMI-standard run-by-run annotation file comparator
Copyright (C) 1989-2009 George B. Moody

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

This program implements the run-by-run comparison algorithms described in
AAMI/ANSI EC38:1998, the American National Standard for ambulatory ECGs, and
in AAMI EC57:1998, the American National Standard for Testing and Reporting
Performance Results of Cardiac Rhythm and ST Segment Measurement Algorithms.
These standards are available from AAMI, 1110 N Glebe Road, Suite 220,
Arlington, VA 22201 USA (http://www.aami.org/)

The -f, -t, and -w options modify the comparison algorithm used by rxr in ways
not permitted by EC38:1998 or EC57:1998.  These options are provided for the
use of developers, who may find them useful for obtaining a more detailed
understanding of algorithm errors.
*/

#include <stdio.h>
#ifndef BSD
#include <string.h>
#else
#include <strings.h>
#endif
#ifndef __STDC__
extern void exit();
#endif

#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

char *pname;		/* name by which this program was invoked */
char *record;
WFDB_Anninfo an[2];
WFDB_Annotation annot[2], tempann;
int fflag = 3;
FILE *ofile, *sfile;
long start, end_time, match_dt;
int verbose;
long s[2][7][7];

main(argc, argv)
int argc;
char *argv[];
{
    void init(), print_results(), rxr();

    /* Read and interpret command-line arguments. */
    init(argc, argv);

    rxr(0, 0);		/* check VE run sensitivity */
    rxr(1, 0);		/* check VE run positive predictivity */
    print_results(0);	/* print VE run statistics */

    if (fflag > 3) {
	rxr(0, 1);		/* check SVE run sensitivity */
	rxr(1, 1);		/* check SVE run positive predictivity */
	print_results(1);	/* print SVE run statistics */
    }
    exit(0);    /*NOTREACHED*/
}

static long f_end;

/* Perform a run-by-run comparison. */
void rxr(stat, type)
int stat, type;
{
    int i, j, goflag = 1;
    int run_length[2];
    unsigned int a, b;
    long run_start, run_end;
    void initamap();

    if (iannsettime(0L) < 0) exit(2);

    /* Initialize state variables. */
    initamap();
    annot[0].time = annot[1].time = run_start = run_end = f_end = 0L;
    for (i = 0; i < 7; i++)
	for (j = 0; j < 7; j++)
	    s[stat][i][j] = 0L;

    /* If stat is 0, check sensitivity;  in this case, annotator 0 defines the
       location of runs. */
    if (stat == 0) {
	a = 0;
	b = 1;
    }
    /* Otherwise, check positive predictivity;  in this case, annotator 1
       defines the location of runs. */
    else {
	a = 1;
	b = 0;
    }
    run_length[a] = -1;
    run_length[b] = 0;

    /* Determine the initial state of annotator a at the beginning of the
       test period. */
    while (getann(a, &annot[a]) >= 0) {
	if (annot[a].time >= start) {
	    if (run_length[a] < 0) run_length[a] = 0;
	    break;
	}
	switch (amap(annot[a], a)) {
	  case '[':
	    if (type == 0) run_length[a] = 6;
	    break;
	  case '{':
	    if (type == 1) run_length[a] = 6;
	    break;
	  case ']':
	  case '}':
	    run_length[a] = 0;
	    break;
	}
    }

    /* If there were no annotations in file a, quit. */
    if (run_length[a] < 0) exit(2);

    /* If VF (AF) began during the learning period, set the start of the match
       window to 150 msec before the normal beginning of the test period. */
    if (run_length[a] > 0) {
	run_start = start - match_dt;
	run_end = start + match_dt;
    }

    if (verbose)
	(void)printf("\n%sVE Run-by-run comparison for %s\n", type ? "S" : "",
	       stat ? "positive predictivity" : "sensitivity");

    /* Process an annotation from file a each time through this loop. */
    do {
	/* If we have found the beginning of a run, look for the end of it. */
	if (run_length[a] > 0) {
	    switch (amap(annot[a], a)) {
	      case 'N':
	      case 'Q':
	      case 'U':
		/* These annotations indicate that the run ended 150 msec
		   after the previous annotation.  Find the longest run from
		   file b in the match window (up to 6 beats). */
		run_length[b] = find_longest_run(b, run_start, run_end, type);
		/* Update the confusion matrix. */
		if (verbose && run_length[0] != run_length[1])
		    (void)printf("%d/%d(%ld-%ld)\n", run_length[0],
			   run_length[1], run_start, run_end);
		s[stat][run_length[0]][run_length[1]]++;
		/* Reset the state variable. */
		run_length[a] = 0;
		break;
	      case 'V':
	      case 'F':
		if (type == 0) {
		    /* Beats of these types extend the run length (but we don't
		       keep track of run lengths beyond 6 beats). */
		    if (run_length[a] < 6) run_length[a]++;
		    run_end = annot[a].time + match_dt;
		}
		else {	/* this beat ends an SVE run */
		    run_length[b] = find_longest_run(b,run_start,run_end,type);
		    if (verbose && run_length[0] != run_length[1])
			(void)printf("%d/%d(%ld-%ld)\n", run_length[0],
			       run_length[1], run_start, run_end);
		    s[stat][run_length[0]][run_length[1]]++;
		    run_length[a] = 0;
		}
		break;
	      case ']':
		if (type == 0)
		    /* This annotation might end the run. */
		    run_end = annot[a].time + match_dt;
		break;
	      case 'S':
		if (type == 1) {
		    if (run_length[a] < 6) run_length[a]++;
		    run_end = annot[a].time + match_dt;
		}
		else {	/* this beat ends a VE run */
		    run_length[b] = find_longest_run(b,run_start,run_end,type);
		    if (verbose && run_length[0] != run_length[1])
			(void)printf("%d/%d(%ld-%ld)\n", run_length[0],
			       run_length[1], run_start, run_end);
		    s[stat][run_length[0]][run_length[1]]++;
		    run_length[a] = 0;
		}
		break;
	      case '}':
		if (type == 1)
		    /* This annotation might end the run. */
		    run_end = annot[a].time + match_dt;
		break;
	      default:
		/* If we come here, the annotation is neither a beat label,
		   nor the end of VF, nor an unreadable signal annotation; it
		   is ignored. */
		break;
	    }
	}
	else {
	    /* Look for the beginning of a run. */
	    switch (amap(annot[a], a)) {
	      case 'V':
	      case 'F':
		if (type == 0) {
		    /* These beats begin a run.  The match window begins 150 ms
		       before the current annotation (to allow for differences
		       in fiducial placement). */
		    run_length[a] = 1;
		    run_start = annot[a].time - match_dt;
		    run_end = annot[a].time + match_dt;
		}
		break;
	      case '[':
		if (type == 0) {
		    /* VF is treated as equivalent to a maximum-length run. */
		    run_length[a] = 6;
		    run_start = annot[a].time - match_dt;
		}
		break;
	      case 'S':
		if (type == 1) {
		    run_length[a] = 1;
		    run_start = annot[a].time - match_dt;
		    run_end = annot[a].time + match_dt;
		}
		break;
	      case '{':
		if (type == 1) {
		    run_length[a] = 6;
		    run_start = annot[a].time - match_dt;
		}
		break;
	      default:
		/* Other annotations can be ignored in this context. */
		break;
	    }
	}
	if (getann(a, &tempann) < 0) {
	    if (run_length[a] > 0)
		annot[a].anntyp = UNKNOWN;
	    else
		goflag = 0;
	}
	else
	    annot[a] = tempann;
    } while (goflag > 0 && (end_time <= 0L || annot[a].time <= end_time));
}

/* `pstat' prints a statistic described by s, defined as the quotient of a and
   b expressed in percentage units.  Undefined values are indicated by `-'. */

void pstat(s, a, b)
char *s;
long a, b;
{
    if (fflag != 2 && fflag != 5) {
	(void)fprintf(ofile, "%s:", s);
	if (b <= 0) (void)fprintf(ofile, "   - ");
	else (void)fprintf(ofile, " %3d%%", (int)((100.*a)/b + 0.5));
	(void)fprintf(ofile, " (%ld/%ld)\n", a, b);
    }
    else if (b <= 0) (void)fprintf(ofile, "   -");
    else (void)fprintf(ofile, " %3d", (int)((100.*a)/b + 0.5));
}


/* This function determines an AAMI test label for the given annotation from
   annotator i.  In addition to the test labels defined in the AAMI RP, these
   include:
     `{'	beginning of AF
     `}'	end of AF
     `C'	end of unreadable segment
 */

int inaf[2], invf[2], unreadable[2];	/* state variables for amap */

int amap(annot, i)
WFDB_Annotation annot;
unsigned int i;
{
    switch (annot.anntyp) {
	case NORMAL:
	case LBBB:
	case RBBB:
	case BBB:	return (inaf[i] ? 'S' : 'N');
	  /* All of the above are treated as equivalent to normal beats; but
	     within AF, they are equivalent to supraventricular ectopic
	     beats. */

	case NPC:
	case APC:
	case SVPB:
	case ABERR:
	case NESC:
	case AESC:
	case SVESC:	return ('S');
	  /* Supraventricular ectopic beats. */

	case PVC:
	case RONT:
	case VESC:	return ('V');
	  /* Ventricular ectopic beats. */

	case FUSION:	return ('F');
	  /* Fusion of ventricular and normal beat. */

	case UNKNOWN:
	case LEARN:	return ('Q');
	  /* Unclassifiable beats.  LEARN annotations should appear only in the
	     test annotation file, and only during the learning period;  if
	     they appear elsewhere, they are treated in the same way as unknown
	     beats. */

	case PACE:
	case PFUS:	return ('Q');
	  /* The AAMI RP excludes records containing paced beats from its
	     reporting requirements.  To permit this program to be used with
	     such records, beats which are either paced (type PACE) or fusions
	     of paced and normal beats (type PFUS) are treated in the same way
	     as unknown beats. */

	case NOISE:
	  /* A `start shutdown' annotation is mapped to `U' only if shutdown
	     is not already in progress according to annotator i. */
	  if ((annot.subtyp & 0x30) == 0x30) {
	      if (unreadable[i]) return ('O');
	      else {
		  unreadable[i] = 1;
		  return ('U');
	      }
	  }
	  else if (unreadable[i]) {
	      unreadable[i] = 0;
	      return ('C');
	  }
	  else
	      return ('O');

	case VFON:
	  /* A VFON annotation is mapped to `[' only if VF is not already in
	     progress according to annotator i. */
	  if (invf[i]) return ('O');
	  else {
	      inaf[i] = 0; invf[i] = 1;
	      return ('[');
	  }
	case VFOFF:
	  /* A VFOFF annotation is mapped to `]' only if VF has been in
	     progress according to annotator i. */
	  if (!invf[i]) return ('O');
	  else {
	      invf[i] = 0;
	      return (']');
	  }
	case RHYTHM:
	  if (annot.aux == NULL || *(annot.aux) == 0)
	      return('O');
	  /* An `(AF' rhythm change annotation is mapped to `{' only if AF is
	     not already in progress.  If VF was in progress, it is assumed to
	     have ended. */
	  if (strncmp(annot.aux+1, "(AF", 3) == 0) {
	      if (inaf[i]) return ('O');
	      else {
		  inaf[i] = 1; invf[i] = 0;
		  return ('{');
	      }
	  }
	  /* A `(VF' rhythm change annotation is mapped to `[' only if VF is
	     not already in progress.  If AF was in progress, it is assumed to
	     have ended. */
	  else if (strncmp(annot.aux+1, "(VF", 3) == 0) {
	      if (invf[i]) return ('O');
	      else {
		  inaf[i] = 0; invf[i] = 1;
		  return ('[');
	      }
	  }
	  /* Other rhythm change annotations are mapped to `}' if AF was in
	     progress ... */
	  else if (inaf[i]) {
	      inaf[i] = 0;
	      return ('}');
	  }
	  /* ... or to `]' if VF was in progress ... */
	  else if (invf[i]) {
	      invf[i] = 0;
	      return (']');
	  }
	  /* ... or to `O' otherwise. */
	  else
	      return ('O');

	default:	return ('O');
	  /* Other annotations are treated as non-beat annotations. */
    }
}


void initamap()		/* initialize state variables for amap() */
{
    inaf[0] = inaf[1] = invf[0] = invf[1] = unreadable[0] = unreadable[1] = 0;
}

/* `find_longest_run' reads forward in annotation file `a' (the reference file
   if a is 0, the test file if a is 1), and finds the longest run (up to 6) of
   consecutive (S)VEBs between t0 and t1.
*/

find_longest_run(a, t0, t1, type)
unsigned int a;
long t0, t1;
int type;	/* 0: find VE run; 1: find SVE run */
{
    int am, len = 0, len0 = 0;

    /* If a VF (AF) episode started before t0 and ends after t0, simply return
       6 (the maximum run length considered by this program). */
    if (f_end < 0L || f_end > t0) return (6);

    /* If there are no more annotations for annotator a, simply return 0. */
    if (annot[a].time < 0L) return (0);

    /* Otherwise, find the first annotation at or beyond t0. */
    while (annot[a].time < t0) {
	if ((type == 0 && amap(annot[a], a) == '[') ||
	    (type == 1 && amap(annot[a], a) == '{')) {	/* VF (AF) begins */
	    do {
		if (getann(a, &annot[a]) < 0) {
		    /* If the annotation file ends without an annotation to
		       indicate the end of VF (AF), assume that the VF (AF)
		       continues to the end of the record (setting the value
		       of f_end less than zero signals this), and return 6
		       (the maximum run length) since the VF (AF) overlaps the
		       window. */
		    f_end = -1L;
		    return (6);
		}
		am = amap(annot[a], a);
	    } while ((type == 0 && am != ']' && am != '{' && am != 'U') ||
		     (type == 1 && am != '}' && am != '[' && am != 'U'));
	    if (annot[a].time > t0) {
		/* If the episode overlaps the window, record when it ends
		   and return the maximum run length. */
		f_end = annot[a].time;
		return (6);
	    }
	}
	/* If the annotation file ends before t0, record that this has happened
	   and return 0. */
	if (getann(a, &annot[a]) < 0) {
	    annot[a].time = -1L;
	    return (0);
	}
    }

    /* Now count consecutive (S)VEBs in the window. */
    while (annot[a].time <= t1) {
	switch(amap(annot[a], a)) {
	  case 'N':
	  case 'Q':
	  case 'U':
	    /* These annotations terminate runs.  At this point, we need to
	       see if the current run length (in len0) is the longest so far
	       within the window; if so, we must update len. */
	    if (len0 > len) len = len0;
	    len0 = 0;
	    break;
	  case 'V':
	  case 'F':
	    if (type == 0) {
		/* These beats continue runs; we count up to 6 beats in each
		   run. */
		if (len0 < 6) len0++;
	    }
	    else {
		if (len0 > len) len = len0;
		len0 = 0;
	    }
	    break;
	  case '[':
	    if (type == 0) {
		do {
		    if (getann(a, &annot[a]) < 0) {
			f_end = -1L;
			return (6);
		    }
		    am = amap(annot[a], a);
		} while (am != ']' && am != '{' && am != 'U');
		f_end = annot[a].time;
		return (6);
	    }
	    else {
		if (len0 > len) len = len0;
		len0 = 0;
	    }
	    break;
	  case 'S':
	    if (type == 1) {
		if (len0 < 6) len0++;
	    }
	    else {
		if (len0 > len) len = len0;
		len0 = 0;
	    }
	    break;
	  case '{':
	    if (type == 1) {
		do {
		    if (getann(a, &annot[a]) < 0) {
			f_end = -1L;
			return (6);
		    }
		    am = amap(annot[a], a);
		} while (am != '}' && am != '[' && am != 'U');
		f_end = annot[a].time;
		return (6);
	    }
	    else {
		if (len0 > len) len = len0;
		len0 = 0;
	    }
	    break;
	  default:
	    /* Annotations in this category are ignored. */
	    break;
	}
	/* if the annotation file ends before t1, record that this has happened
	   and return the length of the longest run seen. */
	if (getann(a, &annot[a]) < 0) {
	    annot[a].time = -1L;
	    break;
	}
    }
    return (len > len0 ? len : len0);
}

char *ofname = "-", *sfname;	/* filenames for reports */
char *record;			/* record name */

void init(argc, argv)
int argc;
char *argv[];
{
    int i;
    char *prog_name();
    void help();

    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'a':	/* annotator names follow */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
		     "%s: reference and test annotator names must follow -a\n",
		       pname);
		exit(1);
	    }
	    an[0].name = argv[i];
	    an[1].name = argv[++i];
	    break;
	  case 'c':	/* condensed output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -c\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 1;
	    break;
	  case 'C':	/* condensed output, with SVE run statistics */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -C\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 4;
	    break;
	  case 'f':	/* start time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,"%s: start time must follow -f\n", pname);
		exit(1);
	    }
	    start = i;	/* save arg index, convert to samples later, when
			   record has been opened and sampling frequency is
			   known */
	    break;
	  case 'h':	/* print usage summary */
	    help();
	    exit(1);
	    break;
	  case 'l':	/* line-format output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -l\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 2;
	    break;
	  case 'L':	/* line-format output, with SVE run statistics */
	    if (++i >= argc-1) {
		(void)fprintf(stderr,
			      "%s: two output file names must follow -L\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    sfname = argv[++i];
	    fflag = 5;
	    break;
	  case 'r':	/* record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* standard-format output */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -s\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 3;
	    break;
	  case 'S':	/* standard-format output, with SVE run statistics */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: output file name must follow -S\n",
			pname);
		exit(1);
	    }
	    ofname = argv[i];
	    fflag = 6;
	    break;
	  case 't':	/* end time follows */
	    if (++i >= argc) {
		(void)fprintf(stderr, "%s: end time must follow -t\n", pname);
		exit(1);
	    }
	    end_time = i;
	    break;
	  case 'v':	/* verbose mode */
	    verbose = 1;
	    break;
	  case 'w':	/* match window follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: match window must follow -w\n", pname);
		exit(1);
	    }
	    match_dt = i;
	    break;
	  default:
	    (void)fprintf(stderr,
			  "%s: unrecognized option %s\n", pname, argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr,
			  "%s: unrecognized argument %s\n",pname,argv[i]);
	    exit(1);
	}
    }

    if (!record || !an[0].name) {
	help();
	exit(1);
    }

    if (start != 0L || end_time != 0L || match_dt != 0)
	(void)fprintf(stderr,"%s: (warning) nonstandard comparison selected\n",
		pname);

    if (sampfreq(record) <= 0) {
	(void)fprintf(stderr,
		      "%s: (warning) %g Hz sampling frequency assumed\n",
		pname, WFDB_DEFFREQ);
	(void)setsampfreq(WFDB_DEFFREQ);
    }

    /* Set the match window, and the times of the start and end of the test
       period. */
    if (match_dt)
	match_dt = (int)strtim(argv[match_dt]);
    else
	match_dt = (int)strtim(".15");		/* 150 milliseconds */
    if (start) {
	start = strtim(argv[(int)start]);
	if (start < (WFDB_Time)0) start = -start;
    }
    else
	start = strtim("5:0");			/* 5 minutes */
    if (end_time) {
	end_time = strtim(argv[(int)end_time]);
	if (end_time < (WFDB_Time)0) end_time = -end_time;
    }
    else if ((end_time = strtim("e")) == 0L)
	end_time = -1L;		/* record length unavailable -- go to end of
				   reference annotation file */
    if (end_time > 0L && end_time < start) {
	(void)fprintf(stderr, "%s: improper interval specified\n", pname);
	exit(1);
    }
    an[0].stat = an[1].stat = WFDB_READ;
    if (annopen(record, an, 2) < 0) exit(2);
}

void print_results(type)
int type;	/* 0: VE; 1: SVE */
{
    int i;
    long CTPs, CFN, CTPp, CFP, STPs, SFN, STPp, SFP, LTPs, LFN, LTPp, LFP;

    /* Open output files.  If line-format output was selected, write column
       headings only if the files must be created from scratch. */
    if (type == 0) {
	if (strcmp(ofname, "-")) {
	    if ((ofile = fopen(ofname, "r")) == NULL) {
		if ((ofile = fopen(ofname, "w")) == NULL) {
		    (void)fprintf(stderr, "%s: can't create %s\n",
				  pname, ofname);
		    exit(3);
		}
		if (fflag == 2 || fflag == 5) {
		    (void)fprintf(ofile,
		     "Record CTs CFN CTp CFP STs SFN STp SFP LTs LFN LTp LFP");
		    (void)fprintf(ofile, "  CSe C+P SSe S+P LSe L+P\n");
		}
	    }
	    else {
		(void)fclose(ofile);
		if ((ofile = fopen(ofname, "a")) == NULL) {
		    (void)fprintf(stderr, "%s: can't modify %s\n",
				  pname, ofname);
		    exit(3);
		}
	    }
	}
	else ofile = stdout;
    }
    if (type == 1 && fflag == 5) {
	if (strcmp(ofname, "-"))
	    (void)fclose(ofile);
	if (strcmp(sfname, "-")) {
	    if ((ofile = fopen(sfname, "r")) == NULL) {
		if ((ofile = fopen(sfname, "w")) == NULL) {
		    (void)fprintf(stderr,
				  "%s: can't create %s\n", pname, sfname);
		    exit(3);
		}
		(void)fprintf(ofile, "(SVE run detection)\n");
		(void)fprintf(ofile,
		     "Record CTs CFN CTp CFP STs SFN STp SFP LTs LFN LTp LFP");
		(void)fprintf(ofile,
			      "  CSe C+P SSe S+P LSe L+P\n");
	    }
	    else {
		(void)fclose(ofile);
		if ((ofile = fopen(sfname, "a")) == NULL) {
		    (void)fprintf(stderr,
				  "%s: can't modify %s\n", pname, sfname);
		    exit(3);
		}
	    }
	}
	else ofile = stdout;
    }
    else sfile = stdout;

    if (fflag == 1 || fflag == 3 || fflag == 4 || fflag == 6) {
	(void)fprintf(ofile,
		      "%sVE run-by-run comparison results for record %s\n",
		      type ? "S" : "", record);
	(void)fprintf(ofile, "Reference annotator: %s\n", an[0].name);
	(void)fprintf(ofile, "     Test annotator: %s\n\n", an[1].name);
    }

    /* Calculate statistics. */
    CTPs = s[0][2][2] + s[0][2][3] + s[0][2][4] + s[0][2][5] + s[0][2][6];
    CFN  = s[0][2][0] + s[0][2][1];
    CTPp = s[1][2][2] + s[1][3][2] + s[1][4][2] + s[1][5][2] + s[1][6][2];
    CFP  = s[1][0][2] + s[1][1][2];
    STPs = s[0][3][3] + s[0][3][4] + s[0][3][5] + s[0][3][6] +
	   s[0][4][3] + s[0][4][4] + s[0][4][5] + s[0][4][6] +
	   s[0][5][3] + s[0][5][4] + s[0][5][5] + s[0][5][6];
    SFN  = s[0][3][0] + s[0][3][1] + s[0][3][2] +
	   s[0][4][0] + s[0][4][1] + s[0][4][2] +
	   s[0][5][0] + s[0][5][1] + s[0][5][2];
    STPp = s[1][3][3] + s[1][3][4] + s[1][3][5] +
	   s[1][4][3] + s[1][4][4] + s[1][4][5] +
	   s[1][5][3] + s[1][5][4] + s[1][5][5] +
	   s[1][6][3] + s[1][6][4] + s[1][6][5];
    SFP =  s[1][0][3] + s[1][0][4] + s[1][0][5] +
	   s[1][1][3] + s[1][1][4] + s[1][1][5] +
	   s[1][2][3] + s[1][2][4] + s[1][2][5];
    LTPs = s[0][6][6];
    LFN  = s[0][6][0] + s[0][6][1] + s[0][6][2] +
	   s[0][6][3] + s[0][6][4] + s[0][6][5];
    LTPp = s[1][6][6];
    LFP  = s[1][0][6] + s[1][1][6] + s[1][2][6] +
	   s[1][3][6] + s[1][4][6] + s[1][5][6];

    /* Output section. */
    switch (fflag) {
      case 1:	/* print condensed format summary tables */
      case 4:
	(void)fprintf(ofile, "          Algorithm Run Length\n");
	(void)fprintf(ofile, "         nc    v    c    s    l\n");
	(void)fprintf(ofile, "     __________________________\n");
	(void)fprintf(ofile, "  NC |      %4ld %4ld %4ld %4ld\n",
		s[0][0][1], s[0][0][2],
		s[0][0][3]+s[0][0][4]+s[0][0][5], s[0][0][6]);
	(void)fprintf(ofile, "   V | %4ld %4ld %4ld %4ld %4ld\n",
		s[0][1][0], s[0][1][1], s[0][1][2],
		s[0][1][3]+s[0][1][4]+s[0][1][5], s[0][1][6]);
	(void)fprintf(ofile, "   C | %4ld %4ld %4ld %4ld %4ld\n",
		s[0][2][0], s[0][2][1], s[0][2][2],
		s[0][2][3]+s[0][2][4]+s[0][2][5], s[0][2][6]);
	(void)fprintf(ofile, "   S | %4ld %4ld %4ld %4ld %4ld\n",
		s[0][3][0]+s[0][4][0]+s[0][5][0],
		s[0][3][1]+s[0][4][1]+s[0][5][1],
		s[0][3][2]+s[0][4][2]+s[0][5][2],
		s[0][3][3]+s[0][4][3]+s[0][5][3]+
		s[0][3][4]+s[0][4][4]+s[0][5][4]+
		s[0][3][5]+s[0][4][5]+s[0][5][5],
		s[0][3][6]+s[0][4][6]+s[0][5][6]);
	(void)fprintf(ofile, "   L | %4ld %4ld %4ld %4ld %4ld\n",
		s[0][6][0], s[0][6][1], s[0][6][2],
		s[0][6][3]+s[0][6][4]+s[0][6][5], s[0][6][6]);
	(void)fprintf(ofile, "\n    (Run Sensitivity Summary Matrix)\n\n\n");
	(void)fprintf(ofile, "          Algorithm Run Length\n");
	(void)fprintf(ofile, "         nc    v    c    s    l\n");
	(void)fprintf(ofile, "     __________________________\n");
	(void)fprintf(ofile, "  NC |      %4ld %4ld %4ld %4ld\n",
		s[1][0][1], s[1][0][2],
		s[1][0][3]+s[1][0][4]+s[1][0][5], s[1][0][6]);
	(void)fprintf(ofile, "   V | %4ld %4ld %4ld %4ld %4ld\n",
		s[1][1][0], s[1][1][1], s[1][1][2],
		s[1][1][3]+s[1][1][4]+s[1][1][5], s[1][1][6]);
	(void)fprintf(ofile, "   C | %4ld %4ld %4ld %4ld %4ld\n",
		s[1][2][0], s[1][2][1], s[1][2][2],
		s[1][2][3]+s[1][2][4]+s[1][2][5], s[1][2][6]);
	(void)fprintf(ofile, "   S | %4ld %4ld %4ld %4ld %4ld\n",
		s[1][3][0]+s[1][4][0]+s[1][5][0],
		s[1][3][1]+s[1][4][1]+s[1][5][1],
		s[1][3][2]+s[1][4][2]+s[1][5][2],
		s[1][3][3]+s[1][4][3]+s[1][5][3]+
		s[1][3][4]+s[1][4][4]+s[1][5][4]+
		s[1][3][5]+s[1][4][5]+s[1][5][5],
		s[1][3][6]+s[1][4][6]+s[1][5][6]);
	(void)fprintf(ofile, "   L | %4ld %4ld %4ld %4ld %4ld\n",
		s[1][6][0], s[1][6][1], s[1][6][2],
		s[1][6][3]+s[1][6][4]+s[1][6][5], s[1][6][6]);
	(void)fprintf(ofile, "\n (Run Positive Predictivity Summary Matrix)\n\n");
	break;
      case 2:	/* print line-format output */
      case 5:
	(void)fprintf(ofile,
	   "%4s %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld %3ld ",
		record,
	   CTPs, CFN, CTPp, CFP, STPs, SFN, STPp, SFP, LTPs, LFN, LTPp, LFP);
	break;
      case 3:	/* print standard format summary tables */
      case 6:
	(void)fprintf(ofile, "               Algorithm Run Length\n");
	(void)fprintf(ofile, "          0    1    2    3    4    5   >5\n");
	(void)fprintf(ofile, "     ____________________________________\n");
	(void)fprintf(ofile, "   0 |      %4ld %4ld %4ld %4ld %4ld %4ld\n",
		s[0][0][1], s[0][0][2], s[0][0][3],
		s[0][0][4], s[0][0][5], s[0][0][6]);
	for (i = 1; i < 6; i++)
	    (void)fprintf(ofile, "   %d | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n", i,
		    s[0][i][0], s[0][i][1], s[0][i][2], s[0][i][3],
		    s[0][i][4], s[0][i][5], s[0][i][6]);
	(void)fprintf(ofile, "  >5 | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		s[0][6][0], s[0][6][1], s[0][6][2], s[0][6][3],
		s[0][6][4], s[0][6][5], s[0][6][6]);
	(void)fprintf(ofile, "\n      (Run Sensitivity Summary Matrix)\n\n\n");
	(void)fprintf(ofile, "               Algorithm Run Length\n");
	(void)fprintf(ofile, "          0    1    2    3    4    5   >5\n");
	(void)fprintf(ofile, "     ____________________________________\n");
	(void)fprintf(ofile, "   0 |      %4ld %4ld %4ld %4ld %4ld %4ld\n",
		s[1][0][1], s[1][0][2], s[1][0][3],
		s[1][0][4], s[1][0][5], s[1][0][6]);
	for (i = 1; i < 6; i++)
	    (void)fprintf(ofile, "   %d | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n", i,
		    s[1][i][0], s[1][i][1], s[1][i][2], s[1][i][3],
		    s[1][i][4], s[1][i][5], s[1][i][6]);
	(void)fprintf(ofile, "  >5 | %4ld %4ld %4ld %4ld %4ld %4ld %4ld\n",
		s[1][6][0], s[1][6][1], s[1][6][2], s[1][6][3],
		s[1][6][4], s[1][6][5], s[1][6][6]);
	(void)fprintf(ofile, "\n (Run Positive Predictivity Summary Matrix)\n\n");
	break;
    }
    pstat("            Couplet sensitivity", CTPs, CTPs + CFN);
    pstat("  Couplet positive predictivity", CTPp, CTPp + CFP);
    pstat("          Short run sensitivity", STPs, STPs + SFN);
    pstat("Short run positive predictivity", STPp, STPp + SFP);
    pstat("           Long run sensitivity", LTPs, LTPs + LFN);
    pstat(" Long run positive predictivity", LTPp, LTPp + LFP);
    (void)fprintf(ofile, "\n");
}

static char *help_strings[] = {
 "usage: %s -r RECORD -a REF TEST [OPTIONS ...]\n",
 "where RECORD is the record name;  REF is reference annotator name;  TEST is",
 "the test annotator name; and OPTIONS may include any of:",
 " -c FILE       append condensed reports (AAMI RP Table 10 format) to FILE",
 " -C FILE       as for -c, but report SVE run statistics also",
 " -f TIME       begin comparison at specified TIME (default: 5 minutes",
 "                after beginning of record)",
 " -h            print this usage summary",
 " -l FILE       append line-format reports (AAMI RP Table 13 format) to FILE",
 " -L FILE FILE2 as for -l, but report SVE run statistics in FILE2",
 " -s FILE       append standard reports (AAMI RP Table 9 format) to FILE",
 " -S FILE       as for -S, but report SVE run statistics also",
 " -t TIME       stop comparison at specified TIME (default: end of record",
 "                if defined, end of reference annotation file otherwise;",
 "                if TIME is 0, the comparison ends when the end of either",
 "                annotation file is reached)",
 " -v            verbose mode:  list all discrepancies",
 " -w TIME       set match window (default: 0.15 seconds)",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
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
