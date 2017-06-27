/* file: wfdbio.c	G. Moody	18 November 1988
                        Last revised:   22 May 2015       wfdblib 10.5.24
Low-level I/O functions for the WFDB library

_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1988-2013 George B. Moody

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License along
with this library; if not, write to the Free Software Foundation, Inc., 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

You may contact the author by e-mail (george@mit.edu) or postal mail
(MIT Room E25-505A, Cambridge, MA 02139 USA).  For updates to this software,
please visit PhysioNet (http://www.physionet.org/).
_______________________________________________________________________________

This file contains a large number of functions related to input and output.
To make them easier to find, the functions are arranged in several groups as
outlined below;  they appear in the body of the file in the same order that
their names appear in these comments.

This file contains definitions of the following WFDB library functions:
 getwfdb		(returns the database path string)
 setwfdb		(sets the database path string)
 resetwfdb [10.5.7]	(restores the database path to its initial value)
 wfdbquiet		(suppresses WFDB library error messages)
 wfdbverbose [4.0]	(enables WFDB library error messages)
 wfdberror [4.5]	(returns the most recent WFDB library error message)
 wfdbfile [4.3]		(returns the complete pathname of a WFDB file)
 wfdbmemerr [10.4.6]    (set behavior on memory errors)

These functions expose config strings needed by the WFDB Toolkit for Matlab:
 wfdbversion [10.4.20]  (return the string defined by VERSION)
 wfdbldflags [10.4.20]  (return the string defined by LDFLAGS)
 wfdbcflags [10.4.20]   (return the string defined by CFLAGS)
 wfdbdefwfdb [10.4.20]  (return the string defined by DEFWFDB)
 wfdbdefwfdbcal [10.4.20] (return the string defined by DEFWFDBCAL)

These functions, also defined here, are intended only for the use of WFDB
library functions defined elsewhere:

 wfdb_me_fatal [10.4.6] (indicates if memory errors are fatal)
 wfdb_g16		(reads a 16-bit integer)
 wfdb_g32		(reads a 32-bit integer)
 wfdb_p16		(writes a 16-bit integer)
 wfdb_p32		(writes a 32-bit integer)
 wfdb_free_path_list [10.0.1] (frees data structures assigned to the path list)
 wfdb_parse_path [10.0.1] (splits WFDB path into components)
 wfdb_export_config [10.3.9] (puts the WFDB path, etc. into the environment)
 wfdb_getiwfdb [6.2]	(sets WFDB from the contents of a file)
 wfdb_addtopath [6.2]	(adds path component of string argument to WFDB path)
 wfdb_error		(produces an error message)
 wfdb_fprintf [10.0.1]	(like fprintf, but first arg is a WFDB_FILE pointer)
 wfdb_open		(finds and opens database files)
 wfdb_checkname		(checks record and annotator names for validity)
 wfdb_striphea [10.4.5] (removes trailing '.hea' from a record name, if present)
 wfdb_setirec [9.7]	(saves current record name)
 wfdb_getirec [10.5.12]	(gets current record name)

(Numbers in brackets in the lists above indicate the first version of the WFDB
library that included the corresponding function.  Functions not so marked
have been included in all published versions of the WFDB library.)

The next two groups of functions, which together enable input from remote
(http and ftp) files, were first implemented in version 10.0.1 by Michael
Dakin.  Thanks, Mike!

These functions, defined here if WFDB_NETFILES is non-zero, are intended only
for the use of the functions in the next group below (their definitions are not
visible outside of this file):
 www_parse_passwords	(load username/password information)
 www_userpwd		(get username/password for a given url)
 wfdb_wwwquit		(shut down libcurl or libwww cleanly)
 www_init		(initialize libcurl or libwww)
 www_get_cont_len	(find length of data for a given url)
 www_get_url_range_chunk (get a block of data from a given url)
 www_get_url_chunk	(get all data from a given url)
 nf_delete		(free data structures associated with an open netfile)
 nf_new			(associate a netfile with a url)
 nf_get_range		(get a block of data from a netfile)
 nf_feof		(emulates feof, for netfiles)
 nf_eof			(TRUE if netfile pointer points to EOF)
 nf_fopen		(emulate fopen, for netfiles; read-only access)
 nf_fclose		(emulates fclose, for netfiles)
 nf_fgetc		(emulates fgetc, for netfiles)
 nf_fgets		(emulates fgets, for netfiles)
 nf_fread		(emulates fread, for netfiles)
 nf_fseek		(emulates fseek, for netfiles)
 nf_ftell		(emulates ftell, for netfiles)
 nf_ferror		(emulates ferror, for netfiles)
 nf_clearerr		(emulates clearerr, for netfiles)
 nf_fflush		(emulates fflush, for netfiles) [stub]
 nf_fwrite		(emulates fwrite, for netfiles) [stub]
 nf_putc		(emulates putc, for netfiles) [stub]
 nf_vfprintf		(emulates fprintf, for netfiles) [stub]

In the current version of the WFDB library, output to remote files is not
implemented;  for this reason, several of the functions listed above are
stubs (placeholders) only, as noted.

These functions, also defined here, are compiled only if WFDB_NETFILES is non-
zero; they permit access to remote files via http or ftp (using libcurl or
libwww) as well as to local files (using the standard C I/O functions).  The
functions in this group are intended primarily for use by other WFDB library
functions, but may also be called directly by WFDB applications that need to
read remote files. Unlike other private functions in the WFDB library, the
interfaces to these are not likely to change, since they are designed to
emulate the similarly-named ANSI/ISO C standard I/O functions:
 wfdb_clearerr		(emulates clearerr)
 wfdb_feof		(emulates feof)
 wfdb_ferror		(emulates ferror)
 wfdb_fflush		(emulates fflush, for local files only)
 wfdb_fgets		(emulates fgets)
 wfdb_fread		(emulates fread)
 wfdb_fseek		(emulates fseek)
 wfdb_ftell		(emulates ftell)
 wfdb_fwrite		(emulates fwrite, for local files only)
 wfdb_getc		(emulates getc)
 wfdb_putc		(emulates putc, for local files only)
 wfdb_fclose		(emulates fclose)
 wfdb_fopen		(emulates fopen, but returns a WFDB_FILE pointer)

(If WFDB_NETFILES is zero, wfdblib.h defines all but the last two of these
functions as macros that invoke the standard I/O functions that they would
otherwise emulate.  The implementations of wfdb_fclose and wfdb_fopen are
below;  they include a small amount of code compiled only if WFDB_NETFILES
is non-zero.  All of these functions are new in version 10.0.1.)

Finally, this file includes several miscellaneous functions needed only in
certain environments:
 strtok		(parses strings into tokens, for old C libraries that need it)
 LibMain        (initializes 16-bit MS-Windows DLL version of this library)
 WEP		(cleans up on exit from 16-bit MS-Windows DLL)
 wgetenv	(replacement for getenv, for use with MS-Windows 16-bit DLLs)
 DllMain	(initialize/cleanup 32-bit MS-Windows DLL)

Functions in signal.c and calib.c use the C library function strtok() to parse
lines into tokens;  this function (and its associated header file <string.h>)
may not be available in certain older C libraries (e.g., UNIX version 7 and BSD
4.2).  This file includes a portable implementation of strtok(), which can be
obtained if necessary by defining the symbol NOSTRTOK when compiling this
module.
*/

#include "wfdblib.h"

/* WFDB library functions */

/* getwfdb is used to obtain the WFDB path, a list of places in which to search
for database files to be opened for reading.  In most environments, this list
is obtained from the shell (environment) variable WFDB, which may be set by the
user (typically as part of the login script).  A default value may be set at
compile time (DEFWFDB in wfdblib.h); this is necessary for environments that do
not support the concept of environment variables, such as MacOS9 and earlier.
If WFDB or DEFWFDB is of the form '@FILE', getwfdb reads the WFDB path from the
specified (local) FILE (using wfdb_getiwfdb); such files may be nested up to
10 levels. */

static char *wfdbpath = NULL, *wfdbpath_init = NULL;

/* resetwfdb is called by wfdbquit, and can be called within an application,
to restore the WFDB path to the value that was returned by the first call
to getwfdb (or NULL if getwfdb was not called). */

FVOID resetwfdb(void)
{
    SSTRCPY(wfdbpath, wfdbpath_init);
}

FSTRING getwfdb(void)
{
    if (wfdbpath == NULL) {
	char *p = getenv("WFDB"), *wfdb_getiwfdb(char *p);

	if (p == NULL) p = DEFWFDB;
	if (*p == '@') p = wfdb_getiwfdb(p);
	SSTRCPY(wfdbpath_init, p);
	setwfdb(p);
    }
    return (wfdbpath);
}

/* setwfdb can be called within an application to change the WFDB path. */

FVOID setwfdb(char *p)
{
    void wfdb_export_config(void);

    if (p == NULL && (p = getenv("WFDB")) == NULL) p = DEFWFDB;
    wfdb_parse_path(p);
    SSTRCPY(wfdbpath, p);
    wfdb_export_config();
}

/* wfdbquiet can be used to suppress error messages from the WFDB library. */

static int error_print = 1;

FVOID wfdbquiet(void)
{
    error_print = 0;
}

/* wfdbverbose enables error messages from the WFDB library. */

FVOID wfdbverbose(void)
{
    error_print = 1;
}

#define MFNLEN	1024	/* max length of WFDB filename, including '\0' */
static char wfdb_filename[MFNLEN];

/* wfdbfile returns the pathname or URL of a WFDB file. */

