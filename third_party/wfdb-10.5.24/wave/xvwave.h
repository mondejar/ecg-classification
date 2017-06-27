/* file: xvwave.h    	G. Moody	27 April 1990
			Last revised:   10 June 2005
XView constants, macros, function prototypes, and global variables for WAVE

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1990-2005 George B. Moody

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

#include <X11/Xlib.h>		/* Xlib definitions */
#include <xview/xview.h>	/* XView definitions */
#include <xview/panel.h>
#include <xview/svrimage.h>

/* Default screen resolution.  All known X servers seem to know something about
   the screen size, even if what they know isn't true.  Just in case, these
   constants define default values to be used if Height/WidthMMOfScreen give
   non-positive results.  If your screen doesn't have square pixels, define
   DPMMX and DPMMY differently.  Note that these definitions can be made using
   -D... options on the C compiler's command line.

   If your X server is misinformed about your screen size, there are three
   possible remedies.  All of the X11R4 sample servers from MIT accept a `-dpi'
   option that allows you to override the server default at the time the
   server is started.  See the X man page for details.  Alternatively, use
   WAVE's `-dpi' option or `dpi' resource (less desirable solutions, since they
   don't solve similar problems that other applications are likely to
   encounter). */
#ifndef DPMM
#define DPMM	4.0	/* default screen resolution (pixels per millimeter) */
#endif
#ifndef DPMMX
#define DPMMX	DPMM
#endif
#ifndef DPMMY
#define DPMMY	DPMM
#endif

/* Annotations are displayed in the font specified by DEFANNFONT if no
   other choice is specified.  At present, there is no other way to specify
   a choice.  Ultimately, this should be done by reading a resource file. */
#ifndef DEFANNFONT
#define DEFANNFONT	"fixed"
#endif

/* The following macro is used to determine the type of display available to
   this program (color, grey scale, monochrome, etc.).  The symbols __cplusplus
   and c_plusplus are defined by AT&T C++ and GNU G++;  the alternate
   definition is needed if WAVE is compiled by a C++ compiler, to avoid clashes
   with the C++ reserved word `class' (see X11/Xlib.h). */
#if defined(__cplusplus) || defined(c_plusplus)
#define ClassOfVisual(visual)	((visual)->c_class)
#else
#define ClassOfVisual(visual)	((visual)->class)
#endif

/* Globally visible X11 and XView objects. */
COMMON Frame frame;	/* base frame object */
COMMON Display *display;/* X server connection */
COMMON Server_image icon_image;	/* Icon bitmap for base and other frames */
COMMON Pixmap osb;      /* the offscreen buffer for the canvas */
COMMON XID xid;		/* Xlib "drawable" id for signal window */
COMMON XFontStruct *font;	/* Font used for text in signal window */

/* Graphics contexts.  For each displayed object (signal, annotation, cursor,
   and grid) there are drawing and erasing graphics contexts;  in addition,
   there is a `clear_all' GC for erasing everything at once.  If change_color
   is zero (i.e., if we have not installed a customized color map), all of the
   clear_* contexts are equivalent to clear_all, except that clear_crs is
   equivalent to draw_crs (which uses GXinvert for the drawing function in this
   case). */
COMMON GC draw_sig, clear_sig,
    draw_ann, clear_ann,
    draw_crs, clear_crs,
    draw_grd, draw_cgrd, clear_grd,
    highlight_sig, unhighlight_sig,
    clear_all;

COMMON int ann_popup_active;	/* <0: annotation template popup not created,
				    0: popup not visible, >0: popup visible */

/* Display lists

A display list contains all information needed to draw a screenful of signals.
For each of the nsig signals, a display list contains a list of the
(x, y) pixel coordinate pairs that specify the vertices of the polyline
that represents the signal.  

A cache of recently-used display lists (up to MAX_DISPLAY_LISTS of them) is
maintained as a singly-linked list;  the first display list is pointed
to by first_list.
*/

struct display_list {
    struct display_list *next;	/* link to next display list */
    long start;		/* time of first sample */
    int nsig;		/* number of signals */
    int npoints;	/* number of (input) points per signal */
    int ndpts;		/* number of (output) points per signal */
    int xmax;		/* largest x, expressed as window abscissa */
    int *sb;		/* signal baselines, expressed as window ordinates */
    XPoint **vlist;	/* vertex list pointers for each signal */
};
COMMON struct display_list *first_list;
COMMON Panel_item *level_name, *level_value, *level_units;  /* see edit.c */
COMMON XSegment *level;

/* Function prototypes for ANSI C and C++ compilers.  (Most of these are in
   wave.h;  those below are only for functions that use X or XView objects
   in their argument lists.) */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
extern Panel create_main_panel(void);		/* in mainpan.c */
extern void disp_proc(Panel_item i, Event *e);	/* in mainpan.c */
extern void window_event_proc(Xv_Window w,	/* in edit.c */
			      Event *e, Notify_arg a);
extern struct display_list *find_display_list(	/* in signal.c */
					      long time);

/* Function return types for K&R C compilers. */
#else
extern Panel create_main_panel();
extern void disp_proc(), window_event_proc();
extern struct display_list *find_display_list();
#endif
