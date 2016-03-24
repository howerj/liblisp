/** @file       liblisp.h
 *  @brief      A minimal lisp interpreter and utility library
 *  @author     Richard Howe (2015, 2016)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  The API documentation for the lisp interpreter is kept here, so if you have
 *  access to this header then you (should) have all the documentation needed
 *  to use this library.
 *
 *  See <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> for the
 *  license.
 *
 *  Do not pass NULL to any of these functions unless they specifically mention
 *  that you can. They "assert()" their inputs.
 *
 **/
#ifndef LIBLISP_H
#define LIBLISP_H
#ifdef __cplusplus
extern "C" {
#endif
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
/** @brief The LIBLISP_API macro is needed due to the way Windows handles
 * DLLs, the build system should define COMPILING_LIBLISP when
 * building "liblisp.dll", but only then. **/
        #ifdef COMPILING_LIBLISP
                #define LIBLISP_API __declspec(dllexport) /**< export function*/
        #else
                #define LIBLISP_API __declspec(dllimport) /**< import function*/
        #endif
#else
        #define LIBLISP_API /**< not needed*/
#endif

/************************* structs, opaque types, enums ***********************/

/** These are all the types needed for the interpreter and associated utility
 *  functions. They should be opaque types if possible to hide the
 *  implementation details. **/

#ifndef USE_SINGLE_PRECISION_FLOATS
typedef double lisp_float_t; /**< float type used by this lisp*/
#define LFLT_EPSILON  DBL_EPSILON /**< machine epsilon of lisp_float_t**/
#define LFLT_MIN      DBL_MIN /**< smallest decimal representable by lisp_float_t**/
#define LFLT_MAX      DBL_MAX /**< maximum representable lisp_float_t number **/
#else
typedef float  lisp_float_t; /**< alternate float type used by this lisp, best for 32-bit systems*/
#define LFLT_EPSILON  FLT_EPSILON /**< machine epsilon of lisp_float_t**/
#define LFLT_MIN      FLT_MIN /**< smallest decimal representable by lisp_float_t**/
#define LFLT_MAX      FLT_MAX /**< maximum representable lisp_float_t number **/
#endif

typedef struct io io_t;                 /**< generic I/O to files, strings, ...*/
typedef struct hash_table hash_table_t; /**< standard hash table implementation */
typedef struct tr_state tr_state_t;     /**< state for translation functions */
typedef struct cell lisp_cell_t;               /**< a lisp object, or "cell" */
typedef struct lisp lisp_t;             /**< a full lisp environment */
typedef lisp_cell_t *(*lisp_subr_func)(lisp_t *, lisp_cell_t *); /**< lisp primitive operations */
typedef void *(*hash_func)(const char *key, void *val); /**< for hash foreach */

typedef void (*lisp_free_func)(lisp_cell_t *);       /**< function to free a user type*/
typedef void (*lisp_mark_func)(lisp_cell_t *);       /**< marking function for user types*/
typedef int  (*lisp_equal_func)(lisp_cell_t *, lisp_cell_t *);  /**< equality function for user types*/
typedef int  (*lisp_print_func)(io_t *, unsigned, lisp_cell_t *); /**< print out user def types*/

/**@brief This is a prototype for the (optional) line editing functionality
 *        that the REPL can use. The editor function should accept a prompt
 *        to print out and return a line of text to be evaluated by the
 *        REPL.**/
typedef char *(*lisp_editor_func)(const char *);

typedef enum {
        TR_OK      =  0, /**< no error*/
        TR_EINVAL  = -1, /**< invalid mode sequence*/
        TR_DELMODE = -2  /**< set 2 string should be NULL in delete mode*/
} tr_errors; /**< possible error values returned by tr_init*/

typedef enum {
	LISP_LOG_LEVEL_OFF,    /**< log messages not printed out*/
	LISP_LOG_LEVEL_ERROR,  /**< default: log only error messages */ 
	LISP_LOG_LEVEL_NOTE,   /**< print out notes*/
	LISP_LOG_LEVEL_DEBUG,  /**< print out huge amounts of debug information if compiled with debug on*/
	LISP_LOG_LEVEL_LAST_INVALID /**< using an invalid log levels causes an abort*/
} lisp_log_level;

typedef struct {
	char *name,        /**< name of function to add*/
		*validate, /**< validation string see lisp_validate_args(), NULL turns checking off */
		*docstring;/**< documentation string for function, a short description (<100 chars is advisable)*/
	lisp_subr_func p; /**< the actual subroutine to add */
} lisp_module_subroutines_t; /**< structure for the convenience function lisp_add_module_subroutines */

/************************** useful functions *********************************/

/* This module mostly has string manipulation functions to make processing
 * them a little more comfortable in C. The interpreter uses them internally
 * but they is little reason to hide them. */

/**@brief print out a neatly formatted error message to stderr then exit(-1).
 * @param msg  message to print out
 * @param file file name to print out, use __FILE__
 * @param func function name to print out, use __func__
 * @param line line number to print out, use __LINE__**/
LIBLISP_API void pfatal(const char *msg, const char *file, const char *func, long line);

/** @brief   lstrdup for where it is not available, despite what POSIX thinks
 *           it is not OK to stick random junk into the C-library.
 *  @param   s  the string to duplicate
 *  @return  char* NULL on error, a duplicate string that can be freed
 *                 otherwise **/
LIBLISP_API char *lstrdup(const char *s);

/** @brief   lstrdup, but it will die if the memory cannot be allocated.
 *  @param   s  the string to duplicate
 *  @return  char* a duplicate string that can be freed */
LIBLISP_API char *lstrdup_or_abort(const char *s);

/** @brief lstrcatend is like "strcat" from "string.h", however it returns the
 *         end of the new string instead instead of the start of it which is
 *         much more useful.
 *  @param  dest a block of memory containing a ASCII NUL terminated string,
 *               it must be capable of hold the "dest" string as well as the
 *               "src" string, plus a single ASCII NUL character
 *  @param  src  An ASCII NUL terminated string to copy into dest
 *  @return char* The end of the new string**/
LIBLISP_API char *lstrcatend(char *dest, const char *src);

/** @brief   a dumb, but cleverly simple, regex matcher, from
 *           <http://c-faq.com/lib/regex.html>, but modified slightly, by
 *           Arjan Kenter, copied below:
 *
 *           int match(char *pat, char *str) {
 *                   switch(*pat) {
 *                   case '\0': return !*str;
 *                   case '*':  return match(pat+1, str) ||
 *                                          *str && match(pat, str+1);
 *                   case '?':  return *str && match(pat+1, str+1);
 *                   default:   return *pat == *str && match(pat+1, str+1);
 *                   }
 *           }
 *
 *           Another regex matcher that is a little longer and more powerful
 *           can be found at:
 *
 *   <http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>
 *
 *           Which is a little more powerful but longer.
 *
 *           This pattern matcher is a little different, to the one shown
 *           above, it has error handling and has a different syntax.
 *
 *           'c'  An arbitrary character
 *           '*'  Any string of characters
 *           '.'  Any character
 *
 *           And allows for characters to be escaped with the backslash
 *           character, "\*" and "\." match only a single asterisk and
 *           period respectively.
 *
 *  @param   pat  a pattern to match
 *  @param   str  the string to match on
 *  @return  int  1 == match, 0 == no match, -1 == error **/
LIBLISP_API int match(char *pat, char *str);

/** @brief   a hash algorithm by Dan Bernstein, see
 *           <http://www.cse.yorku.ca/~oz/hash.html> for more information.
 *  @param   s      the string to hash
 *  @param   len    length of s
 *  @return  uint32_t      the resulting hash **/
LIBLISP_API uint32_t djb2(const char *s, size_t len);

/** @brief   get a line text from a file
 *  @param   in    an input file
 *  @return  char* a line of input, without the newline**/
LIBLISP_API char *getaline(FILE *in);

/** @brief   gets a record delimited by "delim"
 *  @param   in    an input file
 *  @param   delim the delimiter, can be EOF
 *  @return  char* a record of text, minus the delimiter**/
LIBLISP_API char *getadelim(FILE *in, int delim);

/** @brief concatenate a variable amount of strings with a separator
 *         inbetween them
 *  @param separator    a string to insert between the other strings
 *  @param first        the first string to concatenate
 *  @param ...          a list of strings, with a NULL sentinel at the end
 *  @return char *      a NUL terminated string that can be freed**/
LIBLISP_API char *vstrcatsep(const char *separator, const char *first, ...);

/** @brief returns whether a string is balanced, that is if the number
 *         of parenthesis match up, it also takes into account strings.
 *
 *  For example, the following statements are true:
 *
 *         unbalance("(((", '(', ')') > 0;
 *         unbalance(")))", '(', ')') < 0;
 *         unbalance("()", '(', ')') == 0;
 *         unbalance("", '(', ')') == 0;
 *         unbalance("\"( Hello World\\" ( \" ))", '(', ')') > 0;
 *
 *  @param sexpr string to count balance in
 *  @param  lpar left  parenthesis 
 *  @param  rpar right parenthesis
 *  @return int  positive more '(', negative more ')', zero == balanced**/
LIBLISP_API int unbalanced(const char *sexpr, char lpar, char rpar);

/** @brief reverse a block of characters *in place* of a given length
 *  @param  s     character string to reverse
 *  @param  len   length of block to reverse
 *  @return char* reversed block **/
LIBLISP_API char *breverse(char *s, size_t len);

/** @brief  convert an integer into a string in a given base, the string
 *          should be freed when no longer needed.
 *  @warning The current implementation assumes a twos compliment
 *           representation of signed integers
 *  @param  d     an signed integer!
 *  @param  base  base to print in assert(base > 1 && base < 37);
 *  @return char* a string representing the number or NULL on out of memory**/
LIBLISP_API char* dtostr(intptr_t d, unsigned base);

/** @brief  convert an integer into a string in a given base, the string
 *          should be freed when no longer needed.
 *  @param  u     an unsigned integer!
 *  @param  base  base to print in assert(base > 1 && base < 37);
 *  @return char* a string representing the number or NULL on out of memory**/
LIBLISP_API char* utostr(uintptr_t u, unsigned base);

/************************** hash library *************************************/

/* A small hash library implementation, all state needed for it is held
 * within the table. */

typedef void  (*hash_free_key_f)(void *);
typedef void  (*hash_free_val_f)(void *);
typedef int   (*hash_compare_key_f)(const void *, const void*);
typedef uint32_t (*hash_f)(const void *);

/** @brief   create new instance of a hash table, hashes created by
 *           this method will treat keys as string, use strcmp to
 *           compare strings, hash strings with djb2, and will not
 *           free either the key or the value when destroyed.
 *  @param   len number of buckets in the table
 *  @return  hash_table_t* initialized hash table or NULL**/
LIBLISP_API hash_table_t *hash_create(size_t len);

/** @brief  copy a hash table
 *  @param  src  the hash table you want to copy 
 *  @return hash_table_t*  a new hash table, which is a copy of src.*/
LIBLISP_API hash_table_t *hash_copy(hash_table_t *src);

/** @brief   create new instance of a hash table, with custom functions
 *           for freeing the key, the value, comparing keys and hashing
 *           keys.
 *  @param   len number of buckets in the table
 *  @param   k   function called to free a key on destruction of hash
 *  @param   v   function called to free a value on destruction of hash
 *  @param   c   function called to compare two keys
 *  @param   h   function called to hash a key into a bucket
 *  @return  hash_table_t* initialized hash table or NULL**/
LIBLISP_API hash_table_t *hash_create_custom(size_t len, hash_free_key_f k, hash_free_val_f v, hash_compare_key_f c, hash_f h);


/** @brief   destroy and invalidate a hash table, this will not attempt to
 *           free the values associated with a key, or the keys, this is
 *           the responsibility of the caller of hash_insert, at least not
 *           by default.
 *  @param   h table to destroy or NULL**/
LIBLISP_API void hash_destroy(hash_table_t *h);

/** @brief   insert a value into an initialized hash table
 *  @param   ht    table to insert key-value pair into
 *  @param   key   key to associate with a value
 *  @param   val   value to lookup
 *  @return  int   0 on success, < 0 on failure**/
LIBLISP_API int hash_insert(hash_table_t *ht, char *key, void *val);

/** @brief   look up a key in a table
 *  @param   table table to look for value in
 *  @param   key   a key to look up a value with
 *  @return  void* either the value you were looking for a NULL**/
LIBLISP_API void *hash_lookup(const hash_table_t *table, const char *key);

/** @brief  Apply "func" on each key-val pair in the hash table until
 *          the function returns non-NULL or it has been applied to all
 *          the key-value pairs. The callback might be passed NULL
 *          as either the key or the value. The hash remembers the
 *          position of the previous call to hash_foreach, a subsequent
 *          call will carry onto the next element, unless there are
 *          no more elements left in the hash.
 *  @param  h      table to apply func on
 *  @param  func   function to apply
 *  @return void*  the result of the first non-NULL function application**/
LIBLISP_API void *hash_foreach(hash_table_t *h, hash_func func);

/** @brief This resets a hash foreach function to the beginning of the
 *         table, so new calls to hash_foreach begin at the first element
 *         in the table.
 *  @param h hash table to reset foreach loop in**/
LIBLISP_API void hash_reset_foreach(hash_table_t *h);

/** @brief   print out the key-value pairs in a table, keys are
 *           printed as strings, values as the pointer value
 *  @param   h table to print out**/
LIBLISP_API void hash_print(hash_table_t *h);

/** @brief  Get the load factor of a hash (or number of entries / number of
 *          buckets). Depending on the internals this might go over one.
 *  @param  h      hash to query
 *  @return double load factor **/
LIBLISP_API double hash_get_load_factor(hash_table_t *h);

/** @brief  Get the number of records that have collided.
 *  @param  h      hash to query
 *  @return size_t number of record collisions **/
LIBLISP_API size_t hash_get_collision_count(hash_table_t *h);

/** @brief  Get the number of replacements
 *  @param  h      hash to query
 *  @return size_t number of replacements **/
LIBLISP_API size_t hash_get_replacements(hash_table_t *h);

/** @brief  Get the current number of bins in a hash
 *  @param  h     hash to query
 *  @return size_t number of bins **/
LIBLISP_API size_t hash_get_number_of_bins(hash_table_t *h);

/******************* I/O for reading/writing to strings or files *************/

/* A generic set of functions for reading and writing to files, but also
 * to other things like strings, NULL devices, etcetera. It is more limited
 * than the stdio library when operating on files. For example the IO channel
 * or ports used here can only be either outputs or inputs, never both*/

/** @brief  check whether IO channel is input channel
 *  @param  i   IO channel to check
 *  @return int non zero if channel is input channel**/
LIBLISP_API int io_is_in(io_t *i);

/** @brief  check whether IO channel is an output channel
 *  @param  o   IO channel to check
 *  @return int non zero if channel is output channel**/
LIBLISP_API int io_is_out(io_t *o);

/** @brief  check whether IO channel is using a file
 *  @param  f   IO channel to check
 *  @return int non zero if channel is using a file for io_t **/
LIBLISP_API int io_is_file(io_t *f);

/** @brief  check whether IO channel is using a string
 *  @param  s   IO channel to check
 *  @return int non zero if channel is using a string for io_t **/
LIBLISP_API int io_is_string(io_t *s);

/** @brief  check whether IO channel is using a null IO device
 *  @param  n   IO channel to check
 *  @return int non zero if channel is using a null IO device for io_t **/
LIBLISP_API int io_is_null(io_t *n);

/** @brief  get a char from I/O stream
 *  @param  i   I/O stream, must be readable
 *  @return int same as getc, char on success EOF on failure**/
LIBLISP_API int io_getc(io_t *i);

/** @brief Get the string from an I/O port, if a port is of an
 *         output type if might have reallocated its string, so
 *         this function should be used after each call that could have
 *         modified the internal pointer (that is any write to that
 *         string, output port).
 *  @param  x     I/O port, of string type (asserts x && io_is_string(x))
 *  @return char* internal string**/
LIBLISP_API char *io_get_string(io_t *x);

/** @brief  Get the internal file handle used by a file I/O port.
 *  @param  x     I/O port, of a file type (asserts x && io_is_file(x))
 *  @return FILE* internal file handle **/
LIBLISP_API FILE *io_get_file(io_t *x);  

/** @brief  single character of put-back into I/O stream
 *  @param  c   character to put back
 *  @param  i   I/O stream, must be set up for reading
 *  @return int same character as you put in, EOF on failure**/
LIBLISP_API int io_ungetc(char c, io_t *i);

/** @brief  write a single char to an I/O stream
 *  @param  c    character to write
 *  @param  o    I/O stream, must be set up for writing
 *  @return int  same character as you put in, EOF on failure**/
LIBLISP_API int io_putc(char c, io_t *o);

/** @brief  write a string (NUL delimited) to an I/O stream
 *  @param  s    string to write
 *  @param  o    I/O stream, must be setup for writing
 *  @return int  >= 0 is success, < 0 is failure**/
LIBLISP_API int io_puts(const char *s, io_t *o);

/** @brief read a block of data from an I/O stream 
 *  @param  ptr     a point to a block of memory size bytes long at
 *                  minimum
 *  @param  size    size of ptr array
 *  @param  i       stream to read from
 *  @return size_t number of member read in */
LIBLISP_API size_t io_read(char *ptr, size_t size, io_t *i);

/** @brief write a block of data to an I/O stream 
 *  @param  ptr     a point to a block of memory size bytes long at
 *                  minimum
 *  @param  size    size of ptr array
 *  @param  o       stream to write to
 *  @return size_t number of members written */
LIBLISP_API size_t io_write(char *ptr, size_t size, io_t *o);

/** @brief  get a line from an I/O stream
 *  @param  i   I/O stream, must be set up for reading
 *  @return char* a line of input, minus the newline character**/
LIBLISP_API char *io_getline(io_t *i);

/** @brief  get a record delimited by 'delim' from an I/O stream
 *  @param  i      I/O stream, must be set up for reading
 *  @param  delim  A character delimiter, can be EOF
 *  @return char*  A character delimited record**/
LIBLISP_API char *io_getdelim(io_t *i, int delim);

/** @brief  print an integer to an I/O stream (base 10)
 *  @param  d   an integer!
 *  @param  o   I/O stream, must be set up for writing
 *  @return int 0 on success, EOF on failure**/
LIBLISP_API int io_printd(intptr_t d, io_t *o);


/** @brief  print a floating point number to an I/O stream
 *  @param  f   a floating point number
 *  @param  o   I/O stream, must be set up for writing
 *  @return int 0 on success, EOF on failure**/
LIBLISP_API int io_printflt(double f, io_t * o);

/** @brief   read from a block of data, the block will be duplicated
 *  @param   sin block to read from
 *  @param   len length of block
 *  @return  io* an initialized I/O stream (for reading) or NULL**/
LIBLISP_API io_t *io_sin(const char *sin, size_t len);

/** @brief  read from a file
 *  @param  fin an already opened file handle, opened with "r" or "rb"
 *  @return io_t* an initialized I/O stream (for reading) of NULL**/
LIBLISP_API io_t *io_fin(FILE *fin);

/** @brief  write to a string
 *  @param  len  length of string
 *  @return io_t*  an initialized I/O stream (for writing) or NULL**/
LIBLISP_API io_t *io_sout(size_t len);

/** @brief  write to a file
 *  @param  fout an already opened file handle, opened with "w" or "wb"
 *  @return io_t*  an initialized I/O stream (for writing) or NULL**/
LIBLISP_API io_t *io_fout(FILE *fout);

/** @brief  return a null output device, output goes no where
 *  @return a null output port**/
LIBLISP_API io_t *io_nout(void);

/** @brief  close a file, the stdin, stderr and stdout file streams
 *          will not be closed if associated with this I/O stream
 *  @param  close I/O stream to close
 *  @return int   0 on success, EOF on error**/
LIBLISP_API int io_close(io_t *close);

/** @brief  test whether an I/O stream is at the end of the file
 *  @param  f    I/O stream to test
 *  @return int  !0 if EOF marker is set, 0 otherwise**/
LIBLISP_API int io_eof(io_t *f);

/** @brief  flush an I/O stream
 *  @param  f    I/O stream to flush
 *  @return int  EOF on failure, 0 otherwise**/
LIBLISP_API int io_flush(io_t *f);

/** @brief  return the file position indicator of an I/O stream
 *  @param  f    I/O stream to get position from
 *  @return int  less than zero on failure, file position otherwise**/
LIBLISP_API long io_tell(io_t *f);

/** @brief  set the position indicator of an I/O stream
 *  @param  f       I/O stream to set position
 *  @param  offset  offset to add to origin
 *  @param  origin  origin to do set the offset relative to
 *  @return int     -1 on error, 0 otherwise **/
LIBLISP_API int io_seek(io_t *f, long offset, int origin);

/** @brief  Test whether an error occurred with an I/O stream
 *  @param  f    I/O error to test
 *  @return int  non zero on error**/
LIBLISP_API int io_error(io_t *f);

/** @brief turn on colorization for output channel, non of the
 *         io primitives use this (such as io_putc, io_puts, etc),
 *         but other functions that have access to the internals
 *         do, such as the lisp interpreter.
 *  @param out      output channel to set colorize on
 *  @param color_on non zero for true (color on), zero for false**/
LIBLISP_API void io_color(io_t *out, int color_on);

/** @brief turn on colorization for output channel, non of the
 *         io primitives use this (such as io_putc, io_puts, etc),
 *         but other functions that have access to the internals
 *         do, such as the lisp interpreter.
 *  @param out       output channel to set pretty printing on
 *  @param pretty_on non zero for true (pretty printing on),
 *                   zero for false**/
LIBLISP_API void io_pretty(io_t *out, int pretty_on);

/********************* translate or delete characters ************************/

/** @brief  create a new tr state for use in character translation
 *  @return a new tr state or NULL**/
LIBLISP_API tr_state_t *tr_new(void);

/** @brief destroy a tr state
 *  @param st tr state to delete**/
LIBLISP_API void tr_delete(tr_state_t *st);

/** @brief initialize a translation state
 *
 *  The initialization routine does the work of deciding what characters
 *  should be mapped to, or deleted, from one block to another. Valid mode
 *  characters include the following:
 *
 *      ''  (empty string) normal translation
 *      'x' normal translation
 *      'c' compliment set 1
 *      's' squeeze repeated character run
 *      'd' delete characters (s2 should be NULL)
 *      't' truncate set 1 to the size of set 2
 *
 *  @param  tr   tr state to initialize
 *  @param  mode translate mode
 *  @param  s1   character set 1
 *  @param  s2   character set 2
 *  @return int  negative on failure, zero on success **/
LIBLISP_API int tr_init(tr_state_t *tr, char *mode, uint8_t *s1, uint8_t *s2);

/** @brief translate a single character given a translation state
 *  @param  tr  translation state
 *  @param  c   character to translate
 *  @return int negative if the character should be deleted, greater or equal to
 *              zero is the new character **/
LIBLISP_API int tr_char(tr_state_t *tr, uint8_t c);

/** @brief translate a block of memory given an initialized tr state
 *  @param  tr     tr state
 *  @param  in     input buffer
 *  @param  out    output buffer
 *  @param  len    length of both the input and output buffer
 *  @return size_t number of characters written to the output buffer**/
LIBLISP_API size_t tr_block(tr_state_t *tr, uint8_t *in, uint8_t *out, size_t len);

/******************* functions for manipulating lisp lisp_cell_t ********************/

/* This module is used by the internals of the lisp interpreter
 * for manipulating lisp cells; for accessing them, creating new
 * cells and testing their type. */

LIBLISP_API lisp_cell_t *car(lisp_cell_t *con); /**< get car cell from cons*/
LIBLISP_API lisp_cell_t *cdr(lisp_cell_t *con); /**< get cdr cell from cons*/

/**@brief Set the first value in a cons cell. Unlike most functions
 *        this mutates data. The first cell is called the "car" cell
 *        for historical reasons.
 * @param con Cons Cell modify
 * @param val Value to put into car of cons cell, this should be
 *            another lisp cell.**/
LIBLISP_API void set_car(lisp_cell_t *con, lisp_cell_t *val);

/**@brief set cdr cell of a cons cell
 * @param con Cons Cell to modify
 * @param val Value to put into the second cell of a cons cell, this
 *            should be another lisp cell.
 * @warning set cdr does not maintain the internal list length in each
 *          cons cell node correctly.*/
LIBLISP_API void set_cdr(lisp_cell_t *con, lisp_cell_t *val);

/**@brief Given a lisp cell, check that its length field equals an
 *        expected value.
 * @param x      cell to check
 * @param expect expected value
 * @return int !0 if they match, 0 if they do not**/
LIBLISP_API int  lisp_check_length(lisp_cell_t *x, size_t expect);

/**@brief close a cell (io and userdef only)
 * @param x close a cell, invalidating it for further use and freeing any
 *          resources associated with it. */
LIBLISP_API void close_cell(lisp_cell_t *x);

/**@brief  Create a new cons cell from a pair of values, this primitive
 *         is used to construct the basic data structures (lists and trees)
 *         that make lisp, lisp. 
 * @param  l lisp environment used for error handling
 * @param  x lisp cell to become the first, or "car", value of a cons cell.
 * @param  y lisp cell to become the second, or "cdr", value of a cons cell.
 * @return lisp_cell_t* new cons cell. */
LIBLISP_API lisp_cell_t *cons(lisp_t *l, lisp_cell_t *x, lisp_cell_t *y);

/**@brief  extend an environment with a symbol-value pair 
 * @param  l   initialized lisp structure, used for error handling.
 * @param  env a lisp environment (an association list).
 * @param  sym symbol to associate with a value
 * @param  val value to associate with a symbol
 * @return  lisp_cell_t* Non-NULL on success, NULL on failure */
LIBLISP_API lisp_cell_t *lisp_extend(lisp_t *l, lisp_cell_t *env, lisp_cell_t *sym, lisp_cell_t *val);

/**@brief  add a new symbol to the list of all symbols, two interned
 *         symbols containing the same name will compare equal with
 *         a pointer comparison, they will be the same object. The
 *         lisp environment will try to free this name!
 * @param  l    an initialized lisp structure, used for error handling
 *              and keeping track of interned symbols
 * @param  name name of symbol
 * @return lisp_cell_t* a unique symbol cell*/
LIBLISP_API lisp_cell_t *lisp_intern(lisp_t *l, char *name);

/**@brief  true if 'x' is equal to nil
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_nil(lisp_cell_t *x);

/**@brief  true if 'x' is a integer 
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_int(lisp_cell_t *x);

/**@brief  true if 'x' is a floating point number
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_floating(lisp_cell_t *x);

/**@brief  true if 'x' is a cons cell
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_cons(lisp_cell_t *x);

/**@brief  true if 'x' is not a dotted pair cons cell
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_proper_cons(lisp_cell_t *x);

/**@brief  true if 'x' is a I/O type
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_io(lisp_cell_t *x);

/**@brief  true if 'x' is a lambda procedure
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_proc(lisp_cell_t *x);

/**@brief  true if 'x' is a flambda procedure
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_fproc(lisp_cell_t *x);

/**@brief  true if 'x' is a string
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_str(lisp_cell_t *x);

/**@brief  true if 'x' is a symbol
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_sym(lisp_cell_t *x);

/**@brief  true if 'x' is a language primitive
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_subr(lisp_cell_t *x);

/**@brief  true if 'x' is a ASCII nul delimited string
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_asciiz(lisp_cell_t *x);

/**@brief  true if 'x' is integer or float
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_arith(lisp_cell_t *x);

/**@brief  true if 'x' is an input I/O type
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_in(lisp_cell_t *x);

/**@brief  true if 'x' is an output I/O type
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_out(lisp_cell_t *x);

/**@brief  true if 'x' is a hash table type
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_hash(lisp_cell_t *x);

/**@brief  true if 'x' is a user defined type
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_userdef(lisp_cell_t *x);

/**@brief Is 'x' a specific user defined type? For this to be true
 *        'x' has to be both a user defined type. 
 * @param  x    value to perform check on
 * @param  type this is a number that represents a user defined type,
 *              it should have been assigned by "new_user_defined_type"
 * @return int zero if check fails, non zero if check passes  */
LIBLISP_API int  is_usertype(lisp_cell_t *x, int type);

/**@brief  true if 'x' can be applied (is a function) 
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes  */
LIBLISP_API int  is_func(lisp_cell_t *x);

/**@brief  true if 'x' is 'closed' or invalidated
 * @param  x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_closed(lisp_cell_t *x);

/**@brief  true if 'x' is a proper list
 * @return x   value to perform check on
 * @return int zero if check fails, non zero if check passes */
LIBLISP_API int  is_list(lisp_cell_t *x);

/**@brief Make a proper list (terminate args with NULL) from a series
 *        of list cells.
 * @param  l  initialized lisp structure
 * @param  x   first cell, there must be at least one argument
 * @param  ... a list of arguments, each of them lisp cells, terminated
 *             with a NULL
 * @return lisp_cell_t* a new list of values */
LIBLISP_API lisp_cell_t *mk_list(lisp_t *l, lisp_cell_t *x, ...);

/**@brief  Make a lisp cell from an integer
 * @param  l lisp environment for error handling and garbage collection
 * @param  d value of new integer
 * @return lisp_cell_t* new integer lisp lisp_cell_t */
LIBLISP_API lisp_cell_t *mk_int(lisp_t *l, intptr_t d);

/**@brief  Make a lisp cell from a float
 * @param  l lisp environment for error handling and garbage collection
 * @param  f value of new float
 * @return lisp_cell_t* new float lisp lisp_cell_t */
LIBLISP_API lisp_cell_t *mk_float(lisp_t *l, lisp_float_t f);

/**@brief  Make lisp cell from an I/O stream
 * @param  l lisp environment for error handling and garbage collection
 * @param  x I/O stream to add
 * @return lisp_cell_t* new I/O lisp lisp_cell_t */
LIBLISP_API lisp_cell_t *mk_io(lisp_t *l, io_t *x);

/**@brief  Subroutine creation function. This allows the interpreter
 *         to be extended with arbitrary lisp functions
 * @param  l    lisp environment for error handling and garbage collection
 * @param  p    function to make into a lisp subroutine usable by the
 *              interpreter, the function should be of the "subr" type,
 *              accepting a lisp* and an argument list.
 * @param  fmt  format of function validation string, the can be NULL if
 *              the function is to perform its own checking.
 * @param  doc  documentation string, a string containing information about
 *              the subroutine, this can be NULL.
 * @return lisp_cell_t* returns a new lisp subroutine object */
LIBLISP_API lisp_cell_t *mk_subr(lisp_t *l, lisp_subr_func p, const char *fmt, const char *doc);

/**@brief  make a lisp lambda procedure cell.
 * @param  l    lisp environment for error handling and garbage collection
 * @param  args a list of argument names to the function
 * @param  code the code of the procedure
 * @param  env  the environment the procedure should execute in, this should
 *              be an association list
 * @param  doc  The documentation string for the lambda
 * @return lisp_cell_t* a new procedure lisp_cell_t */
LIBLISP_API lisp_cell_t *mk_proc(lisp_t *l, lisp_cell_t *args, lisp_cell_t *code, lisp_cell_t *env, lisp_cell_t *doc);

/**@brief  make a lisp F-expression cell, F-expressions are like lambdas except
 *         they do not evaluate their arguments and bind all their arguments to
 *         a single variable as a list.
 * @param  l    lisp environment for error handling and garbage collection
 * @param  args a list of one argument name for the function
 * @param  code the code of the F-expression
 * @param  env  the environment in which to execute the F-expression in
 * @param  doc  the documentation string for the F-expression
 * @return lisp_cell_t* a new f-expression */
LIBLISP_API lisp_cell_t *mk_fproc(lisp_t *l, lisp_cell_t *args, lisp_cell_t *code, lisp_cell_t *env, lisp_cell_t *doc);

/**@brief  make lisp cell (string) from a string
 * @param  l lisp environment for error handling and garbage collection
 * @param  s a string, the lisp interpreter *will* try to free this
 *           string when it decides to collect this cell. 
 * @return lisp_cell_t* a new lisp cell containing a string */
LIBLISP_API lisp_cell_t *mk_str(lisp_t *l, char *s);

/**@brief  make lisp cell (string) from a string
 * @param  l lisp environment for error handling and garbage collection
 * @param  s a string, the lisp interpreter *will not* try to free this
 *           string at all.
 * @return lisp_cell_t* a new lisp cell containing a string */
LIBLISP_API lisp_cell_t *mk_immutable_str(lisp_t *l, const char *s);

/*LIBLISP_API lisp_cell_t *mksym(lisp_t *l, char *s); // use intern instead to get a unique symbol*/

/**@brief  make lisp cell from hash
 * @param  l lisp environment for error handling and garbage collection
 * @param  h an allocated hash table
 * @return lisp_cell_t* a hash table accessible from a lisp interpreter */
LIBLISP_API lisp_cell_t *mk_hash(lisp_t *l, hash_table_t *h);

/**@brief  make a user defined type
 * @param  l lisp environment for error handling and garbage collection
 * @param  x    data field for the new user defined type
 * @param  type the specific type to check, there is no way for this
 *              function to check whether the data passed in is valid.
 * @return lisp_cell_t* */
LIBLISP_API lisp_cell_t *mk_user(lisp_t *l, void *x, intptr_t type);

/**@brief  Get lisp length, this will get the length field of any object,
 *         even if it does not make sense. The length field is currently
 *         meaningless for integer and floating point types, but could be used
 *         in the future to implement arrays of those types. For strings it
 *         contains the length of that string, for functions it contains the
 *         expected argument length, for lists it contains the length of that
 *         list.
 * @param  x        lisp cell to get the length field from
 * @return unsigned the contents of the internal length field */
LIBLISP_API unsigned get_length(lisp_cell_t *x);

/**@brief  Get raw lisp value 
 * @param  x    This can be of any lisp cell type.
 * @return void*  It will always return the first value of the lisp cell, 
 *              regardless of how much other data is in the lisp object. This
 *              could be NULL, or anything that can fit into void*  */
LIBLISP_API void  *get_raw(lisp_cell_t *x);

/**@brief  cast *any* lisp cell to integer
 * @param  x   This can be of any lisp cell type, but finds most use with
 *             lisp integer types
 * @return intptr_t an intptr_t of whatever was in the first cell of a lisp
 *             object, for a lisp integer object, it will be the integer it
 *             contains, for any other type it could be anything.*/
LIBLISP_API intptr_t get_int(lisp_cell_t *x);

/**@brief  get lisp cell to I/O stream
 * @param  x   'x' must be a lisp object of an I/O port type
 * @return io_t* an input or an output port*/
LIBLISP_API io_t *get_io(lisp_cell_t *x);

/**@brief  get a lisp cell to a primitive func ptr
 * @param  x    'x' must be a lisp object of a subroutine type
 * @return subr an internal lisp subroutine function*/
LIBLISP_API lisp_subr_func get_subr(lisp_cell_t *x);

/**@brief  get args to a procedure/f-expr 
 * @param  x     'x' must be a lisp object of a lambda or f-expression type
 * @return lisp_cell_t* the arguments list to a lambda or f-expression procedure*/
LIBLISP_API lisp_cell_t *get_proc_args(lisp_cell_t *x);

/**@brief  get code from a procedure/f-expr 
 * @param  x
 * @return lisp_cell_t* */
LIBLISP_API lisp_cell_t *get_proc_code(lisp_cell_t *x);

/**@brief  get procedure/f-expr environment
 * @param  x
 * @return lisp_cell_t* */
LIBLISP_API lisp_cell_t *get_proc_env(lisp_cell_t *x);

/**@brief  get the documentation string for a lambda/f-expr/subroutine
 * @param  x
 * @return lisp_cell_t* */
LIBLISP_API lisp_cell_t *get_func_docstring(lisp_cell_t *x);

/**@brief  get the type validation string for lambda/f-expr/subroutine
 * @param  x
 * @return char * */
LIBLISP_API char *get_func_format(lisp_cell_t *x);

/**@brief  get the user-defined-value identifier
 * @param  x
 * @return int */
LIBLISP_API int   get_user_type(lisp_cell_t *x);

/**@brief  get string from a lisp cell
 * @param  x
 * @return char* */
LIBLISP_API char *get_str(lisp_cell_t *x);

/**@brief  get string (symbol) from a lisp cell
 * @param  x
 * @return char* */
LIBLISP_API char *get_sym(lisp_cell_t *x);

/**@brief  get data from user defined type
 * @param  x
 * @return void* */
LIBLISP_API void *get_user(lisp_cell_t *x);

/**@brief  get floating point val from lisp cell
 * @param  x
 * @return lisp_float_t */
LIBLISP_API lisp_float_t get_float(lisp_cell_t *x);

/**@brief  get hash table from a lisp cell
 * @param  x
 * @return hash_table_t* */
LIBLISP_API hash_table_t *get_hash(lisp_cell_t *x);

/**@brief  float/int (arithmetic type) to int 
 * @param  x float or integer
 * @return intptr_t integer */
LIBLISP_API intptr_t get_a2i(lisp_cell_t *x);

/**@brief  float/int (arithmetic type) to float 
 * @param  x float or integer
 * @return lisp_float_t float */
LIBLISP_API lisp_float_t get_a2f(lisp_cell_t *x);

/**@brief  return the "nil" symbol
 * @return lisp_cell_t* The "nil" lisp_cell_t */
LIBLISP_API lisp_cell_t *gsym_nil(void);

/**@brief  return the "t" symbol
 * @return lisp_cell_t* The "t", or true, lisp_cell_t */
LIBLISP_API lisp_cell_t *gsym_tee(void);

/**@brief  return the "quote" symbol 
 * @return lisp_cell_t* The special "quote" symbol, used for quoting expressions */
LIBLISP_API lisp_cell_t *gsym_quote(void);

/**@brief  return the "error" symbol
 * @return lisp_cell_t* The special "error" symbol, used for signifying that an error
 * has occurred during evaluation */
LIBLISP_API lisp_cell_t *gsym_error(void);

/**@brief  return the "if" symbol
 * @return lisp_cell_t* The special "if" symbol, used for if expressions */
LIBLISP_API lisp_cell_t *gsym_iif(void);

/**@brief  return the "lambda" symbol
 * @return lisp_cell_t* The special lambda symbol, used for lambda expressions (functions)*/
LIBLISP_API lisp_cell_t *gsym_lambda(void);

/**@brief  return the "flambda" symbol
 * @return lisp_cell_t* The special "flambda" symbol, used for F-Expressions*/
LIBLISP_API lisp_cell_t *gsym_flambda(void);

/**@brief  return the "define" symbol
 * @return lisp_cell_t* The special "define" symbol, used for adding symbol-value pairs
 * to the global environment */
LIBLISP_API lisp_cell_t *gsym_define(void);

/**@brief  return the "setq" symbol
 * @return lisp_cell_t* The special "setq" symbol, used for binding a symbol to a new
 * variable */
LIBLISP_API lisp_cell_t *gsym_setq(void);

/**@brief  return the progn symbol
 * @return lisp_cell_t* The special "progn" symbol, used for evaluating expressions in
 * order */
LIBLISP_API lisp_cell_t *gsym_progn(void);

/**@brief  return the "cond" symbol
 * @return lisp_cell_t* The special "cond" symbol */
LIBLISP_API lisp_cell_t *gsym_cond(void);

/**@brief  return the "let" symbol
 * @return lisp_cell_t* The special "let" symbol, used for creating local variables
 * and evaluating expressions in the modified local environment */
LIBLISP_API lisp_cell_t *gsym_let(void);

/**@brief  return the "compile" symbol
 * @return lisp_cell_t* The special "compile" symbol, */
LIBLISP_API lisp_cell_t *gsym_compile(void);

/**@brief  return the "macro" symbol
 * @return lisp_cell_t* The special "macro" symbol, */
LIBLISP_API lisp_cell_t *gsym_macro(void);

/**@brief  return the "while" symbol
 * @return lisp_cell_t* The special "while" symbol, */
LIBLISP_API lisp_cell_t *gsym_dowhile(void);

/**@brief  return a new token representing a new type
 * @param  l lisp environment to put the new type in
 * @param  f function to call when freeing type, optional (but free() will be used)
 * @param  m function to call when marking type, optional
 * @param  e function to call when comparing two types, optional
 * @param  p function to call when printing type, optional
 * @return int return -1 if there are no more tokens to give or a positive
 *                number representing a user defined token*/
LIBLISP_API int new_user_defined_type(lisp_t *l, lisp_free_func f, lisp_mark_func m, lisp_equal_func e, lisp_print_func p);

/**@brief determines whether a string contains a number that
 *        can be converted with strtol.
 *        matches "(+|-)?(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]+)"
 * @param  buf ascii delimited string to check if it is a number
 * @return 0 if it is not a number, non zero if it is**/
LIBLISP_API int is_number(const char *buf);

/**@brief  this is much like is_number, but for floating point numbers
 *         matches "[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?", and
 *         excludes valid conversions added in c99 (such as "inf" or
 *         "nan"). This is not a test that a number is exclusively
 *         a floating point number, only that is can be converted into
 *         one sensible, to test for that use is_number() on the
 *         string first.
 * @param  buf ascii delimited string to check if it is a float
 * @return 0 if it is not a float, non zero if it is**/
LIBLISP_API int is_fnumber(const char *buf);

/*********** functions for implementing an interpreter environment ***********/

/* This module allows for reading, printing and evaluating of
 * S-expressions. The reader can be used as a generic way to parser
 * S-expressions, which is part of the intention of the library. */


/** @brief A method for throwing an exception in the lisp interpreter,
 *         this will call exit() if internally a jmp_buf has not been
 *         set for this throw to return to.
 *
 *  A positive number signals that the lisp interpreter can continue
 *  after processing this error, a negative number that it should exit
 *  to whatever called the interpreter. All of the lisp_* functions
 *  such as "lisp_eval" should internally set the jmp_buf to recover
 *  to.
 *
 *  The purpose of this function for the library user is so that they
 *  can call lisp_throw() from inside their user defined functions
 *  which can be added with lisp_add_subr().
 *
 *  @param l   a lisp environment to throw an exception in, if NULL
 *             this will call exit(ret) instead!
 *  @param ret a negative ret will halt the interpreter, a positive
 *             one signals an error occurred, but it can be recovered
 *             from**/
LIBLISP_API void lisp_throw(lisp_t *l, int ret);

/** @brief Allocate a new block of memory with calloc, this memory can 
 *         freed with free() from stdlib, this version of calloc will
 *         attempt to halt the interpreter if it fails.
 *  @param  l     an initialized lisp interpreter environment
 *  @param  size  number of bytes to allocate
 *  @return void* a new block of memory which can be freed with free() */
LIBLISP_API void* lisp_calloc(lisp_t *l, size_t size);

/** @brief Attempt to handle an out of memory condition within the lisp
 *         interpreter by throwing an exception (longjmp). 
 * @param l an initialized lisp interpreter environment **/
LIBLISP_API void lisp_out_of_memory(lisp_t *l);

/**@brief  Get the top level lisp environment
 * @param  l lisp session to get lisp environment from
 * @return lisp_cell_t* the top level lisp environment */
LIBLISP_API lisp_cell_t *lisp_environment(lisp_t *l);

/**@brief  This is a convince function that takes a pointer to an array of
 *         structures, the structures contain the information needed 
 *         for a call to lisp_add_subr().
 * @param  l   interpreter to add lisp routines to
 * @param  ms  array of structures containing routines to add to an interpreter
 * @param  len length of lisp_module_subroutines_t array, if this is zero then
 *             the list is NULL terminated.
 * @return int non zero on failure, zero on success */
LIBLISP_API int lisp_add_module_subroutines(lisp_t *l, const lisp_module_subroutines_t *ms, size_t len);

/** @brief This is like lstrdup_or_abort, but will call lisp_throw with
 *         a negative number for "ret", halting the lisp environment on
 *         error (or aborting if the error handler is not installed).
 *  @param l      lisp environment
 *  @param s      string to duplicate
 *  @return char* duplicated string, or throw an exception using lisp_throw
 *                on error */
LIBLISP_API char *lisp_strdup(lisp_t *l, const char *s);

/** @brief Perform a copy on lisp cells, some forms cannot be copied
 *         (eg. IO port types). The copy is recursive.
 *  @param l   initialized lisp environment
 *  @param src cells to copy
 *  @return lisp_cell_t* a copied lisp cell */
LIBLISP_API lisp_cell_t *lisp_copy(lisp_t *l, lisp_cell_t *src);

/** @brief Serialize a lisp S-Expression, turning it into a string
 *  @param  l      lisp environment
 *  @param  x      S-Expression to serialize
 *  @return char*  NULL on failure, a serialized S-Expression on success */
LIBLISP_API char *lisp_serialize(lisp_t *l, lisp_cell_t *x);

/** @brief Formatted printing of lisp cells, colorized if color enabled on
 *         output stream, it can print out s-expression trees generated by
 *         by "lisp_repl()" or "lisp_read()". The output of s-expressions
 *         trees can be pretty printed, the "depth" parameter specifies
 *         the indentation level.
 *
 *         The "fmt" format string acts like the "printf" format string,
 *
 *         '%'  print out a single escape char '%'
 *         '@'  print out "depth" many of the next character
 *         'c'  print out a single (c)haracter
 *         's'  print out a (s)tring
 *         'S'  print out a (S)-expression
 *
 *         Color/Terminal control options. At the moment they assume that
 *         your terminal can handle ANSI escape codes, which is nearly
 *         every single terminal.
 *
 *         't'  rest (t)erminal color to default
 *         'B'  (B)old text
 *         'v'  reverse (v)ideo
 *         'k'  blac(k)
 *         'r'  (r)ed
 *         'g'  (g)reen
 *         'y'  (y)ellow
 *         'b'  (b)lue
 *         'm'  (m)agenta
 *         'a'  cy(a)n
 *         'w'  (w)hite
 *
 *  @param  l      an initialized lisp environment, this can be NULL but
 *                 maximum depth will not be checked, nor will user defined
 *                 functions be printed out with their user defined printing
 *                 functions for that environment.
 *  @param  o      output I/O stream to print to
 *  @param  depth  indentation level of any pretty printing
 *  @param  fmt    format string
 *  @param  ...    any arguments
 *  @return int    >= 0 on success, less than 0 on failure */
LIBLISP_API int lisp_printf(lisp_t *l, io_t *o, unsigned depth, char *fmt, ...);

/** @brief This function is behaves the same as lisp_printf, but it takes
 *         an argument list instead.
 *  @param  l      an initialized lisp environment, this can be NULL but
 *                 maximum depth will not be checked, nor will user defined
 *                 functions be printed out with their user defined printing
 *                 functions for that environment.
 *  @param  o      output I/O stream to print to
 *  @param  depth  indentation level of any pretty printing
 *  @param  fmt    format string
 *  @param  ap     an argument list generated by va_start
 *  @return int    >=0 on success, less than 0 on failure */
LIBLISP_API int lisp_vprintf(lisp_t *l, io_t *o, unsigned depth, char *fmt, va_list ap);

/** @brief  return a hash-symbol containing all the interned symbols
 *          in a lisp environment, these are not defined symbols, but
 *          all symbols encountered (possibly as data or in error). The
 *          hash contains the symbol name for the key value, which points
 *          to a lisp symbol cell containing the same string.
 *  @param  l     lisp environment to get symbols from
 *  @return lisp_cell_t* hash-symbol of all interned symbols*/
LIBLISP_API lisp_cell_t *lisp_get_all_symbols(lisp_t *l);

/** @brief  add a symbol-val pair and intern a lisp cell
 *  @param  l    lisp environment to add pair to
 *  @param  sym  name of symbol
 *  @param  val  value to add
 *  @return NULL on failure, not NULL on success **/
LIBLISP_API lisp_cell_t *lisp_add_cell(lisp_t *l, const char *sym, lisp_cell_t *val);

/** @brief  add a function primitive to a lisp environment. It will be
 *          referenced internally by the "name" string. This is the long
 *          version, a format string and a documentation string can be added
 *          to the subroutine.
 *  @param  l     lisp environment to add primitive to
 *  @param  name  name to call the function primitive by
 *  @param  func  function primitive
 *  @param  fmt   format string that is passed to lisp_validate_args (can be NULL)
 *  @param  doc   documentation string (can be NULL)
 *  @return lisp_cell_t* pointer to extended environment if successful, NULL
 *                otherwise. You shouldn't do anything with pointer**/
LIBLISP_API lisp_cell_t *lisp_add_subr(lisp_t *l, const char *name, lisp_subr_func func, const char *fmt, const char *doc);

/** @brief  Initialize a lisp environment. By default it will read
 *          from stdin, print to stdout and log errors to stderr.
 *  @return lisp*    A fully initialized lisp environment or NULL**/
LIBLISP_API lisp_t *lisp_init(void);

/** @brief  read in a s-expression, it uses the lisp environment
 *          for error handling, garbage collection and finding
 *          duplicate symbols
 *  @param  l     an initialized lisp environment
 *  @param  i     I/O stream to read from
 *  @return lisp_cell_t* a parsed s-expression or NULL on failure**/
LIBLISP_API lisp_cell_t *lisp_read(lisp_t *l, io_t *i);

/** @brief  print out an s-expression
 *  @param  l    a lisp environment which contains the IO stream to print to
 *  @param  ob   cell to print
 *  @return int  0 >= on success, less than 0 on failure**/
LIBLISP_API int lisp_print(lisp_t *l, lisp_cell_t *ob);

/** @brief  evaluate a lisp expression
 *  @param  l     a initialized lisp environment to evaluate against
 *  @param  exp   an expression to evaluate
 *  @return lisp_cell_t* a lisp expression to print out, or NULL**/
LIBLISP_API lisp_cell_t *lisp_eval(lisp_t *l, lisp_cell_t *exp);

/** @brief  parse and evaluate a string, returning the result, it will
 *          however discard any input after the first evaluation.
 *
 *          For example:
 *          (+ 2 2)
 *          will be evaluated to an integer cell containing 4.
 *
 *          Whereas:
 *          (+ 2 2) (+ 3 3)
 *          The first expression will be evaluated, the second one "(+ 3 3)"
 *          will not be.
 *
 *          The same goes for atoms:
 *          2 "Hello World"
 *          Will return 2, "Hello World" will not even be evaluated.
 *
 *          To evaluate a list of expressions you could use either:
 *          (list (expr) (expr) ... (expr))
 *          Or use lisp_repl.
 *
 *  @param  l       lisp environment to evaluate in
 *  @param  evalme  string to evaluate
 *  @return lisp_cell_t*   result of evaluation or NULL on failure critical failure**/
LIBLISP_API lisp_cell_t *lisp_eval_string(lisp_t *l, const char *evalme);

/** @brief  a simple Read-Evaluate-Print-Loop (REPL)
 *  @param  l      an initialized lisp environment
 *  @param  prompt a  prompt to print out, use the empty string for no prompt
 *  @param  editor_on if a line editor has been provided with
 *                    lisp_set_line_editor then this turns it on (on is non
 *                    zero).
 *  @return int    return non zero on error, negative on fatal error**/
LIBLISP_API int lisp_repl(lisp_t *l, char *prompt, int editor_on);

/** @brief destroy a lisp environment
 *  @param l       lisp environment to destroy**/
LIBLISP_API void lisp_destroy(lisp_t *l);

/** @brief log an error, using the same format as lisp_printf, requires
 * log level to be greater or equal to LISP_LOG_LEVEL_ERROR, otherwise
 * the message will not be logged.
 *  @param  l   an initialized lisp environment
 *  @param  fmt format string
 *  @return int >=0 on success, less than 0 on failure  */
LIBLISP_API int lisp_log_error(lisp_t *l, char *fmt, ...);

/** @brief log a note, using the same format as lisp_printf, requires
 * log level to be greater or equal to LISP_LOG_LEVEL_NOTE, otherwise
 * the message will not be logged.
 *  @param  l   an initialized lisp environment
 *  @param  fmt format string
 *  @return int >=0 on success, less than 0 on failure  */
LIBLISP_API int lisp_log_note(lisp_t *l, char *fmt, ...);

/** @brief log a debug message, using the same format as lisp_printf, requires
 * log level to be greater or equal to LISP_LOG_LEVEL_DEBUG, otherwise
 * the message will not be logged. Setting LISP_LOG_LEVEL_DEBUG can produce
 * very verbose output.
 *  @param  l   an initialized lisp environment
 *  @param  fmt format string
 *  @return int >=0 on success, less than 0 on failure  */
LIBLISP_API int lisp_log_debug(lisp_t *l, char *fmt, ...);

/** @brief set the input for a lisp environment
 *  @param  l  an initialized lisp environment
 *  @param  in the input port
 *  @return int zero on success, non zero otherwise**/
LIBLISP_API int lisp_set_input(lisp_t *l, io_t *in);

/** @brief set the output for a lisp environment
 *  @param  l   an initialized lisp environment
 *  @param  out the output port
 *  @return int zero on success, non zero otherwise**/
LIBLISP_API int lisp_set_output(lisp_t *l, io_t *out);

/** @brief set the output for error logging for a lisp environment
 *  @param l       an initialized lisp environment
 *  @param logging the output port for error logging
 *  @return int    zero on success, non zero otherwise**/
LIBLISP_API int lisp_set_logging(lisp_t *l, io_t *logging);

/** @brief  set the line editor to be used with lisp_repl, main_lisp
 *          and main_lisp_env both call lisp_repl behind the scenes,
 *          and will enable line editor functionality when reading from
 *          stdin and passed the right option. The editor_func accepts
 *          a string to print as the prompt and returns a line of text
 *          that has been read in.
 *  @param  l      an initialized lisp environment
 *  @param  ed     the line editor function**/
LIBLISP_API void lisp_set_line_editor(lisp_t *l, lisp_editor_func ed);

/** @brief  set the internal signal handling variable of a lisp environment,
 *          this is a way for a function such as a signal handler or another
 *          thread to halt the interpreter.
 *  @param  l   an initialized lisp environment
 *  @param  sig signal value, any non zero value halts the lisp environment**/
LIBLISP_API void lisp_set_signal(lisp_t *l, int sig);

/** @brief get the input channel in use in a lisp environment
 *  @param  l lisp environment to retrieve input channel from
 *  @return io_t* pointer to input channel or NULL on failure**/
LIBLISP_API io_t *lisp_get_input(lisp_t *l);

/** @brief get the output channel in use in a lisp environment
 *  @param  l lisp environment to retrieve output channel from
 *  @return io_t* pointer to output channel or NULL on failure**/
LIBLISP_API io_t *lisp_get_output(lisp_t *l);

/** @brief get the logging/error channel in use in a lisp environment
 *  @param  l lisp environment to retrieve error channel from
 *  @return io_t* pointer to error channel or NULL on failure**/
LIBLISP_API io_t *lisp_get_logging(lisp_t *l);

/** @brief set the current log level of a lisp interpreter environment
 *  @param l     lisp environment to set the log level of
 *  @param level the level to set the environment to*/
LIBLISP_API void lisp_set_log_level(lisp_t *l, lisp_log_level level);

/** @brief get the current log level of a lisp interpreter environment
 *  @param l   lisp environment to get the log level of
 *  @return lisp_log_level the log level of the interpreter */
LIBLISP_API lisp_log_level lisp_get_log_level(lisp_t *l);

/** @brief validate an arguments list against a format string, this can either
 *         longjmp to an error handler if recover is non zero or return a
 *         integer (non zero if "args" type and length are correct).
 *
 *         The format string expected is as follows:
 *
 *            s  a symbol
 *            d  integer
 *            c  a cons cell
 *            p  procedure, defined with "lambda"
 *            r  subroutine, a built in subroutine
 *            S  a string
 *            P  io-port, either input or an output port
 *            h  a hash
 *            F  f-expression, defined with "flambda"
 *            f  floating point number
 *            u  a user-defined type
 *            b  t or nil
 *            i  input-port only
 *            o  output-port only
 *            Z  symbol or string (ASCIIZ string)
 *            a  integer or float
 *            x  function, (lambda, f-expression or subroutine)
 *            I  input-port or string
 *            l  any defined procedure (lambda or f-expression)
 *            C  symbol, string or integer
 *            A  any expression
 *
 *         The format is determined by the internal needs of the interpreter.
 *
 *  @param l       lisp environment for error handling
 *  @param msg     error message to print out (can be NULL)
 *  @param len     length of arguments list
 *  @param fmt     expected format of arguments
 *  @param args    argument cons list to validate against
 *  @param recover if non zero this will longjmp to an error handler instead
 *                 of returning.
 *  @return 0 if invalid (or lonjmp if recover is non zero), 1 if valid**/
LIBLISP_API int lisp_validate_args(lisp_t *l, const char *msg, unsigned len, const char *fmt, lisp_cell_t *args, int recover);

/** @brief  validate a function (subroutine, procedure or f-expression) against
 *          the information it carries around within itself, if this is not
 *          available the function returns 1 as if the validation had been
 *          successful. this can either longjmp to an error handler if recover
 *          is non zero or return a integer.
 *  @param l       lisp environment for error handling
 *  @param x       function to validate
 *  @param args    argument cons list to validate against
 *  @param recover if non zero this will longjmp to an error handler instead
 *                 of returning.
 *  @return 0 if invalid (or lonjmp if recover is non zero), 1 if valid***/
LIBLISP_API int lisp_validate_cell(lisp_t *l, lisp_cell_t *x, lisp_cell_t *args, int recover);

/** @brief Given a lisp object it will tell the garbage collector to never collect
 *         that object, ever. 
 *  @param x      object to mark as being not collectible */
LIBLISP_API void lisp_gc_used(lisp_cell_t *x);

/** @brief Given a lisp object it will tell the garbage collector it is free to
 *         collect that object whenever it wants to.
 *  @param x      object to mark as being collectible */
LIBLISP_API void lisp_gc_not_used(lisp_cell_t *x);

/**@brief mark a lisp cell (recursively) to prevent it being collected in
 *        the next collection cycle.
 * @param l  lisp environment, used for error handling and carrying around
 *           any functions added for marking user defined types.
 * @param op object to mark*/
LIBLISP_API void lisp_gc_mark(lisp_t *l, lisp_cell_t *op);

/**@brief Mark all reachable objects then perform a sweep. To prevent
 *        an object from being collected during the next garbage collection
 *        cycle, you can use lisp_gc_mark(). This will only prevent the objects
 *        collection on this cycle, not the cycle after this one.
 * @param l      the lisp environment to perform the mark and sweep in**/
LIBLISP_API void lisp_gc_mark_and_sweep(lisp_t *l);

/**@brief  Get the status of the garbage collector in a lisp environment,
 *         that is whether it is currently enabled. It defaults to being
 *         on.
 * @param  l   the lisp environment to check in
 * @return int non-zero if the garbage collector is enabled, zero if it
 *             is disabled. */
LIBLISP_API int  lisp_gc_status(lisp_t *l);

/**@brief Turn the garbage collector on, this will allow the interpreter to
 *        call lisp_gc_mark_and_sweep() and an indeterminate time in the future,
 *        when it has become necessary to do so.
 * @param l lisp environment to enable garbage collection in.*/
LIBLISP_API void lisp_gc_on(lisp_t *l);

/**@brief Turn off the garbage collector, objects can still be collected
 *        at a later time, but that time will be postponed indefinitely until
 *        lisp_gc_on() is called.
 * @param l lisp environment to disable garbage collection in*/
LIBLISP_API void lisp_gc_off(lisp_t *l);

/************************ test environment ***********************************/

/** @brief  A full lisp interpreter environment in a function call. It will
 *          read from stdin by default and print to stdout, but that is
 *          dependent on the arguments given to it. See the man pages (or
 *          source) for a full list of commands.
 *  @param  argc argument count
 *  @param  argv arguments to pass into interpreter
 *  @return int  return value of interpreter, 0 is normal termination**/
LIBLISP_API int main_lisp(int argc, char **argv);

/** @brief  This is like the main_lisp function, however you can pass in
 *          an initialized lisp environment, and before this
 *          add functionality to the environment you want
 *  @param  l     an initialized lisp environment
 *  @param  argc  argument count
 *  @param  argv  arguments to pass to interpreter
 *  @return int **/
LIBLISP_API int main_lisp_env(lisp_t *l, int argc, char **argv);

/*********************** some handy macros ***********************************/

#ifndef __cplusplus /* variable length macros not available in C++ */
/**@brief A wrapper around vstrcatsep so that the user does not have to
 *        terminate the sequence with a null. Like vstrcatsep is will
 *        concatenate a variable amount of strings with a separator
 *        inbetween them.
 * @param  SEP    a string to insert between the other strings
 * @param  STR    the first string to concatenate
 * @param  ...    a list of strings, no NULL sentinel is needed
 * @return char*  a NUL terminated string that can be freed**/
#define VSTRCATSEP(SEP, STR, ...) vstrcatsep((SEP), (STR), __VA_ARGS__, (char*)0)

/**@brief  A wrapper around vstrcatsep, with no separator and no need
 *         for a sentinel. This concatenates a list of strings together.
 * @param  STR    the first string to concatenate
 * @param  ...    a list of strings, no NULL sentinel is needed
 * @return char* a NUL terminated string that can be freed**/
#define VSTRCAT(STR, ...) vstrcatsep("",    (STR), __VA_ARGS__, (char*)0)

/**@brief  A wrapper around vstrcatsep that only concatenates two string
 *         together.
 * @param  S1    the first string
 * @param  S2    the second string
 * @return char* a NUL terminated string that can be freed**/
#define CONCATENATE(S1, S2) vstrcatsep("",    (S1),  (S2), (char*)0)

/**@brief  A wrapper around fprintf that will print out a nicely formatted
 *         error message with line number, current function and line.
 * @param  FMT fprintf format string
 * @param  ... Arguments to the fprintf format string
 * @return int fprintf return code**/
#define PRINT_ERROR(FMT, ...) fprintf(stderr,\
                                    "(error " FMT " '%s '%s %d)\n",\
                                     __VA_ARGS__, __FILE__, __func__, __LINE__)

