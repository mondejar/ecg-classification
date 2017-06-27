/* file: logpan.c	G. Moody	 1 May 1990
			Last revised:   22 June 1999
Log panel functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1999 George B. Moody

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
#include <string.h>
#include <X11/Xos.h>		/* for <sys/time.h> */
#include <xview/notice.h>
#include <xview/notify.h>
#include <xview/textsw.h>
#include <xview/defaults.h>

#define LLLMAX	(RNLMAX+40+DSLMAX)	/* max length of line in log file */

Frame log_frame;

/* A WAVE log file contains one-line entries.  Each entry specifies a record
   and a time or time interval in that record, and contains an associated
   text string, which (typically) contains a user-entered description of
   features of the signals at that time.  The format is:
      <record><whitespace><time_spec><whitespace><text>
   where
      <record> is the record name (if empty, use the record name given in the
	previous entry, or the name of the currently displayed record if this
	is the first entry)
      <time_spec> is either the start and end times separated by a hyphen, or
	the time of the center of the region of interest, in strtim format
      <text> is the text of the log entry, which cannot begin with whitespace
	but which may contain embedded whitespace (this field may be empty)
   and <whitespace> is a sequence of one or more space or tab characters.
   This format is compatible with `pschart' and `psfd' script file format.

   The log is kept internally as a doubly-linked list of log_entry structures.
*/

struct log_entry {
    struct log_entry *prev, *next;
    char *record, *time_spec, *text;
} *first_entry, *current_entry, *last_entry;

/* This function selects an annotation at a specified time.  If no such
   annotation exists, it moves the log marker (an index mark pseudo-annotation)
   to the specified time and selects the marker.
*/
void set_marker(t)
long t;
{
    static struct ap *log_marker;

    if (locate_annotation(t, 0))
	attached = annp;
    else if (log_marker) {
	move_annotation(log_marker, t);
	attached = log_marker;
    }
    else if (log_marker = get_ap()) {
	log_marker->this.time = t;
	log_marker->this.anntyp = INDEX_MARK;
	insert_annotation(log_marker);
	attached = log_marker;
    }
}

/* add_entry allocates memory for a log_entry structure, fills it in
   using the input arguments, and inserts it in the linked list at the
   location after the current entry.  (If current_entry is NULL, the
   new entry is inserted at the head of the list.)  Upon successful
   (non-zero) return, current_entry points to the newly inserted
   entry, and first_entry and last_entry may have been updated; if
   memory cannot be allocated, add_entry returns 0, and the log entry
   pointers are left unchanged. */

int add_entry(recp, timep, textp)
char *recp, *timep, *textp; /* record name, time specification, and text */
{
    struct log_entry *new_entry;
    char *p;
    long t0;

    /* Allocate memory for structure and strings. */
    if ((new_entry = (struct log_entry *)malloc(sizeof(struct log_entry)))
	== NULL ||
	(new_entry->record = (char *)malloc(strlen(recp)+1)) == NULL ||
	(new_entry->time_spec = (char *)malloc(strlen(timep)+1)) == NULL ||
	(textp &&
	 (new_entry->text = (char *)malloc(strlen(textp)+1)) == NULL)) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			    NOTICE_MESSAGE_STRINGS,
			    "Error in allocating memory for log\n", 0,
			    NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	if (new_entry) {
	    if (new_entry->time_spec) free(new_entry->time_spec);
	    if (new_entry->record) free(new_entry->record);
	    free(new_entry);
	}
	return (0);
    }

    /* Fill in the log_entry structure. */
    strcpy(new_entry->record, recp);
    strcpy(new_entry->time_spec, timep);
    if (textp) strcpy(new_entry->text, textp);
    else new_entry->text = NULL;

    /* Insert the new entry into the linked list. */
    if (current_entry) {
	if (current_entry->next)	/* insert into middle of list */
	    (current_entry->next)->prev = new_entry;
	else				/* append at tail */
	    last_entry = new_entry;
	new_entry->next = current_entry->next;
	new_entry->prev = current_entry;
	current_entry->next = new_entry;
    }
    else if (first_entry) {		/* insert at head of list */
	new_entry->next = first_entry;
	new_entry->prev = NULL;
	first_entry->prev = new_entry;
	first_entry = new_entry;
    }
    else {				/* list is empty, initialize it */
	new_entry->next = new_entry->prev = NULL;
	first_entry = last_entry = new_entry;
    }

    current_entry = new_entry;
    p = strchr(timep, '-');
    if (p) *p = '\0';
    return (1);
}

