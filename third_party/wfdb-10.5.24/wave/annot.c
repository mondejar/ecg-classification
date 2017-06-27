/* file: annot.c	G. Moody	  1 May 1990
			Last revised:    22 June 2010
Annotation list handling and display functions for WAVE

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
#include <sys/time.h>
#include <wfdb/ecgmap.h>
#include <xview/defaults.h>
#include <xview/notice.h>

/* The ANSI C function strstr is defined here for those systems which don't
   include it in their libraries.  This includes all older (pre-ANSI) C
   libraries;  some modern non-ANSI C libraries (notably those supplied with
   SunOS 4.1) do have strstr, so we can't just make this conditional on
   __STDC__. */
#ifdef sun
#ifdef i386
#define NOSTRSTR
#endif
#endif

#ifdef NOSTRSTR
char *strstr(s1, s2)
char *s1, *s2;
{
    char *p = s1;
    int n;

    if (s1 == NULL || s2 == NULL || *s2 == '\0') return (s1);
    n = strlen(s2);
    while ((p = strchr(p, *s2)) && strncmp(p, s2, n))
	p++;
    return (p);
}
#endif

void set_frame_title();

struct ap *get_ap()
{
    struct ap *a;

    if ((a = (struct ap *)malloc(sizeof(struct ap))) == NULL) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		      NOTICE_MESSAGE_STRINGS,
		      "Error in allocating memory for annotations\n", 0,
		      NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
    }
    return (a);
}

int annotations;	/* non-zero if there are annotations to be shown */
time_t tupdate;		/* time of last update to annotation file */

/* Annot_init() (re)opens annotation file(s) for the current record, and
   reads the annotations into memory.  The function returns 0 if no annotations
   can be read, 1 if some annotations were read but memory was exhausted, or 2
   if all of the annotations were read successfully.  On return, ap_start and
   annp both point to the first (earliest) annotation, ap_end points to the
   last one, and attached and scope_annp are reset to NULL. */

