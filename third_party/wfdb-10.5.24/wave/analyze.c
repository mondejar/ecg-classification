/* file: analyze.c	G. Moody	10 August 1990
			Last revised:  26 November 2001
Functions for the analysis panel of WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 2001 George B. Moody

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
#include <stdio.h>
#include <xview/notice.h>
#include <xview/termsw.h>
#include <xview/notify.h>
#include <xview/defaults.h>

#ifndef MENUDIR
#define MENUDIR		"/usr/local/lib"
#endif

#define MENUFILE	"wavemenu.def"	/* name of the default menu file (to
					   be found in MENUDIR) */
#define MAXLL		1024		/* maximum length of a line in the menu
					   file including continuation lines */

void do_analysis(), edit_menu_file(), set_signal(), set_start(), set_stop(),
    set_back(), set_ahead(), set_siglist(), set_siglist_from_string(),
    show_command_window(), add_signal_choice(), delete_signal_choice();
char *wavemenu;
Frame analyze_frame, tty_frame;
Panel analyze_panel;
Panel_item start_item, astart_item, dstart_item, end_item, aend_item,
           dend_item, signal_item, signal_name_item, siglist_item;
Termsw tty;

int analyze_popup_active = -1;

struct MenuEntry {
    char *label;
    char *command;
    struct MenuEntry *nme;
} menu_head, *mep;

static char *menudir;

char *print_command = NULL;
int menu_read = 0;

void print_proc()
{
    char default_print_command[256];
    void read_menu();

    if (menu_read == 0)
	read_menu();
    if (print_command == NULL) {
	sprintf(default_print_command, "echo $RECORD $LEFT-$RIGHT |\
 pschart -a $ANNOTATOR -g -l -L -n 0 -R -t 20 -v 8 - | %s\n", psprint);
	print_command = default_print_command;
    }
    do_command(print_command);
}

char *open_url_command = NULL;

void open_url()
{
    char default_open_url_command[256];
    void read_menu();

    if (menu_read == 0)
	read_menu();
    if (open_url_command == NULL) {
	sprintf(default_open_url_command, "url_view $URL\n");
	open_url_command = default_open_url_command;
    }
    do_command(open_url_command);
}