/* delete_entry removes the current entry from the linked list, and resets the
   current_entry pointer so that it points to the next entry in the linked
   list if there is one, or to the previous entry otherwise. */

void delete_entry()
{
    struct log_entry *p;

    if (current_entry) {
	if (current_entry->prev)
	    (current_entry->prev)->next = current_entry->next;
	else				/* deleting entry at head */
	    first_entry = current_entry->next;
	if (p = current_entry->next)
	    (current_entry->next)->prev = current_entry->prev;
	else				/* deleting entry at tail */
	    p = last_entry = current_entry->prev;
	free(current_entry->record);
	free(current_entry->time_spec);
	if (current_entry->text) free(current_entry->text);
	free(current_entry);
	current_entry = p;
    }
}

/* read_log appends the contents of the log file named by its argument to
   the linked list.  It returns 1 if completely successful, 0 if the file
   cannot be read or if not all properly formatted entries can be stored. */

int read_log(logfname)
char *logfname;
{
    char buf[LLLMAX+1], *p, *recp = record, *timep, *textp = NULL, *strtok();
    FILE *logfile;
    int ignore;
    long t0, t1;

    if ((logfile = fopen(logfname, "r")) == NULL)
	return (0);
    while (fgets(buf, LLLMAX, logfile)) {	/* read and parse an entry */
	/* Check that entry is properly formatted -- if not, skip it. */
	for (p = buf, ignore = 0; *p; p++) {
	    if (!isprint(*p) && !isspace(*p)) { ignore = 1; break; }
	}
	if (ignore) continue;
	if (buf[0] != ' ' && buf[0] != '\t') {
	    recp = strtok(buf, " \t");	/* first token is record name */
	    timep = strtok(NULL, " \t\n");/* second token is time spec */
	}
	else			/* record name missing, use previous value */
	    timep = strtok(buf, " \t");	/* first token is time spec */
	if (recp == NULL || timep == NULL) continue;
	/* skip improperly formatted entries */
	textp = strtok(NULL, "\n");	/* remainder of line is text */

	if (add_entry(recp, timep, textp) == 0) {
	    fclose(logfile);
	    return (0);
	}
    }
    fclose(logfile);
    return (1);
}

/* write_log copies the contents of the linked list to the log file named by
   its argument.  It returns 1 if successful, 0 otherwise. */

static int log_changes, save_log_backup;

int write_log(logfname)
char *logfname;
{
    int result;
    struct log_entry *p;
    FILE *logfile;

    /* Do we need to back up ? */
    if (save_log_backup) {
	char backfname[LNLMAX+2];

	sprintf(backfname, "%s~", logfname);
	if (rename(logfname, backfname)) {
#ifdef NOTICE
	    Xv_notice notice = xv_create((Frame)frame, NOTICE,
					 XV_SHOW, TRUE,
					 NOTICE_STATUS, &result,
#else
	    result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
		NOTICE_MESSAGE_STRINGS,
		"Your log cannot be saved unless you remove the file named",
		backfname,
		"",
		"You may attempt to correct this problem from",
		"another window after pressing `Continue', or",
		"you may exit immediately and discard your",
		"changes by pressing `Exit'.", 0,
		NOTICE_BUTTON_YES, "Continue",
		NOTICE_BUTTON_NO, "Exit", NULL);
#ifdef NOTICE
	    xv_destroy_safe(notice);
#endif
	    if (result == NOTICE_YES) return (0);
	    else if (post_changes()) {
		xv_destroy_safe(frame);
		exit(1);
	    }
	    else return (0);
	}
	save_log_backup = 0;
    }
	
    if ((logfile = fopen(logfname, "w")) == NULL) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
				     NOTICE_STATUS, &result,
#else
	result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
		    NOTICE_MESSAGE_STRINGS,
		    "Your log cannot be saved until you obtain",
		    "write permission for",
		    logfname,
		    "",
		    "You may attempt to correct this problem from",
		    "another window after pressing `Continue', or",
		    "you may exit immediately and discard your",
		    "changes by pressing `Exit'.", 0,
		    NOTICE_BUTTON_YES, "Continue",
		    NOTICE_BUTTON_NO, "Exit", NULL);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	if (result == NOTICE_YES) return (0);
	else if (post_changes()) {
	    xv_destroy_safe(frame);
	    exit(1);
	}
	else return (0);
    }

    for (p = first_entry; p; p = p->next) {
	fprintf(logfile, "%s %s", p->record, p->time_spec);
	if (p->text) fprintf(logfile, " %s", p->text);
	fprintf(logfile, "\n");
    }

    log_changes = 0;
    fclose(logfile);
    return (1);
}

