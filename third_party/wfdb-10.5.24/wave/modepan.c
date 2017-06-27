/* file: modepan.c	G. Moody        30 April 1990
			Last revised:	 12 May 2009
Mode panel functions for WAVE

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

#include "wave.h"
#include "xvwave.h"

Panel_item grid_item, opt_item, sig_item, ann_item, ov_item, tim_item, ts_item,
    vs_item, redraw_item;
Frame mode_frame;
Panel mode_panel;

/* Undo any mode changes. */
void mode_undo()
{
    xv_set(ts_item, PANEL_VALUE, tsa_index, NULL);
    xv_set(vs_item, PANEL_VALUE, vsa_index, NULL);
    xv_set(sig_item, PANEL_VALUE, sig_mode, NULL);
    xv_set(ann_item, PANEL_VALUE, ann_mode, NULL);
    xv_set(ov_item, PANEL_VALUE, overlap, NULL);
    xv_set(tim_item, PANEL_VALUE, time_mode, NULL);
    xv_set(grid_item, PANEL_VALUE, grid_mode, NULL);
    xv_set(opt_item, PANEL_VALUE,
	    (show_subtype & 1) |
	   ((show_chan    & 1) << 1) |
	   ((show_num     & 1) << 2) |
	   ((show_aux     & 1) << 3) |
	   ((show_marker  & 1) << 4) |
	   ((show_signame & 1) << 5) |
	   ((show_baseline& 1) << 6) |
	   ((show_level   & 1) << 7),
	   NULL);
}

/* Redraw and take down the window. */
static void redraw_proc(item, event)
Panel_item item;
Event *event;
{
    dismiss_mode();
    disp_proc(item, event);
}

int mode_popup_active = -1;
char *tchoice[] = {"0.25 mm/hour", "1 mm/hour", "5 mm/hour",
    "0.25 mm/min", "1 mm/min", "5 mm/min", "25 mm/min",
    "50 mm/min", "125 mm/min", "250 mm/min", "500 mm/min",
    "12.5 mm/sec", "25 mm/sec", "50 mm/sec", "125 mm/sec", "250 mm/sec",
    "500 mm/sec", "1000 mm/sec"};
char *vchoice[] = { "1 mm/mV", "2.5 mm/mV", "5 mm/mV", "10 mm/mV", "20 mm/mV",
   "40 mm/mV", "100 mm/mV" };

