#include <stdio.h>
#include <wfdb/wfdb.h>

main()
{
    WFDB_Anninfo a;
    WFDB_Annotation annot;

    a.name = "atr"; a.stat = WFDB_READ;
    if (annopen("100s", &a, 1) < 0)
        exit(1);
    while (getann(0, &annot) == 0)
        printf("%s %s\n", mstimstr(annot.time), annstr(annot.anntyp));
    exit(0);
}
