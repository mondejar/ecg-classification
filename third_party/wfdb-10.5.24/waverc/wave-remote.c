/* file: wave-remote.c		G. Moody	10 October 1996
				Last revised:	 10 June 2005
Remote control for WAVE

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

wave-remote is provided with the WAVE distribution as a simple example to
illustrate how an external program can control WAVE's display.  To keep this
program relatively easy to understand, some of the error-checking that would
be desirable in a more robust application has been omitted;  see wavescript.c
in this directory for a more fully-developed implementation of the ideas
contained in this program.

When a WAVE process starts, it creates an empty mailbox file named
/tmp/.wave.UID.PID, where UID is the user's ID, and PID is the (decimal)
process id;  this file should be removed when WAVE exits.

This program controls a separate WAVE process by writing a message to that
process's mailbox and then by sending a SIGUSR1 signal to that process.
When WAVE receives a SIGUSR1 signal, it reads the message, performs the
requested action(s), and truncates the mailbox file (so that it is once again
empty).

The (text) message written by this program may contain any or all of:
  -r RECORD	[to (re)open RECORD]
  -a ANNOTATOR	[to (re)open the specified ANNOTATOR for the current record]
  -f TIME	[to go to the specified TIME in the current record]
  -s SIGNAL ... [to specify signals to be displayed]

These messages are copies of the corresponding command-line arguments given
to wave-remote.

If a WAVE process exits without deleting its mailbox, wave-remote cannot
discover this directly;  if, however, wave-remote discovers that the message
file is not empty, it notifies the user that the WAVE process may have exited
without cleaning up properly.  See `wavescript' for a more robust (but more
complex) way of handling this problem.

If you wish to control a specific (known) WAVE process, use the -pid option
to specify its process id;  otherwise, wave-remote attempts to control the
WAVE process with the highest process id (in most cases, the most recently
started WAVE process).

If a record name is specified, and no WAVE processes can be found, wave-remote
starts a new WAVE process.  The option -pid 0 prevents wave-remote from
looking for an existing WAVE process, so this method can be used to start
WAVE unconditionally (though it's unclear why this would be useful).

This program would be more reliable if it checked to see if the target process
has the same user ID as that of the wave-remote process.  On a typical
workstation, with only one user running WAVE at a time, however, the current
implementation is adequate.
*/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#ifdef __STDC__
#include <stdlib.h>
#else
extern void exit();
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
    fprintf(stderr, "usage: %s OPTION [ ... OPTION ]\n", pname);
    fprintf(stderr, "OPTIONs may include:\n");
    fprintf(stderr, " -a ANNOTATOR\n");
    fprintf(stderr, " -f TIME\n");
    fprintf(stderr, " -pid PROCESSID\n");
    fprintf(stderr, " -r RECORD\n");
    fprintf(stderr, " -s SIGNAL ...\n");
    fprintf(stderr, "At least one of -a, -f, -r, and -s must be specified.\n");
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

main(argc, argv)
int argc;
char **argv;
{
    char fname[30];
    FILE *ofile;
    int i, j = 0, pid;
    static char *ppid, *record, *annotator, *ptime;
    static int *siglist;

    pname = argv[0];
    for (i = 1; i < argc; i++) {	/* read command-line arguments */
	if (argv[i][0] == '-') switch (argv[i][1]) {
	  case 'a':	/* annotator name follows */
	    if (++i >= argc) {
		fprintf(stderr, "%s: -a must be followed by annotator\n",
			argv[0]);
		exit(1);
	    }
	    annotator = argv[i];
	    break;
	  case 'f':	/* time follows */
	    if (++i >= argc) {
		fprintf(stderr, "%s: -f must be followed by time\n",
			argv[0]);
		exit(1);
	    }
	    ptime = argv[i];
	    break;
	  case 'p':	/* WAVE process id follows */
	    if (++i >= argc) {
		fprintf(stderr, "%s: -p must be followed by a process id\n",
			argv[0]);
		exit(1);
	    }
	    ppid = argv[i];
	    break;
	  case 'r':	/* record name follows */
	    if (++i >= argc) {
		fprintf(stderr, "%s: -r must be followed by a process id\n",
			argv[0]);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  case 's':	/* signal numbers follow */
	    /* count the number of output signals */
	    for (j = 0; ++i < argc && argv[i][0] != '-'; j++)
		;
	    if (j == 0) {
		fprintf(stderr, "%s: signal list must follow -s\n", argv[0]);
		exit(1);
	    }
	    /* allocate storage for the signal list */
	    if ((siglist = (int *)malloc((j+1) * sizeof(int))) == NULL) {
		(void)fprintf(stderr, "%s: insufficient memory\n", argv[0]);
		exit(2);
	    }
	    /* fill the signal list */
	    siglist[j] = -1;
	    for (i -= j, j = 0; i < argc && argv[i][0] != '-'; )
		siglist[j++] = atoi(argv[i++]);
	    i--;
	    break;
	}
    }
    if (annotator == NULL && ptime == NULL && record == NULL && j == 0) {
	help();
	exit(1);
    }
    if (ppid == NULL)	/* we need to find a PID */
	pid = find_wave_pid();
    else
	pid = atoi(ppid);

    if (pid == 0) {
	if (record) {	/* start a new WAVE process */
	    char command[128];

	    sprintf(command, "wave -r %s", record);
	    if (annotator) sprintf(command+strlen(command),
				   " -a %s", annotator);
	    if (ptime) sprintf(command+strlen(command), " -f %s", ptime);
	    if (siglist && (siglist[0] >= 0)) {
		sprintf(command+strlen(command), " -s %d", siglist[0]);
		for (j = 1; siglist[j] >= 0; j++)
		    sprintf(command+strlen(command), " %d", siglist[j]);
	    }
	    system(command);
	    exit(0);
	}
	else {
	    fprintf(stderr, "%s: can't find WAVE process\n", argv[0]);
	    exit(2);
	}
    }

    sprintf(fname, "/tmp/.wave.%d.%d", (int)getuid(), pid);
    ofile = fopen(fname, "r");	/* attempt to read from file */
    if (ofile == NULL) {
	fprintf(stderr, "Can't open WAVE sentinel file %s\n", fname);
	exit(3);
    }
    if (fgetc(ofile) != EOF) {
	fprintf(stderr,
	      "Warning: WAVE process %d may have exited without removing %s\n",
		pid, fname);
	exit(4);
    }

    fclose(ofile);
    ofile = fopen(fname, "w");
    if (record) fprintf(ofile, "-r %s\n", record);
    if (annotator) fprintf(ofile, "-a %s\n", annotator);
    if (ptime) fprintf(ofile, "-f %s\n", ptime);
    if (siglist && (siglist[0] >= 0)) {
	fprintf(ofile, "-s %d", siglist[0]);
	for (j = 1; siglist[j] >= 0; j++)
	    fprintf(ofile, " %d", siglist[j]);
	fprintf(ofile, "\n");
    }
    fclose(ofile);
    
    kill(pid, SIGUSR1);	/* signal to WAVE that the message is ready */
    exit(0);
}


