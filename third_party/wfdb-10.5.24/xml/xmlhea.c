/* file: xmlhea.c	G. Moody	20 August 2010
			Last revised:	22 August 2010
-------------------------------------------------------------------------------
xmlhea: Convert an XML file to a WFDB-compatible .hea (header) file
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

The input to this program should be an XML file containing a <wfdbrecord>,
as specified by 'wfdb.dtd';  see '100s.hea.xml' for a sample in this format.
The inverse transformation can be performed by 'heaxml'.
*/

#include <stdio.h>
#include <string.h>
#include <wfdb/wfdb.h>
#include "xmlproc.h"	/* provides main(), process(), DATALEN, qflag, vflag */

void write_header(char *recname);
void writeinfo(char *tag, char *data);

char *content, *dp;
int depth, plen;

WFDB_Siginfo *si;
char *record, *rec, *sfname, *age, *sex, **diagnosis, *extra, **medication,
    **other;
double sps, cps, cbase, second;
int nseg, nsig, sig, dx, rx, ox;
int year, month, day, hour, minute;
long length, offset;
WFDB_Seginfo *segi;

int mnsig;
double msps, mcps, mcbase, msecond;
int myear, mmonth, mday, mhour, mminute;
long mlength;

struct asiginfo {
    char *fname;
    int fmt;
    int skew;
    long offset;
} *sa;
static char info[254], *ip;

void XMLCALL start(void *data, const char *el, const char **attr)
{
  int i;

  sprintf(data + strlen(data), "/%s", el);
  if (vflag) {
      printf("\n%s", data);
      for (i = 0; attr[i]; i += 2)
	  printf(" %s='%s'", attr[i], attr[i + 1]);
      fflush(stdout);
  }

  if (depth == 0)
      dp = data;
  if (strcmp("wfdbrecord", el) == 0) {
      if (depth) {
	  fprintf(stderr, "Malformed input: wfdbrecord not at root\n");
	  //	  exit(1);
      }
      if (!attr[0] || strcmp("name", attr[0])) {
	  fprintf(stderr, "Malformed input: no name for wfdbrecord\n");
	  //	  exit(1);
      }
      dp = data + strlen(data);
      SSTRCPY(record, attr[1]);
      for (rec = record + strlen(record); rec > record; rec--)
	  if (*rec == '/') { rec++; break; }
      SUALLOC(sfname, strlen(rec)+5, sizeof(char));
      sprintf(sfname, "%s.dat", rec);    /* default signal file name */
  }
  if (strcmp("counterfrequency", el) == 0) {
      for (i = 0; attr[i]; i += 2) {
	  if (strcmp(attr[i], "basecount") == 0)
	      sscanf(attr[i+1], "%lf", &cbase);
      }
  }
  if (strcmp("/wfdbrecord/segment", data) ==  0) {
      dp = data + strlen(data);
      if (!attr[0] || strcmp("name", attr[0])) {
	  fprintf(stderr, "Malformed input: no name for segment %d\n", nseg);
	  //	  exit(1);
      }
      if (nseg == 0) {
	  SUALLOC(segi, 1, sizeof(WFDB_Seginfo));

	  mnsig = nsig; msps = sps; mcps = cps; mcbase = cbase;
	  myear = year; mmonth = month; mday = day;
	  mhour = hour; mminute = minute; msecond = second;
	  mlength = length;

	  if (si && strcmp(sfname, "[none]") == 0) {
	      char *layout_rec;

	      SUALLOC(layout_rec, strlen(rec) + 8, sizeof(char));
	      sprintf(layout_rec, "%s_layout", rec);
	      strncpy(segi[0].recname, layout_rec, WFDB_MAXRNL);
	      for (i = 0; i < nsig; i++) {
		  SSTRCPY(sa[i].fname,"~");
		  segi[0].nsamp = si[i].nsamp = si[i].cksum = 0;
	      }
	      write_header(layout_rec);
	      nseg = 1;
	  }
      }
      if (nseg > 0) {
	  SREALLOC(segi, nseg+1, sizeof(WFDB_Seginfo));
      }
      strncpy(segi[nseg].recname, attr[1], WFDB_MAXRNL);
      sig = dx = rx = ox = 0;
      SFREE(diagnosis); SFREE(medication); SFREE(other);
  }

  depth++;
}