/**@brief Print out an error message and throw a lisp exception.
 * @param LISP The lisp environment to throw the exception in
 * @param RET  Internal lisp exception return code
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define LISP_FAIL_PRINTER(LISP, RET, FMT, ...)\
	do{\
        	lisp_log_error((LISP),\
                        "\n %y'%s%t\n " FMT "\n %r\"%s\"\n %m%d%t",\
                       __func__, __VA_ARGS__, __FILE__, (intptr_t)__LINE__);\
        	lisp_throw((LISP), RET);\
        } while(0)

/**@brief Print out an error message and throw a lisp exception that
 *        can be recovered from.
 * @param LISP The lisp environment to throw the exception in
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define LISP_RECOVER(LISP, FMT, ...) LISP_FAIL_PRINTER(LISP,  1, FMT, __VA_ARGS__)

/**@brief Print out an error message and throw a lisp exception that
 *        the interpreter cannot recover from, it will return.
 * @param LISP The lisp environment to throw the exception in
 * @param FMT  Format string printing error messages
 * @param ...  Arguments for format string**/
#define LISP_HALT(LISP, FMT, ...) LISP_FAIL_PRINTER(LISP, -1, FMT, __VA_ARGS__)

#endif /*end of C99 variable length macros*/

/**@brief A wrapper around pfatal that with file and line filled out
 *        for the error message to print.
 * @param MSG Message to print out**/
