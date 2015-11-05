CC=gcc
CXX=g++
CFLAGS= -Wall -Wextra -O2 -g -pedantic -std=c99 
CXXFLAGS= -Wall -Wextra -O2 -g -pedantic
.PHONY: all clean
TARGETS=cc thread monte prolog
RPATH   = -Wl,-rpath=..
all: $(TARGETS)
../liblisp.so:
	make -C ../ -f makefile
cc: ../liblisp.so cc.cpp
	$(CXX) $(CXXFLAGS) -I../ -L../ $(RPATH)  $^ -llisp -o $@
prolog: prolog.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@
thread: ../liblisp.so thread.c 
	$(CC) $(CFLAGS) -I../ -L../ $(RPATH) $^ -llisp -lpthread -o $@
monte: monte.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -rf $(TARGETS) *.a *.so *.o *.log *.htm *.html doxygen