annot_init()
{
    char *p;
    struct ap *a;

    /* If any annotation editing has been performed, bring the output file
       up-to-date. */
    if (post_changes() == 0)
	return (0);	/* do nothing if the update failed (the user may be
			   able to recover by changing permissions or
			   clearing file space as needed) */

    /* Reset pointers. */
    attached = scope_annp = NULL;

    /* Free any memory that was previously allocated for annotations.
       This might take a while ... */
    if (frame) xv_set(frame, FRAME_BUSY, TRUE, NULL);
    while (ap_end) {
	a = ap_end->previous;
	if (ap_end->this.aux) free(ap_end->this.aux);
	free(ap_end);
	ap_end = a;
    }

    /* Check that the annotator name, if any, is legal. */
    if (nann > 0 && badname(af.name)) {
	char ts[ANLMAX+3];
	int dummy = (int)sprintf(ts, "`%s'", af.name);

#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
			    NOTICE_MESSAGE_STRINGS,
			    "The annotator name:",
			    ts,
			    "cannot be used.  Press `Continue', then",
			    "select an annotator name containing only",
			    "letters, digits, tildes, and underscores.", 0,
			    NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	af.name = NULL;
	annotator[0] = '\0';
	set_annot_item("");
	set_frame_title();
	return (annotations = 0);
    }

    /* Reset the frame title. */
    set_frame_title();

    /* Set time of last update to current time. */
    tupdate = time((time_t *)NULL);

    /* Return 0 if no annotations are requested or available. */
    if (getgvmode() & WFDB_HIGHRES) setafreq(freq);
    else setafreq(0.);
    if (nann < 1 || annopen(record, &af, 1) < 0) {
	ap_start = annp = scope_annp = NULL;
	if (frame) xv_set(frame, FRAME_BUSY, FALSE, NULL);
	return (annotations = 0);
    }
    if (getgvmode() & WFDB_HIGHRES) setafreq(freq);
    else setafreq(0.);
    if ((ap_start = annp = scope_annp = a = get_ap()) == NULL ||
	getann(0, &(a->this))) {
	(void)annopen(record, NULL, 0);
	if (frame) xv_set(frame, FRAME_BUSY, FALSE, NULL);
	return (annotations = 0);
    }

    /* Read annotations into memory, constructing a doubly-linked list on the
       fly. */
    do {
	a->next = NULL;
	a->previous = ap_end;
	if (ap_end) ap_end->next = a;
	ap_end = a;
	/* Copy the aux string, if any (since the aux pointer returned by
	   getann points to static memory that may be overwritten).  Return 1
	   if we run out of memory. */
	if (a->this.aux) {
	    if ((p = (char *)calloc(*(a->this.aux)+2, 1)) == NULL) {
		if (frame)
		    xv_set(frame, FRAME_BUSY, FALSE, NULL);
		if (frame) {
#ifdef NOTICE
		    Xv_notice notice = xv_create((Frame)frame, NOTICE,
						 XV_SHOW, TRUE,
#else
		    (void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		      NOTICE_MESSAGE_STRINGS,
		      "Error in allocating memory for aux string\n", 0,
		      NOTICE_BUTTON_YES, "Continue", 0);
#ifdef NOTICE
		    xv_destroy_safe(notice);
#endif
		}
		return (annotations = 1);
	    }
	    memcpy(p, a->this.aux, *(a->this.aux)+1);
	    a->this.aux = p;
	}
    } while ((a = get_ap()) && getann(0, &(a->this)) == 0);

    /* Return 1 if we ran out of memory while reading the annotation file. */
    if (a == NULL) {
	if (frame) xv_set(frame, FRAME_BUSY, FALSE, NULL);
	return (annotations = 1);
    }

    /* Return 2 if the entire annotation file has been read successfully. */
    else {
	free(a);	/* release last (unneeded) ap structure */
	if (frame) xv_set(frame, FRAME_BUSY, FALSE, NULL);
	return (annotations = 2);
    }
}

/* next_match() returns the time of the next annotation (i.e., the one later
   than and closest to those currently displayed) that matches the template
   annotation.  The mask specifies which fields must match. */
long next_match(template, mask)
struct WFDB_ann *template;
int mask;
{
    if (annotations && annp) {
	/* annp might be NULL if the annotation list is empty, or if the
	   last annotation occurs before display_start_time;  in either
	   case, next_match() returns -1. */
	do {
	    if (mask&M_ANNTYP) {
		if (template->anntyp) {
		    if (template->anntyp != annp->this.anntyp)
			continue;
		}
		else if ((annp->this.anntyp & 0x80) == 0)
		    continue;
	    }
	    if ((mask&M_SUBTYP) && template->subtyp != annp->this.subtyp)
		continue;
	    if ((mask&M_CHAN)   && template->chan   != annp->this.chan  )
		continue;
	    if ((mask&M_NUM)    && template->num    != annp->this.num   )
		continue;
	    if ((mask&M_AUX)    && (annp->this.aux == NULL ||
			  strstr(annp->this.aux+1, template->aux+1) == NULL))
		continue;
	    if ((mask&M_MAP2)&&template->anntyp!=map2(annp->this.anntyp))
		continue;
	    if (annp->this.time < display_start_time + nsamp)
		continue;
	    return (annp->this.time);
	} while (annp->next, annp = annp->next);
    }
    return (-1L);
}

/* previous_match() returns the time of the previous annotation (i.e., the one
   earlier than and closest to those currently displayed) that matches the
   template annotation.  The mask specifies which fields must match. */
long previous_match(template, mask)
struct WFDB_ann *template;
int mask;
{
    if (annotations) {
	/* annp might be NULL if the annotation list is empty, or if the
	   last annotation occurs before display_start_time.  In the first
	   case, previous_match() returns -1;  in the second case, it begins
	   its search with the last annotation in the list. */
	if (annp == NULL) {
	    if (ap_end && ap_end->this.time < display_start_time)
		annp = ap_end;
	    else
		return (-1L);
	}
	do {
	    if (mask&M_ANNTYP) {
		if (template->anntyp) {
		    if (template->anntyp != annp->this.anntyp)
			continue;
		}
		else if ((annp->this.anntyp & 0x80) == 0)
		    continue;
	    }
	    if ((mask&M_SUBTYP) && template->subtyp != annp->this.subtyp)
		continue;
	    if ((mask&M_CHAN)   && template->chan   != annp->this.chan  )
		continue;
	    if ((mask&M_NUM)    && template->num    != annp->this.num   )
		continue;
	    if ((mask&M_AUX)    && (annp->this.aux == NULL ||
			  strstr(annp->this.aux+1, template->aux+1) == NULL))
		continue;
	    if ((mask&M_MAP2)&&template->anntyp!=map2(annp->this.anntyp))
		continue;
	    if (annp->this.time > display_start_time)
		continue;
	    return (annp->this.time);
	} while (annp->previous, annp = annp->previous);
    }
    return (-1L);
}

/* Show_annotations() displays annotations between times left and left+dt at
appropriate x-locations in the ECG display area. */
void show_annotations(left, dt)
long left;
int dt;
{
    char buf[5], *p;
    int n, s, x, y, ytop, xs = -1, ys;
    long t, right = left + dt;

    if (annotations == 0) return;

    /* Find the first annotation to be displayed. */
    (void)locate_annotation(left, -128);  /* -128 is the lowest chan value */
    if (annp == NULL) return;

    /* Display all of the annotations in the window. */
    while (annp->this.time < right) {
	x = (int)((annp->this.time - left)*tscale);
	if (annp->this.anntyp & 0x80) {
	    y = ytop = abase; p = ".";
	}
	else switch (annp->this.anntyp) {
	  case NOTQRS:
	    y = ytop = abase; p = "."; break;
	  case NOISE:
	    y = ytop = abase - linesp;
	    if (annp->this.subtyp == -1) { p = "U"; break; }
	    /* The existing scheme is good for up to 4 signals;  it can be
	       easily extended to 8 or 12 signals using the chan and num
	       fields, or to an arbitrary number of signals using `aux'. */
	    for (s = 0; s < nsig && s < 4; s++) {
		if (annp->this.subtyp & (0x10 << s))
		    buf[s] = 'u';	/* signal s is unreadable */
		else if (annp->this.subtyp & (0x01 << s))
		    buf[s] = 'n';	/* signal s is noisy */
		else
		    buf[s] = 'c';	/* signal s is clean */
	    }
	    buf[s] = '\0';
	    p = buf; break;
	  case STCH:
	  case TCH:
	  case NOTE:
	    y = ytop = abase - linesp;
	    if (!show_aux && annp->this.aux) p = annp->this.aux+1;
	    else p = annstr(annp->this.anntyp);
	    break;
	  case LINK:
	    y = ytop = abase - linesp;
	    if (!show_aux && annp->this.aux) {
		char *p1 = annp->this.aux + 1, *p2 = p1 + *(p1-1);
		p = p1;
		while (p1 < p2) {
		    if (*p1 == ' ' || *p1 == '\t') {
			p = p1 + 1;
			break;
		    }
		    p1++;
		}
	    }	
	    break;		
	  case RHYTHM:
	    y = ytop = abase + linesp;
	    if (!show_aux && annp->this.aux) p = annp->this.aux+1;
	    else p = annstr(annp->this.anntyp);
	    break;
	  case INDEX_MARK:
	    y = ytop = abase - linesp;
	    p = ":";
	    break;
	  case BEGIN_ANALYSIS:
	    y = ytop = abase - linesp;
	    p = "<";
	    break;
	  case END_ANALYSIS:
	    y = ytop = abase - linesp;
	    p = ">";
	    break;
	  case REF_MARK:
	    y = ytop = abase - linesp;
	    p = ";";
	    break;
	  default:
	    y = ytop = abase; p = annstr(annp->this.anntyp); break;
	}
	if (ann_mode == 2 && y == abase) {
	    int yy = y + annp->this.num*vscalea;

	    if (xs >= 0)
		XDrawLine(display, osb, draw_ann, xs, ys, x, yy);
	    xs = x;
	    ys = yy;
	}
	else {
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		if (sig_mode == 0)
		    y = ytop +=
			base[(unsigned)annp->this.chan] - abase + mmy(2);
		else {
		    int i;

		    for (i = 0; i < siglistlen; i++)
			if (annp->this.chan == siglist[i]) {
			    y = ytop += base[i] - abase + mmy(2);
			    break;
			}
		}
	    }

	    n = strlen(p);
	    if (n > 3 && !overlap && annp->next &&
		(annp->next)->this.time < right) {
	        int maxwidth;

		maxwidth = (int)(((annp->next)->this.time-annp->this.time)*
				 tscale)
		          - XTextWidth(font, " ", 1);

	        while (n > 3 && XTextWidth(font, p, n) > maxwidth)
		    n--;
	    }
	    XDrawString(display, osb,
			annp->this.anntyp == LINK ? draw_sig : draw_ann,
			x, y, p, n);

	    if (annp->this.anntyp == LINK) {
		int xx = x + XTextWidth(font, p, n), yy = y + linesp/4;

		XDrawLine(display, osb, draw_sig, x, yy, xx, yy);
	    }
	
	    if (show_subtype) {
		sprintf(buf, "%d", annp->this.subtyp); p = buf; y += linesp;
		XDrawString(display, osb, draw_ann, x, y, p, strlen(p));
	    }
	    if (show_chan) {
		sprintf(buf, "%d", annp->this.chan); p = buf; y += linesp;
		XDrawString(display, osb, draw_ann, x, y, p, strlen(p));
	    }
	    if (show_num) {
		sprintf(buf, "%d", annp->this.num); p = buf; y += linesp;
		XDrawString(display, osb, draw_ann, x, y, p, strlen(p));
	    }
	    if (show_aux && annp->this.aux != NULL) {
		p = annp->this.aux + 1; y += linesp;
		XDrawString(display, osb, draw_ann, x, y, p, strlen(p));
	    }
	}
	if (show_marker && annp->this.anntyp != NOTQRS) {
	    XSegment marker[2];

	    marker[0].x1 = marker[0].x2 = marker[1].x1 = marker[1].x2 = x;
	    if (ann_mode == 1 && (unsigned)annp->this.chan < nsig) {
		unsigned int c = (unsigned)annp->this.chan;

		if (sig_mode == 1) {
		    int i;

		    for (i = 0; i < siglistlen; i++)
			if (c == siglist[i]) {
			    c = i;
			    break;
			}
		    if (i == siglistlen) {
			marker[0].y1 = 0;
			marker[1].y2 = canvas_height;
		    }
		    else {
			marker[0].y1 = (c == 0) ? 0 : (base[c-1] + base[c])/2;
			marker[1].y2 = (c == nsig-1) ? canvas_height :
			    (base[c+1] + base[c])/2;
		    }
		}
		else {
		    marker[0].y1 = (c == 0) ? 0 : (base[c-1] + base[c])/2;
		    marker[1].y2 = (c == nsig-1) ? canvas_height :
			(base[c+1] + base[c])/2;
		}
	    }
	    else {
		marker[0].y1 = 0;
		marker[1].y2 = canvas_height;
	    }
	    marker[0].y2 = ytop - linesp;
	    marker[1].y1 = y + mmy(2);
	    XDrawSegments(display, osb, draw_ann, marker, 2);
	}
	if (annp->next == NULL) break;
	annp = annp->next;
    
    }
}