void read_menu()
{
    char linebuf[MAXLL+1], *p, *p2, *getenv(), *strtok();
    int l;
    FILE *ifile = NULL;

    /* Record that an attempt has been made to read the menu. */
    menu_read = 1;

    /* Clear the menu entry list. */
    mep = &menu_head;
    mep->label = mep->command = (char *)NULL;
    mep->nme = (struct MenuEntry *)NULL;

    /* Try to open the user's menu file, if one is specified. */
    if ((wavemenu != NULL || (wavemenu = getenv("WAVEMENU"))) &&
	(ifile = fopen(wavemenu, "r")) == NULL) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	notice_prompt((Frame)frame, (Event *)NULL,
#endif
			  NOTICE_MESSAGE_STRINGS,
			   "Can't read menu file:",
			   wavemenu, 0,
			  NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
    }

    /* If no user menu was specified, try to open "wavemenu" in the current
       directory. */
    if (ifile == NULL && (ifile = fopen("wavemenu", "r")) != NULL)
	wavemenu = "wavemenu";

    /* If the user's file wasn't specified or can't be read, and "wavemenu"
       can't be read in the current directory, try to open the default menu
       file. */
    if (ifile == NULL) {
	if ((menudir = getenv("MENUDIR")) == NULL) menudir = MENUDIR;
	if (wavemenu = malloc(strlen(menudir)+strlen(MENUFILE)+2)) {
	    sprintf(wavemenu, "%s/%s", menudir, MENUFILE);
	    if ((ifile = fopen(wavemenu, "r")) == NULL) {
#ifdef NOTICE
	    Xv_notice notice = xv_create((Frame)frame, NOTICE,
					 XV_SHOW, TRUE,
#else
	    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			      NOTICE_MESSAGE_STRINGS,
			      "Can't read default menu file:",
			      wavemenu, 0,
			      NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
		xv_destroy_safe(notice);
#endif
	    }
	    free(wavemenu);
	    wavemenu = NULL;
	}
    }

    /* Give up if no menu file can be opened.  In this case, commands may still
       be typed into the tty subwindow directly. */
    if (ifile == NULL) return;

    /* Read the menu file. */
    while (fgets(linebuf, MAXLL+1, ifile)) {	/* read the next line */
	while ((l = strlen(linebuf)) > 1 && l < MAXLL && linebuf[l-2]=='\\' &&
	       fgets(&linebuf[l-2], MAXLL+1 - (l-2), ifile))
	    /* append continuation line(s), if any */
	    if (linebuf[l-2] == '\t' || linebuf[l-2] == ' ') {
		/* discard initial whitespace in continuation lines, if any */
		char *p, *q;

		for (p = q = linebuf+l-2; *p == '\t' || *p == ' '; p++)
		    ;
		while (*p)
		    *q++ = *p++;
		*q = '\0';
	    }

	/* Find the first non-whitespace character (the beginning of the
	   button label). */
	for (p = linebuf; *p; p++) {
	    if (*p == '#') { *p = '\0'; break; }
	    else if (*p != ' ' && *p != '\t') break;
	}

	/* Skip comments and empty lines. */
	if (*p == '\0') continue;

	/* Find the first embedded tab (the end of the button label). */
	for (p2 = p; *p2 && *p2 != '\t'; p2++)
	    ;

	/* Skip lines without an embedded tab. */
	if (*p2 == '\0') continue;

	*p2++ = '\0';	/* Replace the embedded tab with a null. */

	/* Find the next non-whitespace character (the beginning of the
	   command). */
	for ( ; *p2 == '\t' || *p2 == ' '; p2++)
	    ;

	/* Skip lines without a command. */
	if (*p2 == '\n') continue;

	/* Test for special <Print> command definition. */
	if (strcmp(p, "<Print>") == 0) {
	    if (p = (char *)malloc((unsigned)(strlen(p2)+1))) {
		strcpy(p, p2);
		print_command = p;
	    }
	    continue;
	}

	/* Test for special <Open URL> command definition. */
	if (strcmp(p, "<Open URL>") == 0) {
	    if (p = (char *)malloc((unsigned)(strlen(p2)+1))) {
		strcpy(p, p2);
		open_url_command = p;
	    }
	    continue;
	}

	/* Allocate storage for the menu entry, button label, and command. */
	if (!(mep->nme=(struct MenuEntry *)malloc(sizeof(struct MenuEntry))) ||
	    !(mep->nme->label = (char *)malloc((unsigned)(strlen(p)+1))) ||
	    !(mep->nme->command = (char *)malloc((unsigned)(strlen(p2)+1)))) {
#ifdef NOTICE
	    Xv_notice notice = xv_create((Frame)frame, NOTICE,
					 XV_SHOW, TRUE,
#else
	    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			  NOTICE_MESSAGE_STRINGS,
			  "Out of memory while reading menu file",
			  NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	    xv_destroy_safe(notice);
#endif
	    mep->nme = NULL;
	    break;
	}
	strcpy(mep->nme->label, p);
	strcpy(mep->nme->command, p2);
	mep = mep->nme;
	mep->nme = NULL;
    }
    fclose(ifile);
}

int xaf = -1, yaf = -1;

/* Set up analyze menu and terminal emulator windows. */
void create_analyze_popup()
{
    int i;
    Icon menu_icon, tty_icon;
    void recreate_analyze_popup(), reload();

    if (menu_read == 0)
	read_menu();

    analyze_popup_active = 0;
    menu_icon = xv_create(XV_NULL, ICON,
			  ICON_IMAGE, icon_image,
			  ICON_LABEL, "Analyze",
			  NULL);
    analyze_frame = xv_create(frame, FRAME,
			      XV_LABEL, "Analyze",
			      FRAME_ICON, menu_icon, 0);
/*
    if (xaf >= 0 || yaf >= 0)
	xv_set(analyze_frame, XV_X, xaf, XV_Y, yaf);
*/
    analyze_panel = xv_create(analyze_frame, PANEL,
			      XV_WIDTH, mmx(225),
			      0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "<",
	      XV_HELP_DATA, "wave:file.analyze.<",
	      PANEL_NOTIFY_PROC, set_back, 0);
    start_item = xv_create(analyze_panel, PANEL_TEXT,
			   PANEL_LABEL_STRING, "Start (elapsed): ",
			   XV_HELP_DATA, "wave:file.analyze.start",
			   PANEL_VALUE_DISPLAY_LENGTH, 13,
			   PANEL_CLIENT_DATA, (caddr_t)'e',
			   PANEL_NOTIFY_PROC, set_start, 0);
    end_item = xv_create(analyze_panel, PANEL_TEXT,
			 PANEL_LABEL_STRING, "End (elapsed): ",
			 XV_HELP_DATA, "wave:file.analyze.end",
			 PANEL_VALUE_DISPLAY_LENGTH, 13,
			 PANEL_CLIENT_DATA, (caddr_t)'e',
			 PANEL_NOTIFY_PROC, set_stop, 0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, ">",
	      XV_HELP_DATA, "wave:file.analyze.<",
	      PANEL_NOTIFY_PROC, set_ahead, 0);
    signal_item = xv_create(analyze_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "Signal: ",
			    XV_HELP_DATA, "wave:file.analyze.signal",
			    PANEL_VALUE_DISPLAY_LENGTH, 3,
			    PANEL_VALUE, signal_choice,
			    PANEL_MIN_VALUE, 0,
			    PANEL_MAX_VALUE, nsig > 0 ? nsig-1 : 0,
			    PANEL_INACTIVE, nsig > 0 ? FALSE : TRUE,
			    PANEL_NOTIFY_PROC, set_signal, 0);
    signal_name_item = xv_create(analyze_panel, PANEL_MESSAGE,
				 PANEL_LABEL_STRING, "xxxxxxxxxxxxxx", 0);

    astart_item= xv_create(analyze_panel, PANEL_TEXT,
			   PANEL_NEXT_ROW, -1, 
			   PANEL_LABEL_STRING, "From: ",
			   XV_HELP_DATA, "wave:file.analyze.astart",
			   PANEL_VALUE_DISPLAY_LENGTH, 13,
			   PANEL_CLIENT_DATA, (caddr_t)'a',
			   PANEL_NOTIFY_PROC, set_start, 0);
    dstart_item= xv_create(analyze_panel, PANEL_TEXT,
			   XV_HELP_DATA, "wave:file.analyze.dstart",
			   PANEL_VALUE_DISPLAY_LENGTH, 11,
			   PANEL_CLIENT_DATA, (caddr_t)'d',
			   PANEL_NOTIFY_PROC, set_start, 0);
    reset_start();
    aend_item= xv_create(analyze_panel, PANEL_TEXT,
			 PANEL_LABEL_STRING, "To: ",
			 XV_HELP_DATA, "wave:file.analyze.aend",
			 PANEL_VALUE_DISPLAY_LENGTH, 13,
			 PANEL_CLIENT_DATA, (caddr_t)'a',
			 PANEL_NOTIFY_PROC, set_stop, 0);
    dend_item= xv_create(analyze_panel, PANEL_TEXT,
			 XV_HELP_DATA, "wave:file.analyze.dend",
			 PANEL_VALUE_DISPLAY_LENGTH, 13,
			 PANEL_CLIENT_DATA, (caddr_t)'d',
			 PANEL_NOTIFY_PROC, set_stop, 0);
    reset_stop();
    siglist_item = xv_create(analyze_panel, PANEL_TEXT,
			     PANEL_LABEL_STRING, "Signal list: ",
			     XV_HELP_DATA, "wave:file.analyze.signal_list",
			     PANEL_VALUE_DISPLAY_LENGTH, 15,
			     PANEL_VALUE_STORED_LENGTH, 1024,
			     PANEL_INACTIVE, nsig > 0 ? FALSE : TRUE,
			     PANEL_NOTIFY_PROC, set_siglist, 0);
    reset_siglist();
	      

    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_NEXT_ROW, -1, 
	      PANEL_LABEL_STRING, "Show scope window",
	      XV_HELP_DATA, "wave:file.analyze.show_scope_window",
	      PANEL_NOTIFY_PROC, show_scope_window, 0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Show command window",
	      XV_HELP_DATA, "wave:file.analyze.show_command_window",
	      PANEL_NOTIFY_PROC, show_command_window, 0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Edit menu",
	      XV_HELP_DATA, "wave:file.analyze.edit_menu",
	      PANEL_NOTIFY_PROC, edit_menu_file, 0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Reread menu",
	      XV_HELP_DATA, "wave:file.analyze.reread_menu",
	      PANEL_NOTIFY_PROC, recreate_analyze_popup, 0);
    xv_create(analyze_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Reload",
	      XV_HELP_DATA, "wave:file.analyze.reload",
	      PANEL_NOTIFY_PROC, reload, 0);

    for (i = 0, mep = &menu_head; mep->nme != NULL; mep = mep->nme, i++) {
	if (i == 0)
	    xv_create(analyze_panel, PANEL_BUTTON,
		  PANEL_NEXT_ROW, -1, 
		  PANEL_LABEL_STRING, mep->nme->label,
		  XV_HELP_DATA, "wave:file.analyze.analysis_button",
		  PANEL_NOTIFY_PROC, do_analysis,
		  PANEL_CLIENT_DATA, (caddr_t) i, 0);
	else
	    xv_create(analyze_panel, PANEL_BUTTON,
		  PANEL_LABEL_STRING, mep->nme->label,
		  XV_HELP_DATA, "wave:file.analyze.analysis_button",
		  PANEL_NOTIFY_PROC, do_analysis,
		  PANEL_CLIENT_DATA, (caddr_t) i, 0);
    }

    window_fit(analyze_panel);
    window_fit(analyze_frame);

    /* It appears to be necessary to create the signal name item with a
       dummy value as above to reserve space for it in case
       signame[signal_choice] is shorter than some other current or future
       entry in signame[].  Here we reset the value to the correct string. */
    xv_set(signal_name_item, PANEL_LABEL_STRING, signame[signal_choice], 0);

    /* The current version of XView allows us to create a Termsw at most once
       per process, so we have to be careful not to attempt to execute the
       following code more than once. */
    if (tty == XV_NULL) {
	int x, y;

	x = (int)xv_get(analyze_frame, XV_X);
	y = (int)xv_get(analyze_frame, XV_Y) +
	    (int)xv_get(analyze_frame, XV_HEIGHT) + 75;
	tty_icon = xv_create(XV_NULL, ICON,
			     ICON_IMAGE, icon_image,
			     ICON_LABEL, "Commands",
			     NULL);
	tty_frame = xv_create(frame, FRAME,
			      XV_LABEL, "Analysis commands",
			      XV_X, x, XV_Y, y,
			      FRAME_ICON, tty_icon, 0);
	tty = (Termsw)xv_create(tty_frame, TERMSW,
/*			 XV_X, mmx(1), */
			 WIN_ROWS, 10, 0);
	window_fit(tty);
	window_fit(tty_frame);
    }
}

/* Recreate analyze popup (if reread menu button was selected). */
void recreate_analyze_popup()
{
    if (analyze_frame) {
	xaf = (int)xv_get(analyze_frame, XV_X);
	yaf = (int)xv_get(analyze_frame, XV_Y);
	if (xv_destroy_safe(analyze_frame) == XV_OK) {
	    struct MenuEntry *tmep;

	    mep = menu_head.nme;
	    while (mep != NULL) {
		tmep = mep->nme;
		free(mep->command);
		free(mep->label);
	        free(mep);
		mep = tmep;
	    }
	    menu_read = 0;
	    create_analyze_popup();
	    xv_set(analyze_frame, WIN_MAP, TRUE, 0);
	}
    }
}

/* Make the analysis menu window appear. */
void analyze_proc()
{
    if (analyze_popup_active < 0) create_analyze_popup();
    wmgr_top(tty_frame);
    xv_set(tty_frame, WIN_MAP, TRUE, 0);
    wmgr_top(analyze_frame);
    xv_set(analyze_frame, WIN_MAP, TRUE, 0);
    analyze_popup_active = 1;
}

/* Edit the menu file. */
void edit_menu_file()
{
    char *edit_command, *editor, *menu_filename, *getenv();
    int clen, elen, result;

    if ((editor = getenv("EDITOR")) == NULL)
	editor = defaults_get_string("wave.texteditor",
				     "Wave.TextEditor",
				     EDITOR);
    elen = strlen(editor);
    if (wavemenu == NULL) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
				     NOTICE_STATUS, &result,
#else
	result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
		      NOTICE_MESSAGE_STRINGS,
		      "You are now using the system default menu file,",
		      "which you may not edit directly.",
		      "Press `Copy' to copy it into the current directory",
		      "as `wavemenu' (and remember to set the WAVEMENU",
		      "environment variable next time),",
		      "- or -",
		      "Press `Quit' if you prefer not to edit a menu file.",
		      0,
		      NOTICE_BUTTON_YES, "Copy",
		      NOTICE_BUTTON_NO, "Quit", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	if (result != NOTICE_YES)
	    return;
	
	clen = strlen(menudir) + strlen(MENUFILE) + 4; /* "cp ... " */
	if (clen < elen) clen = elen;
	if (edit_command = malloc(clen + 10)) {  /* "wavemenu\n" + null */
	    sprintf(edit_command, "cp %s/%s wavemenu\n", menudir, MENUFILE);
	    do_command(edit_command);
	    wavemenu = "wavemenu";
	}
    }
    else
	edit_command = malloc(elen + strlen(wavemenu) + 3);
    if (edit_command) {
	sprintf(edit_command, "%s %s\n", editor, wavemenu);
	show_command_window();
	do_command(edit_command);
	free(edit_command);
    }
}
    