Panel_item log_name_item, log_text_item, load_button,
    add_button, replace_button, delete_button, edit_button, first_button,
    rreview_button, prev_button, pause_button, next_button, review_button,
    last_button;
struct itimerval timer;

Panel log_panel;

void show_current_entry()
{
    char *p;
    int record_changed = 0;
    long t0;

    if (current_entry) {
	if (current_entry->text)
	    strncpy(description, current_entry->text, DSLMAX);
	else
	    description[0] = '\0';
	if (strcmp(record, current_entry->record)) {
	    set_record_item(current_entry->record);
	    record_changed = 1;
	}
	p = strchr(current_entry->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->time_spec)) < 0L) t0 = -t0;
	if (p)		/* start and end times are given */
	    *p = '-';		
	else {		/* only one time given -- place mark at that time */
	    set_marker(t0);
	    if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	}
	if (!record_changed) {
	    set_frame_title();
	    (void)find_display_list(t0);
	}
	set_start_time(timstr(t0));
	set_end_time(timstr(t0 + nsamp));
	xv_set(log_text_item, PANEL_VALUE, description, NULL);
	disp_proc(log_text_item, (Event *)NULL);
	if (attached)
	  box((int)((attached->this.time - display_start_time)*tscale),
	    (ann_mode==1 && (unsigned)attached->this.chan < nsig) ?
	    (int)(base[(unsigned)attached->this.chan] + mmy(2)) : abase,
	    1);
    }
}

Notify_value show_next_entry()
{
    char *p;
    long t0;

    if (current_entry->next) current_entry = current_entry->next;
    else current_entry = first_entry;
    show_current_entry();
    if (current_entry->next &&
	strcmp(current_entry->next->record, current_entry->record) == 0) {
	p = strchr(current_entry->next->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->next->time_spec)) < 0L) t0 = -t0;
	if (p) *p = '-';		/* t1 = strtim(p+1); */
	else if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	(void)find_display_list(t0);
    }
    return (NOTIFY_DONE);
}

Notify_value show_prev_entry()
{
    char *p;
    long t0;

    if (current_entry->prev) current_entry = current_entry->prev;
    else current_entry = last_entry;
    show_current_entry();
    if (current_entry->prev &&
	strcmp(current_entry->prev->record, current_entry->record) == 0) {
	p = strchr(current_entry->prev->time_spec, '-');
	if (p) *p = '\0';
	if ((t0 = strtim(current_entry->prev->time_spec)) < 0L) t0 = -t0;
	if (p) *p = '-';		/* t1 = strtim(p+1); */
	else if ((t0 -= nsamp/2) < 0L) t0 = 0L;
	(void)find_display_list(t0);
    }
    return (NOTIFY_DONE);
}

int review_delay;
int review_in_progress;

void log_review(direction)
int direction;
{
    review_in_progress = direction;
    timer.it_value.tv_sec = timer.it_interval.tv_sec = review_delay;
    if (direction == 1)
	notify_set_itimer_func(log_frame, show_next_entry, ITIMER_REAL,
			       &timer, NULL);
    else
	notify_set_itimer_func(log_frame, show_prev_entry, ITIMER_REAL,
			       &timer, NULL);
}	

void pause_review()
{
    review_in_progress = 0;
    notify_set_itimer_func(log_frame,NOTIFY_FUNC_NULL,ITIMER_REAL,NULL,NULL);
}