void clear_annotation_display()
{
    if (ann_mode == 1 || (use_overlays && show_marker)) {
	XFillRectangle(display, osb, clear_ann,
		       0, 0, canvas_width+mmx(10), canvas_height);
	if (!use_overlays)
	    do_disp();
    }
    else
	XFillRectangle(display, osb, clear_ann,
		       0, abase-mmy(8), canvas_width+mmx(10), mmy(13));
}

/* This function locates an annotation at time t, attached to signal s, in the
   annotation list.  If there is no such annotation, it returns NULL, and annp
   points to the annotation that follows t (annp is NULL if no annotations
   follow t).  If there is an annotation at time t, the function sets annp to
   point to the first such annotation, and returns the value of annp.  (More
   than one annotation may exist at time t;  if so, on return, annp points to
   the one with the lowest `chan' field than is no less than s.)
 */

struct ap *locate_annotation(t, s)
long t;
int s;
{
    /* First, find out which of ap_start, annp, and ap_end is nearest t. */
    if (annp == NULL) {
	if (ap_start == NULL) return (NULL);
	annp = ap_start;
    }
    if (annp->this.time < t) {
	if (ap_end == NULL || ap_end->this.time < t)
	    /* no annotations follow t */
	    return (annp = NULL);
	if (t - annp->this.time > ap_end->this.time - t)
	    annp = ap_end;	/* closer to end than to previous position */
    }
    else {
	if (t < ap_start->this.time) {	/* no annotations precede t */
	    annp = ap_start;
	    return (NULL);
	}
	if (t - ap_start->this.time < annp->this.time - t)
	    annp = ap_start;
    }

    /* Traverse the list to find the annotation at time t and signal s, or its
       successor. */
    while (annp->this.time >= t && annp->previous)
	annp = annp->previous;
    while ((annp && annp->this.time < t) ||
	   (annp->this.time == t && annp->this.chan < s))
	annp = annp->next;
    if (annp == NULL || annp->this.time != t || annp->this.chan != s)
	return (NULL);
    else
	return (annp);
}

