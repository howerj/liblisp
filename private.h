/** @file       private.h
 *  @brief      An *internal* header for the liblisp project, not for external
 *              use. It defines the internals of what are opaque objects to the
 *              programs outside of the library. As well as any functions which
 *              should not be used outside the project.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com **/

#ifndef PRIVATE_H
#define PRIVATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdio.h>

#define DEFAULT_LEN       (256)   /**< just an arbitrary smallish number*/
#define LARGE_DEFAULT_LEN (4096)  /**< just another arbitrary number*/
#define REGEX_MAX_DEPTH   (8192)  /**< max recursion depth of a regex*/
#define MAX_USER_TYPES    (256)   /**< max number of user defined types*/
#define COLLECTION_POINT  (1<<20) /**< run gc after this many allocs*/

/**@warning the following list must be kept in sync with the
 * gsym_X functions defined in there liblisp.h header (such as gsym_nil,
 * gsym_tee or gsym_error). */
#define CELL_XLIST /**< list of all special cells for initializer*/ \
        X(nil,     "nil")    X(tee,     "t")\
        X(quote,   "quote")  X(iif,     "if")\
        X(lambda,  "lambda") X(flambda, "flambda")\
        X(define,  "define") X(set,     "set!")\
        X(progn,   "progn")  X(cond,    "cond")\
        X(error,   "error")  X(env,     "environment")\
        X(let,     "let")    X(ret,     "return")\
        X(loop,    "loop")

/**@brief This restores a jmp_buf stored in lisp environment if it
 *        has been copied out to make way for another jmp_buf.
 * @param USED is RBUF used?
 * @param ENV  lisp environment to restore jmp_buf to
 * @param RBUF jmp_buf to restore**/
#define RECOVER_RESTORE(USED, ENV, RBUF)\
        if((USED)) memcpy((ENV)->recover, (RBUF), sizeof(jmp_buf));\
        else (ENV)->recover_init = 0;

typedef enum lisp_type { 
        INVALID, /**< invalid object (default), halts interpreter*/
        SYMBOL,  /**< symbol */
        INTEGER, /**< integer, a normal fixed with number*/
        CONS,    /**< cons cell*/
        PROC,    /**< lambda procedure*/
        SUBR,    /**< subroutine or primitive written in C*/
        STRING,  /**< a NUL terminated string*/
        IO,      /**< Input/Output port*/
        HASH,    /**< Associative hash table*/
        FPROC,   /**< F-Expression*/
        FLOAT,   /**< Floating point number; could be float or double*/
        USERDEF  /**< User defined types*/
} lisp_type;     /**< A lisp object*/

typedef union cell_data { /**< ideally we would use void* for everything*/
        void *v;     /**< use this for integers and points to cells*/
        lfloat f;    /**< if lfloat is double it could be bigger than *v */ 
        subr prim;   /**< function pointers are not guaranteed 
                                                      to fit into a void**/
} cell_data; /**< a union of all the different C datatypes used*/

struct cell {
        unsigned type:    4,        /**< Type of the lisp object*/
                 mark:    1,        /**< mark for garbage collection*/
                 uncollectable: 1,  /**< do not free object?*/
                 close:   1,        /**< object closed/invalid?*/
                 len:     32;       /**< length of data p*/
        cell_data p[1]; /**< uses the "struct hack", 
                             c99 does not quite work here*/
}; /**< a tagged object representing all possible lisp objects*/

typedef struct hashentry {      /**< linked list of entries in a bin*/
        char *key;              /**< ASCII nul delimited string*/
        void *val;              /**< arbitrary value*/
        struct hashentry *next; /**< next item in list*/
} hashentry;

struct hashtable {                /**< a hash table*/
        size_t len;               /**< number of 'bins' in the hash table*/
        struct hashentry **table; /**< table of linked lists*/
};

struct io {
        union { FILE *file; char *str; } p; /**< the actual file or string*/
        size_t position, /**< current position, used for string*/
               max;      /**< max position in buffer, used for string*/
        enum { IO_INVALID,    /**< invalid (default)*/ 
               FIN,           /**< file input*/
               FOUT,          /**< file output*/
               SIN,           /**< string input*/
               SOUT,          /**< string output, write to char* block*/
               NULLOUT        /**< null output, discard output*/
        } type; /**< type of the IO object*/
        unsigned ungetc :1, /**< push back is in use?*/
                 color  :1, /**< colorize output? Used in lisp_print*/
                 pretty :1, /**< pretty print output? Used in lisp_print*/
                 eof    :1; /**< End-Of-File marker*/
        char c; /**< one character of push back*/
};

struct tr_state {
        int set_squ[UINT8_MAX+1], /**< squeeze a character sequence?*/
            set_del[UINT8_MAX+1], /**< delete a character?*/
            compliment_seq,  /**< compliment sequence*/
            squeeze_seq,     /**< squeeze sequence*/
            delete_seq,      /**< delete a sequence of character*/
            truncate_seq;    /**< truncate s1 to length of s2*/
        uint8_t set_tr[UINT8_MAX+1], /**< final map of s1 to s2 */
                previous_char; /**< previous translation char, for squeeze*/
};

