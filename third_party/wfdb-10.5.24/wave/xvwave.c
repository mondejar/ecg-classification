/* file: xvwave.c	G. Moody	27 April 1990
			Last revised:  28 October 2009
XView support functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2009 George B. Moody

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

#define INIT
#include "wave.h"
#include "xvwave.h"
#include "bitmaps.h"
#include <pwd.h>		/* for struct passwd definition */
#include <signal.h>		/* for SIGUSR1 definition */
#include <unistd.h>		/* for getpid definition */
#include <xview/canvas.h>
#include <xview/cms.h>		/* for Xv_singlecolor definition */
#include <xview/cursor.h>
#include <xview/defaults.h>
#include <xview/win_input.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <xview/window.h>

#ifdef SOLARIS
#include <sys/systeminfo.h>
#define gethostname(B, C)  sysinfo(SI_HOSTNAME, (B), (C))
#endif

#ifndef RESDIR
#define RESDIR "/usr/lib/X11/app-defaults"
#endif

static int in_xv_main_loop;
static int restore_all = 1;

static GC bg_fill;

/* Handle exposures in the signal window. */
void repaint(canvas, paint_window, repaint_area)
Canvas canvas;
Xv_Window paint_window;
Rectlist *repaint_area;
{
    if (!in_xv_main_loop) return;
    if (restore_all) {
	restore_all = 0;
	XFillRectangle(display, osb, bg_fill,
		       0, 0, canvas_width, canvas_height);
    }
    restore_grid();
    do_disp();
    restore_cursor();
}

static void do_resize(canvas, width, height)
Canvas canvas;
int width;
int height;
{
    int canvas_width_mm, u;
    Pixmap old_osb;
    /* The manipulations below are intended to select a canvas width that
       will allow a (reasonably) standard time interval to be displayed.
       If the canvas is at least 125 mm wide, its width is truncated to
       a multiple of 25 mm;  otherwise, its width is truncated to a multiple
       of 5 mm. */
    canvas_width_mm = width/dmmx(1);
    if (canvas_width_mm > 125) {
	canvas_width_sec = canvas_width_mm / 25;
	canvas_width_mm = 25 * canvas_width_sec;
    }
    else {
	u = canvas_width_mm / 5;
	canvas_width_mm = u * 5;
	canvas_width_sec = u * 0.2;
    }
    canvas_width = mmx(canvas_width_mm);

    /* Similar code might be used to readjust the canvas height, but doing so
       seems unnecessary. */
    canvas_height = height;
    canvas_height_mv = canvas_height / dmmy(10);
    
    /* get a new off screen buffer  */
    old_osb = osb;
    osb = XCreatePixmap(display, xid, width, canvas_height,
			DefaultDepth(display, DefaultScreen(display)));
    XFillRectangle(display, osb, bg_fill,
		   0, 0, width, canvas_height);
    XSetWindowBackgroundPixmap(display, xid, osb);
    if (old_osb) XFreePixmap(display, old_osb);
    
    /* Recalibrate based on selected scales, clear the display list cache. */
    if (*record) {
	set_baselines();
	alloc_sigdata(nsig > 2 ? nsig : 2);
	dismiss_mode();	/* read and set user-selected scales, if any */

	vscale[0] = 0.;	/* force clear_cache() -- see calibrate() */
	calibrate();
    }

    restore_all = 1;	/* be sure that the background is repainted */
    repaint(canvas, NULL, NULL);
}

/* Handle resize events. */
static void resize(canvas, width, height)
Canvas canvas;
int width;
int height;
{
    if (in_xv_main_loop) do_resize(canvas, width, height);
}

/* This function handles interpretation of XView options and initialization of
   the resource (defaults) database.  X applications are supposed to read
   resources in this order:
     Classname file in the app-defaults directory
     Classname file in XUSERFILESEARCHPATH, if set
     Classname file in XAPPLRESDIR, if set and XUSERFILESEARCHPATH is not set
     Server database, or .Xdefaults in the user's home directory if the
       XResourceManagerString macro returns NULL
     File specified by XENVIRONMENT, or .Xdefaults-hostname in the user's
       home directory if XENVIRONMENT is not set
     Command line arguments
   Some time ago, I wrote the following paragraph here:

  "XView makes it very difficult to do this, since xv_init calls Xrminitialize,
   reads ~/.Xdefaults AND the server database, and then sets additional
   resources based on command-line arguments, which it then removes from the
   argument list.  This function follows the standard procedure for reading
   resources, but does not attempt to reread the command-line arguments;  for
   this reason, it is not possible to set WAVE-specific resources using the
   XView -default or -xrm options."

   The last statement is not quite correct, since the -default and -xrm options
   *do* work.  The catch is that settings made in this way can be overridden by
   settings made in any of the resource files, which is contrary to the
   standard X way of doing things.  So it's best not to depend on -default and
   -xrm, since (depending on the resource files) they may or may not have the
   desired effect.
*/
void strip_x_args(pargc, argv)
int *pargc;
char *argv[];
{
    char dfname[256], *resdir, *tmp, *getenv();
    extern int fullscreendebug;

    fullscreendebug = 1;  /* work around bug in Xorg 1.6+ by disabling grabs */
    xv_init(XV_INIT_ARGC_PTR_ARGV, pargc, argv, NULL);

    display = XOpenDisplay(NULL);

    /* Set HOME environment variable if necessary. */
    if (getenv("HOME") == NULL) {
	static char homedir[80];
	int uid;
	struct passwd *pw;

	if (tmp = getenv("USER"))
	    pw = getpwnam(tmp);
	else {
	    uid = getuid();
	    pw = getpwuid(uid);
	}
	sprintf(homedir, "HOME=%s", pw ? pw->pw_dir : ".");
	putenv(homedir);
    }

    if ((resdir = getenv("RESDIR")) == NULL) resdir = RESDIR;
    sprintf(dfname, "%s/Wave", resdir);
    defaults_load_db(dfname);
    if ((tmp=getenv("XUSERFILESEARCHPATH")) || (tmp=getenv("XAPPLRESDIR"))) {
	sprintf(dfname, "%s/Wave", tmp);
	defaults_load_db(dfname);
    }
    if (XResourceManagerString(display))
	defaults_load_db(NULL);		/* read server database */
    else {
	sprintf(dfname, "%s/.Xdefaults", getenv("HOME"));
	defaults_load_db(dfname);
    }
    if (tmp = getenv("XENVIRONMENT"))
	defaults_load_db(tmp);
    else {
	int len;

	sprintf(dfname, "%s/.Xdefaults-", getenv("HOME"));
	len = strlen(dfname);
	gethostname(dfname+len, 256-len);
	defaults_load_db(dfname);
    }
}