FSTRING wfdbfile(char *s, char *record)
{
    WFDB_FILE *ifile;

    if (s == NULL && record == NULL)
	return (wfdb_filename);

    /* Remove trailing .hea, if any, from record name. */
    wfdb_striphea(record);

    if ((ifile = wfdb_open(s, record, WFDB_READ))) {
	(void)wfdb_fclose(ifile);
	return (wfdb_filename);
    }
    else return (NULL);
}

/* Determine how the WFDB library handles memory allocation errors (running
out of memory).  Call wfdbmemerr(0) in order to have these errors returned
to the caller;  by default, such errors cause the running process to exit. */

static int wfdb_mem_behavior = 1;

FVOID wfdbmemerr(int behavior)
{
    wfdb_mem_behavior = behavior;
}

/* Functions that expose configuration constants used by the WFDB Toolkit for
   Matlab. */

#ifndef VERSION
#define VERSION "VERSION not defined"
#endif

#ifndef LDFLAGS
#define LDFLAGS "LDFLAGS not defined"
#endif

#ifndef CFLAGS
#define CFLAGS  "CFLAGS not defined"
#endif

FCONSTSTRING wfdbversion(void)
{
   return VERSION;
}

FCONSTSTRING wfdbldflags(void)
{
   return LDFLAGS;
}

FCONSTSTRING wfdbcflags(void)
{
   return CFLAGS;
}

FCONSTSTRING wfdbdefwfdb(void)
{
   return DEFWFDB;
}

FCONSTSTRING wfdbdefwfdbcal(void)
{
   return DEFWFDBCAL;
}

/* Private functions (for the use of other WFDB library functions only). */

int wfdb_me_fatal()	/* used by the MEMERR macro defined in wfdblib.h */
{
    return (wfdb_mem_behavior);
}

/* The next four functions read and write integers in PDP-11 format, which is
common to both MIT and AHA database files.  The purpose is to achieve
interchangeability of binary database files between machines which may use
different byte layouts.  The routines below are machine-independent; in some
cases (notably on the PDP-11 itself), taking advantage of the native byte
layout can improve the speed.  For 16-bit integers, the low (least significant)
byte is written (read) before the high byte; 32-bit integers are represented as
two 16-bit integers, but the high 16 bits are written (read) before the low 16
bits. These functions, in common with other WFDB library functions, assume that
a byte is 8 bits, a "short" is 16 bits, an "int" is at least 16 bits, and a
"long" is at least 32 bits.  The last two assumptions are valid for ANSI C
compilers, and for almost all older C compilers as well.  If a "short" is not
16 bits, it may be necessary to rewrite wfdb_g16() to obtain proper sign
extension. */

/* read a 16-bit integer in PDP-11 format */
int wfdb_g16(WFDB_FILE *fp)
{
    int x;

    x = wfdb_getc(fp);
    return ((int)((short)((wfdb_getc(fp) << 8) | (x & 0xff))));
}

/* read a 32-bit integer in PDP-11 format */
long wfdb_g32(WFDB_FILE *fp)
{
    long x, y;

    x = wfdb_g16(fp);
    y = wfdb_g16(fp);
    return ((x << 16) | (y & 0xffff));
}

/* write a 16-bit integer in PDP-11 format */
void wfdb_p16(unsigned int x, WFDB_FILE *fp)
{
    (void)wfdb_putc((char)x, fp);
    (void)wfdb_putc((char)(x >> 8), fp);
}

/* write a 32-bit integer in PDP-11 format */
void wfdb_p32(long x, WFDB_FILE *fp)
{
    wfdb_p16((unsigned int)(x >> 16), fp);
    wfdb_p16((unsigned int)x, fp);
}

struct wfdb_path_component {
    char *prefix;
    struct wfdb_path_component *next, *prev;
    int type;		/* WFDB_LOCAL or WFDB_NET */
};
static struct wfdb_path_component *wfdb_path_list;

/* wfdb_free_path_list clears out the path list, freeing all memory allocated
   to it. */
void wfdb_free_path_list(void)
{
   struct wfdb_path_component *c0 = NULL, *c1 = wfdb_path_list;

    while (c1) {
	c0 = c1->next;
	SFREE(c1->prefix);
	SFREE(c1);
	c1 = c0;
    }
    wfdb_path_list = NULL;
}

/* Operating system and compiler dependent code

All of the operating system and compiler dependencies in the WFDB library are
contained within the following section of this file.  There are three
significant types of platforms addressed here:

   UNIX and variants
     This includes GNU/Linux, FreeBSD, Solaris, HP-UX, IRIX, AIX, and other
     versions of UNIX, as well as Mac OS/X and Cygwin/MS-Windows.
   MS-DOS and variants
     This group includes MS-DOS, DR-DOS, OS/2, and all versions of MS-Windows
     when using the native MS-Windows libraries (as when compiling with MinGW
     gcc).
   MacOS 9 and earlier
     "Classic" MacOS.

Differences among these platforms:

1. Directory separators vary:
     UNIX and variants (including Mac OS/X and Cygwin) use '/'.
     MS-DOS and OS/2 use '\'.
     MacOS 9 and earlier uses ':'.

2. Path component separators also vary:
     UNIX and variants use ':' (as in the PATH environment variable)
     MS-DOS and OS/2 use ';' (also as in the PATH environment variable;
       ':' within a path component follows a drive letter)
     MacOS uses ';' (':' is a directory separator, as noted above)
   See the notes above wfdb_open for details about path separators and how
   WFDB file names are constructed.

3. By default, MS-DOS files are opened in "text" mode.  Since WFDB files are
   binary, they must be opened in binary mode.  To accomplish this, ANSI C
   libraries, and those supplied with non-ANSI C compilers under MS-DOS, define
   argument strings "rb" and "wb" to be supplied to fopen(); unfortunately,
   most other non-ANSI C versions of fopen do not recognize these as legal.
   The "rb" and "wb" forms are used here for ANSI and MS-DOS C compilers only,
   and the older "r" and "w" forms are used in all other cases.

4. Before the ANSI/ISO C standard was adopted, there were at least two
   (commonly used but incompatible) ways of declaring functions with variable
   numbers of arguments (such as printf).  The ANSI/ISO C standard defined
   a third way, incompatible with the other two.  For this reason, the only two
   functions in the WFDB library (wfdb_error and wfdb_fprintf) that need to use
   variable argument lists are included below in three different versions (with
   additional minor variations noted below).

OS- and compiler-dependent definitions:

   For MS-DOS/MS-Windows compilers.  Note that not all such compilers
   predefine the symbol MSDOS;  for those that do not, MSDOS is usually defined
   by a compiler command-line option in the "make" description file. */ 
#ifdef MSDOS
#define DSEP	'\\'
#define PSEP	';'
#define AB	"ab"
#define RB	"rb"
#define WB	"wb"
#else

/* For other ANSI C compilers.  Such compilers must predefine __STDC__ in order
   to conform to the ANSI specification. */
#ifdef __STDC__
#ifdef MAC	/* Macintosh only.  Be sure to define MAC in 'wfdblib.h'. */
#define DSEP	':'
#define PSEP	';'
#else		/* Other ANSI C compilers (UNIX and variants). */
#define DSEP	'/'
#define PSEP	':'
#endif
#define AB	"ab"
#define RB	"rb"
#define WB	"wb"

/* For all other C compilers, including traditional UNIX C compilers. */
# else
#define DSEP	'/'
#define PSEP	':'
#define AB	"a"
#define RB	"r"
#define WB	"w"
#endif
#endif

/* wfdb_parse_path constructs a linked list of path components by splitting
its string input (usually the value of WFDB). */

int wfdb_parse_path(char *p)
{
    char *q;
    int current_type, found_end;
    struct wfdb_path_component *c0 = NULL, *c1 = wfdb_path_list;
    static first_call = 1;

    /* First, free the existing wfdb_path_list, if any. */
    wfdb_free_path_list();

    /* Do nothing else if no path string was supplied. */
    if (p == NULL) return (0);

    /* Register the cleanup function so that it is invoked on exit. */
    if (first_call) {
	atexit(wfdb_free_path_list);
	first_call = 0;
    }
    q = p;

    /* Now construct the wfdb_path_list from the contents of p. */
    while (*q) {
	/* Find the beginning of the next component (skip whitespace). */
	while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')
	    q++;
	p = q--;
	current_type = WFDB_LOCAL;
	/* Find the end of the current component. */
	found_end = 0;
	do {
	    switch (*++q) {
	    case ':':	/* might be a component delimiter, part of '://',
			   a drive suffix (MS-DOS), or a directory separator
			   (Mac) */
		if (*(q+1) == '/' && *(q+2) == '/') current_type = WFDB_NET;
#if PSEP == ':'
		else found_end = 1;
#endif
		break;
	    case ';':	/* definitely a component delimiter */
	    case ' ':
	    case '\t':
	    case '\n':
	    case '\r':
	    case '\0':
		found_end = 1;
		break;
	    }
	} while (!found_end);

	/* current component begins at p, ends at q-1 */
	SUALLOC(c1, 1, sizeof(struct wfdb_path_component));
	SALLOC(c1->prefix, q-p+1, sizeof(char));
	memcpy(c1->prefix, p, q-p);
	c1->type = current_type;
	c1->prev = c0;
	if (c0) c0->next = c1;
	else wfdb_path_list = c1;
	c0 = c1;
	if (*q) q++;
    }
    return (0);
}	


/* wfdb_getiwfdb reads a new value for WFDB from the file named by the second
through last characters of its input argument.  If that value begins with '@',
this procedure is repeated, with nesting up to ten levels.

Note that the input file must be local (it is accessed using the standard C I/O
functions rather than their wfdb_* counterparts).  This limitation is
intentional, since the alternative (to allow remote files to determine the
contents of the WFDB path) seems an unnecessary security risk. */

