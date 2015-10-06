CC?=gcc
CFLAGS?=-Wall -Wextra -std=gnu99 -g -O2 -fPIC 
MODULES=liblisp_sql.so liblisp_os.so liblisp_crc.so\
       	liblisp_tcc.so liblisp_math.so liblisp_line.so\
	liblisp_x11.so liblisp_diff.so liblisp_tsort.so\
	liblisp_bignum.so

all: help $(MODULES)

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

liblisp_math.so: liblisp_math.o
	$(CC) $(CFLAGS) -shared $^ -lm -o $@

liblisp_line.so: liblisp_line.o
	$(CC) $(CFLAGS) -shared $^ -lline -o $@

liblisp_x11.so: liblisp_x11.o
	$(CC) $(CFLAGS) -shared $^ -lX11 -o $@

liblisp_diff.so: diff.o liblisp_diff.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_tsort.so: tsort.o liblisp_tsort.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_bignum.so: bignum.o liblisp_bignum.o
	$(CC) $(CFLAGS) -shared $^ -o $@

help:
	@echo ""
	@echo "This makefile builds lisp modules for the liblisp library."
	@echo ""
	@echo "The modules can be loaded at run time by the lisp interpreter"
	@echo "and the modules will automatically add the functions and variables"
	@echo "they contain to the lisp interpreter."
	@echo ""
	@echo "Remember to run 'make install' for both the main interpreters"
	@echo "makefile and to run 'ldconfig' after running 'make install' for"
	@echo "this makefile."
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all               make all of the modules"
	@echo "     help              this help message!"
	@echo "     liblisp_sql.so    SQL module"
	@echo "     liblisp_os.so     OS module"
	@echo "     liblisp_crc.so    CRC module"
	@echo "     liblisp_tcc.so    Tiny C Compiler module"
	@echo "     liblisp_math.so   Math module"
	@echo "     liblisp_line.so   Line editor module"
	@echo "     liblisp_x11.so    X-Window module"
	@echo "     liblisp_diff.so   Print-the-difference module"
	@echo "     liblisp_tsort.so  Topological sorting module"
	@echo "     liblisp_bignum.so Arbitrary precision arithemtic module"
	@echo "     install           Install all of the modules"
	@echo "     clean             Remove all compiled files"
	@echo ""

install: $(MODULES)
	cp $(MODULES) /usr/local/lib/
	#ldconfig

clean:
	rm -f *.so *.a *.o
