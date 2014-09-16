#include "type.h"
#include "sexpr.h"

int main(void){
  io *i = calloc(1, io_sizeof_io());
  io *o = calloc(1, io_sizeof_io());
  io *e = calloc(1, io_sizeof_io());
  expr x;

  io_file_in(i, stdin);
  io_file_out(o, stdout);
  io_file_out(e, stderr);
 
  while((x = sexpr_parse(i,e)))
    sexpr_print(x,o,0,e);

  free(i);
  free(o);
  free(e);

  return 0;
}
