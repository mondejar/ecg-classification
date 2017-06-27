/* file: scope.c	G. Moody	31 July 1991
			Last revised:	10 June 2005
Scope window functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1991-2005 George B. Moody

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
#include <wfdb/ecgmap.h>
#include <X11/Xos.h>
#include <xview/canvas.h>
#include <xview/cms.h>
#include <xview/cursor.h>
#include <xview/defaults.h>
#include <xview/notice.h>
#include <xview/notify.h>

#define SQRTMAXSPEED	(30)
#define MAXSPEED  (SQRTMAXSPEED*SQRTMAXSPEED)

static int scope_use_overlays, use_color, grey;
static void scan(), create_scope_panel(), set_dt();
static void scope_proc(Panel_item item, Event *event);

void save_scope_params(a, b, c)
int a, b, c;
{
    scope_use_overlays = a;
    use_color = b;
    grey = c;
}

static void scope_repaint(canvas, paint_window, repaint_area)
Canvas canvas;
Xv_Window paint_window;
Rectlist *repaint_area;
{
    ;
}

static unsigned int scope_height, scope_width;
static XPoint *sbuf;
static int v0n, v0f, v0v, xt, yt;
static long scope_dt;
static Display *scope_display;
static XID scope_xid;
static GC clear_plane[4], plot_sig;

static void scope_resize(canvas, w, h)
Canvas canvas;
int w, h;
{
    int i;

    xt = mmx(2);
    if (w > scope_width) {
	XPoint *sbt;
	
	if (sbuf == NULL &&
	    (sbt = (XPoint *)malloc(w * sizeof(XPoint))) == NULL) {
#ifdef NOTICE
	    Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
				NOTICE_MESSAGE_STRINGS,
				"Error in allocating memory for scope\n", 0,
				NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	    xv_destroy_safe(notice);
#endif
	    return;
	}
	else if (sbuf != NULL &&
		 (sbt = (XPoint *)realloc(sbuf, w * sizeof(XPoint))) == NULL) {
#ifdef NOTICE
	    Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			    NOTICE_MESSAGE_STRINGS,
			    "Error in allocating memory for scope\n", 0,
			    NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	    xv_destroy_safe(notice);
#endif
	    return;
	}
	sbuf = sbt;
    }
    if (w != scope_width-1) {
	if (scope_width == 0)
	    scope_dt = strtim("0.5");
	scope_width = w-1;
	if (tscale <= 1.0)	/* resolution limited by display */
	    for (i = 0; i <= scope_width; i++)
		sbuf[i].x = i;
	else {			/* resolution limited by input data */
	    for (i = 0; i <= scope_width; i++)
		sbuf[i].x = i*tscale;
	}
    }
    if (h != scope_height) {
	scope_height = h;
	yt = scope_height - mmy(2);
	v0n = scope_height/3;
	v0f = scope_height/2;
	v0v = 2*scope_height/3;
    }

    if (scope_xid) {
      if (scope_use_overlays) {
        static XPoint tbuf[2];

	tbuf[0].y = 0; tbuf[1].y = scope_height-1;
	for (i = 0; i <scope_width; i++) {
	  tbuf[0].x = tbuf[1].x = i;
	  XDrawLines(scope_display, scope_xid, plot_sig, tbuf, 2,
		     CoordModeOrigin);
	}
      }
      for (i = 0; i < 4; i++)
	XFillRectangle(scope_display, scope_xid, clear_plane[i],
		       0, 0, scope_width, scope_height);
    }
}

