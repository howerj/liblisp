#include <iostream>
// Should work without the extern "C" {}; as it has that in the headers.
#include "lisp.h"
int main(void){
        std::cout << "C++ test" << std::endl;
        lisp_end(lisp_repl(lisp_init()));
        return 0;
}
