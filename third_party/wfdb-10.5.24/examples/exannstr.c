#include <stdio.h>
#include <wfdb/wfdb.h>
#include <wfdb/ecgcodes.h>

main()
{
    int i;

    printf("Code\tMnemonic\tDescription\n");
    for (i = 1; i <= ACMAX; i++) {
        printf("%3d\t%s", i, annstr(i));
        if (anndesc(i) != NULL)
            printf("\t\t%s", anndesc(i));
        printf("\n");
    }
}
