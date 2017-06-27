/* file: ecgeval.c	G. Moody	22 March 1992
			Last revised:  19 November 2013		wfdb 10.5.21

-------------------------------------------------------------------------------
ecgeval: Generate and run a script of commands to compare sets of annotations
Copyright (C) 1992-2013 George B. Moody

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

Note: versions prior to 10.5.21 included code that was conditionally compiled
if MSDOS was defined.  This code truncated file names to 8.3 format.  If you
need to run ecgeval on an ancient OS that cannot handle longer filenames, use
an archived version (available on PhysioNet).
*/

#include <stdio.h>
#include <time.h>
#include <wfdb/wfdb.h>

#define NDBMAX	50	/* maximum number of databases in `dblist' */
#define ECHONOTHING "echo >>%s\n"

char buf[256];

void getans(p, n)
char *p;
int n;
{
    int l;

    if (n > 256) n = 256;
    (void)fgets(buf, n, stdin);
    l = strlen(buf);
    if (buf[l-1] == '\n')
	buf[--l] = '\0';
    if (buf[0] && p)
	(void)strcpy(p, buf);
}

char *dbname[NDBMAX], *dbfname[NDBMAX], *dbdesc[NDBMAX], *dblname;
int ndb;

int getdblists()
{
    char *p, *sep = "\t\n\r", *getenv();
    FILE *dblfile;

    if ((dblname = getenv("DBLIST")) == NULL)
	dblname = "dblist";
    if ((p = wfdbfile(dblname, (char *)NULL)) == NULL) {
	(void)fprintf(stderr, "can't find `%s' (list of databases available)\n",
		dblname);
	return (-1);
    }
    if ((dblfile = fopen(p, "r")) == NULL) {
	(void)fprintf(stderr, "can't read `%s' (list of databases available)\n", p);
	return (-1);
    }
    while (fgets(buf, 256, dblfile)) {	/* read an entry */
	for (p = buf; *p == ' ' || *p == '\t'; p++)
	    ;	/* skip any leading whitespace */
	if (*p == '#' || (p = strtok(p, sep)) == NULL)
	    continue;	/* comment or empty line -- ignore */
	if (ndb >= NDBMAX) {
	    (void)fprintf(stderr, "(warning) list of databases is too long\n");
	    break;
	}
	if ((dbname[ndb] = (char *)malloc((unsigned)(strlen(p)+1))) == NULL) {
	    (void)fprintf(stderr, "insufficient memory\n");
	    exit(2);
	}
	(void)strcpy(dbname[ndb], p);
	if ((p = strtok((char *)NULL, sep)) == NULL)
	    continue;	/* ignore lines with missing fields */
	if ((dbfname[ndb] = (char *)malloc((unsigned)(strlen(p)+1))) == NULL) {
	    (void)fprintf(stderr, "insufficient memory\n");
	    exit(2);
	}
	(void)strcpy(dbfname[ndb], p);
	if ((p = strtok((char *)NULL, sep)) == NULL)
	    continue;
	if ((dbdesc[ndb] = (char *)malloc((unsigned)(strlen(p)+1))) == NULL) {
	    (void)fprintf(stderr, "insufficient memory\n");
	    exit(2);
	}
	(void)strcpy(dbdesc[ndb++], p);
    }
    (void)fclose(dblfile);
    return (0);
}

char *month_name[12] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December" };