/* Handle events in the scope window. */
void scope_window_event_proc(window, event, arg)
Xv_Window window;
Event *event;
Notify_arg arg;
{
    int e, x, y;
    static int handling_event;

    e = (int)event_id(event);
    x = (int)event_x(event);
    y = (int)event_y(event);

    if (handling_event) return;
    handling_event = 1;
    if (event_action(event) == ACTION_HELP)
	xv_help_show(window, "wave:scope_canvas", event);
    else switch (e) {
        case KEY_RIGHT(10):	/* left-arrow: simulate left mouse button */
	case MS_LEFT:
	  if (event_is_down(event)) {
	      if (event_ctrl_is_down(event))
		  scope_proc(XV_NULL, (Event *) '[');	/* scan backward */
	      else
		  scope_proc(XV_NULL, (Event *) '<');	/* single step back */
	  }
          break;
	case KEY_RIGHT(8):	/* up-arrow: simulate middle mouse button */
	case KEY_RIGHT(11):	/* <5> on numeric keypad: simulate middle
				   mouse button */
	case MS_MIDDLE:
	  if (event_is_down(event)) {
	      if (event_ctrl_is_down(event))
		  set_dt(x);
	      else
		  scope_proc(XV_NULL, (Event *) '*');	/* interrupt scan */
	  }
	  break;
	case KEY_RIGHT(12):	/* right-arrow: simulate right mouse button */
	case MS_RIGHT:
	  if (event_is_down(event)) {
	      if (event_ctrl_is_down(event))
		  scope_proc(XV_NULL, (Event *) ']');	/* scan forward */
	      else
		  scope_proc(XV_NULL, (Event *) '>');	/* single step */
	  }
          break;
      }
    handling_event = 0;
}

static Colormap colormap;
static XColor color[16];
static unsigned long pixel_table[16];
static Canvas canvas;
static Frame scope_frame;
static Panel scope_panel;
#ifndef USE_OLC_PLUS
extern Server_image cursor_image;
#endif

