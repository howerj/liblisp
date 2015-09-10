CC=gcc
CFLAGS=-Wall -Wextra -std=gnu99 -fPIC
MODULES=liblisp_sql.so liblisp_os.so

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

liblisp_sql.so: liblisp_sql.o
	$(CC) $(CFLAGS) -shared $^ -lsqlite3 -o $@

# linking with the system C library should be enough
liblisp_os.so: liblisp_os.o
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

install: $(MODULES)
	cp $(MODULES) /usr/local/lib/

clean:
	rm -f *.so *.a *.o
