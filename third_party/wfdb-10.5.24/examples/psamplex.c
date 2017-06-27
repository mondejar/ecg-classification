#include <stdio.h>
#include <wfdb/wfdb.h>

main(int argc, char **argv)
{
    WFDB_Frequency f = (WFDB_Frequency)0;
    WFDB_Sample v[2];
    WFDB_Siginfo s[2];
    WFDB_Time t, t0, t1;

    if (argc > 1) sscanf(argv[1], "%lf", &f);
    if (f <= (WFDB_Frequency)0) f = sampfreq("100s");

    if (isigopen("100s", s, 2) < 1)
	exit(1);
    setifreq(f);
    t0 = strtim("1");
    isigsettime(t);
    t1 = strtim("2");
    for (t = t0; t <= t1; t++) {
	if (getvec(v) < 0)
	    break;
	printf("%d\t%d\n", v[0], v[1]);
    }
    exit(0);
}