/* Set up popup window for adjusting display modes. */
void create_mode_popup()
{
    extern void save_defaults();	/* in xvwave.c */
    Icon icon;

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "View",
		     NULL);
    mode_frame = xv_create(frame, FRAME_CMD,
	XV_LABEL, "View",
	FRAME_ICON, icon, 0);
    mode_panel = xv_get(mode_frame, FRAME_CMD_PANEL, 0);

    opt_item = xv_create(mode_panel, PANEL_CHOICE,
        PANEL_CHOOSE_ONE, FALSE,
	XV_HELP_DATA, "wave:view.show",
	PANEL_LABEL_STRING, "Show: ",
	PANEL_CHOICE_STRINGS,
	      "subtype", "`chan' field", "`num' field", "`aux' field",
	      "markers", "signal names", "baselines", "level", NULL,
	PANEL_VALUE, 0,
        0);
    ts_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.time_scale",
	PANEL_LABEL_STRING, "Time scale: ",
	PANEL_CHOICE_STRINGS,
	      tchoice[0], tchoice[1], tchoice[2], tchoice[3], tchoice[4],
	      tchoice[5], tchoice[6], tchoice[7], tchoice[8], tchoice[9],
	      tchoice[10],tchoice[11],tchoice[12],tchoice[13],tchoice[14],
	      NULL,
	PANEL_VALUE, DEF_TSA_INDEX,
	PANEL_DEFAULT_VALUE, DEF_TSA_INDEX,
	0);
    vs_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.amplitude_scale",
	PANEL_LABEL_STRING, "Amplitude scale: ",
	PANEL_CHOICE_STRINGS,
	      vchoice[0], vchoice[1], vchoice[2], vchoice[3], vchoice[4],
	      vchoice[5], vchoice[6], NULL,
	PANEL_VALUE, DEF_VSA_INDEX,
	PANEL_DEFAULT_VALUE, DEF_VSA_INDEX,
	0);
    sig_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.draw",
	PANEL_LABEL_STRING, "Draw: ",
	PANEL_CHOICE_STRINGS,
	      "all signals", "listed signals only", "valid signals only", NULL,
	PANEL_VALUE, 0,
	PANEL_DEFAULT_VALUE, 0,
	0);
    grid_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.grid",
	PANEL_LABEL_STRING, "Grid: ",
	PANEL_CHOICE_STRINGS,"None", "0.2 s", "0.5 mV", "0.2 s x 0.5 mV",
			     "0.04 s x 0.1 mV", "1 m x 0.5 mV", "1 m x 0.1 mV",
			     NULL,
	PANEL_VALUE, 0,
	PANEL_DEFAULT_VALUE, 3,
	0);
    ann_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	PANEL_NEXT_ROW, -1,
	XV_HELP_DATA, "wave:view.show_annotations",
	PANEL_LABEL_STRING, "Show annotations: ",
	PANEL_CHOICE_STRINGS,
	      "centered", "attached to signals", "as a signal", NULL,
	PANEL_VALUE, 0,
	PANEL_DEFAULT_VALUE, 0,
	0);
    ov_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.overlap",
	PANEL_CHOICE_STRINGS,
	      "avoid overlap", "allow overlap", NULL,
        PANEL_VALUE, 0,
        PANEL_DEFAULT_VALUE, 0,
	0);
    tim_item = xv_create(mode_panel, PANEL_CHOICE_STACK,
	XV_HELP_DATA, "wave:view.time_display",
	PANEL_LABEL_STRING, "Time display: ",
	PANEL_CHOICE_STRINGS,
	      "elapsed", "absolute", "in sample intervals", NULL,
	PANEL_VALUE, 0,
	PANEL_DEFAULT_VALUE, 0,
	0);
    xv_create(mode_panel, PANEL_BUTTON,
	PANEL_NEXT_ROW, -1,
	XV_HELP_DATA, "wave:view.undo",
	PANEL_LABEL_STRING, "Undo changes",
	PANEL_NOTIFY_PROC, mode_undo,
	0);
    redraw_item = xv_create(mode_panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:view.redraw",
	PANEL_LABEL_STRING, "Redraw",
	PANEL_NOTIFY_PROC, redraw_proc,
	PANEL_CLIENT_DATA, (caddr_t) '.',
	0);
    xv_create(mode_panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:view.save_as_new_defaults",
	PANEL_LABEL_STRING, "Save as new defaults",
	PANEL_NOTIFY_PROC, save_defaults,
	0);

    xv_set(mode_panel, PANEL_DEFAULT_ITEM, redraw_item, NULL);

    window_fit(mode_panel);
    window_fit(mode_frame);
    mode_popup_active = 0;
}

/* Make the display mode popup window appear. */
void show_mode()
{
    if (mode_popup_active < 0) create_mode_popup();
    xv_set(mode_frame, WIN_SHOW, TRUE, 0);
    mode_popup_active = 1;
}

