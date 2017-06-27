/* file: mit2edf.c		G. Moody	2 November 2002
				Last revised:    12 March 2014
-------------------------------------------------------------------------------
Convert MIT format header and signal files to EDF (European Data Format) file
Copyright (C) 2002-2014 George B. Moody

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

#define EDFMAXBLOCK	61440	/* maximum data block length, in bytes */

char *pname;

main(argc, argv)
int argc;
char **argv;
{
    char buf[100];
    char *header, *ofname = NULL, *p, *block, **blockp, *record = NULL;
    double *pmax, *pmin, frames_per_second, seconds_per_block;
    FILE *ofile = NULL;
    int *dmax, *dmin, i, j, k, nsig, samples_per_frame, start_date_recorded,
	edfplusflag = 0, vflag = 0, day, month, year, hour, minute, second;
    long bytes_per_block, frames_per_block, n, nblocks, blocks_per_minute,
	blocks_per_hour;
    WFDB_Sample *v, *vp;
    WFDB_Siginfo *si;
    static char *month_name[] = {  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
				   "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    void help();

    /* Interpret the command line. */
    pname = argv[0];
    for (i = 1; i < argc; i++) {
	if (*argv[i] == '-') switch (argv[i][1]) {
	  case 'h':	/* show usage and quit */
	    help();
	    exit(0);
	  case 'o':	/* output file name follows */
	    if (++i < argc)
		ofname = argv[i];
	    else {
		fprintf(stderr, "%s: output file name must follow -o\n",pname);
		exit(1);
	    }
	    break;
	  case 'p':	/* format output as EDF+ */
	    edfplusflag = 1;
	    break;
	  case 'r':	/* record name follows */
	    if (++i < argc)
		record = argv[i];
	    else {
		fprintf(stderr, "%s: record name must follow -r\n", pname);
		exit(1);
	    }
	    break;
	  case 'v':	/* select verbose mode */
	    vflag = 1;
	    break;
	}
    }

    /* Quit if no record was specified. */
    if (record == NULL) {
	help();
	exit(1);
    }

    /* Construct or validate the name of the output file. */
    if (ofname == NULL) {
	for (p = record + strlen(record); p > record; p--)
	    if (*(p-1) == '/') break;
	strncpy(buf, p, WFDB_MAXRNL);
	strcat(buf, ".edf");
	ofname = buf;
    }
    else if (edfplusflag) {
	p = ofname + strlen(ofname) - 4;
	if (strcmp(p, ".edf") && strcmp(p, ".EDF")) {
	    (void)fprintf(stderr, "%s: '%s' is not a valid EDF+ file name\n",
			  pname, ofname);
	    (void)fprintf(stderr,
		         " EDF+ file names must end with '.edf' or '.EDF')\n");

	    exit(1);
	}
    }

    /* Open the input record. */
    if ((nsig = isigopen(record, NULL, 0)) <= 0) exit(2);
    if ((si = malloc(nsig * sizeof(WFDB_Siginfo))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    if ((nsig = isigopen(record, si, nsig)) <= 0)
	exit(3);

    /* Open the output (EDF) file. */
    if ((ofile = fopen(ofname, "wb")) == NULL) {
	(void)fprintf(stderr, "%s: can't create %s\n", pname, ofname);
	exit(4);
    }

    /* Get start date and time */
    p = timstr(0L);
    start_date_recorded = 1;
    if (*p != '[') {	/* start date/time not recorded -- use current date */
	setbasetime(NULL);
	p = timstr(0L);
	start_date_recorded = 0;
    }
    i = sscanf(p, "[%d:%d:%d %d/%d/%d",
	       &hour, &minute, &second, &day, &month, &year);
    if (i != 6) {		/* start time, but not date, recorded */
	start_date_recorded = 0;
	day = month = 1;
	year = 1985;		/* beginning of EDF epoch */
    }

    /* Calculate block duration.  (In the EDF spec, blocks are called "records"
       or "data records", but this would be confusing here since "record"
       refers to the entire recording -- so here we say "blocks".) */
    for (i = samples_per_frame = 0; i < nsig; i++)
	samples_per_frame += si[i].spf;
    frames_per_second = strtim("1:0")/60.0;	/* i.e., the number of frames
						   per minute, divided by 60 */
    frames_per_block = 10 * frames_per_second + 0.5;	/* ten seconds */
    bytes_per_block = 2 * samples_per_frame * frames_per_block;
    				   /* EDF specifies 2 bytes per sample */
    while (bytes_per_block > EDFMAXBLOCK) {
	/* blocks would be too long -- reduce their length by a factor of 10 */
	frames_per_block /= 10;
	bytes_per_block = samples_per_frame * 2 * frames_per_block;
    }

    seconds_per_block = frames_per_block / frames_per_second;

    if (frames_per_block < 1 && bytes_per_block < EDFMAXBLOCK/60) {
	frames_per_block = strtim("1:0");     /* the number of frames/minute */
	bytes_per_block = 2* samples_per_frame * frames_per_block;
	seconds_per_block = 60;
    }

    if (bytes_per_block > EDFMAXBLOCK) {
	fprintf(stderr, "%s: can't convert record %s to EDF\n", pname, record);
	fprintf(stderr,
 " EDF blocks cannot be larger than %d bytes, but each input frame requires\n",
		EDFMAXBLOCK);
	fprintf(stderr,
 " %d bytes.  Use 'snip' to select a subset of the input signals, or use\n"
		" 'xform' to reduce the sampling frequency.\n",
		samples_per_frame * 2);
	exit(5);
    }

    /* Calculate the number of blocks to be written.  strtim("e") is the frame
       number of the last frame in the record, and that of the first frame is
       0, so the number of frames is strtim("e") + 1.  The calculation rounds
       up so that we don't lose any frames, even if the number of frames is not
       an exact multiple of frames_per_block. */
    nblocks = strtim("e") / frames_per_block + 1;

    /* Allocate and initialize arrays and buffers. */
    if ((dmax = malloc(nsig * sizeof(int))) == NULL ||
	(dmin = malloc(nsig * sizeof(int))) == NULL ||
	(pmax = malloc(nsig * sizeof(double))) == NULL ||
	(pmin = malloc(nsig * sizeof(double))) == NULL ||
	(header = malloc((nsig + 1) * 256)) == NULL ||
	(v = malloc(samples_per_frame * sizeof(WFDB_Sample))) == NULL ||
	(block = malloc(bytes_per_block)) == NULL ||
	(blockp = malloc(nsig * sizeof(char *))) == NULL) {
	(void)fprintf(stderr, "%s: insufficient memory\n", pname);
	exit(2);
    }
    for (i = 0; i < (nsig + 1)*256; i++)
	    header[i] = ' ';

    if (vflag)
	printf("Converting record %s to %s (%s mode)\n", record, ofname,
	       edfplusflag == 1 ? "EDF+" : "EDF");

    /* Calculate physical and digital extrema. */
    for (i = 0; i < nsig; i++) {
	if (si[i].adcres < 1) {	/* invalid ADC resolution in input .hea file */
	    switch (si[i].fmt) { /* guess ADC resolution based on format */
	      case 24: si[i].adcres = 24; break;
	      case 32: si[i].adcres = 32; break;
	      case 80: si[i].adcres = 8; break;
	      case 212: si[i].adcres = 12; break;
	      case 310:
	      case 311: si[i].adcres = 10; break;
	      default: si[i].adcres = 16; break;
	    }
	}
	dmax[i] = si[i].adczero + (1 << (si[i].adcres - 1)) - 1;
	dmin[i] = si[i].adczero - (1 << (si[i].adcres - 1));
	pmax[i] = aduphys(i, dmax[i]);
	pmin[i] = aduphys(i, dmin[i]);
    }

    /* Start filling in the header.  The first line of comments above each
       entry is as given in the EDF technical specification
       (http://www.hsr.nl/edf/edf_spec.htm). */
    p = header;

    /* Version of this data format (0). */
    strncpy(p, "0", 1);
    p += 8;

    /* Local patient identification. */
    if (strlen(record) > 80) record[79] = '\0';
    strncpy(p, record, strlen(record));
    p += 80;

    /* Local recording identification.

       Bob Kemp recommends using this field to encode the start date including
       an abbreviated month name in English and a full (4-digit) year, as is
       done here if this information is available in the input record.  EDF+
       requires this. */

    if (start_date_recorded)
	sprintf(buf, "Startdate %02d-%s-%d", day, month_name[month-1], year);
    else {
	sprintf(buf, "Startdate not recorded");
	if (edfplusflag)
	    fprintf(stderr,
	            "WARNING (%s): EDF+ requires start date (not specified)\n",
		    pname);
    }
    strncpy(p, buf, strlen(buf));
    p += 80;

    /* Start date of recording (dd.mm.yy). */
    sprintf(buf, "%02d.%02d.%02d", day, month, year % 100);
    strncpy(p, buf, 8);
    p += 8;

    /* Start time of recording (hh.mm.ss). */
    sprintf(buf, "%02d.%02d.%02d", hour, minute, second);
    strncpy(p, buf, 8);
    p += 8;

    /* Number of bytes in header. */
    sprintf(buf, "%ld", (nsig + 1)*256L);
    strncpy(p, buf, strlen(buf));
    p += 8;

    /* Reserved. */
    if (edfplusflag)
	strncpy(p, "EDF+C", 5);
    p += 44;

    /* Number of blocks (-1 if unknown). */
    sprintf(buf, "%ld", nblocks);
    strncpy(p, buf, strlen(buf));
    p += 8;

    /* Duration of a block, in seconds. */
    sprintf(buf, "%g", seconds_per_block);
    if (strlen(buf) > 8) buf[8] = '\0';
    strncpy(p, buf, strlen(buf));
    p += 8;

    /* Number of signals. */
    sprintf(buf, "%d", nsig);
    strncpy(p, buf, strlen(buf));
    p += 4;

    /* Label (e.g., EEG FpzCz or Body temp). */
    for (i = 0; i < nsig; i++, p += 16) {
	if (strlen(si[i].desc) > 16) si[i].desc[16] = '\0';
	strncpy(p, si[i].desc, strlen(si[i].desc));
    }

    /* Transducer type (e.g., AgAgCl electrode). */
    for (i = 0; i < nsig; i++, p += 80) {
	strncpy(p, "transducer type not recorded",
		strlen("transducer type not recorded"));
    }

    /* Physical dimension (e.g., uV or degreeC). */
    for (i = 0; i < nsig; i++, p += 8) {
	if (si[i].units == NULL) si[i].units = "mV";
	else if (strlen(si[i].units) > 8) si[i].units[8] = '\0';
	strncpy(p, si[i].units, strlen(si[i].units));
    }

    /* Physical minimum (e.g., -500 or 34). */
    for (i = 0; i < nsig; i++, p += 8) {
	sprintf(buf, "%g", pmin[i]);
	strncpy(p, buf, strlen(buf));
    }

    /* Physical maximum (e.g., 500 or 40). */
    for (i = 0; i < nsig; i++, p += 8) {
	sprintf(buf, "%g", pmax[i]);
	strncpy(p, buf, strlen(buf));
    }

    /* Digital minimum (e.g., -2048). */
    for (i = 0; i < nsig; i++, p += 8) {
	sprintf(buf, "%d", dmin[i]);
	strncpy(p, buf, strlen(buf));
    }

    /* Digital maximum (e.g., 2047). */
    for (i = 0; i < nsig; i++, p += 8) {
	sprintf(buf, "%d", dmax[i]);
	strncpy(p, buf, strlen(buf));
    }

    /* Prefiltering (e.g., HP:0.1Hz LP:75Hz). */
    for (i = 0; i < nsig; i++, p += 80) {
	strncpy(p, "prefiltering not recorded",
		strlen("prefiltering not recorded"));
    }

    /* Number of samples per block. */
    for (i = 0; i < nsig; i++, p += 8) {
	sprintf(buf, "%ld", frames_per_block * si[i].spf);
	strncpy(p, buf, strlen(buf));
    }

    /* (The last 32*nsig bytes in the header are unused.) */

    /* Write the header to the output file. */
    fwrite(header, 1, (nsig+1) * 256, ofile);

    /* Check that all characters in the header are valid (printable ASCII
       between 32 and 126 inclusive).  Note that this test does not prevent
       generation of files containing invalid characters;  it merely warns
       the user if this has happened. */
    for (i = 0; i < (nsig+1) * 256; i++)
	if (header[i] < 32 || header[i] > 126)
	    fprintf(stderr,
		    "WARNING (%s): output contains an invalid character, %d,"
		    " at byte %d\n", pname, header[i], i);

    /* In verbose mode, summarize what we've done so far. */
    if (vflag) {
	printf(" Header block size: %d bytes\n", (nsig+1) * 256);
	printf(" Data block size: %g second%s (%ld frame%s or %ld bytes)\n",
	       seconds_per_block, seconds_per_block == 1.0 ? "" : "s",
	       frames_per_block, frames_per_block == 1 ? "" : "s",
	       bytes_per_block);
	for (p = timstr(strtim("e")); *p == ' '; p++)
		 ;
	printf(" Recording length: %s"
	       " (%ld data blocks, %ld frames, %ld bytes)\n",
	       p, nblocks, nblocks*frames_per_block, nblocks*bytes_per_block);
	printf(" Total length of file to be written: %ld bytes\n",
	       (nsig+1)*256 + nblocks*bytes_per_block);

	blocks_per_minute = (long)(60 / seconds_per_block);
	blocks_per_hour = (long)60 * blocks_per_minute;
    }

    /* Write the data blocks. */
    for (n = 1; n <= nblocks; n++) {
	blockp[0] = block;
	for (j = 1; j < nsig; j++)
	    blockp[j] = blockp[j-1] + 2 * frames_per_block * si[j-1].spf;
	for (i = 0; i < frames_per_block; i++) {
	    if (nsig != getframe(v)) {
		/* end of input: pad last block with invalid samples */
		for (j = 0; j < samples_per_frame; j++)
		    v[j] = WFDB_INVALID_SAMPLE;
	    }
	    vp = v;
	    for (j = 0; j < nsig; j++) {
		for (k = 0; k < si[j].spf; k++) {
		    *(blockp[j]++) = (*vp) & 0xff;
		    *(blockp[j]++) = ((*vp++ >> 8) & 0xff);
		}
	    }
	}
	fwrite(block, 1, bytes_per_block, ofile);
	if (vflag) {
	    if (n % blocks_per_minute == 0) { printf("."); fflush(stdout); }
	    if (n % blocks_per_hour == 0) printf("\n");
	}
    }
    (void)fclose(ofile);
    printf("\n");

    if (edfplusflag) {
	fprintf(stderr,
"\nWARNING (%s): EDF+ requires the subject's gender, birthdate, and name, as\n"
" well as additional information about the recording that is not usually\n"
" available.  This information is not saved in the output file even if\n"
" available.  EDF+ also requires the use of standard names for signals and\n"
" for physical units;  these requirements are not enforced by this program.\n"
" To make the output file fully EDF+ compliant, its header must be edited\n"
" manually.\n",
		pname);
	for (i = nsig-1; i >= 0; i--)
	    if (strcmp(si[i].desc, "EDF-Annotations") == 0)
		break;
	if (i < 0)
	    fprintf(stderr,
"\nWARNING:  The output file does not include EDF annotations, which are\n"
		    " required for EDF+.\n");
    }
    
    wfdbquit();
    exit(0);
}

static char *help_strings[] = {
 "usage: %s -r RECORD [OPTIONS ...]\n",
 "where RECORD is the name of the input record, and OPTIONS may include:",
 " -h          print this usage summary",
 " -o EDFILE   write the specified European Data Format file (default:",
 "              RECORD.edf)",
 " -p          write the output as an EDF+ file",
 " -v          select verbose mode",
 "This program reads MIT-format signal and header files and writes an EDF",
 "file containing the same data.",
NULL
};

void help()
{
    int i;

    (void)fprintf(stderr, help_strings[0], pname);
    for (i = 1; help_strings[i] != NULL; i++)
	(void)fprintf(stderr, "%s\n", help_strings[i]);
}
