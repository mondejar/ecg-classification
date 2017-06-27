/* file: annpan.c	G. Moody	1 May 1990
			Last revised:	29 April 1999
Annotation template panel functions for WAVE

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
#include <xview/notice.h>

Frame ann_frame;
Panel ann_panel;
Panel_item anntyp_item, subtyp_item, num_item, chan_item, aux_item;

int ann_popup_active = -1;

/* Make the annotation template popup window disappear. */
static void dismiss_ann_template()
{
    if (ann_popup_active > 0) {
	xv_set(ann_frame, WIN_MAP, FALSE, 0);
	ann_popup_active = 0;
    }
}

/* Update the annotation template following a selection in the popup window. */
static void ann_select(item, event)
Panel_item item;
Event *event;
{
    int a;
    static char client_data, m[2], tmp_aux[257];

    switch (a = (int)xv_get(item, PANEL_CLIENT_DATA)) {
      case 't': ann_template.anntyp = (int)xv_get(item, PANEL_VALUE); break;
      case 's':	ann_template.subtyp = (int)xv_get(item, PANEL_VALUE); break;
      case 'n': ann_template.num    = (int)xv_get(item, PANEL_VALUE); break;
      case 'c': ann_template.chan   = (int)xv_get(item, PANEL_VALUE); break;
      case 'a': strcpy(tmp_aux+1, (char *)xv_get(item, PANEL_VALUE));
	if (tmp_aux[0] = strlen(tmp_aux+1))
	    ann_template.aux = tmp_aux;
	else
	    ann_template.aux = NULL;
	break;
    }
}

static char *mstr[54];

static void create_mstr_array()
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
    mstr[ACMAX+4] = ";    (Reference mark)";
}

/* Set up popup window for defining an annotation template. */
static void create_ann_template_popup()
{
    Icon icon;

    create_mstr_array();
    ann_template.anntyp = 1;
    icon = xv_create(XV_NULL, ICON,
		     ICON_IMAGE, icon_image,
		     ICON_LABEL, "Template",
		     NULL);
    ann_frame = xv_create(frame, FRAME,
			  XV_LABEL, "Annotation Template",
			  FRAME_ICON, icon, 0);
    ann_panel = xv_create(ann_frame, PANEL, 0);
    anntyp_item = xv_create(ann_panel, PANEL_CHOICE,
			    PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			    PANEL_LABEL_STRING, "Type: ",
			    XV_HELP_DATA, "wave:annot.type",
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
			     mstr[50], mstr[51], mstr[52], mstr[53],
			     NULL,
			    PANEL_VALUE, 1,
			    PANEL_NOTIFY_PROC, ann_select,
			    PANEL_CLIENT_DATA, (caddr_t) 't',
			    0);
    aux_item    = xv_create(ann_panel, PANEL_TEXT,
			    PANEL_LABEL_STRING, "Text: ",
			    XV_HELP_DATA, "wave:annot.text",
			    PANEL_VALUE_DISPLAY_LENGTH, 20,
			    PANEL_VALUE_STORED_LENGTH, 255,
			    PANEL_VALUE, "",
			    PANEL_NOTIFY_PROC, ann_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'a',
			    0);
    subtyp_item = xv_create(ann_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "Subtype: ",
			    XV_HELP_DATA, "wave:annot.subtype",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, ann_select,
			    PANEL_CLIENT_DATA, (caddr_t) 's',
			    0);
    chan_item   = xv_create(ann_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "`Chan' field: ",
			    XV_HELP_DATA, "wave:annot.chan",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, ann_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'c',
			    0);
    num_item    = xv_create(ann_panel, PANEL_NUMERIC_TEXT,
			    PANEL_LABEL_STRING, "`Num' field: ",
			    XV_HELP_DATA, "wave:annot.num",
			    PANEL_VALUE, 0,
			    PANEL_MIN_VALUE, -128,
			    PANEL_MAX_VALUE, 127,
			    PANEL_NOTIFY_PROC, ann_select,
			    PANEL_CLIENT_DATA, (caddr_t) 'n',
			    0);
    xv_create(ann_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Change all in range",
	      XV_HELP_DATA, "wave:annot.change",
	      PANEL_NOTIFY_PROC, change_annotations,
	      0);
    xv_create(ann_panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, "Dismiss",
	      XV_HELP_DATA, "wave:annot.dismiss",
	      PANEL_NOTIFY_PROC, dismiss_ann_template,
	      0);
    window_fit(ann_panel);
    window_fit(ann_frame);
}

/* Make the annotation template popup window appear. */
void show_ann_template()
{
    if (ann_popup_active < 0) create_ann_template_popup();
    wmgr_top(ann_frame);
    xv_set(ann_frame, WIN_MAP, TRUE, 0);
    ann_popup_active = 1;
}

/* Set the annotation template window `Type' item. */
void set_anntyp(i)
int i;
{
    if (ann_popup_active < 0) create_ann_template_popup();
    xv_set(anntyp_item, PANEL_VALUE, i, NULL);
}

/* Set the annotation template window `Aux' item. */
void set_ann_aux(s)
char *s;
{
    if (ann_popup_active >= 0)
	xv_set(aux_item, PANEL_VALUE, s ? s+1 : "", NULL);
}

/* Set the annotation template window `Subtype' item. */
void set_ann_subtyp(i)
int i;
{
    if (ann_popup_active >= 0)
	xv_set(subtyp_item, PANEL_VALUE, i, NULL);
}

/* Set the annotation template window `Chan' item. */
void set_ann_chan(i)
int i;
{
    if (ann_popup_active >= 0)
	xv_set(chan_item, PANEL_VALUE, i, NULL);
}

/* Set the annotation template window `Num' item. */
void set_ann_num(i)
int i;
{
    if (ann_popup_active >= 0)
	xv_set(num_item, PANEL_VALUE, i, NULL);
}
