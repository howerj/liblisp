#include "type.h"
#include "sexpr.h"

int main(void){
  io i = {IO_FILE_IN,  {NULL}, 0, 0, '\0', false};
  io o = {IO_FILE_OUT, {NULL}, 0, 0, '\0', false};
  io e = {IO_FILE_OUT, {NULL}, 0, 0, '\0', false};
  expr x;
  i.ptr.file = stdin;
  e.ptr.file = stderr;
  o.ptr.file = stdout;
 
  while((x = sexpr_parse(&i,&e)))
    sexpr_print(x,&o,0,&e);

  return 0;
}
