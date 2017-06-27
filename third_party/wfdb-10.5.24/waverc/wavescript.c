/* file: wavescript.c		G. Moody	10 October 1996
				Last revised:	 10 June 2005
Remote control for WAVE via script

-------------------------------------------------------------------------------
WAVE: Waveform analyzer, viewer, and editor
Copyright (C) 1996-2005 George B. Moody

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

When a WAVE process starts, it creates an empty `mailbox' file named
/tmp/.wave.UID.PID, where UID is the user's ID, and PID is the (decimal)
process id;  this file should be removed when WAVE exits.

This program controls a separate WAVE process by writing a message to that
process's mailbox and then by sending a SIGUSR1 signal to that process.
When WAVE receives a SIGUSR1 signal, it reads the message, performs the
requested action(s), and truncates the mailbox (so that it is once again
empty).

This program reads a command file specified in its first argument, and
constructs the message sent to WAVE based on the contents of the command file.
The (text) message written by this program may contain any or all of:
  -r RECORD	[to (re)open RECORD]
  -a ANNOTATOR	[to (re)open the specified ANNOTATOR for the current record]
  -f TIME	[to go to the specified TIME in the current record]
  -p PATH	[to add PATH to the list of locations for input files ]
  -s SIGNAL	[to specify signals to be displayed]
These messages are copied from the file named in the first command-line
argument, with one exception.

If the PATH argument to -p begins with "http://<", neither it nor the -p
argument are included in the message.  This exception is because this program
is most often used as a helper application for a web browser, the file it reads
is usually obtained by the web browser from an HTTP server, and -p is used to
specify a URL prefix using a server-side include of this form:
  -p http://<!--#echo var="SERVER_NAME" --><!--#echo var="DOCUMENT_URI" -->
If the file containing this string is served by an HTTP server that supports
SSI (server-side includes), the argument of -p is replaced by the URL of the
file.  When WAVE receives the message from this program, WAVE (via
wfdb_addtopath) strips the file name from the end and appends the rest to the
WFDB path, allowing the RECORD to be read from the web server if there is no
local copy (or, more precisely, if there is no copy in any of the locations
that were already in the WFDB path).  If, on the other hand, the file
containing this string is read directly by this program from a local disk
(i.e., not from a copy received by a web browser), then the literal string
"<!--#echo ... -->" appears, and this should not be added to the WFDB path.

If you wish to control a specific (known) WAVE process, use the -pid option
to specify its process id;  otherwise, wave-remote attempts to control the
WAVE process with the highest process id (in most cases, the most recently
started WAVE process).

wavescript attempts to detect orphaned mailboxes (those left behind as a
result of WAVE or the system crashing, for example).  If a mailbox is not
empty when wavescript first looks in it, wavescript waits for a short interval
to give WAVE a chance to empty it.  If the mailbox is still not empty on a
second look, wavescript advises the user to delete it and try again.

In unusual cases, wavescript may be unable to read the mailbox.  In such cases,
wavescript advises the user to delete the mailbox and try again.

If a record name is specified, and no WAVE processes can be found, wavescript
starts a new WAVE process.  The option -pid 0 prevents wavescript from
looking for an existing WAVE process, so this method can be used to start
WAVE unconditionally (though it's unclear why this would be useful).
*/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef __STDC__
#include <stdlib.h>
#else
extern void exit();
# ifdef HAVE_MALLOC_H
# include <malloc.h>
# else
extern char *malloc(), *calloc(), *realloc();
# endif
#endif

#ifndef BSD
# include <string.h>
#else           /* for Berkeley UNIX only */
# include <strings.h>
# define strchr index
#endif

char *pname;

void help()
{
    fprintf(stderr, "usage: %s SCRIPT [ -pid PROCESSID ]\n", pname);
    fprintf(stderr, "The SCRIPT may include:\n");
    fprintf(stderr, " -a ANNOTATOR\n");
    fprintf(stderr, " -f TIME\n");
    fprintf(stderr, " -r RECORD\n");
    fprintf(stderr, " -p PATH\n");
    fprintf(stderr, " -s SIGNAL ...\n");
    fprintf(stderr,
	    "Any lines in the SCRIPT not beginning with '-' are ignored.\n");
}

