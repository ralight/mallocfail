/*
https://stackoverflow.com/questions/1711170/unit-testing-for-failed-malloc

I saw a cool solution to this problem which was presented to me by S.
Paavolainen. The idea is to override the standard malloc(), which you can do
just in the linker, by a custom allocator which

 1. reads the current execution stack of the thread calling malloc()
 2. checks if the stack exists in a database that is stored on hard disk
    1. if the stack does not exist, adds the stack to the database and returns NULL
    2. if the stack did exist already, allocates memory normally and returns

Then you just run your unit test many times---this system automatically
enumerates through different control paths to malloc() failure and is much more
efficient and reliable than e.g. random testing.

*/

#define uthash_malloc libc_malloc

#include "uthash.h"
#include "sha3.h"

#include <stdio.h>
#include <string.h>
#include <execinfo.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <backtrace.h>

#define HASH_BITS 256
#define HASH_BYTES ((HASH_BITS)/8)
#define HASH_HEX_BYTES ((HASH_BITS)/4)

extern int force_libc;
extern void *(*libc_malloc)(size_t);
extern void *(*libc_calloc)(size_t, size_t);
extern void *(*libc_realloc)(void *, size_t);

struct traces_s {
	UT_hash_handle hh;
	char hash[HASH_HEX_BYTES+1];
};

static struct traces_s *traces = NULL;
static struct backtrace_state *state = NULL;
static char strbuf[1024];
static char *hashfile = NULL;
static char hashfile_default[] = "mallocfail_hashes";
static int debug = -1;
static int backtrace_count;
static int fail_count = 0;
static int max_fail_count = -1;


static void hex_encode(const unsigned char *in, unsigned int in_len, char *encoded)
{
	int i;
	for(i=0; i<in_len; i++){
		sprintf(&encoded[i*2], "%02x", in[i]);
	}
}


static int append_stack_context(const char *filename, const char *hash_str)
{
	FILE *fptr;

	struct traces_s *t = libc_malloc(sizeof(struct traces_s));
	memcpy(t->hash, hash_str, HASH_HEX_BYTES);
	t->hash[HASH_HEX_BYTES] = '\0';
	HASH_ADD_STR(traces, hash, t);

	fptr = fopen(filename, "at");
	if(!fptr){
		return 1;
	}
	fprintf(fptr, "%s\n", hash_str);

	fclose(fptr);
	return 0;
}


static void load_traces(const char *filename)
{
	FILE *fptr;
	char buf[1024];

	fptr = fopen(filename, "rt");
	if(!fptr){
		return;
	}

	while(!feof(fptr)){
		if(fgets(buf, 1024, fptr)){
			if(buf[strlen(buf)-1] == '\n'){
				buf[strlen(buf)-1] = '\0';
				
				struct traces_s *t = libc_malloc(sizeof(struct traces_s));
				memcpy(t->hash, buf, HASH_HEX_BYTES);
				t->hash[HASH_HEX_BYTES] = '\0';
				HASH_ADD_STR(traces, hash, t);
			}
		}
	}
	fclose(fptr);
}


static int stack_context_exists(const char *filename, const char *hash_str)
{
	struct traces_s *found_trace;
	int rc;

	force_libc = 1;
	if(traces == NULL){
		load_traces(filename);
	}

	HASH_FIND_STR(traces, hash_str, found_trace);
	if(found_trace){
		rc = 1;
	}else{
		append_stack_context(filename, hash_str);
		rc = 0;
	}

	force_libc = 0;
	return rc;
}


static int backtrace_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
	int len;
	sha3_context *hash_context = (sha3_context *)data;

	if(lineno){
		len = snprintf(strbuf, 1024, "%s:%s:%d\n", filename, function, lineno);
		sha3_Update(hash_context, strbuf, len);
	}

	return 0;
}


static void create_backtrace_hash(char *hash_str)
{
	const unsigned char *hash;
	sha3_context hash_context;

	force_libc = 1;

	sha3_Init256(&hash_context);
	backtrace_full(state, 0, backtrace_callback, NULL, &hash_context);
	hash = sha3_Finalize(&hash_context);
	hex_encode(hash, HASH_BYTES, hash_str);

	force_libc = 0;
}


static int backtrace_print_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
	if(lineno && ++backtrace_count > -3){
		printf("%s:%s:%d\n", filename, function, lineno);
	}

	return 0;
}


static void print_backtrace(void)
{
	force_libc = 1;
	printf("------- Start trace -------\n");
	backtrace_count = 0;
	backtrace_full(state, 0, backtrace_print_callback, NULL, NULL);
	printf("------- End trace -------\n");
	force_libc = 0;
}


int should_malloc_fail(void)
{
	char hash_str[1024];
	int exists;

	if(max_fail_count == -1){
		char *env = getenv("MALLOCFAIL_FAIL_COUNT");
		if(env){
			max_fail_count = atoi(env);
			if(max_fail_count < 0){
				max_fail_count = 0;
			}
		}else{
			max_fail_count = 0;
		}
	}

	if(max_fail_count > 0 && fail_count >= max_fail_count){
		return 0;
	}

	if(!state){
		state = backtrace_create_state(NULL, 1, NULL, NULL);
	}

	force_libc = 1;
	if(!hashfile){
		hashfile = getenv("MALLOCFAIL_FILE");
		if(!hashfile){
			hashfile = hashfile_default;
		}
	}

	if(debug == -1){
		if(getenv("MALLOCFAIL_DEBUG")){
			debug = 1;
		}else{
			debug = 0;
		}
	}
	force_libc = 0;

	create_backtrace_hash(hash_str);
	exists = stack_context_exists(hashfile, hash_str);
	if(!exists && debug){
		print_backtrace();
	}
	if(exists){
		return 0;
	}else{
		fail_count++;
		return 1;
	}
}

