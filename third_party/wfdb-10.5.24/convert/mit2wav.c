/* file: mit2wav.c	G. Moody	12 February 2003
			Last revised:	  27 July 2010
-------------------------------------------------------------------------------
mit2wav: Convert WFDB format signal file(s) to .wav format
Copyright (C) 2003-2010 George B. Moody

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

This program converts a WFDB record into .wav format (format 16, multiplexed
signals, with embedded header information).  Use 'wav2mit' to perform the
reverse conversion.

You may be able to use this program's output to create analog signals by
playing the .wav file through a sound card, but you should be aware of the
following potential pitfalls:

* Your sound card, and the software that comes with it, may not be able to
  play .wav files containing three or more signals.  If this is a problem,
  you will need to extract one or two signals to include in the .wav file
  from your original recording (for example, using 'xform').

* Your sound card and its software may be unable to play .wav files at other
  than certain fixed sampling frequencies (typically 11025, 22050, and 44100
  Hz).  If this is a problem, you will need to resample the input at one of
  the frequencies supported by your sound card (for example, using 'xform')
  before converting it to .wav format using this program.

* Your sound card may not be able to reproduce the frequencies present in
  the input.  This is *very* likely if you are trying to recreate physiologic
  signals such as ECGs (with most of the useful information in the 0.1 to 30
  Hz band) using a consumer sound card (which probably does not reproduce
  frequencies below the lower limit of human hearing (around 30 Hz).  One
  possible solution to this problem is to create a digital signal containing
  a higher-frequency carrier signal, amplitude-modulated by the signal of
  interest, and to convert this AM signal into a .wav file;  on playback,
  an analog AM demodulator would then recover the original low-frequency
  signal of interest.  If you successfully implement this solution, please
  send details to the author (george@mit.edu).

  If the output contains 2, 3, 4, or 6 signals, and you play it through a
  sound card with at least that many outputs, the association between the
  digital inputs and the analog outputs is as follows:

  signals  input  output
     2	     0     left
             1     right

     3	     0     left
             1     right
             2     center

     4       0     front left     -or-  left
             1     front right          center
             2     rear left            right
             3     rear right           surround

     6       0     left
             1     left center
             2     center
             3     right center
             4     right
             5     surround          

Output files created by this program always have exactly three chunks (a header
chunk, a format chunk, and a data chunk).  Files created by other software in
.wav format may include additional chunks.  In .wav files, binary data are
always written in little-endian format (least significant byte first).  The
format of mit2wav's output files is as follows:

[Header chunk]
Bytes  0 -  3: "RIFF" [4 ASCII characters]
Bytes  4 -  7: L-8 (number of bytes to follow in the file, excluding bytes 0-7)
Bytes  8 - 11: "WAVE" [4 ASCII characters]

[Format chunk]
Bytes 12 - 15: "fmt " [4 ASCII characters, note trailing space]
Bytes 16 - 19: 16 (format chunk length in bytes, excluding bytes 12-19)
Bytes 20 - 35: format specification, consisting of:
 Bytes 20 - 21: 1 (format tag, indicating no compression is used)
 Bytes 22 - 23: number of signals (1 - 65535)
 Bytes 24 - 27: sampling frequency in Hz (per signal)
                Note that the sampling frequency in a .wav file must be an
	        integer multiple of 1 Hz, a restriction that is not imposed
	        by MIT (WFDB) format.
 Bytes 28 - 31: bytes per second (sampling frequency * frame size in bytes)
 Bytes 32 - 33: frame size in bytes
 Bytes 34 - 35: bits per sample (ADC resolution in bits)
		Note that the actual ADC resolution (e.g., 12) is written in
		this field, although each output sample is right-padded to fill
		a full (16-bit) word.  (.wav format allows for 8, 16, 24, and
		32 bits per sample;  all mit2wav output is 16 bits per sample.)

[Data chunk]
Bytes 36 - 39: "data" [4 ASCII characters]
Bytes 40 - 43: L-44 (number of bytes to follow in the data chunk)
Bytes 44 - L-1: sample data, consisting of:
 Bytes 44 - 45: sample 0, channel 0
 Bytes 46 - 47: sample 0, channel 1 
 ... etc. (same order as in a multiplexed WFDB signal file)

*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#ifdef NOMKSTEMP
#define mkstemp mktemp
#endif

char *pname;	/* name of this program, for use in error messages */
FILE *ofile;	/* output (.wav) file pointer */

int out16(short);
int out32(long);
char *prog_name(char *);
void help(void);