void XMLCALL middle(void *data, const char *el, int len)
{
    if (plen == 0) {	/* not in the same element as last time */
        if (len == 1 && *el == '\n')
	    return;  /* ignore newlines outside of tags */
	SALLOC(content, plen = len + 1, sizeof(char));
	strncpy(content, el, len);
    }
    else {
	SREALLOC(content, plen + len, sizeof(char));
	strncpy(content + plen - 1, el, len);
	plen += len;
    }
    if (vflag) { printf("\t|%s|", content); fflush(stdout); }
}
 
void XMLCALL end(void *data, const char *el)
{
  char *p;

  depth--;
  if (depth == 0) {
      if (strcmp("wfdbrecord", el) == 0) {
	  if (nseg == 0)
	      write_header(rec);
	  else {
	      char *ofname;
	      FILE *ofile;
	      int i;

	      SUALLOC(ofname, strlen(rec)+5, sizeof(char));
	      sprintf(ofname, "%s.hea", rec);
	      ofile = fopen(ofname, "wt");
	      fprintf(ofile, "%s/%d %d %.12g", rec, nseg, mnsig, msps);
	      if (mcps) {
		  fprintf(ofile, "%.12g", mcps);
		  if (mcbase)
		      fprintf(ofile, "(%.12g)", mcbase);
	      }
	      fprintf(ofile, " %ld %02d:%02d:%02d",
		      mlength, mhour, mminute, (int)msecond);
	      if (i = ((int)(msecond*1000. + 0.5)) % 1000)
		  fprintf(ofile, ".%03d", i);
	      if (myear || mmonth || mday)
		  fprintf(ofile, " %02d/%02d/%d", mday, mmonth, myear);
	      fprintf(ofile, "\r\n");
	      for (i = 0; i < nseg; i++)
		  fprintf(ofile, "%s %ld\r\n", segi[i].recname, segi[i].nsamp);
	      if (!qflag) printf("%s\n", ofname);
	  }
      }
      else if (record) {
	  fprintf(stderr, "Malformed input ends without /wfdbrecord\n");
	  //	  exit(1);
      }
      else {
	  fprintf(stderr, "No wfdbrecord in input\n");
	  //	  exit(1);
      }
  }
  else if (depth == 1) {
      if (strcmp("length", el) == 0)
	  sscanf(content, "%ld", &length);
      else if (strcmp("samplingfrequency", el) == 0)
	  sscanf(content, "%lf", &sps);
      else if (strcmp("counterfrequency", el) == 0)
	  sscanf(content, "%lf", &cps);
      else if (strcmp("signals", el) == 0) {
	  sscanf(content, "%ld", &nsig);
	  SUALLOC(si, sizeof(WFDB_Siginfo), nsig);
	  SUALLOC(sa, sizeof(struct asiginfo), nsig);
      }
      else if (strcmp("segment", el) == 0) {
	  if (strcmp(segi[nseg].recname, "~"))
	      write_header(segi[nseg].recname);
	  nseg++;
      }
  }

  else if (strcmp("/wfdbrecord/segment/length", data) == 0) {
      sscanf(content, "%ld", &length);
      segi[nseg].nsamp = length;
  }
  else if (strcmp("/wfdbrecord/segment/gap/length", data) == 0) {
      strcpy(segi[nseg].recname, "~");
      sscanf(content, "%ld", &length);
      segi[nseg].nsamp = length;
  }
  else if (strcmp("/wfdbrecord/segment/signals", data) == 0)
      sscanf(content, "%ld", &nsig);
  
  else if (strcmp("/info/age", dp) == 0) {
      SSTRCPY(age, content);
  }
  else if (strcmp("/info/sex", dp) == 0) {
      SSTRCPY(sex, content);
  }
  else if (strcmp("/info/extra", dp) == 0) {
      SSTRCPY(extra, content);
  }
  else if (strcmp("/info/diagnosis", dp) == 0) {
      if (dx == 0) { SUALLOC(diagnosis, 1, sizeof(char *)); }
      else { SREALLOC(diagnosis, dx+1, sizeof(char *)); }
      if (diagnosis[dx]) {
	  SREALLOC(diagnosis[dx], strlen(content)+1, sizeof(char));
      }
      else {
	  SUALLOC(diagnosis[dx], strlen(content)+1, sizeof(char));
      }
      strcpy(diagnosis[dx++], content);
  }
  else if (strcmp("/info/medication", dp) == 0) {
      if (rx == 0) { SUALLOC(medication, 1, sizeof(char *)); }
      else { SREALLOC(medication, rx+1, sizeof(char *)); }
      if (medication[rx]) {
	  SREALLOC(medication[rx], strlen(content)+1, sizeof(char));
      }
      else {
	  SUALLOC(medication[rx], strlen(content)+1, sizeof(char));
      }
      strcpy(medication[rx++], content);
  }
  else if (strcmp("/info/other", dp) == 0) {
      if (ox == 0) { SUALLOC(other, 1, sizeof(char *)); }
      else { SREALLOC(other, ox+1, sizeof(char *)); }
      SUALLOC(other[ox], strlen(content)+1, sizeof(char));
      strcpy(other[ox++], content);
  }

  else if (strcmp("/start/year", dp) == 0) {
      sscanf(content, "%ld", &year);
  }
  else if (strcmp("/start/month", dp) == 0) {
      sscanf(content, "%ld", &month);
  }
  else if (strcmp("/start/day", dp) == 0) {
      sscanf(content, "%ld", &day);
  }
  else if (strcmp("/start/hour", dp) == 0) {
      sscanf(content, "%ld", &hour);
  }
  else if (strcmp("/start/minute", dp) == 0) {
      sscanf(content, "%ld", &minute);
  }
  else if (strcmp("/start/second", dp) == 0) {
      sscanf(content, "%lf", &second);
  }

  else if (strcmp("/signalfile/filename", dp) == 0) {
      SSTRCPY(sfname, content);
  }
  else if (strcmp("/signalfile/preamblelength", dp) == 0) {
      sscanf(content, "%ld", &offset);
  }
  else if (strcmp("/signalfile/signal", dp) == 0) {
      SSTRCPY(sa[sig].fname, sfname);
      sa[sig].offset = offset;
      si[sig].nsamp = length;
      sig++;
  }
  else if (strcmp("/signalfile/signal/description", dp) == 0) {
      SSTRCPY(si[sig].desc, content);
  }
  else if (strcmp("/signalfile/signal/gain", dp) == 0) {
      sscanf(content, "%lf", &(si[sig].gain));
  }
  else if (strcmp("/signalfile/signal/units", dp) == 0) {
      SSTRCPY(si[sig].units, content);
  }
  else if (strcmp("/signalfile/signal/initialvalue", dp) == 0) {
      sscanf(content, "%d", &(si[sig].initval));
  }
  else if (strcmp("/signalfile/signal/storageformat", dp) == 0) {
      sscanf(content, "%d", &sa[sig].fmt);
  }
  else if (strcmp("/signalfile/signal/blocksize", dp) == 0) {
      sscanf(content, "%d", &(si[sig].bsize));
  }
  else if (strcmp("/signalfile/signal/adcresolution", dp) == 0) {
      sscanf(content, "%d", &(si[sig].adcres));
  }
  else if (strcmp("/signalfile/signal/adczero", dp) == 0) {
      sscanf(content, "%d", &(si[sig].adczero));
  }
  else if (strcmp("/signalfile/signal/baseline", dp) == 0) {
      sscanf(content, "%d", &(si[sig].baseline));
  }
  else if (strcmp("/signalfile/signal/checksum", dp) == 0) {
      sscanf(content, "%d", &(si[sig].cksum));
  }
  else if (strcmp("/signalfile/signal/samplesperframe", dp) == 0) {
      sscanf(content, "%d", &(si[sig].spf));
  }
  else if (strcmp("/signalfile/signal/skew", dp) == 0) {
      sscanf(content, "%d", &sa[sig].skew);
  }

  plen = 0;
  for (p = data + strlen(data) - 1; p > (char *)data; p--)
      if (*p == '/') { *p = '\0'; break; }
  if (vflag) { printf("."); fflush(stdout); }
}

