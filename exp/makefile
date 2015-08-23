CC=gcc
CFLAGS=-Wall -Wextra -O2 -g -pedantic
.PHONY: all clean
TARGETS=cc thread tcc hash
all: $(TARGETS)
../liblisp.a:
	make -C ../ -f makefile
cc: ../liblisp.a cc.cpp
	g++ -Wall -Wextra -I../ -L../  $^ -llisp -o $@
thread: ../liblisp.a thread.c 
	$(CC) -Wall -Wextra -I../ -L../  $^ -llisp -lpthread -o $@
tcc:  ../liblisp.a tcc.c
	$(CC) -Wall -Wextra -I../ -L../ $^ -llisp -ltcc -ldl -o $@
hash: hash_linear.c
	$(CC) -Wall -Wextra $^ -o $@
clean:
	rm -rf $(TARGETS) *.a *.so *.o *.log *.htm *.html doxygen