static int allowdottedlines;
static int graphicsmode;

/* Save current settings as new defaults. */
void save_defaults()
{
    char tstring[256], *getenv();

    sprintf(tstring, "%gx%g", 25.4*dpmmx, 25.4*dpmmy);
    defaults_set_boolean("Wave.AllowDottedLines", allowdottedlines);
    defaults_set_string("Wave.Dpi", tstring);
    defaults_set_integer("Wave.SignalWindow.Height_mm",
			 (int)(canvas_height/(dpmmy*10))*10 + 10);
    defaults_set_integer("Wave.SignalWindow.Width_mm",
			 (int)(canvas_width/(dpmmx*25))*25 + 25);
    defaults_set_integer("Wave.GraphicsMode", graphicsmode);
    defaults_set_boolean("Wave.View.Subtype", show_subtype);
    defaults_set_boolean("Wave.View.Chan", show_chan);
    defaults_set_boolean("Wave.View.Num", show_num);
    defaults_set_boolean("Wave.View.Aux", show_aux);
    defaults_set_boolean("Wave.View.Markers", show_marker);
    defaults_set_boolean("Wave.View.SignalNames", show_signame);
    defaults_set_boolean("Wave.View.Baselines", show_baseline);
    defaults_set_boolean("Wave.View.Level", show_level);
    if (sampfreq(NULL) >= 10.0)
	defaults_set_integer("Wave.View.TimeScale", tsa_index);
    else
	defaults_set_integer("Wave.View.CoarseTimeScale", tsa_index);
    defaults_set_integer("Wave.View.AmplitudeScale", vsa_index);
    defaults_set_integer("Wave.View.AnnotationMode", ann_mode);
    defaults_set_integer("Wave.View.AnnotationOverlap", overlap);
    defaults_set_integer("Wave.View.SignalMode", sig_mode);
    defaults_set_integer("Wave.View.TimeMode", time_mode);
    if (tsa_index > MAX_COARSE_TSA_INDEX)
	defaults_set_integer("Wave.View.GridMode", grid_mode);
    else
	defaults_set_integer("Wave.View.CoarseGridMode", grid_mode);

    sprintf(tstring, "%s/.Xdefaults", getenv("HOME"));
    defaults_store_db(tstring);
}

static char sentinel[30];

Notify_value destroy_func(client, status)
Notify_client client;
Destroy_status status;
{
    if (status == DESTROY_CHECKING) {
	int result;

	result = notice_prompt((Frame)client, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS, "Are you sure you want to Quit?", NULL,
				   NOTICE_BUTTON_YES, "Confirm",
				   NOTICE_BUTTON_NO, "Cancel",
				   NULL);
	if (result != NOTICE_YES)
	    notify_veto_destroy(client);
    } else if (status == DESTROY_CLEANUP) {
	if (post_changes())
	    finish_log();
	unlink(sentinel);	/* remove the sentinel file */
	/* allow frame to be destroyed */
	return notify_next_destroy_func(client, status);
    }
    return NOTIFY_DONE;
}