int changes;	/* number of edits since last save */

void check_post_update()
{
    if (++changes >= 20 || time((time_t *)NULL) > tupdate+60)
	(void)post_changes();
    if (changes < 2)
	set_frame_title();
}

/* This function deletes the annotation at time t and attached to signal s,
   if there is one.  It leaves behind a "phantom" annotation, which is
   displayed by this program as a ".";  this makes it relatively simple to
   replace the deleted annotation at the same point if it was deleted in error.
   "Phantom" annotations are not written to the output annotation file.

   If an attempt is made to delete a "phantom" annotation, the effect is to
   undelete it (i.e., its original state is restored).

   If there is a marker (INDEX_MARK, BEGIN_ANALYSIS, or END_ANALYSIS) at t,
   this function deletes the marker without leaving a "phantom" annotation.
 */
void delete_annotation(t, s)
long t;
int s;
{
    if (locate_annotation(t, s)) {
	if (annp->this.anntyp <= ACMAX) {	/* not a marker */
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
		return;
	    }
	    annp->this.anntyp ^= 0x80;		/* MSB of anntyp is "phantom"
						   marker */
	    check_post_update();
	}
	else {					/* marker to be deleted */
	    struct ap *a = annp;

	    switch (a->this.anntyp) {
	      case BEGIN_ANALYSIS:
		begin_analysis_time = -1L; reset_start(); break;
	      case END_ANALYSIS:
		end_analysis_time = -1L; reset_stop(); break;
	      default: break;
	    }
	    /* Remove the annotation from the list by reconstructing the
	       links around it, free its memory allocation, and reset annp. */
	    if (a->previous) (a->previous)->next = a->next;
	    else ap_start = a->next;
	    if (a->next) {
		annp = a->next;
		(a->next)->previous = a->previous;
	    }
	    else annp = ap_end = a->previous;
	    if (a->this.aux) free(a->this.aux);
	    free(a);
	}
    }
}

