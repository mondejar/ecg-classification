#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    WFDB_Siginfo *s;
    int i, nsig, nsamp=1000;
    WFDB_Sample *vin, *vout;

    if (argc < 2) {
        fprintf(stderr, "usage: %s record\n", argv[0]); exit(1);
    }
    if ((nsig = isigopen(argv[1], NULL, 0)) <= 0) exit(2);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    vin = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    vout = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    if (s == NULL || vin == NULL || vout == NULL) {
	fprintf(stderr, "insufficient memory\n");
	exit(3);
    }
    if (isigopen(argv[1], s, nsig) != nsig) exit(2);
    if (osigopen("8l", s, nsig) <= 0) exit(3);
    while (nsamp-- > 0 && getvec(vin) > 0) {
        for (i = 0; i < nsig; i++)
            vout[i] -= vin[i];
        if (putvec(vout) < 0) break;
        for (i = 0; i < nsig; i++)
            vout[i] = vin[i];
    }
    (void)newheader("dif");
    wfdbquit();
    exit(0);
}
