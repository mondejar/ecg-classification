/* file: wfdblib.h	G. Moody	13 April 1989
                        Last revised: 27 September 2012       wfdblib 10.5.16
External definitions for WFDB library private functions

_______________________________________________________________________________
wfdb: a library for reading and writing annotated waveforms (time series data)
Copyright (C) 1989-2012 George B. Moody

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

These definitions may be included in WFDB library source files to permit them
to share information about library private functions, which are not intended
to be invoked directly by user programs.  By convention, all externally defined
symbols reserved to the library begin with the characters "wfdb_".
*/

#include "wfdb.h"

/* MS-Windows users are strongly advised to compile the WFDB library using the
   free Cygwin tools (www.cygwin.com).  Use of other compilers may be possible
   but is not supported.

   Cygwin users:  Note that the symbols _WINDOWS, _WINDLL, WIN32, etc. are
   not defined and should not be defined when compiling the WFDB software
   package using gcc.  Cygwin's POSIX emulation layer avoids the need for
   the various workarounds required when using other compilers under
   MS-Windows.

   If you are using another MS-Windows compiler, you will need these
   workarounds, and you should be careful to define _WINDOWS, _WINDLL, and
   _WIN32 as appropriate.  Older versions of the WFDB library were successfully
   compiled under MS-DOS and under 16-bit versions of MS-Windows, but the
   conditionally compiled code written to support these platforms has not been
   tested recently.

   Define the symbol _WINDLL if this library is to be compiled as a Microsoft
   Windows DLL.  Note that a DLL's private functions (such as those listed
   below) *cannot* be called by Windows user programs, which can call only DLL
   functions that use the Pascal calling interface (see wfdb.h).  To compile
   this library for use in a Microsoft Windows application, but *not* as a DLL,
   define the symbol _WINDOWS instead of _WINDLL. */
/* #define _WINDLL */

#if defined(_WINDLL) && !defined(_WINDOWS)
#define _WINDOWS
#endif

#if defined(_WIN16) && !defined(_WINDOWS)
#define _WINDOWS
#endif

#if defined(_WIN32) && !defined(_WINDOWS)
#define _WINDOWS
#endif

#if defined(_WINDOWS) && !defined (_WIN16) && !defined(_WIN32)
#define _WIN16
#endif

/* Define the symbol MSDOS if this library is to be used under MS-DOS or MS
   Windows.  Note: MSDOS is predefined by some MS-DOS and MS Windows compilers.
*/
#if defined(_WINDOWS)
# if !defined(MSDOS)
#  define MSDOS
# endif
#endif

/* Macintosh users are strongly advised to use Mac OS X or later.  If you are
   doing so, no special configuration is needed, and you may ignore this
   section.

   If you are running Mac OS 9 or earlier, you may be able to use the WFDB
   software package, but this configuration has not been tested for many years,
   and is not supported!  If you would like to try it, define MAC. */
/* #define MAC */

/* DEFWFDB is the default value of the WFDB path if the WFDB environment
   variable is not set.  This value is edited by the configuration script
   (../configure), which also edits this block of comments to match.

   If WFDB_NETFILES support is disabled, the string ". /usr/local/database" is
   usually sufficient for a default WFDB path, thus restricting the search for
   WFDB files to the current directory ("."), followed by /usr/local/database).
   
   If WFDB_NETFILES support is enabled, the first setting below adds the
   web-accessible PhysioBank databases to the default path; you may wish to
   change this to use a nearby PhysioNet mirror (for a list of mirrors, see
   http://physionet.org/mirrors/).  DEFWFDB must not be NULL, however.
*/

#ifndef WFDB_NETFILES
# define DEFWFDB	". /usr/local/database"
#else
# define DEFWFDB ". /usr/local/database http://physionet.org/physiobank/database"
#endif

/* Mac OS 9 and earlier, only:  The value of DEFWFDB given below specifies
   that the WFDB path is to be read from the file udb/dbpath.mac on the third
   edition of the MIT-BIH Arrhythmia Database CD-ROM (which has a volume name
   of `MITADB3');  you may prefer to use a file on a writable disk for this
   purpose, to make reconfiguration possible.  See getwfdb() in wfdbio.c for
   further information.
  
   If the version of "ISO 9660 File Access" in the "System:Extensions" folder
   is earlier than 5.0, either update your system software (recommended) or
   define FIXISOCD. */

#ifdef MAC
/* #define FIXISOCD */
# ifdef FIXISOCD
#  define DEFWFDB	"@MITADB3:UDB:DBPATH.MAC;1"
# else
#  define DEFWFDB	"@MITADB3:UDB:DBPATH.MAC"
#  define __STDC__
# endif
#endif

/* DEFWFDBCAL is the name of the default WFDB calibration file, used if the
   WFDBCAL environment variable is not set.  This name need not include path
   information if the calibration file is located in a directory included in
   the WFDB path.  The value given below is the name of the standard
   calibration file supplied on the various CD-ROM databases.  DEFWFDBCAL may
   be NULL if you prefer not to have a default calibration file.  See calopen()
   in calib.c for further information. */
