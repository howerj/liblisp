
TARGET_SYSTEM := $(subst -, ,$(shell $(CC) -dumpmachine))

ifeq (mingw32, $(TARGET_SYSTEM))
DLL=dll
ADDITIONAL=..\liblisp.$(DLL)
RM=del
RM_FLAGS= /Q 
FPIC=
CP=copy
else
DLL=so
ADDITIONAL=
RM=rm 
RM_FLAGS= -f 
FPIC= -fPIC
CP=cp
endif


CFLAGS?=-Wall -Wextra -std=gnu99 -g -O2 $(FPIC)
MODULES=liblisp_sql.$(DLL) liblisp_unix.$(DLL) liblisp_crc.$(DLL)\
       	liblisp_bignum.$(DLL) liblisp_math.$(DLL) liblisp_line.$(DLL)\
	liblisp_x11.$(DLL) liblisp_diff.$(DLL) liblisp_tsort.$(DLL)\
	liblisp_tcc.$(DLL)

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) -I.. $^ -c -o $@

liblisp_line.o: liblisp_line.c 
	$(CC) $(CFLAGS) -I.. -Ilibline/ $^ -c -o $@

liblisp_sql.$(DLL): liblisp_sql.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -lsqlite3 -o $@

liblisp_unix.$(DLL): liblisp_unix.o 
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -o $@

liblisp_crc.$(DLL): crc.o liblisp_crc.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -o $@

liblisp_tcc.$(DLL): liblisp_tcc.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -ltcc -o $@

liblisp_math.$(DLL): liblisp_math.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -lm -o $@

liblisp_line.$(DLL): liblisp_line.o libline.$(DLL)
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -L. -lline -o $@

liblisp_x11.$(DLL): liblisp_x11.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -lX11 -o $@

liblisp_diff.$(DLL): diff.o liblisp_diff.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -o $@

liblisp_tsort.$(DLL): tsort.o liblisp_tsort.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -o $@

liblisp_bignum.$(DLL): bignum.o liblisp_bignum.o
	$(CC) $(CFLAGS) -shared $^ $(ADDITIONAL) -o $@

libline.$(DLL):
	make -C libline
	$(CP) libline/libline.so .

install: $(MODULES)
	$(CP) $(MODULES) /usr/local/lib/
	#ldconfig

clean:
	-$(RM) $(RM_FLAGS) *.$(DLL)
	-$(RM) $(RM_FLAGS) *.a
	-$(RM) $(RM_FLAGS) *.o
