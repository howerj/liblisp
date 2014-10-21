#include "type.h"
#include "sexpr.h"

int main(void){
  io *i = calloc(1, io_sizeof_io());
  io *o = calloc(1, io_sizeof_io());
  io *e = io_get_error_stream();
  expr x;

  io_file_in(i, stdin);
  io_file_out(o, stdout);
  io_file_out(e, stderr);
 
  while((x = sexpr_parse(i)))
    sexpr_print(x,o,0);

  free(i);
  free(o);

  return 0;
}