main()
{
    FILE *dbf, *rfile = NULL, *sfile = NULL;
    int dbi = -1, i, nhr = 1;
    int evalsv = 1, evalvf = 1, evalaf = 1, evalst = 1, evalst2 = 1;
    struct tm *now;
    static char rhrname[20], tname[20], tans[20], *dbfn, dbtn[5],
        reportname[30], scriptname[20], *epicmpSoption, evalcommand[30];
    static char bxbcommand[256], rxrcommand[256], mxmcommand[256],
		epicmpcommand[256];
    static char bxbfile1[40] = "bxb.out", bxbfile2[40] = "sd.out",
	        rxrfile1[40] = "vruns.out", rxrfile2[40] = "sruns.out",
		mxmfile[40] = "hr%d.out", epicmpaffile[40] = "af.out",
		epicmpvffile[40] = "vf.out", epicmpstfile1[40] = "st.out",
		epicmpstfile2[40] = "stm.out", plotstmfile[40] = "stm.ps";
    static char *rname = "atr";

#ifdef __STDC__
    time_t t, time();

    t = time((time_t *)NULL);    /* get current time from system clock */
#else
    long t, time();

    t = time((long *)NULL);
#endif
    now = localtime(&t);

    (void)fprintf(stderr, "\t\t_________________________________________\n");
    (void)fprintf(stderr, "\t\tAutomated ECG Analyzer Evaluation Program\n");
    (void)fprintf(stderr, "\t\t_________________________________________\n\n");

    (void)fprintf(stderr,
"The most recent version of this program can always be obtained as part of\n");
    (void)fprintf(stderr,
"the WFDB Software Package, from PhysioNet (http://physionet.org/).\n");
    (void)fprintf(stderr,
 "If you have questions about this software, please contact the author:\n");
    (void)fprintf(stderr,
 "\tGeorge B. Moody\n\tMIT Room E25-505A\n\tCambridge, MA 02139 USA\n");
    (void)fprintf(stderr, "\temail: george@mit.edu\n\n");
    if (getdblists() < 0) exit(1);

    (void)fprintf(stderr,
 "This program constructs a script (batch) file which evaluates a set of\n");
    (void)fprintf(stderr,
 "test annotation files by comparing them with reference annotation files\n");
    (void)fprintf(stderr,
 "in accordance with American National Standards ANSI/AAMI EC38:1998 and\n");
    (void)fprintf(stderr,
 "ANSI/AAMI EC57:1998 for ambulatory ECGs and for testing and reporting\n");
    (void)fprintf(stderr,
 "performance results of cardiac rhythm and ST segment measurement\n");
    (void)fprintf(stderr,
"algorithms.  For some questions, a default answer is provided in brackets.\n");
    (void)fprintf(stderr,
 "Press RETURN (ENTER) to accept the default, or type the desired answer\n");
    (void)fprintf(stderr,
 "followed by RETURN.  After you have answered all of the questions, you\n");
    (void)fprintf(stderr,
 "are given a chance to change any of your answers before beginning the\n");
    (void)fprintf(stderr,
 "actual evaluation.  At that time, you may exit from this program and\n");
    (void)fprintf(stderr,
 "examine the evaluation script which it has generated before running that\n");
    (void)fprintf(stderr,
 "script.\n\n");

    (void)fprintf(stderr, "Press RETURN to begin: ");
    getans((char *)NULL, 5);
    (void)fprintf(stderr, "\n");

  getanswers:
    do {
	(void)fprintf(stderr,
	      "Which database do you wish to use? (press RETURN for list) ");
	if (dbi >= 0) (void)fprintf(stderr, "[%s]: ", dbname[dbi]);
	getans(tans, 20);
	if (tans[0] == '\0') {
	    for (i = 0; i < ndb; i++)
		(void)fprintf(stderr, " %-10s  %s\n", dbname[i], dbdesc[i]);
	    (void)fprintf(stderr, "Enter a choice from the first column: ");
	    getans(tans, 20);
	}
	for (i = 0; i < ndb; i++)
	    if (strcmp(tans, dbname[i]) == 0)
		break;
	if (i < ndb) dbi = i;
	else tans[0] = '\0';
    } while (dbi < 0);

    if ((dbfn = wfdbfile(dbfname[dbi], (char *)NULL)) == NULL ||
	(dbf = fopen(dbfn, "r")) == NULL) {
	(void)fprintf(stderr,
		      "Sorry, I can't find the list of records for the %s.\n",
		      dbname[i]);
	(void)fprintf(stderr, "Check the file `%s' for errors.\n", dblname);
	exit(3);
    }

    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr,
 "You must have a set of annotation files generated by the device under\n");
    (void)fprintf(stderr,
 "test from its analysis of the signal files of the %s.  These files\n",
		  dbname[dbi]);
    (void)fprintf(stderr,
 "are identified by the test annotator name (the part of the file name\n");
    (void)fprintf(stderr,
 "that precedes the `.' and the record name).\n");
    (void)fprintf(stderr,
 "If you don't yet have a set of test annotation files, you can add the\n");
    (void)fprintf(stderr,
 "commands needed to create them to the evaluation script that will be\n");
    (void)fprintf(stderr,
 "generated by this program.\n"); 
    do {
	(void)fprintf(stderr, "What is the test annotator name? ");
	if (tname[0]) (void)fprintf(stderr, "[%s]: ", tname);
	getans(tname, 20);
    } while (tname[0] == '\0');

    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr,
 "To evaluate heart rate or HRV measurement error, you must have a set of\n");
    (void)fprintf(stderr,
 "reference heart rate annotation files.  These must be generated from the\n");
    (void)fprintf(stderr,
 "reference (`atr') annotation files supplied with the %s.\n", dbname[dbi]);
    (void)fprintf(stderr,
 "If you don't yet have a set of reference heart rate annotation files,\n");
    (void)fprintf(stderr,
 "add the commands needed to create them to the evaluation script that\n");
    (void)fprintf(stderr,
 "will be generated by this program.\n"); 
    do {
	(void)fprintf(stderr,
		      "What is the reference heart rate annotator name? ");
	if (rhrname[0]) (void)fprintf(stderr, "[%s]: ", rhrname);
	getans(rhrname, 20);
    } while (rhrname[0] == '\0');

    (void)fprintf(stderr, "\n");
    (void)fprintf(stderr,
 "The next several questions refer to evaluation of optional ECG analyzer\n");
    (void)fprintf(stderr,
 "outputs.\n\n");

    (void)fprintf(stderr,
 "Test and reference heart rate annotation files may contain more than one\n");
    (void)fprintf(stderr,
 "type of heart rate, HRV, or RRV measurement.\n");
    do {
	(void)fprintf(stderr, "How many types are there? [%d]: ", nhr);
	tans[0] = '\0';
	getans(tans, 20);
	if (tans[0] == '\0')
	    i = 0;
	else {
	    i = -1;
	    if (sscanf(tans, "%d", &i) == 1 && 0 <= i && i <= 127)
		nhr = i;
	}
    } while (i < 0);

    tans[0] = evalsv ? 'y' : 'n';
    (void)fprintf(stderr, "Do you wish to evaluate SVEB detection? [%c]: ",
		  tans[0]);
    getans(tans, 20);
    evalsv = (tans[0] == 'y' || tans[0] == 'Y');

    tans[0] = evalvf ? 'y' : 'n';
    (void)fprintf(stderr, "Do you wish to evaluate VF detection? [%c]: ",
		  tans[0]);
    getans(tans, 20);
    evalvf = (tans[0] == 'y' || tans[0] == 'Y');

    tans[0] = evalaf ? 'y' : 'n';
    (void)fprintf(stderr, "Do you wish to evaluate AF detection? [%c]: ",
		  tans[0]);
    getans(tans, 20);
    evalaf = (tans[0] == 'y' || tans[0] == 'Y');

    tans[0] = evalst ? 'y' : 'n';
    (void)fprintf(stderr, "Do you wish to evaluate ST analysis? [%c]: ",
		  tans[0]);
    getans(tans, 20);
    evalst = (tans[0] == 'y' || tans[0] == 'Y');

    if (evalst) {
	tans[0] = evalst2 ? 'y' : 'n';
	(void)fprintf(stderr, "Use both signals to define ST episodes? [%c]: ",
		      tans[0]);
	getans(tans, 20);
	evalst2 = (tans[0] == 'y' || tans[0] == 'Y');
	if (!evalst2) {
	    (void)fprintf(stderr,
		  "ST episodes will be defined for signal 0 only.\n");
	    epicmpSoption = " -S0 ";
	}
	else
	    epicmpSoption = " -S ";
    }
    (void)fprintf(stderr,
		  "\nDo you wish to change any of your answers? [y]: ");
    tans[0] = 'y';
    getans(tans, 20);
    if (tans[0] == 'y' || tans[0] == 'Y') goto getanswers;

    for (i = 0; i < 4; i++) {
	if (dbname[dbi][i] == ' ') {
	    dbtn[i] = '\0';
	    break;
	}
	if ('A' <= dbname[dbi][i] && dbname[dbi][i] <= 'Z')
	    dbtn[i] = dbname[dbi][i] - 'A' + 'a';
	else
	    dbtn[i] = dbname[dbi][i];
    }
    (void)sprintf(scriptname, "eval-%s-%s", tname, dbtn);
    (void)sprintf(reportname, "%s-%s-evaluation", tname, dbtn);

    while (sfile == NULL) {
	do {
	    (void)fprintf(stderr, "\nChoose a name for the evaluation script");
	    if (scriptname[0])
		(void)fprintf(stderr, " [%s]", scriptname);
	    (void)fprintf(stderr, ": ");
	    getans(scriptname, 20);
	} while (scriptname[0] == '\0');

	if (sfile = fopen(scriptname, "r")) {
	    (void)fclose(sfile);
	    (void)fprintf(stderr,
	     "There is already a file named `%s' in the current directory;\n",
			  scriptname);
	    (void)fprintf(stderr,
        "type `a' to append the evaluation script to the existing file, or\n");
	    (void)fprintf(stderr,
		      "type `r' to replace the existing file, or\n");
	    (void)fprintf(stderr,
		      "press RETURN to choose another name for the script:  ");
	    tans[0] = '\0';
	    getans(tans, 20);
	    if (tans[0] == 'a') {
		if ((sfile = fopen(scriptname, "a")) == NULL)
		    (void)fprintf(stderr, "Sorry, I can't append to `%s'.\n",
				  scriptname);
	    }
	    else if (tans[0] == 'r') {
		if ((sfile = fopen(scriptname, "w")) == NULL)
		    (void)fprintf(stderr, "Sorry, I can't replace `%s'.\n",
				  scriptname);
	    }
	    else
		sfile = NULL;
	}
	else if ((sfile = fopen(scriptname, "w")) == NULL)
	    (void)fprintf(stderr, "Sorry, I can't create `%s'.\n", scriptname);

	if (sfile == NULL)
	    scriptname[0] = '\0';
    }

    while (rfile == NULL) {
	do {
	    (void)fprintf(stderr, "\nChoose a name for the evaluation report");
	    if (reportname[0])
		(void)fprintf(stderr, " [%s]", reportname);
	    (void)fprintf(stderr, ": ");
	    getans(reportname, 20);
	} while (reportname[0] == '\0');

	if (rfile = fopen(reportname, "r")) {
	    (void)fclose(rfile);
	    (void)fprintf(stderr,
	     "There is already a file named `%s' in the current directory;\n",
			  reportname);
	    (void)fprintf(stderr,
        "type `a' to append the evaluation report to the existing file, or\n");
	    (void)fprintf(stderr,
		      "type `r' to replace the existing file, or\n");
	    (void)fprintf(stderr,
		      "press RETURN to choose another name for the report:  ");
	    tans[0] = '\0';
	    getans(tans, 20);
	    if (tans[0] == 'a') {
		if ((rfile = fopen(reportname, "a")) == NULL)
		    (void)fprintf(stderr, "Sorry, I can't append to `%s'.\n",
				  reportname);
	    }
	    else if (tans[0] == 'r') {
		if ((rfile = fopen(reportname, "w")) == NULL)
		    (void)fprintf(stderr, "Sorry, I can't replace `%s'.\n",
				  reportname);
	    }
	    else
		rfile = NULL;
	}
	else if ((rfile = fopen(reportname, "w")) == NULL)
	    (void)fprintf(stderr, "Sorry, I can't create `%s'.\n", reportname);

	if (rfile == NULL)
	    reportname[0] = '\0';
    }

    (void)fprintf(stderr,
        "The next group of questions refers to the names of files in which\n");
    (void)fprintf(stderr,
"intermediate summary statistics are to be written.  If any of these exist\n");
    (void)fprintf(stderr,
"already, new statistics will be appended, and the aggregate statistics\n");
    (void)fprintf(stderr,
"in `%s' will be based on the entire contents of these files.\n", reportname);

  getfilenames:
    (void)fprintf(stderr,
		  "Beat detection and classification file [%s]: ", bxbfile1);
    getans(bxbfile1, 40);

    (void)fprintf(stderr,
		  "Analysis shutdown file [%s]: ", bxbfile2);
    getans(bxbfile2, 40);

    (void)fprintf(stderr,
		  "Ventricular ectopic run file [%s]: ", rxrfile1);
    getans(rxrfile1, 40);

    if (evalsv) {
	(void)fprintf(stderr,
		  "Supraventricular ectopic run file [%s]: ", rxrfile2);
	getans(rxrfile2, 40);
    }

    if (evalvf) {
	(void)fprintf(stderr,
		  "Ventricular fibrillation file [%s]: ", epicmpvffile);
	getans(epicmpvffile, 40);
    }

    if (evalaf) {
	(void)fprintf(stderr,
		  "Atrial fibrillation file [%s]: ", epicmpaffile);
	getans(epicmpaffile, 40);
    }

    if (evalst) {
	(void)fprintf(stderr,
		  "ST analysis file [%s]: ", epicmpstfile1);
	getans(epicmpstfile1, 40);

	(void)fprintf(stderr,
		  "ST measurement file [%s]: ", epicmpstfile2);
	getans(epicmpstfile2, 40);

	(void)fprintf(stderr,
	  "PostScript scatter plot of ST measurements [%s]: ", plotstmfile);
	getans(plotstmfile, 40);
    }

    (void)fprintf(stderr,
 "The name given for the heart rate measurement file must contain `%%d',\n");
    (void)fprintf(stderr,
	  "which is replaced by the measurement number.\n");
    (void)fprintf(stderr,
		  "Heart rate measurement file [%s]: ", mxmfile);
    getans(mxmfile, 40);

    (void)fprintf(stderr,
		  "\nDo you wish to change any of these answers? [y]: ");
    tans[0] = 'y';
    getans(tans, 20);
    if (tans[0] == 'y' || tans[0] == 'Y') goto getfilenames;

    /* Generate report file header. */
    (void)fprintf(rfile, "file: %s\tecgeval\t\t%d %s %d\n", reportname,
	    now->tm_mday, month_name[now->tm_mon], now->tm_year+1900);
    (void)fprintf(rfile, "Evaluation of `%s' on the %s\n\n",
		  tname, dbdesc[dbi]);
    (void)fclose(rfile);

    (void)fprintf(stderr, "\nGenerating `%s' ...", scriptname);

    (void)sprintf(bxbcommand, "bxb -r %%s -a %s %s %s %s %s\n", rname, tname,
		  evalsv ? "-L" : "-l", bxbfile1, bxbfile2);
    if (evalsv == 0) rxrfile2[0] = '\0';
    (void)sprintf(rxrcommand, "rxr -r %%s -a %s %s %s %s %s\n", rname, tname,
		  evalsv ? "-L" : "-l", rxrfile1, rxrfile2);
    (void)sprintf(mxmcommand, "mxm -r %%s -a %s %s -L %s -m %%d\n",
		  rhrname, tname, mxmfile);
    if (evalaf || evalvf || evalst) {
	(void)sprintf(epicmpcommand,"epicmp -r %%s -a %s %s -L", rname, tname);
	if (evalaf) {
	    (void)strcat(epicmpcommand, " -A ");
	    (void)strcat(epicmpcommand, epicmpaffile);
	}
	if (evalvf) {
	    (void)strcat(epicmpcommand, " -V ");
	    (void)strcat(epicmpcommand, epicmpvffile);
	}
	if (evalst) {
	    (void)strcat(epicmpcommand, epicmpSoption);
	    (void)strcat(epicmpcommand, epicmpstfile1);
	    (void)strcat(epicmpcommand, " ");
	    (void)strcat(epicmpcommand, epicmpstfile2);
	}
	(void)strcat(epicmpcommand, "\n");
    }

    (void)fprintf(sfile, ": file: %s\tecgeval\t\t%d %s %d\n", scriptname,
	    now->tm_mday, month_name[now->tm_mon], now->tm_year+1900);
    (void)fprintf(sfile,
	    ":\n: Evaluate test annotator %s on the %s\n", tname, dbname[dbi]);
    (void)fprintf(sfile,
 "\n: This file was automatically generated by ecgeval.  Do not edit it\n");
    (void)fprintf(sfile,  ": unless you know what you are doing!\n");

    while (fgets(buf, 256, dbf)) {
	char *record;

	record = strtok(buf, " \t\n\r");
	if (*record == '#' || *record == '\0')
	    continue;	/* comment or empty line -- ignore */
	if ((int)strlen(record) > WFDB_MAXRNL) {
	    (void)fprintf(stderr,
		    "Illegal record name, `%s', found in `%s' (ignored).\n",
			  record, dbfn);
	    continue;
	}
	(void)fprintf(sfile, "\n: Record %s\n", record);
	(void)fprintf(sfile, bxbcommand, record);
	(void)fprintf(sfile, rxrcommand, record);
	for (i = 0; i < nhr; i++)
	    (void)fprintf(sfile, mxmcommand, record, i, i);
	if (evalaf || evalvf || evalst)
	    (void)fprintf(sfile, epicmpcommand, record);
    }
    (void)fclose(dbf);

    (void)fprintf(sfile, "\n: Generate summary report\n");
    (void)fprintf(sfile,
		  "echo Beat detection and classification performance >>%s\n",
		  reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);
    (void)fprintf(sfile, "sumstats %s >>%s\n", bxbfile1, reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);

    (void)fprintf(sfile, "echo Analysis shutdowns >>%s\n",
		  reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);
    (void)fprintf(sfile, "sumstats %s >>%s\n", bxbfile2, reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);

    (void)fprintf(sfile,
		  "echo Ventricular ectopic run detection performance >>%s\n",
		  reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);
    (void)fprintf(sfile, "sumstats %s >>%s\n", rxrfile1, reportname);
    (void)fprintf(sfile, ECHONOTHING, reportname);

    if (evalsv) {
	(void)fprintf(sfile,
	      "echo Supraventricular ectopic run detection performance >>%s\n",
		      reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
	(void)fprintf(sfile, "sumstats %s >>%s\n", rxrfile2, reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
    }

    for (i = 0; i < nhr; i++) {
	(void)fprintf(sfile,
		   "echo Heart rate measurement number %d performance >>%s\n",
		      i, reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
	(void)fprintf(sfile, "sumstats ");
	(void)fprintf(sfile, mxmfile, i);
	(void)fprintf(sfile, " >>%s\n", reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
    }

    if (evalvf) {
	(void)fprintf(sfile,
	      "echo Ventricular fibrillation detection performance >>%s\n",
		      reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
	(void)fprintf(sfile, "sumstats %s >>%s\n", epicmpvffile, reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
    }

    if (evalaf) {
	(void)fprintf(sfile,
	      "echo Atrial fibrillation detection performance >>%s\n",
		      reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
	(void)fprintf(sfile, "sumstats %s >>%s\n", epicmpaffile, reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
    }

    if (evalst) {
	(void)fprintf(sfile,
	      "echo Ischemic ST detection performance >>%s\n",
		      reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);
	(void)fprintf(sfile, "sumstats %s >>%s\n", epicmpstfile1, reportname);
	(void)fprintf(sfile, ECHONOTHING, reportname);

	(void)fprintf(sfile,
		   ": Generate PostScript scatter plot of ST measurements\n");
	(void)fprintf(sfile, "plotstm %s >%s\n", epicmpstfile2, plotstmfile);
    }

    (void)fprintf(sfile,
		  "echo The evaluation is complete.  Print text file %s\n",
		  reportname);
    if (evalst)
	(void)fprintf(sfile,
		      "echo and PostScript file %s to get the results.\n",
		      plotstmfile);
    else 
	(void)fprintf(sfile,
		      "echo to get the results.\n");

    (void)fclose(sfile);
    (void)fprintf(stderr, " done\n\n");
    (void)sprintf(evalcommand, "sh ./%s", scriptname);
    (void)fprintf(stderr,
	    "Do you wish to run the evaluation script now? [y]: ");
    tans[0] = 'y';
    getans(tans, 20);
    if (tans[0] == 'y')
	(void)system(evalcommand);
    else {
	(void)fprintf(stderr, "Inspect and edit `%s' as necessary, then type\n",
		scriptname);
	(void)fprintf(stderr, "   %s\nto run the evaluation.\n", evalcommand);
    }
    exit(0);	/*NOTREACHED*/
}
