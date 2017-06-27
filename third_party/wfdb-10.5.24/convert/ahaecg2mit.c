/* file: ahaecg2mit.c		G. Moody		7 May 2008
				Last revised:	       11 July 2012
Convert *.ecg or *.txt files from an AHA Database DVD to WFDB-compatible format
*/

#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

int format;
FILE *ifile;
WFDB_Annotation a;
WFDB_Sample v[2];

void process(char *r);
int readbindata(void)
{
    char data[5];

    if (fread(data, 1, 5, ifile) != 5)
	return (0);	/* EOF */
    v[0] = (data[0] & 0xff) | ((data[1] << 8) & 0xff00);
    v[1] = (data[2] & 0xff) | ((data[3] << 8) & 0xff00);
    if (data[4] == '.')
	return (1);	/* samples, no annotation */
    a.anntyp = ammap(data[4]);
    a.subtyp = (data[4] == 'U' ? -1 : 0);
    return (2);		/* samples and annotation */
}

int readtxtdata(void)
{
    char buf[16];
    int c, i, n;

    for (i = 0; i < sizeof(buf); i++) {
	c = getc(ifile);
	if (c == '\r' || c == EOF || c == '\n') break;
	buf[i] = c;
    }
    if (c == EOF) return (0);
    n = sscanf(buf, "%d,%d,\"%c", v, v+1, &c);
    if (n < 3) {
	fprintf(stderr, "Error: improperly formatted input!\n");
	fprintf(stderr,
		" n = %d, buf = |%s|, v[0] = %d, v[1] = %d, c = |%c|\n",
		n, buf, v[0], v[1], c);
	return (0);
    }
    if (c == '"')
	return (1);	/* samples, no annotation */
    a.anntyp = ammap(c);
    a.subtyp = (c == 'U' ? -1 : 0);
    return (2);		/* samples and annotation */
}
	
main(int argc, char **argv)
{
    char *p, *record;
    int i, sflag = 0;

    if (argc < 2 || strcmp(argv[1], "-h") == 0) {
	fprintf(stderr, "usage: %s [-s] RECORD.XXX [RECORD.XXX]...\n",
		argv[0]);
	fprintf(stderr, 
		" where RECORD.XXX is the name of the AHA DVD input file to"
		" be converted\n"
		" ('RECORD' is one of the 4-digit AHA DB record names, and"
		" 'XXX' may be either\n 'ecg' or 'txt', depending on the"
		" version of the AHA DVD.)\n"
		" Output files are RECORD.hea (text), RECORD.atr (binary),"
		" RECORD.dat (binary).\n"
		" Two or more input files may be converted in a single run.\n"
		" Use -s to make short-format (35-minute) records from"
		" long-format (2.5 hour)\n input files.  When using -s, the"
		" second digit in the output record name is\n"
		" incremented by 2.\n");
	exit(1);
    }
    if (strcmp(argv[i = 1], "-s") == 0) {
	sflag = 1;	/* produce short-format (35-minute) records */
	i = 2;
    }
    for ( ; i < argc; i++) {
	p = argv[i] + strlen(argv[i]) - 4;    /* pointer to '.ecg' or '.txt' */
	if (strcmp(".ecg", p) == 0 || strcmp(".ECG", p) == 0)
	    format = 1;
	else if (strcmp(".txt", p) == 0 || strcmp(".TXT", p) == 0)
	    format = 2;
	else {
	    fprintf(stderr, "%s: ignoring '%s'\n", argv[0], argv[i]);
	    continue;	/* not an .ecg or .txt file */
	}
	if ((ifile = fopen(argv[i], "rb")) == NULL) {
	    fprintf(stderr, "%s: can't open '%s'\n", argv[0], argv[1]);
	    continue;
	}
	*p = '\0';	/* strip off extension */
	record = p - 4;	/* AHA record names are 4 (ASCII) digits */
	if (sflag) {/* skip first 145 minutes if making a short-format record */
	    if (format == 1)
		fseek(ifile, 145L*60L*250L*5L, SEEK_SET);
	    else if (format == 2) {
		long t;

		for (t = 0; t < 145L*60L*250L; t++)
		    (void)readtxtdata();
	    }		
	    record[1] += 2;	/* fix record name (n0nn->n2nn, n1nn->n3nn) */
	}
	process(record);
	fclose(ifile);
    }
    exit(0);
}

void process(char *record)
{
    char ofname[10];
    int stat;
    WFDB_Time t = -1;
    static WFDB_Siginfo si[2];
    static WFDB_Anninfo ai;

    setsampfreq(250.0);	/* AHA DB is sampled at 250 Hz for each signal */
    sprintf(ofname, "%s.dat", record);
    si[0].fname = ofname;
    si[0].desc = "ECG0";
    si[0].units = "mV";
    si[0].gain = 400;
    si[0].fmt = 212;
    si[0].adcres = 12;
    si[1] = si[0];
    si[1].desc = "ECG1";
    ai.name = "atr";
    ai.stat = WFDB_WRITE;
    if (osigfopen(si, 2) != 2 || annopen(record, &ai, 1) < 0) {
	wfdbquit();
	return;
    }
    if (format == 1) {
	while (stat = readbindata()) {
	    (void)putvec(v);
	    if (stat > 1) {
		a.time = t;
		(void)putann(0, &a);
	    }
	    t++;
	}
    }
    else {
	while (stat = readtxtdata()) {
	    t++;
	    (void)putvec(v);
	    if (stat > 1) {
		a.time = t;
		(void)putann(0, &a);
	    }
	}
    }
    (void)newheader(record);
    wfdbquit();
    fprintf(stderr, "wrote %s.atr, %s.dat, and %s.hea\n", record,record,record);
    return;
}
