CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -fPIC
MODULES=libmodsql.so

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

libmodsql.so: libmodsql.o
	$(CC) $(CFLAGS) -shared $^ -lsqlite3 -o $@

help:
	@echo ""
	@echo "lisp modules"
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all"
	@echo "     help"
	@echo "     clean"

clean:
	rm -f *.so *.a *.o