#ifndef SEEK_END
#define SEEK_END 2
#endif

char *wfdb_getiwfdb(char *p)
{
    FILE *wfdbpfile;
    int i = 0;
    long len;

    for (i = 0; i < 10 && *p == '@'; i++) {
	if ((wfdbpfile = fopen(p+1, RB)) == NULL) p = "";
	else {
	    if (fseek(wfdbpfile, 0L, SEEK_END) == 0)
		len = ftell(wfdbpfile);
	    else len = 255;
	    SUALLOC(p, 1, len+1);
	    rewind(wfdbpfile);
	    len = fread(p, 1, (int)len, wfdbpfile);
	    while (p[len-1] == '\n' || p[len-1] == '\r')
		p[--len] = '\0';
	    (void)fclose(wfdbpfile);
	}
    }	
    if (*p == '@') {
	wfdb_error("getwfdb: files nested too deeply\n");
	p = "";
    }
    return (p);
}

/* wfdb_export_config is invoked from setwfdb to place the configuration
   variables into the environment if possible. */

#ifndef HAS_PUTENV
#define wfdb_export_config()
#else
static char *p_wfdb, *p_wfdbcal, *p_wfdbannsort, *p_wfdbgvmode;

/* wfdb_free_config frees all memory allocated by wfdb_export_config.
   This function must be invoked before exiting to avoid a memory leak.
   It must not be invoked at any other time, since pointers passed to
   putenv must be maintained by the caller, according to POSIX.1-2001
   semantics for putenv.  */
void wfdb_free_config(void)
{
    SFREE(p_wfdb);
    SFREE(p_wfdbcal);
    SFREE(p_wfdbannsort);
    SFREE(p_wfdbgvmode);
}

void wfdb_export_config(void)
{
    static int first_call = 1;

    /* Register the cleanup function so that it is invoked on exit. */
    if (first_call) {
	atexit(wfdb_free_config);
	first_call = 0;
    }
    SALLOC(p_wfdb, 1, strlen(wfdbpath)+6);
    sprintf(p_wfdb, "WFDB=%s", wfdbpath);
    putenv(p_wfdb);
    if (getenv("WFDBCAL") == NULL) {
	SALLOC(p_wfdbcal, 1, strlen(DEFWFDBCAL)+9);
	sprintf(p_wfdbcal, "WFDBCAL=%s", DEFWFDBCAL);
	putenv(p_wfdbcal);
    }
    if (getenv("WFDBANNSORT") == NULL) {
	SALLOC(p_wfdbannsort, 1, 14);
	sprintf(p_wfdbannsort, "WFDBANNSORT=%d", DEFWFDBANNSORT == 0 ? 0 : 1);
	putenv(p_wfdbannsort);
    }
    if (getenv("WFDBGVMODE") == NULL) {
	SALLOC(p_wfdbgvmode, 1, 13);
	sprintf(p_wfdbgvmode, "WFDBGVMODE=%d", DEFWFDBGVMODE == 0 ? 0 : 1);
	putenv(p_wfdbgvmode);
    }
}
#endif

/* wfdb_addtopath adds the path component of its string argument (i.e.
everything except the file name itself) to the WFDB path, inserting it
there if it is not already in the path.  If the first component of the WFDB
path is '.' (the current directory), the new component is moved to the second
position; otherwise, it is moved to the first position.

wfdb_open calls this function whenever it finds and opens a file.

Since the files comprising a given record are most often kept in the
same directory, this strategy improves the likelihood that subsequent
files to be opened will be found in the first or second location wfdb_open
checks.

If the current directory (.) is at the head of the WFDB path, it remains there,
so that wfdb_open will continue to find the user's own files in preference to
like-named files elsewhere in the path.  If this behavior is not desired, the
current directory should not be specified initially as the first component of
the WFDB path.
 */

void wfdb_addtopath(char *s)
{
    char *p, *t;
    int i, len;
    struct wfdb_path_component *c0, *c1;

    if (s == NULL || *s == '\0') return;

    /* Start at the end of the string and search backwards for a directory
       separator (accept any of the possible separators). */
    for (p = s + strlen(s) - 1; p >= s &&
	      *p != '/' && *p != '\\' && *p != ':'; p--)
	;

    /* A path component specifying the root directory must be treated as a
       special case;  normally the trailing directory separator is not
       included in the path component, but in this case there is nothing
       else to include. */

    if (p == s && (*p == '/' || *p == '\\' || *p == ':')) p++;

    if (p < s) return;		/* argument did not contain a path component */

    /* If p > s, then p points to the first character following the path
       component of s. Search the current WFDB path for this path component. */
    if (wfdbpath == NULL) (void)getwfdb();
    for (c0 = c1 = wfdb_path_list, i = p-s; c1; c1 = c1->next) {
	if (strncmp(c1->prefix, s, i) == 0) {
	    if (c0 == c1 || (c1->prev == c0 && strcmp(c0->prefix, ".") == 0))
		return; /* no changes needed, quit */
	    /* path component of s is already in WFDB path -- unlink its node */
	    if (c1->next) (c1->next)->prev = c1->prev;
	    if (c1->prev) (c1->prev)->next = c1->next;
	    break;
	}
    }
    if (!c1) {
	/* path component of s not in WFDB path -- make a new node for it */
 	SUALLOC(c1, 1, sizeof(struct wfdb_path_component));
	SALLOC(c1->prefix, p-s+1, sizeof(char));
	memcpy(c1->prefix, s, p-s);
	if (strstr(c1->prefix, "://")) c1->type = WFDB_NET;
	else c1->type = WFDB_LOCAL;
    }
    /* (Re)link the unlinked node. */
    if (strcmp(c0->prefix, ".") == 0) {  /* skip initial "." if present */
	c1->prev = c0;
	if ((c1->next = c0->next) != NULL)
	    (c1->next)->prev = c1;
	c0->next = c1;
    }	
    else { /* no initial ".";  insert the node at the head of the path */ 
	wfdb_path_list = c1;
	c1->prev = NULL;
	c1->next = c0;
	c0->prev = c1;
    }
    return;
}

/* The wfdb_error function handles error messages, normally by printing them
on the standard error output.  Its arguments can be any set of arguments which
would be legal for printf, i.e., the first one is a format string, and any
additional arguments are values to be filled into the '%*' placeholders
in the format string.  It can be silenced by invoking wfdbquiet(), or
re-enabled by invoking wfdbverbose().

The wfdb_fprintf function handles all formatted output to files.  It is used
in the same way as the standard fprintf function, except that its first
argument is a pointer to a WFDB_FILE rather than a FILE.

There are three major versions of each of wfdb_error and wfdb_fprintf below.
The first version is compiled by ANSI C compilers.  (A variant of this version
of wfdb_error can be used with Microsoft Windows; it puts the error message
into a message box, rather than using the standard error output.)  The second
version is compiled by traditional UNIX C compilers (System V, Berkeley 4.x)
that are not ANSI-conforming.  The third version can be compiled by many older
C compilers, if the symbol OLDC is defined; do so only if you are using a C
library which does not include a vsprintf function and a "stdarg.h" or
"varargs.h" header file.  This third version uses an undocumented function
(_doprnt) which works for most if not all older UNIX C compilers, and several
others as well.

If OLDC is not defined, the function wfdberror (without the underscore) returns
the most recent error message passed to wfdb_error (even if output was
suppressed by wfdbquiet).  This feature permits programs to handle errors
somewhat more flexibly (in windowing environments, for example, where using the
standard error output may be inappropriate).  */

#ifndef OLDC
static char error_message[256];
#endif

FSTRING wfdberror(void)
{
    if (*error_message == '\0')
	sprintf(error_message,
	       "WFDB library version %d.%d.%d (%s).\n",
	       WFDB_MAJOR, WFDB_MINOR, WFDB_RELEASE, __DATE__);
    return (error_message);
}

/* First version: for ANSI C compilers and Microsoft Windows */
#if defined(__STDC__) || defined(_WINDOWS)
#include <stdarg.h>
#ifdef _WINDOWS
#include <windows.h>	      /* contains function prototype for MessageBox */
#endif

void wfdb_error(char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
#if 1				/* standard variant: use stderr output */
    (void)vsprintf(error_message, format, arguments);
    if (error_print) {
	(void)fprintf(stderr, "%s", error_message);
	(void)fflush(stderr);
    }
#else				/* MS Windows variant: use message box */
    (void)wvsprintf(error_message, format, arguments);
    if (error_print)
	MessageBox(GetFocus(), error_message, "WFDB Library Error",
		    MB_ICONASTERISK | MB_OK);
#endif
    va_end(arguments);
}

#if WFDB_NETFILES
static int nf_vfprintf(netfile *nf, const char *format, va_list ap)
{
    /* no support yet for writing to remote files */
    errno = EROFS;
    return (0);
}
#endif

int wfdb_fprintf(WFDB_FILE *wp, const char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);
#if WFDB_NETFILES
    if (wp->type == WFDB_NET)
	ret = nf_vfprintf(wp->netfp, format, args);
    else
#endif
	ret = vfprintf(wp->fp, format, args);
    va_end(args);
    return (ret);
}

#else
#ifndef OLDC	     /* Second version: for traditional UNIX K&R C compilers */
#include <varargs.h>
void wfdb_error(va_alist)
va_dcl
{
    va_list arguments;
    char *format;

    va_start(arguments);
    format = va_arg(arguments, char *);
    (void)vsprintf(error_message, format, arguments);
    if (error_print) {
	(void)fprintf(stderr, "%s", error_message);
	(void)fflush(stderr);
    }
    va_end(arguments);
}

