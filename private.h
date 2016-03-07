/** @file       private.h
 *  @brief      An *internal* header for the liblisp project, not for external
 *	      use. It defines the internals of what are opaque objects to the
 *	      programs outside of the library. As well as any functions which
 *	      should not be used outside the project.
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

#define SMALL_DEFAULT_LEN (64)    /**< just an arbitrary small number*/
#define DEFAULT_LEN       (256)   /**< just an arbitrary number*/
#define LARGE_DEFAULT_LEN (4096)  /**< just another arbitrary number*/
#define MAX_USER_TYPES    (256)   /**< max number of user defined types*/
#define COLLECTION_POINT  (1<<20) /**< run gc after this many allocs*/
#define BITS_IN_LENGTH    (32)    /**< number of bits in a length field*/
#define MAX_RECURSION_DEPTH (4096) /**< maximum recursion depth*/

/**@warning the following list must be kept in sync with the
 * gsym_X functions defined in there liblisp.h header (such as gsym_nil,
 * gsym_tee or gsym_error). */
#define CELL_XLIST /**< list of all special cells for initializer*/ \
	X(nil,     "nil")    X(tee,     "t")      X(quote,   "quote")\
	X(iif,     "if")     X(lambda,  "lambda") X(flambda, "flambda")\
	X(define,  "define") X(set,     "set!")   X(progn,   "progn")\
	X(cond,    "cond")   X(error,   "error")  X(loop,    "loop")\
	X(let,     "let")    X(ret,     "return") X(compile, "compile")
      /*X(lambda_eval, "lambda-eval"); // @note evaluate an expression and create a  lambda from it */

/**@brief This restores a jmp_buf stored in lisp environment if it
 *	has been copied out to make way for another jmp_buf.
 * @param USED is RBUF used?
 * @param ENV  lisp environment to restore jmp_buf to
 * @param RBUF jmp_buf to restore**/
#define LISP_RECOVER_RESTORE(USED, ENV, RBUF)\
	do {\
		if((USED)) memcpy((ENV)->recover, (RBUF), sizeof(jmp_buf));\
		else (ENV)->recover_init = 0;\
	} while(0)

typedef enum lisp_type { 
	INVALID, /**< invalid object (default), halts interpreter*/
	SYMBOL,  /**< symbol */
	INTEGER, /**< integer, a normal fixed with number*/
	CONS,    /**< cons cell*/
	PROC,    /**< lambda procedure*/
	SUBR,    /**< subroutine or primitive written in C*/
	STRING,  /**< a NUL terminated string, @todo strings should operate on UTF-8 */
	IO,      /**< Input/Output port*/
	HASH,    /**< Associative hash table*/
	FPROC,   /**< F-Expression*/
	FLOAT,   /**< Floating point number; could be float or double*/
	USERDEF  /**< User defined types*/
	/**@todo CLOSURE, MACRO (replaces FPROC) */
} lisp_type;     /**< A lisp object*/

typedef union { /**< ideally we would use void* for everything*/
	void *v;     /**< use this for integers and pointers to cells*/
	lisp_float_t f;    /**< if lisp_float_t is double it could be bigger than *v */ 
	subr prim;   /**< function pointers are not guaranteed 
	                                              to fit into a void**/
} cell_data_t; /**< a union of all the different C datatypes used*/

/**@brief A tagged object representing all possible lisp data types.
 *
 * It is possible to further reduce the memory usage if you are willing to
 * sacrifice portability.
 *
 * See:
 * <http://www.more-magic.net/posts/internals-data-representation.html>
 * For more details. The premise is to use the lowest bits of a pointer
 * to store information, they have to be zero for an allocated pointer,
 * so if they are set then it follows that the object cannot be a pointer.
 *
 * The cell uses the "struct hack", see
 * <http://c-faq.com/struct/structhack.html> **/
struct cell {
	unsigned type:   4,        /**< Type of the lisp object*/
		mark:    1,        /**< mark for garbage collection*/
		uncollectable: 1,  /**< do not free object?*/
		close:   1,        /**< object closed/invalid?*/
		used:    1, /**< object is in use by something outside lisp interpreter*/
		len:     BITS_IN_LENGTH; /**< length of data p*/
	cell_data_t p[1]; /**< uses the "struct hack", 
	                     c99 does not quite work here*/
} /*__attribute__((packed)) <- saves a fair bit of space */;  

/** @brief This describes an entry in a hash table, which is an
 *	 implementation detail of the hash, so should not be
 *	 counted upon. It represents a node in a chained hash
 *	 table */
typedef struct hash_entry {      /**< linked list of entries in a bin*/
	char *key;              /**< ASCII nul delimited string*/
	void *val;              /**< arbitrary value*/
	struct hash_entry *next; /**< next item in list*/
} hash_entry_t;

/**@todo turn into a **table into a flexible array member*/
struct hash_table {	        /**< a hash table*/
	hash_entry_t **table; /**< table of linked lists*/
	size_t len,  /**< number of 'bins' in the hash table*/
	       collisions,   /**< number of collisions */
	       replacements, /**< number of entries replaced*/
	       used          /**< number of bins used*/;
	/*state used for the foreach loop*/
	unsigned foreach :1,  /**< if true, we are in a foreach loop*/
		 free_key:1,  /**< if true, hash_destroy will free the key using free()*/
		 free_value:1; /**< if true, hash_destroy will free the value using free()*/
	size_t foreach_index; /**< index into foreach loop*/
	void *foreach_cur;    /**< current entry in foreach loop*/
};

/** @brief A structure that is used to wrap up the I/O operations 
 *	 of the lisp interpreter. */
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

/** @brief The internal state used to translate a block of memory 
 *	 using the "tr" routines, which behave similarly to the
 *	 Unix "tr" command. */
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