void write_header(char *recname)
{
    int i, dxlen, rxlen, oxlen;

    if (!qflag) printf("%s.hea\n", recname);
    if (sps > 0)
	setsampfreq(sps);
    if (cps > 0) {
	setcfreq(cps);
	if (cbase > 0) setbasecount(cbase);
    }
    if (year || month || day || hour || minute || second) {
	char tstart[24];
	if (year || month || day)
	    sprintf(tstart, "%02d:%02d:%g %02d/%02d/%d",
		    hour, minute, second, day, month, year);
	else
	    sprintf(tstart, "%02d:%02d:%g", hour, minute, second);
	setbasetime(tstart);
    }

    /* IMPORTANT: Before invoking osigopen, si[*].fname is set to "~", and
       si[*].fmt is set to 0, to ensure that osigfopen does not actually create
       (empty) output signal files (which would destroy any existing files of
       the same names).  It's necessary to call osigfopen in order to allow
       skews and offsets to be set, however.  si[*].fname and si[*].fmt must be
       set to their intended values (which have been preserved in sa[*]) after
       calling osigfopen and before calling setheader.
    */
    for (i = 0; i < nsig; i++) {
	si[i].fname = "~";
	si[i].fmt = 0;
    }
     if (osigfopen(si, nsig) == nsig) {
	for (i = 0; i < nsig; i++) {
	    si[i].fname = sa[i].fname;
	    si[i].fmt = sa[i].fmt;
	    wfdbsetskew(i, sa[i].skew);
	    wfdbsetstart(i, sa[i].offset);
	}
    }
    setheader(recname, si, nsig);

    /* After calling setheader, the info strings are written to the output. */
    if (age == NULL || strlen(age) > 5) age = "?";
    if (sex == NULL || strlen(sex) > 20) sex = "?";
    sprintf(info, " <age>: %s <sex>: %s", age, sex);
    if (dx == 0) sprintf(info + strlen(info), " <diagnoses>: ?");
    if (dx == 1 && strlen(diagnosis[0]) < 78 - (strlen(info) + 14)) {
	sprintf(info + strlen(info), " <diagnoses>: %s", diagnosis[0]);
	dx = 0;
    }
    if (rx == 0) sprintf(info + strlen(info), " <medications>: ?");
    if (rx == 1 && strlen(medication[0]) < 78 - (strlen(info) + 16)) {
	sprintf(info + strlen(info), " <medications>: %s", medication[0]);
	rx = 0;
    }
    putinfo(info);
    if (dx)
	for (i = 0; i < dx; i++)
	    writeinfo("<diagnoses>: ", diagnosis[i]);
    if (rx)
	for (i = 0; i < rx; i++)
	    writeinfo("<medications>: ", medication[i]);
    if (extra)
	writeinfo(" ", extra);
    if (ox > 1)
	for (i = 0; i < ox; i++)
	    writeinfo("", other[i]);

    wfdbquit();	/* flush any pending writes and close files */
}