#define DEFWFDBCAL "wfdbcal"

/* WFDB applications may write annotations out-of-order, but in almost all
   cases, they expect that annotations they read must be in order.  The
   environment variable WFDBANNSORT specifies if wfdbquit() should attempt to
   sort annotations in any output annotation files before closing them (it
   does this if WFDBANNSORT is non-zero, or if WFDBANNSORT is not set, and
   DEFWFDBANNSORT is non-zero).  Sorting is done by invoking 'sortann' (see
   ../app/sortann.c) as a separate process;  since this cannot be done from
   an MS-Windows DLL, sorting is disabled by default in this case. */
#if defined(_WINDLL)
#define DEFWFDBANNSORT 0
#else
#define DEFWFDBANNSORT 1
#endif

/* When reading multifrequency records, getvec() can operate in two modes:
   WFDB_LOWRES (returning one sample per signal per frame), or WFDB_HIGHRES
   (returning each sample of any oversampled signals, and duplicating samples
   of other signals as necessary).  If the operating mode is not selected by
   invoking setgvmode(), the value of the environment variable WFDBGVMODE
   determines the mode (0: WFDB_LOWRES, 1: WFDB_HIGHRES);  if WFDBGVMODE
   is not set, the value of DEFWFDBMODE determines the mode. */
#define DEFWFDBGVMODE WFDB_LOWRES

/* putenv() is available in POSIX, SVID, and BSD Unices and in MS-DOS and
   32-bit MS Windows, but not under 16-bit MS Windows or under MacOS.  If it is
   available, getwfdb() (in wfdbio.c) detects when the environment variables
   WFDB, WFDBCAL, WFDBANNSORT, and WFDBGVMODE are not set, and sets them
   according to DEFWFDB, DEFWFDBCAL, DEFWFDBANNSORT, and DEFWFDBGVMODE as
   needed using putenv().  This feature is useful mainly for programs such as
   WAVE, where these variables are set interactively and it is useful to show
   their default values to the user; setwfdb() and getwfdb() do not depend on
   it.
*/
#if !defined(_WIN16) && !defined(MAC)
#define HAS_PUTENV
#endif

#ifndef FILE
#include <stdio.h>
/* stdin/stdout may not be defined in some environments (e.g., for MS Windows
   DLLs).  Defining them as NULL here allows the WFDB library to be compiled in
   such environments (it does not allow use of stdin/stdout when the operating
   environment does not provide them, however). */
#ifndef stdin
#define stdin NULL
#endif
#ifndef stdout
#define stdout NULL
#endif
#endif

#ifndef TRUE
#define TRUE 1 
#endif 
#ifndef FALSE
#define FALSE 0
#endif 

/* Structures used by internal WFDB library functions only */
struct netfile {
  char *url;
  char *data;
  int mode;
  long base_addr;
  long cont_len;
  long pos;
  long err;
  int fd;
};

struct WFDB_FILE {
  FILE *fp;
  struct netfile *netfp;
  int type;
};

/* Values for WFDB_FILE 'type' field */
#define WFDB_LOCAL	0	/* a local file, read via C standard I/O */
#define WFDB_NET	1	/* a remote file, read via libwww */

/* Composite data types */
typedef struct netfile netfile;
typedef struct WFDB_FILE WFDB_FILE;

/* To enable http and ftp access as well as standard (local file) I/O via the
   WFDB library, define WFDB_NETFILES=1 and link with libwww (see 'Makefile').
   Otherwise, the WFDB library uses only the ANSI/ISO standard I/O library. */
#if WFDB_NETFILES
#if WFDB_NETFILES_LIBCURL
# include <curl/curl.h>
#else
# include <WWWLib.h>
# include <WWWInit.h>
#endif
#include <errno.h>
#ifndef EROFS	 /* errno value: attempt to write to a read-only file system */
# ifdef EINVAL
#  define EROFS EINVAL
# else
#  define EROFS 1
# endif
#endif

/* Constants */
/* #define USEHTCACHE */	/* uncomment to enable caching by libwww */

/* http cache parameters (effective only if USEHTCACHE is defined) */
#define CACHEDIR	"/tmp"	/* should be world-writable */
#define CACHESIZE	100	/* max size of the entire http cache in MB */
#define ENTRYSIZE	20	/* max size of a single cache entry in MB */

#define NF_PAGE_SIZE	32768 	/* default bytes per http range request */

/* values for netfile 'err' field */
#define NF_NO_ERR	0	/* no errors */
#define NF_EOF_ERR	1	/* file pointer at EOF */
#define NF_REAL_ERR	2	/* read request failed */

/* values for netfile 'mode' field */
#define NF_CHUNK_MODE	0	/* http range requests supported */
#define NF_FULL_MODE	1	/* http range requests not supported */

#else	    /* WFDB_NETFILES = 0 -- use standard I/O functions only */

