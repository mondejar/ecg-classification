/* file: wave.h		G. Moody	26 April 1990
			Last revised:   13 July 2010
Constants, macros, global variables, and function prototypes for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2010 George B. Moody

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

#include <stdio.h>	/* standard I/O library definitions */
#include <wfdb/wfdb.h>	/* WFDB library constants, typedefs, and prototypes */
#include <wfdb/ecgcodes.h>	/* needed for definition of ACMAX */

/* Constants */

/* WAVE attempts to intuit the user's next display request, and maintains
   a cache of signal display lists that correspond to WAVE's guesses as
   well as of recently displayed segments.  Reduce this parameter if necessary
   to conserve memory, but do not reduce it below 2. */
#define MAX_DISPLAY_LISTS	8

/* Bits for display mode (passed from main() to initialize_graphics()). */
#define MODE_MONO	1    /* use monochrome mode even on a color screen */
#define MODE_GREY	2    /* use shades of grey even on a color screen */
#define MODE_SHARED	4    /* use shared color map only */
#define MODE_OVERLAY	8    /* use overlays */

/* Mask bits for next_match() and previous_match(). */
#define M_ANNTYP	1
#define M_SUBTYP	2
#define M_CHAN		4
#define M_NUM		8
#define M_AUX		16
#define M_MAP2		32

/* Marker types.  Markers are displayed and handled like annotations, but are
   not written to the output annotation file. */
#define INDEX_MARK	(ACMAX+1)
#define	BEGIN_ANALYSIS	(ACMAX+2)
#define END_ANALYSIS	(ACMAX+3)
#define REF_MARK	(ACMAX+4)

/* These can be increased harmlessly.  Since record names and log file
   names can include path information, it is probably best not to decrease
   ANLMAX or LNLMAX. */
#define ANLMAX	39	/* length of longest permissible annotator name */
#define LNLMAX	79	/* length of longest permissible log file name */
#define RNLMAX	79	/* length of longest permissible record name */
#define DSLMAX	80	/* length of longest permissible description in log */

/* Default array indices for scale settings (see modepan.c and xvwave.c). */
#define DEF_TSA_INDEX 12
#define DEF_COARSE_TSA_INDEX 6
#define MAX_COARSE_TSA_INDEX 8
#define DEF_VSA_INDEX 3

/* Macros */

/* These macros convert arguments in millimeters into numbers of pixels.  Use
mmx and mmy with positive arguments for proper rounding; dmmx and dmmy return
floating-point results. */
#define dmmx(A)	((A)*dpmmx)
#define dmmy(A)	((A)*dpmmy)
#define mmx(A)	((int)(dmmx(A) + 0.5))
#define mmy(A)	((int)(dmmy(A) + 0.5))

/* These convert arguments in millivolts or seconds to numbers of pixels;
   the results are floating-point quantities. */
#define millivolts(A)	((A)*(canvas_height/canvas_height_mv))
#define seconds(A)	((A)*(canvas_width/canvas_width_sec))

/* The name of the default text editor is difficult to specify portably.  Any
   system with the XView libraries should have `textedit', but it may be in
   /usr/openwin/bin, /usr/openwin/bin/xview, /usr/bin/xview, or somewhere else.
   Rather than specifying a possibly erroneous absolute path, we just give the
   filename, and hope it's in the user's PATH.  Of course, the user may prefer
   `emacs', or `vi', or something else;  an alternative can be specified using
   the EDITOR environment variable or the `Wave.TextEditor' resource. */
#ifndef EDITOR
#define EDITOR		"textedit"
#endif

/* Definitions and declarations of global variables appear below.  When
   compiling `xvwave.c', INIT is defined;  at other times, INIT must not be
   defined.  If INIT is defined, COMMON becomes an empty string and the
   statements beginning with COMMON are syntactically definitions (i.e.,
   storage is allocated);  otherwise, COMMON becomes `extern' and these
   statements become declarations.  Since declarations cannot contain
   initializers, any initialization of these variables (other than to zero)
   must be done elsewhere. */
#ifdef INIT
#define COMMON
#else
#define COMMON extern
#endif

/* Global variables. */
COMMON char *pname;			/* the name by which WAVE was invoked
					   (less any path information). */
COMMON int wave_ppid;			/* PID of the WAVE process that is the
					   parent of this one, if any */
COMMON int make_sync_button;		/* if non-zero, this is the last WAVE
					   process in this group -- make a
					   sync button on the main panel */