int wfdb_fprintf(va_alist)
va_dcl
{
    va_list arguments;
    WFDB_FILE *wp;
    char *format;
    static char obuf[1024];	/* careful: buffer overflows possible! */
    int ret;

    va_start(arguments);
    wp = va_arg(arguments, WFDB_FILE *);
    format = va_arg(arguments, char *);
    (void)vsprintf(obuf, format, arguments);
    ret = fprintf(wp->fp, format, obuf);
    va_end(arguments);
    return (ret);
}
    
#else			/* Third version: for many older C compilers */
void wfdb_error(format, arguments)
char *format;
{
    if (error_print) {
	(void)_doprnt(format, &arguments, stderr);
	(void)fflush(stderr);
    }
}

int wfdb_fprintf(wp, format, arguments)
WFDB_FILE *wp;
char *format;
{
    return (_doprnt(format, &arguments, wp->fp));
}
#endif
#endif

#define spr1(S, RECORD, TYPE)   ((*TYPE == '\0') ? \
				     (void)sprintf(S, "%s", RECORD) : \
				     (void)sprintf(S, "%s.%s", RECORD, TYPE))
#ifdef FIXISOCD
# define spr2(S, RECORD, TYPE)  ((*TYPE == '\0') ? \
				     (void)sprintf(S, "%s;1", RECORD) : \
				     (void)sprintf(S, "%s.%.3s;1",RECORD,TYPE))
#else
# define spr2(S, RECORD, TYPE)  ((*TYPE == '\0') ? \
				     (void)sprintf(S, "%s.", RECORD) : \
				     (void)sprintf(S, "%s.%.3s", RECORD, TYPE))
#endif

static char irec[WFDB_MAXRNL+1]; /* current record name, set by wfdb_setirec */

/* wfdb_open is used by other WFDB library functions to open a database file
for reading or writing.  wfdb_open accepts two string arguments and an integer
argument.  The first string specifies the file type ("hea", "atr", etc.),
and the second specifies the record name.  The integer argument (mode) is
either WFDB_READ or WFDB_WRITE.  Note that a function which calls wfdb_open
does not need to know the filename itself; thus all system-specific details of
file naming conventions can be hidden in wfdb_open.  If the first argument is
"-", or if the first argument is "hea" and the second is "-", wfdb_open
returns a file pointer to the standard input or output as appropriate.  If
either of the string arguments is null or empty, wfdb_open takes the other as
the file name.  Otherwise, it constructs the file name by concatenating the
string arguments with a "." between them.  If the file is to be opened for
reading, wfdb_open searches for it in the list of directories obtained from
getwfdb(); output files are normally created in the current directory.  By
prefixing the record argument with appropriate path specifications, files can
be opened in any directory, provided that the WFDB path includes a null
(empty) component.

Beginning with version 10.0.1, the WFDB library accepts whitespace (space, tab,
or newline characters) as path component separators under any OS.  Multiple
consecutive whitespace characters are treated as a single path component
separator.  Use a '.' to specify the current directory as a path component when
using whitespace as a path component separator.

If the WFDB path includes components of the forms 'http://somewhere.net/mydata'
or 'ftp://somewhere.else/yourdata', the sequence '://' is explicitly recognized
as part of a URL prefix (under any OS), and the ':' and '/' characters within
the '://' are not interpreted further.  Note that the MS-DOS '\' is *not*
acceptable as an alternative to '/' in a URL prefix.  To make WFDB paths
containing URL prefixes more easily (human) readable, use whitespace for path
component separators.

WFDB file names are usually formed by concatenating the record name, a ".", and
the file type, using the spr1 macro (above).  If an input file name, as
constructed by spr1, does not match that of an existing file, wfdb_open uses
spr2 to construct an alternate file name.  In this form, the file type is
truncated to no more than 3 characters (as MS-DOS does).  When searching for
input files, wfdb_open tries both forms with each component of the WFDB path
before going on to the next path component.

If the record name is empty, wfdb_open swaps the record name and the type
string.  If the type string (after swapping, if necessary) is empty, spr1 uses
the record name as the literal file name, and spr2 uses the record name with an
appended "." as the file name.

In environments in which ISO 9660 version numbers are visible in CD-ROM file
names, define the symbol FIXISOCD.  This causes spr2 to append the
characters ";1" (the version number for an ordinary file) to the file names
it generates.  This feature is needed in order to read post-1992 CD-ROMs
with pre-5.0 versions of the Macintosh "ISO 9660 File Access" software,
with some versions of HP-UX, and possibly in other environments as well.

Pre-10.0.1 versions of this library that were compiled for environments other
than MS-DOS used file names in the format TYPE.RECORD.  This file name format
is no longer supported. */

WFDB_FILE *wfdb_open(const char *s, const char *record, int mode)
{
    char *wfdb, *p, *q, *r;
    int rlen;
    struct wfdb_path_component *c0;
    WFDB_FILE *ifile;

    /* If the type (s) is empty, replace it with an empty string so that
       strcmp(s, ...) will not segfault. */
    if (s == NULL) s = "";

    /* If the record name is empty, use s as the record name and replace s
       with an empty string. */
    if (record == NULL || *record == '\0') {
	if (*s) { record = s; s = ""; }
	else return (NULL);	/* failure -- both components are empty */
    }

    /* Check to see if standard input or output is requested. */
    if (strcmp(record, "-") == 0)
	if (mode == WFDB_READ) {
	    static WFDB_FILE wfdb_stdin;

	    wfdb_stdin.type = WFDB_LOCAL;
	    wfdb_stdin.fp = stdin;
	    return (&wfdb_stdin);
	}
        else {
	    static WFDB_FILE wfdb_stdout;

	    wfdb_stdout.type = WFDB_LOCAL;
	    wfdb_stdout.fp = stdout;
	    return (&wfdb_stdout);
	}

    /* If the record name ends with '/', expand it by adding another copy of
       the final element (e.g., 'abc/123/' becomes 'abc/123/123'). */
    rlen = strlen(record);
    p = (char *)(record + rlen - 1);
    if (*p == '/') {
	for (q = p-1; q > record; q--)
	    if (*q == '/') { q++; break; }
	if (q < p-1) {
	    SUALLOC(r, rlen + p-q + 1, 1); /* p-q is length of final element */
	    strcpy(r, record);
	    strncpy(r + rlen, q, p-q);
	}
    }
    else {
	SUALLOC(r, rlen + 1, 1);
	strcpy(r, record);
    }

    /* If the file is to be opened for output, use the current directory.
       An output file can be opened in another directory if the path to
       that directory is the first part of 'record'. */
    if (mode == WFDB_WRITE) {
	spr1(wfdb_filename, r, s);
	SFREE(r);
	return (wfdb_fopen(wfdb_filename, WB));
    }
    else if (mode == WFDB_APPEND) {
	spr1(wfdb_filename, r, s);
	SFREE(r);
	return (wfdb_fopen(wfdb_filename, AB));
    }

    /* Parse the WFDB path if not done previously. */
    if (wfdb_path_list == NULL) (void)getwfdb();

    /* If the filename begins with 'http://' or 'https://', it's a URL.  In
       this case, don't search the WFDB path, but add its parent directory
       to the path if the file can be read. */
    if (strncmp(r, "http://", 7) == 0 || strncmp(r, "https://", 8) == 0) {
	if (strlen(r) + strlen(s) >= MFNLEN)
	    return (NULL);  /* name too long */
	spr1(wfdb_filename, r, s);
	if ((ifile = wfdb_fopen(wfdb_filename, RB)) != NULL) {
	    /* Found it! Add its path info to the WFDB path. */
	    wfdb_addtopath(wfdb_filename);
	    SFREE(r);
	    return (ifile);
	}
    }

    for (c0 = wfdb_path_list; c0; c0 = c0->next) {
	static char long_filename[MFNLEN];

        p = wfdb_filename;
	wfdb = c0->prefix;
	while (*wfdb && p < wfdb_filename+MFNLEN-20) {
	  if (*wfdb == '%') {
		/* Perform substitutions in the WFDB path where '%' is found */
		wfdb++;
		if (*wfdb == 'r') {
		    /* '%r' -> record name */
		    (void)strcpy(p, irec);
		    p += strlen(p);
		    wfdb++;
		}
		else if ('1' <= *wfdb && *wfdb <= '9' && *(wfdb+1) == 'r') {
		    /* '%Nr' -> first N characters of record name */
		    int n = *wfdb - '0';
		    int len = strlen(irec);

		    if (len < n) n = len;
		    (void)strncpy(p, irec, n);
		    p += n;
		    *p = '\0';
		    wfdb += 2;
		}
		else    /* '%X' -> X, if X is neither 'r', nor a non-zero digit
			   followed by 'r' */
		    *p++ = *wfdb++;
	    }
	    else *p++ = *wfdb++;
	}
	/* Unless the WFDB component was empty, or it ended with a directory
	   separator, append a directory separator to wfdb_filename;  then
	   append the record and type components.  Note that names of remote
	   files (URLs) are always constructed using '/' separators, even if
	   the native directory separator is '\' (MS-DOS) or ':' (Macintosh).
	*/
	if (p != wfdb_filename) {
	    if (c0->type == WFDB_NET) {
		if (*(p-1) != '/') *p++ = '/';
	    }
#ifndef MSDOS
	    else if (*(p-1) != DSEP)
#else
	    else if (*(p-1) != DSEP && *(p-1) != ':')
#endif
		*p++ = DSEP;
	}
	if (p + strlen(r) + (s ? strlen(s) : 0) > wfdb_filename + MFNLEN-5)
	    continue;	/* name too long -- skip */
	spr1(p, r, s);
	if ((ifile = wfdb_fopen(wfdb_filename, RB)) != NULL) {
	    /* Found it! Add its path info to the WFDB path. */
	    wfdb_addtopath(wfdb_filename);
	    SFREE(r);
	    return (ifile);
	}
	/* Not found -- try again, using an alternate form of the name,
	   provided that that form is distinct. */
	strcpy(long_filename, wfdb_filename);
	spr2(p, r, s);
	if (strcmp(wfdb_filename, long_filename) && 
	    (ifile = wfdb_fopen(wfdb_filename, RB)) != NULL) {
	    wfdb_addtopath(wfdb_filename);
	    SFREE(r);
	    return (ifile);
	}
    }
    /* If the file was not found in any of the directories listed in wfdb,
       return a null file pointer to indicate failure. */
    SFREE(r);
    return (NULL);
}

