CC?=gcc
CFLAGS?=-Wall -Wextra -std=gnu99 -g -O2 -fPIC 
MODULES=liblisp_sql.so liblisp_unix.so liblisp_crc.so\
       	liblisp_bignum.so liblisp_math.so liblisp_line.so\
	liblisp_x11.so liblisp_diff.so liblisp_tsort.so\
	#liblisp_tcc.so

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) -I.. $^ -c -o $@

liblisp_line.o: liblisp_line.c 
	$(CC) $(CFLAGS) -I.. -Ilibline/ $^ -c -o $@

liblisp_sql.so: liblisp_sql.o
	$(CC) $(CFLAGS) -shared $^ -lsqlite3 -o $@

liblisp_unix.so: liblisp_unix.o 
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_crc.so: crc.o liblisp_crc.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_tcc.so: liblisp_tcc.o
	$(CC) $(CFLAGS) -shared $^ -ltcc -o $@

liblisp_math.so: liblisp_math.o
	$(CC) $(CFLAGS) -shared $^ -lm -o $@

liblisp_line.so: liblisp_line.o libline.so
	$(CC) $(CFLAGS) -shared $^ -L. -lline -o $@

liblisp_x11.so: liblisp_x11.o
	$(CC) $(CFLAGS) -shared $^ -lX11 -o $@

liblisp_diff.so: diff.o liblisp_diff.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_tsort.so: tsort.o liblisp_tsort.o
	$(CC) $(CFLAGS) -shared $^ -o $@

liblisp_bignum.so: bignum.o liblisp_bignum.o
	$(CC) $(CFLAGS) -shared $^ -o $@

libline.so:
	cd libline && make
	cp libline/libline.so .

install: $(MODULES)
	cp $(MODULES) /usr/local/lib/
	#ldconfig

clean:
	rm -f *.so *.a *.o
