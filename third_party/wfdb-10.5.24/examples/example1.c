#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgmap.h>

main()
{
    WFDB_Anninfo an[2];
    char record[8], iann[10], oann[10];
    WFDB_Annotation annot;

    printf("Type record name: ");
    fgets(record, 8, stdin); record[strlen(record)-1] = '\0';
    printf("Type input annotator name: ");
    fgets(iann, 10, stdin); iann[strlen(iann)-1] = '\0';
    printf("Type output annotator name: ");
    fgets(oann, 10, stdin); oann[strlen(oann)-1] = '\0';
    an[0].name = iann; an[0].stat = WFDB_READ;
    an[1].name = oann; an[1].stat = WFDB_WRITE;
    if (annopen(record, an, 2) < 0) exit(1);
    while (getann(0, &annot) == 0)
        if (isqrs(annot.anntyp)) {
            annot.anntyp = NORMAL;
            if (putann(0, &annot) < 0) break;
        }
    wfdbquit();
}