COMMON char record[RNLMAX+1];		/* record name */
COMMON char **signame;			/* signal names */
COMMON char **sigunits;			/* signal units */
COMMON char *calibrated;		/* 0: not calibrated, 1: calibrated */
COMMON WFDB_Sample *scope_v;		/* sample vector (see scope.c) */

COMMON WFDB_Sample *vref;		/* see edit.c */
COMMON WFDB_Sample *level_v;
COMMON char **level_name_string;
COMMON char **level_value_string;
COMMON char **level_units_string;

COMMON WFDB_Sample *v;			/* see sig.c */
COMMON WFDB_Sample *v0;
COMMON WFDB_Sample *vmax;
COMMON WFDB_Sample *vmin;
COMMON int *vvalid;

COMMON double *vmag;			/* user-adjustable multiplier for
					   resizing signal scales */
COMMON double *vscale;			/* amplitude scales for each signal
					   (pixels/adu) */
COMMON double vscalea;			/* amplitude scale for annotation num
					   fields shown as a signal */
COMMON int *base;			/* baseline ordinate for each signal */
COMMON int *dc_coupled;			/* if non-zero, signal is DC coupled
					   (i.e., has a defined baseline) */
COMMON int *sigbase;			/* baselines for DC-coupled signals */
COMMON char **blabel;			/* baseline label strings */

COMMON char *cfname;			/* calibration file name */
COMMON char *helpdir;			/* name of directory containing on-line
					   help files for WAVE */
COMMON char log_file_name[LNLMAX+1];	/* name of log file, if any */
COMMON char description[DSLMAX+1];	/* description from log file */
COMMON int nsig;			/* number of signals */
COMMON int nann;			/* number of annotators (0 or 1) */
COMMON char annotator[ANLMAX+1];	/* annotator name */
COMMON WFDB_Anninfo af;			/* annotator info, passed to annopen */
COMMON int savebackup;			/* if non-zero, the current annotation
					   file should be backed up if any
					   edits are made */
COMMON double dpmmx, dpmmy;		/* screen resolution, in pixels/mm */
COMMON int canvas_height, canvas_width;	/* sig window dimensions, in pixels */
COMMON double canvas_height_mv;		/* sig window height, in millivolts */
COMMON double canvas_width_sec;		/* sig window width, in seconds */
COMMON int nsamp;			/* sig window width, in samples */
COMMON int linesp;			/* text line spacing, in pixels */
COMMON long display_start_time;		/* sample number at left edge of signal
					   window */
COMMON long begin_analysis_time;	/* sample number of BEGIN_ANALYSIS
					   marker (or -1 if none) */
COMMON long end_analysis_time;		/* sample number of END_ANALYSIS
					   marker (or -1 if none) */
COMMON long ref_mark_time;		/* sample number of REF_MARK marker
					   (or -1 if none) */
COMMON double freq;			/* sampling frequency (Hz) */
COMMON int atimeres;			/* annot time resolution in samples */
COMMON double tmag;			/* user-adjustable multiplier for
					   resizing time scale */
COMMON double tscale;			/* time scale (pixels/samp interval) */
COMMON double mmpermv;			/* amplitude scale for export */
COMMON double mmpersec;			/* time scale for export */
COMMON int abase;			/* baseline ordinate for annotation
					   display */
COMMON int use_overlays;		/* if non-zero, the grid, cursor,
					   annotations, and signals can be
					   erased independently */
COMMON int ghflag;			/* 1: horizontal grid lines enabled */
COMMON int gvflag;			/* 1: vertical grid lines enabled */
COMMON int visible;			/* 1: grid is visible */
COMMON int accept_edit;			/* 1: editing is allowed */
COMMON int show_subtype;		/* 1: show annotation subtypes */
COMMON int show_chan;			/* 1: show annotation `chan' fields */
COMMON int show_num;			/* 1: show annotation `num' fields */
COMMON int show_aux;			/* 1: show annotation `aux' fields */
COMMON int show_marker;			/* 1: show annotation markers */
COMMON int show_signame;		/* 1: show signal names */
COMMON int show_baseline;		/* 1: show baselines if DC-coupled */
COMMON int show_level;			/* 1: show level of selected annot */

COMMON int tsa_index;			/* time scale array index (see
					   set_modes, in modepan.c) */
COMMON int coarse_tsa_index;		/* as above, for records sampled at
					   10 Hz or below */
COMMON int fine_tsa_index;		/* as above, for other records */
COMMON int vsa_index;			/* amplitude scale array index */

COMMON int ann_mode;			/* 0: show annotations centered
					   1: show annotations attached to
					      signals (by `chan' field)
					   2: show annotations as a signal
					      (by `num' field) */