/* Make the analysis command window appear. */
void show_command_window()
{
    wmgr_top(tty_frame);
    xv_set(tty_frame, WIN_MAP, TRUE, 0);
}

/* Set variables needed for analysis routines. */
void set_signal()
{
    signal_choice = (int)xv_get(signal_item, PANEL_VALUE);
    xv_set(signal_name_item, PANEL_LABEL_STRING, signame[signal_choice], 0);
    sig_highlight(signal_choice);
}

void set_signal_choice(i)
int i;
{
    int j;

    if (sig_mode == 0)
	j = i;
    else if (0 <= i && i < siglistlen)
	j = siglist[i];
    if (0 <= j && j < nsig) {
	signal_choice = j;
	if (analyze_popup_active >= 0) {
	    xv_set(signal_item, PANEL_VALUE, signal_choice, 0);
	    xv_set(signal_name_item, PANEL_LABEL_STRING,
		   signame[signal_choice], 0);
	}
	sig_highlight(signal_choice);
    }
}

void set_siglist()
{
    set_siglist_from_string((char *)xv_get(siglist_item, PANEL_VALUE));
}

void set_siglist_from_string(s)
char *s;
{
    char *p;

    /* Count the number of signals named in the string (s). */
    for (p = s, siglistlen = 0; *p; ) {
	while (*p && (*p == ' ' || *p == '\t'))
	    p++;
	if (*p) siglistlen++;
	while (*p && (*p != ' ' && *p != '\t'))
	    p++;
    }
    /* Allocate storage for siglist. */
    if (siglistlen > maxsiglistlen) {
	siglist = realloc(siglist, siglistlen * sizeof(int));
	base = realloc(base, siglistlen * sizeof(int));
	level = realloc(level, siglistlen * sizeof(XSegment));
	maxsiglistlen = siglistlen;
    }
    /* Now store the signal numbers in siglist. */
    for (p = s, siglistlen = 0; *p && siglistlen < maxsiglistlen; ) {
	while (*p && (*p == ' ' || *p == '\t'))
	    p++;
	if (*p) siglist[siglistlen++] = atoi(p);
	while (*p && (*p != ' ' && *p != '\t'))
	    p++;
    }
    reset_siglist();
}

