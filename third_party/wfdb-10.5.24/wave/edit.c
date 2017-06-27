/* file: edit.c		G. Moody	 1 May 1990
			Last revised:	13 July 2010
Annotation-editing functions for WAVE

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

#include "wave.h"
#include "xvwave.h"
#include <unistd.h>	/* getcwd */
#include <wfdb/ecgmap.h>
#include <xview/notice.h>
#include <xview/win_input.h>

WFDB_Time level_time;
Frame level_frame;
Panel level_panel;
Panel_item level_mode_item, level_time_item;
static char level_time_string[36];
int bar_on, bar_x, bar_y;
int level_mode, level_popup_active = -1;
int selected = -1;

void reset_ref()
{
    (void)isigsettime(ref_mark_time);
    (void)getvec(vref);
}

void recreate_level_popup()
{
    int stat = level_popup_active;
    void show_level_popup();

    if (stat >= 0 && xv_destroy_safe(level_frame) == XV_OK) {
	level_popup_active = -1;
	show_level_popup(stat);
    }
}

void set_level_mode(item, event)
Panel_item item;
Event *event;
{
    void show_level_popup();

    level_mode = (int)xv_get(level_mode_item, PANEL_VALUE);
    show_level_popup(TRUE);
}

void show_level_popup(stat)
int stat;
{
    int i, invalid_data;
    void create_level_popup();

    switch (level_mode) {
      case 0:
	  sprintf(level_time_string, "Time: %s", mstimstr(-level_time));
	  break;
      case 1:
	  if (level_time >= ref_mark_time)
	      sprintf(level_time_string, "Interval: %s",
		      mstimstr(level_time - ref_mark_time));
	  else
	      sprintf(level_time_string, "Interval: -%s",
		      mstimstr(ref_mark_time - level_time));
	  break;
      case 2:
	  sprintf(level_time_string, "Sample number: %ld", level_time);
	  break;
      case 3:
	  sprintf(level_time_string, "Interval: %ld samples",
		  level_time - ref_mark_time);
	  break;
    }
    invalid_data = (isigsettime(level_time) < 0 || getvec(level_v) < 0);
    for (i = 0; i < nsig; i++) {
	sprintf(level_name_string[i], "%s: ", signame[i]);
	if (invalid_data || level_v[i] == WFDB_INVALID_SAMPLE) {
	    sprintf(level_value_string[i], " ");
	    sprintf(level_units_string[i], " ");
	}
	else switch (level_mode) {
	  default:
	  case 0:	/* physical units (absolute) */
	      sprintf(level_value_string[i], "%8.3lf", aduphys(i, level_v[i]));
	      sprintf(level_units_string[i], "%s%s", sigunits[i],
		      calibrated[i] ? "" : " *"); 
	      break;
	  case 1:	/* physical units (relative) */
	      sprintf(level_value_string[i], "%8.3lf",
		      aduphys(i, level_v[i]) - aduphys(i, vref[i]));
	      sprintf(level_units_string[i], "%s%s", sigunits[i],
		      calibrated[i] ? "" : " *"); 
	      break;
	  case 2:	/* raw units (absolute) */
	      sprintf(level_value_string[i], "%6d", level_v[i]);
	      sprintf(level_units_string[i], "adu");
	      break;
	  case 3:	/* raw units (relative) */
	      sprintf(level_value_string[i], "%6d", level_v[i] - vref[i]);
	      sprintf(level_units_string[i], "adu");
	      break;
	}
    }

    if (level_popup_active < 0) create_level_popup();
    else
	xv_set(level_time_item,
	       PANEL_LABEL_STRING, level_time_string, 0);
	for (i = 0; i < nsig; i++) {
	    xv_set(level_name[i],
		   PANEL_LABEL_STRING, level_name_string[i], 0);
	    xv_set(level_value[i],
		   PANEL_LABEL_STRING, level_value_string[i], 0);
	    xv_set(level_units[i],
		   PANEL_LABEL_STRING, level_units_string[i], 0);
	}
    xv_set(level_frame, WIN_MAP, stat, 0);
    level_popup_active = stat;
}

static void dismiss_level_popup()
{
    if (level_popup_active > 0) {
	xv_set(level_frame, WIN_MAP, FALSE, 0);
	level_popup_active = 0;
    }
}

