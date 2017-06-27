/* file: wave.c		G. Moody	27 April 1990
			Last revised:	10 June 2005
main() function for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2005 George B. Moody

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

#include "wave.h"
#include "xvwave.h"
#include <unistd.h>		/* for getpid declaration */

/* Spot help files for WAVE are located in HELPDIR/wave.  Their names are
   of the form `HELPDIR/wave/*.info'. */
#ifndef HELPDIR
#define HELPDIR		"/usr/local/help"
#endif

main(argc, argv)
int argc;
char *argv[];
{
    char *wfdbp, *hp, *p, *start_string = NULL, *tp, *getenv();
    int do_demo = 0, i, j, mode = 0;
    static char *helppath;
    static int wave_procno;
    void set_frame_footer();

    /* Extract program name for use in error messages. */
    for (p = pname = argv[0]; *p; p++)
	if (*p == '/') pname = p+1;

    /* Initialize non-zero global variables. */
    begin_analysis_time = end_analysis_time = -1L;
    tsa_index = vsa_index = ann_mode = overlap = sig_mode = time_mode =
	grid_mode = -1;
    tscale = 1.0;

    if ((helpdir = getenv("HELPDIR")) == NULL) helpdir = HELPDIR;

    /* Find the record name argument. */
    for (i = 1; i < argc-1; i++) {
	if (strcmp(argv[i], "-r") == 0)
	    break;
    }

    /* Provide (non-window based) usage information if needed. */
    if (i == argc-1 || argc == 1 || getenv("DISPLAY") == NULL)
	help();	/* print info and quit */

    /* Set the path for XView spot help. */
    if ((hp = getenv("HELPPATH")) == NULL)
	hp = "/usr/lib/help";
    helppath = (char *)malloc(strlen(hp) + strlen(helpdir) + 16);
    	/* (strlen("HELPPATH=") + strlen(":") + strlen("/wave") + 1 = 16) */
    if (helppath) {
	sprintf(helppath, "HELPPATH=%s:%s/wave", hp, helpdir);
	putenv(helppath);
    }

    /* Check for requests to open more than one record. */
    strcpy(record, argv[++i]);
    while (p = strchr(record, '+')) {
	static int wave_pid;

	wave_pid = (int)getpid();	/* save the PID of this process */
	if (fork() == 0) {		/* child */
	    wave_ppid = wave_pid;
	    wave_procno++;
	    p++; tp = record;
	    while (*tp++ = *p++)
		;	/* delete the first record name from the string */
	    strcpy(argv[i], record);
	    if (strchr(record, '+') == NULL)
		make_sync_button = 1;
	}
	else {	/* parent */
	    *p = '\0';	/* delete everything after the first record from the
			   string */
	    strcpy(argv[i], record);
	}
    }

    /* Remove any command-line arguments that are specific to another WAVE
       process, and remove the `+n/' prefix from any that are specific to
       this process. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '+') {
	    if ((atoi(argv[i]+1) == wave_procno) && (p = strchr(argv[i], '/')))
	      argv[i] = p+1;
	    else {
		int j;
		for (j = i+1; j < argc; j++)
		    argv[j-1] = argv[j];
		argc--;
	    }
	}
    }

    /* Process window system related command-line arguments. */
    strip_x_args(&argc, argv);

    /* Process WAVE-specific arguments. */
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-')
	    switch (*(argv[i] + 1)) {
	      case 'a':	/* annotator name */
		if (++i >= argc) {
		    fprintf(stderr, "%s: annotator name must follow -a\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > ANLMAX) {
		    fprintf(stderr, "%s: annotator name is too long\n",
			    pname);
		    exit(1);
		}
		strcpy(annotator, argv[i]);
		break;
	      case 'd':	/* display resolution */
		if (strcmp(argv[i], "-dpi")) break;
		if (++i >= argc) {
		    fprintf(stderr, "%s: resolution must follow -dpi\n",
			    pname);
		    exit(1);
		}
		(void)sscanf(argv[i], "%lfx%lf", &dpmmx, &dpmmy);
		if (dpmmx < 0.) dpmmx = 0.;
		if (dpmmy <= 0.) dpmmy = dpmmx;
		dpmmx /= 25.4; dpmmy /= 25.4;
		break;
	      case 'D': /* demo mode */
		if (++i >= argc) {
		    fprintf(stderr, "%s: log file name must follow -D\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > LNLMAX) {
		    fprintf(stderr, "%s: log file name is too long\n", pname);
		    exit(1);
		}
		strcpy(log_file_name, argv[i]);
		do_demo = 1;
		break;
	      case 'f':	/* start at time specified in following argument */
		if (++i >= argc) {
		    fprintf(stderr, "%s: start time must follow -f\n",
			    pname);
		    exit(1);
		}
		start_string = argv[i];
		if (start_string[0] == '[')
		    time_mode = 1;	/* display absolute times */
		else if (start_string[0] == 's')
		    time_mode = 2;	/* display times in sample intervals */
		break;
	      case 'g':	/* use shades of grey, even on color screen */
		mode |= MODE_GREY;
		break;
	      case 'H':	/* use getvec's high-resolution mode for
			   multi-frequency records */
	        setgvmode(WFDB_HIGHRES);
		break;
	      case 'm':	/* run in monochrome, even on color screen */
		mode |= MODE_MONO;
		break;
	      case 'O':	/* use overlay mode */
		mode |= MODE_OVERLAY;
		break;
	      case 'p': /* add argument to WFDB path */
		if (++i >= argc) {
		    fprintf(stderr,
			    "%s: input file location(s) must follow -p\n",
			    pname);
		    exit(1);
		}
		wfdb_addtopath(argv[i]);
		break;
	      case 'r':	/* record name */
		if (++i >= argc) {
		    fprintf(stderr, "%s: record name must follow -r\n",
			    pname);
		    exit(1);
		}
		if (strlen(argv[i]) > RNLMAX) {
		    fprintf(stderr, "%s: record name is too long\n",
			    pname);
		    exit(1);
		}
		strcpy(record, argv[i]);
		break;
	      case 's':	/* signal list follows */
	        /* count the number of signals */
		for (j = 0; ++i < argc && *argv[i] != '-'; j++)
		    ;
		if (j == 0) {
		    (void)fprintf(stderr,
			     "%s: one or more signal numbers must follow -s\n",
				  pname);
		    exit(1);
		}
		/* allocate storage for the signal list */
		if (siglistlen + j > maxsiglistlen) {
		    if ((siglist = realloc(siglist,
				      (siglistlen+j) * sizeof(int))) == NULL ||
			(base = realloc(base,
				      (siglistlen+j) * sizeof(int))) == NULL ||
			(level = realloc(level,
				 (siglistlen+j) * sizeof(XSegment))) == NULL) {
			(void)fprintf(stderr, "%s: insufficient memory\n",
				      pname);
			exit(2);
		    }
		    maxsiglistlen = siglistlen + j;
		}
		/* fill the signal list */
		for (i -= j; i < argc && *argv[i] != '-'; )
		    siglist[siglistlen++] = atoi(argv[i++]);
		i--;
		sig_mode = 1;	/* display listed signals only (may be
				   overridden using -VS 0) */
		break;
	      case 'S':   /* use shared color map only */
		mode |= MODE_SHARED;
		break;
	      case 'V':	/* View options */
		switch (*(argv[i]+2)) {
		  case 'a':	/* display annotation `aux' fields */
		    show_aux = 1;
		    break;
		  case 'A':	/* set annotation display mode */
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: annotation display mode must follow -VA\n",
				pname);
			exit(1);
		    }
		    ann_mode = atoi(argv[i]);
		    break;
		  case 'b':	/* display signal baselines */
		    show_baseline = 1;
		    break;
		  case 'c':	/* display annotation `chan' fields */
		    show_chan = 1;
		    break;
		  case 'G':	/* set grid display mode */
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: grid display mode must follow -VG\n",
				pname);
			exit(1);
		    }
		    grid_mode = coarse_grid_mode = fine_grid_mode =
			atoi(argv[i]);
		    break;
		  case 'l':	/* display cursor levels */
		    show_level = 1;
		    break;
		  case 'm':	/* display annotation markers */
		    show_marker = 1;
		    break;
		  case 'n':	/* display annotation `num' fields */
		    show_num = 1;
		    break;
		  case 'N':	/* display signal names */
		    show_signame = 1;
		    break;
		  case 's':	/* display annotation subtypes */
		    show_subtype = 1;
		    break;
		  case 'S':	/* set signal display mode */
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: signal display mode must follow -VS\n",
				pname);
			exit(1);
		    }
		    sig_mode = atoi(argv[i]);
		    break;
		  case 't':	/* set time scale array index */
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: time scale choice must follow -Vt\n",
				pname);
			exit(1);
		    }
		    tsa_index = coarse_tsa_index = fine_tsa_index =
			atoi(argv[i]);
		    break;
		  case 'T':	/* set time display mode */
		    if (++i >= argc) {
			fprintf(stderr,
			       "%s: time display mode must follow -VT\n",
				pname);
			exit(1);
		    }
		    time_mode = atoi(argv[i]);
		    break;
		  case 'v':	/* set amplitude scale array index */
		    if (++i >= argc) {
			fprintf(stderr,
				"%s: amplitude scale choice must follow -Vv\n",
				pname);
			exit(1);
		    }
		    vsa_index = atoi(argv[i]);
		    break;
		}
		break;
	    }
    }

    /* Provide help if no record was specified. */
    if (record[0] == '\0') help();

    /* Set up base frame, quit if unsuccessful. */
    if (initialize_graphics(mode)) exit(1);

    /* Make sure that the WFDB path begins with an empty component (otherwise,
       annotation editing may become confused). */
    wfdbp = getwfdb();
    if (*wfdbp != ':') {
	char *nwfdbp;

	if ((nwfdbp = (char *)malloc(strlen(wfdbp)+2)) == NULL) {
	    fprintf(stderr, "%s: memory allocation error\n", pname);
	    exit(1);
	}
	sprintf(nwfdbp, ":%s", wfdbp);
	setwfdb(nwfdbp);
    }

    /* Make sure there is a calibration database defined. */
    if (!getenv("WFDBCAL"))
	putenv("WFDBCAL=wfdbcal");
    /* Initialize the annotation table. */
    (void)read_anntab();

    /* Open the selected record (and annotation file, if specified). */
    if (record_init(record)) {
	if (annotator[0]) {
	    af.name = annotator; af.stat = WFDB_READ;
	    nann = 1;
	    annot_init();
	}
    }

    /* Indicate that the input annotation file must be saved before writing
       any edits. */
    savebackup = 1;

    /* If requested, set the start time. */
    if (start_string) {
	display_start_time = strtim(start_string);
	if (start_string[0] == '[') {
	    if (display_start_time > 0L)
		/* a time before the beginning of the record was requested --
		   go to the beginning of the record instead */
		display_start_time = 0;
	    else
		/* strtim converts a valid absolute time string into a negated
		   sample number -- change it to a positive sample number */
		display_start_time = -display_start_time;
	}
    }
    set_frame_footer();

    /* If requested, start demonstration. */
    if (do_demo)
      start_demo();

    /* Do initial display */
    XFillRectangle(display, osb, clear_all, 0, 0, canvas_width+mmx(10), canvas_height);
    do_disp();

    /* Enter the main loop. */
    display_and_process_events();

    exit(0);
}