void set_start(item, event)
Panel_item item;
Event *event;
{
    struct ap *a;
    char *start_string, *astart_string, *dstart_string, *p, buf[30];
    int i;
    WFDB_Time t;

    i = (int)xv_get(item, PANEL_CLIENT_DATA);
    if (a = get_ap()) {
	int redraw;

	redraw = (display_start_time <= begin_analysis_time &&
		  begin_analysis_time < display_start_time + nsamp);
	switch (i) {
	  case 'e':
	    start_string = (char *)xv_get(start_item, PANEL_VALUE);
	    t = strtim(start_string);
	    p = mstimstr(-t);
	    if (*p == '[') {
		*(p+13) = *(p+24) =  '\0';
		xv_set(astart_item, PANEL_VALUE, p+1,
		       PANEL_INACTIVE, FALSE, 0);
		xv_set(dstart_item, PANEL_VALUE, p+14,
		       PANEL_INACTIVE, FALSE, 0);
	    }
	    else {
		xv_set(astart_item, PANEL_VALUE, "",
		       PANEL_INACTIVE, TRUE, 0);
		xv_set(dstart_item, PANEL_VALUE, "",
		       PANEL_INACTIVE, TRUE, 0);
	    }
	    break;
	  case 'a':
	    astart_string = (char *)xv_get(astart_item, PANEL_VALUE);
	    dstart_string = (char *)xv_get(dstart_item, PANEL_VALUE);
	    sprintf(buf, "[%s %s]", astart_string, dstart_string);
	    if ((t = -strtim(buf)) <= 0L) {
		/* tried to set a time before the beginning of the record */
		t = 0L;	/* go to the beginning instead */
		xv_set(start_item, PANEL_VALUE, "beginning", 0);
	    }
	    else
		xv_set(start_item, PANEL_VALUE, mstimstr(t), 0);
	    set_start(start_item, (Event *)NULL);
	    break;
	  case 'd':
	    dstart_string = (char *)xv_get(dstart_item, PANEL_VALUE);
	    /* changed the date -- try to go to midnight on that date */
	    sprintf(buf, "[0:0:0 %s]", dstart_string);
	    if ((t = -strtim(buf)) <= 0L) {
		/* tried to set a time before the beginning of the record */
		t = 0L;	/* go to the beginning instead */
		xv_set(start_item, PANEL_VALUE, "beginning", 0);
	    }
	    else
		xv_set(start_item, PANEL_VALUE, mstimstr(t), 0);
	    set_start(start_item, (Event *)NULL);
	    break;
	}

	a->this.anntyp = BEGIN_ANALYSIS;
	a->this.subtyp = a->this.num = 0;
	a->this.chan = 127;
	a->this.aux = NULL;
	a->this.time = t;
	insert_annotation(a);
	if (redraw || (display_start_time <= begin_analysis_time &&
		  begin_analysis_time < display_start_time + nsamp)) {
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	}
    }
}