/* wfdb_checkname checks record and annotator names -- they must not be empty,
   and they must contain only letters, digits, hyphens, tildes, underscores, and
   directory separators. */

int wfdb_checkname(char *p, char *s)
{
    do {
	if (('0' <= *p && *p <= '9') || *p == '_' || *p == '~' || *p== '-' ||
	    *p == DSEP ||
#ifdef MSDOS
	    *p == ':' || *p == '/' ||
#endif
	    ('a' <= *p && *p <= 'z') || ('A' <= *p && *p <= 'Z'))
	    p++;
	else {
	    wfdb_error("init: illegal character %d in %s name\n", *p, s);
	    return (-1);
	}
    } while (*p);
    return (0);
}

/* wfdb_setirec saves the current record name (its argument) in irec (defined
above) to be substituted for '%r' in the WFDB path by wfdb_open as necessary.
wfdb_setirec is invoked by isigopen (except when isigopen is invoked
recursively to open a segment within a multi-segment record) and by annopen
(when it is about to open a file for input). */

void wfdb_setirec(const char *p)
{
    const char *r;
    int len;

    for (r = p; *r; r++)
	if (*r == DSEP) p = r+1;	/* strip off any path information */
#ifdef MSDOS
	else if (*r == ':') p = r+1;
#endif
    len = strlen(p);
    if (len > WFDB_MAXRNL)
	len = WFDB_MAXRNL;
    if (strcmp(p, "-")) {       /* don't record '-' (stdin) as record name */
	strncpy(irec, p, len);
	irec[len] = '\0';
    }
}

char *wfdb_getirec(void)
{
    return (*irec ? irec: NULL);
}

/* Remove trailing '.hea' from a record name, if present. */
void wfdb_striphea(char *p)
{
    if (p) {
	int len = strlen(p);

	if (len > 4 && strcmp(p + len-4, ".hea") == 0)
	    p[len-4] = '\0';
    }
}


/* WFDB file I/O functions

The WFDB library normally reads and writes local files.  If libcurl
(http://curl.haxx.se/) or libwww (http://www.w3.org/Library) is available,
the WFDB library can also read files from any accessible World Wide Web (HTTP)
or FTP server.  (Writing files to a remote WWW or FTP server may be
supported in the future.)

If you do not wish to allow access to remote files, or if neither libcurl nor
libwww is available, simply define the symbol WFDB_NETFILES as 0 when compiling
the WFDB library.  If the symbol WFDB_NETFILES is zero, wfdblib.h defines
wfdb_fread as fread, wfdb_fwrite as fwrite, etc.;  thus in this case, the
I/O is performed using the standard C I/O functions, and the function
definitions in the next section are not compiled.  This behavior exactly
mimics that of versions of the WFDB library earlier than version 10.0.1
(which did not support remote file access), with no additional run-time
overhead.

If WFDB_NETFILES is non-zero, however, these functions are compiled.  The
WFDB_FILE pointers that are among the arguments to these functions point to
objects that may contain either (local) FILE handles or (remote) NETFILE
handles, depending on the value of the 'type' member of the WFDB_FILE object.
All access to local files is handled by passing the 'fp' member of the
WFDB_FILE object to the appropriate standard C I/O function.  Access to remote
files via http or ftp is handled by passing the 'netfp' member of the WFDB_FILE
object to the appropriate libcurl or libwww function(s).

In order to read remote files, the WFDB environment variable should include
one or more components that specify http:// or ftp:// URL prefixes.  These
components are concatenated with WFDB file names to obtain complete URLs.  For
example, if the value of WFDB is
  /usr/local/database http://dilbert.bigu.edu/wfdb /cdrom/database
then an attempt to read the header file for record xyz would look first for
/usr/local/database/xyz.hea, then http://dilbert.bigu.edu/wfdb/xyz.hea, and
finally /cdrom/database/xyz.hea.  The second and later possibilities would
be checked only if the file had not been found already.  As a practical matter,
it would be best in almost all cases to search all available local file systems
before looking on remote http or ftp servers, but the WFDB library allows you
to set the search order in any way you wish, as in this example.
*/

#if WFDB_NETFILES

static int nf_open_files = 0;		/* number of open netfiles */
static long page_size = NF_PAGE_SIZE;	/* bytes per range request (0: disable
					   range requests) */
static int www_done_init = FALSE;	/* TRUE once libcurl or libwww is
					   initialized */

#if WFDB_NETFILES_LIBCURL
static CURL *curl_ua = NULL;

/* Construct the User-Agent string to be sent with HTTP requests. */
static char *curl_get_ua_string(void)
{
    char *libcurl_ver;
    static char *s = NULL;

    libcurl_ver = curl_version();
    SALLOC(s, 1, 32 + strlen(libcurl_ver));
    sprintf(s, "libwfdb/%d.%d.%d (%s)", WFDB_MAJOR, WFDB_MINOR,
		WFDB_RELEASE, libcurl_ver);
    return (s);
}

static char curl_error_buf[CURL_ERROR_SIZE];

/* This function will print out the curl error message if there was an
   error.  Zero means there was no error. */
static int curl_try(CURLcode err)
{
    if (err) {
      wfdb_error("curl error: %s\n", curl_error_buf);
    }
    return err;
}

struct chunk {
    long size, buffer_size;
    char *data;
};

/* This is a dummy write callback, for when we don't care about the
   data curl is receiving. */

static size_t curl_null_write(void *ptr, size_t size, size_t nmemb,
			      void *stream)
{
    return (size*nmemb);
}

typedef struct chunk CHUNK;
#define chunk_size(x) ((x)->size)
#define chunk_data(x) ((x)->data)
#define chunk_new curl_chunk_new
#define chunk_delete curl_chunk_delete
#define chunk_putb curl_chunk_putb
#else
#define CHUNK HTChunk
#define chunk_size HTChunk_size
#define chunk_data HTChunk_data
#define chunk_new HTChunk_new
#define chunk_delete HTChunk_delete
#define chunk_putb HTChunk_putb
#endif

static char **passwords;

/* www_parse_passwords parses the WFDBPASSWORD environment variable.
This environment variable contains a list of URL prefixes and
corresponding usernames/passwords.  Alternatively, the environment
variable may contain '@' followed by the name of a file containing
password information.

Each item in the list consists of a URL prefix, followed by a space,
then the username and password separated by a colon.  For example,
setting WFDBPASSWORD to "https://example.org john:letmein" would use
the username "john" and the password "letmein" for all HTTPS requests
to example.org.

If there are multiple items in the list, they must be separated by
end-of-line or tab characters. */
static void www_parse_passwords(const char *str)
{
    static char sep[] = "\t\n\r";
    char *xstr = NULL, *p, *q;
    int n;

    SSTRCPY(xstr, str);
    if (!xstr)
	return;
    if (*xstr == '@')
	xstr = wfdb_getiwfdb(xstr);

    SALLOC(passwords, 1, sizeof(char *));
    n = 0;
    for (p = strtok(xstr, sep); p; p = strtok(NULL, sep)) {
	if (!(q = strchr(p, ' ')) || !strchr(q, ':'))
	    continue;
	SREALLOC(passwords, n + 2, sizeof(char *));
	if (!passwords)
	    return;
	SSTRCPY(passwords[n], p);
	n++;
    }
    passwords[n] = NULL;

    SFREE(xstr);
}

/* www_userpwd determines which username/password should be used for a
given URL.  It returns a string of the form "username:password" if one
is defined, or returns NULL if no login information is required for
that URL. */
static const char *www_userpwd(const char *url)
{
    int i, n;
    const char *p;

    for (i = 0; passwords && passwords[i]; i++) {
	p = strchr(passwords[i], ' ');
	if (!p || p == passwords[i])
	    continue;

	n = p - passwords[i];
	if (strncmp(passwords[i], url, n) == 0 &&
	    (url[n] == 0 || url[n] == '/' || url[n - 1] == '/')) {
	    return &passwords[i][n + 1];
	}
    }

    return NULL;
}

static void wfdb_wwwquit(void)
{
    int i;
    if (www_done_init) {
#if WFDB_NETFILES_LIBCURL
# ifndef _WINDOWS
	curl_easy_cleanup(curl_ua);
	curl_ua = NULL;
	curl_global_cleanup();
# endif
#else
#ifdef USEHTCACHE
	HTCacheTerminate();
#endif
	HTProfile_delete();
#endif
	www_done_init = FALSE;
	for (i = 0; passwords && passwords[i]; i++)
	    SFREE(passwords[i]);
	SFREE(passwords);
    }
}

