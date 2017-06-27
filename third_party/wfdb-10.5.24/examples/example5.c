#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    WFDB_Siginfo *s;
    int i, nsig;

    if (argc < 2) {
        fprintf(stderr, "usage: %s record\n", argv[0]);
        exit(1);
    }
    nsig = isigopen(argv[1], NULL, 0);
    if (nsig < 1) exit(2);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    if (s == NULL) {
	fprintf(stderr, "insufficient memory\n");
	exit(3);
    }
    if (isigopen(argv[1], s, nsig) != nsig) exit(2);
    printf("Record %s\n", argv[1]);
    printf("Starting time: %s\n", timstr(0L));
    printf("Sampling frequency: %g Hz\n", sampfreq(argv[1]));
    printf("%d signals\n", nsig);
    for (i = 0; i < nsig; i++) {
        printf("Group %d, Signal %d:\n", s[i].group, i);
        printf(" File: %s\n", s[i].fname);
        printf(" Description: %s\n", s[i].desc);
        printf(" Gain: ");
        if (s[i].gain == 0.)
            printf("uncalibrated; assume %g", WFDB_DEFGAIN);
        else printf("%g", s[i].gain);
        printf(" adu/%s\n", s[i].units ? s[i].units : "mV");
        printf(" Initial value: %d\n", s[i].initval);
        printf(" Storage format: %d\n", s[i].fmt);
        printf(" I/O: ");
        if (s[i].bsize == 0) printf("can be unbuffered\n");
        else printf("%d-byte blocks\n", s[i].bsize);
        printf(" ADC resolution: %d bits\n", s[i].adcres);
        printf(" ADC zero: %d\n", s[i].adczero);
        if (s[i].nsamp > 0L) {
            printf(" Length: %s (%ld sample intervals)\n",
                   timstr(s[i].nsamp), s[i].nsamp);
            printf(" Checksum: %d\n", s[i].cksum);
        }
        else printf(" Length undefined\n");
    }
    exit(0);
}