void set_stop(item, event)
Panel_item item;
Event *event;
{
    struct ap *a;
    char *end_string, *aend_string, *dend_string, *p, buf[30];
    int i;
    WFDB_Time t;

    i = (int)xv_get(item, PANEL_CLIENT_DATA);
    if (a = get_ap()) {
	int redraw;

	redraw = (display_start_time <= end_analysis_time &&
		  end_analysis_time < display_start_time + nsamp);
	switch (i) {
	  case 'e':
	    end_string = (char *)xv_get(end_item, PANEL_VALUE);
	    t = strtim(end_string);
	    p = mstimstr(-t);
	    if (*p == '[') {
		*(p+13) = *(p+24) =  '\0';
		xv_set(aend_item, PANEL_VALUE, p+1,
		       PANEL_INACTIVE, FALSE, 0);
		xv_set(dend_item, PANEL_VALUE, p+14,
		       PANEL_INACTIVE, FALSE, 0);
	    }
	    else {
		xv_set(aend_item, PANEL_VALUE, "",
		       PANEL_INACTIVE, TRUE, 0);
		xv_set(dend_item, PANEL_VALUE, "",
		       PANEL_INACTIVE, TRUE, 0);
	    }
	    break;
	  case 'a':
	    aend_string = (char *)xv_get(aend_item, PANEL_VALUE);
	    dend_string = (char *)xv_get(dend_item, PANEL_VALUE);
	    sprintf(buf, "[%s %s]", aend_string, dend_string);
	    if ((t = -strtim(buf)) <= 0L) {
		/* tried to set a time before the beginning of the record */
		t = 0L;	/* go to the beginning instead */
		xv_set(end_item, PANEL_VALUE, "beginning", 0);
	    }
	    else
		xv_set(end_item, PANEL_VALUE, mstimstr(t), 0);
	    set_stop(end_item, (Event *)NULL);
	    break;
	  case 'd':
	    dend_string = (char *)xv_get(dend_item, PANEL_VALUE);
	    /* changed the date -- try to go to midnight on that date */
	    sprintf(buf, "[0:0:0 %s]", dend_string);
	    if ((t = -strtim(buf)) <= 0L) {
		/* tried to set a time before the beginning of the record */
		t = 0L;	/* go to the beginning instead */
		xv_set(end_item, PANEL_VALUE, "beginning", 0);
	    }
	    else
		xv_set(end_item, PANEL_VALUE, mstimstr(t), 0);
	    set_stop(end_item, (Event *)NULL);
	    break;
	}

	a->this.anntyp = END_ANALYSIS;
	a->this.subtyp = a->this.num = 0;
	a->this.chan = 127;
	a->this.aux = NULL;
	a->this.time = t;
	insert_annotation(a);
	if (redraw || (display_start_time <= end_analysis_time &&
		  end_analysis_time < display_start_time + nsamp)) {
	    clear_annotation_display();
	    show_annotations(display_start_time, nsamp);
	}
    }
}

