#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

main(argc, argv)
int argc;
char *argv[];
{
    int rr, *rrhist, rrmax;
    long t;
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    void *calloc();

    if (argc < 3) {
        fprintf(stderr, "usage: %s annotator record\n", argv[0]);
        exit(1);
    }
    a.name = argv[1]; a.stat = WFDB_READ;
    if (annopen(argv[2], &a, 1) < 0) exit(2);
    if ((rrmax = (int)(3*sampfreq(argv[2]))) <= 0) exit(3);
    if ((rrhist = (int *)calloc(rrmax+1, sizeof(int))) == NULL) {
        fprintf(stderr, "%s: insufficient memory\n", argv[0]);
        exit(4);
    }
    while (getann(0, &annot) == 0 && !isqrs(annot.anntyp))
        ;
    t = annot.time;
    while (getann(0, &annot) == 0)
        if (isqrs(annot.anntyp)) {
            if ((rr = annot.time - t) > rrmax) rr = rrmax;
            rrhist[rr]++;
            t = annot.time;
        }
    for (rr = 1; rr < rrmax; rr++)
        printf("%4d %s\n", rrhist[rr], mstimstr((long)rr));
    printf("%4d %s (or longer)\n", rrhist[rr], mstimstr((long)rr));
    exit(0);
}
