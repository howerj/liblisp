#ifndef IO_H
#define IO_H

/**** macros ******************************************************************/
#define BUFLEN      (256u)
#define report(X)   doreport(X,__FILE__,__LINE__,e)
#define NULLCHK(X)  if(NULL == (X)){ report("null dereference"); exit(-1);}
/******************************************************************************/

/**** function prototypes *****************************************************/
int wgetc(io *p, io *e);
int wputc(char c, io *p, io *e);
int wungetc(char c, io *p, io *e);
int wprintd(cell_t d, io *o, io *e);
void doreport(const char *s, char *cfile, unsigned int linenum, io *e);
/******************************************************************************/
#endif