void create_level_popup()
{
    int i;
    Icon icon;

    if (level_popup_active >= 0) return;
    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Levels",
		     NULL);
    level_frame = xv_create(frame, FRAME,
	XV_LABEL, "Levels",
	FRAME_ICON, icon, 0);
    level_panel = xv_create(level_frame, PANEL,
			    WIN_ROW_GAP, 12,
			    WIN_COLUMN_GAP, 70,
			    0);
    level_mode_item = xv_create(level_panel, PANEL_CHOICE,
			 PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			 PANEL_LABEL_STRING, "Show: ",
			 XV_HELP_DATA, "wave:level.show",
			 PANEL_CHOICE_STRINGS,
			     "physical units (absolute)",
			     "physical units (relative)",
			     "raw units (absolute)",
			     "raw units (relative)", NULL,
			 PANEL_VALUE, 0,
			 PANEL_NOTIFY_PROC, set_level_mode,
			 PANEL_CLIENT_DATA, (caddr_t) 's',
			 0);
    level_time_item = xv_create(level_panel, PANEL_MESSAGE,
				XV_X, xv_col(level_panel, 0),
				XV_Y, xv_row(level_panel, 1),
				PANEL_LABEL_STRING, level_time_string,
				PANEL_LABEL_BOLD, TRUE,
				XV_HELP_DATA, "wave:level.time",
				0);
   for (i = 0; i < nsig; i++) {
	level_name[i] = xv_create(level_panel, PANEL_MESSAGE,
				  XV_X, xv_col(level_panel, 0),
				  XV_Y, xv_row(level_panel, i+2),
				  PANEL_LABEL_STRING, level_name_string[i],
				  PANEL_LABEL_BOLD, TRUE,
				  PANEL_VALUE_DISPLAY_LENGTH, 10,
				  XV_HELP_DATA, "wave:level.signame",
				  0);
	level_value[i] = xv_create(level_panel, PANEL_MESSAGE,
				  XV_X, xv_col(level_panel, 1),
				  XV_Y, xv_row(level_panel, i+2),
				  PANEL_LABEL_STRING, level_value_string[i],
				  PANEL_VALUE_DISPLAY_LENGTH, 10,
				  XV_HELP_DATA, "wave:level.value",
				  0);
	level_units[i] = xv_create(level_panel, PANEL_MESSAGE,
				  XV_X, xv_col(level_panel, 2),
				  XV_Y, xv_row(level_panel, i+2),
				  PANEL_LABEL_STRING, level_units_string[i],
				  PANEL_VALUE_DISPLAY_LENGTH, 10,
				  XV_HELP_DATA, "wave:level.units",
				  0);
    }
				  
    xv_create(level_panel, PANEL_BUTTON,
	      XV_X, xv_col(level_panel, 1),
	      XV_Y, xv_row(level_panel, nsig+3),
	      PANEL_LABEL_STRING, "Dismiss",
	      XV_HELP_DATA, "wave:level.dismiss",
	      PANEL_NOTIFY_PROC, dismiss_level_popup,
	      0);
    window_fit(level_panel);
    window_fit(level_frame);
    level_popup_active = 0;
}

void bar(x, y, do_bar)
int x, y, do_bar;
{
    int  ya = y - mmy(8) - 1, yb = y + mmy(5) + 1;
    static int level_on;
    static XSegment bar[2];

    /* Erase any other bar and levels. */
    if (bar_on) {
	XDrawSegments(display, osb, clear_crs, bar, 2);
	bar_on = 0;
    }
    if (level_on) {
	XDrawSegments(display, osb, clear_crs, level, level_on);
	level_on = 0;
    }
    if (do_bar && 0 <= x && x < canvas_width) {
	bar[0].x1 =    bar[0].x2 =     bar[1].x1 =     bar[1].x2 = bar_x = x;
	bar[0].y1 = 0; bar[0].y2 = ya; bar[1].y1 = yb; bar[1].y2=canvas_height;
	XDrawSegments(display, osb, draw_crs, bar, 2);
	bar_on = 1;
	bar_y = y;
	if (show_level) {
	    int i, n;

	    n = sig_mode ? siglistlen : nsig;
	    for (i = 0; i < n; i++) {
		level[level_on].x1 = 0;
		level[level_on].x2 = canvas_width;
		level[level_on].y1 = level[level_on].y2 = sigy(i, x);
		level_on++;
	    }
	    if (level_on) {
		XDrawSegments(display, osb, draw_crs, level, level_on);
		level_time = display_start_time + x/tscale;
		show_level_popup(TRUE);
	    }
	}
    }
}

int box_on, box_left, box_xc, box_yc, box_right, box_top, box_bottom;

void box(x, y, do_box)
int x, do_box;
{
    static XPoint box[5];

    /* Clear any other box. */
    if (box_on) {
	XDrawLines(display, osb, clear_crs, box, 5, CoordModeOrigin);
	box_on = 0;
    }
    if (do_box && 0 <= x && x < canvas_width) {
	box_xc = x; box_yc = y;
	box[0].x = box[1].x = box[4].x = box_left = x - mmx(1.5);
	box[2].x = box[3].x = box_right = x + mmx(2.5);
	box[0].y = box[3].y = box[4].y = box_bottom = y - mmy(7.5);
	box[1].y = box[2].y = box_top = y + mmy(4.5);
	XDrawLines(display, osb, draw_crs, box, 5, CoordModeOrigin);
	box_on = 1;
    }
}

/* This function redraws the box and bars, if any, after the ECG window has
   been damaged.  Do not call it except from the repaint procedure. */
void restore_cursor()
{
    if (bar_on) {
	bar_on = 0;
	bar(bar_x, bar_y, 1);
    }
    if (box_on) {
	box_on = 0;
	box(box_xc, box_yc, 1);
    }
}

static int in_box(x, y)
int x, y;
{
    return (box_on && box_left <= x && x <= box_right &&
	box_bottom <= y && y <= box_top);
}

static void attach_ann(a)
struct ap *a;
{
    int y;
    void set_frame_footer();

    attached = a;
    if (ann_mode == 1 && (unsigned)a->this.chan < nsig) {
	if (sig_mode == 0)
	    y = base[(unsigned)a->this.chan] + mmy(2);
	else {
	    int i;

	    y = abase;
	    for (i = 0; i < siglistlen; i++)
		if (a->this.chan == siglist[i]) {
		    y = base[i] + mmy(2);
		    break;
		}
	}
    }
    else
	y = abase;
    box((int)((a->this.time - display_start_time)*tscale), y, 1);
    set_frame_footer();
}

static void detach_ann()
{
    void set_frame_footer();

    attached = NULL;
    box(0, 0, 0);
    set_frame_footer();
}

#define ANNTEMPSTACKSIZE	16
static struct WFDB_ann ann_stack[ANNTEMPSTACKSIZE];
static int ann_stack_index;

static int safestrcmp(a, b)
char *a, *b;
{
    if (a == NULL) return (b != NULL);
    else if (b == NULL) return (-1);
    else return (strcmp(a, b));
}

