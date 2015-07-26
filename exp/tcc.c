#include <libtcc.h>
#include <stdio.h>
#include <stdlib.h>

#define error(MSG) do{ fprintf(stderr, "%s %s %d\n", (MSG), __FILE__, __LINE__); exit(1); } while(0)

char prog[] = {
        "int padd(int x, int y)\n"
        "{\n"
        "       printf(\"%d + %d = %d\\n\", x, y, x+y);\n"
        "       return x+y;\n"
        "}\n"
};

int main(void) {
        TCCState *s;
        int (*func)(int, int);

        if(!(s = tcc_new())) error("tcc_new failed");

        tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

        if(tcc_compile_string(s, prog) < 0) error("compilation failed");

        if(tcc_relocate(s, TCC_RELOCATE_AUTO) < 0) error("relocate failed");

        if(!(func = tcc_get_symbol(s, "padd"))) error("get symbol failed");

        printf("returned: %d\n", func(2, 10));

        tcc_delete(s);
        return 0;
}

