# mem.md
## Synopsis

A wrapper around malloc, realloc, calloc and free.

## Files

* mem.c
* mem.h

## Rationale

Wrapping up C memory handling functions allows for the insertion of debugging
code for when each memory operation happens, but it also allows us to validate
the inputs (especially for realloc) and handle out of memory events in the 
wrapper.

## Description
## Specification
## References
