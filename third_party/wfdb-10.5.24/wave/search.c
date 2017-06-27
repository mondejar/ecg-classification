/* file: search.c	G. Moody	17 October 1996
			Last revised:    13 July 2010
Search template functions for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1996-2010 George B. Moody

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

Frame search_frame;
Panel search_panel;
Panel_item s_anntyp_item, s_subtyp_item, s_num_item, s_chan_item, s_aux_item;
Panel_item s_anntyp_mask, s_subtyp_mask, s_num_mask, s_chan_mask, s_aux_mask;

int search_popup_active = -1;

/* Make the search template popup window disappear. */
static void dismiss_search_template()
{
    if (search_popup_active > 0) {
	xv_set(search_frame, WIN_MAP, FALSE, 0);
	search_popup_active = 0;
    }
}

/* Update the search template following a selection in the popup window. */
static void search_select(item, event)
Panel_item item;
Event *event;
{
    int a;
    static char client_data, m[2], tmp_aux[257];

    switch (a = (int)xv_get(item, PANEL_CLIENT_DATA)) {
      case 'T':
	if ((int)xv_get(item, PANEL_VALUE) == 1)
	    search_mask |= M_ANNTYP;	/* set anntyp mask bit */
	else
	    search_mask &= ~M_ANNTYP;	/* clear anntyp mask bit */
	break;
      case 'S':
	if ((int)xv_get(item, PANEL_VALUE) == 1)
	    search_mask |= M_SUBTYP;	/* set subtyp mask bit */
	else
	    search_mask &= ~M_SUBTYP;	/* clear subtyp mask bit */
	break;
      case 'N':
	if ((int)xv_get(item, PANEL_VALUE) == 1)
	    search_mask |= M_NUM;	/* set num mask bit */
	else
	    search_mask &= ~M_NUM;	/* clear num mask bit */
	break;
      case 'C':
	if ((int)xv_get(item, PANEL_VALUE) == 1)
	    search_mask |= M_CHAN;	/* set chan mask bit */
	else
	    search_mask &= ~M_CHAN;	/* clear chan mask bit */
	break;
      case 'A':
	if ((int)xv_get(item, PANEL_VALUE) == 1)
	    search_mask |= M_AUX;	/* set aux mask bit */
	else
	    search_mask &= ~M_AUX;	/* clear aux mask bit */
	break;
      case 't': search_template.anntyp = (int)xv_get(item, PANEL_VALUE); break;
      case 's':	search_template.subtyp = (int)xv_get(item, PANEL_VALUE); break;
      case 'n': search_template.num    = (int)xv_get(item, PANEL_VALUE); break;
      case 'c': search_template.chan   = (int)xv_get(item, PANEL_VALUE); break;
      case 'a': strcpy(tmp_aux+1, (char *)xv_get(item, PANEL_VALUE));
	if (tmp_aux[0] = strlen(tmp_aux+1))
	    search_template.aux = tmp_aux;
	else
	    search_template.aux = NULL;
	break;
    }
}

static char *mstr[53];

static void s_create_mstr_array()
{
    char *dp, mbuf[5], *mp, *msp;
    int i;
    unsigned int l, lm;

    mstr[0] = ".    (Deleted annotation)";
    for (i = 1; i <= ACMAX; i++) {
	if ((mp = annstr(i)) == NULL) {
	    sprintf(mbuf, "[%d]", i);
	    mp = mbuf;
	}
	if ((dp = anndesc(i)) == NULL)
	    dp = "(unassigned annotation type)";
	lm = strlen(mp);
	l = strlen(dp) + 6;
	if (lm > 4) l += lm - 4;
 	if ((mstr[i] = (char *)malloc(l)) == NULL)
	    break;
	strcpy(mstr[i], mp);
	do {
	    mstr[i][lm] = ' ';
	} while (++lm < 5);
	strcpy(mstr[i]+lm, dp);
    }
    mstr[ACMAX+1] = ":    (Index mark)";
    mstr[ACMAX+2] = "<    (Start of analysis)";
    mstr[ACMAX+3] = ">    (End of analysis)";
}

/* Set the annotation template window `Type' item. */
static void match_selected_annotation()
{
    if (attached == NULL) return;
    xv_set(s_anntyp_item, PANEL_VALUE, attached->this.anntyp, NULL);
    xv_set(s_aux_item, PANEL_VALUE,
	   attached->this.aux ? (char *)(attached->this.aux+1) : "", NULL);
    xv_set(s_subtyp_item, PANEL_VALUE, attached->this.subtyp, NULL);
    xv_set(s_chan_item, PANEL_VALUE, attached->this.chan, NULL);
    xv_set(s_num_item, PANEL_VALUE, attached->this.num, NULL);
    xv_set(s_anntyp_mask, PANEL_VALUE, 1, NULL);
    xv_set(s_aux_mask, PANEL_VALUE, 1, NULL);
    xv_set(s_subtyp_mask, PANEL_VALUE, 1, NULL);
    xv_set(s_chan_mask, PANEL_VALUE, 1, NULL);
    xv_set(s_num_mask, PANEL_VALUE, 1, NULL);
    search_template = attached->this;
    search_mask = M_ANNTYP | M_SUBTYP | M_CHAN | M_NUM;
    if (search_template.aux) search_mask |= M_AUX;
}