void set_modes()
{
    int i, otsai = tsa_index, ovsai = vsa_index;
    double osh = canvas_height_mv, osw = canvas_width_sec;
    static double vsa[] = { 1.0, 2.5, 5.0, 10.0, 20.0, 40.0, 100.0 };

    /* If the panel has never been instantiated, there is nothing to do. */
    if (mode_popup_active < 0) return;

    /* Read the current panel settings, beginning with the grid option. */
    switch (grid_mode = (int)xv_get(grid_item, PANEL_VALUE)) {
      case 0: ghflag = gvflag = visible = 0; break;
      case 1: ghflag = 0; gvflag = visible = 1; break;
      case 2: ghflag = visible = 1; gvflag = 0; break;
      case 3: ghflag = gvflag = visible = 1; break;
      case 4: ghflag = gvflag = visible = 2; break;
      case 5: ghflag = visible = 1; gvflag = 3; break;
      case 6: ghflag = visible = 2; gvflag = 3; break;
    }

    /* Next, check the annotation display options.  The bit mask assignments
       below are determined by the order of the PANEL_CHOICE_STRINGS for
       opt_item in create_mode_popup(), above. */
    i = (int)xv_get(opt_item, PANEL_VALUE);
    show_subtype =  i       & 1;
    show_chan    = (i >> 1) & 1;
    show_num     = (i >> 2) & 1;
    show_aux	 = (i >> 3) & 1;
    show_marker  = (i >> 4) & 1;
    show_signame = (i >> 5) & 1;
    show_baseline= (i >> 6) & 1;
    show_level	 = (i >> 7) & 1;

    /* Check the signal display options next. */
    i = sig_mode;
    sig_mode = (int)xv_get(sig_item, PANEL_VALUE);
    if (i != sig_mode || sig_mode == 2)
	set_baselines();

    /* Check the annotation display mode next. */
    ann_mode = (int)xv_get(ann_item, PANEL_VALUE);
    overlap = (int)xv_get(ov_item, PANEL_VALUE);

    /* Check the time display mode next. */
    i = time_mode;
    time_mode = (int)xv_get(tim_item, PANEL_VALUE);
    /* The `nsig > 0' test is a bit of a kludge -- we don't want to reset the
       time_mode before the record has been opened, because we don't know if
       absolute times are available until then. */
    if (nsig > 0 && time_mode == 1)
	(void)wtimstr(0L);	/* check if absolute times are available --
				   if not, time_mode is reset to 0 */
    if (i != time_mode) {
	set_start_time(wtimstr(display_start_time));
	set_end_time(wtimstr(display_start_time + nsamp));
	reset_start();
	reset_stop();
    }

    /* The purpose of the complex method of computing canvas_width_sec is to
       obtain a "rational" value for it even if the frame has been resized.
       First, we determine the number of 5 mm time-grid units in the window
       (note that the resize procedure in xvwave.c guarantees that this will
       be an integer;  the odd calculation is intended to take care of
       roundoff error in pixel-to-millimeter conversion).  For each scale,
       the multiplier of u is simply the number of seconds that would be
       represented by 5 mm.   Since u is usually a multiple of 5 (except on
       small displays, or if the frame has been resized to a small size),
       the calculated widths in seconds are usually integers, at worst
       expressible as an integral number of tenths of seconds. */
    if ((i = xv_get(ts_item, PANEL_VALUE)) >= 0 && i < 15) {
	int u = ((int)(canvas_width/dmmx(1) + 1)/5);	/* number of 5 mm
							   time-grid units */
	switch (tsa_index = i) {
	  case 0:	/* 0.25 mm/hour */
	    mmpersec = (0.25/3600.);
	    canvas_width_sec = 72000 * u; break;
	  case 1:	/* 1 mm/hour */
	    mmpersec = (1./3600.);
	    canvas_width_sec = 18000 * u; break;
	  case 2:	/* 5 mm/hour */
	    mmpersec = (5./3600.);
	    canvas_width_sec = 3600 * u; break;
	  case 3:	/* 0.25 mm/min */
	    mmpersec = (0.25/60.);
	    canvas_width_sec = 1200 * u; break;
	  case 4:	/* 1 mm/min */
	    mmpersec = (1./60.);
	    canvas_width_sec = 300 * u; break;
	  case 5:	/* 5 mm/min */
	    mmpersec = (5./60.);
	    canvas_width_sec = 60 * u; break;
	  case 6:	/* 25 mm/min */
	    mmpersec = (25./60.);
	    canvas_width_sec = 12 * u; break;
	  case 7:	/* 50 mm/min */
	    mmpersec = (50./60.);
	    canvas_width_sec = 6 * u; break;
	  case 8:	/* 125 mm/min */
	    mmpersec = (125./60.);
	    canvas_width_sec = (12 * u) / 5; break;
	  case 9:	/* 250 mm/min */
	    mmpersec = (250./60.);
	    canvas_width_sec = (6 * u) / 5; break;
	  case 10:	/* 500 mm/min */
	    mmpersec = (500./60.);
	    canvas_width_sec = (3 * u) / 5; break;
	  case 11:	/* 12.5 mm/sec */
	    mmpersec = 12.5;
	    canvas_width_sec = (2 * u) / 5; break;
	  case 12:	/* 25 mm/sec */
	    mmpersec = 25.;
	    canvas_width_sec = u / 5; break;
	  case 13:	/* 50 mm/sec */
	    mmpersec = 50.;
	    canvas_width_sec = u / 10; break;
	  case 14:	/* 125 mm/sec */
	    mmpersec = 125.;
	    canvas_width_sec = u / 25; break;
	  case 15:	/* 250 mm/sec */
	    mmpersec = 250.;
	    canvas_width_sec = u / 50.0; break;
	  case 16:	/* 500 mm/sec */
	    mmpersec = 500.;
	    canvas_width_sec = u / 100.0; break;
	  case 17:	/* 1000 mm/sec */
	    mmpersec = 1000.;
	    canvas_width_sec = u / 200.0; break;
	}
    }

    /* Computation of canvas_height_mv could be as complex as above, but
       it doesn't seem so important to obtain a "rational" value here. */
    if ((i = xv_get(vs_item, PANEL_VALUE)) >= 0 && i < 7) {
	mmpermv = vsa[i];
	canvas_height_mv = canvas_height/dmmy(vsa[vsa_index = i]);
    }
    if (osh != canvas_height_mv || osw != canvas_width_sec ||
	otsai != tsa_index || ovsai != vsa_index) {
	vscale[0] = 0.0;
	calibrate();
    }
}

