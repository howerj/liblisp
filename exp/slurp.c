#include <stdio.h>
#include <stdlib.h>

char *getdelimited(FILE *fp, char delim)
{ /*<http://c-faq.com/malloc/reallocnull.html>*/
        char *newbuf, *retbuf = NULL;
        size_t nchmax = 0, nchread = 0;
        int c;

        while((c = getc(fp)) != EOF) {
                if(nchread >= nchmax) {
                        nchmax += 20;
                        if(nchread >= nchmax) /*overflow check*/
                                return free(retbuf), NULL;
                        if(!(newbuf = realloc(retbuf, nchmax + 1)))
                                return free(retbuf), NULL;
                        retbuf = newbuf;
                }

                if(c == delim)
                        break;
                retbuf[nchread++] = c;
        }

        if(retbuf) {
                retbuf[nchread] = '\0';
                newbuf = realloc(retbuf, nchread + 1);
                if(newbuf)
                        retbuf = newbuf;
        }

        return retbuf;
}

char *getaline(FILE *ifp) { return getdelimited(ifp, '\n'); }

char **slurp(FILE *ifp)
{ 
        char *p, **lines = NULL;
        size_t nalloc = 0, nlines = 0;
        while((p = getaline(ifp))) {
                if(nlines >= nalloc) {
                        nalloc += 50;
                        if(!(lines = realloc(lines, nalloc * sizeof(char *) + 1)))
                                return NULL;
                }
                lines[nlines++] = p;
        }
        lines[nlines] = NULL;
        return lines;
}

int main(void)
{
        char *line, **lines = slurp(stdin);
        while((line = *lines++))
                printf("%s\n", line);
        return 0;
}
