/* file: mainpan.c	G. Moody	30 April 1990
			Last revised:	13 July 2010
Functions for the main control panel of WAVE

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
#include <wfdb/ecgcodes.h>
#include "xvwave.h"
#include <X11/Xos.h>		/* for <sys/time.h> */
#include <xview/notice.h>
#include <xview/textsw.h>
#include <xview/notify.h>

#ifdef NOMKSTEMP
#define mkstemp mktemp
#endif

Panel panel;	/* main control panel (above ECG window) */
Panel load_panel;	/* File menu load command window */
Panel print_setup_panel;     	/* File menu print setup window */

/* Pulldown menus from "File", "Edit", and "Properties" menu buttons */
Menu file_menu,	edit_menu, prop_menu;

Panel_item desc_item,		/* Description field (from log file) */
	   annot_item,		/* "Annotator:" field */
           find_item,		/* "Search for:" field */
           findsig_item,	/* "Find signal:" field */
	   record_item,		/* "Record:" field */
	   time_item,		/* "Start time:" field */
    	   time2_item,		/* "End time:" field */
	   wfdbpath_item,	/* "WFDB path:" field */
	   wfdbcal_item,	/* "Calibration file:" field */
	   psprint_item,	/* "PostScript print command:" field */
	   textprint_item;	/* "Text print command:" field */

Frame load_frame;
Frame print_setup_frame;

char wfdbpath[512];	/* database path */
char wfdbcal[128];	/* WFDB calibration file name */

void wfdbp_proc(item, event)
Panel_item item;
Event *event;
{
    char *p = (char *)xv_get(wfdbpath_item, PANEL_VALUE);

    if (p != wfdbpath)
	strncpy(wfdbpath, p, sizeof(wfdbpath)-1);
    setwfdb(wfdbpath);
}

void wfdbc_proc(item, event)
Panel_item item;
Event *event;
{
    char *p = (char *)xv_get(wfdbcal_item, PANEL_VALUE);

    if (strcmp(wfdbcal, p)) {
	strncpy(wfdbcal, p, sizeof(wfdbcal)-1);
	if (calopen(cfname = wfdbcal) == 0)
	    calibrate();
    }
}

static void create_load_panel()
{
    char *p, *getenv();
    Icon icon;
    void reinitialize();

    strncpy(wfdbpath, getwfdb(), sizeof(wfdbpath)-1);
    if (cfname != wfdbcal) {
	if (cfname != NULL)
	    strncpy(wfdbcal, cfname, sizeof(wfdbcal)-1);
	else if (p = getenv("WFDBCAL"))
	    strncpy(wfdbcal, p, sizeof(wfdbcal)-1);
    }
    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Load",
		     NULL);
    load_frame = xv_create(frame, FRAME_CMD,
	FRAME_LABEL, "File: Load",
	FRAME_ICON, icon, 0);
    load_panel = xv_get(load_frame, FRAME_CMD_PANEL);
    record_item = xv_create(load_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:file.load.record",
	PANEL_LABEL_STRING, "Record: ",
	PANEL_VALUE_DISPLAY_LENGTH, 32,
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_VALUE, record,
	0);
    annot_item = xv_create(load_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:file.load.annotator",
	PANEL_LABEL_STRING, "Annotator: ",
	PANEL_VALUE_DISPLAY_LENGTH, 8,
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_VALUE, annotator,
	0);
    xv_create(load_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Reload",
	      XV_HELP_DATA, "wave:file.load.reload",
	      PANEL_NOTIFY_PROC, reinitialize,
	      0);
    wfdbcal_item = xv_create(load_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:file.load.calibration_file",
	PANEL_LABEL_STRING, "Calibration file: ",
	PANEL_VALUE_DISPLAY_LENGTH, 15,
	PANEL_NOTIFY_PROC, wfdbc_proc,
	PANEL_VALUE, wfdbcal,
	0);
    wfdbpath_item = xv_create(load_panel, PANEL_TEXT,
	PANEL_NEXT_ROW, -1, 
	XV_HELP_DATA, "wave:file.load.wfdb_path",
	PANEL_LABEL_STRING, "WFDB Path: ",
	PANEL_VALUE_DISPLAY_LENGTH, 60,
	PANEL_VALUE_STORED_LENGTH, sizeof(wfdbpath),
	PANEL_NOTIFY_PROC, wfdbp_proc,
	PANEL_VALUE, wfdbpath,
	0);
    window_fit(load_panel);
    window_fit(load_frame);
}