/* This function moves the annotation pointed to by `a' to a new time `t'.
   Note that the links from `a' must be valid or NULL when this function is
   called. */
void move_annotation(a, t)
struct ap *a;
long t;
{
    if (a->this.anntyp <= ACMAX && accept_edit == 0) {
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
	return;
    }
    /* Remove the annotation from the list by reconstructing the links
       around it. */
    if (a->previous) (a->previous)->next = a->next;
    else ap_start = a->next;
    if (a->next) (a->next)->previous = a->previous;
    else ap_end = a->previous;

    /* Adjust the time. */
    a->this.time = t;

    /* Reinsert the annotation into the list.  (Make sure that annp doesn't
       point to `a' first.) */
    annp = a->next;
    (void)insert_annotation(a);
}

/* This function is used by insert_annotation (below) to insert the annotation
   pointed to by `a' into the annotation list before the annotation pointed to
   by `annp'. */
static void do_insertion(a)
struct ap *a;
{
    if (annp == NULL) {		/* append to end of annotation list */
	a->next = NULL;
	a->previous = ap_end;
	if (ap_end == NULL) {	/* annotation list is empty */
	    ap_start = annp = ap_end = a; annotations = 1; }
	else {
	    ap_end->next = a;
	    annp = ap_end = a;
	}
    }
    else {			/* insert before annp */
	a->next = annp;    
	a->previous = annp->previous;
	annp->previous = a;
	if (a->previous) (a->previous)->next = a;
	else ap_start = a;
    }
}

/* This function inserts the annotation pointed to by `a' into the annotation
   list.  If there is already an annotation at time a->time, with the same
   `chan' field, it is overwritten by the inserted annotation. */