void set_back()
{
    long step = end_analysis_time - begin_analysis_time, t0, t1;

    if (begin_analysis_time <= 0L || step <= 0L) return;
    if ((t0 = begin_analysis_time - step) < 0L) t0 = 0L;
    t1 = t0 + step;
    xv_set(start_item, PANEL_VALUE, mstimstr(t0), NULL);
    set_start(start_item, (Event *)NULL);
    xv_set(end_item, PANEL_VALUE, mstimstr(t1), NULL);
    set_stop(end_item, (Event *)NULL);
}

void set_ahead()
{
    long step = end_analysis_time - begin_analysis_time, t0, t1,
        te = strtim("e");

    if ((te > 0L && end_analysis_time >= te) || step <= 0L) return;
    t0 = begin_analysis_time + step;
    t1 = t0 + step;
    xv_set(end_item, PANEL_VALUE, mstimstr(t1), NULL);
    set_stop(end_item, (Event *)NULL);
    xv_set(start_item, PANEL_VALUE, mstimstr(t0), NULL);
    set_start(start_item, (Event *)NULL);
}


void reset_start()
{
    if (analyze_popup_active >= 0) {
	char *p;

	if (begin_analysis_time == -1L) begin_analysis_time = 0L;
	xv_set(start_item, PANEL_VALUE, begin_analysis_time == 0L ?
	         "beginning" : mstimstr(begin_analysis_time),
	       NULL);
	p = mstimstr(-begin_analysis_time);
	if (*p == '[') {
	    *(p+13) = *(p+24) =  '\0';
	    xv_set(astart_item, PANEL_VALUE, p+1, PANEL_INACTIVE, FALSE, 0);
	    xv_set(dstart_item, PANEL_VALUE, p+14, PANEL_INACTIVE, FALSE, 0);
	}
	else {
	    xv_set(astart_item, PANEL_VALUE, "", PANEL_INACTIVE, TRUE, 0);
	    xv_set(dstart_item, PANEL_VALUE, "", PANEL_INACTIVE, TRUE, 0);
	}
    }
}

void reset_stop()
{
    if (analyze_popup_active >= 0) {
	char *p;

	if (end_analysis_time == -1L) end_analysis_time = strtim("e");
	xv_set(end_item, PANEL_VALUE,
	       end_analysis_time == 0L ? "end" : mstimstr(end_analysis_time),
	       NULL);
	p = mstimstr(-end_analysis_time);
	if (*p == '[') {
	    *(p+13) = *(p+24) =  '\0';
	    xv_set(aend_item, PANEL_VALUE, p+1, PANEL_INACTIVE, FALSE, 0);
	    xv_set(dend_item, PANEL_VALUE, p+14, PANEL_INACTIVE, FALSE, 0);
	}
	else {
	    xv_set(aend_item, PANEL_VALUE, "", PANEL_INACTIVE, TRUE, 0);
	    xv_set(dend_item, PANEL_VALUE, "", PANEL_INACTIVE, TRUE, 0);
	}
    }
}

