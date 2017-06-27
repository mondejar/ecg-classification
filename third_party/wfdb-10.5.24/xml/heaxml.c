/* file: heaxml.c	G. Moody	28 June 2010
			Last revised:	 1 July 2010
-------------------------------------------------------------------------------
heaxml: Convert a WFDB .hea (header) file to XML format
Copyright (C) 2010 George B. Moody

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

#define WFDBXMLPROLOG  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
 "<?xml-stylesheet type=\"text/xsl\"" \
 " href=\"wfdb.xsl\"?>\n" \
 "<!DOCTYPE wfdbrecord PUBLIC \"-//PhysioNet//DTD WFDB 1.0//EN\"" \
 " \"http://physionet.org/physiobank/database/XML/wfdb.dtd\">\n"

char *token(char *p)
{
  if (p) {
    while (*p && *p != ' ' && *p != '\t' && *p != '\n')
      p++;	/* find whitespace */
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n'))
      p++;	/* find first non-whitespace */
    if (*p == '\0') p = NULL;
  }
  return (p);
}

void output_xml(FILE *ofile, char *tag, char *p)
{
  if (p) {
    fprintf(ofile, "<%s>", tag);
    while (*p) {
      if (*p == '<') fprintf(ofile, "&lt;");
      else if (*p == '>') fprintf(ofile, "&gt;");
      else if (*p == '&') fprintf(ofile, "&amp;");
      else if (*p == '>') fprintf(ofile, "&gt;");
      else if (*p == '"') fprintf(ofile, "&quot;");
      else if (*p == '\'') fprintf(ofile, "&apos;");
      else fprintf(ofile, "%c", *p);
      p++;
    }
    fprintf(ofile, "</%s>\n", tag);
  }
  return;
}

int nsig;
FILE *ofile;
WFDB_Siginfo *s;
WFDB_Time t;

void process_start(char *tstring);