Notify_value accept_remote_command(client, sig, when)
Notify_client client;
int sig;
Notify_signal_mode when;
{
    char buf[80], new_annotator[30], new_time[30], new_record[70],
	new_siglist[70];
    FILE *sfile;
    extern void wfdb_addtopath();

    sfile = fopen(sentinel, "r");
    new_annotator[0] = new_time[0] = new_record[0] = new_siglist[0] = '\0';
    while (fgets(buf, sizeof(buf), sfile)) {	/* read a command */
	if (buf[0] != '-') continue;	/* illegal command -- ignore */
	else switch (buf[1]) {
	  case 'a':	/* (re)open annotator */
	      new_annotator[sizeof(new_annotator)-1] = '\0';
	      strncpy(new_annotator, buf+3, sizeof(new_annotator)-1);
	      new_annotator[strlen(new_annotator)-1] = '\0';
	      break;
	  case 'f':	/* go to specified time */
	      new_time[sizeof(new_time)-1] = '\0';
	      strncpy(new_time, buf+3, sizeof(new_time)-1);
	      new_time[strlen(new_time)-1] = '\0';
	      break;
	  case 'p':	/* add path to WFDB path variable */
	      wfdb_addtopath(buf+3);
	      break;
	  case 'r':	/* (re)open record */
	      new_record[sizeof(new_record)-1] = '\0';
	      strncpy(new_record, buf+3, sizeof(new_record)-1);
	      new_record[strlen(new_record)-1] = '\0';
	      break;
	  case 's':	/* choose signals for display */
	      new_siglist[sizeof(new_siglist-1)] = '\0';
	      strncpy(new_siglist, buf+3, sizeof(new_siglist)-1);
	      new_siglist[strlen(new_siglist)-1] = '\0';
	}
    }
    if (*new_record && strcmp(record, new_record)) {
	/* If new_record contains a list of records, and the current record
	   is one of them, don't change records. */
	char *p = new_record, *q;
	int change_record = 1;

	while (*p && (q = strchr(p, '+'))) {
	    if (strncmp(p, record, q-p) == 0) {
		/* the current record is in this list */
		change_record = 0;
		break;
	    }
	    p = q+1;
	}
	if (change_record) set_record_item(p);
	if (p > new_record) p--;
	*p = '\0';
    }
    if (*new_annotator && strcmp(annotator, new_annotator))
	set_annot_item(new_annotator);
    if (*new_time)
	set_start_time(new_time);
    if (*new_siglist) {
	set_siglist_from_string(new_siglist);
	if (sig_mode == 0) {
	    sig_mode = 1;	/* display listed signals only */
	    mode_undo();	/* change setting in View panel to match */
	    set_baselines();	/* recalculate display positions of signals */
	}
	freeze_siglist = 1;
	/* avoid undoing the effects of setting siglist here, by
	   suppressing the rebuild of siglist that would normally be
	   done by record_init (see init.c) */
    }
    fclose(sfile);
    if (wave_ppid) {	/* synch parent WAVE, if any, with this one */
	if (*new_time == '\0')
	    strcpy(new_time, mstimstr(display_start_time));
	if (*new_record)
	    sprintf(buf, "wave-remote -pid %d -r %s -f '%s'",
		    wave_ppid, new_record, new_time);
	else
	    sprintf(buf, "wave-remote -pid %d -f '%s'", wave_ppid, new_time);
	if (*new_siglist) {
	    strcat(buf, " -s ");
	    strcat(buf, new_siglist);
	}
	strcat(buf, "\n");
	system(buf);
    }
    disp_proc(XV_NULL, (Event *) '.');	/* redraw display */
    fclose(fopen(sentinel, "w"));
    return NOTIFY_DONE;
}

void sync_other_wave_processes()
{
    char buf[80];

    if (wave_ppid) {
	sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
		mstimstr(-display_start_time));
	system(buf);
    }
}

/* After determining the display's size, resolution, and color capabilities,
`initialize_graphics' opens the base frame, and invokes `create_main_panel' to
set up the main control panel.  The remainder of the function sets up the
signal window;  this task includes setting display parameters based on
the contents of the resource database and various command-line options, setting
up a color map for the display, and defining the graphics contexts (GCs) used
to draw and erase the grid, editing cursor, annotations, and signals, as well
as installing a cursor and an event handler.  The most complex part of the
function deals with color usage.  WAVE selects a color model by querying the X
server to determine the capabilities of the display (and by the `-g', `-m',
and `-S' options).

Normally WAVE will use color if the display supports it;  you may select the
greyscale mode on a color display with the `-g' command-line option, or the
monochrome mode on a color or greyscale display with the `-m' option.  If the
color map is writeable (i.e., for PseudoColor or GrayScale visuals) and large
enough, WAVE draws independently-erasable overlays for the grid, signals,
editing cursor, and annotations;  this feature can be disabled using the `-S'
option (which may be used in combination with `-g').

Colors are normally assigned as follows:

Logical color		Greyscale representation	Color representation
background_color	    white			    white
grid_color		    grey75 (75% white, 25% black)   grey90
cursor_color		    grey50			    orange red
annotation_color	    grey25			    yellow green
signal_color		    black			    blue

These defaults may be overridden by resources named Wave.SignalWindow.TTT.CCC,
where `TTT' is either `Grey' or `Color', and `CCC' is one of `Background',
`Grid', `Cursor', `Annotation', or `Signal' (or their lower-case equivalents).
The `Color.*' resources are used only if the display is color-capable and
neither greyscale nor monochrome mode has been specified.

In monochrome mode, the background color is normally white, and all others
are normally black;  the reverse can be obtained by setting the resource named
Wave.SignalWindow.Mono.Background to `black'.

If the color map can be modified (i.e., if the X server supports the GrayScale,
PseudoColor, or DirectColor models), WAVE attempts to allocate 16 read/write
cells in 4 plane groups, with one plane group assigned to each color other than
the background_color.  This permits the grid, cursor, annotations, and signals
to be drawn and erased independently using suitably defined graphics contexts.
The plane groups are set up to provide opaque overlays (i.e., the RGB values
are determined by the highest-indexed non-zero mask bit).  Using this scheme,
the grid overlays the background, the cursor overlays the grid and background,
annotations overlay the cursor, grid, and background, and signals overlay
everything else.  For example, suppose that XAllocColorCells returns:
	            mask[0] = 0000 0001
		    mask[1] = 0000 0010
		    mask[2] = 0000 0100
		    mask[3] = 0000 1000
	     pixel_table[0] = 1111 0000
In this case, the color map would be set up thus:
	    color map index   color
		  1111 0000   background_color
		  1111 0001   grid_color
		  1111 001x   cursor_color
		  1111 01xx   annotation_color
		  1111 1xxx   signal_color
where all numbers in the example are binary, and `x' can be either 0 or 1.  In
addition, the grid can be made to appear and disappear without redrawing it,
simply by modifying the RGB values assigned to the grid_color cell.

If the color map cannot be modified, or if 16 read/write cells cannot be
allocated in it, or if you have used the `-s' command-line option in order to
conserve color resources, this program does not perform the color manipulations
described in the previous paragraph.  In such cases, this program redraws
the grid as required.  If the server is slow, this is sufficiently distracting
that you may prefer not to display the grid.

If fewer than five colors are available, two or more logical colors share a
slot in the color map.  This can be done in two ways, depending on the number
of available colors.  If there are exactly four colors (as on the NeXT),
cursor_color is mapped to annotation_color (i.e., the value of the cursor color
resource is ignored, and the cursor is drawn in the color that has been
assigned to annotations).  Otherwise, all colors except background_color are
mapped to signal_color, but distinct line styles are used for grid and cursor
drawing;  this mode may be selected on other types of displays using the `-m'
command-line option.

After the color map has been set up, the first `ncolors' entries in the
`color' array contain the RGB values and color map indices corresponding to
each of the colors used by WAVE.  The `pixel_table' array contains copies of
the color map indices, used when setting the graphics contexts and during
color map manipulation by `hide_grid' and `unhide_grid'. */