static void www_init(void)
{
    if (!www_done_init) {
	char *p, version[20];

	if ((p = getenv("WFDB_PAGESIZE")) && *p)
	    page_size = strtol(p, NULL, 10);

#if WFDB_NETFILES_LIBCURL
	/* Initialize the curl "easy" handle. */
	curl_global_init(CURL_GLOBAL_ALL);
	curl_ua = curl_easy_init();
	/* Buffer for error messages */
	curl_easy_setopt(curl_ua, CURLOPT_ERRORBUFFER, curl_error_buf);
	/* Return an error code when the server replies with status >= 400 */
	curl_easy_setopt(curl_ua, CURLOPT_FAILONERROR, 1L);
	/* String to send as a User-Agent header */
	curl_easy_setopt(curl_ua, CURLOPT_USERAGENT, curl_get_ua_string());
#ifdef USE_NETRC
	/* Search $HOME/.netrc for passwords */
	curl_easy_setopt(curl_ua, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
#endif
	/* Get password information from the environment if available */
	if ((p = getenv("WFDBPASSWORD")) && *p)
            www_parse_passwords(p);

	/* Get the name of the CA bundle file */
	if ((p = getenv("CURL_CA_BUNDLE")) && *p)
	    curl_easy_setopt(curl_ua, CURLOPT_CAINFO, p);

	/* Use any available authentication method */
	curl_easy_setopt(curl_ua, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

	/* Show details of URL requests if WFDB_NET_DEBUG is set */
	if ((p = getenv("WFDB_NET_DEBUG")) && *p)
	    curl_easy_setopt(curl_ua, CURLOPT_VERBOSE, 1L);
#else

#ifdef USEHTCACHE
	char *cachedir = CACHEDIR;	/* root of the netfile data cache */
	int cachesize = CACHESIZE;	/* maximum size of the cache in MB */
	int entrysize = ENTRYSIZE;	/* maximum cache entry size in MB */

	if ((p = getenv("WFDB_CACHEDIR")) && *p)
	    cachedir = p;
	if ((p = getenv("WFDB_CACHESIZE")) && *p)
	    cachesize = strtol(p, NULL, 10);
	if ((p = getenv("WFDB_CACHEENTRYSIZE")) && *p)
	    entrysize = strtol(p, NULL, 10);
#endif
	sprintf(version, "%d.%d.%d", WFDB_MAJOR, WFDB_MINOR, WFDB_RELEASE);
	HTProfile_newPreemptiveClient("WFDB", version);
	HTAlert_setInteractive(NO);
#ifdef USEHTCACHE
	HTLib_setSecure(TRUE);
	HTCacheInit(cachedir, cachesize);
	HTCacheMode_setMaxCacheEntrySize(entrysize);
#endif
	/*	HTHost_setMaxPipelinedRequests(1);	*/
	HTEventInit();	/* added 19 July 2001 -- necessary for use with
			   WINSOCK, seems to be harmless otherwise */
#endif
	atexit(wfdb_wwwquit);
	www_done_init = TRUE;
    }
}

#if WFDB_NETFILES_LIBCURL
/* This function is called when a header is received.  ptr points to
   the string received; size*nmemb is the number of bytes, and stream
   is the pointer specified as CURLOPT_WRITEHEADER. */
static size_t curl_header_length_write(void *ptr, size_t size, size_t nmemb,
				       void *stream)
{
    char *s = (char *) ptr;
    double *d = (double *) stream;

    if (0 == strncasecmp(s, "Content-Length:", 15)) {
	sscanf(s + 15, "%lf", d);
    }
    return size*nmemb;
}
#endif

static long www_get_cont_len(const char *url)
{
#if WFDB_NETFILES_LIBCURL
    static double length;

    length = 0;
    if (/* We just want the content length; NOBODY means we want to
	   send a HEAD request rather than GET */
	curl_try(curl_easy_setopt(curl_ua, CURLOPT_NOBODY, 1L))
	/* Set the URL to retrieve */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_URL, url))
	/* Set username/password */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_USERPWD,
	                             www_userpwd(url)))
	/* Don't send a range request */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_RANGE, NULL))
	/* If any body data is received, ignore it */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEFUNCTION,
				     curl_null_write))
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEDATA, NULL))
	/* Process received headers using curl_header_length_write */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_HEADERFUNCTION,
				     curl_header_length_write))
	/* Set the user data for curl_header_length_write */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEHEADER,
				     &length))
	/* Actually perform the request and wait for a response */
	|| curl_easy_perform(curl_ua))
	return 0;

    return (long) length;
#else
    HTRequest *request = NULL;
    HTParentAnchor *a = NULL;
    HTAssocList *headers = NULL;
    HTAssoc *pres = NULL;
    long length = 0L;

    if (url && *url && (request = HTRequest_new())) {
	HTHeadAbsolute(url, request);
	if ((a = HTRequest_anchor(request)) && (headers = HTAnchor_header(a)))
	    while ((pres = (HTAssoc *)HTAssocList_nextObject(headers)))
		if (HTStrCaseMatch("Content-Length", HTAssoc_name(pres)))
		    length = strtol(HTAssoc_value(pres), NULL, 10);
	HTRequest_delete(request);  
    }
    return (length);
#endif
}

#if WFDB_NETFILES_LIBCURL
/* Create a new, empty chunk. */
static CHUNK *curl_chunk_new(long len)
{
    struct chunk *c;

    SUALLOC(c, 1, sizeof(struct chunk));
    SALLOC(c->data, 1, len);
    c->size = 0L;
    c->buffer_size = len;
    return c;
}

/* Delete a chunk */
static void curl_chunk_delete(struct chunk *c)
{
    if (c) {
	SFREE(c->data);
	SFREE(c);
    }
}

/* Write data into a chunk.  This function is called by curl and must
   take the same arguments as fwrite().  ptr points to the data
   received, size*nmemb is the number of bytes, and stream is the user
   data specified by CURLOPT_WRITEDATA. */
static size_t curl_chunk_write(void *ptr, size_t size, size_t nmemb,
			       void *stream)
{
    size_t count=0;
    char *p;
    struct chunk *c = (struct chunk *) stream;

    while (nmemb > 0) {
	while ((c->size + size) > c->buffer_size) {
	    c->buffer_size += 1024;
	    SREALLOC(c->data, 1, c->buffer_size);
	}
	memcpy(c->data + c->size, ptr, size);
	c->size += size;
	count += size;
	p = (char *)ptr + size;	/* avoid arithmetic on void pointer */
	ptr = (void *)p;
	nmemb--;
    }
    return (count);
}

/* This function emulates the libwww function HTChunk_putb. */
static void curl_chunk_putb(struct chunk *chunk, char *data, size_t len)
{
    curl_chunk_write(data, 1, len, chunk);
}
#endif

static CHUNK *www_get_url_range_chunk(const char *url, long startb, long len)
{
#if !WFDB_NETFILES_LIBCURL
    HTRequest *request = NULL;
    HTList *request_err = NULL;
    HTError *err = NULL;
#endif
    CHUNK *chunk = NULL, *extra_chunk = NULL;
    char range_req_str[6*sizeof(long) + 2];

    if (url && *url) {
	sprintf(range_req_str, "%ld-%ld", startb, startb+len-1);
#if WFDB_NETFILES_LIBCURL
	chunk = chunk_new(len);

	if (/* In this case we want to send a GET request rather than
	       a HEAD */
	    curl_try(curl_easy_setopt(curl_ua, CURLOPT_NOBODY, 0L))
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_HTTPGET, 1L))
	    /* URL to retrieve */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_URL, url))
	    /* Set username/password */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_USERPWD,
	                                 www_userpwd(url)))
	    /* Range request */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_RANGE,
					 range_req_str))
	    /* This function will be used to "write" data as it is received */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEFUNCTION,
					 curl_chunk_write))
	    /* The pointer to pass to the write function */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEDATA, chunk))
	    /* Don't bother writing the header data anywhere */
	    || curl_try(curl_easy_setopt(curl_ua, CURLOPT_HEADERFUNCTION,
					 curl_null_write))
	    /* Perform the request */
	    || curl_try(curl_easy_perform(curl_ua))) {

	    chunk_delete(chunk);
	    return NULL;
	}

#else
	request = HTRequest_new();
	HTRequest_addRange(request, "bytes", range_req_str);
	HTRequest_setOutputFormat(request, WWW_SOURCE);
	HTRequest_setPreemptive(request, YES);
	chunk = HTLoadToChunk(url, request);
	request_err = HTRequest_error(request);
	while ((err = (HTError *) HTList_nextObject(request_err)) != NULL) {
	    if (HTError_severity(err) == ERR_FATAL) {
		wfdb_error(
		  "www_get_url_range_chunk: fatal error requesting %s (%s)\n",
		  url, range_req_str);
		if (chunk) {
		    chunk_delete(chunk);
		    chunk = NULL;
		}
	    }
	}
