# io.md
## Synopsis

A I/O wrapper that allows you to write to strings and file descriptors like they
are the same. It also handles color output.

## Files

* io.c
* io.h

## Rationale

An I/O wrapper like this one allows us to abstract out whether we are reading or
writing to a file descriptor, a string or whatever else is implemented. It also 
allows for handling of color codes via a simplified version of the printf
family.

## Description
## Specification
## References