/* Handle enabling/disabling log navigation buttons for non-review modes. */
void set_buttons()
{
    xv_set(pause_button, PANEL_INACTIVE, TRUE, 0);
    if (log_file_name) {
	xv_set(load_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(add_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(edit_button, PANEL_INACTIVE, FALSE, 0);
    }
    if (current_entry) {
	xv_set(replace_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(delete_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(first_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(last_button, PANEL_INACTIVE, FALSE, 0);
	xv_set(next_button, PANEL_INACTIVE,
	       current_entry->next ? FALSE : TRUE, 0);
	xv_set(prev_button, PANEL_INACTIVE,
	       current_entry->prev ? FALSE : TRUE, 0);
	xv_set(review_button, PANEL_INACTIVE,
	       (current_entry->next || current_entry->prev) ? FALSE : TRUE, 0);
	xv_set(rreview_button, PANEL_INACTIVE,
	       (current_entry->next || current_entry->prev) ? FALSE : TRUE, 0);
    }
    else {
	xv_set(replace_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(delete_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(first_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(rreview_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(prev_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(next_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(review_button, PANEL_INACTIVE, TRUE, 0);
	xv_set(last_button, PANEL_INACTIVE, TRUE, 0);
    }
}

void disable_buttons()
{
    xv_set(load_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(add_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(replace_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(delete_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(edit_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(first_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(rreview_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(prev_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(pause_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(next_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(review_button, PANEL_INACTIVE, TRUE, 0);
    xv_set(last_button, PANEL_INACTIVE, TRUE, 0);
}

/* Edit the log file. */
void edit_log_file()
{
    char *edit_command, *editor, *getenv();
    int clen, elen, result;

    if ((editor = getenv("EDITOR")) == NULL)
	editor = defaults_get_string("wave.texteditor",
				     "Wave.TextEditor",
				     EDITOR);
    elen = strlen(editor);
    edit_command = malloc(elen + strlen(log_file_name) + 3);
    if (edit_command) {
	sprintf(edit_command, "%s %s\n", editor, log_file_name);
	analyze_proc();
	do_command(edit_command);
	free(edit_command);
    }
}
    
/* Handle selections in the log window. */
static void log_select(item, event)
Panel_item item;
Event *event;
{
    int client_data = (int)xv_get(item, PANEL_CLIENT_DATA);
    char timestring[25];
    switch (client_data) {
      case 'a':		/* add an entry */
	if (attached && display_start_time < attached->this.time &&
	    attached->this.time < display_start_time + nsamp)
	    strcpy(timestring, mstimstr(attached->this.time));
	else {
	    strcpy(timestring, strtok(timstr(display_start_time), " "));
	    strcat(timestring, "-");
	    strcat(timestring, strtok(timstr(display_start_time+nsamp), " "));
	}
	if (add_entry(record, timestring, xv_get(log_text_item,PANEL_VALUE))) {
	    if (++log_changes > 10) write_log(log_file_name);
	    set_buttons();
	}
	break;
      case 'd':		/* delete the current entry */
	delete_entry();
	if (++log_changes > 10) write_log(log_file_name);
	set_buttons();
	show_current_entry();
	break;
      case 'e':		/* edit the log file */
	if (log_file_name) {
	    disable_buttons();
	    if (log_changes > 0) write_log(log_file_name);
	    edit_log_file();
	    /* Clear out the old entries. */
	    for (current_entry = first_entry; current_entry; )
		delete_entry();
	    /* Reinitialize from the new log file. */
	    if (read_log(log_file_name))
		save_log_backup = 1;
	    log_changes = 0;
	    current_entry = first_entry;
	    set_buttons();
	}
	break;
      case 'f':		/* specify log file name */
	/* Do nothing unless the name has been changed. */
	if (strncmp(log_file_name, (char *)xv_get(log_name_item,PANEL_VALUE),
		    LNLMAX)) {
	    /* If edits have been made, write the current log to the old file.
	       If the write fails, reset the file name (after write_log has
	       alerted the user). */
	    if (log_changes && write_log(log_file_name) == 0)
		xv_set(log_name_item, PANEL_VALUE, log_file_name, 0);
	    else {
		/* Clear out the old entries. */
		for (current_entry = first_entry; current_entry; )
		    delete_entry();
		strncpy(log_file_name,
			(char *)xv_get(log_name_item, PANEL_VALUE), LNLMAX);
		/* Reinitialize from the new log file. */
		if (read_log(log_file_name))
		    save_log_backup = 1;
		log_changes = 0;
		current_entry = first_entry;
	    }
	    set_buttons();
	    show_current_entry();
	}
	break;
      case 'l':		/* force reload of log file */
	/* If edits have been made, write the current log to the old file.
	   If the write fails, reset the file name (after write_log has
	   alerted the user). */
	if (log_changes) {
	    char backfname[LNLMAX+2];

	    sprintf(backfname, "%s~", log_file_name);
	    save_log_backup = 0;
	    write_log(backfname);
	}
	/* Clear out the old entries. */
	for (current_entry = first_entry; current_entry; )
	    delete_entry();
	strncpy(log_file_name,
		(char *)xv_get(log_name_item, PANEL_VALUE), LNLMAX);
	/* Reinitialize from the new log file. */
	if (read_log(log_file_name))
	    save_log_backup = 1;
	log_changes = 0;
	current_entry = first_entry;
	set_buttons();
	show_current_entry();
	break;
      case 'p':		/* pause review */
	pause_review();
	set_buttons();
	break;
      case 'r':		/* replace description field of current entry */
	if (current_entry) {
	    char *newtext = (char *)xv_get(log_text_item, PANEL_VALUE);
	    char *newtextp;

	    if (strcmp(newtext, current_entry->text) &&
		(newtextp = (char *)malloc(strlen(newtext)+1))) {
		strcpy(newtextp, newtext);
		free(current_entry->text);
		current_entry->text = newtextp;
		if (++log_changes > 10) write_log(log_file_name);
	    }
	}
	break;
      case 'A':		/* show first entry */
	if (first_entry) {
	    current_entry = first_entry;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '<':		/* show previous entry */
	if (current_entry->prev) {
	    current_entry = current_entry->prev;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '>':		/* show next entry */
	if (current_entry->next) {
	    current_entry = current_entry->next;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case 'Z':		/* show last entry */
	if (last_entry) {
	    current_entry = last_entry;
	    set_buttons();
	    show_current_entry();
	}
	break;
      case '+':		/* review log entries */
      case '-':		/* review log entries in reverse order */
	disable_buttons();
	xv_set(pause_button, PANEL_INACTIVE, FALSE, 0);
	log_review(client_data == '+' ? 1 : -1);
	break;
    }
}

static void adjust_delay(item, value)
Panel_item item;
int value;
{
    review_delay = value;
    if (review_in_progress) log_review(review_in_progress);
}

/* Set up log window. */
static void create_log_popup()
{
    int dx;
    Icon icon;

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Log",
		     NULL);
    log_frame = xv_create(frame, FRAME_CMD,
			  FRAME_LABEL, "WAVE log",
			  FRAME_ICON, icon, 0);
    log_panel = xv_get(log_frame, FRAME_CMD_PANEL);
    log_name_item = xv_create(log_panel, PANEL_TEXT,
	XV_X, xv_col(log_panel, 0),
	XV_Y, xv_row(log_panel, 0),
	PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
	PANEL_LABEL_STRING, "File: ",
	XV_HELP_DATA, "wave:file.log.file",
	PANEL_VALUE_DISPLAY_LENGTH, 60,
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'f',
	0);
    load_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Load",
	XV_HELP_DATA, "wave:file.log.load",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'l',
	PANEL_INACTIVE, TRUE,
	0);
    log_text_item = xv_create(log_panel, PANEL_TEXT,
 	XV_X, xv_col(log_panel, 0),
	XV_Y, xv_row(log_panel, 1),
	PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
	PANEL_LABEL_STRING, "Description: ",
	XV_HELP_DATA, "wave:file.log.description",
	PANEL_VALUE_DISPLAY_LENGTH, 50,
	PANEL_CLIENT_DATA, (caddr_t) '!',  /* used by disp_proc, see annot.c */
	0);
    xv_create(log_panel, PANEL_SLIDER,
	XV_HELP_DATA, "wave:file.log.review_delay",
	PANEL_LABEL_STRING, "Delay:",
	PANEL_DIRECTION, PANEL_HORIZONTAL,
	PANEL_VALUE, 5,
	PANEL_MAX_VALUE, 10,
	PANEL_MIN_VALUE, 1,
	PANEL_SHOW_RANGE, TRUE,
	PANEL_SHOW_VALUE, FALSE,
	PANEL_NOTIFY_PROC, adjust_delay,
	NULL);
    add_button = xv_create(log_panel, PANEL_BUTTON,
        XV_X, xv_col(log_panel, 0),
	XV_Y, xv_row(log_panel, 3),
	PANEL_LABEL_STRING, "Add",
	XV_HELP_DATA, "wave:file.log.add",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'a',
	PANEL_INACTIVE, TRUE,
	0);
    replace_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Replace",
	XV_HELP_DATA, "wave:file.log.replace",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'r',
	PANEL_INACTIVE, TRUE,
	0);
    delete_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Delete",
	XV_HELP_DATA, "wave:file.log.delete",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'd',
	PANEL_INACTIVE, TRUE,
	0);
    edit_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Edit",
	XV_HELP_DATA, "wave:file.log.edit",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'e',
	PANEL_INACTIVE, TRUE,
	0);

    dx = xv_get(log_panel, PANEL_ITEM_X_GAP);
    xv_set(log_panel, PANEL_ITEM_X_GAP, 4*dx, 0);

    first_button = xv_create(log_panel, PANEL_BUTTON,
        XV_X, xv_col(log_panel, 0),
	XV_Y, xv_row(log_panel, 4),
	PANEL_LABEL_STRING, "|<",
	XV_HELP_DATA, "wave:file.log.|<",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'A',
	PANEL_INACTIVE, TRUE,
	0);

    xv_set(log_panel, PANEL_ITEM_X_GAP, dx, 0);

    rreview_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "<<",
	XV_HELP_DATA, "wave:file.log.<<",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) '-',
	PANEL_INACTIVE, TRUE,
	0);
    prev_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "<",
	XV_HELP_DATA, "wave:file.log.<",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) '<',
	PANEL_INACTIVE, TRUE,
	0);
    pause_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, "Pause",
	XV_HELP_DATA, "wave:file.log.pause",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'p',
	PANEL_INACTIVE, TRUE,
	0);
    next_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, ">",
	XV_HELP_DATA, "wave:file.log.>",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) '>',
	PANEL_INACTIVE, TRUE,
	0);
    review_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, ">>",
	XV_HELP_DATA, "wave:file.log.>>",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) '+',
	PANEL_INACTIVE, TRUE,
	0);
    last_button = xv_create(log_panel, PANEL_BUTTON,
	PANEL_LABEL_STRING, ">|",
	XV_HELP_DATA, "wave:file.log.>|",
	PANEL_NOTIFY_PROC, log_select,
	PANEL_CLIENT_DATA, (caddr_t) 'Z',
	PANEL_INACTIVE, TRUE,
	0);

    window_fit(log_panel);
    window_fit(log_frame);
    xv_set(log_frame, FRAME_CMD_PUSHPIN_IN, TRUE, 0);
}

int log_popup_active = -1;

/* Make the log popup window appear. */
void show_log()
{
    if (log_popup_active < 0) create_log_popup();
    wmgr_top(log_frame);
    xv_set(log_frame, WIN_MAP, TRUE, 0);
    log_popup_active = 1;
}

/* Update and close the log file. */
void finish_log()
{
    if (log_changes > 0) write_log(log_file_name);
}

/* Enter demonstration mode. */
void start_demo()
{
    char *filename, *p, *title, *getenv();
    int c, r, x, y;
    Frame text_frame;
    Textsw textsw;
    Textsw_status status;
    extern void mode_undo();

    if (filename = malloc(strlen(helpdir) + strlen("wave/demo.txt") + 2)) {
        if ((title = getenv("DEMOTITLE")) == NULL)
	    title = "Demonstration of WAVE";
	if ((p = getenv("DEMOX")) == NULL)
	    x = 10;
	else
	    x = atoi(p);
	if ((p = getenv("DEMOY")) == NULL)
	    y = 700;
	else
	    y = atoi(p);
	if ((p = getenv("DEMOCOLS")) == NULL)
	    c = 80;
	else
	    c = atoi(p);
	if ((p = getenv("DEMOROWS")) == NULL)
	    r = 20;
	else
	    r = atoi(p);
        text_frame = xv_create(frame, FRAME,
			       XV_LABEL, title, XV_X, x, XV_Y, y,
			       WIN_COLUMNS, c, WIN_ROWS, r, 0);
	textsw = (Textsw)xv_create(text_frame, TEXTSW, NULL);
	sprintf(filename, "%s/wave/demo.txt", helpdir);
	xv_set(textsw,
	       TEXTSW_STATUS, &status,
	       TEXTSW_FILE, filename,
	       TEXTSW_FIRST, 0,
	       TEXTSW_READ_ONLY, TRUE,
	       NULL);
	if (status == TEXTSW_STATUS_OKAY)
	    xv_set(text_frame, WIN_MAP, TRUE, 0);
	free(filename);
    }
    create_log_popup();
    log_popup_active = 0;
    xv_set(log_name_item, PANEL_VALUE, log_file_name, NULL);
    show_mode();
    ghflag = gvflag = visible = 1;
    show_signame = 16;
    mode_undo();
    dismiss_mode();
    if (read_log(log_file_name)) {
	xv_set(pause_button, PANEL_INACTIVE, FALSE, 0);
	log_review((Panel_item)NULL, (Event *)NULL);
    }
}