#define FATAL(MSG) pfatal((MSG), __FILE__, __func__, __LINE__)

/** @brief  validate a lisp expression and automatically put file, function and
 *          line information into the macro.
 *  LISP    lisp environment
 *  LEN     number of
 *  FMT     format string described in lisp_validate_args comment
 *  ARGS    arguments to check
 *  LISP_RECOVER halt lisp environment if negative, recover if positive **/
#ifndef LISP_DO_NOT_VALIDATE_ARGS
#define LISP_VALIDATE_ARGS(LISP, MSG, LEN, FMT, ARGS, LISP_RECOVER)\
        lisp_validate_args((LISP), (MSG), (LEN), (FMT), (ARGS), (LISP_RECOVER))
#else
#define LISP_VALIDATE_ARGS(LISP, MSG, LEN, FMT, ARGS, LISP_RECOVER) 0
#endif

/**@brief Stringify X, turn X into a string.
 * @param X      Thing to turn into a string  **/
#define STRINGIFY(X) #X

/**@brief Stringify with macro expansion
 * @param X      Thing to turn into a string with macro expansion  **/
#define XSTRINGIFY(X) STRINGIFY(X)

/**@brief Like assert() but will not be disabled by NDEBUG.  
 * @param X Test to perform**/
#define ASSERT(X) do { if(!(X)) FATAL("assertion failed: " # X ); } while(0)

