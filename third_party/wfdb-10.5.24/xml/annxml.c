/* file: annxml.c	G. Moody	28 June 2010

-------------------------------------------------------------------------------
heaxml: Convert a WFDB annotation file to XML format
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
#include <wfdb/ecgcodes.h>

#define WFDBXMLPROLOG  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
 "<?xml-stylesheet type=\"text/xsl\"" \
 " href=\"wfdb.xsl\"?>\n" \
 "<!DOCTYPE wfdbannotationset PUBLIC \"-//PhysioNet//DTD WFDB 1.0//EN\"" \
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
    fprintf(ofile, "</%s>", tag);
  }
  return;
}

int nsig;
FILE *ofile;
WFDB_Annotation annot;
WFDB_Frequency sfreq;

void process_start(char *tstring);
void process_anntab(void);
void process_annotation(void);

main(int argc, char **argv)
{
  char *annotator, *ofname, *p, *pname, *record, *prog_name();
    WFDB_Anninfo ai;

    pname = prog_name(argv[0]);
    if (argc < 3) {
      (void)fprintf(stderr, "usage: %s RECORD ANNOTATOR\n", pname);
      exit(1);
    }
    record = argv[1];
    annotator = argv[2];

    /* Discover the number of signals defined in the header. */
    if ((nsig = isigopen(record, NULL, 0)) < 0) exit(2);
    sfreq = sampfreq(record);

    /* Open the input annotation file. */
    ai.name = annotator;
    ai.stat = WFDB_READ;
    if (annopen(record, &ai, 1) < 0)
      exit(3);

    /* The name of the output file is of the form 'RECORD.ANNOTATOR.xml'.  Any
       directory separators (/) in the record name are replaced by hyphens (-)
       in the output file name, so that the output file is always written into
       the current directory. */
    ofname = calloc(strlen(record)+strlen(annotator)+6, sizeof(char));
    sprintf(ofname, "%s.%s.xml", record, annotator);
    for (p = ofname; *p; p++)
      if (*p == '/') *p = '-';

    /* Open the output file and write the XML prolog. */
    if ((ofile = fopen(ofname, "wt")) == NULL) {
      fprintf(stderr, "%s: can't create %s\n", pname, ofname);
      exit(4);
    }
    fprintf(ofile, WFDBXMLPROLOG);

    (void)fprintf(ofile, "<wfdbannotationset annotator=\"%s\""
		  " record=\"%s\">\n", annotator, record);

    process_start(mstimstr(0));

    (void)fprintf(ofile, "<samplingfrequency>%.12g</samplingfrequency>\n",
		  sfreq);

    while (getann(0, &annot) >= 0)
      process_annotation();

    process_anntab();

    fprintf(ofile, "</wfdbannotationset>\n");
    wfdbquit();
    exit(0);
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

static long anncount[ACMAX+1];

void process_anntab()
{
  int i;

  fprintf(ofile, "<anntab>");
  for (i = 0; i <= ACMAX; i++) {
    if (anncount[i]) {
      fprintf(ofile, "<anntabentry><anntype>%d</anntype>", i);
      output_xml(ofile, "anncode", annstr(i));
      output_xml(ofile, "anndescription", anndesc(i));
      fprintf(ofile, "<anncount>%ld</anncount></anntabentry>\n", anncount[i]);
    }
  }
  fprintf(ofile, "</anntab>");
}

void process_annotation()
{
  fprintf(ofile, "<annotation><time>%ld</time><anncode>%s</anncode>",
	  annot.time, annstr(annot.anntyp));
  if (annot.subtyp) fprintf(ofile, "<subtype>%d</subtype>", annot.subtyp);
  if (annot.chan) fprintf(ofile, "<chan>%d</chan>", annot.chan);
  if (annot.num) fprintf(ofile, "<num>%d</num>", annot.num);
  if (annot.aux) output_xml(ofile, "aux", annot.aux+1);
  fprintf(ofile, "</annotation>\n");
  anncount[annot.anntyp]++;
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
