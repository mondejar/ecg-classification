#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    WFDB_Anninfo an[2];
    WFDB_Annotation annot;

    if (argc < 2) {
        fprintf(stderr, "usage: %s record\n", argv[0]);
        exit(1);
    }
    an[0].name = "atr"; an[0].stat = WFDB_READ;
    an[1].name = "aha"; an[1].stat = WFDB_AHA_WRITE;
    if (annopen(argv[1], an, 2) < 0) exit(2);
    while (getann(0, &annot) == 0 && putann(0, &annot) == 0)
        ;
    wfdbquit();
    exit(0);
}
