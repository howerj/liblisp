CC=gcc
CFLAGS=-Wall -Wextra -O2 -g
.PHONY: all clean
TARGETS=cc thread unix tcc
all: $(TARGETS)
../liblisp.a:
	make -C ../ -f makefile
cc: ../liblisp.a cc.cpp
	g++ -Wall -Wextra -I../ -L../  $^ -llisp -o $@
thread: ../liblisp.a thread.c 
	$(CC) -Wall -Wextra -I../ -L../  $^ -llisp -lpthread -o $@
unix: ../liblisp.a unix.c 
	$(CC) -Wall -Wextra -I../ -L../  $^ -llisp -lpthread -o $@
tcc:  ../liblisp.a tcc.c
	$(CC) -Wall -Wextra -I../ -L../ $^ -llisp -ltcc -ldl -o $@
clean:
	rm -rf $(TARGETS) *.a *.so *.o *.log *.htm *.html doxygen
