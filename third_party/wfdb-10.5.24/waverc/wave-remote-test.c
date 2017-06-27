/* file: wave-remote-test.c	G. Moody	10 October 1996
				Last revised:	29 April 1999
Testbed for wave-remote

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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

char fname[30];
int i;

void exitproc(s)
int s;
{
    unlink(fname);
    printf("\nReceived signal %d -- exiting\n", s);
    exit(s);
}

void actionproc(s)
{
    char buf[80];
    FILE *ifile;

    printf("\n");
    ifile = fopen(fname, "r");
    while (fgets(buf, sizeof(buf), ifile))
	fputs(buf, stdout);
    fclose(ifile);
    fclose(fopen(fname, "w"));
    signal(SIGUSR1, actionproc);
    i = 0;
}
    
main(argc, argv)
int argc;
char **argv;
{
    sprintf(fname, "/tmp/.wave.%d.%d", (int)getuid(), (int)getpid());
    fclose(fopen(fname, "w"));	/* create fname as an empty file */

    signal(SIGHUP, exitproc);
    signal(SIGINT, exitproc);
    signal(SIGQUIT, exitproc);
    signal(SIGUSR1, actionproc);
    signal(SIGTERM, exitproc);

    printf("wave-remote-test now running, sentinel file: %s\n", fname);
    while (1) {
	sleep(1);
	printf(".");
	if (++i > 79) {
	    i = 0;
	    printf("\n");
	}
	fflush(stdout);
    }
}