void show_load()
{
    wmgr_top(load_frame);
    xv_set(load_frame, WIN_MAP, TRUE, 0);
}

void save_proc()
{
    if (post_changes())
	set_frame_title();
}

void print_setup_proc(item, event)
Panel_item item;
Event *event;
{
    strncpy(psprint, (char *)xv_get(psprint_item, PANEL_VALUE),
	    sizeof(psprint)-1);
    strncpy(textprint, (char *)xv_get(textprint_item, PANEL_VALUE),
	    sizeof(textprint)-1);
}

static void create_print_setup_panel()
{
    char *p, *printer, *getenv();
    Icon icon;

    if ((p = getenv("TEXTPRINT")) == NULL || strlen(p) > sizeof(textprint)-1) {
	if ((printer = getenv("PRINTER")) == NULL)
	    sprintf(textprint, "lpr");
	else
	    sprintf(textprint, "lpr -P%s", printer);
    }
    else
        strcpy(textprint, p);
    if ((p = getenv("PSPRINT")) == NULL || strlen(p) > sizeof(psprint)-1) {
	if ((printer = getenv("PRINTER")) == NULL)
	    sprintf(psprint, "lpr");
	else
	    sprintf(psprint, "lpr -P%s", printer);
    }
    else
        strcpy(psprint, p);

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Print setup",
		     NULL);
    print_setup_frame = xv_create(frame, FRAME_CMD,
				  FRAME_LABEL, "Print setup",
				  FRAME_ICON, icon, 0);
    print_setup_panel = xv_get(print_setup_frame, FRAME_CMD_PANEL);
    psprint_item = xv_create(print_setup_panel, PANEL_TEXT,
			     XV_HELP_DATA, "wave:file.printsetup.psprint",
			     PANEL_LABEL_STRING, "PostScript print command: ",
			     PANEL_VALUE_DISPLAY_LENGTH, 16,
			     PANEL_VALUE_STORED_LENGTH, sizeof(psprint),
			     PANEL_NOTIFY_PROC, print_setup_proc,
			     PANEL_VALUE, psprint,
			     NULL);
    textprint_item = xv_create(print_setup_panel, PANEL_TEXT,
			     XV_HELP_DATA, "wave:file.printsetup.textprint",
			     PANEL_LABEL_STRING, "Text print command: ",
			     PANEL_VALUE_DISPLAY_LENGTH, 16,
			     PANEL_VALUE_STORED_LENGTH, sizeof(textprint),
			     PANEL_NOTIFY_PROC, print_setup_proc,
			     PANEL_VALUE, textprint,
			     NULL);
    window_fit(print_setup_panel);
    window_fit(print_setup_frame);
}

void show_print_setup()
{
    wmgr_top(print_setup_frame);
    xv_set(print_setup_frame, WIN_MAP, TRUE, 0);
}

static void create_file_menu()
{
    create_load_panel();
    create_print_setup_panel();
    file_menu = (Menu)xv_create(XV_NULL, MENU,
	MENU_ITEM,
	    MENU_STRING, "Load...",
	    XV_HELP_DATA, "wave:file.load",
	    MENU_NOTIFY_PROC, show_load,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "Save",
	    XV_HELP_DATA, "wave:file.save",
	    MENU_NOTIFY_PROC, save_proc,
	    NULL,
        MENU_ITEM,
	    MENU_STRING, "Print",
	    XV_HELP_DATA, "wave:file.print",
	    MENU_NOTIFY_PROC, print_proc,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "Print setup...",
	    XV_HELP_DATA, "wave:file.printsetup",
	    MENU_NOTIFY_PROC, show_print_setup,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "Analyze...",
	    XV_HELP_DATA, "wave:file.analyze",
	    MENU_NOTIFY_PROC, analyze_proc,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "Log...",
	    XV_HELP_DATA, "wave:file.log",
	    MENU_NOTIFY_PROC, show_log,
	    NULL,
	NULL);
    xv_create(panel, PANEL_BUTTON,
	      /*	PANEL_NEXT_ROW, -1, */
	PANEL_LABEL_STRING, "File",
	XV_HELP_DATA, "wave:file",
	PANEL_ITEM_MENU, file_menu,
	0);
}