static void save_ann_template()
{
    int i;

    /* Search for ann_template in ann_stack.  We don't bother checking the
       last entry in ann_stack since we'll push ann_template onto the top
       of the stack anyway. */
    for (i = 0; i < ANNTEMPSTACKSIZE-1; i++) {
	if (ann_stack[i].anntyp == ann_template.anntyp &&
	    ann_stack[i].subtyp == ann_template.subtyp &&
	    /* note: chan fields do not have to match */
	    ann_stack[i].num == ann_template.num &&
	    safestrcmp(ann_stack[i].aux, ann_template.aux) == 0)
	    break;	/* ann_template is already in the stack */
    }
    /* Make room for ann_template at the top of the stack by discarding
       the copy of ann_template we found in the stack, or the least recently
       used template if we didn't find ann_template in the stack. */
    for ( ; i > 0; i--)
	ann_stack[i] = ann_stack[i-1];
    ann_stack[ann_stack_index = 0] = ann_template;
}

static void set_ann_template(a)
struct WFDB_ann *a;
{
    if (ann_template.anntyp != a->anntyp ||
	ann_template.subtyp != a->subtyp ||
	ann_template.num    != a->num    ||
	safestrcmp(ann_template.aux, a->aux) != 0) {
	ann_template = *a;
	set_anntyp(ann_template.anntyp);
	set_ann_aux(ann_template.aux);
	set_ann_subtyp(ann_template.subtyp);
	set_ann_chan(ann_template.chan);
	set_ann_num(ann_template.num);
    }
}

static void set_next_ann_template()
{
    if (ann_stack_index > 0)
	set_ann_template(&ann_stack[--ann_stack_index]);
}

static void set_prev_ann_template()
{
    if (ann_stack_index < ANNTEMPSTACKSIZE-1)
	set_ann_template(&ann_stack[++ann_stack_index]);
}

/* Parse the aux string of a LINK annotation to obtain the URL of the external
   data, and open the URL in a browser window. */
static void parse_and_open_url(s)
char *s;	/* aux string -- first byte is count of bytes to follow */
{
    char *p1, *p2, *p3;
    int use_path = 1;

    if (s == NULL || *s == 0 || *(s+1) == ' ' || *(s+1) == '\t')
	return;		/* no link defined, do nothing */
    p1 = p2 = s + 1;	/* first data byte */
    p3 = p1 + *s; 	/* first byte after valid data */

    /* First, scan the string to see if it includes a protocol prefix
       (typically `http:' or `ftp:'), a tag suffix (`#' followed by a string
       specifying a location within a file), or a label (a string following
       a space, not a part of the URL but intended to be displayed by WAVE). */
    while (p2 < p3) {
	if (*p2 == ':')
	    /* The URL appears to include a protocol prefix -- pass the string
	       to the browser as is except for removal of the label, if any. */
	    use_path = 0;
	else if (*p2 == ' ' || *p2 == '\t')
	    /* There is a label in the string -- we don't need to scan further.
	       p2 marks the end of the URL. */
	    break;
	else if (*p2 == '#' && use_path)
	    /* The URL includes a tag, and it's also incomplete.  In this case,
	       we need to find the file specified by the portion of the string
	       up to p2, then furnish the complete pathname to the browser,
	       appending the tag again (but still removing the label, if any).
	       We don't need to scan further just yet. */
	    break;
	p2++;
    }
    strncpy(url, p1, p2-p1);
    url[p2-p1] = '\0';
    if (use_path == 0 || url[0] == '/') {
	/* Here, we have either a full URL with a protocol prefix (if use_path
	   is 0) or an absolute pathname of a local file, possibly with a tag
	   suffix.  In either case, the URL can be passed to the browser as is.
	   */
	open_url();
	return;
    }

    /* In this case, the string specifies a relative pathname (possibly
       referring to a file in another directory in the WFDB path).  First,
       let's try to find the file using wfdbfile to search the WFDB path. */
    if (p1 = wfdbfile(url, NULL)) { /* Success: wfdbfile has found the file! */
	if (*p1 != '/') {
	    /* We still have only a relative pathname, and we need an absolute
	       pathname (the browser may not accept a relative path, and may
	       have a different working directory than WAVE in any case). */
	    static char *cwd;

	    if (cwd == NULL) cwd = getcwd(NULL, 256);
	    if (cwd && strlen(cwd) + strlen(p1) < sizeof(url) - 1)
		sprintf(url, "%s/%s", cwd, p1);
	    else {
		strncpy(url, p1, sizeof(url)-1);
		url[strlen(url)] = '\0';
	    }
	}
	else
	    strcpy(url, p1);
	/* We should now have a null-terminated absolute pathname in url. */
    }
    
    /* We still need to reattach the tag suffix, if any, to the URL. */
    if (*p2 == '#') {
	p1 = url + strlen(url);
	while (p1 < url+sizeof(url)-1 && p2 < p3 &&
	       *p2 != ' ' && *p2 != '\t' && *p2 != '\0')
	    *p1++ = *p2++;
	*p1 = '\0';
    }

    /* We should now have a complete URL (unless wfdbfile or getcwd failed
       above, in which case we'll let the browser try to find it anyway). */
    open_url();
}

