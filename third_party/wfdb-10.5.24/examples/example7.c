#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    double *c, one = 1.0, vv, atof();
    int i, j, nc = argc - 4, nsig;
    WFDB_Time nsamp, t, t0, t1;
    static WFDB_Sample *v;
    static WFDB_Siginfo *s;

    if (argc < 4) {
        fprintf(stderr,
          "usage: %s record start duration [ coefficients ... ]\n",
                argv[0]);
        exit(1);
    }
    if (nc < 1) {
        nc = 1; c = &one;
    }
    else if ((c = (double *)calloc(nc, sizeof(double))) == NULL) {
        fprintf(stderr, "%s: too many coefficients\n", argv[0]);
        exit(2);
    }
    for (i = 0; i < nc; i++)
        c[i] = atof(argv[i+4]);
    if ((nsig = isigopen(argv[1], NULL, 0)) < 1)
        exit(3);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    if (s == NULL || v == NULL) {
	fprintf(stderr, "insufficient memory\n");
	exit(4);
    }
    if (isigopen(argv[1], s, nsig) != nsig)
        exit(5);
    t0 = strtim(argv[2]);
    if (t0 < (WFDB_Time)0) t0 = -t0;
    (void)sample(0, t0);
    if (!sample_valid()) {
	fprintf(stderr, "%s: inappropriate value for start time\n",
		argv[0]);
	exit(6);
    }
    if ((nsamp = strtim(argv[3])) < 1) {
        fprintf(stderr, "%s: inappropriate value for duration\n",
                argv[0]);
        exit(7);
    }
    t1 = t0 + nsamp;
    if (osigopen("16l", s, nsig) != nsig)
        exit(8);

    for (t = t0; t < t1 && sample_valid(); t++) {
        for (j = 0; j < nsig; j++) {
            for (i = 0, vv = 0.; i < nc; i++)
                if (c[i] != 0.) vv += c[i]*sample(j, t+i);
            v[j] = (WFDB_Sample)vv;
        }
        if (putvec(v) < 0) break;
    }

    (void)newheader("out");
    wfdbquit();
    exit(0);
}
