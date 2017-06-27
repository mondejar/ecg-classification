/* file: xmlann.c	G. Moody	22 August 2010

-------------------------------------------------------------------------------
xmlann: Convert a WFDB-XML file to a WFDB-compatible annotation file
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

The input to xmlann should be an XML file containing a <wfdbannotationset>,
as specified by 'wfdb.dtd';  see '100s.atr.xml' for a sample in this format.
The inverse transformation can be performed by 'annxml'.

TODO: collect anntabentry items and write anntab (rearrange in annhea
so this can be done in one pass).  Optionally write a simple header
file and include sampling frequency/length/start attributes in it.
Define and handle new annotation dur and url items.  */

#include <stdio.h>
#include <string.h>
#include <wfdb/wfdb.h>
#include "xmlproc.h"	/* provides main(), process(), DATALEN, qflag, vflag */

char *content;
int depth, plen, output_not_open;

WFDB_Anninfo ai;
WFDB_Annotation a;
char *record, *rec;
double sps;

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

  if (strcmp("wfdbannotationset", el) == 0) {
      if (depth) {
	  fprintf(stderr, "Malformed input: wfdbannotationset not at root\n");
	  //	  exit(1);
      }
      for (i = 0; attr[i]; i += 2) {
	  if (attr[i] && strcmp("annotator", attr[i]) == 0)
	      SSTRCPY(ai.name, attr[i+1]);
	  if (attr[i] && strcmp("record", attr[i]) == 0)
	      SSTRCPY(record, attr[i+1]);
      }
      if (ai.name == NULL || record == NULL) {
	  fprintf(stderr, "Malformed input: wfdbannotationset is missing a"
		  " required annotator or record attribute\n");
	  //	  exit(1);
      }
      for (rec = record + strlen(record); rec > record; rec--)
	  if (*rec == '/') { rec++; break; }
      output_not_open = 1;
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
  int i;
  long t;

  depth--;
  if (depth == 0) {
      if (strcmp("wfdbannotationset", el) == 0) {
	  wfdbquit();
	  if (!qflag) printf("%s.%s\n", rec, ai.name);
      }
      else if (record) {
	  fprintf(stderr, "Malformed input ends without /wfdbannotationset\n");
	  //	  exit(1);
      }
      else {
	  fprintf(stderr, "No wfdbannotationset in input\n");
	  //	  exit(1);
      }
  }
  else if (strcmp("samplingfrequency", el) == 0)
      sscanf(content, "%lf", &sps);
  else if (strcmp("/wfdbannotationset/annotation", data) == 0) {
      if (output_not_open) {
	  if (sps > 0) setafreq(sps);
	  ai.stat = WFDB_WRITE;
	  annopen(rec, &ai, 1);
	  output_not_open = 0;
      }
      putann(0, &a);
      a.anntyp = a.subtyp = a.chan = a.num = 0;
      a.aux = NULL;
  }
  else if (strcmp("/wfdbannotationset/annotation/time", data) == 0) {
      sscanf(content, "%ld", &t);
      a.time = t;
  }
  else if (strcmp("/wfdbannotationset/annotation/anncode", data) == 0) {
      a.anntyp = strann(content);
  }
  else if (strcmp("/wfdbannotationset/annotation/subtype", data) == 0) {
      sscanf(content, "%d", &i);
      a.subtyp = i;
  }
  else if (strcmp("/wfdbannotationset/annotation/chan", data) == 0) {
      sscanf(content, "%d", &i);
      a.chan = i;
  }
  else if (strcmp("/wfdbannotationset/annotation/num", data) == 0) {
      sscanf(content, "%d", &i);
      a.num = i;
  }
  else if (strcmp("/wfdbannotationset/annotation/aux", data) == 0) {
      static char auxbuf[256];

      if ((i = strlen(content)) > 254) { i = 254; }
      a.aux = auxbuf;
      auxbuf[0] = i;
      strncpy(auxbuf+1, content, i);
      auxbuf[i+1] = '\0';
  }

  plen = 0;
  for (p = data + strlen(data) - 1; p > (char *)data; p--)
      if (*p == '/') { *p = '\0'; break; }
  if (vflag) { printf("."); fflush(stdout); }
}

void cleanup()
{
    content = NULL;
    depth = plen = output_not_open = 0;
    ai.name = NULL;
    a.anntyp = a.subtyp = a.chan = a.num = 0;
    a.aux = record = rec = NULL;
    sps = 0;
}

void help()
{
    fprintf(stderr, "usage: %s [ OPTION ...] [ FILE ... ]\n", pname);
    fprintf(stderr, " OPTIONs may include:\n"
	    "  -h   print this usage summary\n"
	    "  -q   quiet mode (print errors only)\n"
	    "  -v   verbose mode\n"
	    " FILE is the name of a WFDB-XML annotation file to be converted\n"
	    " into one or more WFDB annotation files, or '-' to convert the\n"
	    " standard input.\n");
    exit(1);
}