/** @brief Type used to form linked list of all allocations */
typedef struct gc_list { 
	lisp_cell_t *ref; /**< reference to cell for the garbage collector to act on*/
	struct gc_list *next; /**< next in list*/
} gc_list_t; 

/** @brief functions the interpreter uses for user defined types */
typedef struct { 
	lisp_free_func   free;  /**< to free a user defined type*/
	lisp_mark_func   mark;  /**< to mark a user defined type*/
	lisp_equal_func  equal; /**< to compare two user defined types*/
	lisp_print_func  print; /**< to print two user defined types*/
} lisp_user_defined_funcs_t; 

/** @brief The state for a lisp interpreter, multiple such instances
 *	 can run at the same time. It contains everything needed
 *	 to run a complete lisp environment. */
struct lisp {
	jmp_buf recover; /**< longjmp when there is an error */
#define X(CNAME, LNAME) * CNAME,
	lisp_cell_t CELL_XLIST Unused; /**< list of special forms/symbols*/
#undef X
	lisp_cell_t *all_symbols, /**< all intern'ed symbols*/
		*top_env,     /**< top level lisp environment (association list)*/
		*top_hash,    /**< top level hash (member of association list)*/
		*input,       /**< interpreter input stream*/
		*output,      /**< interpreter output stream*/
		*logging,     /**< interpreter logging/error stream*/
		*cur_env,     /**< current interpreter depth*/
		*empty_docstr,/**< empty doc string */
		**gc_stack;    /**< garbage collection stack for working items*/
	gc_list_t *gc_head;  /**< linked list of all allocated objects*/
	char *token /**< one token of put back for parser*/, 
		*buf   /**< input buffer for parser*/;
	size_t buf_allocated,/**< size of buffer "l->buf"*/
		buf_used,     /**< amount of buffer used by current string*/
		gc_stack_allocated, /**< length of buffer of GC stack*/
		gc_stack_used,      /**< elements used in GC stack*/
		gc_collectp;  /**< garbage collect after it goes too high*/
	uint64_t random_state[2] /**< PRNG state*/;
	lisp_editor_func editor; /**< line editor to use, optional*/
	lisp_user_defined_funcs_t ufuncs[MAX_USER_TYPES]; /**< for user defined types*/
	int user_defined_types_used;   /**< number of user defined types allocated*/
	int sig;   /**< set by signal handlers or other threads*/
	int log_level; /** of lisp_log_level type, the log level */
	unsigned ungettok:     1, /**< do we have a put-back token to read?*/
		recover_init: 1, /**< has the recover buffer been initialized?*/
		errors_halt:  1, /**< any error halts the interpreter if true*/
		color_on:     1, /**< REPL Colorize output*/
		prompt_on:    1, /**< REPL '>' Turn prompt on*/
		trace_on:     1, /**< turn tracing on or off*/
		gc_off:       1, /**< turn the garbage collector off*/
		editor_on:    1; /**< REPL Turn the line editor on*/
	unsigned cur_depth; /**< current recursion depth of the interpreter*/
};

/*************************** internal functions *******************************/
/* Ideally these functions would only have internal file linkage*/

/**@brief  Add a lisp object to the stack of temporary variables, anything
 *	 on this stack will not be collected until it becomes unreachable
 *	 (by being overwritten or by being popped off the stack).
 * @param  l     the lisp environment to add the cell to
 * @param  op    the cell to add
 * @return cell* the added cell, or NULL when an internal allocation failed**/
lisp_cell_t *lisp_gc_add(lisp_t *l, lisp_cell_t *op);

/**@brief This only performs a sweep, no objects are marked, this effectively
 *	invalidates the lisp environment!
 * @param l      the lisp environment to sweep and invalidate**/
void lisp_gc_sweep_only(lisp_t *l);

/**@brief Read in a lisp expression
 * @param l      a lisp environment 
 * @param i      the input port
 * @return cell* a fully parsed lisp expression**/
lisp_cell_t *reader(lisp_t *l, io_t *i);

/**@brief  Print out a lisp expression
 * @param  l      a lisp environment
 * @param  o      the output port
 * @param  op     S-Expression to print out
 * @param  depth  depth to print out S-Expression 
 * @return int    negative on error **/
int printer(lisp_t *l, io_t *o, lisp_cell_t *op, unsigned depth);

/**@brief  Evaluate a lisp expression
 * @param  l      the lisp environment to evaluate in
 * @param  depth  current evaluation depth, not to exceed a limit
 * @param  exp    the expression to evaluate
 * @param  env    the environment to evaluate in
 * @return cell*  the evaluated expression **/
lisp_cell_t *eval(lisp_t *l, unsigned depth, lisp_cell_t *exp, lisp_cell_t *env);

/**@brief  find a key in an association list (a-list)
 * @param  key    key to search for
 * @param  alist  association list
 * @return if key is found it returns a cons of the key and the associated
 *	 value, if not found it returns nil**/
lisp_cell_t *lisp_assoc(lisp_cell_t *key, lisp_cell_t *alist);

/**@brief  Extend the top level lisp environment with a key value pair
 * @param  l   the lisp environment to perform the extension on
 * @param  sym the symbol to associate with a value
 * @param  val value add to the top level environment
 * @return NULL on failure, not NULL on success**/
lisp_cell_t *lisp_extend_top(lisp_t *l, lisp_cell_t *sym, lisp_cell_t *val);

/**@brief  Count the number of arguments in a validation format string
 *         validation format string, as passed to lisp_validate_args()
 * @return argument count**/
size_t lisp_validate_arg_count(const char *fmt);

#ifdef __cplusplus
}
#endif
#endif