static void allow_editing()
{
    accept_edit = 1;
}

static void view_only()
{
    accept_edit = 0;
}

static void create_edit_menu()
{
    edit_menu = (Menu)xv_create(XV_NULL, MENU,
	MENU_ITEM,
	    MENU_STRING, "Allow editing",
	    XV_HELP_DATA, "wave:edit.allow_editing",
	    MENU_NOTIFY_PROC, allow_editing,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "View only",
	    XV_HELP_DATA, "wave:edit.view_only",
	    MENU_NOTIFY_PROC, view_only,
	    NULL,
	NULL);
    xv_create(panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Edit",
	XV_HELP_DATA, "wave:edit",
	PANEL_ITEM_MENU, edit_menu,
	0);
}

static char filename[40], *title;

static void show_print()
{
    char print_command[128];

    if (*filename) {
	if (strncmp(filename, "/tmp/wave-s", 11) == 0)
	    sprintf(print_command, "wfdbdesc $RECORD | %s\n", textprint);
	else if (strncmp(filename, "/tmp/wave-a", 11) == 0)
	    sprintf(print_command, "sumann -r $RECORD -a $ANNOTATOR | %s\n",
		    textprint);
	else
	    sprintf(print_command, "%s <%s\n", textprint, filename);
	do_command(print_command);
    }
}

static void show_file()
{
    Frame show_subframe;
    Panel show_subpanel;
    Icon icon;
    Textsw textsw;
    Textsw_status status;

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, title,
		     NULL);
    show_subframe = xv_create(frame, FRAME,
			      XV_LABEL, title,
			      FRAME_ICON, icon, 0);
    show_subpanel = xv_create(show_subframe, PANEL,
			      WIN_ROWS, 1,
			      0);
    xv_create(show_subpanel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Print",
	      XV_HELP_DATA, "wave:help.print",
	      PANEL_NOTIFY_PROC, show_print,
	      0);
    textsw = (Textsw)xv_create(show_subframe, TEXTSW,
			       WIN_BELOW, show_subpanel,
			       WIN_X, 0,
			       NULL);
    xv_set(textsw,
	   TEXTSW_STATUS, &status,
	   TEXTSW_FILE, filename,
	   TEXTSW_FIRST, 0,
	   TEXTSW_READ_ONLY, TRUE,
	   NULL);
    if (status != TEXTSW_STATUS_OKAY) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		      NOTICE_MESSAGE_STRINGS,
		      "Sorry, no property information",
		      "is available for this topic.", 0,
		      NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	xv_destroy_safe(show_subframe);
    }
    else {
	wmgr_top(show_subframe);
	xv_set(show_subframe, WIN_MAP, TRUE, 0);
    }
}

/* This function gets called once per second while the timer is running.
   Once the file named by filename contains readable data, it waits one
   more second, turns off the timer, invokes show_file, and then deletes
   the file. */
Notify_value check_file()
{
    char tstring[20];
    FILE *tfile;
    static int file_ready;

    if (file_ready) {
	notify_set_itimer_func(prop_menu, NOTIFY_FUNC_NULL, ITIMER_REAL,
			       NULL, NULL);
	show_file();
	unlink(filename);
	file_ready = 0;
	return (NOTIFY_DONE);
    }
    if ((tfile = fopen(filename, "r")) == NULL)
	return (NOTIFY_DONE);
    if (fgets(tstring, 20, tfile) == NULL) {
	fclose(tfile);
	return (NOTIFY_DONE);
    }
    fclose(tfile);
    file_ready++;
    return (NOTIFY_DONE);
}

static struct itimerval timer;

/* This function sets up the timer for check_file. */
void wait_for_file()
{
    timer.it_value.tv_sec = timer.it_interval.tv_sec = 1;
    notify_set_itimer_func(prop_menu, check_file, ITIMER_REAL,
			   &timer, NULL);
}

static char command[80];

