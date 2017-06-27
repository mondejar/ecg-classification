#include <stdio.h>
#include <wfdb/wfdb.h>

main()
{
    char answer[32], record[8], directory[32];
    int i, nsig = 0;
    long nsamp, t;
    double freq = 0.;
    char **filename, **description, **units;
    WFDB_Sample *v;
    WFDB_Siginfo *s;

    do {
        printf("Choose a record name [up to 6 characters]: ");
        fgets(record, 8, stdin); record[strlen(record)-1] = '\0';
    } while (newheader(record) < 0);
    do {
        printf("Number of signals to be recorded [>0]: ");
        fgets(answer, 32, stdin); sscanf(answer, "%d", &nsig);
    } while (nsig < 1);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    filename = (char **)malloc(nsig * sizeof(char *));
    description = (char **)malloc(nsig * sizeof(char *));
    units = (char **)malloc(nsig * sizeof(char *));
    if (s == NULL || v == NULL || filename == NULL ||
	description == NULL || units == NULL) {
	fprintf(stderr, "insufficient memory\n");
	exit(1);
    }
    for (i = 0; i < nsig; i++) {
	if ((filename[i] = (char *)malloc(32)) == NULL ||
	    (description[i] = (char *)malloc(32)) == NULL ||
	    (units[i] = (char *)malloc(32)) == NULL) {
	    fprintf(stderr, "insufficient memory\n");
	    exit(1);
	}
    }
    do {
        printf("Sampling frequency [Hz per signal, > 0]: ");
        fgets(answer, 32, stdin); sscanf(answer, "%lf", &freq);
    } while (setsampfreq(freq) < 0);
    do {
        printf("Length of record (H:M:S): ");
        fgets(answer, 32, stdin);
    } while ((nsamp = strtim(answer)) < 1L);
    printf("Directory for signal files [up to 30 characters]: ");
    fgets(directory, 32, stdin);
    directory[strlen(directory)-1] = '\0';
    printf("Save signals in difference format? [y/n]: ");
    fgets(answer, 32, stdin);
    s[0].fmt = (answer[0] == 'y') ? 8 : 16;
    printf("Save all signals in one file? [y/n]: ");
    fgets(answer, 32, stdin);
    if (answer[0] == 'y') {
        sprintf(filename[0], "%s/d.%s", directory, record);
        for (i = 0; i < nsig; i++) {
             s[i].fname = filename[0];
             s[i].group = 0;
        }
    }
    else {
        for (i = 0; i < nsig; i++) {
             sprintf(filename[i], "%s/d%d.%s", directory,i,record);
             s[i].fname = filename[i];
             s[i].group = i;
        }
    }
    for (i = 0; i < nsig; i++) {
        s[i].fmt = s[0].fmt; s[i].bsize = 0;
        printf("Signal %d description [up to 30 characters]: ", i);
        fgets(description[i], 32, stdin);
        description[i][strlen(description[i])-1] = '\0';
        s[i].desc = description[i];
        printf("Signal %d units [up to 20 characters]: ", i);
        fgets(units[i], 22, stdin);
        units[i][strlen(units[i])-1] = '\0';
        s[i].units = (*units[i]) ? units[i] : "mV";
        do {
            printf(" Signal %d gain [adu/%s]: ", i, s[i].units);
            fgets(answer, 32, stdin);
            sscanf(answer, "%lf", &s[i].gain);
        } while (s[i].gain < 0.);
        do {
            printf(" Signal %d ADC resolution in bits [8-16]: ", i);
            fgets(answer, 32, stdin);
            sscanf(answer, "%d", &s[i].adcres);
        } while (s[i].adcres < 8 || s[i].adcres > 16);
        printf(" Signal %d ADC zero level [adu]: ", i);
        fgets(answer, 32, stdin);
        sscanf(answer, "%d", &s[i].adczero);
    }
    if (osigfopen(s, nsig) < nsig) exit(1);
    printf("To begin sampling, press RETURN;  to specify a\n");
    printf(" start time other than the current time, enter\n");
    printf(" it in H:M:S format before pressing RETURN: ");
    fgets(answer, 32, stdin); answer[strlen(answer)-1] = '\0';
    setbasetime(answer);

    adinit();

    for (t = 0; t < nsamp; t++) {
        for (i = 0; i < nsig; i++)
            v[i] = adget(i);
        if (putvec(v) < 0) break;
    }

    adquit();

    (void)newheader(record);
    wfdbquit();
    exit(0);
}

adinit() { printf("%s\n", timstr(0L)); }

adget(i)
int i;
{
    return (i);
}

adquit() { ; }