/**@brief Swap two values, with a specified type. For example to swap
 *        two integers values 'a' and 'b': SWAP(a, b, int);
 * @param X    variable X
 * @param Y    variable Y
 * @param TYPE type of variables**/
#define SWAP(X, Y, TYPE) do { TYPE SWAP = X; X = Y; Y = SWAP; } while(0);

#ifndef ABS
#define ABS(X) ((X) < 0 ? -(X) : (X)) /**< absolute value*/
#endif

#ifndef MAX
#define MAX(X, Y)    ((X)>(Y)?(X):(Y)) /**< largest of two values*/
#endif

#ifndef MIN
#define MIN(X, Y)    ((X)>(Y)?(Y):(X)) /**< smallest of two values*/
#endif

#define UNUSED(X)    ((void)(X)) /**< unused variable*/

/* The following macros are helper macros for lisp list access */
#define CAAR(X)    car(car((X))) /**< ((1))      ==> 1*/
#define CADR(X)    car(cdr((X))) /**< (1 2 )     ==> 2*/
#define CDAR(X)    cdr(car((X))) /**< ((1 2 3) ) ==> (2 3) */
#define CDDR(X)    cdr(cdr((X))) /**< (1 2 3 4)  ==> (3 4)*/
#define CADAR(X)   car(cdr(car((X))))
#define CADDR(X)   car(cdr(cdr((X))))
#define CDDDR(X)   cdr(cdr(cdr((X))))
#define CADDDR(X)  car(cdr(cdr(cdr((X)))))
#define CADDDDR(X) car(cdr(cdr(cdr(cdr((X))))))

#define BEL "\007" /**< ASCII Bell */
#define ESC "\033" /**< ASCII Escape*/
#define GS  "\035" /**< ASCII Group Separator*/
#define RS  "\036" /**< ASCII Record Separator*/
#define US  "\037" /**< ASCII Unit Separator*/
#define DEL "\177" /**< ASCII Delete */

#define SEP ":"    /**< Separator for docstrings */
#define MK_DOCSTR(NAME, DOCSTR) (NAME SEP __FILE__ SEP DOCSTR)

/*****************************************************************************/
/* module stuff to move into separate header */

typedef int (*lisp_module_initializer_t)(lisp_t *l);
/**@brief This function does not exist in the library, user supplied DLLs must
 * contain this function, which dlsym (or your systems equivalent) will look
 * for an execute. 
 * @param  l   lisp environment
 * @return int 0 or greater for success, less than zero for failure **/
int lisp_module_initialize(lisp_t *l);

/*****************************************************************************/
#ifdef __cplusplus
}
#endif
#endif /* LIBLISP_H */