void writeinfo(char *tag, char *data)
{
    char info[80];
    int len = strlen(tag) + strlen(data);

    while (len >= 80) {
	strcpy(info, tag);
	strncat(info, data, 79-strlen(tag));
	putinfo(info);
	data += 79-strlen(tag);
	tag = "# ";
    }
    strcpy(info, tag);
    strcat(info, data);
    putinfo(info);
}

void cleanup()
{
    content = dp = NULL;
    depth = plen = 0;
    si = NULL;
    record = rec = sfname = age = sex = extra = NULL;
    diagnosis = medication = other = NULL;
    sps = cps = cbase = second = 0;
    nseg = nsig = sig = dx = rx = ox = 0;
    year = month = day = hour = minute = 0;
    length = offset = 0;
    segi = NULL;
    mnsig = 0;
    msps = mcps = mcbase = msecond = 0;
    myear = mmonth = mday = mhour = mminute = 0;
    mlength = 0;
    sa = NULL;
    info[0] = '\0';
    ip = NULL;
}

void help()
{
    fprintf(stderr, "usage: %s [ OPTION ...] [ FILE ... ]\n", pname);
    fprintf(stderr, " OPTIONs may include:\n"
	    "  -h   print this usage summary\n"
	    "  -q   quiet mode (print errors only)\n"
	    "  -v   verbose mode\n"
	    " FILE is the name of a WFDB-XML header file to be converted\n"
	    " into one or more WFDB .hea files, or '-' to convert the\n"
	    " standard input.\n");
    exit(1);
}
