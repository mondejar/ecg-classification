#ifdef __linux__
#define __COMPILE_XVHELP
#endif
#ifdef __COMPILE_XVHELP
#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)help.c 1.77 93/06/28";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE 
 *	file for terms of the license.
 */

#include <stdio.h>
#include <string.h>
#include <xview_private/i18n_impl.h>
#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/canvas.h>
#include <xview/cursor.h>
#include <xview/defaults.h>
#include <xview/notify.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include <xview/seln.h>
#include <xview/scrollbar.h>
#include <xview/textsw.h>
#include <xview_private/draw_impl.h>
#include <unistd.h>
#include <sys/types.h>

#include "mglass.xbm"
#include "mglass_mask.xbm"

int	help_notice_key;
extern char *xv_app_name;
extern wchar_t *xv_app_name_wcs;

/*
 * There is a maximum of 10 lines of text of 50 chars each visible in the
 * help text subwindow.  If the help text exceeds 10 lines, a scrollbar is
 * shown.
 */
#define HELPTEXTCOLS 50
#define HELPTEXTLINES 10
#define HELP_CANVAS_MARGIN 10
#define HELP_IMAGE_X 35
#define HELP_IMAGE_Y 5
#define HELP_IMAGE_WIDTH 80
#define HELP_IMAGE_HEIGHT 73
#define MORE_BUTTON_OFFSET 30

#define MAX_HELP_STRING_LENGTH 128
#define MAX_FILE_KEY_LENGTH 64

#define MORE_HELP_KEY 1


Xv_private void	    frame_set_rect();
Xv_private void	    screen_adjust_gc_color();

Pkg_private FILE   *xv_help_find_file();
Pkg_private int	    xv_help_get_arg();
Pkg_private char   *xv_help_get_text();

typedef struct {
     Xv_Cursor    busy_pointer;
     Frame	  help_frame;
     Server_image help_image;
     GC	    help_stencil_gc;
     Textsw	  help_textsw;
     Scrollbar    help_textsw_sb;
     Server_image mglass_image; /* magnifying glass only */
     Panel_item   mglass_msg;	/* magnifying glass Message item */
     Server_image mglass_stencil_image; /* magnifying glass stencil */
     Panel_item   more_help_button;
} Help_info;

int help_info_key;

/*ARGSUSED*/
static void
invoke_more_help(client_window, sys_command)
    Xv_Window	    client_window;
    char	   *sys_command;
{
    char	   *display_env;
    pid_t	    pid;


    /* Insure that More Help application comes up on same display as
     * client application.
     */
    display_env = defaults_get_string("server.name", "Server.Name", NULL);
    if (display_env) {
	char *p = malloc(strlen(display_env) + 9);
	sprintf(p, "DISPLAY=%s", display_env);
	putenv(p);
    }

    /* Invoke More Help application */
    switch ( pid = fork() ) {
    case -1:		/* error */
	xv_error ( 0,
		  ERROR_LAYER,	ERROR_SYSTEM,
		  ERROR_STRING,	XV_MSG("Help package:  cannot invoke More Help"),
		  NULL );
	break;
    case 0:		/* child */
	(void) execl("/bin/sh", "sh", "-c", sys_command, (char *)0);
	_exit(-1);
	break;
    default:		/* parent */
	/* reap child -- do nothing with it... */
	(void) notify_set_wait3_func(client_window, notify_default_wait3, pid);
	break;
    }
}


static void
more_help_proc(item, event)
    Panel_item item;
    Event *event;
{
    char	   *sys_command;

    sys_command = (char *) xv_get(item, XV_KEY_DATA, MORE_HELP_KEY);
    if (sys_command)
	invoke_more_help(event_window(event), sys_command);
}