static void prop_signals()
{
    sprintf(filename, "/tmp/wave-s.XXXXXX");
    /* The `echo' is to make sure that something gets written to the file,
       even if `wfdbdesc' doesn't work (so that check_file doesn't wait
       forever). */
    mkstemp(filename);
    sprintf(command, "(wfdbdesc $RECORD; echo =====) >%s\n", filename);
    do_command(command);
    title = "Signals";
    wait_for_file();
}

static void prop_annotations()
{
    post_changes();
    sprintf(filename, "/tmp/wave-a.XXXXXX");
    mkstemp(filename);
    sprintf(command, "(sumann -r $RECORD -a $ANNOTATOR; echo =====) >%s\n",
	    filename);
    do_command(command);
    title = "Annotations";
    wait_for_file();
}

static void prop_wave()
{
    sprintf(filename, "%s/wave/news.hlp", helpdir);
    title = "About WAVE";
    show_file();
}

static void create_prop_menu()
{
    prop_menu = (Menu)xv_create(XV_NULL, MENU,
	MENU_ITEM,
	    MENU_STRING, "Signals...",
	    XV_HELP_DATA, "wave:prop.signals",
	    MENU_NOTIFY_PROC, prop_signals,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "Annotations...",
	    XV_HELP_DATA, "wave:prop.annotations",
	    MENU_NOTIFY_PROC, prop_annotations,
	    NULL,
	MENU_ITEM,
	    MENU_STRING, "About WAVE...",
	    XV_HELP_DATA, "wave:prop.wave",
	    MENU_NOTIFY_PROC, prop_wave,
	    NULL,
	NULL);
    xv_create(panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Properties",
	XV_HELP_DATA, "wave:prop",
	PANEL_ITEM_MENU, prop_menu,
	0);
}

Frame find_frame;
Panel find_panel;

static void create_find_panel()
{
    Icon icon;

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Find",
		     NULL);
    find_frame = xv_create(frame, FRAME_CMD,
	FRAME_LABEL, "Find",
	FRAME_ICON, icon, 0);
    find_panel = xv_get(find_frame, FRAME_CMD_PANEL);
    time_item = xv_create(find_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:find.start_time",
	PANEL_LABEL_STRING, "Start time: ",
	PANEL_VALUE_DISPLAY_LENGTH, 15,
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_VALUE, "0",
	0);
    time2_item = xv_create(find_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:find.end_time",
	PANEL_LABEL_STRING, "End time: ",
	PANEL_VALUE_DISPLAY_LENGTH, 15,
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) ':',
	PANEL_VALUE, "10",
	0);
    find_item = xv_create(find_panel, PANEL_TEXT,
	PANEL_NEXT_ROW, -1, 
	XV_HELP_DATA, "wave:find.search_for",
	PANEL_LABEL_STRING, "Search for annotation: ",
        PANEL_VALUE_DISPLAY_LENGTH, 6,
	PANEL_NOTIFY_PROC, disp_proc,
        PANEL_CLIENT_DATA, (caddr_t) ']',
	PANEL_VALUE, "",
	0);				  
    xv_create(find_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "More options...",
	      XV_HELP_DATA, "wave:find.more_options",
	      PANEL_NOTIFY_PROC, show_search_template,
	      NULL);
    findsig_item = xv_create(find_panel, PANEL_TEXT,
	XV_HELP_DATA, "wave:find.search_for_signal",
	PANEL_LABEL_STRING, "Find signal: ",
        PANEL_VALUE_DISPLAY_LENGTH, 6,
	PANEL_NOTIFY_PROC, disp_proc,
        PANEL_CLIENT_DATA, (caddr_t) '}',
	PANEL_VALUE, "",
	0);				  
    window_fit(find_panel);
    window_fit(find_frame);
}

void show_find()
{
    wmgr_top(find_frame);
    xv_set(find_frame, WIN_MAP, TRUE, 0);
}