int find_wave_pid()
{
    DIR *dir;
    struct dirent *dir_entry;
    char pattern[16];
    int i, imax = 0, pl;

    dir = opendir("/tmp");
    sprintf(pattern, ".wave.%d.", (int)getuid());
    pl = strlen(pattern);
    while (dir_entry = readdir(dir)) {
	if (strncmp(dir_entry->d_name, pattern, pl) == 0) {
	    i = atoi(dir_entry->d_name+pl);
	    if (i > imax) imax = i;
	}
    }
    closedir(dir);
    return (imax);
}

char **environ;
#ifndef BINDIR
#define BINDIR /usr/bin
#endif
#define STRING(A)	#A
#define PATH(A,B)	STRING(A) "/" #B

int start_new_wave(record, annotator, ptime, siglist, path)
char *record, *annotator, *ptime, **siglist, *path;
{
    if (*record) {
	char **arg;
	int i, nargs;

	nargs = 4;
	if (*annotator) nargs += 2;
	if (*ptime) nargs += 2;
	if (*path) nargs += 2;
	if (siglist) {
	    for (i = 0; siglist[i]; i++)
		;
	    nargs += i+1;
	}
	arg = (char **)malloc(nargs * sizeof(char *));
	arg[0] = PATH(BINDIR, wave);
	arg[1] = "-r";
	arg[2] = record;
	nargs = 3;
	if (*annotator) {
	    arg[nargs++] = "-a";
	    arg[nargs++] = annotator;
	}
	if (*ptime) {
	    arg[nargs++] = "-f";
	    arg[nargs++] = ptime;
	}
	if (*path) {
	    arg[nargs++] = "-p";
	    arg[nargs++] = path;
	}
	if (siglist) {
	    arg[nargs++] = "-s";
	    while (*siglist)
		arg[nargs++] = *siglist++;
	}
	arg[nargs] = NULL;

	/* Send the standard error output to /dev/null.  This avoids having
	   such error output appear as dialog boxes when wavescript is run from
	   Netscape.  WAVE's and the WFDB library's error messages are
           unaffected by this choice (since they are handled within WAVE), but
           the XView library sometimes generates warning messages that may be
	   safely ignored. */
	freopen("/dev/null", "w", stderr);
	return (execve(arg[0], arg, environ));
    }

    else {	/* We can't start WAVE without specifying which record to open
		   -- give up. */
	fprintf(stderr, "%s: no record is open or specified\n", pname);
	return (2);
    }
}

