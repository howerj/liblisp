/* 
 * Richard James Howe
 * Howe Lisp.
 *
 * Lisp interpreter.
 *
 * @author         Richard James Howe.
 * @copyright      Copyright 2013 Richard James Howe.
 * @license        LGPL      
 * @email          howe.r.j.89@gmail.com
 */

#ifndef lisp_header_guard
#define lisp_header_guard

#define true    1
#define false   0

#define MAX_STR  256            /*maximum length of allocated symbol or string */

enum file_io_type {
        io_stdin,               /*read from stdin */
        io_stdout,              /*write to stdout */
        io_stderr,              /*output to stderr */
        io_wr_file,             /*write to file */
        io_rd_file,             /*read from file */
        io_wr_str,              /*write to a string */
        io_rd_str               /*read from a string (null terminated!) */
};

/*if input or output is a file or string, store point to it*/
union io_ptr_u {
        FILE *f;
        char *s;
};

/*IO redirections.*/
struct file_io_struct {
        enum file_io_type fiot;
        int ungetc_flag;        /*for wrap_ungetc, flag */
        char ungetc_char;
        int str_index;          /*index into string */
        int str_max_len;        /*max string length */
        union io_ptr_u io_ptr;
};

typedef struct file_io_struct file_io_t;

enum error_type {
        ERR_OK,
        ERR_GENERIC_PARSE
};

enum cell_type {
        type_null,
        type_list,
        type_number,
        type_symbol,
        type_str,
        type_error
};

union cell_content {
        int i;
        struct cell *cell;
        char *s;
};

struct cell {
        enum cell_type type;
        union cell_content car;
        union cell_content cdr;
};

typedef struct cell cell_t;

/*Export these functions*/
cell_t *parse_sexpr(file_io_t * in, file_io_t * err);
void print_sexpr(cell_t * list, int depth, file_io_t * out, file_io_t * err);
void free_sexpr(cell_t * list, file_io_t * err);

#endif