main(int argc, char **argv)
{
    char *ofname, *info, *p, *pname, *record, *prog_name();
    int i, in_msrec = 0, nsegments;
    WFDB_Seginfo *seg, *sp;

    pname = prog_name(argv[0]);
    if (argc < 2) {
      (void)fprintf(stderr, "usage: %s RECORD\n", pname);
      exit(1);
    }
    record = argv[1];

    /* Discover the number of signals defined in the header. */
    if ((nsig = isigopen(record, NULL, 0)) < 0) exit(2);

    /* The name of the output file is the record name with an
       appended ".hea.xml".  Any directory separators (/) in
       the record name are replaced by hyphens (-) in the
       output file name, so that the output file is always
       written into the current directory. */
    ofname = calloc(strlen(record)+9, sizeof(char));
    strcpy(ofname, record);
    for (p = ofname; *p; p++)
      if (*p == '/') *p = '-';
    strcat(ofname, ".hea.xml");

    /* Open the output file and write the XML prolog. */
    if ((ofile = fopen(ofname, "wt")) == NULL) {
      fprintf(stderr, "%s: can't create %s\n", pname, ofname);
      exit(3);
    }
    fprintf(ofile, WFDBXMLPROLOG);

    /* Allocate storage for nsig signal information structures. */
    if (nsig > 0 && (s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(4);
    }
    nsig = isigopen(record, s, -nsig);

    (void)fprintf(ofile, "<wfdbrecord name=\"%s\">\n\n", record);
    setgvmode(WFDB_LOWRES);
    t = strtim("e");
    if (nsig > 0 && (s[0].fmt == 0 || s[0].nsamp != 0) && s[0].nsamp != t) {
 	in_msrec = 1;
	nsegments = getseginfo(&sp);
	seg = calloc(nsegments, sizeof(WFDB_Seginfo));
	for (i = 0; i < nsegments; i++)
	  seg[i] = sp[i];
	sp = seg;
    }

    process_record();

    if (in_msrec) {
      static char *segname, nextts[30];

      segname = calloc(strlen(record)+20, sizeof(char));
      for (p = record + strlen(record) - 1; p > record && *p != '/'; p--)
	;
      *p = '\0';
      /* If segment 0 has 0 samples, it's a layout segment, and the information
	 in it has already been output above, so skip it; otherwise, it's a
	 regular segment, so process it.  The initialization of i in the for
	 loop below tests for this. */
      for (i = sp[0].nsamp ? 0 : 1; i < nsegments; i++) {
	fprintf(stderr, "segment %d: %s\n", i, sp[i].recname);
	if (strcmp("~", sp[i].recname) == 0) {
	  fprintf(ofile, "\n<segment name=\"gap_%d\">\n<gap>\n", i);
	  process_start(nextts);
	  fprintf(ofile, "<length>%ld</length></gap>\n", sp[i].nsamp);
	}
	else {
	  sprintf(segname, "%s/%s", record, sp[i].recname);
	  wfdbquit();
	  nsig = isigopen(segname, NULL, 0);
	  nsig = isigopen(segname, s, -nsig);
	  fprintf(ofile, "\n<segment name=\"%s\">\n", sp[i].recname);
	  setgvmode(WFDB_LOWRES);
	  t = strtim("e");
	  strcpy(nextts, mstimstr(-t));
	  process_record();
	}
	fprintf(ofile, "</segment>\n");
      }
    }
    fprintf(ofile, "</wfdbrecord>\n");
    exit(0);
}

void process_info(void)
{
    char *info, *p;

    if (info = getinfo((char *)NULL)) {
      double age = -1.0;
      char *sex = NULL;

      fprintf(ofile, "\n<info>\n");
      /* Find the first non-space in the first info string. */
      for (p = info; *p && *p == ' '; p++)
	;
      if ('0' <= *p && *p <= '9') {
	/* If the first token of the first info string is numeric, the
	   current .hea file does not have tagged info, and the first
	   and second tokens are the age and sex; and the second info
	   string (if present) contains the medications. Handle this
	   case first. */
	sscanf(p, "%lf", &age);
	if (age >= 0) fprintf(ofile, "<age>%g</age>\n", age);
	p = token(p); /* go to the next token */
	if (p && (*p == 'm' || *p == 'M')) sex = "M";
	else if (p && (*p == 'f' || *p == 'F')) sex = "F";
	if (sex) fprintf(ofile, "<sex>%s</sex>\n", sex);
	/* If there are any more tokens, save them as 'extra' info. */
	if (p = token(p))
	  output_xml(ofile, "extra", p);
	if (info = getinfo((char *)NULL)) {
	  output_xml(ofile, "medication", info);
	  info = getinfo((char *)NULL);
	}
      }
      /* process standard (tagged) info */
      while (info) {
	if (age < 0) {
	  if ((p = strstr(info, "age")) || (p = strstr(info, "Age"))) {
	    if (p = token(p)) {
	      sscanf(p, "%lf", &age);
	      if (age >= 0) fprintf(ofile, "<age>%g</age>\n", age);
	    }
	    /* Additional tagged data may follow age.  Continue processing
	       the remainder of this info string below. */
	    if (!(info = token(p)))
	      /* If there is nothing else, get the next info if any. */
	      info = getinfo((char *)NULL);
	  }
	}
	if (sex == NULL) {
	  if (info &&
	      ((p = strstr(info, "sex"))||(p = strstr(info, "Sex")))) {
	    if ((p = token(p)) && (*p == 'm' || *p == 'M')) sex = "M";
	    else if (p && (*p == 'f' || *p == 'F')) sex = "F";
	    if (sex) fprintf(ofile, "<sex>%s</sex>\n", sex);
	    /* Additional tagged data may follow sex.  Continue processing
	       the remainder of this info string. */
	    if (!(info = token(p)))
		/* If there is nothing else, get the next info if any. */
	      info = getinfo((char *)NULL);
	  }
	}
	/* Diagnoses may be present in more than one info string. */
	if (info && *info &&
	    ((p=strstr(info,"diagnos")) || (p=strstr(info,"Diagnos")))) {
	  if ((p = token(p)) == NULL)
	    /* If nothing follows the 'diagnosis' tag, assume the next info
	       is the diagnosis. */
	    p = getinfo((char *)NULL);
	  if (p) {
	    output_xml(ofile, "diagnosis", p);
	  /* This info has been consumed;  get the next info if any. */
	    info = getinfo((char *)NULL);
	    continue;
	  }
	}
	if (info && *info && 
	    ((p=strstr(info,"medication"))||(p=strstr(info,"Medication")))) {
	  if ((p = token(p)) == NULL)
	    /* If nothing follows the 'medication' tag, assume the next info
	       is the medication. */
	    p = getinfo((char *)NULL);
	  if (p) {
	    output_xml(ofile, "medication", p);
	    /* This info has been consumed;  get the next info if any. */
	    info = getinfo((char *)NULL);
	    continue;
	  }
	}
	/* Process any info that was not identified above. */
	if (info && *info)
	  output_xml(ofile, "other", info);
	info = getinfo((char *)NULL);
      }
      fprintf(ofile, "</info>\n");
    }
}

void process_start(char *p)
{
    if (*p == '[') {
      int day = -1, month = -1, year = -1;
      double hour = -1.0, minute = -1.0, second = -1.0;

      fprintf(ofile, "<start>\n");
      sscanf(p+1, "%lf:%lf:%lf %d/%d/%d",
	     &hour, &minute, &second, &day, &month, &year);
      if (year >= 0) {
	if (year < 100) year += 1900;
	if (year < 1975) year += 100;
	fprintf(ofile, "<year>%d</year>\n", year);
      }
      if (month > 0) fprintf(ofile, "<month>%d</month>\n", month);
      if (day > 0) fprintf(ofile, "<day>%d</day>\n", day);

      if (second < 0) { /* incomplete start time in MM:SS or SS format */
	if (minute < 0) { second = hour; hour = -1; } /* SS format */
	else { second = minute; minute = hour; hour = -1; } /* MM:SS */
      }

      if (hour >= 0) fprintf(ofile, "<hour>%g</hour>\n", hour);
      if (minute >= 0) fprintf(ofile, "<minute>%g</minute>\n", minute);
      if (second >= 0) fprintf(ofile, "<second>%g</second>\n", second);
      fprintf(ofile, "</start>\n");
    }
}

int process_record(void)
{
    char *p;
    double cfreq, sfreq;
    int i, skew;
    FILE *ifile;

    /* Process and output info. */
    process_info();

    /* Output the <start> section if the starting time is specified. */
    process_start(mstimstr(0L));

    if (nsig < 1 || t > 0L)
      (void)fprintf(ofile, "<length>%ld</length>\n", t);
    else if (s[0].fmt && (ifile = fopen(s[0].fname, "rb")) &&
	     (fseek(ifile, 0L, 2) == 0)) {
	int framesize = 0;
	long nbytes = ftell(ifile) - wfdbgetstart(0);

	fclose(ifile);
	for (i = 0; i < nsig && s[i].group == 0; i++)
	    framesize += s[i].spf;
	switch (s[0].fmt) {
	  case 8:
	  case 80:
	    t = nbytes / framesize;
	    break;
	  default:
	  case 16:
	  case 61:
	  case 160:
	    t = nbytes / (2*framesize);
	    break;
	  case 212:
	    t = (2L * nbytes) / (3*framesize);
	    break;
	  case 310:
	  case 311:
	    t = (3L * nbytes) / (4*framesize);
	    break;
	}
      (void)fprintf(ofile, "<length>%ld</length>\n", t);
    }

    (void)fprintf(ofile, "<samplingfrequency>%.12g</samplingfrequency>\n",
		  sfreq = sampfreq(NULL));
    (void)fprintf(ofile, "<signals>%d</signals>\n", nsig);
    cfreq = getcfreq();
    if (sfreq != cfreq) {
      double basecount = getbasecount();
      fprintf(ofile, "<counterfrequency");
      if (basecount > 0.0) fprintf(ofile, " basecount=\"%g\"", basecount);
      fprintf(ofile, ">%.12g</counterfrequency>\n", cfreq);
    }

    if (nsig > 0) { 
      for (i = 0; i < nsig; i++) {
	if (i == 0 || s[i].group != s[i-1].group) {
	  long plen;

	  if (i > 0) fprintf(ofile, "</signalfile>\n\n");
	  fprintf(ofile, "<signalfile>\n<filename>%s</filename>\n\n",
		  s[i].fmt ? s[i].fname : "[none]");
	  if (plen = wfdbgetstart(i))
	    fprintf(ofile, "<preamblelength>%ld</preamblelength>\n", plen);
	}
	fprintf(ofile, "<signal>\n<description>%s</description>\n",
		s[i].desc);
	fprintf(ofile, "<gain>%.12g</gain>\n",
		s[i].gain == 0. ? WFDB_DEFGAIN : s[i].gain);
	fprintf(ofile, "<units>%s</units>\n",
		s[i].units ? s[i].units : "mV");
	fprintf(ofile, "<initialvalue>%d</initialvalue>\n", s[i].initval);
	fprintf(ofile, "<storageformat>%d</storageformat>\n", s[i].fmt);
	if (s[i].spf > 1)
	  fprintf(ofile,"<samplesperframe>%d</samplesperframe>\n", s[i].spf);
	if (skew = wfdbgetskew(i))
	  fprintf(ofile, "<skew>%ld</skew>\n", skew);
	fprintf(ofile, "<blocksize>%d</blocksize>\n", s[i].bsize);
	fprintf(ofile, "<adcresolution>%d</adcresolution>\n", s[i].adcres);
	fprintf(ofile, "<adczero>%d</adczero>\n", s[i].adczero);
	fprintf(ofile, "<baseline>%d</baseline>\n", s[i].baseline);
	fprintf(ofile, "<checksum>%d</checksum>\n", s[i].cksum);
	fprintf(ofile, "</signal>\n\n");
      }
    }
    if (nsig > 0) fprintf(ofile, "</signalfile>\n\n");
    return (0);
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