void create_scope_popup(overlay_flag, use_color, grey)
int overlay_flag, use_color, grey;
{
    char *bgcname, *fgcname;
    int i, ncolors;
    static unsigned long mask[4];
    static XGCValues gcvalues;
    Icon icon;
    Xv_Cursor scope_c;
    Xv_singlecolor scope_c_bg, scope_c_fg;
    Xv_Window window;

    icon = xv_create(frame, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Scope",
		     NULL);
    scope_frame = xv_create(frame, FRAME,
			    XV_LABEL, "Scope",
			    XV_WIDTH, mmx(25)+4,
			    XV_HEIGHT, mmy(150),
			    FRAME_ICON, icon,
			    NULL);
    create_scope_panel();
    window_fit_height(scope_panel);
    canvas = xv_create(scope_frame, CANVAS,
			     CANVAS_REPAINT_PROC, scope_repaint,
			     CANVAS_RESIZE_PROC, scope_resize,
			     CANVAS_WIDTH, mmx(25),
			     CANVAS_HEIGHT, mmy(100),
			     CANVAS_MIN_PAINT_WIDTH, mmx(25),
			     CANVAS_MIN_PAINT_HEIGHT, mmy(20),
			     CANVAS_AUTO_CLEAR, TRUE,
			     WIN_X, 0,
			     WIN_BELOW, scope_panel,
			     WIN_DYNAMIC_VISUAL, overlay_flag,
			     0);

    scope_resize(canvas, (int)xv_get(canvas, CANVAS_WIDTH),
		         (int)xv_get(canvas, CANVAS_HEIGHT));

    window_fit(canvas);
    window_fit(scope_frame);

    scope_display = (Display *)xv_get(canvas, XV_DISPLAY);
    window = (Xv_Window)xv_get(canvas, CANVAS_NTH_PAINT_WINDOW, 0);
    scope_xid = (Window)xv_get(window, XV_XID);

    mask[0] = mask[1] = mask[2] = mask[3] = ~0;

    /* Get the appropriate foreground and background colors. */
    if (use_color && !grey) {
	bgcname = defaults_get_string("wave.scope.color.background",
				      "Wave.Scope.Color.background",
				      "white");
	fgcname = defaults_get_string("wave.scope.color.foreground",
				      "Wave.Scope.Color.Foreground",
				      "blue");
    }
    else if (grey) {
	bgcname = defaults_get_string("wave.scope.grey.background",
				      "Wave.Scope.Grey.background",
				      "white");
	fgcname = defaults_get_string("wave.scope.grey.foreground",
				      "Wave.Scope.Grey.Foreground",
				      "black");
    }
    else {
	bgcname = defaults_get_string("wave.scope.mono.background",
				      "Wave.Scope.Mono.background",
				      "white");
	if (strcmp(bgcname, "black") && strcmp(bgcname, "Black"))
	    fgcname = "black";
	else
	    fgcname = "white";
    }

    /* Allocate a colormap for the scope display area. */
    colormap = DefaultColormap(scope_display, DefaultScreen(scope_display));

    /* Determine how to "fade" the display (i.e., how to make the oldest data
       disappear into the background).  There are two techniques:

       A. If 16 read/write color cells in 4 plane groups are available, we
          number them 0 to 15.  Color 0 is pure background, and color 15 is
	  pure foreground.  The remainder are blends of foreground and
	  background, and the amount of foreground in the mix is determined
	  by the number of non-zero bits in the color number.  Before writing
	  new data, one of the four planes is cleared (the choice of which
	  plane is made by circulating through the 4 in a fixed order).  New
	  data are always written in color 15.  Thus new data are 100%
	  foreground, data one cycle old are 75%, data two cycles old are 50%,
	  data three cycles old are 25%, and all older data are erased (i.e.,
	  set to the background color).

       B. Otherwise, before writing new data, we set one of every four pixels
          in the window to the background color using one of four bitmasks
	  (each of which includes one-fourth of the pixels, with no overlap
	  between the bitmasks).  Thus 100% of all the pixels in new data are
	  in the foreground color, data one cycle old are 75% visible (on
	  average), data two cycles old are 50% visible, data three cycles old
	  are 25% visible, and all older data are erased.

       It might be even better to combine these techniques if both can be
       used on a given display, to give 16 levels of fading;  this would also
       require 16 GCs, however, which might pose a problem for the server.
    */

    if (overlay_flag) {
	/* Try to get read/write color cells if possible. */
	if (!XAllocColorCells(scope_display, colormap, 0, mask, 4,
			     pixel_table, 1))
	    overlay_flag = 0;	/* impossible -- use plan B */

	else {			/* execute plan A */
	    /* Color 0: background */
	    XParseColor(scope_display, colormap, bgcname, &color[0]);

	    /* Color 15: foreground */
	    XParseColor(scope_display, colormap, fgcname, &color[15]);

	    /* Color numbers with one non-zero bit: 1, 2, 4, 8 */
	    color[1].red   = color[2].red   = color[4].red   = color[8].red   =
		(color[15].red   + 3*color[0].red  ) / 4;
	    color[1].green = color[2].green = color[4].green = color[8].green =
		(color[15].green + 3*color[0].green) / 4;
	    color[1].blue  = color[2].blue  = color[4].blue  = color[8].blue  =
		(color[15].blue  + 3*color[0].blue ) / 4;

	    /* Color numbers with two non-zero bits: 3, 5, 6, 9, 10, 12 */
	    color[3].red   = color[5].red   = color[6].red   = color[9].red   =
		color[10].red   = color[12].red   =
		    (color[15].red   + color[0].red  ) / 2;
	    color[3].green = color[5].green = color[6].green = color[9].green =
		color[10].green = color[12].green =
		    (color[15].green + color[0].green) / 2;
	    color[3].blue  = color[5].blue  = color[6].blue  = color[9].blue  =
		color[10].blue  = color[12].blue  =
		    (color[15].blue  + color[0].blue ) / 2;

	    /* Color numbers with three non-zero bits: 7, 11, 13, 14 */
	    color[7].red   = color[11].red   = color[13].red   =
		color[14].red   = (3*color[15].red   + color[0].red  )/4;
	    color[7].green = color[11].green = color[13].green =
		color[14].green = (3*color[15].green + color[0].green)/4;
	    color[7].blue  = color[11].blue  = color[13].blue  =
		color[14].blue  = (3*color[15].blue  + color[0].blue )/4;

	    for (i = 0; i < 16; i++) {
		color[i].pixel = pixel_table[0];
		if (i & 1) color[i].pixel |= mask[0];
		if (i & 2) color[i].pixel |= mask[1];
		if (i & 4) color[i].pixel |= mask[2];
		if (i & 8) color[i].pixel |= mask[3];
		color[i].flags = DoRed | DoGreen | DoBlue;
		pixel_table[i] = color[i].pixel;
	    }
	    ncolors = 16;
	    XStoreColors(scope_display, colormap, color, ncolors);

	    /* Create plan A graphics contexts. */
	    gcvalues.foreground = pixel_table[15];
	    gcvalues.background = pixel_table[0];
	    plot_sig = XCreateGC(scope_display, scope_xid,
				 GCForeground | GCBackground,
				 &gcvalues);
	    gcvalues.foreground = gcvalues.background;
	    for (i = 0; i < 4; i++) {
		gcvalues.plane_mask = mask[i];
		clear_plane[i] = XCreateGC(scope_display, scope_xid,
					   GCBackground | GCForeground |
					   GCPlaneMask,
					   &gcvalues);
	    }
	}
    }

    if (!overlay_flag) {	/* execute plan B */
	int j;
	unsigned int stipple_height, stipple_width;
	GC set_stipple, clear_stipple;
	Pixmap stipple;

	XParseColor(scope_display, colormap, bgcname, &color[0]);
	XParseColor(scope_display, colormap, fgcname, &color[1]);
	(void)XAllocColor(scope_display, colormap, &color[0]);
	(void)XAllocColor(scope_display, colormap, &color[1]);
	pixel_table[0] = color[0].pixel;
	pixel_table[1] = color[1].pixel;
	ncolors = 2;

	/* Create plan B graphics contexts. */
	XQueryBestStipple(scope_display, scope_xid,
			  scope_width, scope_height,
			  &stipple_width, &stipple_height);
	stipple = XCreatePixmap(scope_display, scope_xid,
				stipple_width, stipple_height, 1);
	gcvalues.foreground = gcvalues.background =
		    BlackPixel(scope_display, DefaultScreen(scope_display));
	clear_stipple = XCreateGC(scope_display, stipple,
				  GCForeground, &gcvalues);
	gcvalues.foreground =
		    WhitePixel(scope_display, DefaultScreen(scope_display));
	set_stipple = XCreateGC(scope_display, stipple,
			        GCForeground | GCBackground, &gcvalues);
	for (j = 0; j < stipple_height-1; j += 2)
	  for (i = 0; i < stipple_width-1; i += 2) {
		XDrawPoint(scope_display, stipple, set_stipple, i, j);
		XDrawPoint(scope_display, stipple, clear_stipple, i+1, j);
		XDrawPoint(scope_display, stipple, clear_stipple, i+1, j+1);
		XDrawPoint(scope_display, stipple, clear_stipple, i, j+1);
	  }
	gcvalues.stipple = stipple;
	gcvalues.fill_style = FillStippled;
	for (i = 0; i < 4; i++) {
	    gcvalues.ts_x_origin = (i & 1);
	    gcvalues.ts_y_origin = (i == 1 || i == 2);
	    clear_plane[i] = XCreateGC(scope_display, scope_xid,
				       GCForeground | GCStipple | GCFillStyle |
				       GCTileStipXOrigin | GCTileStipYOrigin,  
				       &gcvalues);
	}
       	gcvalues.foreground = pixel_table[1];
	gcvalues.background = pixel_table[0];
	plot_sig = XCreateGC(scope_display, scope_xid,
			     GCForeground | GCBackground,
			     &gcvalues);
	gcvalues.foreground = gcvalues.background;
    }

    /* Create and install a cursor for the scope window. Record the cursor
       colors in terms of the XView color model (8 bits for each of red, green,
       blue, rather than 16 as in the X model). */
    scope_c_fg.red = color[ncolors - 1].red >> 8;
    scope_c_fg.green = color[ncolors - 1].green >> 8;
    scope_c_fg.blue = color[ncolors - 1].blue >> 8;
    scope_c_bg.red = color[0].red >> 8;
    scope_c_bg.green = color[0].green >> 8;
    scope_c_bg.blue = color[0].blue >> 8;
    scope_c = (Xv_Cursor)xv_create(scope_frame, CURSOR,
#ifndef USE_OLC_PLUS
    /* See xvwave.c for details on the alternative cursors. */
				   CURSOR_IMAGE, cursor_image,
				   CURSOR_XHOT, 7,
				   CURSOR_YHOT, 7,
#else
				   CURSOR_SRC_CHAR, OLC_PLUS,
#endif
				   CURSOR_FOREGROUND_COLOR, &scope_c_fg,
				   CURSOR_BACKGROUND_COLOR, &scope_c_bg,
				   NULL);
    xv_set(window, WIN_CURSOR, scope_c, NULL);

    /* Register the event handler for the scope window. */
    xv_set(window, WIN_EVENT_PROC, scope_window_event_proc,
	   WIN_CONSUME_EVENTS, WIN_NO_EVENTS, LOC_WINENTER, ACTION_HELP,
	   WIN_ASCII_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, NULL,
	   NULL);
    xv_set(window, WIN_IGNORE_EVENTS, WIN_UP_ASCII_EVENTS, NULL, NULL);

    for (i = 0; i < 4; i++)
      XFillRectangle(scope_display, scope_xid, clear_plane[i],
		     0, 0, scope_width, scope_height);

    scope_resize(canvas, (int)xv_get(canvas, CANVAS_WIDTH),
		         (int)xv_get(canvas, CANVAS_HEIGHT));
}

