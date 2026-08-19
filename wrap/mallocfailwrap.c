#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/random.h>
#include <unistd.h>

#include "mallocfail/wrap.h"

extern void *__real_malloc(size_t);
extern void *__real_calloc(size_t, size_t);
extern void *__real_realloc(void *, size_t);
extern char *__real_strdup(const char *);

void *__wrap_malloc(size_t);
void *__wrap_calloc(size_t, size_t);
void *__wrap_realloc(void *, size_t);
char *__wrap_strdup(const char *);

static inline uint64_t hash_fnv1a(const void *data, size_t size);
static inline uint64_t splitmix64(void);
static bool should_malloc_fail(void);

static uint64_t rnd_state;
static long fail_chance = -1;
static long ignore_initial_count = 0;


/* --------------------------------------------------
 * Env vars
 * -------------------------------------------------- */
static long env_to_long(const char *envvar, long def)
{
	char *value = getenv(envvar);
	if(value && value[0] != '\0'){
		char *endptr = NULL;
		errno = 0;
		long num = strtol(value, &endptr, 10);
		if(endptr[0] != '\0' || errno != 0){
			return def;
		}else{
			return num;
		}
	}else{
		return def;
	}
}


/* --------------------------------------------------
 * Hash and random number generation
 * -------------------------------------------------- */
static inline uint64_t hash_fnv1a(const void *data, size_t size)
{
	uint64_t hash = 14695981039346656037ULL;
	const uint8_t *d8 = (const uint8_t *)data;
	for(size_t i=0; i<size; i++){
		hash = hash ^ d8[i];
		hash = hash * 1099511628211ULL;
	}
	return hash;
}


static inline uint64_t splitmix64(void)
{
	rnd_state += 0x9e3779b97f4a7c15;
	uint64_t z = rnd_state;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}


/* --------------------------------------------------
 * Wrapped memory functions
 * -------------------------------------------------- */
void *__wrap_malloc(size_t size)
{
	if(!should_malloc_fail()){
		return __real_malloc(size);
	}else{
		return NULL;
	}
}


void *__wrap_calloc(size_t nmemb, size_t size)
{
	if(!should_malloc_fail()){
		return __real_calloc(nmemb, size);
	}else{
		return NULL;
	}
}


void *__wrap_realloc(void *ptr, size_t size)
{
	if(!should_malloc_fail()){
		return __real_realloc(ptr, size);
	}else{
		return NULL;
	}
}


char *__wrap_strdup(const char *str)
{
	if(!should_malloc_fail()){
		return __real_strdup(str);
	}else{
		return NULL;
	}
}


/* --------------------------------------------------
 * Core function - should the allocation fail?
 * -------------------------------------------------- */
static bool should_malloc_fail(void)
{
	if(fail_chance < 0){
		return false;
	}
	static int alloc_count = 0;
	if(alloc_count < ignore_initial_count){
		alloc_count++;
		return false;
	}

	bool fail = (splitmix64() % fail_chance) == 0;

	return fail;
}


/* --------------------------------------------------
 * Init
 * -------------------------------------------------- */
void mallocfailwrap_init(const void *data, size_t size)
{
	fail_chance = env_to_long("MALLOCFAIL_FAIL_CHANCE", 1000);
	ignore_initial_count = env_to_long("MALLOCFAIL_IGNORE_INITIAL", 0);

	if(size > 0 && data){
		rnd_state = hash_fnv1a(data, size);
	}else{
		getentropy(&rnd_state, sizeof(rnd_state));
	}
}
