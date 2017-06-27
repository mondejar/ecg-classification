/* file: helppan.c	G. Moody        1 May 1990
			Last revised:  24 June 2002
Help panel functions for WAVE

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
#include <xview/notice.h>
#include <xview/textsw.h>

/* On-line help files for WAVE are located in *(helpdir)/wave.  Their names are
   of the form *(helpdir)/wave/TOPIC.hlp, where TOPIC is one of the topics
   given in the help popup menu (possibly abbreviated).  The master help file
   is *(helpdir)/wave/HELPFILE. */
#define HELPFILE	"wave.hlp"

void find_user_guide()
{
    FILE *ifile;

    sprintf(url, "%s/html/wug/wug.htm", helpdir);
    if (ifile = fopen(url, "r")) fclose(ifile);
    else if (ifile = fopen("wug.htm", "r")) {
	fclose(ifile);
	sprintf(url, "%s/wug.htm", getcwd(NULL, 256));
    }
    else
	strcpy(url, "http://www.physionet.org/physiotools/wug/");
}

void find_faq()
{
    FILE *ifile;

    sprintf(url, "%s/html/wug/wave-faq.htm", helpdir);
    if (ifile = fopen(url, "r")) fclose(ifile);
    else if (ifile = fopen("wave-faq.htm", "r")) {
	fclose(ifile);
	sprintf(url, "%s/wave-faq.htm", getcwd(NULL, 256));
    }
    else
	strcpy(url, "http://www.physionet.org/physiotools/wug/wave-faq.htm");
}

void help()
{
    find_user_guide();
    fprintf(stderr, "WAVE version %s (%s)\n %s", WAVEVERSION, __DATE__,
	    wfdberror());
    fprintf(stderr, "usage: %s -r RECORD[+RECORD] [ options ]\n", pname);
    fprintf(stderr, "\nOptions are:\n");
    fprintf(stderr, " -a annotator-name  Open an annotation file\n");
    fprintf(stderr," -dpi XX[xYY]       Calibrate for XX [by YY] dots/inch\n");
    fprintf(stderr, " -f TIME            Open the record beginning at TIME\n");
    fprintf(stderr, " -g                 Use shades of grey only\n");
    fprintf(stderr, " -H                 Use high-resolution mode\n");
    fprintf(stderr, " -m                 Use black and white only\n");
    fprintf(stderr, " -O                 Use overlay graphics\n");
    fprintf(stderr, " -p PATH            Search for input files in PATH\n");
    fprintf(stderr, "                     (if not found in $WFDB)\n");
    fprintf(stderr, " -s SIGNAL [SIGNAL ...]  Initialize the signal list\n");
    fprintf(stderr, " -S                 Use a shared colormap\n");
    fprintf(stderr, " -Vx                Set initial display option x\n");
    if (getenv("DISPLAY") == NULL) {
	fprintf(stderr, "\n%s is an X11 client.  ", pname);
	fprintf(stderr, "You must specify the X server\n");
	fprintf(stderr, "connection for it in the DISPLAY environment ");
	fprintf(stderr, "variable.\n");
    }
    if (getenv("WFDB") == NULL) {
	fprintf(stderr, "\nBe sure to set the WFDB environment variable to\n");
	fprintf(stderr, "indicate a list of locations that contain\n");
	fprintf(stderr, "input files for %s.\n", pname);
    }
    fprintf(stderr, "\nFor more information, type `more %s/wave/%s',\n",
	    helpdir, HELPFILE);
    fprintf(stderr, "or open `%s' using\nyour web browser.\n", url);
    exit(1);
}

Frame help_frame;
Panel help_panel;
char *topic;

static void help_print()
{
    char *print_command;

    if (topic &&
	(print_command = 
	 malloc(strlen(textprint) + strlen(helpdir) + strlen(topic) + 14))) {
	sprintf(print_command, "%s %s/wave/%s.hlp\n",
		textprint, helpdir, topic);
	do_command(print_command);
	free(print_command);
    }
}


/* Open the WAVE User's Guide in a web browser. */
static void help_user_guide(item, event)
Panel_item item;
Event *event;
{
    find_user_guide();
    open_url();
}
/* Open the WAVE FAQ in a web browser. */
static void help_faq(item, event)
Panel_item item;
Event *event;
{
    find_faq();
    open_url();
}

