/* file: xmlproc.h	G. Moody	20 August 2010
			Last revised:	22 August 2010
-------------------------------------------------------------------------------
xmlproc: generic functions for processing XML files
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

This code should be compiled together with one of xmlhea.c, xmlann.c, or
xmldat.c, which provide implementations of the callback functions start(),
middle(), and end() for processing WFDB-XML header, annotation, and signal
files respectively, as well as the functions cleanup() and help().
*/

#include <expat.h>

/* functions including callbacks defined in xmlhea.c, xmlann.c, xmldat.c */
void XMLCALL start(void *data, const char *el, const char **attr);
void XMLCALL middle(void *data, const char *el, int len);
void XMLCALL end(void *data, const char *el);
void cleanup(void);
void help(void);

#define DATALEN	1024	/* max length of *data passed among callbacks,
			   in characters */

char *pname;
int qflag, vflag;

int main(int argc, char **argv)
{
    FILE *ifile;
    int i;
    void process(FILE *ifile);

    pname = argv[0];
    if (argc < 2) help();
    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	    case '\0': process(stdin); break;
	    case 'h': help(); break;
	    case 'q': qflag = 1; break;
	    case 'v': vflag = 1; break;
	    default: fprintf(stderr, "%s: unrecognized option %s\n",
			     argv[0], argv[i]);
		exit(1);
		break;
	    }
	}
	else {
	    ifile = fopen(argv[i], "rt");
	    if (ifile) {
		process(ifile);
		fclose(ifile);
	    }
	    else {
		fprintf(stderr, "%s: can't open %s\n", argv[0], argv[i]);
		exit(1);
	    }
	}	
    }
    exit(0);
}

/* Definitions needed by process().  XML_LARGE_SIZE and XML_USE_MSC_EXTENSIONS
   may be defined in <expat.h>. */
#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

#define BUFLEN		8192

void process(FILE *ifile)
{
    int done = 0, len;
    static char buf[BUFLEN], userdata[DATALEN];
    XML_Parser p = XML_ParserCreate(NULL);

    if (! p) {
	fprintf(stderr, "Couldn't allocate memory for parser\n");
	exit(2);
    }

    XML_SetUserData(p, userdata);
    XML_SetElementHandler(p, start, end);
    XML_SetCharacterDataHandler(p, middle);

    do {
	len = (int)fread(buf, 1, BUFLEN, ifile);
	if (ferror(ifile)) {
	    fprintf(stderr, "Read error\n");
	    exit(-1);
	}
	done = feof(ifile);

	if (XML_Parse(p, buf, len, done) == XML_STATUS_ERROR) {
	    fprintf(stderr, "Parse error at line %" XML_FMT_INT_MOD "u:\n%s\n",
		    XML_GetCurrentLineNumber(p),
		    XML_ErrorString(XML_GetErrorCode(p)));
	    exit(-1);
	}
    } while (!done);
    XML_ParserFree(p);
    cleanup();
    userdata[0] = '\0';
    if (vflag) printf("\n");
}