/* Set up control/status panel at top of frame. */
Panel create_main_panel()
{
    int dx;

    panel = xv_create(frame, PANEL,
		      XV_X, 0,	/* seems necessary to avoid getting an empty
				   space at the left edge - why? */
		      XV_HELP_DATA, "wave:main_panel",
		      0);

    dx = xv_get(panel, PANEL_ITEM_X_GAP);
    xv_set(panel, PANEL_ITEM_Y_GAP, 5, 0);
    create_file_menu();

    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:view",
	PANEL_LABEL_STRING, "View...",
	PANEL_NOTIFY_PROC, show_mode,
	0);

    create_edit_menu();

    create_prop_menu();

    xv_set(panel, PANEL_ITEM_X_GAP, 4*dx, 0);
    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:<search",
	PANEL_LABEL_STRING, "< Search",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) '[',
	0);

    xv_set(panel, PANEL_ITEM_X_GAP, dx, 0);
    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:<<",
	PANEL_LABEL_STRING, "<<",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) '<',
	0);

    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:<",
	PANEL_LABEL_STRING, "<",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) '(',
	0);

    create_find_panel();
    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:find",
	PANEL_LABEL_STRING, "Find...",
	PANEL_NOTIFY_PROC, show_find,
	0);

    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:>",
	PANEL_LABEL_STRING, ">",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) ')',
	0);

    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:>>",
	PANEL_LABEL_STRING, ">>",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) '>',
	0);

    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:search>",
	PANEL_LABEL_STRING, "Search >",
	PANEL_NOTIFY_PROC, disp_proc,
	PANEL_CLIENT_DATA, (caddr_t) ']',
	0);

    xv_set(panel, PANEL_ITEM_X_GAP, 4*dx, 0);
    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:help",
	PANEL_LABEL_STRING, "Help",
        PANEL_NOTIFY_PROC, show_help,
	0);
    xv_set(panel, PANEL_ITEM_X_GAP, dx, 0);
    xv_create(panel, PANEL_BUTTON,
	XV_HELP_DATA, "wave:quit",
	PANEL_LABEL_STRING, "Quit",
	PANEL_NOTIFY_PROC, quit_proc,
	0);

    if (make_sync_button)
	xv_create(panel, PANEL_BUTTON,
		  XV_HELP_DATA, "wave:sync",
		  PANEL_LABEL_STRING, "Sync",
		  PANEL_NOTIFY_PROC, sync_other_wave_processes,
		  0);

    return (panel);
}

/* This function converts a noise mnemonic string into a noise subtype.  It
   returns -2 if the input string is not a noise mnemonic string. */
static int noise_strsub(s)
char *s;
{
    int i, imax, n = 0;

    if (('0' <= *s && *s <= '9') || strcmp(s, "-1") == 0)
	return (atoi(s));
    else if (strcmp(s, "U") == 0)
	return (-1);
    imax = (nsig <= 4) ? nsig : 4;
    if (strlen(s) != imax)
	return (-2);
    for (i = 0; i < imax; i++) {
	if (s[i] == 'c')
	    continue;		/* signal i is clean */
        else if (s[i] == 'n')
	    n |= (1 << i);	/* signal i is noisy */
	else if (s[i] == 'u')
	    n |= (0x11 << i);	/* signal i is unreadable */
	else
	    return (-2);	/* the string is not a noise mnemonic */
    }
    return (n);
}

void view_menu_proc(menu, menu_item)
Menu menu;
Menu_item menu_item;
{
    char *p = (char *)xv_get(menu_item, MENU_STRING);

    printf("View menu item: %s\n", p);
}

void prop_menu_proc(menu, menu_item)
Menu menu;
Menu_item menu_item;
{
    char *p = (char *)xv_get(menu_item, MENU_STRING);

    printf("Prop menu item: %s\n", p);
}

static int reload_signals, reload_annotations;
void reinitialize()
{
    reload_annotations = reload_signals = 1;
    disp_proc((Panel_item)NULL, (Event *)'.');
}