static int show_this_frame()
{
    static char plane = 3, first_frame = 1;
    int i, i0, tt, tt0 = 0, v0;
    long t;

    if (first_frame && scope_use_overlays) {
      first_frame = 0;
      scope_resize(canvas, (int)xv_get(canvas, CANVAS_WIDTH),
		   (int)xv_get(canvas, CANVAS_HEIGHT));
    }

    if (scope_annp == NULL) return(0);
    if ((t = scope_annp->this.time - scope_dt) < 0L) {
	tt0 = (int)(-t);
	t = 0L;
    }
    if (isigsettime(t) < 0 || getvec(scope_v) < 0) return (0);
    switch (map2(scope_annp->this.anntyp)) {
      case NORMAL:
      case LEARN:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0n;
	break;
      case FUSION:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0f;
	break;
      case PVC:
	v0 = scope_v[signal_choice] * vscale[signal_choice] - v0v;
	break;
    }
    if (tscale >= 1.0) {		/* resolution limited by input data */
	for (i = i0 = tt0; i < scope_width; i++) {
	    if (getvec(scope_v) <= 0) break;
	    sbuf[i].y = scope_v[signal_choice] * vscale[signal_choice] - v0;
	}
	i--;
    }
    else {			/* resolution limited by display */
	int vmax, vmin, vv, x;

	(void)getvec(scope_v);
	vmax = vmin = scope_v[signal_choice];
	i = i0 = tt0*tscale;
	if (i < scope_width)
	    sbuf[i].y = scope_v[signal_choice] * vscale[signal_choice] - v0;
	for (tt = tt0 + 1; i < scope_width && getvec(scope_v) > 0; tt++) {
	    if (scope_v[signal_choice] > vmax) vmax = scope_v[signal_choice];
	    else if (scope_v[signal_choice]<vmin) vmin=scope_v[signal_choice];
	    if ((x = tt*tscale) > i) {
		i = x;
		if (vmax - vv > vv - vmin)
		    vv = vmin = vmax;
		else
		    vv = vmax = vmin;
		sbuf[i].y = vv * vscale[signal_choice] - v0;
	    }
	}
    }
    XFillRectangle(scope_display, scope_xid, clear_plane[plane],
		   0, 0, scope_width, scope_height);
    if (++plane > 3) {
	char *tp;

	plane = 0;
	tp = wtimstr(t);
	XDrawString(scope_display,scope_xid,plot_sig,xt,yt,tp,strlen(tp));
    }
    if (i > i0)
	XDrawLines(scope_display, scope_xid, plot_sig,
		   sbuf + i0, i - i0, CoordModeOrigin);
    return (1);
}