main(int argc, char **argv)
{
    char buf[80], *nrec = NULL, *ofname, *record = NULL, tfname[10];
    double sps;
    FILE *sfile;
    int bitspersample = 0, bytespersecond, framelen, i, mag, nsig, *offset,
	*shift;
    long nsamp;
    static WFDB_Sample *x, *y;
    static WFDB_Siginfo *s;

    /* Interpret the command line. */
    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'h':	/* help requested */
	    help();
	    exit(1);
	    break;
	  case 'n':	/* new record name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: new record name must follow -n\n", pname);
		exit(1);
	    }
	    nrec = argv[i];
	    break;
	  case 'o':	/* output file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		      "%s: name of wav-format output file must follow -o\n",
			      pname);
		exit(1);
	    }
	    ofname = argv[i];
	    if (strlen(ofname)<5 || strcmp(".wav", ofname+strlen(ofname)-4)) {
		(void)fprintf(stderr,
			      "%s: name of output file must end in '.wav'\n",
			      pname);
		exit(1);
	    }
	    break;
	  case 'r':
	    if (++i >= argc) {
		(void)fprintf(stderr,
			      "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    record = argv[i];
	    break;
	  default:
	    (void)fprintf(stderr, "%s: unrecognized option %s\n", pname,
			  argv[i]);
	    exit(1);
	}
	else {
	    (void)fprintf(stderr, "%s: unrecognized argument %s\n", pname,
			  argv[i]);
	    exit(1);
	}
    }

    /* Check that required arguments are present and valid. */
    if (ofname == NULL || record == NULL) {
	help();
	exit(1);
    }
    if (nrec && strcmp(record, nrec) == 0) {
	(void)fprintf(stderr,
	      "%s: names of input and output records must not be identical\n",
		      pname);
	exit(1);
    }

    /* Open the input record and allocate memory for per-signal objects. */
    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL ||
	(offset = malloc(nsig * sizeof(int))) == NULL ||
	(shift = malloc(nsig * sizeof(int))) == NULL ||
	(x = malloc(nsig * sizeof(WFDB_Sample))) == NULL ||
	(y = malloc(nsig * sizeof(WFDB_Sample))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, s, nsig)) <= 0)
	exit(3);

    /* Check that the new record name is legal before proceeding.  Note that
       this step creates a header file, which is overwritten after the
       processing is completed. */
    if (nrec != NULL && newheader(nrec) < 0) exit(3);

    /* Set the output signal parameters. */
    for (i = 0; i < nsig; i++) {
	if (s[i].adcres > bitspersample) bitspersample = s[i].adcres;
	offset[i] = s[i].adczero;
	s[i].adczero = 0;
	shift[i] = 16 - s[i].adcres;
	s[i].adcres = 16;
	mag = 1 << shift[i];
	s[i].gain *= mag;
	s[i].baseline -= offset[i];
	s[i].baseline *= mag;
	s[i].fname = ofname;
	s[i].group = 0;
	s[i].fmt = 16;
    }
    
    /* Get information needed for the header and format chunks. */
    nsamp = strtim("e");
    sps = strtim("1");
    framelen = nsig * 2;
    bytespersecond = sps * framelen;

    /* Give up if the output cannot be written. */
    if (osigfopen(s, (unsigned)nsig) < nsig) exit(4);

    for (i = 0; i < nsig; i++)
	wfdbsetstart(i, 44L);

    /* Create a temporary file for use below. */
    (void)strcpy(tfname, "wavXXXXXX");
    (void)mkstemp(tfname);
    if ((ofile = fopen(tfname, "wb")) == NULL) {
	(void)fprintf(stderr, "%s: can't create temporary file %s\n",
		      pname, tfname);
	exit(1);
    }
    
    /* Write the header and format chunks, and the first 8 bytes of the
       data chunk, to the temporary file. */
    if (fwrite("RIFF", 1, 4, ofile) != 4) {
	fprintf(stderr, "%s: can't write to %s\n", pname, ofname);
	exit(2);
    }
    out32(nsamp*framelen + 36);  /* nsamp*framelen sample bytes, and 36 more
				    bytes of miscellaneous embedded header */
    fwrite("WAVEfmt ", 1, 8, ofile);
    out32(16);	/* number of bytes to follow in format chunk */
    out16(1);	/* format tag */
    out16(nsig);
    out32(sps);
    out32(sps*framelen);
    out16(framelen);
    out16(bitspersample);
    fwrite("data", 1, 4, ofile);
    out32(nsamp*framelen);
    fclose(ofile);

    /* Copy the input to the output.  Reformatting is handled by putvec(). */
    while (getvec(x) == nsig) {
	for (i = 0; i < nsig; i++)
	    y[i] = (x[i] - offset[i]) << shift[i];
	if (putvec(y) != nsig)
	    break;
    }

    /* Write the new header file if requested. */
    if (nrec) (void)newheader(nrec);

    /* Clean up. */
    wfdbquit();

    /* Prepend the temporary file to the new signal file. */
    sprintf(buf, "mv %s %s.1", ofname, ofname);
    system(buf);
    sprintf(buf, "cat %s %s.1 >%s", tfname, ofname, ofname);
    system(buf);
    sprintf(buf, "rm -f %s %s.1", tfname, ofname);
    system(buf);

    exit(0);	/*NOTREACHED*/
}

int out16(short x)
{
    if (putc(x & 0xff, ofile) == EOF ||
	putc((x >> 8) & 0xff, ofile) == EOF)
	return (EOF);
    return (2);
}

int out32(long x)
{
    if (putc(x & 0xff, ofile) == EOF ||
	putc((x >> 8) & 0xff, ofile) == EOF ||
	putc((x >> 16) & 0xff, ofile) == EOF ||
	putc((x >> 24) & 0xff, ofile) == EOF)
	return (EOF);
    return (4);
}

char *prog_name(char *s)
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
    "usage: %s -o FILE.wav -r RECORD [ OPTIONS ... ]\n",
    "where FILE.wav is the name of the wav-format output signal file,",
    "RECORD is the record name, and OPTIONS may include:",
    " -h         print this usage summary",
    " -n NEWREC  create a header file for the output signal file, so it",
    "             may be read as record NEWREC",
    NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