void reset_siglist()
{
    if (analyze_popup_active >= 0) {
        char *p;
	int i;
	static char *sigliststring;
	static int maxrssiglistlen;

	if (siglistlen > maxrssiglistlen) {
	    sigliststring = realloc(sigliststring, 8 * siglistlen);
	    maxrssiglistlen = siglistlen;
	}
	p = sigliststring;
	if (p) *p = '\0';
	for (i = 0; i < siglistlen; i++) {
	    sprintf(p, "%d ", siglist[i]);
	    p += strlen(p);
	}
	xv_set(siglist_item, PANEL_VALUE, sigliststring, NULL);
    }
    if (sig_mode)
	set_baselines();
}

void reset_maxsig()
{
    if (analyze_popup_active >= 0) {
	xv_set(signal_item, PANEL_INACTIVE, nsig > 0 ? FALSE : TRUE, 0);
	xv_set(signal_item, PANEL_MAX_VALUE, nsig > 0 ? nsig-1 : 0, 0);
	if (signal_choice >= nsig || signal_choice < 0)
	    xv_set(signal_item, PANEL_VALUE, signal_choice = 0, 0);
	xv_set(signal_name_item, PANEL_INACTIVE, nsig > 0 ? FALSE : TRUE, 0);
	xv_set(signal_name_item, PANEL_LABEL_STRING,signame[signal_choice], 0);
    }
}

/* Signal-list manipulation. */
void add_to_siglist(i)
int i;
{
    if (0 <= i && i < nsig) {
	if (++siglistlen >= maxsiglistlen) {
	    siglist = realloc(siglist, siglistlen * sizeof(int));
	    base = realloc(base, nsig * sizeof(int));
	    level = realloc(level, nsig * sizeof(XSegment));
	    maxsiglistlen = siglistlen;
	}
	siglist[siglistlen-1] = i;
    }
    reset_siglist();
}

void delete_from_siglist(i)
int i;
{
    int nsl;

    for (nsl = 0; nsl < siglistlen; nsl++) {
	if (siglist[nsl] == i) {
	    siglistlen--;
	    for ( ; nsl < siglistlen; nsl++)
		siglist[nsl] = siglist[nsl+1];
	    reset_siglist();
	}
    }
}

void add_signal_choice()
{
    add_to_siglist(signal_choice);
}

void delete_signal_choice()
{
    delete_from_siglist(signal_choice);
}

/* This function executes the command string provided as its argument, after
   substituting for WAVE's internal variables (RECORD, ANNOTATOR, etc.). */