void insert_annotation(a)
struct ap *a;
{
    if (accept_edit == 0 && a->this.anntyp <= ACMAX) {
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
	return;
    }
    if (locate_annotation(a->this.time, a->this.chan)) {
	/* annp points to an existing annotation that must be replaced. */
	if (accept_edit || annp->this.anntyp > ACMAX) {
	    /* overwrite annotation or marker at annp */
	    if (annp->this.aux) free(annp->this.aux);
	    annp->this = a->this;
	}
	else {
	    /* If we get here, we must be inserting a marker, not a real
	       annotation.  In this case, we can't overwrite the real
	       annotation, so we change the marker so that it points to
	       the next sample. */
	    a->this.time++;
	    insert_annotation(a);
	    return;
	}
    }
    else
	do_insertion(a);
    if (a->this.anntyp <= ACMAX)
	check_post_update();
    else {
	switch (a->this.anntyp) {
	  case BEGIN_ANALYSIS:
	    if (begin_analysis_time >= 0L &&
		begin_analysis_time != a->this.time)
		delete_annotation(begin_analysis_time, -1);
	    begin_analysis_time = a->this.time;
	    a->this.chan = -1;
	    reset_start();
	    break;
	  case END_ANALYSIS:
	    if (end_analysis_time >= 0L &&
		end_analysis_time != a->this.time)
		delete_annotation(end_analysis_time, -1);
	    end_analysis_time = a->this.time;
	    a->this.chan = -1;
	    reset_stop();
	    break;
	  case REF_MARK:
	    if (ref_mark_time >= 0L &&
		ref_mark_time != a->this.time)
		delete_annotation(ref_mark_time, -1);
	    ref_mark_time = a->this.time;
	    a->this.chan = -1;
	    reset_ref();
	    break;
	  default:
	    break;
	}
    }
}

/* This function changes all annotations between begin_analysis_time and
   end_analysis_time to that specified by the annotation template, after asking
   for confirmation if doing so would affect annotations not currently
   visible.  If the annotation template's anntyp is NOTQRS, then previously
   deleted annotations within the range are undeleted, and all other
   annotations within the range are deleted.  Any markers within the range are
   unaffected.
 */
void change_annotations()
{
    struct ap *a;

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
	return;
    }
    if (begin_analysis_time == -1L || end_analysis_time == -1L ||
	begin_analysis_time >= end_analysis_time) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		    NOTICE_MESSAGE_STRINGS,
		    "You must specify a range by inserting `<' and `>'",
		    "markers (or by setting the Start and End times",
		    "in the Analyze panel).", 0,
		     NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	return;
    }
    if (ann_template.anntyp == BEGIN_ANALYSIS ||
	ann_template.anntyp == END_ANALYSIS) {
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
#else
	(void)notice_prompt((Frame)frame, (Event *)NULL,
#endif
		    NOTICE_MESSAGE_STRINGS,
		    "Select a different annotation type.", 0,
		     NOTICE_BUTTON_YES, "Continue", NULL);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	return;
    }
    if (begin_analysis_time < display_start_time ||
	end_analysis_time > display_start_time + nsamp) {
	int result;

#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
				     NOTICE_STATUS, &result,
#else
	result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
		    NOTICE_MESSAGE_STRINGS,
		    "The range of this action is not limited to visible",
		    "annotations.  Press `Cancel' to continue without",
		    "changing any annotations, or `Confirm' to make",
		    "changes anyway.", 0,
		    NOTICE_BUTTON_YES, "Cancel",
		    NOTICE_BUTTON_NO, "Confirm", NULL);
#ifdef NOTICE
	xv_destroy_safe(notice);
#endif
	if (result == NOTICE_YES) return;
    }
    (void)locate_annotation(begin_analysis_time, -128);
    a = annp;
    while (a && a->this.time < end_analysis_time) {
	if (a->this.anntyp <= ACMAX) {		/* leave markers alone */
	    if (ann_template.anntyp == NOTQRS)
		a->this.anntyp ^= 0x80;		/* see delete_annotation() */
	    else {
		long t = a->this.time;
		a->this = ann_template;
		a->this.time = t;
	    }
	    ++changes;
	}
	a = a->next;
    }
    check_post_update();
    set_frame_title();
    clear_annotation_display();
    show_annotations(display_start_time, nsamp);
}


/* This function writes the current contents of the annotation list to an
   annotation file, if any changes have been made since the previous call
   (or since the annotation list was initialized).  The name of the output
   file is the same as that of the input file, but it is always written to
   the current directory (even if explicit path information was provided in
   the input annotator name).  If no annotator name was specified, the name
   by which this program was invoked (normally `wave') is used for this
   purpose.

   This function attempts to ensure that the input file is not overwritten.
   If the current directory already contains a file with the name to be used
   for the output file, the existing file is first renamed by appending a "~"
   to the annotator name (unless this was done during a previous invocation
   of this function and the record and annotator names have not been changed
   since).  Only one level of backup is preserved, so you will overwrite the
   original annotation file if it is in the current directory and you open the
   same annotator more than once.

   If the "save" operation succeeds, the function returns 1;  if it fails for
   any reason, it returns 0.
 */