static void refresh_time()
{
    char *tp;
    int i, ytt;
    long t;

    ytt = scope_height - mmy(5);
    for (i = 0; i < 4; i++)
	XFillRectangle(scope_display, scope_xid, clear_plane[i],
		   0, ytt, scope_width, scope_height);
    if (scope_annp) {
	t = scope_annp->this.time - scope_width/2;
	tp = wtimstr(t);
	XDrawString(scope_display,scope_xid,plot_sig,xt,yt,tp,strlen(tp));
    }
}

static int show_next_frame()
{
    if (scope_annp == NULL) {
	scan(0);
	return (0);
    }
    while (scope_annp->this.time < begin_analysis_time) {
	if (scope_annp->next == NULL)
	    break;
	scope_annp = scope_annp->next;
    }	
    do {
	if (((scope_annp = scope_annp->next) == NULL) ||
	    (scope_annp->this.anntyp == INDEX_MARK) ||
	    (end_analysis_time > 0L &&
	     scope_annp->this.time > end_analysis_time)) {
	    if (scope_annp == NULL) scope_annp = ap_end;
	    else if (scope_annp->this.anntyp == INDEX_MARK &&
		     scope_annp->next != NULL)
		scope_annp = scope_annp->next;
	    else scope_annp = scope_annp->previous;
	    scan(0);
	    return (0);
	}
    } while (!isqrs(scope_annp->this.anntyp) ||
	    (ann_mode == 1 && scope_annp->this.chan != signal_choice));
    return (show_this_frame());
}