void do_command(p1)
char *p1;
{
    char *p2, *tp;

    post_changes();	/* make sure that the annotation file is up-to-date */
    finish_log();	/* make sure that the log file is up-to-date */
    if (analyze_popup_active < 0) create_analyze_popup();
    for (p2 = p1; *p2; p2++) {
	if (*p2 == '$') {
	    ttysw_input(tty, p1, p2-p1);
	    p1 = ++p2;
	    if (strncmp(p1, "RECORD", 6) == 0) {
		ttysw_input(tty, record, strlen(record));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "ANNOTATOR", 9) == 0) {
		if (annotator[0])
		    ttysw_input(tty, annotator, strlen(annotator));
		else
		    ttysw_input(tty, "\"\"", 2);
		p1 = p2 += 9;
	    }
	    else if (strncmp(p1, "START", 5) == 0) {
		if (begin_analysis_time != -1L && begin_analysis_time != 0L) {
		    tp = mstimstr(begin_analysis_time);
		    while (*tp == ' ') tp++;
		}
		else tp = "0";
		ttysw_input(tty, tp, strlen(tp));
		p1 = p2 += 5;
	    }
	    else if (strncmp(p1, "END", 3) == 0) {
		if (end_analysis_time != -1L)
		    tp = mstimstr(end_analysis_time);
		else if (end_analysis_time == 0L)
		    tp = "0";
		else tp = mstimstr(strtim("e"));
		while (*tp == ' ') tp++;
		ttysw_input(tty, tp, strlen(tp));
		p1 = p2 += 3;
	    }
	    else if (strncmp(p1, "DURATION", 8) == 0) {
		long t0 = begin_analysis_time, t1 = end_analysis_time;

		/* If end_analysis_time is unspecified, determine the
		   record length. */
		if (t1 == -1L) t1 = strtim("e");
		/* If the record length is also unspecified, use zero
		   in place of the duration.  Programs that accept
		   duration arguments must be written to accept zero
		   as meaning "unspecified". */
		if (t1 == 0L) ttysw_input(tty, "0", 1);
		else {
		    if (t0 == -1L) t0 = 0L;
		    tp = mstimstr(t1-t0);
		    while (*tp == ' ') tp++;
		    ttysw_input(tty, tp, strlen(tp));
		}
		p1 = p2 += 8;
	    }		    
	    else if (strncmp(p1, "SIGNALS", 7) == 0) {
		char s[4];
		int nsl;

		for (nsl = 0; nsl < siglistlen; nsl++) {
		    sprintf(s, "%d%s", siglist[nsl],
			    nsl < siglistlen ? " " : "");
		    ttysw_input(tty, s, strlen(s));
		}
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "SIGNAL", 6) == 0) {
		char s[3];

		sprintf(s, "%d", signal_choice);
		ttysw_input(tty, s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "LEFT", 4) == 0) {
		if (display_start_time < 1L) tp = "0";
		else {
		    tp = mstimstr(display_start_time);
		    while (*tp == ' ') tp++;
		}
		ttysw_input(tty, tp, strlen(tp));
		p1 = p2 += 4;
	    }
	    else if (strncmp(p1, "RIGHT", 5) == 0) {
		tp = mstimstr(display_start_time + nsamp);
		while (*tp == ' ') tp++;
		ttysw_input(tty, tp, strlen(tp));
		p1 = p2 += 5;
	    }
	    else if (strncmp(p1, "WIDTH", 5) == 0) {
		tp = mstimstr(nsamp);
		while (*tp == ' ') tp++;
		ttysw_input(tty, tp, strlen(tp));
		p1 = p2 += 5;
	    }		    
	    else if (strncmp(p1, "LOG", 3) == 0) {
		if (*log_file_name == '0')
		    sprintf(log_file_name, "log.%s", record);
		ttysw_input(tty, log_file_name, strlen(log_file_name));
		p1 = p2 += 3;
	    }
	    else if (strncmp(p1, "WFDBCAL", 7) == 0) {
		ttysw_input(tty, cfname, strlen(cfname));
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "WFDB", 4) == 0) {
		ttysw_input(tty, getwfdb(), strlen(getwfdb()));
		p1 = p2 += 4;
	    }
	    else if (strncmp(p1, "TSCALE", 6) == 0) {
		char s[10];

		sprintf(s, "%g", mmpersec);
		ttysw_input(tty, s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "VSCALE", 6) == 0) {
		char s[10];

		sprintf(s, "%g", mmpermv);
		ttysw_input(tty, s, strlen(s));
		p1 = p2 += 6;
	    }
	    else if (strncmp(p1, "DISPMODE", 8) == 0) {
		char s[3];

		sprintf(s, "%d", (ann_mode << 1) | show_marker);
		ttysw_input(tty, s, strlen(s));
		p1 = p2 += 8;
	    }
	    else if (strncmp(p1, "PSPRINT", 7) == 0) {
		ttysw_input(tty, psprint, strlen(psprint));
		p1 = p2 += 7;
	    }
	    else if (strncmp(p1, "TEXTPRINT", 7) == 0) {
		ttysw_input(tty, textprint, strlen(textprint));
		p1 = p2 += 9;
	    }
	    else if (strncmp(p1, "URL", 3) == 0) {
		ttysw_input(tty, url, strlen(url));
		p1 = p2 += 3;
	    }
	    else
		p1--;
	}
    }
    ttysw_input(tty, p1, p2-p1);
}

void do_analysis(item, event)
Panel_item item;
Event *event;
{
    char *p1, *p2, *tp;
    int i, j;

    i = (int)xv_get(item, PANEL_CLIENT_DATA);
    for (j=0, mep = &menu_head; j<i && mep->nme != NULL; j++, mep = mep->nme)
	;
    if (j == i && mep->nme != NULL && mep->nme->command != NULL)
	do_command(mep->nme->command);
}

static char fname[20];

/* This function gets called once per second while the timer is running.
   Once the temporary file named by fname contains readable data, it
   waits one more second, turns off the timer, and then deletes the file. */

Notify_value check_if_done()
{
    FILE *tfile;
    static int file_ready;
    void reinitialize();

    if (file_ready) {
	notify_set_itimer_func(analyze_frame, NOTIFY_FUNC_NULL, ITIMER_REAL,
			       NULL, NULL);
	unlink(fname);
	file_ready = 0;
	reinitialize();
	set_start(start_item, (Event *)NULL);
	set_stop(end_item, (Event *)NULL);
	return (NOTIFY_DONE);
    }
    if ((tfile = fopen(fname, "r")) == NULL)
	return (NOTIFY_DONE);
    fclose(tfile);
    file_ready++;
    return (NOTIFY_DONE);
}

void reload()
{
  static char command[80];
    static struct itimerval timer;

    if (fname[0] == '\0') {
	sprintf(fname, "/tmp/wave.XXXXXX");
	mkstemp(fname);
	sprintf(command, "touch %s\n", fname);
    }
    do_command(command);
    timer.it_value.tv_sec = timer.it_interval.tv_sec = 1;
    notify_set_itimer_func(analyze_frame, check_if_done, ITIMER_REAL,
			   &timer, NULL);
}
