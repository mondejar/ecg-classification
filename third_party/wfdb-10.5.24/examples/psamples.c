#include <stdio.h>
#include <wfdb/wfdb.h>

main()
{
    int i;
    WFDB_Sample v[2];
    WFDB_Siginfo s[2];

    if (isigopen("100s", s, 2) < 1)
	exit(1);
    for (i = 0; i < 10; i++) {
	if (getvec(v) < 0)
	    break;
	printf("%d\t%d\n", v[0], v[1]);
    }
    exit(0);
}