/* Create a text subwindow and display help on selected topic. */
static void help_select(item, event)
Panel_item item;
Event *event;
{
    char *filename;
    Frame help_subframe;
    Panel help_subpanel;
    Textsw textsw;
    Textsw_status status;

    switch ((int)xv_get(item, PANEL_CLIENT_DATA)) {
      case 'a': topic = "analysis"; break;
      case 'b': topic = "buttons"; break;
      case 'e': topic = "editing"; break;
      case 'f': topic = "faq"; break;
      case 'l': topic = "log"; break;
      case 'n': topic = "news"; break;
      case 'p': topic = "printing"; break;
      case 'r': topic = "resource"; break;
      default:
      case 'i':	topic = "intro"; break;
    }
    if (filename = malloc(strlen(helpdir) + strlen(topic) + 11)) {
	/* strlen("/wave/") + strlen(."hlp") + 1 for the trailing null = 11 */
	Icon icon;

	icon = xv_create(XV_NULL, ICON,
			 ICON_IMAGE, icon_image,
			 ICON_LABEL, topic,
			 NULL);
	help_subframe = xv_find(help_frame, FRAME,
				  XV_LABEL, topic,
				  WIN_COLUMNS, 100,
				  FRAME_ICON, icon, 0);
	help_subpanel = xv_find(help_subframe, PANEL,
				WIN_ROWS, 1,
				0);
	xv_find(help_subpanel, PANEL_BUTTON,
		PANEL_LABEL_STRING, "Print",
		XV_HELP_DATA, "wave:help.print",
		PANEL_NOTIFY_PROC, help_print,
		0);
	textsw = (Textsw)xv_find(help_subframe, TEXTSW,
				 WIN_BELOW, help_subpanel,
				 WIN_X, 0,
				 NULL);
	sprintf(filename, "%s/wave/%s.hlp", helpdir, topic);
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
			  "Sorry, help is not available for this topic.", 0,
			  NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	    xv_destroy_safe(notice);
#endif
	    xv_destroy_safe(help_subframe);
	}
	else {
	    wmgr_top(help_subframe);
	    xv_set(help_subframe, WIN_MAP, TRUE, 0);
	}
	free(filename);
    }
}

/* Set up popup window for help. */
void create_help_popup()
{
    Icon icon;

    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Help",
		     NULL);
    help_frame = xv_create(frame, FRAME,
			   XV_LABEL, "Help Topics",
			   FRAME_ICON, icon, 0);
    help_panel = xv_create(help_frame, PANEL, 0);
    xv_create(help_panel, PANEL_MESSAGE,
	      PANEL_NEXT_ROW, -1,
	      PANEL_LABEL_STRING, "WAVE",
	      PANEL_LABEL_BOLD, TRUE,
	      0);
    xv_create(help_panel, PANEL_MESSAGE,
	      PANEL_LABEL_STRING, WAVEVERSION,
	      PANEL_LABEL_BOLD, TRUE,
	      0);
    xv_create(help_panel, PANEL_MESSAGE,
	      PANEL_LABEL_STRING,
	     "Copyright \251 1990-2010 George B. Moody.",
	      0);

    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Introduction",
	      XV_HELP_DATA, "wave:help.intro",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'i',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Buttons",
	      XV_HELP_DATA, "wave:help.buttons",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'b',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Annotation Editing",
	      XV_HELP_DATA, "wave:help.editing",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'e',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "WAVE Logs",
	      XV_HELP_DATA, "wave:help.logs",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'l',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Printing",
	      XV_HELP_DATA, "wave:help.printing",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'p',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Analysis",
	      XV_HELP_DATA, "wave:help.analysis",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'a',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Resources",
	      XV_HELP_DATA, "wave:help.resources",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'r',
	      0);
    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "What's new",
	      XV_HELP_DATA, "wave:help.news",
	      PANEL_NOTIFY_PROC, help_select,
	      PANEL_CLIENT_DATA, (caddr_t) 'n',
	      0);

    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Frequently asked questions",
	      XV_HELP_DATA, "wave:help.faq",
	      PANEL_NOTIFY_PROC, help_faq,
	      0);

    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "User's Guide",
	      XV_HELP_DATA, "wave:help.ug",
	      PANEL_NOTIFY_PROC, help_user_guide,
	      0);

    xv_create(help_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Quit from Help",
	      XV_HELP_DATA, "wave:help.quit",
	      PANEL_NOTIFY_PROC, dismiss_help,
	      0);
    window_fit(help_panel);
    window_fit(help_frame);
}

int help_popup_active = -1;

/* Make the help popup window appear. */
void show_help()
{
    if (help_popup_active < 0) create_help_popup();
    wmgr_top(help_frame);
    xv_set(help_frame, WIN_MAP, TRUE, 0);
    help_popup_active = 1;
}

/* Make the help popup window disappear. */
void dismiss_help()
{
    if (help_popup_active > 0) {
	xv_set(help_frame, WIN_MAP, FALSE, 0);
	help_popup_active = 0;
    }
}
