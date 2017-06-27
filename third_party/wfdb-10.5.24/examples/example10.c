#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

#define abs(A)	((A) >= 0 ? (A) : -(A))

main(argc, argv)
int argc;
char *argv[];
{
    int filter, time=0, slopecrit, sign, maxslope=0, nsig, nslope=0,
        qtime, maxtime, t0, t1, t2, t3, t4, t5, t6, t7, t8, t9,
        ms160, ms200, s2, scmax, scmin = 0;
    WFDB_Anninfo a;
    WFDB_Annotation annot;
    WFDB_Sample *v;
    WFDB_Siginfo *s;

    if (argc < 2) {
        fprintf(stderr, "usage: %s record [threshold]\n", argv[0]);
        exit(1);
    }
    a.name = "qrs"; a.stat = WFDB_WRITE;

    if ((nsig = isigopen(argv[1], NULL, 0)) < 1) exit(2);
    s = (WFDB_Siginfo *)malloc(nsig * sizeof(WFDB_Siginfo));
    v = (WFDB_Sample *)malloc(nsig * sizeof(WFDB_Sample));
    if (s == NULL || v == NULL) {
	fprintf(stderr, "%s: insufficient memory\n", argv[0]);
	exit(2);
    }
    if (wfdbinit(argv[1], &a, 1, s, nsig) != nsig) exit(2);
    if (sampfreq((char *)NULL) < 240. ||
	sampfreq((char *)NULL) > 260.)
	setifreq(250.);
    if (argc > 2) scmin = muvadu(0, atoi(argv[2]));
    if (scmin < 1) scmin = muvadu(0, 1000);
    slopecrit = scmax = 10 * scmin;
    ms160 = strtim("0.16"); ms200 = strtim("0.2"); s2 = strtim("2");
    annot.subtyp = annot.chan = annot.num = 0; annot.aux = NULL;
    (void)getvec(v);
    t9 = t8 = t7 = t6 = t5 = t4 = t3 = t2 = t1 = v[0];

    do {
        filter = (t0 = v[0]) + 4*t1 + 6*t2 + 4*t3 + t4
                - t5         - 4*t6 - 6*t7 - 4*t8 - t9;
        if (time % s2 == 0) {
            if (nslope == 0) {
                slopecrit -= slopecrit >> 4;
                if (slopecrit < scmin) slopecrit = scmin;
            }
            else if (nslope >= 5) {
                slopecrit += slopecrit >> 4;
                if (slopecrit > scmax) slopecrit = scmax;
            }
        }
        if (nslope == 0 && abs(filter) > slopecrit) {
            nslope = 1; maxtime = ms160;
            sign = (filter > 0) ? 1 : -1;
            qtime = time;
        }
        if (nslope != 0) {
            if (filter * sign < -slopecrit) {
                sign = -sign;
                maxtime = (++nslope > 4) ? ms200 : ms160;
            }
            else if (filter * sign > slopecrit &&
                     abs(filter) > maxslope)
                maxslope = abs(filter);
            if (maxtime-- < 0) {
                if (2 <= nslope && nslope <= 4) {
                    slopecrit += ((maxslope>>2) - slopecrit) >> 3;
                    if (slopecrit < scmin) slopecrit = scmin;
                    else if (slopecrit > scmax) slopecrit = scmax;
                    annot.time = strtim("i") - (time - qtime) - 4;
                    annot.anntyp = NORMAL; (void)putann(0, &annot);
                    time = 0;
                }
                else if (nslope >= 5) {
                    annot.time = strtim("i") - (time - qtime) - 4;
                    annot.anntyp = ARFCT; (void)putann(0, &annot);
                }
                nslope = 0;
            }
        }
        t9 = t8; t8 = t7; t7 = t6; t6 = t5; t5 = t4;
        t4 = t3; t3 = t2; t2 = t1; t1 = t0; time++;
    } while (getvec(v) > 0);

    wfdbquit();
    exit(0);
}
