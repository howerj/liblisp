#include "type.h"
#include "lisp.h"

int main(void){
  lisp_end(lisp_repl(lisp_init()));
  return 0;
}
