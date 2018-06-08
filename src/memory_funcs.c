#define _GNU_SOURCE

#include "mallocfail.h"
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>


void *(*libc_malloc)(size_t) = NULL;
void *(*libc_calloc)(size_t, size_t) = NULL;
void *(*libc_realloc)(void *, size_t) = NULL;

int force_libc = 0;
static int init_state = 0;
static char tmpbuf[1024];
static size_t tmppos = 0;


static void init(void)
{
	if(init_state) return;

	init_state = 1;
	libc_malloc = dlsym(RTLD_NEXT, "malloc");
	libc_calloc = dlsym(RTLD_NEXT, "calloc");
	libc_realloc = dlsym(RTLD_NEXT, "realloc");

	if(!libc_malloc || !libc_calloc || !libc_realloc){
		// FIXME - it would be nice to print an error here
		exit(1);
	}

	init_state = 2;
}


void *malloc(size_t size)
{
	void *retptr;

	if(init_state != 2){
		if(init_state == 1){
			if(tmppos + size >= sizeof(tmpbuf)) exit(1);

			retptr = tmpbuf + tmppos;
			tmppos += size;
			return retptr;
		}else{
			init();
		}
	}

	if(force_libc || !should_malloc_fail()){
		return libc_malloc(size);
	}else{
		return NULL;
	}
}


void *calloc(size_t nmemb, size_t size)
{
	void *ptr;
	int i;

	if(init_state != 2){
		if(init_state == 1){
			ptr = malloc(nmemb*size);
			for(i=0; i<nmemb*size; i++){
				((char *)ptr)[i] = '\0';
			}
			return ptr;
		}else{
			init();
		}
	}

	if(force_libc || !should_malloc_fail()){
		return libc_calloc(nmemb, size);
	}else{
		return NULL;
	}
}


void *realloc(void *ptr, size_t size)
{
	if(init_state != 2) init();

	if(force_libc || !should_malloc_fail()){
		return libc_realloc(ptr, size);
	}else{
		return NULL;
	}
}