#define wfdb_feof(wp)			feof(wp->fp)
#define wfdb_ferror(wp)			ferror(wp->fp)
#define wfdb_fflush(wp)		((wp == NULL) ? fflush(NULL) : fflush(wp->fp))
#define wfdb_fgets(s, n, wp)		fgets(s, n, wp->fp)
#define wfdb_fread(p, s, n, wp)		fread(p, s, n, wp->fp)
#define wfdb_fseek(wp, o, w)		fseek(wp->fp, o, w)
#define wfdb_ftell(wp)			ftell(wp->fp)
#define wfdb_fwrite(p, s, n, wp)	fwrite(p, s, n, wp->fp)
#define wfdb_getc(wp)			getc(wp->fp)
#define wfdb_putc(c, wp)		putc(c, wp->fp)

#endif

#ifdef _WINDOWS
#ifndef _WIN32	/* these definitions are needed for 16-bit MS Windows only */
#define strcat _fstrcat
#define strchr _fstrchr
#define strcmp _fstrcmp
#define strcpy _fstrcpy
#define strlen _fstrlen
#define strncmp _fstrncmp
#define strtok _fstrtok
#endif
#endif

/* Define MKDIR as either the one-argument mkdir() (for the native MSDOS and
   MS-Windows API) or the standard two-argument mkdir() (everywhere else). */
#ifndef MKDIR
#ifdef MSDOS
#include <direct.h>
#define MKDIR(D,P)	mkdir((D))
#else
#define MKDIR(D,P)	mkdir((D),(P))
#endif
#endif

/* Define function prototypes for ANSI C, MS Windows C, and C++ compilers */
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus) || defined(_WINDOWS)
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* These functions are defined in wfdbio.c */
extern int wfdb_fclose(WFDB_FILE *fp);
extern WFDB_FILE *wfdb_open(const char *file_type, const char *record, int mode);
extern int wfdb_checkname(char *name, char *description);
extern void wfdb_striphea(char *record);
extern int wfdb_g16(WFDB_FILE *fp);
extern long wfdb_g32(WFDB_FILE *fp);
extern void wfdb_p16(unsigned int x, WFDB_FILE *fp);
extern void wfdb_p32(long x, WFDB_FILE *fp);
extern int wfdb_parse_path(char *wfdb_path);
extern void wfdb_addtopath(char *pathname);
extern void wfdb_error(char *format_string, ...);
extern WFDB_FILE *wfdb_fopen(char *fname, const char *mode);
extern int wfdb_fprintf(WFDB_FILE *fp, const char *format, ...);
extern void wfdb_setirec(const char *record_name);
extern char *wfdb_getirec(void);

#if WFDB_NETFILES
extern void wfdb_clearerr(WFDB_FILE *fp);
extern int wfdb_feof(WFDB_FILE *fp);
extern int wfdb_ferror(WFDB_FILE *fp);
extern int wfdb_fflush(WFDB_FILE *fp);
extern char *wfdb_fgets(char *s, int size, WFDB_FILE *fp);
extern size_t wfdb_fread(void *ptr, size_t size, size_t nmemb, WFDB_FILE *fp);
extern int wfdb_fseek(WFDB_FILE *fp, long offset, int whence);
extern long wfdb_ftell(WFDB_FILE *fp);
extern size_t wfdb_fwrite(void *ptr, size_t size, size_t nmemb, WFDB_FILE *fp);
extern int wfdb_getc(WFDB_FILE *fp);
extern int wfdb_putc(int c, WFDB_FILE *fp);
#endif

/* These functions are defined in signal.c */
extern void wfdb_sampquit(void);
extern void wfdb_sigclose(void);
extern void wfdb_osflush(void);
extern void wfdb_freeinfo(void);
extern void wfdb_oinfoclose(void);

/* These functions are defined in annot.c */
extern void wfdb_anclose(void);
extern void wfdb_oaflush(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#else        /* declare only function return types for non-ANSI C compilers */

extern char *wfdb_getirec();
extern int wfdb_fclose(), wfdb_checkname(), wfdb_g16(), wfdb_parse_path(),
    wfdb_fprintf();
extern long wfdb_g32();
extern void wfdb_striphea(), wfdb_p16(), wfdb_p32(), wfdb_addtopath(),
    wfdb_error(), wfdb_setirec(), wfdb_sampquit(), wfdb_sigclose(),
    wfdb_osflush(), wfdb_freeinfo(), wfdb_oinfoclose(),
    wfdb_anclose(), wfdb_oaflush();
extern WFDB_FILE *wfdb_open(), *wfdb_fopen();

# if WFDB_NETFILES
extern char *wfdb_fgets();
extern int wfdb_feof(), wfdb_ferror(), wfdb_fflush(), wfdb_fseek(),
    wfdb_getc(), wfdb_putc();
extern long wfdb_ftell();
extern size_t wfdb_fread(), wfdb_fwrite();
extern void wfdb_clearerr();
# endif

/* Some non-ANSI C libraries (e.g., version 7, BSD 4.2) lack an implementation
   of strtok(); define NOSTRTOK to compile the portable version in wfdbio.c. */
# ifdef NOSTRTOK
extern char *strtok();
# endif

#endif