typedef struct gc_list { 
        cell *ref; /**< reference to cell for the garbage collector to act on*/
        struct gc_list *next; /**< next in list*/
} gc_list; /**< type used to form linked list of all allocations*/

typedef struct userdef_funcs { 
        ud_free   free;  /**< to free a user defined type*/
        ud_mark   mark;  /**< to mark a user defined type*/
        ud_equal  equal; /**< to compare two user defined types*/
        ud_print  print; /**< to print two user defined types*/
} userdef_funcs; /**< functions the interpreter uses for user defined types*/

struct lisp {
        jmp_buf recover; /**< jump here when there is an error*/
        io *ifp /**< input port @todo replace with cell *input (completely) */, 
           *ofp /**< output port @todo replace with cell *output */, 
           *efp /**< error output port @todo replace with cell *logging*/;
        cell *all_symbols, /**< all intern'ed symbols*/
             *top_env,     /**< top level lisp environment*/
             **gc_stack;   /**< garbage collection stack for working items*/
        gc_list *gc_head;  /**< linked list of all allocated objects*/
        char *token /**< one token of put back for parser*/, 
             *buf   /**< input buffer for parser*/;
        size_t buf_allocated,/**< size of buffer "l->buf"*/
               buf_used,     /**< amount of buffer used by current string*/
               gc_stack_allocated,    /**< length of buffer of GC stack*/
               gc_stack_used,         /**< elements used in GC stack*/
               gc_collectp,  /**< garbage collect after it goes too high*/
               max_depth,    /**< max recursion depth*/
               cur_depth     /**< current depth*/;
        uint64_t random_state[2] /**< PRNG state*/;
        int sig;   /**< set by signal handlers or other threads*/
        int trace; /**< turn tracing on or off */
        unsigned ungettok:     1, /**< do we have a put-back token to read?*/
                 recover_init: 1, /**< has the recover buffer been initialized?*/
                 errors_halt:  1, /**< any error halts the interpreter if true*/
                 color_on:     1, /**< REPL Colorize output*/
                 prompt_on:    1, /**< REPL '>' Turn prompt on*/
                 editor_on:    1; /**< REPL Turn the line editor on*/
        userdef_funcs ufuncs[MAX_USER_TYPES]; /**< for user defined types*/
        int user_defined_types_used;   /**< number of user defined types allocated*/
        editor_func editor; /**< line editor to use, optional*/

#define X(CNAME, LNAME) * CNAME, 
        /* This is a list of all the special symbols in use within the
         * interpreter*/
        cell CELL_XLIST Unused;
#undef X
        cell *input, *output, *logging;
};

/*************************** internal functions *******************************/
/* Ideally these functions would only have internal file linkage*/

/**@brief  Add a lisp object to the stack of temporary variables, anything
 *         on this stack will not be collected until it becomes unreachable
 *         (by being overwritten or by being popped off the stack).
 * @param  l     the lisp environment to add the cell to
 * @param  op    the cell to add
 * @return cell* the added cell, or NULL when an internal allocation failed**/
cell *gc_add(lisp *l, cell* op);

/**@brief Mark all reachable objects then perform a sweep.
 * @param l      the lisp environment to perform the mark and sweep in**/
void gc_mark_and_sweep(lisp *l);

/**@brief This only performs a sweep, no objects are marked, this effectively
 *        invalidates the lisp environment!
 * @param l      the lisp environment to sweep and invalidate**/
void gc_sweep_only(lisp *l);

/**@brief Read in a lisp expression
 * @param l      a lisp environment 
 * @param i      the input port
 * @return cell* a fully parsed lisp expression**/
cell *reader(lisp *l, io *i);

/**@brief  Print out a lisp expression
 * @param  l      a lisp environment
 * @param  o      the output port
 * @param  op     S-Expression to print out
 * @param  depth  depth to print out S-Expression 
 * @return int    negative on error **/
int printer(lisp *l, io *o, cell *op, unsigned depth);

/**@brief  Evaluate a lisp expression
 * @param  l      the lisp environment to evaluate in
 * @param  depth  current evaluation depth, not to exceed a limit
 * @param  exp    the expression to evaluate
 * @param  env    the environment to evaluate in
 * @return cell*  the evaluated expression **/
cell *eval(lisp *l, unsigned depth, cell *exp, cell *env);

/**@brief  find a key in an association list (a-list)
 * @param  key    key to search for
 * @param  alist  association list
 * @return if key is found it returns a cons of the key and the associated
 *         value, if not found it returns nil**/
cell *assoc(cell *key, cell *alist);

/**@brief  Extend the top level lisp environment with a key value pair
 * @param  l   the lisp environment to perform the extension on
 * @param  sym the symbol to associate with a value
 * @param  val value add to the top level environment
 * @return NULL on failure, not NULL on success**/
cell *extend_top(lisp *l, cell *sym, cell *val);

/**@brief  Count the number of arguments in a validation format string
 * @brief  validation format string, as passed to lisp_validate_args()
 * @return argument count**/
size_t validate_arg_count(const char *fmt);

#ifdef __cplusplus
}
#endif
#endif