/* Handle events in the ECG display window. */
void window_event_proc(window, event, arg)
Xv_Window window;
Event *event;
Notify_arg arg;
{
    int e, i, x, y;
    long t, tt;
    struct ap *a;
    static int left_down, middle_down, right_down, redrawing, dragged, warped;
    void delete_annotation(), move_annotation();

    e = (int)event_id(event);
    x = (int)event_x(event);
    y = (int)event_y(event);
    t = display_start_time + x/tscale;
    if (atimeres > 1)	/* drop to next lower frame boundary if reading a
			   multi-frequency record in WFDB_HIGHRES mode (see
			   init.c) */
	t -= (t % atimeres);

    /* If there is an attached (selected) annotation, detach it if it is no
       longer on-screen (the user may have used a main control panel button
       to move to another location in the record). */
    if (attached && (attached->this.time < display_start_time ||
		     attached->this.time >= display_start_time + nsamp))
	detach_ann();

    if (event_action(event) == ACTION_HELP)
        xv_help_show(window, "wave:canvas", event);

    /* Handle simple keyboard events. */
    else if (event_is_ascii(event)) {
	if (event_is_up(event)) return;
	if (e == '.') ann_template.anntyp = NOTQRS;
	else if (e == ':') ann_template.anntyp = INDEX_MARK;
	else if (e == '<') ann_template.anntyp = BEGIN_ANALYSIS;
	else if (e == '>') ann_template.anntyp = END_ANALYSIS;
	else if (e == ';') ann_template.anntyp = REF_MARK;
	else if (e == '\r' && attached && attached->this.anntyp == LINK)
	    parse_and_open_url(attached->this.aux);
	else if (e == '+' && event_ctrl_is_down(event)) {
	    /* Increase size of selected signal, if any */
	    if (0 <= selected && selected < nsig)
		vmag[selected] *= 1.1;
	    /* or of all signals, otherwise */
	    else
		for (i = 0; i < nsig; i++)
		    vmag[i] *= 1.1;
	    vscale[0] = 0.0;
	    calibrate();
	    disp_proc(XV_NULL, (Event *) '.');
	}
	else if (e == '-' && event_ctrl_is_down(event)) {
	    /* Decrease size of selected signal, if any */
	    if (0 <= selected && selected < nsig)
		vmag[selected] /= 1.1;
	    /* or of all signals, otherwise */
	    else
		for (i = 0; i < nsig; i++)
		    vmag[i] /= 1.1;
	    vscale[0] = 0.0;
	    calibrate();
	    disp_proc(XV_NULL, (Event *) '.');
	}
	else if (e == '*' && event_ctrl_is_down(event)) {
	    /* Invert selected signal, if any */
	    if (0 <= selected && selected < nsig)
		vmag[selected] *= -1.0;
	    /* or all signals, otherwise */
	    else
		for (i = 0; i < nsig; i++)
		    vmag[i] *= -1.0;
	    vscale[0] = 0.0;
	    calibrate();
	    disp_proc(XV_NULL, (Event *) '.');
	}
	else if (e == ')' && event_ctrl_is_down(event)) {
	    /* Show more context, less detail (zoom out) */
	    tmag /= 1.01;
	    clear_cache();
	    if (display_start_time < 0)
		display_start_time = -display_start_time;
	    display_start_time -= (nsamp + 100)/200;
	    if (display_start_time < 0) display_start_time = 0;
	    calibrate();
	    disp_proc(XV_NULL, (Event *) '^');
	}
	else if (e == '(' && event_ctrl_is_down(event)) {
	    /* Show less context, more detail (zoom in) */
	    tmag *= 1.01;
	    clear_cache();
	    if (display_start_time < 0)
		display_start_time = -display_start_time;
	    display_start_time += (nsamp + 99)/198;
	    calibrate();
	    disp_proc(XV_NULL, (Event *) '^');
	}
	else if (e == '=' && event_ctrl_is_down(event)) {
	    /* Reset size of selected signal, if any */
	    if (0 <= selected && selected < nsig)
		vmag[selected] = 1.0;
	    /* or of all signals, otherwise */
	    else
		for (i = 0; i < nsig; i++)
		    vmag[i] = 1.0;
	    /* Reset time scale */
	    tmag = 1.0;
	    vscale[0] = 0.0;
	    if (display_start_time < 0)
		display_start_time = -display_start_time;
	    display_start_time += nsamp/2;
	    calibrate();
	    display_start_time -= nsamp/2;
	    if (display_start_time < 0)
		display_start_time = 0;
	    disp_proc(XV_NULL, (Event *) '^');
	}
	else {
	    static char es[2];

	    es[0] = e;
	    if ((i = strann(es)) != NOTQRS) ann_template.anntyp = i;
	}
	if (ann_popup_active < 0) show_ann_template();
	if (ann_template.anntyp != -1)
	    set_anntyp(ann_template.anntyp);
    }

    /* Lock out mouse and other events while display is being updated. */
    else if (redrawing) return;

    /* Handle mouse and other events. */
    else switch (e) {
      case LOC_WINENTER:	/* This doesn't seem to do anything useful. */
	win_set_kbd_focus(window, xv_get(window, XV_XID));
	break;
      case KEY_LEFT(6):		/* <Copy>: copy selected annotation into
				   Annotation Template */
      case KEY_TOP(6):		/* <F6>: same as <Copy> */
        if (attached) {
	    set_ann_template(&(attached->this));
	    save_ann_template();
	}
	break;
      case KEY_LEFT(9):		/* <Find>:  search */
      case KEY_TOP(9):		/* <F9>: same as <Find> */
	if (event_is_down(event)) {
	    if (event_shift_is_down(event)) {
		if (event_ctrl_is_down(event))	/* <ctrl><shift><Find>: home */
		    disp_proc(XV_NULL, (Event *) 'h');
		else				/* <shift><Find>: end */
		    disp_proc(XV_NULL, (Event *) 'e');
	    }
	    else if (event_ctrl_is_down(event))	/* <ctrl>+<Find>: backward */
		disp_proc(XV_NULL, (Event *) '[');
	    else				/* <Find>: forward */
		disp_proc(XV_NULL, (Event *) ']');
	}
	selected = -1;
	break; 

      /* <F10> = +half-screen		<shift><F10> = -half_screen
	 <ctrl><F10> = +full-screen	<shift><ctrl><F10> = -full-screen
       */
      case KEY_LEFT(10):
      case KEY_TOP(10):
	  if (event_is_down(event)) {
	    if (event_shift_is_down(event)) {
		if (event_ctrl_is_down(event))
		    disp_proc(XV_NULL, (Event *) '<');
	        else
		    disp_proc(XV_NULL, (Event *) '(');
	    }
	    else {
		if (event_ctrl_is_down(event))
		    disp_proc(XV_NULL, (Event *) '>');
		else
		    disp_proc(XV_NULL, (Event *) ')');
	    }
	}
	if (event_is_down(event)) {
	}
	selected = -1;
	break;
      case KEY_RIGHT(7):	/* home:
				   Ignore key release events.

				   Invoke disp_proc to move to the beginning of
				   the record. */
	if (event_is_down(event))
	    disp_proc(XV_NULL, (Event *) 'h');	/* strange but correct! */
	selected = -1;
	break;
      case KEY_RIGHT(13):	/* end:
				   Ignore key release events.

				   Invoke disp_proc to move to the end of
				   the record. */
	if (event_is_down(event))
	    disp_proc(XV_NULL, (Event *) 'e');
	selected = -1;
	break;
      case KEY_RIGHT(9):	/* page-up:
				   Ignore key release events.

				   Invoke disp_proc to move to the previous
				   frame. */
	if (event_is_down(event)) {
	    if (event_ctrl_is_down(event))
		disp_proc(XV_NULL, (Event *) '<');
	    else
		disp_proc(XV_NULL, (Event *) '(');
	}
	selected = -1;
	break;
      case KEY_RIGHT(15):	/* page-down:
				   Ignore key release events.

				   Invoke disp_proc to move to the next
				   frame. */
	if (event_is_down(event)) {
	    if (event_ctrl_is_down(event))
		disp_proc(XV_NULL, (Event *) '>');
	    else
		disp_proc(XV_NULL, (Event *) ')');
	}
	selected = -1;
	break;

      case KEY_RIGHT(8):	/* up-arrow:
				   Ignore key release events.

				   Do nothing unless in multi-edit mode and
				   an annotation with chan > 0 is attached.

				   If there is another annotation with the
				   same time as the attached annotation, but
				   on the previous signal, attach that
				   annotation. (`Previous signal' means with
				   a `chan' field one less than that of the
				   attached annotation.)
				   
				   Otherwise, if <control> is not down, move
				   the attached annotation to the previous
				   signal.

				   Otherwise, if <control> is down, copy the
				   attached annotation to the previous signal,
				   and attach the copy.			    */

	if (event_is_down(event) && ann_mode == 1 && attached &&
	    annp->this.chan > 0) {
	    if (accept_edit == 0) {
#ifdef NOTICE
		Xv_notice notice = xv_create((Frame)frame, NOTICE,
					     XV_SHOW, TRUE,
#else
		(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		     NOTICE_MESSAGE_STRINGS,
		     "You may not edit annotations unless you first",
		     "enable editing from the `Edit' menu.", 0,
		     NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
		xv_destroy_safe(notice);
#endif
		break;
	    }
	    if (annp->previous &&
		(annp->previous)->this.time == annp->this.time &&
		(annp->previous)->this.chan == annp->this.chan - 1)
		attach_ann(annp->previous);
	    else {
		struct ap *a;

		if (event_ctrl_is_down(event) && (a = get_ap())) {
		    a->this = annp->this;
		    a->this.chan--;
		    if (a->this.aux) {
			char *p;

			if ((p = (char *)calloc(*(a->this.aux)+2,1)) == NULL) {
#ifdef NOTICE
			    Xv_notice notice = xv_create((Frame)frame, NOTICE,
							 XV_SHOW, TRUE,
#else
			    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
				      NOTICE_MESSAGE_STRINGS,
				      "This annotation cannot be inserted",
				      "because there is insufficient memory.",
				      0,
				      NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
			    xv_destroy_safe(notice);
#endif
			    return;
			}
			memcpy(p, a->this.aux, *(a->this.aux)+1);
			a->this.aux = p;
		    }
		    insert_annotation(a);
		    set_ann_template(&(a->this));
		    save_ann_template();
		    attach_ann(a);
		}
		else {
		    annp->this.chan--;
		    check_post_update();
		}
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    annp = attached;
	    x = (attached->this.time - display_start_time)*tscale;
	    if (sig_mode == 0)
		y = base[(unsigned)attached->this.chan] + mmy(2);
	    else {
		int i;

		y = abase;
		for (i = 0; i < siglistlen; i++)
		    if (attached->this.chan == siglist[i]) {
			y = base[i] + mmy(2);
			break;
		    }
	    }
	    warped = 1;
	    xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	}
	break;
      case KEY_RIGHT(14):	/* down-arrow:
				   Ignore key release events.

				   Do nothing unless in multi-edit mode and an
				   annotation with chan < nsig-1 is attached.

				   If there is another annotation with the
				   same time as the attached annotation, but
				   on the next signal, attach that annotation.
				   (`Next signal' means with a `chan' field one
				   more than that of the attached annotation.)
				   
				   Otherwise, if <control> is not down, move
				   the attached annotation to the next signal.

				   Otherwise, if <control> is down, copy the
				   attached annotation to the next signal, and
				   attach the copy.			    */

	if (event_is_down(event) && ann_mode == 1 && attached &&
	    annp->this.chan < nsig-1) {
	    if (accept_edit == 0) {
#ifdef NOTICE
		Xv_notice notice = xv_create((Frame)frame, NOTICE,
					     XV_SHOW, TRUE,
#else
	        (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		     NOTICE_MESSAGE_STRINGS,
		     "You may not edit annotations unless you first",
		     "enable editing from the `Edit' menu.", 0,
		     NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
		xv_destroy_safe(notice);
#endif
		break;
	    }
	    if (annp->next &&
		(annp->next)->this.time == annp->this.time &&
		(annp->next)->this.chan == annp->this.chan + 1)
		attach_ann(annp->next);
	    else {
		struct ap *a;

		if (event_ctrl_is_down(event) && (a = get_ap())) {
		    a->this = annp->this;
		    a->this.chan++;
		    if (a->this.aux) {
			char *p;

			if ((p = (char *)calloc(*(a->this.aux)+2,1)) == NULL) {
#ifdef NOTICE
			    Xv_notice notice = xv_create((Frame)frame, NOTICE,
							 XV_SHOW, TRUE,
#else
			    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
				      NOTICE_MESSAGE_STRINGS,
				      "This annotation cannot be inserted",
				      "because there is insufficient memory.",
				      0,
				      NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
			    xv_destroy_safe(notice);
#endif
			    return;
			}
			memcpy(p, a->this.aux, *(a->this.aux)+1);
			a->this.aux = p;
		    }
		    insert_annotation(a);
		    set_ann_template(&(a->this));
		    save_ann_template();
		    attach_ann(a);
		}
		else {
		    annp->this.chan++;
		    check_post_update();
		}
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    annp = attached;
	    x = (attached->this.time - display_start_time)*tscale;
	    if (sig_mode == 0)
		y = base[(unsigned)attached->this.chan] + mmy(2);
	    else {
		int i;

		y = abase;
		for (i = 0; i < siglistlen; i++)
		    if (attached->this.chan == siglist[i]) {
			y = base[i] + mmy(2);
			break;
		    }
	    }
	    warped = 1;
	    xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	}
	break;
      case KEY_RIGHT(10):	/* left-arrow: simulate left mouse button */
      case MS_LEFT:
	if (event_is_down(event)) {
	    /* The left button was pressed:
	         1. If the <Shift> key is depressed, select the signal nearest
		    the pointer and return. (A selected signal is highlighted.)
		 2. If the <Control> key is depressed, select the signal
		    nearest the pointer, insert it into the signal list, and
		    return.
		 3. If the <Meta> key is depressed, select the signal nearest
		    the pointer, delete its first occurrence (if any) in the
		    signal list, and return.
		 4. If annotation editing is disabled and  if this instance of
		    WAVE has a sync button, signal other WAVE processes to
		    recenter their signal windows at the time indicated by
		    the mouse, and return.
	         5. If the middle button is down, switch the annotation
		    template to the previous entry in the annotation template
		    buffer.
	         6. Make the annotation template popup visible.
	         7. If the middle or right button is down, or if there are no
		    annotations left of the pointer, return.
		 8. If annotations are shown attached to signals, and the
		    pointer is in a selection box, attach the previous
		    annotation.
		 9. If annotations are shown attached to signals, and the
		    pointer is not in a selection box, attach the closest
		    annotation to the left of the pointer.
		10. Otherwise, find the previous group of simultaneous
		    annotations and attach the first annotation of that group.
		11. Recenter the display around the attached annotation, if
		    it is not currently displayed.
		12. Draw marker bars above and below the attached annotation.
	    */
	    if (event_shift_is_down(event) ||
		event_ctrl_is_down(event) ||
		event_meta_is_down(event)) {
		int d, dmin = -1, i, imin = -1, n;

		n = sig_mode ? siglistlen : nsig;
		for (i = 0; i < n; i++) {
		    d = y - base[i];
		    if (d < 0) d = -d;
		    if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
		}
		if (imin >= 0) {
		    set_signal_choice(imin);
		    if (selected == imin) selected = -1;
		    else selected = imin;
		    if (event_ctrl_is_down(event)) add_signal_choice();
		    if (event_meta_is_down(event)) delete_signal_choice();
		}
		break;
	    }
	    dragged = 0;
	    if (accept_edit == 0 && wave_ppid) {
		char buf[80];
		sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
			mstimstr(-t));
		system(buf);
		break;
	    }
	    if (middle_down) set_prev_ann_template();
	    show_ann_template();
	    if (middle_down || right_down) break;
	    left_down = 1;
	    (void)locate_annotation(t, -128);	/* -128 is lowest chan value */
	    if (annp) {
		if (annp->previous) annp = annp->previous;
		else break;
	    }
	    else if (ap_end) annp = ap_end;
	    else break;
	    redrawing = 1;
	    if (ann_mode == 1) {
		if (attached && in_box(x, y)) {
		    if (attached->previous) annp = attached->previous;
		    else annp = attached;
		}
		else {
		    double d, dx, dy, dmin = -1.0;
		    struct ap *a = annp;

		    while (a->next && annp->this.time == (a->next)->this.time)
			a = a->next;
		    while (a && a->this.time >= display_start_time) {
			dx = x - (a->this.time - display_start_time)*tscale;
			if (sig_mode == 0)
			    dy = y - (base[a->this.chan] + mmy(2));
			else {
			    int i;

			    dy = y - abase;
			    for (i = 0; i < siglistlen; i++)
				if (a->this.chan == siglist[i]) {
				    dy = y - (base[i] + mmy(2));
				    break;
				}
			}
			d = dx*dx + dy*dy;
			if (dmin < 0. || d < dmin) {
			    dmin = d;
			    annp = a;
			}
			a = a->previous;
		    }
		}
	    }
	    if (annp->this.time < display_start_time) {
		struct ap *a = annp;

		XFillRectangle(display, osb, clear_all,
			       0, 0, canvas_width+mmx(10), canvas_height);
		if ((tt = annp->this.time - (long)((nsamp-freq)/2)) < 0L)
		    display_start_time = 0L;
		else
		    display_start_time = strtim(timstr(tt));
		do_disp();
		left_down = bar_on = box_on = 0;
		annp = a;
	    }

	    /* Attach the annotation, move the pointer to it, and draw the
	       marker bars. */
	    attach_ann(annp);
	    x = (annp->this.time - display_start_time)*tscale;
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		if (sig_mode == 0)
		    y = base[(unsigned)annp->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (annp->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
	    }
	    else
		y = abase;
	    warped = 1;
	    xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	    bar(x, y, 1);
	    redrawing = 0;
	}
	else {
	    /* The left button was released:
	         1. If the initial button press occurred while mouse events
		    were locked out, ignore this event.
		 2. If there is an attached annotation, and the pointer has
		    been dragged outside the box, move the annotation to the
		    pointer (keeping it attached), and redraw the annotations.
		 3. Erase the marker bars.
            */
	    if (!left_down) break;
	    left_down = 0;
	    if (attached && dragged && !in_box(x, y)) {
		move_annotation(attached, t);
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    bar(x, 0, 0);
	}
	break;
      case KEY_LEFT(2):
      case KEY_TOP(2):
      case KEY_RIGHT(11):	/* <5> on numeric keypad: simulate middle
				   mouse button */
      case MS_MIDDLE:
	if (event_is_down(event)) {
	    /* The middle button was pressed:
	         1. If the left or right button is down, ignore this event.
		 2. Draw marker bars above and below the pointer.
		 3. If annotation editing is disabled or if the <Control> key
		    is depressed, and if this instance of WAVE has a sync
		    button, signal other WAVE processes to recenter their
		    signal windows at the time indicated by the mouse, and
		    return.
		 4. If there is an attached annotation, and the pointer is
		    outside the box, detach the annotation (erase the box).
	    */
	    if (left_down || right_down || ann_template.anntyp < 0) break;
	    middle_down = 1;
	    bar(x, y /* ? */, 1);
	    if ((accept_edit == 0 || event_ctrl_is_down(event)) && wave_ppid) {
		char buf[80];
		sprintf(buf, "wave-remote -pid %d -f '%s'\n", wave_ppid,
			mstimstr(-t));
		system(buf);
	    }
	    else if (attached && !in_box(x, y))
		detach_ann();
	    break;
	}
	else {
	    /* The middle button was released:
	         1. If the initial button press occurred while mouse events
		    were locked out, or while the left or right buttons were
		    down, ignore this event.
		 2. If ann_mode is 1 (i.e., if annotations are attached
		    to signals), set the `chan' field of the current template
		    annotation according to the y-coordinate of the pointer.
		 3. If there is an attached annotation, and the pointer is
		    inside the box, change it or delete it (according to the
		    current template annotation).
		 4. Otherwise, if the current template annotation has a valid
		    annotation type, insert and attach it.
		 5. Redraw the annotation display and clear the marker bars.
	    */
	    if (!middle_down) break;
	    middle_down = 0;
	    if (ann_mode == 1) {
		int d, dmin = -1, i, imin = -1, n;

		n = sig_mode ? siglistlen : nsig;
		for (i = 0; i < n; i++) {
		    d = y - base[i];
		    if (d < 0) d = -d;
		    if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
		}
		if (imin >= 0) {
		    if (sig_mode) imin = siglist[imin];
		    set_ann_chan(ann_template.chan = imin);
		}
	    }
	    if (attached && in_box(x, y)) {
		if (ann_template.anntyp == NOTQRS) {
		    save_ann_template();
		    delete_annotation(attached->this.time,attached->this.chan);
		    a = NULL;
		}
		else if (a = get_ap()) {
		    a->this = ann_template;
		    a->this.time = attached->this.time;
		}
	    }
	    else if (ann_template.anntyp != NOTQRS && (a = get_ap())) {
		a->this = ann_template;
		a->this.time = t;
	    }
	    else
		a = NULL;
	    if (a) {
		/* There is an annotation to be inserted.  Copy the aux string,
		   if any (since the template aux pointer points to static
		   memory that can be changed at any time by the user). */
		if (a->this.aux) {
		    char *p;

		    if ((p = (char *)calloc(*(a->this.aux)+2, 1)) == NULL) {
#ifdef NOTICE
			Xv_notice notice = xv_create((Frame)frame, NOTICE,
						     XV_SHOW, TRUE,
#else
			(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
				      NOTICE_MESSAGE_STRINGS,
				      "This annotation cannot be inserted",
				      "because there is insufficient memory.",
				      0,
				      NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
			xv_destroy_safe(notice);
#endif
			return;
		    }
		    memcpy(p, a->this.aux, *(a->this.aux)+1);
		    a->this.aux = p;
		}
		insert_annotation(a);
		set_ann_template(&(a->this));
		save_ann_template();
	    }
	    box(0,0,0);
	    bar(0,0,0);
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	    box_on = 0;
	    bar(x,0,0);
	}
	break;
      case KEY_RIGHT(12):	/* right-arrow: simulate right mouse button */
      case MS_RIGHT:
	if (event_is_down(event)) {
	    /* The right button was pressed:
	         1. If the middle button is down, switch the annotation
		    template to the next entry in the annotation template
		    stack, and make the annotation template popup visible.
	         2. If the left or middle button is down, or if there are no
		    annotations right of the pointer, ignore this event.
		 3. If annotations are shown attached to signals, and the
		    pointer is not in a selection box, attach the closest
		    annotation to the right of the pointer.
		 4. Otherwise, attach the next annotation.
		 5. Recenter the display around the attached annotation, if
		    it is not currently displayed.
		 6. Draw marker bars above and below the attached annotation.
	    */
	    if (middle_down) {
		set_next_ann_template();
		show_ann_template();
	    }
	    if (left_down || middle_down) break;
	    dragged = 0;
	    right_down = 1;
	    if (attached && in_box(x, y)) annp = attached->next;
	    else (void)locate_annotation(t, -128);
	    if (annp == NULL) break;
	    redrawing = 1;
	    if (ann_mode == 1 && (!attached || !in_box(x, y))) {
		double d, dx, dy, dmin = -1.0;
		struct ap *a = annp;

		while (a && a->this.time < display_start_time + nsamp) {
		    dx = x - (a->this.time - display_start_time)*tscale;
		    if (sig_mode == 0)
			dy = y - (base[a->this.chan] + mmy(2));
		    else {
			int i;

			dy = y - abase;
			for (i = 0; i < siglistlen; i++)
			    if (a->this.chan == siglist[i]) {
				dy = y - (base[i] + mmy(2));
				break;
			    }
		    }
		    d = dx*dx + dy*dy;
		    if (dmin < 0. || d < dmin) {
			dmin = d;
			annp = a;
		    }
		    a = a->next;
		}
	    }
	    if (annp->this.time >= display_start_time + nsamp) {
		struct ap *a = annp;

		XFillRectangle(display, osb, clear_all,
			       0, 0, canvas_width+mmx(10), canvas_height);
		tt = annp->this.time - (long)((nsamp-freq)/2);
		display_start_time = strtim(timstr(tt));
		do_disp();
		right_down = bar_on = box_on = 0;
		annp = a;
	    }

	    /* Attach the annotation, move the pointer to it, and draw the
	       marker bars. */
	    attach_ann(annp);
	    x = (annp->this.time - display_start_time)*tscale;
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		if (sig_mode == 0)
		    y = base[(unsigned)annp->this.chan] + mmy(2);
		else {
		    int i;

		    y = abase;
		    for (i = 0; i < siglistlen; i++)
			if (annp->this.chan == siglist[i]) {
			    y = base[i] + mmy(2);
			    break;
			}
		}
	    }
	    else
		y = abase;
	    warped = 1;
	    xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	    bar(x, y, 1);
	    redrawing = 0;
	}
	else {
	    /* The right button was released:
	         1. If the initial button press occurred while mouse events
		    were locked out, ignore this event.
		 2. If there is an attached annotation, and the pointer has
		    been dragged outside the box, move the annotation to the
		    pointer (keeping it attached), and redraw the annotations.
		 3. Erase the marker bars.
            */
	    if (!right_down) break;
	    right_down = 0;
	    if (attached && dragged && !in_box(x, y)) {
		move_annotation(attached, t);
		box(0,0,0);
		bar(0,0,0);
		clear_annotation_display();
		show_annotations(display_start_time, nsamp);
		box_on = dragged = 0;
		attach_ann(attached);
	    }
	    bar(x,0,0);
	}
	break;
      case KEY_LEFT(3):
      case KEY_TOP(3):
      case KEY_RIGHT(4):	/* <=> on numeric keypad: simulate drag left */
	  {
	      static int count = 1;

	      if (event_is_down(event)) {
		  if ((x -= count) < 0) x = 0;
		  if (count < 100) count++;
	      }
	      else
		  count = 1;
	  }
	warped = 1;
	if (middle_down) bar(x, y, 1);
	xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	break;
      case KEY_LEFT(4):
      case KEY_TOP(4):
      case KEY_RIGHT(6):	/* <*> on numeric keypad: simulate drag right*/
	  {
	      static int count = 1;

	      if (event_is_down(event)) {
		  if ((x += count) >= canvas_width) x = canvas_width - 1;
		  if (count < 100) count++;
	      }
	      else
		  count = 1;
	  }
	warped = 1;
	xv_set(window, WIN_MOUSE_XY, (short)x, (short)y, NULL);
	if (middle_down) bar(x, y, 1);
	break;
      case LOC_DRAG:
	/* The mouse moved while one or more buttons were depressed:
	     1. If the initial button press occurred while mouse events were
	        locked out, or if the current pointer abscissa matches the
		marker bar position, ignore this event.
	     2. If the pointer was warped since the previous drag event, ignore
	        this event.
	     3. If there is an attached annotation, and the pointer is inside
	        the box, move the marker bars to the box center abscissa unless
		they are there already.
	     4. If ann_mode is 1, compare the signal number of the
	        nearest signal with the `chan' field of the template
		annotation.  If these are unequal, reset the `chan' field to
		match, and redraw the marker bars to surround the selected
		signal.
	     5. Otherwise, move the marker bars to the current pointer
	        abscissa.
        */
	if ((!middle_down && !left_down && !right_down) || x == bar_x)
	    break;
	else if (warped) {
	    warped = 0;
	    break;
	}
	else if (attached && in_box(x, y)) {
	    if (bar_x != box_xc) bar(box_xc, box_yc, 1);
	}
	else if (ann_mode == 1) {
	    int d, dmin = -1, i, ii, imin = -1, n;

	    n = sig_mode ? siglistlen : nsig;
	    for (i = 0; i < n; i++) {
		d = y - base[i];
		if (d < 0) d = -d;
		if (dmin < 0 || d < dmin) { imin = i; dmin = d; }
	    }
	    ii = (imin >= 0 && sig_mode) ? siglist[imin] : imin;
	    if (imin >= 0 && ann_template.chan != ii) {
		set_ann_chan(ann_template.chan = ii);
		if (attached) attached->this.chan = ii;
	    }
	    bar(x, imin >= 0 ? base[(unsigned)imin] + mmy(2) : abase, 1);
	}
	else
	    bar(x, abase, 1);
	dragged = 1;
	break;
      default:
#ifdef DEBUG
	fprintf(stderr, "event %d at sample %ld (%s)\n", e, t, timstr(t));
#endif
	break;
    }
    XClearWindow(display, xid);
}
