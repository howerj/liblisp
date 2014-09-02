#include "type.h"
#include "lisp.h"
#include "linenoise.h"

int main(void){
  lisp_end(lisp_repl(lisp_init()));
  return 0;
}