/* Effect any mode changes that were selected and make the popup disappear. */
void dismiss_mode()
{
    /* If the panel is currently visible, make it go away. */
    if (mode_popup_active > 0 &&
	(int)xv_get(mode_frame, FRAME_CMD_PUSHPIN_IN) == FALSE) {
	xv_set(mode_frame, WIN_SHOW, FALSE, 0);
	mode_popup_active = 0;
    }
    set_modes();
}

/* Time-to-string conversion functions.  These functions use those in the
   WFDB library, but ensure that (1) elapsed times are displayed if time_mode
   is 0, and (2) absolute times (if available) are displayed without brackets
   if time_mode is non-zero. */

long wstrtim(s)
char *s;
{
    long t;

    while (*s == ' ' || *s == '\t') s++;
    if (time_mode == 1 && *s != '[' && *s != 's' && *s != 'c' && *s != 'e') {
	char buf[80];

	sprintf(buf, "[%s]", s);
	s = buf;
    }
    t = strtim(s);
    if (*s == '[') {	/* absolute time specified - strtim returns a negated
			   sample number if s is valid */
	if (t > 0L) t = 0L;	/* invalid s (before sample 0) -- reset t */
	else t = -t;	/* valid s -- convert t to a positive sample number */
    }
    return (t);
}

char *wtimstr(t)
long t;
{
    switch (time_mode) {
      case 0:
      default:
	if (t == 0L) return ("0:00");
	else if (t < 0L) t = -t;
	return (timstr(t));
      case 1:
	{
	    char *p, *q;

	    if (t > 0L) t = -t;
	    p = timstr(t);
	    if (*p == '[') {
		p++;
		q = p + strlen(p) - 1;
		if (*q == ']') *q = '\0';
	    }
	    else {
		time_mode = 0;
		if (tim_item) xv_set(tim_item, PANEL_VALUE, time_mode, NULL);
	    }
	    return (p);
        }
      case 2:
	{
	    static char buf[12];

	    if (t < 0L) t = -t;
	    sprintf(buf, "s%ld", t);
	    return (buf);
	}
    }
}

char *wmstimstr(t)
long t;
{
    switch (time_mode) {
      case 0:
      default:
	if (t == 0L) return ("0:00");
	else if (t < 0L) t = -t;
	return (mstimstr(t));
      case 1:
	{
	    char *p, *q;

	    if (t > 0L) t = -t;
	    p = mstimstr(t);
	    if (*p == '[') {
		p++;
		q = p + strlen(p) - 1;
		if (*q == ']') *q = '\0';
	    }
	    else {
		time_mode = 0;
		if (tim_item) xv_set(tim_item, PANEL_VALUE, time_mode, NULL);
	    }
	    return (p);
	}
      case 2:
	{
	    static char buf[12];

	    if (t < 0L) t = -t;
	    sprintf(buf, "s%ld", t);
	    return (buf);
	}
    }
}