COMMON int overlap;			/* 1: allow displayed annotations to
					   overlap */
COMMON int sig_mode;			/* 0: show all signals
					   1: show signals in siglist only */
COMMON int time_mode;			/* 0: show elapsed times (h:m:s)
					   1: show absolute times if known
					   2: show times in sample intervals */
COMMON int grid_mode;			/* 0: no grid
					   1: horizontal grid lines only
					   2: vertical grid lines only
					   3: horizontal and vertical grid
					   4: fine horizontal and vertical grid
					   5: coarse horiz, normal vert grid
					   6: coarse horiz, fine vert grid */
COMMON int coarse_grid_mode;		/* as above, for use when tsa_index <=
					   MAX_COARSE_TSA_INDEX */
COMMON int fine_grid_mode;		/* as above, for use at other times */
COMMON int signal_choice;		/* signal to be analyzed (from analyze
					   panel) */
COMMON int *siglist;			/* signals to be analyzed (from analyze
					   panel) */
COMMON int siglistlen;			/* number of valid siglist entries */
COMMON int maxsiglistlen;		/* length of siglist array */
COMMON int freeze_siglist;		/* 1: don't rebuild in record_init */
COMMON int scan_active;			/* 1: scope display is running */

COMMON char psprint[80];		/* command used to print PostScript */
COMMON char textprint[80];		/* command used to print text */
COMMON char url[512];			/* link to external data */

/* The fields of the annotation template specify what action is to be taken
   during an edit.  If ann_template.anntyp is NOTQRS, then changing an existing
   annotation is equivalent to deleting it.  Otherwise, ann_template's anntyp,
   subtyp, chan, num, and aux fields are copied into any inserted or changed
   annotations. */
COMMON struct WFDB_ann ann_template;

/* The fields of the search template specify the match criteria for annotation
   searches.  Not all fields are necessarily used, however;  the search mask
   (below) determines which fields of the search template must be matched. */
COMMON struct WFDB_ann search_template;
COMMON int search_mask;

/* The annotation list is built from `ap' structures; ap_start and ap_end
   point to the first and last annotations in the list, and annp is a
   general-use pointer that usually points to an annotation in the current
   region of interest.  During editing, `attached' points to the annotation
   that is to be changed, if any. `scope_annp' points to the annotation
   associated with the waveform most recently drawn in the scope window. */
struct ap {
    struct WFDB_ann this;
    struct ap *previous, *next;
};
COMMON struct ap *ap_start, *ap_end, *annp, *attached, *scope_annp;

/* Function prototypes for ANSI C and C++ compilers.  If you attempt to compile
   WAVE with a C++ compiler, you will need to change the function definitions
   in the *.c sources to `new-style' definitions.  There are a few more
   function prototypes in `xvwave.h', which include all those that use X or
   XView objects in their argument lists. */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)

extern int record_init(char *record_name);	/* in init.c */
extern void set_baselines(void);		/* in init.c */
extern void calibrate(void);			/* in init.c */
extern void set_record_item(char *record_name);	/* in mainpan.c */
extern void set_annot_item(char *annot_name);	/* in mainpan.c */
extern void set_start_time(char *time_string);	/* in mainpan.c */
extern void set_end_time(char *time_string);	/* in mainpan.c */
extern void set_find_item(char *find_string);	/* in mainpan.c */
extern void show_search_template(void);		/* in search.c */
extern void create_mode_popup(void);		/* in modepan.c */
extern void show_mode(void);			/* in modepan.c */
extern void set_modes(void);			/* in modepan.c */
extern void dismiss_mode(void);			/* in modepan.c */
extern long wstrtim(char *tstring);		/* in modepan.c */
extern char *wtimstr(long t);			/* in modepan.c */
extern char *wmstimstr(long t);			/* in modepan.c */
extern void help(void);				/* in helppan.c */
extern void create_help_popup(void);		/* in helppan.c */
extern void show_help(void);			/* in helppan.c */
extern void dismiss_help(void);			/* in helppan.c */
extern void show_log(void);			/* in logpan.c */
extern void finish_log(void);			/* in logpan.c */
extern void start_demo(void);			/* in logpan.c */
extern void show_ann_template(void);		/* in annpan.c */
extern void set_anntyp(int annotation_code);	/* in annpan.c */
extern void set_ann_aux(char *auxstring);	/* in annpan.c */
extern void set_ann_subtyp(int subtyp);		/* in annpan.c */
extern void set_ann_chan(int chan);		/* in annpan.c */
extern void set_ann_num(int num);		/* in annpan.c */
extern void reset_ref(void);			/* in edit.c */
extern void recreate_level_popup(void);		/* in edit.c */
extern void bar(int x, int y, int do_bar);	/* in edit.c */
extern void box(int x, int y, int do_box);	/* in edit.c */
extern void restore_cursor(void);		/* in edit.c */
extern void restore_grid(void);			/* in grid.c */
extern void show_grid(void);			/* in grid.c */
extern void sig_highlight();			/* in signal.c */
extern int in_siglist();			/* in signal.c */
extern void do_disp(void);			/* in signal.c */
extern void clear_cache(void);			/* in signal.c */
extern int sigy(int sig, int x);		/* in signal.c */
extern struct ap *get_ap(void);			/* in annot.c */
extern int annot_init(void);			/* in annot.c */
extern long next_match(struct WFDB_ann *template,	/* in annot.c */
		       int mask);
