CC=gcc
INSTALL=install
prefix=/usr/local
CFLAGS=-Wall -ggdb -O2 -Ideps/uthash -Ideps/sha3 -Ild_preload
LDFLAGS=

.PHONY : all test clean install

all : mf_test mallocfail_preload.so

ld_preload/mallocfail_preload.o : ld_preload/mallocfail_preload.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

ld_preload/memory_funcs.o : ld_preload/memory_funcs.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

sha3.o : deps/sha3/sha3.c
	$(CC) -c $(CFLAGS) -fPIC -o $@ $<

mf_test.o : mf_test.c
	$(CC) -c $(CFLAGS) -o $@ $<

mallocfail_preload.so : ld_preload/mallocfail_preload.o ld_preload/memory_funcs.o sha3.o
	$(CC) -shared -o $@ $^ ${LDFLAGS} -fPIC -ldl -lbacktrace

mf_test : mf_test.o
	$(CC) -o $@ $^ ${LDFLAGS}

test : mf_test mallocfail_preload.so
	LD_PRELOAD=./mallocfail_preload.so ./mf_test

clean :
	-rm -f *.o mallocfail_preload.so mf_test mallocfail_hashes.txt

install : mallocfail.so
	$(INSTALL) mallocfail_preload.so ${DESTDIR}${prefix}/lib/mallocfail_preload.so
	$(INSTALL) mallocfail_preload ${DESTDIR}${prefix}/bin/mallocfail_preload
	sed -i "s#/usr/local#${prefix}#" ${DESTDIR}${prefix}/bin/mallocfail_preload