static void
help_request_failed(window, data, str)
    Xv_Window       window;
    char           *data;	/* "file:key" */
    char	   *str;	/* explanation of why help request failed */
{
    char            message[256];
    Xv_Window	    notice_window;
    Xv_Notice	    help_notice;

    if (!help_notice_key)  {
	help_notice_key = xv_unique_key();
    }

    if (data)
	sprintf(message, XV_MSG("%s for %s."), str, data);
    else
	sprintf(message, XV_MSG("%s."), str);
    notice_window = xv_get(window, WIN_FRAME);
    if (!notice_window || !xv_get(notice_window, XV_IS_SUBTYPE_OF, FRAME_CLASS))  {
	/*
	 * No top level frame
	 * May be a menu, try using WIN_FRAME as key data
	 */
        notice_window = (Xv_Window)xv_get(window, XV_KEY_DATA, WIN_FRAME);

        if (!notice_window)  {
	    notice_window = window;   /* No frame: must be top level window */
        }
    }

    help_notice = (Xv_Notice)xv_get(notice_window, 
                                XV_KEY_DATA, help_notice_key, 
                                NULL);

    if (!help_notice)  {
        help_notice = xv_create(notice_window, NOTICE,
                        NOTICE_LOCK_SCREEN, FALSE,
			NOTICE_BLOCK_THREAD, TRUE,
                        NOTICE_MESSAGE_STRINGS,
			    message,
                        0,
                        NOTICE_BUTTON_YES, XV_MSG("OK"),
                        XV_SHOW, TRUE,
                        0);

        xv_set(notice_window, 
            XV_KEY_DATA, help_notice_key, help_notice,
            NULL);
    }
    else  {
        xv_set(help_notice, 
	    NOTICE_LOCK_SCREEN, FALSE,
	    NOTICE_BLOCK_THREAD, TRUE,
            NOTICE_MESSAGE_STRINGS,
                message,
            0,
            NOTICE_BUTTON_YES, XV_MSG("OK"),
            XV_SHOW, TRUE, 
            NULL);
    }
}


static          Notify_value
help_frame_destroy_proc(client, status)
    Notify_client   client;
    Destroy_status  status;
{
    Help_info	   *help_info;

    if (status == DESTROY_CLEANUP) {
	help_info = (Help_info *) xv_get(client, XV_KEY_DATA, help_info_key);
	if (help_info) {
	    if (help_info->help_image) {
		xv_destroy(help_info->help_image);
		help_info->help_image = NULL;
	    }
	    help_info->help_frame = NULL;
	}
    }
    return (notify_next_destroy_func(client, status));
}


Xv_private void
xv_help_save_image(pw, client_width, client_height, mouse_x, mouse_y)
    Xv_Window       pw;
    int             client_width, client_height;
    int             mouse_x, mouse_y;	/* offset of mouse pointer in pixwin */
{
    int		    dst_x, dst_y;
    int             src_x, src_y;
    Help_info	   *help_info;
    int             image_width, image_height;
    Xv_Drawable_info *info;
    Xv_Drawable_info *src_info;
    Xv_Screen       screen;
    GC             *gc_list;

    DRAWABLE_INFO_MACRO(pw, info);
    screen = xv_screen(info);
    gc_list = (GC *)xv_get(screen, SCREEN_OLGC_LIST, pw);
    screen_adjust_gc_color(pw, SCREEN_CLR_GC);
    screen_adjust_gc_color(pw, SCREEN_SET_GC);

    if (!help_info_key)
	help_info_key = xv_unique_key();
    help_info = (Help_info *) xv_get(screen, XV_KEY_DATA, help_info_key);
    if (!help_info) {
	help_info = xv_alloc(Help_info);
	xv_set(screen, XV_KEY_DATA, help_info_key, help_info, 0);
    }
    /* destroy the cached help_image if the depth no longer matches */
    if (help_info->help_image && 
	(xv_depth(info) != xv_get(help_info->help_image,SERVER_IMAGE_DEPTH))) {
	xv_destroy(help_info->help_image);
	help_info->help_image = NULL;
    }
    if (!help_info->help_image) {
	/* Create a server image for magnifying glass with help target image */
	help_info->help_image = xv_create(screen, SERVER_IMAGE,
			                  XV_WIDTH, mglass_width,
			                  XV_HEIGHT, mglass_height,
			                  SERVER_IMAGE_DEPTH, xv_depth(info),
			                  0);
    }

    /* Fill magnifying glass with client window's background color */
    DRAWABLE_INFO_MACRO(help_info->help_image, info);
    XFillRectangle(xv_display(info), xv_xid(info),
		   gc_list[SCREEN_CLR_GC],
		   HELP_IMAGE_X, HELP_IMAGE_Y,
		   HELP_IMAGE_WIDTH, HELP_IMAGE_HEIGHT);

    if (mouse_x < client_width && mouse_y < client_height) {
	/* Store copy of Frame Buffer pixels into magnifying glass circle */
	src_x = mouse_x - HELP_IMAGE_WIDTH / 2;
	if (src_x < 0)
	    src_x = 0;
	if (src_x + HELP_IMAGE_WIDTH > client_width) {
	    image_width = client_width - src_x;
	    dst_x = HELP_IMAGE_X + (HELP_IMAGE_WIDTH-image_width)/2;
	} else {
	    image_width = HELP_IMAGE_WIDTH;
	    dst_x = HELP_IMAGE_X;
	}
	src_y = mouse_y - HELP_IMAGE_HEIGHT / 2;
	if (src_y < 0)
	    src_y = 0;
	if (src_y + HELP_IMAGE_HEIGHT > client_height) {
	    image_height = client_height - src_y;
	    dst_y = HELP_IMAGE_Y + (HELP_IMAGE_HEIGHT-image_height)/2;
	} else {
	    image_height = HELP_IMAGE_HEIGHT;
	    dst_y = HELP_IMAGE_Y;
	}
	DRAWABLE_INFO_MACRO(pw, src_info);
	XCopyArea(xv_display(info), xv_xid(src_info), xv_xid(info),
		  gc_list[SCREEN_SET_GC],
		  src_x, src_y, image_width, image_height,
		  dst_x, dst_y);
    }
}


