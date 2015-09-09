CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -fPIC
MODULES=modsql.so

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

%.so: %.o
	$(CC) $(CFLAGS) -shared $^ -o $@

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