extern long previous_match(struct WFDB_ann *template,/* in annot.c */
			   int mask);
extern void show_annotations(long start_time,	/* in annot.c */
			     int duration);
extern void clear_annotation_display(void);	/* in annot.c */
extern struct ap *locate_annotation(long time, int chan); /* in annot.c */
extern void delete_annotation(long time, int chan); /* in annot.c */
extern void move_annotation(struct ap *a,	/* in annot.c */
			    long time);
extern void insert_annotation(struct ap *a);	/* in annot.c */
extern void change_annotations(void);		/* in annot.c */
extern void check_post_update(void);		/* in annot.c */
extern int post_changes(void);			/* in annot.c */
extern void set_frame_title(void);		/* in annot.c */
extern void analyze_proc(void);			/* in analyze.c */
extern void reset_start(void);			/* in analyze.c */
extern void reset_stop(void);			/* in analyze.c */
extern void reset_maxsig(void);			/* in analyze.c */
extern void reset_siglist(void);		/* in analyze.c */
extern void open_url(void);			/* in analyze.c */
extern void do_command(char *command);		/* in analyze.c */
extern void set_signal_choice(int sig);		/* in analyze.c */
extern void set_siglist_from_string(char *s);	/* in analyze.c */
extern void add_signal_choice(void);		/* in analyze.c */
extern void delete_signal_choice(void);		/* in analyze.c */
extern void print_proc(void);			/* in analyze.c */
extern void save_scope_params(int use_overlays,	/* in scope.c */
			      int use_color,
			      int grey);
extern void show_scope_window(void);		/* in scope.c */
extern void strip_x_args(int *argc,char **argv);/* in xvwave.c */
extern int initialize_graphics(int mode);	/* in xvwave.c */
extern void hide_grid(void);			/* in xvwave.c */
extern void unhide_grid(void);			/* in xvwave.c */
extern void display_and_process_events(void);	/* in xvwave.c */
extern void quit_proc(void);			/* in xvwave.c */
extern void sync_other_wave_processes(void);	/* in xvwave.c */

/* Function return types for K&R C compilers. */
#else
extern char *wmstimstr(), *wtimstr();
extern int record_init(), annot_init(), post_changes(), initialize_graphics(),
    sigy(), in_siglist();
extern long next_match(), previous_match(), wstrtim();
extern struct ap *get_ap(), *locate_annotation();
extern void set_baselines(), calibrate(), set_record_item(), set_annot_item(),
    set_start_time(), set_end_time(), set_find_item(), show_search_template(),
    create_mode_popup(), show_mode(),
    set_modes(), dismiss_mode(), help(), create_help_popup(), show_help(),
    dismiss_help(), show_log(), dismiss_log(), start_demo(),
    show_ann_template(), set_anntyp(), set_ann_aux(), set_ann_subtyp(),
    set_ann_chan(), set_ann_aux(), bar(), box(),
    restore_cursor(), restore_grid(), show_grid(), sig_highlight(), do_disp(),
    clear_cache(), show_annotations(), clear_annotation_display(),
    delete_annotation(), move_annotation(), insert_annotation(),
    change_annotations(), check_post_update(), set_frame_title(),
    analyze_proc(), reset_start(), reset_stop(), reset_maxsig(),
    reset_siglist(), open_url(), do_command(), set_signal_choice(),
    set_siglist_from_string(), add_signal_choice(), delete_signal_choice(),
    print_proc(), save_scope_params(), show_scope_window(), strip_x_args(),
    hide_grid(), unhide_grid(), display_and_process_events(), quit_proc(),
    sync_other_wave_processes(), reset_ref(), recreate_level_popup();
#endif