#endif
	if (chunk && (chunk_size(chunk) > len)) {
	    /* We received a larger chunk than requested. */
	    if (chunk_size(chunk) >= startb + len) {
		/* If the chunk is large enough to include the requested range
		   and everything before it, assume that that's what we have
		   (it may be the entire file).  This might happen if the
		   file was in the cache or if the server does not support the
		   range request.  Since the caller expects only a chunk of
		   len bytes beginning with the data of interest, we need to
		   create a new chunk of the proper length, fill it, and

		   return it to the caller.  HTChunk_new makes a new chunk,
		   which grows as needed in multiples of its argument (in
		   bytes). */
		extra_chunk = chunk_new(len);
		/* Copy the desired range out of the chunk we received into the
		   new chunk. */
		chunk_putb(extra_chunk, &chunk_data(chunk)[startb], len);
		/* Discard the chunk we received. */
		chunk_delete(chunk);
		/* Arrange for the new chunk to be returned. */
		chunk = extra_chunk;
	    }
	    else {
		/* We received some chunk of the file (not the whole file, but
		   not what we requested, either).  Since we don't know what
		   we have, let's try again, but only once. */
		static int retry = 1;

		if (retry) {
		    retry = 0;
#if !WFDB_NETFILES_LIBCURL
		    HTRequest_delete(request);
#endif
		    fflush(stderr);
		    chunk = www_get_url_range_chunk(url, startb, len);
		    retry = 1;
		    return (chunk);
		}
		else {
		    /* We did no better the second time, so let's return an
		       error to the caller. */
		    wfdb_error(
		   "www_get_url_range_chunk: fatal error requesting %s (%s)\n",
		   url, range_req_str);
		    if (chunk) {
			chunk_delete(chunk);
			chunk = NULL;
		    }
		    retry = 1;
		}
	    }
	}
#if !WFDB_NETFILES_LIBCURL
	HTRequest_delete(request);
#endif
    }
    return (chunk);
}

static CHUNK *www_get_url_chunk(const char *url)
{
    CHUNK *chunk = NULL;

#if WFDB_NETFILES_LIBCURL
    chunk = chunk_new(1024);

    if (/* Send a GET request */
	curl_try(curl_easy_setopt(curl_ua, CURLOPT_NOBODY, 0L))
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_HTTPGET, 1L))
	/* URL to retrieve */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_URL, url))
	/* No range request */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_RANGE, NULL))
	/* Write to the chunk specified ... */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEFUNCTION,
				     curl_chunk_write))
	/* ... by this pointer */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_HEADERFUNCTION,
				     curl_null_write))
	/* and ignore the header data */
	|| curl_try(curl_easy_setopt(curl_ua, CURLOPT_WRITEDATA, chunk))
	/* perform the request */
	|| curl_easy_perform(curl_ua)) {

	chunk_delete(chunk);
	return NULL;
    }

#else
    HTRequest *request = NULL;
    HTList *request_err = NULL;
    HTError *err = NULL;

    if (url && *url) {
	request = HTRequest_new();
	HTRequest_setOutputFormat(request, WWW_SOURCE);
	HTRequest_setPreemptive(request, YES);
	chunk = HTLoadToChunk(url, request);
	request_err = HTRequest_error(request);
	while ((err = (HTError *) HTList_nextObject(request_err)) != NULL) {
	    if (HTError_severity(err) == ERR_FATAL) {
		/* This occurs if the remote file doesn't exist.  This happens
		   routinely while searching the WFDB path, so it's not flagged
		   as a WFDB library error. */
		if (chunk) {
		    chunk_delete(chunk);
		    chunk = NULL;
		}
	    }
	}
	HTRequest_delete(request);
    }
#endif
    return (chunk);
}

static void nf_delete(netfile *nf)
{
    if (nf) {
	SFREE(nf->url);
	SFREE(nf->data);
	SFREE(nf);
    }
}

/* nf_new attempts to read (at least part of) the file named by its
   argument (normally an http:// or ftp:// url).  If page_size is nonzero and
   the file can be read in segments (this will be true for files served by http
   servers that support range requests, and possibly for other types of files
   if NETFILES support is available), nf_new reads the first page_size bytes
   (or fewer, if the file is shorter than page_size).  Otherwise, nf_new
   attempts to read the entire file into memory.  If there is insufficient
   memory, if the file contains no data, or if the file does not exist (the
   most common of these three cases), nf_new returns a NULL pointer; otherwise,
   it allocates, fills in, and returns a pointer to a netfile structure that
   can be used by nf_fread, etc., to obtain the contents of the file.

   It would be useful to be able to copy a file that cannot be read in segments
   to a local file, but this operation is not currently implemented.  The way
   to do this would be to invoke
      HTLoadToFile(url, request, filename);
*/
static netfile *nf_new(const char* url)
{
    netfile *nf;
    CHUNK *chunk = NULL;
    long bytes_received = 0L;

    SUALLOC(nf, 1, sizeof(netfile));
    if (nf && url && *url) {
	SSTRCPY(nf->url, url);
	nf->base_addr = 0;
	nf->pos = 0;
	nf->data = NULL;
	nf->err = NF_NO_ERR;
	nf->fd = -1;

	/* First try to get the content length.  If it works, we're probably
	   trying to read an http url. */
	if (page_size > 0L && (nf->cont_len = www_get_cont_len(url))) {
	    /* Success!  Let's try a range request now. */
	    long bytes_requested = nf->cont_len;
	    if (bytes_requested > page_size) bytes_requested = page_size;
	    if (chunk = www_get_url_range_chunk(nf->url,0L, bytes_requested)) {
		bytes_received = chunk_size(chunk);
	        if (bytes_requested != bytes_received)
		    wfdb_error(
			      "nf_new: requested %ld, got %ld bytes from %s\n",
			      bytes_requested, bytes_received, url);
		if (bytes_received > 0)
		    nf->mode = NF_CHUNK_MODE;
	    }
	}
	if (bytes_received == 0L && (chunk = www_get_url_chunk(nf->url))) {
	    nf->mode = NF_FULL_MODE;
	    bytes_received = nf->cont_len = chunk_size(chunk);
	}
	if (bytes_received > 0L) {
	    SALLOC(nf->data, bytes_received, sizeof(char));
	    memcpy(nf->data, chunk_data(chunk), bytes_received);
	}
	if (nf->data == NULL) {
	    if (bytes_received > 0L)
		wfdb_error("nf_new: insufficient memory (needed %ld bytes)\n",
			   bytes_received);
	    /* If no bytes were received, the remote file probably doesn't
	       exist.  This happens routinely while searching the WFDB path, so
	       it's not flagged as an error.  Note, however, that we can't tell
	       the difference between a nonexistent file and an empty one.
	       Another possibility is that range requests are unsupported
	       (e.g., if we're trying to read an ftp url), and there is
	       insufficient memory for the entire file. */
	    nf_delete(nf);
	    nf = NULL;
	}
	if (chunk) chunk_delete(chunk);
    }
    return(nf);
}

static long nf_get_range(netfile* nf, long startb, long len, char *rbuf)
{
    CHUNK *chunk = NULL;
    char *rp;
    long avail = nf->cont_len - startb;

    if (len > avail) len = avail;	/* limit request to available bytes */
    if (nf == NULL || nf->url == NULL || *nf->url == '\0' ||
	startb < 0L || startb >= nf->cont_len || len <= 0L || rbuf == NULL)
	return (0L);	/* invalid inputs -- fail silently */

    if (nf->mode == NF_CHUNK_MODE) {	/* range requests acceptable */
	long rlen = (avail >= page_size) ? page_size : avail;

	if (len <= page_size) {	/* short request -- check if cached */
	    if ((startb < nf->base_addr) ||
		((startb + len) > (nf->base_addr + page_size))) {
		/* requested data not in cache -- update the cache */
		if (chunk = www_get_url_range_chunk(nf->url, startb, rlen)) {
		    if (chunk_size(chunk) != rlen) {
			wfdb_error(
		     "nf_get_range: requested %ld bytes, received %ld bytes\n",
		                   rlen, (long)chunk_size(chunk));
			len = 0L;
		    }
		    else {
			nf->base_addr = startb;
			memcpy(nf->data, chunk_data(chunk), rlen);
		    }
		}
		else {	/* attempt to update cache failed */
		    wfdb_error(
	     "nf_get_range: couldn't read %ld bytes of %s starting at %ld\n", 
	                       len, nf->url, startb);
		    len = 0L;
		}
	    } 
      
	    /* move cached data to the return buffer */
	    rp = nf->data + startb - nf->base_addr;
	}

	else if (chunk = www_get_url_range_chunk(nf->url, startb, len)) {
	    /* long request (> page_size) */
	    if (chunk_size(chunk) != len) {
		wfdb_error(
		       "nf_get_range: requested %d bytes, received %d bytes\n",
		           len, (long)chunk_size(chunk));
	    }
	    rp = chunk_data(chunk);
	}
    }

    else  /* cannot use range requests -- cache contains full file */
	rp = nf->data + startb;		

    memcpy(rbuf, rp, len);
    if (chunk) chunk_delete(chunk);
    return (len);
}

/* nf_feof returns true after reading past the end of a file but before
   repositioning the pos in the file. */
static int nf_feof(netfile *nf)
{
    return ((nf->err == NF_EOF_ERR) ? TRUE : FALSE);
}

/* nf_eof returns true if the file pointer is at the EOF. */
static int nf_eof(netfile *nf)
{
    return (nf->pos >= nf->cont_len) ? TRUE : FALSE;
}

static netfile* nf_fopen(const char *url, const char *mode)
{
    netfile* nf = NULL;

    if (!www_done_init)
	www_init();
    if (*mode == 'w' || *mode == 'a')
	errno = EROFS;	/* no support for output */
    else if (*mode != 'r')
	errno = EINVAL;	/* invalid mode string */
    else if (nf = nf_new(url))
	nf_open_files++;
    return (nf);
}

static int nf_fclose(netfile* nf)
{
    nf_delete(nf);
    nf_open_files--;
    return (0);
}

static int nf_fgetc(netfile *nf)
{
    char c;

    if (nf_get_range(nf, nf->pos++, 1, &c))
	return (c & 0xff);
    nf->err = (nf->pos >= nf->cont_len) ? NF_EOF_ERR : NF_REAL_ERR;
    return (EOF);
}