static int show_prev_frame()
{
    if (scope_annp == NULL) {
	scan(0);
	return (0);
    }
    while (end_analysis_time > 0L &&
	   scope_annp->this.time > end_analysis_time) {
	if (scope_annp->previous == NULL)
	    break;
	scope_annp = scope_annp->previous;
    }	
    do {
	if (((scope_annp = scope_annp->previous) == NULL) ||
	    (scope_annp->this.anntyp == INDEX_MARK) ||
	    (scope_annp->this.time < begin_analysis_time)) {
	    if (scope_annp == NULL) scope_annp = ap_start;
	    else if (scope_annp->this.anntyp == INDEX_MARK &&
		     scope_annp->previous != NULL)
		scope_annp = scope_annp->previous;
	    else scope_annp = scope_annp->next;
	    scan(0);
	    return (0);
	}
    } while (!isqrs(scope_annp->this.anntyp) ||
	    (ann_mode == 1 && scope_annp->this.chan != signal_choice));
    return (show_this_frame());
}

static int speed = MAXSPEED;

static Notify_value show_next_n_frames()
{
    int i;

    for (i = speed/10 + 1; i > 0 ; i--)
	if (show_next_frame() == 0) break;
    refresh_time();
    return (NOTIFY_DONE);
}

static Notify_value show_prev_n_frames()
{
    int i;

    for (i = speed/10 + 1; i > 0 ; i--)
	if (show_prev_frame() == 0) break;
    refresh_time();
    return (NOTIFY_DONE);
}

static struct itimerval sc_timer;

