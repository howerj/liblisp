## plan.md 2014 01 08

* Read SICP
* Create full plan, actually plan everything out! All the data structures,
the stages, everything.

### Data structure, native types:

* Unsigned int, primitive, list, symbol, string, hash, null, file handle ...

    typedef struct{
      enum cell_t type;
      unsigned int len;
      void *data;
    } sexpr;

* Libraries for manipulations on stacks, safe allocation of memory, for
walking linked lists, for garbage collections, ...

### Stages

1. S-expr parser, data structures, safe memory allocation, error reporting.
2. Eval{ find, add, apply, primitives, ... }
3. Library functions.

### Regexs

Special regexs for matching S-expressions (non recursive ones) and internal
dictionaries.

### Library plans

#### Algorithms

* Binary search
* Quick sort 
* Garbage collection


### Tools used

* Make extensive use of Valgrind throughout the process.