/* Handle a display request. */
void disp_proc(item, event)
Panel_item item;
Event *event;
{
    int etype, i;
    long cache_time, next_match(), previous_match();
    void set_frame_footer();

    /* Reset display modes if necessary. */
    set_modes();

    /* If a new record has been selected, re-initialize. */
    if (reload_signals ||
	strncmp(record, (char *)xv_get(record_item, PANEL_VALUE), RNLMAX)) {
	wfdbquit();

	/* Reclaim memory previously allocated for baseline labels, if any. */
	for (i = 0; i < nsig; i++)
	    if (blabel[i]) {
		free(blabel[i]);
		blabel[i] = NULL;
	    }

	if (!record_init((char *)xv_get(record_item, PANEL_VALUE))) return;
	annotator[0] = '\0';	/* force re-initialization of annotator if
				   record was changed */
	savebackup = 1;
    }

    /* If a new annotator has been selected, re-initialize. */
    if (reload_annotations ||
	strncmp(annotator, (char *)xv_get(annot_item,PANEL_VALUE),ANLMAX)) {
	strncpy(annotator, (char *)xv_get(annot_item, PANEL_VALUE), ANLMAX);
	if (annotator[0]) {
	    af.name = annotator; af.stat = WFDB_READ;
	    nann = 1;
	}
	else
	    nann = 0;
	annot_init();
	savebackup = 1;
    }

    reload_signals = reload_annotations = 0;

    /* Get the event type.  This procedure is usually invoked with a Panel_item
       in item in response to a panel event, but it can also be called
       directly (with item = NULL and event = etype), as from window_event_proc
       in edit.c. */
    if (item) etype = (int)xv_get(item, PANEL_CLIENT_DATA);
    else etype = (int)event;

    /* Find out which button was pushed, and act on it. */
    switch (etype) {
      default:
      case '.':	/* Start at time specified on panel. */
      case '*':		/* (from scope_proc(), see scope.c */
      case '!':		/* (from show_next_entry(), see logpan.c */
	display_start_time = wstrtim((char *)xv_get(time_item, PANEL_VALUE));
	if (display_start_time < 0L) display_start_time = -display_start_time;
	cache_time = -1L;
	break;
      case '^':	/* Start at display_start_time. */
	cache_time = -1L;
	break;
      case ':':	/* End at time specified on panel. */
	display_start_time = wstrtim((char *)xv_get(time2_item, PANEL_VALUE));
	if (display_start_time < 0L) display_start_time = -display_start_time;
	if ((display_start_time -= nsamp) < 0L) display_start_time = 0L;
	cache_time = -1L;
	break;
      case 'h':	/* Go to the beginning of the record (see edit.c) */
	display_start_time = 0L;
	cache_time = -1L;
	break;
      case 'e':	/* Go to the end of the record (see edit.c) */
	if ((display_start_time = strtim("e") - nsamp) < 0L)
	    display_start_time = 0L;
	cache_time = -1L;
	break;
      case '}': /* Find next occurrence of specified signal. */
	if (1) {
	    char *fp = (char *)xv_get(findsig_item, PANEL_VALUE);

	    if ((i = findsig(fp)) >= 0) {
		WFDB_Time tnext = tnextvec(i, display_start_time + nsamp);

		if (tnext >= 0L) {
		    display_start_time = tnext;
		    cache_time = -1L;
		    break;
		}
		else {
#ifdef NOTICE
		    Xv_notice notice = xv_create((Frame)frame, NOTICE,
					         XV_SHOW, TRUE,
#else
		    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			                NOTICE_MESSAGE_STRINGS,
			                "No match found!", 0,
			                NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
		    xv_destroy_safe(notice);
#endif
		    break;
		}
	    }
	}
        break;
      case ']':	/* Find next occurrence of specified annotation. */
      case '[':	/* Find previous occurrence of specified annotation. */
	if (annotator[0]) {
	    char *fp = (char *)xv_get(find_item, PANEL_VALUE);
	    static char auxstr[256];
	    int mask, noise_mask, target;
	    long t;
	    struct WFDB_ann template;

	    if (*fp == '\0') {
		/* if find_item is empty, use the search template */
		template = search_template;
		mask = search_mask;
		if (template.aux == NULL) mask &= ~M_AUX;
	    }
	    else if (target = strann(fp)) {
		template.anntyp = target;
		mask = M_ANNTYP;
	    }
	    else if ((noise_mask = noise_strsub(fp)) >= -1) {
		template.anntyp = NOISE;
		template.subtyp = noise_mask;
		mask = M_ANNTYP | M_SUBTYP;
	    }
	    else if (strcmp(fp, ".") == 0) {
		template.anntyp = NOTQRS;
		mask = M_ANNTYP;
	    }
	    else if (strcmp(fp, ":") == 0) {
		template.anntyp = INDEX_MARK;
		mask = M_ANNTYP;
	    }
	    else if (strcmp(fp, "<") == 0) {
		template.anntyp = BEGIN_ANALYSIS;
		mask = M_ANNTYP;
	    }
	    else if (strcmp(fp, ">") == 0) {
		template.anntyp = END_ANALYSIS;
		mask = M_ANNTYP;
	    }
	    else if (strcmp(fp, "*n") == 0) {
		template.anntyp = NORMAL;
		mask = M_MAP2;
	    }
	    else if (strcmp(fp, "*s") == 0) {
		template.anntyp = SVPB;
		mask = M_MAP2;
	    }
	    else if (strcmp(fp, "*v") == 0) {
		template.anntyp = PVC;
		mask = M_MAP2;
	    }
	    else if (strcmp(fp, "*") == 0 || *fp == '\0')
		mask = 0;	/* match annotation of any type */
	    else {
		strncpy(auxstr+1, fp, 255);
		auxstr[0] = strlen(auxstr+1);
		template.aux = auxstr;
		mask = M_AUX;
	    }
	    if ((etype == (int)']' && (t = next_match(&template, mask)) < 0L)||
		(etype == (int)'[' && (t=previous_match(&template,mask)) <0L)){
#ifdef NOTICE
		Xv_notice notice = xv_create((Frame)frame, NOTICE,
					     XV_SHOW, TRUE,
#else
		(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			     NOTICE_MESSAGE_STRINGS,
			     "No match found!", 0,
			     NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
		xv_destroy_safe(notice);
#endif
	    }
	    else {
		display_start_time = strtim(timstr(t-(long)((nsamp-freq)/2)));
	        if (etype == (int)']') t = next_match(&template, mask);
		else t = previous_match(&template, mask);
		if (t > 0) cache_time=strtim(timstr(t-(long)((nsamp-freq)/2)));
		else cache_time = -1L;
	    }
	}
	break;
      case '<':	/* Go backwards one frame. */
	if ((display_start_time -= nsamp) < 0) display_start_time = 0;
	cache_time = display_start_time - nsamp;
	break;
      case '(':	/* Go backwards one-half frame. */
	if ((display_start_time -= nsamp/2) < 0) display_start_time = 0;
	cache_time = display_start_time - nsamp/2;
	break;
      case ')':	/* Go forwards one-half frame. */
	display_start_time += nsamp/2;
	cache_time = display_start_time + nsamp/2;
	break;
      case '>':	/* Go forwards one frame. */
	display_start_time += nsamp;
	cache_time = display_start_time + nsamp;
	break;
    }

    if (etype != '!' && description[0]) {
	description[0] = '\0';
	set_frame_title();
    }

    /* Erase the drawing region, and a little more.  The width of the canvas
       can be slightly greater than canvas_width (because of the adjustments
       performed by do_resize() in xvwave.h);  it is possible for text such
       as annotations to be written in this region to the right of the drawing
       area, and we want to be sure to erase it if this has happened. */
    bar(0,0,0);
    box(0,0,0);
    XFillRectangle(display, osb, clear_all,
		   0, 0, canvas_width+mmx(10), canvas_height);

    /* Reset the pointer for the scope display unless the scope is running or
       has just been paused. */
    if (!scan_active && etype != '*') {
	(void)locate_annotation(display_start_time, -128);
	scope_annp = annp;
    }

    /* Display the selected data. */
    do_disp();
    set_frame_footer();

    /* Read ahead if possible. */
    if (cache_time >= 0) (void)find_display_list(cache_time);
}

void set_record_item(s)
char *s;
{
    xv_set(record_item, PANEL_VALUE, s, NULL);
}

void set_annot_item(s)
char *s;
{
    xv_set(annot_item, PANEL_VALUE, s, NULL);
}

void set_start_time(s)
char *s;
{
    xv_set(time_item, PANEL_VALUE, s, NULL);
}

void set_end_time(s)
char *s;
{
    xv_set(time2_item, PANEL_VALUE, s, NULL);
}

void set_find_item(s)
char *s;
{
    xv_set(find_item, PANEL_VALUE, s, NULL);
    xv_set(findsig_item, PANEL_VALUE, s, NULL);
}