static void scan(speed)
int speed;
{
    if (speed > 0) {
	if (speed > MAXSPEED) speed = MAXSPEED;
	sc_timer.it_value.tv_usec = sc_timer.it_interval.tv_usec =
	    1000000L/speed;
	notify_set_itimer_func(scope_frame, show_next_n_frames, ITIMER_REAL,
			       &sc_timer, NULL);
	scan_active = 1;
    }
    else if (speed == 0) {
	notify_set_itimer_func(scope_frame, NOTIFY_FUNC_NULL, ITIMER_REAL,
			       NULL, NULL);
	scan_active = 0;
    }
    else {
	if (speed < -MAXSPEED) speed = -MAXSPEED;
	sc_timer.it_value.tv_usec = sc_timer.it_interval.tv_usec =
	    -1000000L/speed;
	notify_set_itimer_func(scope_frame, show_prev_n_frames, ITIMER_REAL,
			       &sc_timer, NULL);
	scan_active = -1;
    }
}

static void scope_proc(Panel_item item, Event *event)
{
    int client_data;
    long t0;

    if (item) client_data = (int)xv_get(item, PANEL_CLIENT_DATA);
    else client_data = (int)event;

    if (ap_start == NULL) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			    NOTICE_MESSAGE_STRINGS,
			    "Scope functions cannot be used while the",
			    "annotation list is empty.", 0,
			    NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	return;
    }
    if (attached &&
	(begin_analysis_time <= attached->this.time &&
	 (attached->this.time <= end_analysis_time || end_analysis_time<0L))) {
	scope_annp = attached;
	attached = NULL;
    }
    else if (scope_annp == NULL) {
	(void)locate_annotation(display_start_time, -128);
	scope_annp = annp;
    }

    switch (client_data) {
      case '[':		/* scan backwards */
	box(0, 0, 0);
	scan(-speed);
	break;
      case '<':		/* single-step backwards */
	show_prev_frame();
	refresh_time();
	if (display_start_time < scope_annp->this.time &&
	    scope_annp->this.time < display_start_time + nsamp)
	    box((int)((scope_annp->this.time - display_start_time)*tscale),
		(ann_mode==1 && (unsigned)scope_annp->this.chan < nsig) ?
		(int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
		1);
	else
	    box(0, 0, 0);
	break;
      case '*':		/* interrupt scan */
	scan(0);
	refresh_time();
	if ((t0 = scope_annp->this.time - nsamp/2) < 0L) t0 = 0L;
	find_display_list(t0);
	set_start_time(wtimstr(t0));
	set_end_time(wtimstr(t0 + nsamp));
	if (item)
	    disp_proc(item, (Event *)NULL);
	else
	    disp_proc(XV_NULL, event);
	box((int)((scope_annp->this.time - display_start_time)*tscale),
	    (ann_mode==1 && (unsigned)scope_annp->this.chan < nsig) ?
	    (int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
	    1);
	break;
      case '>':		/* single-step forwards */
	show_next_frame();
	refresh_time();
	if (display_start_time < scope_annp->this.time &&
	    scope_annp->this.time < display_start_time + nsamp)
	    box((int)((scope_annp->this.time - display_start_time)*tscale),
		(ann_mode==1 && (unsigned)scope_annp->this.chan < nsig) ?
		(int)(base[(unsigned)scope_annp->this.chan] + mmy(2)) : abase,
		1);
	else
	    box(0, 0, 0);
	break;
      case ']':		/* scan forwards */
      default:
	box(0, 0, 0);
	scan(speed);
	break;
    }
}

static void adjust_speed(item, value)
Panel_item item;
int value;
{
    speed = value*value;
    if (scan_active) scan(scan_active*speed);
}

static char *lmstimstr(t)
long t;
{
    char *p, *p0;

    if (t == 0L)
	return ("0");
    else if (t > 0L)
	p = mstimstr(t);
    else
	p = p0 = mstimstr(-t);
    while (*p == ' ' || *p == '0' || *p == ':')
	p++;
    if (*p == '.') p--;
    if (t < 0L && p > p0)
	*(--p) = '-';
    return (p);
}

Panel_item dt_item;

static void adjust_dt(item, value)
Panel_item item;
int value;
{
    char *dt_string = (char *) xv_get(item, PANEL_VALUE);

    while (*dt_string == ' ' || *dt_string == '\t')
	dt_string++;
    if (*dt_string != '-')
	scope_dt = strtim(dt_string);
    else
	scope_dt = -strtim(dt_string+1);
    xv_set(item, PANEL_VALUE, lmstimstr(scope_dt));
}

static void set_dt(x)
int x;
{
    scope_dt = x;
    scope_dt /= tscale;
    xv_set(dt_item, PANEL_VALUE, lmstimstr(scope_dt));
}

static void create_scope_panel()
{
    scope_panel = xv_create(scope_frame, PANEL,
			    XV_X, 0,
			    XV_HELP_DATA, "wave:scope_panel",
			    0);
    xv_create(scope_panel, PANEL_SLIDER,
	      XV_HELP_DATA, "wave:scope_panel.speed",
	      PANEL_LABEL_STRING, "Speed",
	      PANEL_DIRECTION, PANEL_VERTICAL,
	      PANEL_VALUE, SQRTMAXSPEED,
	      PANEL_MAX_VALUE, SQRTMAXSPEED,
	      PANEL_SHOW_RANGE, FALSE,
	      PANEL_SHOW_VALUE, FALSE,
	      PANEL_NOTIFY_PROC, adjust_speed,
	      NULL);
    dt_item = xv_create(scope_panel, PANEL_TEXT,
	      XV_HELP_DATA, "wave:scope_panel.dt",
	      PANEL_LABEL_STRING, "dt: ",
	      PANEL_VALUE_DISPLAY_LENGTH, 6,
	      PANEL_VALUE, "0.500",
	      PANEL_NOTIFY_PROC, adjust_dt,
	      NULL);
    xv_create(scope_panel, PANEL_BUTTON,
	      XV_HELP_DATA, "wave:scope_panel.<<",
	      PANEL_LABEL_STRING, "<<",
	      PANEL_NOTIFY_PROC, scope_proc,
	      PANEL_CLIENT_DATA, (caddr_t) '[',
	      NULL);
    xv_create(scope_panel, PANEL_BUTTON,
	      XV_HELP_DATA, "wave:scope_panel.<",
	      PANEL_LABEL_STRING, "<",
	      PANEL_NOTIFY_PROC, scope_proc,
	      PANEL_CLIENT_DATA, (caddr_t) '<',
	      NULL);
    xv_create(scope_panel, PANEL_BUTTON,
	      XV_HELP_DATA, "wave:scope_panel.pause",
	      PANEL_LABEL_STRING, "  Pause  ",
	      PANEL_NOTIFY_PROC, scope_proc,
	      PANEL_CLIENT_DATA, (caddr_t) '*',
	      NULL);
    xv_create(scope_panel, PANEL_BUTTON,
	      XV_HELP_DATA, "wave:scope_panel.>",
	      PANEL_LABEL_STRING, ">",
	      PANEL_NOTIFY_PROC, scope_proc,
	      PANEL_CLIENT_DATA, (caddr_t) '>',
	      NULL);
    xv_create(scope_panel, PANEL_BUTTON,
	      XV_HELP_DATA, "wave:scope_panel.>>",
	      PANEL_LABEL_STRING, ">>",
	      PANEL_NOTIFY_PROC, scope_proc,
	      PANEL_CLIENT_DATA, (caddr_t) ']',
	      NULL);
}

static int scope_popup_active = -1;

void show_scope_window()
{
    if (scope_popup_active < 0)
	create_scope_popup(scope_use_overlays, use_color, grey);
    wmgr_top(scope_frame);
    xv_set(scope_frame, WIN_MAP, TRUE, 0);
    scope_popup_active = 1;
}