Xv_private int
xv_help_render(client_window, client_data, client_event)
    Xv_Window       client_window;
    caddr_t         client_data;
    Event          *client_event;
{
    char           *text;
    CHAR            client_name[80];
    XGCValues	    gc_values;
    int             i;
    Xv_Drawable_info *dst_info;	/* destination */
    int             length;
    Frame           client_frame;
    Xv_Cursor       current_pointer;
    Rect	    help_frame_rect;
    Help_info	   *help_info;
    Panel	    mglass_panel;	/* magnifying glass Panel */
    char	   *more_help_cmd;
    Panel	    more_help_panel;
    Xv_Window	    root_window;
    Xv_object       screen;
    Xv_object       server;
    Xv_Drawable_info *stencil_info;
    Xv_Drawable_info *src_info; /* source */
    Xv_Window	    textsw_view;
    CHAR	*application_name;


    if (xv_help_get_arg(client_data, &more_help_cmd) == XV_OK)
	text = xv_help_get_text();
    else
	text = NULL;
    if (!text) {
	help_request_failed(client_window, client_data, 
		XV_MSG("No help is available"));
	return XV_ERROR;
    }
    if (event_action(client_event) == ACTION_MORE_HELP ||
	event_action(client_event) == ACTION_MORE_TEXT_HELP) {
	if (more_help_cmd) {
	    invoke_more_help(client_window, more_help_cmd);
	    return XV_OK;
	} else {
	    help_request_failed(client_window, client_data,
		XV_MSG("More help is not available"));
	    return XV_ERROR;
	}
    }

    DRAWABLE_INFO_MACRO(client_window, src_info);
    screen = xv_screen(src_info);
    server = xv_server(src_info);

#ifdef OW_I18N
    if((application_name = (CHAR *)xv_get(server,XV_APP_NAME_WCS)) == NULL){
    	application_name =  wsdup(xv_app_name_wcs);
#else
    if((application_name = (CHAR *)xv_get(server,XV_APP_NAME)) == NULL){
    	application_name = xv_strsave(xv_app_name);
#endif
    }

    if (!help_info_key)
	help_info_key = xv_unique_key();
    help_info = (Help_info *) xv_get(screen, XV_KEY_DATA, help_info_key);
    if (!help_info) {
	help_info = xv_alloc(Help_info);
	xv_set(screen, XV_KEY_DATA, help_info_key, help_info, 0);
    }

    /* Change to busy pointer */
    if (!help_info->busy_pointer) {
	help_info->busy_pointer = xv_get(server, XV_KEY_DATA, CURSOR_BUSY_PTR);
	if (!help_info->busy_pointer) {
	    help_info->busy_pointer = xv_create(screen, CURSOR,
				     CURSOR_SRC_CHAR, OLC_BUSY_PTR,
				     CURSOR_MASK_CHAR, OLC_BUSY_MASK_PTR,
				     0);
	    xv_set(server,
		   XV_KEY_DATA, CURSOR_BUSY_PTR, help_info->busy_pointer,
		   0);
	}
    }
    current_pointer = xv_get(client_window, WIN_CURSOR);
    xv_set(client_window, WIN_CURSOR, help_info->busy_pointer, 0);

    length = STRLEN(application_name);

    if (length > 73)
	length = 73;

    STRCPY(client_name, application_name);
    /*strncpy(client_name, application_name, length);*/

    client_name[length] = 0;

#ifdef OW_I18N
    SPRINTF(client_name,"%ws: Help",client_name);
#else
    SPRINTF(client_name,"%s: Help",client_name);
    /*strcat(client_name, XV_MSG(": Help"));*/
#endif

    if (!help_info->help_frame) {
	/* Help frame has not been created yet */
	client_frame = xv_get(client_window, WIN_FRAME);
	if (!client_frame ||
	    !xv_get(client_frame, XV_IS_SUBTYPE_OF, FRAME_CLASS))
	    /* No frame: may be a menu, in which case client_frame
	     * can be found in XV_KEY_DATA WIN_FRAME.
	     */
	    client_frame = xv_get(client_window, XV_KEY_DATA, WIN_FRAME);
	if (!client_frame) {
	    help_request_failed(client_window, client_data,
		XV_MSG("No frame associated with this window"));
	    xv_set(client_window, WIN_CURSOR, current_pointer, 0);
	    return XV_ERROR;
	}
	root_window = xv_get(screen, XV_ROOT);
	help_info->help_frame = xv_create(client_frame, FRAME_HELP,
					  WIN_PARENT, root_window,
					  XV_KEY_DATA, help_info_key, help_info,
#ifdef OW_I18N
			                  XV_LABEL_WCS, client_name,
					  WIN_USE_IM, FALSE,
#else
			                  XV_LABEL, client_name,
#endif /* OW_I18N */
			                  0);
	help_frame_rect = *(Rect *) xv_get(help_info->help_frame, XV_RECT);
	help_frame_rect.r_left = 0;
	help_frame_rect.r_top = 0;
	frame_set_rect(help_info->help_frame, &help_frame_rect);
	notify_interpose_destroy_func(help_info->help_frame,
				      help_frame_destroy_proc);
	help_info->help_textsw = xv_create(help_info->help_frame, TEXTSW,
				XV_X, mglass_width,
				XV_Y, 0,
				WIN_COLUMNS, HELPTEXTCOLS,
				WIN_ROWS, HELPTEXTLINES,
				TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
				TEXTSW_LINE_BREAK_ACTION, TEXTSW_WRAP_AT_WORD,
				TEXTSW_LOWER_CONTEXT, -1, /* disable automatic
							   * scrolling */
				TEXTSW_DISABLE_LOAD, TRUE,
				TEXTSW_READ_ONLY, TRUE,
				0);
	textsw_view = xv_get(help_info->help_textsw, OPENWIN_NTH_VIEW, 0);
	xv_set(textsw_view,
	       XV_HELP_DATA, "xview:helpWindow",
	       0);
	help_info->help_textsw_sb = xv_get(help_info->help_textsw,
	    OPENWIN_VERTICAL_SCROLLBAR, textsw_view);
	xv_set(help_info->help_textsw_sb, SCROLLBAR_SPLITTABLE, FALSE, 0);
	mglass_panel = xv_create(help_info->help_frame, PANEL,
			       XV_X, 0,
			       XV_Y, 0,
			       XV_WIDTH, mglass_width,
			       XV_HEIGHT,
				   xv_get(help_info->help_textsw, XV_HEIGHT),
			       XV_HELP_DATA, "xview:helpWindow",
			       0);
	help_info->mglass_msg = xv_create(mglass_panel, PANEL_MESSAGE,
	    XV_HELP_DATA, "xview:helpMagnifyingGlass",
	    0);
	more_help_panel = xv_create(help_info->help_frame, PANEL,
	    XV_X, 0,
	    WIN_BELOW, help_info->help_textsw,
	    XV_WIDTH,
		mglass_width + xv_get(help_info->help_textsw, XV_WIDTH),
	    XV_HELP_DATA, "xview:helpWindow",
	    0);
	help_info->more_help_button = xv_create(more_help_panel, PANEL_BUTTON,
	    XV_X, mglass_width + MORE_BUTTON_OFFSET,
	    PANEL_LABEL_STRING, XV_MSG("More"),
	    PANEL_NOTIFY_PROC, more_help_proc,
	    XV_SHOW, FALSE,
	    0);
	window_fit_height(more_help_panel);
	window_fit(help_info->help_frame);
    } else {
	/* Help frame already exists: set help frame header and
	 * empty text subwindow.
	 */
	xv_set(help_info->help_frame,
#ifdef OW_I18N
	       XV_LABEL_WCS, client_name,
#else
	       XV_LABEL, client_name,
#endif
	       0);
	textsw_reset(help_info->help_textsw, 0, 0);
    }

    /* Draw magnifying glass over help image */
    if (!help_info->mglass_image) {
	help_info->mglass_image = xv_create(screen, SERVER_IMAGE,
			                    XV_WIDTH, mglass_width,
			                    XV_HEIGHT, mglass_height,
			                    SERVER_IMAGE_DEPTH, 1,
				            SERVER_IMAGE_X_BITS, mglass_bits,
				            0);
	help_info->mglass_stencil_image = xv_create(screen, SERVER_IMAGE,
	    XV_WIDTH, mglass_mask_width,
	    XV_HEIGHT, mglass_mask_height,
	    SERVER_IMAGE_DEPTH, 1,
	    SERVER_IMAGE_X_BITS, mglass_mask_bits,
	    0);
    }
    if (!help_info->help_stencil_gc) {
	DRAWABLE_INFO_MACRO(mglass_panel, dst_info);
	DRAWABLE_INFO_MACRO(help_info->mglass_stencil_image, stencil_info);
	DRAWABLE_INFO_MACRO(help_info->mglass_image, src_info);
	gc_values.foreground = xv_fg(dst_info);
	gc_values.background = xv_bg(dst_info);
	gc_values.fill_style = FillOpaqueStippled;
	gc_values.stipple = xv_xid(src_info);
	gc_values.clip_mask = xv_xid(stencil_info);
	help_info->help_stencil_gc = XCreateGC(xv_display(dst_info),
	    xv_xid(dst_info),
	    GCForeground | GCBackground | GCFillStyle | GCStipple | GCClipMask,
	    &gc_values);
    }

    /* check to make sure that server image can actually be displayed in
       the frame.  If not, then just display the magnifying glass. */

    if (xv_get(help_info->help_image,SERVER_IMAGE_DEPTH)==
	xv_get(help_info->help_frame,XV_DEPTH)) {
	DRAWABLE_INFO_MACRO(help_info->help_image, dst_info);
	XFillRectangle(xv_display(dst_info), xv_xid(dst_info),
		       help_info->help_stencil_gc,
		       0, 0, mglass_mask_width, mglass_mask_height);
	xv_set(help_info->mglass_msg,
 	       PANEL_LABEL_IMAGE, help_info->help_image, 
	       NULL);
    } else {
	xv_set(help_info->mglass_msg,
	       PANEL_LABEL_IMAGE,help_info->mglass_image,
	       NULL);
    }	

    xv_set(help_info->more_help_button,
	   XV_SHOW,	more_help_cmd ? TRUE : FALSE,
	   XV_KEY_DATA, MORE_HELP_KEY, more_help_cmd,
	   0);

    for (i = 0; text; i++) {
	(void) textsw_insert(help_info->help_textsw, text, strlen(text));
	text = xv_help_get_text();
    }
    xv_set(help_info->help_textsw, TEXTSW_FIRST, 0, 0);

    xv_set(help_info->help_textsw_sb, XV_SHOW, i > HELPTEXTLINES, 0);

    /* Show window, in front of all other windows */
    xv_set(help_info->help_frame,
	   XV_SHOW, TRUE,
	   WIN_FRONT,
	   0);

    /* Restore pointer */
    xv_set(client_window, WIN_CURSOR, current_pointer, 0);

    return XV_OK;
}


/*
 * Public "show help" routine
 */
Xv_public int
xv_help_show(client_window, client_data, client_event)
    Xv_Window       client_window;
    char           *client_data;	/* "file:key" */
    Event          *client_event;
{
    int		    client_height;
    int             client_width;
    char	   *data;
    char	   *err_msg;
    char	    file_key[MAX_FILE_KEY_LENGTH]; /* from String File */
    FILE	   *file_ptr;
    char	    help_string[MAX_HELP_STRING_LENGTH]; /* from String File */
    char	   *help_string_filename;
    Seln_holder	    holder;
    char	   *msg;
    Seln_request   *result;
    char	   *seln_string; /* from Primary Selection */
    Xv_Window	    window;

    if (event_action(client_event) == ACTION_TEXT_HELP ||
	event_action(client_event) == ACTION_MORE_TEXT_HELP) {
	/* Get Primary Selection */
	holder = seln_inquire(SELN_PRIMARY);
	if (holder.state != SELN_EXISTS) {
	    help_request_failed(client_window, NULL, 
		XV_MSG("No Primary Selection"));
	    return XV_ERROR;
	}
	result = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
	data = result->data;
	if (!data) {
	    help_request_failed(client_window, NULL, 
		XV_MSG("No Primary Selection"));
	    return XV_ERROR;
	}
	data += sizeof(Seln_attribute);
	seln_string = xv_malloc(strlen(data)+1);
	strcpy(seln_string, data);

	/* Get the Help String File name */
	window = client_window;
	do {
	    help_string_filename = (char *) xv_get(window,
		HELP_STRING_FILENAME);
	} while (!help_string_filename &&
		 (window = xv_get(window, XV_OWNER)));
	if (!help_string_filename) {
	    free(seln_string);
	    help_request_failed(client_window, NULL,
		XV_MSG("No Help String Filename specified for window"));
	    return XV_ERROR;
	}

	/* Search the Help String File for the Primary Selection */
	file_ptr = xv_help_find_file(help_string_filename);
	if (!file_ptr) {
	    free(seln_string);
	    help_request_failed(client_window, NULL, 
		XV_MSG("Help String File not found"));
	    return XV_ERROR;
	}
	client_data = NULL;
	while (fscanf(file_ptr, "%s %s\n", help_string, file_key) != EOF) {
	    if (!strcmp(help_string, seln_string)) {
		client_data = file_key;
		break;
	    }
	}
	fclose(file_ptr);
	if (!client_data) {
	    err_msg = XV_MSG("\" not found in Help String File");
	    msg = xv_malloc(strlen(seln_string) + strlen(err_msg) + 2);
	    sprintf(msg, "\"%s%s", seln_string, err_msg);
	    help_request_failed(client_window, NULL, msg);
	    free(msg);
	    free(seln_string);
	    return XV_ERROR;
	}
	free(seln_string);
    }

    client_width = (int) xv_get(client_window, XV_WIDTH);
    client_height = (int) xv_get(client_window, XV_HEIGHT);
    if (event_action(client_event) != ACTION_MORE_HELP &&
	event_action(client_event) != ACTION_MORE_TEXT_HELP)
	xv_help_save_image(client_window, client_width, client_height,
			   event_x(client_event), event_y(client_event));
    return xv_help_render(client_window, client_data, client_event);
}
#endif