static Colormap colormap;
static XColor color[16];
static unsigned long pixel_table[16];
static int background_color, grid_color, cursor_color, annotation_color,
    signal_color;
#ifndef USE_OLC_PLUS
Server_image cursor_image;
#endif

int initialize_graphics(mode)
int mode;
{
    char *annfontname, *cname, *rstring;
    int i, j;
    int height_mm, width_mm;		/* screen dimensions, in millimeters */
    int height_px, width_px;		/* screen dimensions, in pixels */
    int hmmpref = 120, wmmpref = 250;	/* preferred height and width of signal
					   window, in millimeters */
    int use_color = 0;			/* if non-zero, there are >=4 colors */
    int grey = 0;			/* if non-zero, use grey shades only */
    int ncolors;
    static unsigned long mask[4];
    Canvas canvas;
    Icon icon;
    Panel main_panel;
    Screen *screen;
    Visual *visual;
    static XGCValues gcvalues;
    Xv_Cursor cursor;
    Xv_singlecolor cursor_fg, cursor_bg;
    Xv_Window window;

    /* Get information about the dimensions, resolution, and type of the
       display available to this program. */
    screen = DefaultScreenOfDisplay(display);

    if (dpmmx == 0) {
        rstring = defaults_get_string("wave.dpi", "Wave.Dpi", "0x0");
	(void)sscanf(rstring, "%lfx%lf", &dpmmx, &dpmmy);
	dpmmx /= 25.4;	/* convert dots/inch into dots/mm */
	dpmmy /= 25.4;
    }
    height_mm = HeightMMOfScreen(screen);
    height_px = HeightOfScreen(screen);
    if (height_mm > 0) {
	if (dpmmy == 0.) dpmmy = ((double)height_px)/height_mm;
	else height_mm = height_px/dpmmy;
    }
    else { dpmmy = DPMMY; height_mm = height_px/dpmmy; }
    width_mm = WidthMMOfScreen(screen);
    width_px = WidthOfScreen(screen);
    if (width_mm > 0) {
	if (dpmmx == 0.) dpmmx = ((double)width_px)/width_mm;
	else width_mm = width_px/dpmmx;
    }
    else { dpmmx = DPMMX; width_mm = width_px/dpmmx; }

    visual = DefaultVisualOfScreen(screen);
    use_color = (visual->map_entries >= 4);
    switch (ClassOfVisual(visual)) {
      case StaticGray:	grey = 1; break;
      case StaticColor:	break;
      case TrueColor:	break;
      case GrayScale:	grey = use_overlays = 1; break;
      case PseudoColor:	use_overlays = 1; break;
      case DirectColor:	use_overlays = 1; break;
      default:
	fprintf(stderr, "%s: unsupported display type, visual class %d\n",
		pname, ClassOfVisual(visual));
	return (1);
    }
    if (mode == 0)
	mode = defaults_get_integer("wave.graphicsmode",
				    "Wave.GraphicsMode",
				    MODE_OVERLAY);
    if (mode & MODE_GREY) grey = 1;
    if (mode & MODE_MONO) use_color = grey = use_overlays = 0;
    if (mode & MODE_SHARED) use_overlays = 0;
    graphicsmode = mode;

    /* Choose default sizes for the frame and the signal window. */
    if (width_mm < 53 || height_mm < 75) {
	fprintf(stderr, "%s: display too small\n", pname);
	return (1);
    }
    wmmpref = defaults_get_integer("wave.signalwindow.width_mm",
				   "Wave.SignalWindow.Width_mm",
				   250);
    if (wmmpref > width_mm - 3) wmmpref = width_mm - 3;
    else if (wmmpref < 50) wmmpref = 50;
    hmmpref = defaults_get_integer("wave.signalwindow.height_mm",
				   "Wave.SignalWindow.Height_mm",
				   120);
    if (hmmpref > height_mm - 25) hmmpref = height_mm - 25;
    else if (hmmpref < 50) hmmpref = 50;
    canvas_width = mmx(wmmpref);
    canvas_height = mmy(hmmpref);
    linesp = mmy(4);

    /* Create frame for the main window. */
    frame = xv_create(XV_NULL, FRAME,
	XV_LABEL, pname,
	XV_HEIGHT, canvas_height + mmy(15),	/* allow 15 mm for main_panel -
						   any extra will be stolen
						   from canvas_height */
	XV_WIDTH, canvas_width + 4,		/* allow 4 pixels for canvas
						   and frame borders */
     /*	FRAME_NO_CONFIRM, FALSE,	*/	/* force confirmation on
						   quit */
	FRAME_SHOW_FOOTER, TRUE,
	FRAME_LEFT_FOOTER, "",
	FRAME_RIGHT_FOOTER, "",
	NULL);

    /* Create the sentinel file. */
    sprintf(sentinel, "/tmp/.wave.%d.%d", (int)getuid(), (int)getpid());
    fclose(fopen(sentinel, "w"));	/* create as an empty file */

    /* Install the destroy_func signal handler for cleanup before exiting. */
    notify_interpose_destroy_func(frame, destroy_func);

    /* Install the remote control signal handler. */
    notify_set_signal_func(frame, accept_remote_command, SIGUSR1, NOTIFY_SYNC);

    /* Create the `View' panel, read the resource database, and set the `View'
       options accordingly. */
    create_mode_popup();

    if (!show_subtype)
	show_subtype = defaults_get_boolean("wave.view.subtype",
					    "Wave.View.Subtype",
					    0);
    if (!show_chan)
	show_chan    = defaults_get_boolean("wave.view.chan",
					    "Wave.View.Chan",
					    0);
    if (!show_num)
	show_num     = defaults_get_boolean("wave.view.num",
					    "Wave.View.Num",
					    0);
    if (!show_aux)
	show_aux     = defaults_get_boolean("wave.view.aux",
					    "Wave.View.Aux",
					    0);
    if (!show_marker)
	show_marker  = defaults_get_boolean("wave.view.markers",
					    "Wave.View.Markers",
					    0);
    if (!show_signame)
	show_signame = defaults_get_boolean("wave.view.signalnames",
					    "Wave.View.SignalNames",
					    0);
    if (!show_baseline)
	show_baseline= defaults_get_boolean("wave.view.baselines",
					    "Wave.View.Baselines",
					    0);
    if (!show_level)
	show_level   = defaults_get_boolean("wave.view.level",
					    "Wave.View.Level",
					    0);
    if (tsa_index < 0) {
	tsa_index = fine_tsa_index =
	               defaults_get_integer("wave.view.timescale",
					    "Wave.View.TimeScale",
					    DEF_TSA_INDEX);
	coarse_tsa_index = defaults_get_integer("wave.view.coarsetimescale",
					    "Wave.View.CoarseTimeScale",
					    DEF_COARSE_TSA_INDEX);
    }
    if (vsa_index < 0)
	vsa_index    = defaults_get_integer("wave.view.amplitudescale",
					    "Wave.View.AmplitudeScale",
					    DEF_VSA_INDEX);
    if (ann_mode < 0)
	ann_mode     = defaults_get_integer("wave.view.annotationmode",
					    "Wave.View.AnnotationMode",
					    0);
    if (overlap < 0)
    	overlap      = defaults_get_integer("wave.view.annotationoverlap",
					    "Wave.View.AnnotationOverlap",
					    0);
    if (sig_mode < 0)
	sig_mode     = defaults_get_integer("wave.view.signalmode",
					    "Wave.View.SignalMode",
					    0);
    if (time_mode < 0)
	time_mode    = defaults_get_integer("wave.view.timemode",
					    "Wave.View.TimeMode",
					    0);
    if (grid_mode < 0) {
	grid_mode    = fine_grid_mode =
	               defaults_get_integer("wave.view.gridmode",
					    "Wave.View.GridMode",
					    0);
	coarse_grid_mode = defaults_get_integer("wave.view.coarsegridmode",
					    "Wave.View.CoarseGridMode",
					    0);
    }
    /* Set the controls in the `View' panel to match the variables above. */
    mode_undo();

    icon_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
					 XV_WIDTH,  64,
					 XV_HEIGHT, 64,
					 SERVER_IMAGE_X_BITS, icon_bits,
					 NULL);
    icon = xv_create(frame, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "WAVE",
		     NULL);
    xv_set(frame, FRAME_ICON, icon, NULL);

    main_panel = create_main_panel();
    window_fit_height(main_panel);

    canvas = xv_create(frame, CANVAS,
	CANVAS_REPAINT_PROC, repaint,
	CANVAS_RESIZE_PROC, resize,
	CANVAS_WIDTH, canvas_width,
	CANVAS_HEIGHT, canvas_height,
	CANVAS_MIN_PAINT_WIDTH, mmx(50),
	CANVAS_MIN_PAINT_HEIGHT, mmy(20),
	CANVAS_AUTO_CLEAR, TRUE,
	WIN_X, 0,
	WIN_BELOW, main_panel,
	WIN_DYNAMIC_VISUAL, use_overlays,
	0);

    /* Get the display and paint window of the canvas, and the X ID (drawable)
       of its paint window. */
    display = (Display *)xv_get(canvas, XV_DISPLAY);
    window = (Xv_Window)xv_get(canvas, CANVAS_NTH_PAINT_WINDOW, 0);
    xid = (Window)xv_get(window, XV_XID);

    /* Set soft function key labels for the canvas. */
    xv_set(canvas_paint_window(canvas), WIN_SOFT_FNKEY_LABELS,
	   "Help\nMiddle\nDrag left\nDrag right\n\nCopy\n\n\nFind\n>\n\n\n",
	   WIN_EVENT_PROC, disp_proc,
	   NULL);

    /* Determine the color usage. */
    background_color = 0;
    grid_color = 1;
    mask[0] = mask[1] = mask[2] = mask[3] = ~0;
    if (!use_color) {
	cursor_color = 1;
	annotation_color = 1;
	signal_color = 1;
	cname = defaults_get_string("wave.signalwindow.mono.background",
				    "Wave.SignalWindow.Mono.Background",
				    "white");
	if (strcmp(cname, "black") && strcmp(cname, "Black")) {
	    pixel_table[0] = WhitePixel(display, DefaultScreen(display));
	    pixel_table[1] = BlackPixel(display, DefaultScreen(display));
	    cursor_bg.red = cursor_bg.green = cursor_bg.blue = 255;
	    cursor_fg.red = cursor_fg.green = cursor_fg.blue = 0;
	}
	else {	/* reverse-video monochrome mode */
	    pixel_table[0] = BlackPixel(display, DefaultScreen(display));
	    pixel_table[1] = WhitePixel(display, DefaultScreen(display));
	    cursor_bg.red = cursor_bg.green = cursor_bg.blue = 0;
	    cursor_fg.red = cursor_fg.green = cursor_fg.blue = 255;
	}
	ncolors = 2;
    }	
    else {
	/* Allocate a colormap for the signal window. */
	colormap = DefaultColormap(display, DefaultScreen(display));

	/* Try to get read/write color cells if possible. */
	if (use_overlays && XAllocColorCells(display, colormap, 0, mask, 4,
					     pixel_table, 1)) {
	    cursor_color = 2;
	    annotation_color = 4;
	    signal_color = 8;
	    ncolors = 16;
	}

	/* Otherwise, set up for using the shared colormap. */
	else {
	    use_overlays = 0;
	    if (visual->map_entries >= 5) {
		cursor_color = 2;
		annotation_color = 3;
		signal_color = 4;
		ncolors = 5;
	    }
	    /* If we are here, there must be exactly four colors available. */
	    else {
		cursor_color = 2;
		annotation_color = 2;
		signal_color = 3;
		ncolors = 4;
	    }
	}

	/* Set the desired RGB values. */
	if (grey)
	    cname = defaults_get_string("wave.signalwindow.grey.background",
					"Wave.SignalWindow.Grey.Background",
					"white");
	else
	    cname = defaults_get_string("wave.signalwindow.color.background",
					"Wave.SignalWindow.Color.Background",
					"white");
	XParseColor(display, colormap, cname, &color[background_color]);

	if (grey)
	    cname = defaults_get_string("wave.signalwindow.grey.grid",
					"Wave.SignalWindow.Grey.Grid",
					"grey75");
	else
	    cname = defaults_get_string("wave.signalwindow.color.grid",
					"Wave.SignalWindow.Color.Grid",
					"grey90");
	XParseColor(display, colormap, cname, &color[grid_color]);

	if (cursor_color != annotation_color) {
	    if (grey)
		cname = defaults_get_string("wave.signalwindow.grey.cursor",
					    "Wave.SignalWindow.Grey.Cursor",
					    "grey50");
	    else
		cname = defaults_get_string("wave.signalwindow.color.cursor",
					    "Wave.SignalWindow.Color.Cursor",
					    "orange red");
	    XParseColor(display, colormap, cname, &color[cursor_color]);
	}

	if (grey)
	    cname = defaults_get_string("wave.signalwindow.grey.annotation",
					"Wave.SignalWindow.Grey.Annotation",
					"grey25");
	else
	    cname = defaults_get_string("wave.signalwindow.color.annotation",
					"Wave.SignalWindow.Color.Annotation",
					"yellow green");
	XParseColor(display, colormap, cname, &color[annotation_color]);

	if (cursor_color == annotation_color)
	    color[cursor_color] = color[annotation_color];

	if (grey)
	    cname = defaults_get_string("wave.signalwindow.grey.signal",
					"Wave.SignalWindow.Grey.Signal",
					"black");
	else
	    cname = defaults_get_string("wave.signalwindow.color.signal",
					"Wave.SignalWindow.Color.Signal",
					"blue");
	XParseColor(display, colormap, cname, &color[signal_color]);

	if (use_overlays) {
	    color[3] = color[2];
	    color[7] = color[6] = color[5] = color[4];
	    color[15] = color[14] = color[13] = color[12] = color[11] =
		color[10] = color[9] = color[8];
	    for (i = 0; i < ncolors; i++) {
		color[i].pixel = pixel_table[0];
		if (i & 1) color[i].pixel |= mask[0];
		if (i & 2) color[i].pixel |= mask[1];
		if (i & 4) color[i].pixel |= mask[2];
		if (i & 8) color[i].pixel |= mask[3];
		color[i].flags = DoRed | DoGreen | DoBlue;
	    }
	    XStoreColors(display, colormap, color, ncolors);
	}
	else {
	    (void)XAllocColor(display, colormap, &color[background_color]);
	    (void)XAllocColor(display, colormap, &color[grid_color]);
	    (void)XAllocColor(display, colormap, &color[cursor_color]);
	    if (cursor_color != annotation_color)
		(void)XAllocColor(display, colormap, &color[annotation_color]);
	    else
		color[annotation_color] = color[cursor_color];
	    (void)XAllocColor(display, colormap, &color[signal_color]);
	}

	/* Get the indices into the color map and store them. */
	for (i = 0; i < ncolors; i++)
	    pixel_table[i] = color[i].pixel;

	/* Record the cursor colors in terms of the XView color model (8 bits
	   for each of red, green, blue, rather than 16 as in the X model). */
	cursor_fg.red = color[cursor_color].red >> 8;
	cursor_fg.green = color[cursor_color].green >> 8;
	cursor_fg.blue = color[cursor_color].blue >> 8;
	cursor_bg.red = color[background_color].red >> 8;
	cursor_bg.green = color[background_color].green >> 8;
	cursor_bg.blue = color[background_color].blue >> 8;
    }

    /* Create the graphics contexts for writing into the signal window.
       The signal window (except for the grid) is erased using clear_all. */
    gcvalues.foreground = gcvalues.background = pixel_table[background_color];
    gcvalues.plane_mask = use_overlays ? ~pixel_table[grid_color] : ~0;
    clear_all = XCreateGC(display,xid,GCForeground|GCPlaneMask, &gcvalues);

    /* The grid is drawn using draw_grd, and erased using clear_grd. */
    gcvalues.foreground = pixel_table[grid_color];
    gcvalues.plane_mask = use_overlays ? gcvalues.foreground : ~0;
    allowdottedlines = defaults_get_boolean("wave.allowdottedlines",
					    "Wave.AllowDottedLines",
					    1);
    if (allowdottedlines && !use_color)
	gcvalues.line_style = LineOnOffDash;
    else
	gcvalues.line_style = LineSolid;
    gcvalues.dashes = 1;
    draw_grd = XCreateGC(display, xid, GCBackground | GCForeground |
			 GCLineStyle | GCDashList | GCPlaneMask, &gcvalues);
    gcvalues.line_width = 2;
    draw_cgrd = XCreateGC(display, xid, GCBackground | GCForeground |
			  GCLineWidth | GCLineStyle | GCDashList | GCPlaneMask,
			  &gcvalues);
    if (use_overlays) {
	gcvalues.foreground = gcvalues.background;
	gcvalues.plane_mask = mask[0];
	clear_grd = XCreateGC(display, xid, GCForeground | GCPlaneMask,
			      &gcvalues);
    }
    else
	clear_grd = clear_all;

    /* Editing cursors (boxes and bars) are drawn using draw_crs, and erased
       using clear_crs.  If we can't use a read/write color map for this
       purpose, the two GCs are identical, and we use GXxor to do the
       drawing. */
    if (allowdottedlines && !use_color)
	gcvalues.line_style = LineOnOffDash;
    else
	gcvalues.line_style = LineSolid;
    if (use_overlays) {
	gcvalues.foreground = pixel_table[cursor_color];
	gcvalues.plane_mask = gcvalues.foreground;
	gcvalues.function =  GXcopy;
	draw_crs = XCreateGC(display, xid,
			     GCBackground | GCForeground | GCFunction |
			     GCLineStyle | GCPlaneMask, &gcvalues);
	gcvalues.foreground = gcvalues.background;
	gcvalues.plane_mask = mask[1];
	clear_crs = XCreateGC(display, xid, GCForeground | GCPlaneMask,
			      &gcvalues);
    }
    else {
	gcvalues.foreground =
	    pixel_table[cursor_color] ^ pixel_table[background_color];
	gcvalues.plane_mask = ~0;
	gcvalues.function = GXxor;
	draw_crs = XCreateGC(display, xid,
			     GCBackground | GCForeground | GCFunction |
			     GCLineStyle | GCPlaneMask, &gcvalues);
	clear_crs = draw_crs;
    }

    /* Annotations are printed using draw_ann, and erased using clear_ann. */
    annfontname = defaults_get_string("wave.signalwindow.font",
				      "Wave.SignalWindow.Font",
				      DEFANNFONT);
    if ((font = XLoadQueryFont(display, annfontname)) == NULL) {
	fprintf(stderr, "can't find font %s", annfontname);
	if ((font = XLoadQueryFont(display, DEFANNFONT)) == NULL) {
	    fprintf(stderr, " -- exiting\n");
	    return(1);
	}
	fprintf(stderr, " -- substituting font %s\n", DEFANNFONT);
    }
    gcvalues.font = font->fid;
    gcvalues.foreground = pixel_table[annotation_color];
    if (allowdottedlines)
	gcvalues.line_style = LineOnOffDash;
    else
	gcvalues.line_style = LineSolid;
    gcvalues.dashes = use_color ? 1 : 2;
    gcvalues.plane_mask = use_overlays ? gcvalues.foreground : ~0;
    draw_ann = XCreateGC(display, xid, GCBackground | GCFont | GCForeground |
			 GCLineStyle | GCDashList | GCPlaneMask, &gcvalues);
    if (use_overlays) {
	gcvalues.foreground = gcvalues.background;
	gcvalues.plane_mask = mask[2];
	clear_ann = XCreateGC(display, xid,
			 GCBackground | GCFont | GCForeground | GCPlaneMask,
			 &gcvalues);
    }
    else
	clear_ann = clear_all;

    /* Signals are drawn using draw_sig, and erased using clear_sig. */
    gcvalues.foreground = pixel_table[signal_color];
    gcvalues.plane_mask = use_overlays ? gcvalues.foreground : ~0;
    gcvalues.line_width = defaults_get_integer("wave.signalwindow.line_width",
					       "Wave.SignalWindow.Line_width",
					       1);
    draw_sig = XCreateGC(display, xid,
	      GCBackground | GCFont | GCForeground | GCPlaneMask | GCLineWidth,
			 &gcvalues);
    if (use_overlays) {
	gcvalues.foreground = gcvalues.background;
	gcvalues.plane_mask = mask[3];
	clear_sig = XCreateGC(display, xid, GCForeground | GCPlaneMask,
			      &gcvalues);
    }
    else
	clear_sig = clear_all;

    /* Signals are highlighted using highlight_sig, and highlighting is
       removed using unhighlight_sig.  The settings are identical to those
       used for cursor drawing if overlays are enabled;  otherwise, the
       settings are those used for annotation drawing. */
    if (use_overlays) {
	highlight_sig = draw_crs;
	unhighlight_sig = clear_crs;
    }
    else {
	highlight_sig = draw_ann;
	unhighlight_sig = clear_ann;
    }

    gcvalues.foreground = pixel_table[background_color];
    bg_fill = XCreateGC(display, xid, GCForeground, &gcvalues);
    if (!clear_all || !draw_grd || !draw_cgrd || !clear_grd || !draw_crs ||
	!clear_crs || !draw_ann || !clear_ann || !draw_sig || !clear_sig ||
	!bg_fill) {
	fprintf(stderr,  "Error allocating graphics context\n");
	return (1);
    }

    /* Create and install a cursor for the signal window.  (This is the
       crosshair cursor that tracks the mouse pointer, not to be confused
       with the editing cursor, which is a pair of bars extending the entire
       height of the window except for the annotation area, with an optional
       box around an annotation.) */