/* Set up popup window for defining the search template. */
static void create_search_template_popup()
{
    Icon icon;

    s_create_mstr_array();
    search_template.anntyp = 1;
    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Search",
		     NULL);
    search_frame = xv_create(frame, FRAME,
			  XV_LABEL, "Search Template",
			  FRAME_ICON, icon, 0);
    search_panel = xv_create(search_frame, PANEL, 0);
    s_anntyp_mask = xv_create(search_panel, PANEL_CHOICE_STACK,
			      PANEL_CHOICE_STRINGS, "Ignore", "Match", NULL,
			      PANEL_VALUE, 0,
			      PANEL_NOTIFY_PROC, search_select,
			      PANEL_CLIENT_DATA, (caddr_t) 'T',
			      NULL);
    s_anntyp_item = xv_create(search_panel, PANEL_CHOICE,
			    PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			    PANEL_LABEL_STRING, "Type: ",
			    XV_HELP_DATA, "wave:search.type",
			    PANEL_CHOICE_STRINGS,
			     mstr[ 0], mstr[ 1], mstr[ 2], mstr[ 3], mstr[ 4],
			     mstr[ 5], mstr[ 6], mstr[ 7], mstr[ 8], mstr[ 9],
			     mstr[10], mstr[11], mstr[12], mstr[13], mstr[14],
			     mstr[15], mstr[16], mstr[17], mstr[18], mstr[19],
			     mstr[20], mstr[21], mstr[22], mstr[23], mstr[24],
			     mstr[25], mstr[26], mstr[27], mstr[28], mstr[29],
			     mstr[30], mstr[31], mstr[32], mstr[33], mstr[34],
			     mstr[35], mstr[36], mstr[37], mstr[38], mstr[39],
			     mstr[40], mstr[41], mstr[42], mstr[43], mstr[44],
			     mstr[45], mstr[46], mstr[47], mstr[48], mstr[49],
			     mstr[50], mstr[51], mstr[52],
			     NULL,
			    PANEL_VALUE, 1,
			    PANEL_NOTIFY_PROC, search_select,
			    PANEL_CLIENT_DATA, (caddr_t) 't',
			    0);
    s_aux_mask = xv_create(search_panel, PANEL_CHOICE_STACK,
			      PANEL_NEXT_ROW, -1,
			      PANEL_CHOICE_STRINGS, "Ignore", "Match", NULL,
			      PANEL_VALUE, 0,
			      PANEL_NOTIFY_PROC, search_select,
			      PANEL_CLIENT_DATA, (caddr_t) 'A',
			      NULL);
    s_aux_item    = xv_create(search_panel, PANEL_TEXT,
			    PANEL_LABEL_STRING, "Text: ",
			    XV_HELP_DATA, "wave:search.text",
			    PANEL_VALUE_DISPLAY_LENGTH, 20,
			    PANEL_VALUE_STORED_LENGTH, 255,
			    PANEL_VALUE, "",
			    PANEL_NOTIFY_PROC, search_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'a',
			    0);
    s_subtyp_mask = xv_create(search_panel, PANEL_CHOICE_STACK,
			      PANEL_NEXT_ROW, -1,
			      PANEL_CHOICE_STRINGS, "Ignore", "Match", NULL,
			      PANEL_VALUE, 0,
			      PANEL_NOTIFY_PROC, search_select,
			      PANEL_CLIENT_DATA, (caddr_t) 'S',
			      NULL);
    s_subtyp_item = xv_create(search_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "Subtype: ",
			    XV_HELP_DATA, "wave:search.subtype",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, search_select,
			    PANEL_CLIENT_DATA, (caddr_t) 's',
			    0);
    s_chan_mask = xv_create(search_panel, PANEL_CHOICE_STACK,
			      PANEL_NEXT_ROW, -1,
			      PANEL_CHOICE_STRINGS, "Ignore", "Match", NULL,
			      PANEL_VALUE, 0,
			      PANEL_NOTIFY_PROC, search_select,
			      PANEL_CLIENT_DATA, (caddr_t) 'C',
			      NULL);
    s_chan_item   = xv_create(search_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "`Chan' field: ",
			    XV_HELP_DATA, "wave:search.chan",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, search_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'c',
			    0);
    s_num_mask = xv_create(search_panel, PANEL_CHOICE_STACK,
			      PANEL_NEXT_ROW, -1,
			      PANEL_CHOICE_STRINGS, "Ignore", "Match", NULL,
			      PANEL_VALUE, 0,
			      PANEL_NOTIFY_PROC, search_select,
			      PANEL_CLIENT_DATA, (caddr_t) 'N',
			      NULL);
    s_num_item    = xv_create(search_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "`Num' field: ",
			    XV_HELP_DATA, "wave:search.num",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, search_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'n',
			    0);
    xv_create(search_panel, PANEL_BUTTON,
	      PANEL_NEXT_ROW, -1,
	      PANEL_LABEL_STRING, "Match selected annotation",
	      XV_HELP_DATA, "wave:search.matchselected",
	      PANEL_NOTIFY_PROC, match_selected_annotation,
	      0);
    xv_create(search_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Dismiss",
	      XV_HELP_DATA, "wave:search.dismiss",
	      PANEL_NOTIFY_PROC, dismiss_search_template,
	      0);
    window_fit(search_panel);
    window_fit(search_frame);
}

/* Make the search template popup window appear. */
void show_search_template()
{
    if (search_popup_active < 0) create_search_template_popup();
    wmgr_top(search_frame);
    xv_set(search_frame, WIN_MAP, TRUE, 0);
    search_popup_active = 1;
    set_find_item("");
}

