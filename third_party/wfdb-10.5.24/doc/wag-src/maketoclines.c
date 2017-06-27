/* file: maketoclines.c		G. Moody	29 October 2002

Create TOC entries from man page name lines
*/

#include <stdio.h>
#include <string.h>

main()
{
    static char buf[200], *p, *q;

    while (fgets(buf, sizeof(buf), stdin)) {
	if ((p = strstr(buf, " \\- ")) == NULL)
	    continue;
	*p = '\0';
        p += 4;
	if ((q = strstr(p, "\t")) == NULL)
	    continue;
	*q = '\0';
	q++;
	q[strlen(q) - 1] = '\0';
	printf("\\contentsline {section}{\\textbf{%s}: %s}{%s}\n", buf, p, q);
    }
}

	
