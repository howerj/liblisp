CC=gcc
CFLAGS=-Wall -Wextra -std=gnu99 -g -fPIC 
MODULES=liblisp_sql.so liblisp_os.so liblisp_crc.so liblisp_tcc.so

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

liblisp_sql.so: liblisp_sql.o
	$(CC) $(CFLAGS) -shared $^ -lsqlite3 -o $@

liblisp_os.so: liblisp_os.o 
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_crc.so: crc.o liblisp_crc.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_tcc.so: liblisp_tcc.o
	$(CC) $(CFLAGS) -shared $^ -ltcc -o $@

help:
	@echo ""
	@echo "This makefile builds lisp modules for the liblisp library."
	@echo ""
	@echo "The modules can be loaded at run time by the lisp interpreter"
	@echo "and the modules will automatically add the functions and variables"
	@echo "they contain to the lisp interpreter."
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all              make all of the modules"
	@echo "     help             this help message!"
	@echo "     liblisp_sql.so   SQL module"
	@echo "     liblisp_os.so    OS module"
	@echo "     liblisp_crc.so   CRC module"
	@echo "     liblisp_tcc.so   Tiny C Compiler module"
	@echo "     install          Install all of the modules"
	@echo "     clean            Remove all compiled files"
	@echo ""

install: $(MODULES)
	cp $(MODULES) /usr/local/lib/
	#ldconfig

clean:
	rm -f *.so *.a *.o
