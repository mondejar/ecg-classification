#include <stdio.h>
#include <wfdb/wfdb.h>

main(argc, argv)
int argc;
char *argv[];
{
    WFDB_Anninfo a;
    WFDB_Annotation annot;

    if (argc < 3) {
        fprintf(stderr, "usage: %s annotator record\n", argv[0]);
        exit(1);
    }
    a.name = argv[1]; a.stat = WFDB_READ;
    (void)sampfreq(argv[2]);
    if (annopen(argv[2], &a, 1) < 0) exit(2);
    while (getann(0, &annot) == 0)
        printf("%s (%ld) %s %d %d %d %s\n",
               timstr(-(annot.time)),
               annot.time,
               annstr(annot.anntyp),
               annot.subtyp, annot.chan, annot.num,
               (annot.aux != NULL && *annot.aux > 0) ?
                annot.aux+1 : "");
    exit(0);
}
