/* file: wav2mit.c	G. Moody	12 February 2003
			Last revised:	23 December 2004
-------------------------------------------------------------------------------
wav2mit: Convert a .wav format file to WFDB format
Copyright (C) 2003-2004 George B. Moody

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

Most .wav files are already written in a WFDB-compatible format.  This program
simply creates a .hea (WFDB header) file that describes the .wav file;  other
WFDB applications can then read the .wav file directly.

This process may not work with some .wav files that are encoded using variants
of the original .wav format that are not WFDB-compatible.  In principle, this
program should be able to recognize such files by their format codes, and it
will produce an error message in such cases.  If the format code is incorrect,
however, 'wav2mit' may not recognize that an error has occurred.

To perform the reverse conversion, use 'mit2wav'.

Files that can be processed successfully using 'wav2mit' always have exactly
three chunks (a header chunk, a format chunk, and a data chunk).  In .wav
files, binary data are always written in little-endian format (least
significant byte first).  The format of wav2mit's input files is as follows:

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

char *pname;	/* name of this program, for use in error messages */
int errflag=0;	/* non-zero if EOF reached in input file */
FILE *ifile;	/* input (.wav) file pointer */

short in16(void);
long in32(void);
char *prog_name(char *);
void help(void);

main(int argc, char **argv)
{
    char buf[80], *ifname, *record = NULL;
    double sps, wgain;
    int bitspersample, bytespersecond, framelen, i, nsig, tag, wres;
    long flen, len, wnsamp;
    static WFDB_Siginfo *s;

    /* Interpret the command line. */
    pname = prog_name(argv[0]);
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (*(argv[i]+1)) {
	  case 'h':	/* help requested */
	    help();
	    exit(1);
	    break;
	  case 'i':	/* input file name follows */
	    if (++i >= argc) {
		(void)fprintf(stderr,
		      "%s: name of wav-format input file must follow -i\n",
			      pname);
		exit(1);
	    }
	    ifname = argv[i];
	    if (strlen(ifname)<5 || strcmp(".wav", ifname+strlen(ifname)-4)) {
		(void)fprintf(stderr,
			      "%s: name of input file must end in '.wav'\n",
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
    if (ifname == NULL) {
	help();
	exit(1);
    }

    /* If the record name was not specified, generate it from ifname. */
    if (record == NULL) {
	record = malloc(strlen(ifname));
	strncpy(record, ifname, strlen(ifname)-4);
	record[strlen(ifname)-3] = '\0';
    }

    /* Open the input file. */
    if ((ifile = fopen(ifname, "rb")) == NULL) {
	fprintf(stderr, "%s: can't open %s\n", pname, ifname);
	exit(2);
    }
 
    /* Determine the size of the input file. */
    fseek(ifile, 0L, SEEK_END);
    flen = ftell(ifile);
    fseek(ifile, 0L, SEEK_SET);
   
    /* Read and check the header chunk. */
    if (fread(buf, 1, 4, ifile) != 4 ||
	strncmp(buf, "RIFF", 4)) {
	fprintf(stderr, "%s: %s is not a .wav-format file\n", pname, ifname);
	exit(3);
    }
    len = in32() + 8;
    if (len != flen) {
	fprintf(stderr,
	  "%s: header chunk of %s has incorrect length (%ld, should be %ld)\n",
		pname, ifname, len, flen);
	exit(3);
    }
    if (fread(buf, 1, 4, ifile) != 4 ||
	strncmp(buf, "WAVE", 4)) {
	fprintf(stderr, "%s: %s is not a .wav-format file\n", pname, ifname);
	exit(3);
    }

    /* Read and check the format chunk. */
    if (fread(buf, 1, 4, ifile) != 4 ||
	strncmp(buf, "fmt ", 4)) {
	fprintf(stderr, "%s: format chunk missing or corrupt in %s\n",
		pname, ifname);
	exit(3);
    }
    len = in32();
    tag = in16();
    if (len != 16 || tag != 1) {
	fprintf(stderr, "%s: unsupported format %d in %s\n", 
		pname, tag, ifname);
	exit(3);
    }
    nsig = in16();
    sps = in32();
    bytespersecond = in32();
    framelen = in16();
    bitspersample = in16();
    if (bitspersample <= 8) {
	wres = 8;
	wgain = 12.5;
    }
    else if (bitspersample <= 16) {
	wres = 16;
	wgain = 6400;
    }
    else {
	fprintf(stderr, "%s: unsupported resolution (%d bits/sample) in %s\n",
		pname, bitspersample, ifname);
	exit(3);
    }
    if (framelen != nsig * wres / 8) {
	fprintf(stderr, "%s: format chunk of %s has incorrect frame length\n",
		pname, ifname);
	exit(3);
    }

    /* Read and check the beginning of the data chunk. */
    if (fread(buf, 1, 4, ifile) != 4 ||
	strncmp(buf, "data", 4)) {
	fprintf(stderr, "%s: data chunk missing or corrupt in %s\n",
		pname, ifname);
	exit(3);
    }
    len = in32();
    wnsamp = len / framelen;

    if ((s = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }

    for (i = 0; i < nsig; i++) {
	s[i].fname = ifname;
	s[i].desc = NULL;
	s[i].units = NULL;
	s[i].gain = wgain;
	s[i].initval = 0;
	s[i].group = 0;
	s[i].fmt = (wres == 16) ? 16 : 80;
	s[i].spf = 1;
	s[i].bsize = 0;
	s[i].adcres = wres;
	s[i].adczero = (wres == 16) ? 0 : 128;
	s[i].baseline = s[i].adczero;
	s[i].nsamp = wnsamp;
	s[i].cksum = 0;
	wfdbsetstart(i, 44L);
    }
    setsampfreq(sps);

    if (setheader(record, s, nsig))
	exit(4);

    /* Clean up. */
    wfdbquit();

    exit(0);
}

short in16()
{
    short a, b, r;

    if ((a = getc(ifile)) == EOF || (b = getc(ifile)) == EOF) {
	errflag = EOF;
	return (EOF);
    }
    r = (a & 0xff) | ((b << 8) & 0xff00);
    return (r);
}

long in32()
{
    long a, b, c, d, r;

    if ((a = getc(ifile)) == EOF || (b = getc(ifile)) == EOF ||
	(c = getc(ifile)) == EOF || (d = getc(ifile)) == EOF) {
	errflag = EOF;
	return (EOF);
    }
    r = (a & 0xff) | ((b << 8) & 0xff00) |
	((c << 16) & 0xff0000) | ((d << 24) & 0xff000000);
    return (r);
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
    "usage: %s -i FILE.wav -r RECORD [ OPTIONS ... ]\n",
    "where FILE.wav is the name of the wav-format input signal file,",
    "and OPTIONS may include:",
    " -h         print this usage summary",
    " -r RECORD  create RECORD.hea (default: FILE.hea)",
    NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