static char* nf_fgets(char *s, int size, netfile *nf)
{
    int c = 0, i = 0; 

    if (s == NULL) return (NULL);
    while (c != '\n' && i < (size-1) && (c = nf_fgetc(nf)) != EOF)
	s[i++] = c;
    if ((c == EOF) && (i == 0))	return (NULL);
    s[i] = 0;
    return (s);
}

static size_t nf_fread(void *ptr, size_t size, size_t nmemb, netfile *nf)
{
    long bytes_available, bytes_read = 0L, bytes_requested = size * nmemb;

    if (nf == NULL || ptr == NULL || bytes_requested == 0) return ((size_t)0);
    bytes_available = nf->cont_len - nf->pos;
    if (bytes_requested > bytes_available) bytes_requested = bytes_available;
    if (bytes_requested > page_size && page_size) bytes_requested = page_size;
    nf->pos += bytes_read = nf_get_range(nf, nf->pos, bytes_requested, ptr);
    return ((size_t)(bytes_read / size));
}

static int nf_fseek(netfile* nf, long offset, int whence)
{
  int ret = -1;

  if (nf)
    switch (whence) {
      case SEEK_SET:
	if (offset < nf->cont_len) {
	  nf->pos = offset;
	  nf->err = NF_NO_ERR;
	  ret = 0;
	}
	break;
      case SEEK_CUR:
	if ((nf->pos + offset) < nf->cont_len) {
	  nf->pos += offset;
	  nf->err = NF_NO_ERR;
	  ret = 0;
	}
	break;
      case SEEK_END:
	if (((nf->cont_len + offset) >= 0) &&
	    ((nf->cont_len + offset) <= nf->cont_len)) {
	  nf->pos = nf->cont_len + offset;
	  nf->err = NF_NO_ERR;
	  ret = 0; 
	}
	break;
      default:
	break;
    }
  return (ret);
}

static long nf_ftell(netfile *nf)
{
    return (nf->pos);
}

static int nf_ferror(netfile *nf)
{
    return ((nf->err == NF_REAL_ERR) ? TRUE : FALSE);
}

static void nf_clearerr(netfile *nf)
{
    nf->err = NF_NO_ERR;
}

static int nf_fflush(netfile *nf)
{
    /* no support yet for writing to remote files */
    errno = EROFS;
    return (EOF);
}

static size_t nf_fwrite(const void *ptr, size_t size, size_t nmemb,netfile *nf)
{
    /* no support yet for writing to remote files */
    errno = EROFS;
    return (0);
}

static int nf_putc(int c, netfile *nf)
{
    /* no support yet for writing to remote files */
    errno = EROFS;
    return (EOF);
}

/* The definition of nf_vfprintf (which is a stub) has been moved;  it is
   now just before wfdb_fprintf, which refers to it.  There is no completely
   portable way to make a forward reference to a static (local) function. */

void wfdb_clearerr(WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_clearerr(wp->netfp));
    return (clearerr(wp->fp));
}

int wfdb_feof(WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_feof(wp->netfp));
    return (feof(wp->fp));
}

int wfdb_ferror(WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_ferror(wp->netfp));
    return (ferror(wp->fp));
}

int wfdb_fflush(WFDB_FILE *wp)
{
    if (wp == NULL) {	/* flush all WFDB_FILEs */
	nf_fflush(NULL);
	return (fflush(NULL));
    }
    else if (wp->type == WFDB_NET)
	return (nf_fflush(wp->netfp));
    else
	return (fflush(wp->fp));
}

char* wfdb_fgets(char *s, int size, WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_fgets(s, size, wp->netfp));
    return (fgets(s, size, wp->fp));
}

size_t wfdb_fread(void *ptr, size_t size, size_t nmemb, WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_fread(ptr, size, nmemb, wp->netfp));
    return (fread(ptr, size, nmemb, wp->fp));
}

int wfdb_fseek(WFDB_FILE *wp, long int offset, int whence)
{
    if (wp->type == WFDB_NET)
	return (nf_fseek(wp->netfp, offset, whence));
    return(fseek(wp->fp, offset, whence));
}

long wfdb_ftell(WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_ftell(wp->netfp));
    return (ftell(wp->fp));
}

size_t wfdb_fwrite(void *ptr, size_t size, size_t nmemb, WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_fwrite(ptr, size, nmemb, wp->netfp));
    return (fwrite(ptr, size, nmemb, wp->fp));
}

int wfdb_getc(WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_fgetc(wp->netfp));
    return (getc(wp->fp));
}

int wfdb_putc(int c, WFDB_FILE *wp)
{
    if (wp->type == WFDB_NET)
	return (nf_putc(c, wp->netfp));
    return (putc(c, wp->fp));
}

#endif	/* WFDB_NETFILES */

int wfdb_fclose(WFDB_FILE *wp)
{
    int status;

#if WFDB_NETFILES
    status = (wp->type == WFDB_NET) ? nf_fclose(wp->netfp) : fclose(wp->fp);
#else
    status = fclose(wp->fp);
#endif
    if (wp->fp != stdin)
	SFREE(wp);
    return (status);
}

WFDB_FILE *wfdb_fopen(char *fname, const char *mode)
{
    char *p = fname;
    WFDB_FILE *wp;

    if (p == NULL || strstr(p, ".."))
	return (NULL);
    SUALLOC(wp, 1, sizeof(WFDB_FILE));
    while (*p)
	if (*p++ == ':' && *p++ == '/' && *p++ == '/') {
#if WFDB_NETFILES
	    if (wp->netfp = nf_fopen(fname, mode)) {
		wp->type = WFDB_NET;
		return (wp);
	    }
#endif
	    SFREE(wp);
	    return (NULL);
	}
    if (wp->fp = fopen(fname, mode)) {
	wp->type = WFDB_LOCAL;
	return (wp);
    }
    if (strcmp(mode, WB) == 0 || strcmp(mode, AB) == 0) {
        int stat = 1;

	/* An attempt to create an output file failed.  Check to see if all
	   of the directories in the path exist, create them if necessary
	   and possible, then try again. */
        for (p = fname; *p; p++)
	    if (*p == '/') {	/* only Unix-style directory separators */
		*p = '\0';
		stat = MKDIR(fname, 0755);
		/* MKDIR is defined as mkdir in wfdblib.h;  depending on
		   the platform, mkdir may take two arguments (POSIX), or
		   only the first argument (MS-Windows without Cygwin).
		   The '0755' means that (under Unix), the directory will
		   be world-readable, but writable only by the owner. */
		*p = '/';
	    }
	/* At this point, we may have created one or more directories.
	   Only the last attempt to do so matters here:  if and only if
	   it was successful (i.e., if stat is now 0), we should try again
	   to create the output file. */
	if (stat == 0 && (wp->fp = fopen(fname, mode))) {
		wp->type = WFDB_LOCAL;
		return (wp);
	}
    }
    SFREE(wp);
    return (NULL);
}

/* Miscellaneous OS-specific functions. */

#ifdef NOSTRTOK
char *strtok(char *p, char *sep)
{
    char *psep;
    static char *s;

    if (p) s = p;
    if (!s || !(*s) || !sep || !(*sep)) return ((char *)NULL);
    for (psep = sep; *psep; psep++)
	if (*s == *psep) {
	    if (!(*(++s))) return ((char *)NULL);
	    psep = sep;
	}
    p = s;
    while (*s)
	for (psep = sep; *psep; psep++)
	    if (*s == *psep) {
		*s++ == '\0';
		return (p);
	    }
    return (p);
}
#endif

#ifdef _WINDLL
#ifndef _WIN32
/* Functions named LibMain and WEP are required in all 16-bit MS Windows
dynamic link libraries. The private library function wgetenv is a replacement
for the standard C getenv function, useful since the DOS environment is not
available to Windows DLLs except by use of the Windows GetDOSEnvironment
function.
*/

FINT LibMain(HINSTANCE hinst, WORD wDataSeg, WORD cbHeapSize,
	               LPSTR lpszCmdLine)
{
    if (cbHeapSize != 0)	/* if DLL data segment is moveable */
	UnlockData(0);
    return (1);
}

FINT WEP(int nParameter)
{
    if (nParameter == WEP_SYSTEM_EXIT || WEP_FREE_DLL)
	wfdbquit();	/* Always close WFDB files before shutdown or
			   when the last WFDB application exits. */
    return (1);
}


/* This is a quick-and-dirty reimplementation of getenv for the Windows 16-bit
   DLL environment.  It searches the MSDOS environment for a line beginning
   with the specified variable name, followed by '='.  This function can be
   fooled by pathologic variable names (e.g., with embedded '=' characters),
   but should be adequate for typical use. */

char FAR *wgetenv(char far *var)
{
    char FAR *ep = GetDOSEnvironment();
    int l = _fstrlen(var);
    
    if (var == (char FAR *)NULL || *var = '\0') return ((char FAR *)NULL);
    while (*ep) {
	if (_fstrncmp(ep, var, l) == 0 && *(ep+l) == '=')
	    /* Got it!  Skip '=', return a pointer to the value string. */
	    return (ep+l+1);
	/* Go on to the next environment string. */
	ep += _fstrlen(ep) + 1;
    }

    /* If here, the specified variable was not found in the environment. */
    return ((char FAR *)NULL);
}

#else	/* 32-bit MS Windows DLLs require DllMain instead of LibMain and WEP */

int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    switch (fdwReason) {
	/* nothing to do when process (or thread) begins */
      case DLL_PROCESS_ATTACH:
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
	break;
	/* when process terminates, always close WFDB files before shutdown or
	   when the last WFDB application exits */
      case DLL_PROCESS_DETACH:
	wfdbquit();
	break;
    }
    return (TRUE);
}
#endif
#endif
