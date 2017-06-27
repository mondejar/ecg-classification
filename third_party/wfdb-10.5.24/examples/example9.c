#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

main(argc, argv)
int argc;
char *argv[];
{
    int btype, i, j, nbeats = 0, nsig, hwindow, window;
    long stoptime = 0L, **sum;
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    WFDB_Sample *v, *vb;
    WFDB_Siginfo *s;
    void *calloc();

    if (argc < 3) {
        fprintf(stderr,
                "usage: %s annotator record [beat-type from to]\n",
                argv[0]);
        exit(1);
    }
    a.name = argv[1]; a.stat = WFDB_READ;
    if ((nsig = isigopen(argv[2], NULL, 0)) < 1) exit(2);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    vb = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    sum = (long **)malloc(nsig * sizeof(long *));
    if (s == NULL || v == NULL || vb == NULL || sum == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", argv[0]);
	exit(2);
    }
    if (wfdbinit(argv[2], &a, 1, s, nsig) != nsig) exit(3);
    hwindow = strtim(".05"); window = 2*hwindow + 1;
    for (i = 0; i < nsig; i++)
        if ((sum[i]=(long *)calloc(window,sizeof(long))) == NULL) {
            fprintf(stderr, "%s: insufficient memory\n", argv[0]);
            exit(2);
        }
    btype = (argc > 3) ? strann(argv[3]) : NORMAL;
    if (argc > 4) iannsettime(strtim(argv[4]));
    if (argc > 5) {
        if ((stoptime = strtim(argv[5])) < 0L)
            stoptime = -stoptime;
        if (s[0].nsamp > 0L && stoptime > s[0].nsamp)
            stoptime = s[0].nsamp;
    }
    else stoptime = s[0].nsamp;
    if (stoptime > 0L) stoptime -= hwindow;
    while (getann(0, &annot) == 0 && annot.time < hwindow)
        ;
    do {
        if (annot.anntyp != btype) continue;
        isigsettime(annot.time - hwindow - 1);
        getvec(vb);
        for (j = 0; j < window && getvec(v) > 0; j++)
            for (i = 0; i < nsig; i++)
                sum[i][j] += v[i] - vb[i];
        nbeats++;
    } while (getann(0, &annot) == 0 &&
             (stoptime == 0L || annot.time < stoptime));
    if (nbeats < 1) {
        fprintf(stderr, "%s: no `%s' beats found\n",
                argv[0], annstr(btype));
        exit(4);
    }
    printf("Average of %d `%s' beats:\n", nbeats, annstr(btype));
    for (j = 0; j < window; j++)
        for (i = 0; i < nsig; i++)
            printf("%g%c", (double)sum[i][j]/nbeats,
                   (i == nsig-1) ? '\n' : '\t');
    exit(0);
}
