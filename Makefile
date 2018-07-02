CC=gcc
INSTALL=install
prefix=/usr/local
CFLAGS=-Wall -ggdb -O2 -Ideps/uthash -Ideps/sha3 -Isrc
LDFLAGS=

.PHONY : all test clean install

all : mf_test mallocfail.so

mallocfail.o : src/mallocfail.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

memory_funcs.o : src/memory_funcs.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

sha3.o : deps/sha3/sha3.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

mf_test.o : mf_test.c
	$(CC) -c $(CFLAGS) -o $@ $<

mallocfail.so : mallocfail.o memory_funcs.o sha3.o
	$(CC) -shared -o $@ $^ ${LDFLAGS} -fPIC -ldl -lbacktrace

mf_test : mf_test.o
	$(CC) -o $@ $^ ${LDFLAGS}

test : mf_test mallocfail.so
	LD_PRELOAD=./mallocfail.so ./mf_test

clean :
	-rm -f *.o mallocfail.so mf_test mallocfail_hashes.txt

install : mallocfail.so
	$(INSTALL) mallocfail.so ${DESTDIR}${prefix}/lib/mallocfail.so
	$(INSTALL) mallocfail ${DESTDIR}${prefix}/bin/mallocfail