main(argc, argv, env)
int argc;
char **argv, **env;
{
    char fname[30], *p, *q;
    FILE *ofile = NULL, *script;
    int i = 0, pid;
    static char buf[80], record[80], annotator[80], ptime[80], path[80];
    static char **siglist, sigstrings[80];

    pname = argv[0];
    environ = env;
    if (argc < 2 || (script = fopen(argv[1], "r")) == NULL) {
	help();
	exit(1);
    }
    while (fgets(buf, sizeof(buf), script)) {
	for (p = buf; p < buf+sizeof(buf) && (*p == ' ' || *p == '\t'); p++)
	    ;
	if (p >= buf+sizeof(buf) || *p != '-') continue;
	for (q = p+3; q < buf+sizeof(buf) && (*q == ' ' || *q == '\t'); q++)
	    ;
	if (q >= buf+sizeof(buf) || *q == '\n' || *q == '\0') continue;
	switch (*(p+1)) {
	  case 'a':	/* annotator name follows */
	    strcpy(annotator, q);
	    annotator[strlen(annotator)-1] = '\0';
	    break;
	  case 'f':	/* time follows */
	    strcpy(ptime, q);
	    ptime[strlen(ptime)-1] = '\0';
	    break;
	  case 'p':	/* path follows */
	    if (strncmp("http://<", q, 8)) {
		strcpy(path, q);
		path[strlen(path)-1] = '\0';
	    }
	    break;
	  case 'r':	/* record name follows */
	    strcpy(record, q);
	    record[strlen(record)-1] = '\0';
	    break;
	  case 's':	/* signal numbers follow */
	    strcpy(sigstrings, q);	  /* copy the list of signal numbers */
	    sigstrings[strlen(sigstrings)-1] = ' ';	   /* append a space */
	    q = sigstrings;
	    while (*q) {		     /* count the tokens in the list */
		while (*q && (*q == ' ' || *q == '\t'))
		    q++;		  /* look for the next token, if any */
		if (*q) i++;				 /* count this token */
		while (*q && (*q != ' ' && *q != '\t'))
		    q++;			/* find the end of the token */
	    }
	    siglist = (char **)malloc((i+1) * sizeof(char *));
	    q = sigstrings;
	    i = 0;
	    while (*q) {		       /* split the list into tokens */
		while (*q && (*q == ' ' || *q == '\t'))
		    q++;		  /* look for the next token, if any */
		if (*q) siglist[i++] = q++; /* save pointer to current token */
		while (*q && (*q != ' ' && *q != '\t'))
		    q++;			/* find the end of the token */
		*q++ = '\0';	   /* split the list at the end of the token */
	    }
	    siglist[i] = NULL;
	    break;
	  default:
	    break;
	}
    }
    fclose(script);

    if (*annotator == '\0' && *ptime == '\0' && *record == '\0') {
	help();
	exit(1);
    }
    if (argc == 4 && strncmp(argv[2], "-p", 2) == 0) {	/* pid specified */
	pid = atoi(argv[3]);
	if (pid == 0)
	  exit(start_new_wave(record, annotator, ptime, siglist, path));
	sprintf(fname, "/tmp/.wave.%d.%d", (int)getuid(), pid);
	if ((ofile = fopen(fname, "r")) == NULL) {
	    fprintf(stderr,
   "You don't seem to have a WAVE process with pid %d.  Please try again.\n",
		    pid);
	    exit(2);
	}
    }
    else {		/* Try to find a running WAVE process. */
	pid = find_wave_pid();
	if (pid) {
	    sprintf(fname, "/tmp/.wave.%d.%d", (int)getuid(), pid);
	    ofile = fopen(fname, "r");	/* attempt to read from file */
	    if (ofile == NULL && pid == find_wave_pid()) {
		/* The mailbox is unreadable */
		fprintf(stderr, "Please remove %s and try again.\n", fname);
		exit(3);
	    }
	}
    }

    if (ofile) {	/* We seem to have found a running WAVE process. */
	if (fgetc(ofile) != EOF) {	/* ... but the mailbox isn't empty! */
	    fclose(ofile);
	    sleep(2);			/* give WAVE a chance to empty it */
	    if ((ofile = fopen(fname, "r")) == NULL)
		/* WAVE must have just exited, or someone else cleaned up. */
		exit(start_new_wave(record, annotator, ptime, siglist, path));
	    if (fgetc(ofile) != EOF) {
		/* The mailbox is still full -- WAVE may be stuck, or
		   it may have crashed without removing the mailbox. */
		fclose(ofile);
		unlink(fname);
		fprintf(stderr,
	    "WAVE process %d not responding -- starting new WAVE process\n",
			pid);
		exit(start_new_wave(record, annotator, ptime, siglist, path));
	    }
	}

	/* OK, we've got an empty mailbox -- let's post the message! */
	fclose(ofile);
	ofile = fopen(fname, "w");
	if (*record) fprintf(ofile, "-r %s\n", record);
	if (*annotator) fprintf(ofile, "-a %s\n", annotator);
	if (*ptime) fprintf(ofile, "-f %s\n", ptime);
	if (*path) fprintf(ofile, "-p %s\n", path);
	if (siglist && siglist[0]) {
	    fprintf(ofile, "-s %s", siglist[0]);
	    for (i = 1; siglist[i]; i++)
		fprintf(ofile, " %s", siglist[i]);
	    fprintf(ofile, "\n");
	}
	fclose(ofile);
    
	kill(pid, SIGUSR1);	/* signal to WAVE that the message is ready */
	exit(0);
    }

    else 
	exit(start_new_wave(record, annotator, ptime, siglist, path));
}
