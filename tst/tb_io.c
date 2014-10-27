#include "type.h"
#include "io.h"

#define INPUT_BUF_LEN   (256)
static char fmt[INPUT_BUF_LEN];

int main(void){
        static char c;
        size_t count = 0;

        io_set_color_on(1);

        io *i = calloc(1,io_sizeof_io());
        io *o = calloc(1,io_sizeof_io());

        io_file_in(i,stdin);
        io_file_out(o,stdout);

        while(0 <= (c = io_getc(i)) && (c != '\n')){
                fmt[count++] = c;
        }

        io_printer(o, fmt);

        return EXIT_SUCCESS;
}
