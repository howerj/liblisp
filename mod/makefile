CC?=gcc
CFLAGS?=-Wall -Wextra -std=gnu99 -g -O2

TARGET_SYSTEM := $(subst -, ,$(shell $(CC) -dumpmachine))

# modules to compile, system dependent modules are added later.
MODULES=liblisp_crc.$(DLL)\
       	liblisp_bignum.$(DLL) liblisp_math.$(DLL) liblisp_line.$(DLL)\
	liblisp_diff.$(DLL) liblisp_tsort.$(DLL)

# These variables should be inherited from the calling makefile (if
# it is possible to included this file within it), or read from a global
# configuration makefile
ifeq (mingw32, $(TARGET_SYSTEM))
# @todo make this work for cygwin as well
DLL=dll
ADDITIONAL=..\liblisp.$(DLL)
RM=del
RM_FLAGS= /Q 
FPIC=
CP=copy
FS=\\
INSTALL_PATH=C:\lisp
MODULES+=
else # unix is default
DLL=so
ADDITIONAL=
RM=rm 
RM_FLAGS= -f 
FPIC= -fPIC
CP=cp
FS=/
INSTALL_PATH=/usr/local/lib
MODULES+=liblisp_tcc.$(DLL) liblisp_sql.$(DLL) liblisp_unix.$(DLL)\
	 liblisp_x11.$(DLL)
endif

all: $(MODULES)

%.o: %.c
	$(CC) $(CFLAGS) $(FPIC) -I.. $^ -c -o $@

liblisp_line.o: liblisp_line.c 
	$(CC) $(CFLAGS) $(FPIC) -I.. -Ilibline/ $^ -c -o $@

liblisp_sql.$(DLL): liblisp_sql.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -lsqlite3 -o $@

liblisp_unix.$(DLL): liblisp_unix.o 
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -o $@

liblisp_crc.$(DLL): crc.o liblisp_crc.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -o $@

liblisp_tcc.$(DLL): liblisp_tcc.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -ltcc -o $@

liblisp_math.$(DLL): liblisp_math.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -lm -o $@

liblisp_line.$(DLL): liblisp_line.o libline.$(DLL)
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -L. -lline -o $@

liblisp_x11.$(DLL): liblisp_x11.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -lX11 -o $@

liblisp_diff.$(DLL): diff.o liblisp_diff.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -o $@

liblisp_tsort.$(DLL): tsort.o liblisp_tsort.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -o $@

liblisp_bignum.$(DLL): bignum.o liblisp_bignum.o
	$(CC) $(CFLAGS) $(FPIC) -shared $^ $(ADDITIONAL) -o $@

libline.$(DLL):
	make -C libline
	$(CP) libline$(FS)libline.$(DLL) .

install: $(MODULES)
	$(CP) $(MODULES) $(INSTALL_PATH)
	$(CP) libline.$(DLL) $(INSTALL_PATH)

clean:
	-$(RM) $(RM_FLAGS) *.$(DLL)
	-$(RM) $(RM_FLAGS) *.a
	-$(RM) $(RM_FLAGS) *.o
