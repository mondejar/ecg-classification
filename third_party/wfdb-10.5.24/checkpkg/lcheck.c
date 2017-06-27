/* file: lcheck.c	G. Moody       7 September 2001
			Last revised:  26 November 2010
-------------------------------------------------------------------------------
wfdbcheck: test WFDB library
Copyright (C) 2001-2010 George B. Moody

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
#include <wfdb/wfdb.h>

char *info, *pname, *prog_name();
int n, nsig, i, j, framelen, errors = 0, istat, vflag = 0;
char headerversion[40];
char *libversion;
char *p, *q, *defpath, *dbpath;
WFDB_Anninfo aiarray[2];
WFDB_Annotation annot;
WFDB_Calinfo cal;
WFDB_Siginfo *si;
WFDB_Sample *vector;
void help(), list_untested();

main(argc, argv)
int argc;
char *argv[];
{

  pname = prog_name(argv[0]);
  if (argc > 1) {
    if (strcmp(argv[1], "-v") == 0) vflag = 1;
    else if (strcmp(argv[1], "-V") == 0) vflag = 2;
    else { help(); exit(1); }
  }

  if ((p = wfdberror()) == NULL) {
    printf("Error: wfdberror() did not return a version string\n");
    p = "unknown version of the WFDB library";
    errors++;
  }
  libversion = calloc(sizeof(char), strlen(p)+1);
  strcpy(libversion, p);

  /* Print the library version number and date. */
  fprintf(stderr, "Testing %s", libversion = wfdberror());

  /* Check that the installed <wfdb/wfdb.h> matches the library. */
  sprintf(headerversion, "WFDB library version %d.%d.%d",
	  WFDB_MAJOR, WFDB_MINOR, WFDB_RELEASE);
  if (strncmp(libversion, headerversion, strlen(headerversion))) {
    printf("Error: Library version does not match <wfdb/wfdb.h>\n"
                    "       (<wfdb/wfdb.h> is from %s)\n", headerversion);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  Library version matches <wfdb/wfdb.h>\n");

  /* If in verbose mode, print library defaults. */
  if (vflag) {
    static int format[WFDB_NFMTS] = WFDB_FMT_LIST;

    printf("[OK]:  WFDB %s NETFILES\n", WFDB_NETFILES ? "supports" :
	    "does not support");
    printf("[OK]:  WFDB_MAXANN = %d\n", WFDB_MAXANN);
    printf("[OK]:  WFDB_MAXSIG = %d\n", WFDB_MAXSIG);
    printf("[OK]:  WFDB_MAXSPF = %d\n", WFDB_MAXSPF);
    printf("[OK]:  WFDB_MAXRNL = %d\n", WFDB_MAXRNL);
    printf("[OK]:  WFDB_MAXUSL = %d\n", WFDB_MAXUSL);
    printf("[OK]:  WFDB_MAXDSL = %d\n", WFDB_MAXDSL);
    printf("[OK]:  Signal formats = {");
    for (i = 0; i < WFDB_NFMTS; i++)
      printf("%d%s", format[i], (i < WFDB_NFMTS-1) ? ", " : "}\n");
    printf("[OK]:  WFDB_DEFFREQ = %g\n",  WFDB_DEFFREQ);
    printf("[OK]:  WFDB_DEFGAIN = %g\n",  WFDB_DEFGAIN);
    printf("[OK]:  WFDB_DEFRES = %d\n",  WFDB_DEFRES);
  }

  /* Test the WFDB library functions. */
  aiarray[0].name = "atr"; aiarray[0].stat = WFDB_READ;
  aiarray[1].name = "chk"; aiarray[1].stat = WFDB_WRITE;

  /* *** getwfdb *** */
  if ((p = getwfdb()) == NULL) {
    printf("Error: No default WFDB path defined\n");
    defpath = "";
    errors++;
  }
  else {
    defpath = calloc(sizeof(char), strlen(p)+1);
    strcpy(defpath, p);

    if (vflag)
      printf("[OK]:  Default WFDB path = %s\n", defpath);
  }

  /* *** setwfdb *** */
  dbpath = calloc(sizeof(char), strlen(defpath)+10);
  sprintf(dbpath, "data %s\n", defpath);
  setwfdb(dbpath);
  if ((p = getwfdb()) == NULL || strcmp(p, dbpath)) {
    printf("Error: Could not set WFDB path\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  WFDB path modified successfully\n");

  /* *** calopen *** */
  i = calopen(NULL);
  if (i != 0) {
    printf("Error: Could not open calibration list, calopen returned %d\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  WFDB calibration list opened successfully\n");

  /* *** getcal *** */
  i = getcal("NBPfoo", "mmHg", &cal);
  if (i != 0 || cal.low != 0.0 || cal.high != 100.0 || cal.scale != 100.0 ||
      strcmp(cal.sigtype, "NBP") != 0 || strcmp(cal.units, "mmHg") != 0 ||
      cal.caltype != (WFDB_DC_COUPLED | WFDB_CAL_SQUARE)) {
    printf("Error: getcal returned %d, cal = { %g %g %g %s %s %d }\n", i,
	   cal.low, cal.high, cal.scale, cal.sigtype, cal.units, cal.caltype);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  getcal was successful\n");

  /* *** putcal *** */
  cal.sigtype = "foobar";
  i = putcal(&cal);
  if (i != 0) {
    printf("Error: putcal returned %d\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  putcal returned 0\n");

  i = getcal("foobar", "mmHg", &cal);
  if (i != 0 || cal.low != 0.0 || cal.high != 100.0 || cal.scale != 100.0 ||
      strcmp(cal.sigtype, "foobar") != 0 || strcmp(cal.units, "mmHg") != 0 ||
      cal.caltype != (WFDB_DC_COUPLED | WFDB_CAL_SQUARE)) {
    printf("Error: putcal failed, cal = { %g %g %g %s %s %d }\n",
	   cal.low, cal.high, cal.scale, cal.sigtype, cal.units, cal.caltype);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  putcal was successful\n");

  /* *** newcal *** */
  i = newcal("lcheck_cal");
  if (i != 0) {
    printf("Error: newcal returned %d\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  newcal was successful\n");

  /* Test I/O using the local record first. */
  check("100s", "100z");

  /* Test I/O again using the remote record. */
  if (WFDB_NETFILES) {
    if (vflag)
      printf("[OK]:  Repeating tests using NETFILES");
    if (strstr(defpath, "http://") == NULL) {
      fprintf(stderr,
       "\nWarning: default WFDB path does not include an http:// component\n");
      setwfdb(". http://www.physionet.org/physiobank/database");
      printf("WFDB path has been set to: %s\n", getwfdb());
    }
    else if (vflag) {
      printf(" (reverting to default WFDB path)\n");
      setwfdb(defpath);
    }
    check("udb/100s", "udb/100z");
  }

  /* If there were any errors detected by the WFDB library but not by this
     program, this test will pick them up. */
  if (errors == 0) {
    if ((p = wfdberror()) == NULL) {
      printf("Error: wfdberror() did not return a version string\n");
      p = "unknown version of the WFDB library";
      errors++;
    }
    if (strcmp(libversion, p)) {
      printf("Error: WFDB library error(s) not detected by %s\n",
	      pname);
      printf(" Last library error was: '%s'\n", p);
      errors++;
    }
    else if (vflag)
      printf("[OK]:  no WFDB library errors\n");
  }

  /* In this section, test functions that can only be checked by looking
     for library errors. */

  /* *** flushcal *** */

  flushcal();

  i = getcal("foobar", "mmHg", &cal);
  if (i == 0) {
    printf("Error: flushcal did not empty the calibration list\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  flushcal was successful\n");

  /* Summarize the results and exit. */
  if (errors)
    printf("%d error%s: test failed\n", errors, errors > 1 ? "s" :"");
  else if (vflag)
    printf("no errors: test succeeded\n");
  if (vflag == 2)
    list_untested();
  exit(errors);
}

int check(char *record, char *orec)
{
  WFDB_Date d;
  WFDB_Frequency f;
  WFDB_Time t, tt;
  double x;

  /* *** sampfreq *** */
  if ((f = sampfreq(NULL)) != 0.0) {
    printf("Error: sampfreq(NULL) returned %g (should have been 0)\n", f);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  sampfreq(NULL) returned %g\n", f);

  /* *** setsampfreq *** */
  if (istat = setsampfreq(100.0)) {
    printf("Error: setsampfreq returned %d (should have been 0)\n", istat);
    errors++;
  }
  if (sampfreq(NULL) != 100.0) {
    printf("Error: failed to set sampling frequency using setsampfreq\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  setsampfreq changed sampling frequency successfully\n");

  /* *** sampfreq, again *** */
  if ((f = sampfreq(record)) != 360.0) {
    printf("Error: sampfreq(%s) returned %g (should have been 360)\n",
	   record, f);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  sampfreq(%s) returned %g\n", record, f);

  /* *** annopen *** */
  istat = annopen(record, aiarray, 1);
  if (istat) {
    fprintf(stderr,
	  "Error: annopen of 1 file returned %d (should have been 0)\n", istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  annopen of 1 file succeeded\n");

  istat = annopen(record, aiarray, 2);
  if (istat) {
    fprintf(stderr,
	 "Error: annopen of 2 files returned %d (should have been 0)\n", istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  annopen of 2 files succeeded\n");

  /* *** strecg, ecgstr *** */
  i = strecg("N");
  if (i != 1) {
    printf("Error: strecg returned %d (should have been 1)\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  strecg returned %d\n", i);

  p = ecgstr(i);
  if (strcmp(p, "N")) {
    printf("Error: ecgstr returned '%s' (should have been 'N')\n", p);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  ecgstr returned '%s'\n", p);

  /* *** strann, annstr, anndesc *** */
  i = strann("N");
  if (i != 1) {
    printf("Error: strann returned %d (should have been 1)\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  strann returned %d\n", i);

  p = annstr(i);
  if (strcmp(p, "N")) {
    printf("Error: annstr returned '%s' (should have been 'N')\n", p);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  annstr returned '%s'\n", p);

  p = anndesc(i);
  if (strcmp(p, "Normal beat")) {
    printf("Error: anndesc returned '%s' (should have been 'Normal beat')\n",
	   p);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  anndesc returned '%s'\n", p);

  /* *** setecgstr, setannstr, setanndesc *** */

  i = setecgstr(1, "X");
  p = ecgstr(1);
  if (i != 0 || strcmp(p, "X")) {
    printf("Error: setecgstr failed\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  setecgstr succeeded\n");

  i = setannstr(-1, "Y");
  p = annstr(1);
  if (i != 0 || strcmp(p, "Y")) {
    printf("Error: setannstr failed\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  setannstr succeeded\n");

  i = setanndesc(-1, "ZZ zz");
  p = anndesc(1);
  if (i != 0 || strcmp(p, "ZZ zz")) {
    printf("Error: setanndesc failed\n");
    errors++;
  }
  else if (vflag)
    printf("[OK]:  setanndesc succeeded\n");

  /* *** strtim, timstr *** */
  t = strtim("0:5");
  if (t != (WFDB_Time)(5.0 * f)) {
    printf("Error: strtim returned %ld (should have been %ld)\n", t,
	    (WFDB_Time)(5.0 * f));
    errors++;
  }
  else if (vflag)
    printf("[OK]:  strtim returned %ld\n", t);

  p = timstr(t); q = "    0:05";
  if (strcmp(p, q)) {
    printf("Error: timstr returned '%s' (should have been '%s')\n", p, q);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  timstr returned '%s'\n", p);

  /* *** strdat, datstr *** */
  q = " 31/12/1999";
  d = strdat(q);
  if (d != 2451544L) {
    printf("Error: strdat returned %ld (should have been 2451544)\n", d);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  strdat returned %ld\n", d);
  p = datstr(d);
  if (strcmp(p, q)) {
    printf("Error: datstr returned '%s' (should have been '%s')\n", p, q);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  datstr returned '%s'\n", p);

  /* *** iannsettime *** */
  istat = iannsettime(t);
  if (istat) {
    printf("Error: iannsettime returned %d (should have been 0)\n",
	    istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  iannsettime skipping forward to %s\n",
	    timstr(t));

  /* *** getann, stimstr *** */
  for (i = 0; i < 5; i++) {
    istat = getann(0, &annot);
    if (istat != 0 && istat != -1) {
      printf("Error: getann returned %d (should have been 0 or -1)\n",
	     istat);
      errors++;
    }
    else if (vflag)
      printf("[OK]:  getann read: {%s %d %d %d} at %s (%ld)\n",
	     annstr(annot.anntyp), annot.subtyp, annot.chan, annot.num,
	     mstimstr(annot.time), annot.time);
  }

  /* *** iannsettime, again *** */
  istat = iannsettime(0L);
  if (istat) {
    printf("Error: iannsettime returned %d (should have been 0)\n",
	    istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  iannsettime skipping backward to %s\n",
	    timstr(0L));

  /* *** getann, putann *** */
  i = j = istat = 0;
  while (istat == 0) {
    istat = getann(0, &annot);
    if (istat != 0 && istat != -1) {
      printf("Error: getann returned %d (should have been 0 or -1)\n",
	     istat);
      errors++;
    }
    else if (istat == 0) {
      i++;
      istat = putann(0, &annot);
      if (istat != 0) {
	printf("Error: putann returned %d (should have been 0)\n",
	       istat);
	errors++;
      }
      else j++;
    }
  }
  if (vflag)
    printf("[OK]:  %d annotations read, %d written\n", i, j);

  /* *** isigopen *** */
  /* Get the number of signals without opening any signal files. */
  n = isigopen(record, NULL, 0);
  if (n != 2) {
    fprintf(stderr,
	    "Error: isigopen(%s, NULL, 0) returned %d (should have been 2)\n",
	    record, n);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  isigopen(%s, NULL, 0) succeeded\n", record);
	
  /* Allocate WFDB_Siginfo structures before calling isigopen again. */
  si = calloc(n, sizeof(WFDB_Siginfo));
  nsig = isigopen(record, si, n);
  /* Note that nsig equals n only if all signals were readable. */

  /* Get the number of samples per frame. */
  for (i = framelen = 0; i < nsig; i++)
    framelen += si[i].spf;
  /* Allocate WFDB_Samples before calling getframe. */
  vector = calloc(framelen, sizeof(WFDB_Sample));

  for (t = 0L; t < 5L; t++) {
    if ((n = getframe(vector)) != nsig) {
      printf("Error: getframe returned %d (should have been %d)\n",
	      n, nsig);
      errors++;
      break;
    }
    else if (vflag) {
      printf("[OK]:  (at %s) getframe returned {", mstimstr(t));
      for (i = 0; i < framelen; i++)
	printf("%5d%s", vector[i], i < framelen-1 ? ", " : "}\n");
    }
  }

  /* *** aduphys, physadu *** */
  x = aduphys((WFDB_Signal)0, (WFDB_Sample)1000);
  if (x != -0.12) {
    printf("Error: aduphys returned %g (should have been -0.12)\n", x);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  aduphys returned %g\n", x);

  i = physadu((WFDB_Signal)0, x);
  if (i != (WFDB_Sample)1000) {
    printf("Error: physadu returned %d (should have been 1000)\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  physadu returned %d\n", i);

  /* *** adumuv, muvadu *** */
  i = adumuv((WFDB_Signal)0, (WFDB_Sample)1000);
  if (i != 5000) {
    printf("Error: adumuv returned %d (should have been 5000)\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  adumuv returned %d\n", i);

  i = muvadu((WFDB_Signal)0, i);
  if (i != (WFDB_Sample)1000) {
    printf("Error: muvadu returned %d (should have been 1000)\n", i);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  muvadu returned %d\n", i);

  /* *** sampfreq *** */
  f = sampfreq(NULL);
  if (f != 360.0) {
    printf("Error: sampfreq returned %g (should have been 360)\n", f);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  sampfreq returned %g\n", f);

  /* *** strtim *** */
  t = strtim("0:20");
  if (t != (WFDB_Time)(20.0 * f)) {
    printf("Error: strtim returned %ld (should have been %ld)\n", t,
	    (WFDB_Time)(20.0 * f));
    errors++;
  }
  else if (vflag)
    printf("[OK]:  strtim returned %ld\n", t);

  p = timstr(t); q = "    0:20";
  if (strcmp(p, q)) {
    printf("Error: timstr returned '%s' (should have been '%s')\n", p, q);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  timstr returned '%s'\n", p);
  
  /* *** isigsettime *** */
  istat = isigsettime(t);
  if (istat) {
    printf("Error: isigsettime returned %d (should have been 0)\n",
	    istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  isigsettime skipping forward to %s\n",
	    timstr(t));

  /* *** getvec *** */
  for (tt = t; t < tt+5L; t++) {
    if ((n = getvec(vector)) != nsig) {
      printf("Error: getvec returned %d (should have been %d)\n",
	      n, nsig);
      errors++;
      break;
    }
    else if (vflag) {
      printf("[OK]:  (at %s) getvec returned {", mstimstr(t));
      for (i = 0; i < nsig; i++)
	printf("%5d%s", vector[i], i < nsig-1 ? ", " : "} [");
      for (i = 0; i < nsig; i++)
	printf("%5.3lf%s", aduphys(i, vector[i]), i < nsig-1?", ":"]\n");
    }
  }

  /* *** isigsettime *** */
  t = tt-1; /* try a backward skip, to one sample before the previous set */
  istat = isigsettime(t);
  if (istat) {
    printf("Error: isigsettime returned %d (should have been 0)\n",
	    istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  isigsettime skipping backward to %s\n",
	    mstimstr(t));

  /* *** getframe, again *** */
  for ( ; t < tt+5L; t++) {
    if ((n = getframe(vector)) != nsig) {
      printf("Error: getframe returned %d (should have been %d)\n",
	      n, nsig);
      errors++;
      break;
    }
    else if (vflag) {
      printf("[OK]:  (at %s) getframe returned {", mstimstr(t));
      for (i = 0; i < framelen; i++)
	printf("%5d%s", vector[i], i < framelen-1 ? ", " : "}\n");
    }
  }

  /* Now return to the beginning of the record and copy it. */
  istat = isigsettime(t = 0L);
  if (istat) {
    printf("Error: isigsettime returned %d (should have been 0)\n",
	    istat);
    errors++;
  }
  else if (vflag)
    printf("[OK]:  isigsettime skipping backward to %s\n",
	    mstimstr(t));

  /* *** osigfopen *** */
  for (i = 0; i < nsig; i++) {
    si[i].fname = realloc(si[i].fname, strlen(orec) + 5);
    sprintf(si[i].fname, "%s.dat", orec);
  }
  istat = osigfopen(si, nsig);
  if (istat != nsig) {
      printf("Error: osigfopen returned %d (should have been %d)\n",
	     istat, nsig);
      errors++;
  }
  else if (vflag)
      printf("[OK]:  osigfopen returned %d\n", istat);

  /* *** getframe (again), putvec *** */
  while ((n = getframe(vector)) == nsig) {
    t++;
    if ((istat = putvec(vector)) != nsig) {
      printf("Error: putvec returned %d (should have been %d)\n",
	     istat, nsig);
      errors++;
      break;
    }
  }
  if (n != -1) {	/* some error occurred while reading samples */
    printf("Error: getframe returned %d (should have been %d) at %s\n",
	   n, nsig, mstimstr(t));
    errors++;
  }
  else if (vflag)	/* getframe reached EOF, checksums OK */
    printf("[OK]:  getframe read %ld samples\n", t);
  if (istat != nsig) {	/* some error occurred while writing samples */
    printf("Error: putvec returned %d (should have been %d) at %s\n",
	   istat, nsig, mstimstr(t));
    errors++;
  }
  else if (vflag)	/* putvec wrote all samples without apparent error */
    printf("[OK]:  putvec wrote %ld samples\n", t);

  /* *** newheader *** */
  istat = newheader(orec);
  if (istat) {	/* some error occurred while writing the header */
    printf("Error: newheader returned %d (should have been 0)\n", istat);
    errors++;
  }
  else if (vflag)	/* putvec wrote all samples without apparent error */
    printf("[OK]:  newheader created header for output record %s\n", orec);

  /* *** getinfo, putinfo *** */
  n = 0;
  if (info = getinfo(record)) {
    do {
      istat = putinfo(info);
      if (istat) {
	printf("Error: putinfo returned %d (should have been 0)\n", istat);
	errors++;
      }
      else
	  n++;
    } while (info = getinfo(NULL));
    if (vflag)
	printf("[OK]:  %d info strings copied to record %s header\n", n, orec);
  }

  wfdbquit();
  setecgstr(1, "N");
  setannstr(-1, "N");
  setanndesc(-1, "Normal beat");
}

char *prog_name(s)
char *s;
{
    char *p = s + strlen(s);

#ifdef MSDOS
    while (p >= s && *p != '\\' && *p != ':') {
	if (*p == '.')
	    *p = '\0';		/* strip off extension */
	if ('A' <= *p && *p <= 'Z')
	    *p += 'a' - 'A';	/* convert to lower case */
	p--;
    }
#else
    while (p >= s && *p != '/')
	p--;
#endif
    return (p+1);
}

static char *help_strings[] = {
 "usage: %s [OPTIONS ...]\n",
 " -v          verbose mode",
 " -V          verbose mode, also list untested functions",
 NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}

void list_untested()
{
    printf(
   "This program does not (yet) test the following WFDB library functions:\n");
    printf("osigopen\n");
    printf("wfdbinit\n");
    printf("getspf\n");
    printf("setgvmode\n");
    printf("ungetann\n");
    printf("isgsettime\n");
    printf("iannclose\n");
    printf("oannclose\n");
    printf("setheader\n");
    printf("setmsheader\n");
    printf("wfdbgetskew\n");
    printf("wfdbsetskew\n");
    printf("wfdbgetstart\n");
    printf("wfdbsetstart\n");
    printf("getcfreq\n");
    printf("setcfreq\n");
    printf("getbasecount\n");
    printf("setbasecount\n");
    printf("setbasetime\n");
    printf("wfdbquiet\n");
    printf("wfdbverbose\n");
    printf("setibsize\n");
    printf("setobsize\n");
    printf("wfdbfile\n");
    printf("wfdbflush\n");
    printf("setifreq\n");
    printf("getifreq\n");
}