#ifndef USE_OLC_PLUS
    /* This code exists because OLC_PLUS is not a "standard" Open Look cursor,
       according to <xview/cursor.h>, so it might not exist (as in X11R4).
       OLC_PLUS seems to be usable under X11R5 and X11R6. */
    cursor_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
	XV_WIDTH, 16,
	XV_HEIGHT, 16,
	SERVER_IMAGE_X_BITS, cursor_bits,
	NULL);
#endif
    cursor = (Xv_Cursor)xv_create(frame, CURSOR,
#ifndef USE_OLC_PLUS
	CURSOR_IMAGE, cursor_image,
	CURSOR_XHOT, 7,
	CURSOR_YHOT, 7,
#else
	CURSOR_SRC_CHAR, OLC_PLUS,
#endif
	CURSOR_FOREGROUND_COLOR, &cursor_fg,
	CURSOR_BACKGROUND_COLOR, &cursor_bg,
	NULL);
    xv_set(window, WIN_CURSOR, cursor, NULL);

    /* Register the event handler for the signal window. */
    xv_set(window, WIN_EVENT_PROC, window_event_proc,
	   WIN_CONSUME_EVENTS, WIN_NO_EVENTS, LOC_WINENTER, ACTION_HELP,
	       WIN_ASCII_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, NULL,
	   NULL);
    xv_set(window, WIN_IGNORE_EVENTS, WIN_UP_ASCII_EVENTS, NULL, NULL);

    /* Get the actual dimensions of the created canvas.  Use the resize
       procedure to adjust the width if necessary and to set the scales. */
    canvas_height = (int)xv_get(canvas, CANVAS_HEIGHT);
    canvas_width = (int)xv_get(canvas, CANVAS_WIDTH);
    do_resize(canvas, canvas_width, canvas_height);
    window_fit(canvas);
    window_fit(frame);

    /* Save parameters for the scope window. */
    save_scope_params(use_overlays, use_color, grey);

    return (0);
}

/* Hide_grid() makes the grid invisible by modifying the color map so that the
   color used for the grid is identical to that used for the background. */
void hide_grid()
{
    if (use_overlays) {
	color[background_color].pixel = pixel_table[grid_color];
	XStoreColor(display, colormap, &color[background_color]);
	color[background_color].pixel = pixel_table[background_color];
    }
}

/* Unhide_grid() reverses the action of hide_grid to make the grid visible
   again. */
void unhide_grid()
{
    if (use_overlays)
	XStoreColor(display, colormap, &color[grid_color]);
}


void display_and_process_events()
{
    in_xv_main_loop = 1;

    xv_main_loop(frame);
}

/* Exit from the program unless the annotation list was edited and the
   changes can't be saved (in such cases, the user may be able to recover
   by changing permissions or freeing additional file space). */
void quit_proc()
{
    if (post_changes()) {
	finish_log();
	xv_destroy_safe(frame);
    }
}