int post_changes()
{
    int result;
    struct ap *a;

    if (changes <= 0) return (1);

    /* If there was no annotator name specified, use the name by which this
       program was invoked for this purpose. */
    if (af.name == NULL) {
	af.name = pname;
	strcpy(annotator, pname);
	set_annot_item(pname);
    }

    if (savebackup) {
	char afname[ANLMAX+RNLMAX+2], afbackup[ANLMAX+RNLMAX+2];
	FILE *tfile;

	/* Generate a name for the updated annotation file. */
	sprintf(afname, "%s.%s", record, af.name);

	/* If the file already exists in the current directory, rename it. */
	if (tfile = fopen(afname, "r")) {
	    fclose(tfile);		/* yes -- try to do so */

	    /* Generate a name for a backup file by appending a `~'. */
	    sprintf(afbackup, "%s~", afname);
	    if (rename(afname, afbackup)) {
#ifdef NOTICE
		Xv_notice notice = xv_create((Frame)frame, NOTICE,
					     XV_SHOW, TRUE,
					     NOTICE_STATUS, &result,
#else
		result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
			     NOTICE_MESSAGE_STRINGS,
			     "Your changes cannot be saved unless you remove,",
			     "or obtain permission to rename, the file named",
			     afname,
			     "in the current directory.",
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
		else {
		    finish_log();
		    xv_destroy_safe(frame);
		    exit(1);
		}
	    }
	}
	savebackup = 0;
    }

    af.stat = (af.stat == WFDB_AHA_READ) ? WFDB_AHA_WRITE : WFDB_WRITE;
    if (getgvmode() & WFDB_HIGHRES) setafreq(freq);
    else setafreq(0.);
    if (annopen(record, &af, 1)) {
	/* An error from annopen is most likely to result from not being able
	   to create the output file.  Warn the user and try again later. */
#ifdef NOTICE
	Xv_notice notice = xv_create((Frame)frame, NOTICE,
				     XV_SHOW, TRUE,
				     NOTICE_STATUS, &result,
#else
	result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
			       NOTICE_MESSAGE_STRINGS,
			       "Your changes cannot be saved until you obtain",
			       "write permission in the current directory.",
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
	else {
	    finish_log();
	    xv_destroy_safe(frame);
	    exit(1);
	}
    }
    af.stat = (af.stat == WFDB_AHA_WRITE) ? WFDB_AHA_READ : WFDB_READ;
    a = ap_start;

    /* Write the annotation list to the output file.  This might take a while
       .... */
    xv_set(frame, FRAME_BUSY, TRUE, NULL);
    while (a) {
	if (isann(a->this.anntyp) && putann(0, &(a->this))) {
	    /* An error from putann is most likely to be the result of file
	       space exhaustion.  Warn the user and try again later. */
#ifdef NOTICE
	    Xv_notice notice;
#endif
	    xv_set(frame, FRAME_BUSY, FALSE, NULL);
#ifdef NOTICE
	    notice = xv_create((Frame)frame, NOTICE,
			       XV_SHOW, TRUE,
			       NOTICE_STATUS, &result,
#else
	    result = notice_prompt((Frame)frame, (Event *)NULL,
#endif
			       NOTICE_MESSAGE_STRINGS,
			       "Your changes cannot be saved until additional",
			       "file space is made available.",
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
	    else {
		finish_log();
		xv_destroy_safe(frame);
		exit(1);
	    }
	}
	a = a->next;
    }
    if (getgvmode() & WFDB_HIGHRES) setafreq(freq);
    else setafreq(0.);
    (void)annopen(record, NULL, 0);	/* force flush and close of output */

    changes = 0;
    xv_set(frame, FRAME_BUSY, FALSE, NULL);

    /* Set time of last update to current time. */
    tupdate = time((time_t *)NULL);

    return (1);
}

/* Reset the base frame title. */
void set_frame_title()
{
    static char frame_title[7+RNLMAX+1+ANLMAX+2+DSLMAX+1];
         /* 7 for "Record ", RNLMAX for record name, 1 for " " or "(",
	    ANLMAX for annotator name, 2 for "  " or ") ",
	    DSLMAX for description from log file, 1 for null */

    if (annotator[0]) {
	if (changes)
	    sprintf(frame_title, "WAVE %s  Record %s(%s) ", WAVEVERSION,
		    record, annotator);
	else
	    sprintf(frame_title, "WAVE %s  Record %s %s  ", WAVEVERSION,
		    record, annotator);
    }
    else
	sprintf(frame_title, "Record %s ", record);
    if (description[0] != '\0')
	strcat(frame_title, description);
    xv_set(frame, FRAME_LABEL, frame_title, NULL);
}

/* Reset the base frame footer. */
void set_frame_footer()
{
    /* Keep this in sync with grid choice menu in modepan.c! */
    static char *grid_desc[7] = { "",
				 "Grid intervals: 0.2 sec",
				 "Grid intervals: 0.5 mV",
				 "Grid intervals: 0.2 sec x 0.5 mV",
				 "Grid intervals: 0.04 sec x 0.1 mV",
				 "Grid intervals: 1 min x 0.5 mV",
				 "Grid intervals: 1 min x 0.1 mV" };

    xv_set(frame, FRAME_RIGHT_FOOTER, grid_desc[grid_mode], NULL);
    if (attached && (attached->this).aux &&
	display_start_time <= (attached->this).time &&
	(attached->this).time < display_start_time + nsamp)
        xv_set(frame, FRAME_LEFT_FOOTER, attached->this.aux + 1, NULL);
    else if (time_mode) {
	int tm_save = time_mode;

	time_mode = 0;
        xv_set(frame, FRAME_LEFT_FOOTER, wtimstr(display_start_time), NULL);
	time_mode = tm_save;
    }
    else {
	char *p = timstr(-display_start_time);

	if (*p == '[') {
	   time_mode = 1;
	   xv_set(frame, FRAME_LEFT_FOOTER, wtimstr(display_start_time), NULL);
	   time_mode = 0;
	}
	else
            xv_set(frame, FRAME_LEFT_FOOTER, "", NULL);
    }
}

    
/* Return 1 if p[] would not be a legal annotator name, 0 otherwise. */
int badname(p)
char *p;
{
    char *pb;

    if (p == NULL || *p == '\0')
	return (1);	/* empty name is illegal */
    for (pb = p + strlen(p) - 1; pb >= p; pb--) {
	if (('a' <= *pb && *pb <= 'z') || ('A' <= *pb && *pb <= 'Z') ||
	    ('0' <= *pb && *pb <= '9') || *pb == '~' || *pb == '_')
	    continue;	/* legal character */
	else if (*pb == '/')
	    return (0);	/* we don't need to check directory names */
	else
	    return (1);	/* illegal character */
    }
    return (0);
}

int read_anntab()
{
    char *atfname, buf[256], *p1, *p2, *s1, *s2, *getenv(), *strtok();
    int a;
    FILE *atfile;

    if ((atfname =
	 defaults_get_string("wave.anntab","Wave.Anntab",getenv("ANNTAB"))) &&
	(atfile = fopen(atfname, "r"))) {
	while (fgets(buf, 256, atfile)) {
	    p1 = strtok(buf, " \t");
	    if (*p1 == '#') continue;
	    a = atoi(p1);
	    if (0 < a && a <= ACMAX && (p1 = strtok((char *)NULL, " \t"))) {
		p2 = p1 + strlen(p1) + 1;
	    if ((s1 = (char *)malloc(((unsigned)(strlen(p1) + 1)))) == NULL ||
		(*p2 &&
		 (s2 = (char *)malloc(((unsigned)(strlen(p2)+1)))) == NULL)) {
		wfdb_error("read_anntab: insufficient memory\n");
		return (-1);
	    }
	    (void)strcpy(s1, p1);
	    (void)setannstr(a, s1);
	    if (*p2) {
		(void)strcpy(s2, p2);
		(void)setanndesc(a, s2);
	    }
	    else
		(void)setanndesc(a, (char*)NULL);
	    }
	}
	(void)fclose(atfile);
	return (0);
    }
    else
	return (-1);
}

int write_anntab()
{
    char *atfname;
    FILE *atfile;
    int a;

    if ((atfname = getenv("ANNTAB")) &&
	(atfile = fopen(atfname, "w"))) {
	for (a = 1; a <= ACMAX; a++)
	    if (anndesc(a))
		(void)fprintf(atfile, "%d %s %s\n", a, annstr(a), anndesc(a));
	return (0);
    }
    else
	return (-1);
}    
