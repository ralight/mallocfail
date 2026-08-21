#define uthash_malloc __real_malloc

#include <backtrace.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/random.h>
#include <unistd.h>
#include <uthash.h>

#include "mallocfail/wrap.h"

struct frame{
	char *filename;
	char *function;
	uintptr_t pc;
	int lineno;
};

struct function_allowlist{
	UT_hash_handle hh;
	char function[1];
};

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
static void first_run(void);

static uint64_t rnd_state;
static uint64_t allocation_count = 0;
static int backtrace_count = 0;
static struct backtrace_state *bt_state = NULL;
static struct frame *callstack = NULL;
static long callstack_max_depth = 200;
static long callstack_min_depth = 0;
static long debug = 0;
static long fail_chance = -1;
static bool force_allow = false;
static long filename_len = 200;
static long function_len = 100;
static struct function_allowlist *allowlist = NULL;

static long ignore_initial_count = 0;
static bool initialised = false;


/* --------------------------------------------------
 * Output
 * -------------------------------------------------- */

void debug_printf(int level, const char *fmt, ...)
{
	if(level > debug){
		return;
	}

	va_list va;

	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}


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


static void splitmix64_seed(void)
{
	long env_seed = env_to_long("MALLOCFAIL_RNG_SEED", 0);

	if(env_seed == 0){
		getentropy(&rnd_state, sizeof(rnd_state));
	}
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
 * Backtrace
 * -------------------------------------------------- */
static void backtrace_cleanup(void)
{
	if(callstack){
		for(int i=0; i<callstack_max_depth; i++){
			free(callstack[i].filename);
			free(callstack[i].function);
		}
	}
	free(callstack);
}


static void backtrace_init(void)
{
	bt_state = backtrace_create_state(NULL, 1, NULL, NULL);
	atexit(backtrace_cleanup);
	callstack = __real_calloc(callstack_max_depth, sizeof(struct frame));
	if(!callstack){
		fprintf(stderr, "mallocfail: Out of memory in init, exiting.\n");
		exit(1);
	}
	for(int i=0; i<callstack_max_depth; i++){
		callstack[i].filename = __real_calloc(filename_len, sizeof(char));
		callstack[i].function = __real_calloc(function_len, sizeof(char));
		if(!callstack[i].filename || !callstack[i].function){
			fprintf(stderr, "mallocfail: Out of memory in init, exiting.\n");
			exit(1);
		}
	}
}


static int backtrace_collect_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function)
{
	if(lineno && callstack && backtrace_count < callstack_max_depth){
		snprintf(callstack[backtrace_count].filename, filename_len-1, "%s", filename);
		snprintf(callstack[backtrace_count].function, function_len-1, "%s", function);
		callstack[backtrace_count].pc = pc;
		callstack[backtrace_count].lineno = lineno;

		backtrace_count++;
	}

	return 0;
}


static void collect_backtrace(void)
{
	force_allow = true;
	backtrace_count = 0;
	backtrace_full(bt_state, 0, backtrace_collect_callback, NULL, NULL);
	force_allow = false;
}


static void print_backtrace(void)
{
	if(debug < 2){
		return;
	}
	debug_printf(2, "------- Start trace -------\n");
	for(int i=0; i<backtrace_count-callstack_min_depth; i++){
		struct frame *f = &callstack[i];

		debug_printf(2, "#%d %p in %s at %s:%d\n", i, f->pc, f->function, f->filename, f->lineno);
	}
	debug_printf(2, "------- End trace -------\n");
	force_allow = false;
}


/* --------------------------------------------------
 * Allow list
 * -------------------------------------------------- */
static int allowlist_add(const char *function)
{
	struct function_allowlist *item = NULL;
	size_t len = strlen(function);
	item = __real_calloc(1, sizeof(struct function_allowlist) + len + 1);
	if(!item){
		return 1;
	}
	strncpy(item->function, function, len+1);
	HASH_ADD_KEYPTR(hh, allowlist, item->function, len, item);
	return 0;
}


static bool allowlist_check(void)
{
	struct function_allowlist *item = NULL;
	for(int i=backtrace_count-1; i>=0; i--){
		HASH_FIND(hh, allowlist, callstack[i].function, strlen(callstack[i].function), item);
		if(item){
			return false;
		}
	}
	return true;
}


static void allowlist_cleanup(void)
{
	struct function_allowlist *item, *tmp;
	HASH_ITER(hh, allowlist, item, tmp){
		HASH_DELETE(hh, allowlist, item);
		free(item);
	}
}


static void allowlist_print(void)
{
	struct function_allowlist *item, *tmp;
	if(allowlist){
		debug_printf(1, "Function allow list:\n");
		HASH_ITER(hh, allowlist, item, tmp){
			debug_printf(1, "- %s\n", item->function);
		}
	}
}


static int allowlist_init(const char *function_file)
{
	atexit(allowlist_cleanup);

	char *func = __real_calloc(function_len+1, sizeof(char));
	if(!func){
		fprintf(stderr, "mallocfail: Out of memory in init, exiting.\n");
		exit(1);
	}
	FILE *fptr = fopen(function_file, "rt");
	if(!fptr){
		free(func);
		return 0;
	}

	while(fgets(func, function_len+1, fptr)){
		size_t pos = strlen(func)-1;
		while(pos >= 0 &&
				(func[pos] == '\n' || func[pos] == '\r' || func[pos] == ' ')){
			func[pos] = '\0';
			pos--;
		}
		if(func[0] == '+'){
			allowlist_add(&func[1]);
		}
	}
	free(func);

	return 0;
}

/* --------------------------------------------------
 * Core function - should the allocation fail?
 * -------------------------------------------------- */
static bool should_malloc_fail(void)
{
	if(!initialised){
		first_run();
	}
	if(fail_chance < 0 || force_allow == true){
		return false;
	}
	allocation_count++;

	static int alloc_count = 0;
	if(alloc_count < ignore_initial_count){
		alloc_count++;
		return false;
	}

	bool fail = (splitmix64() % fail_chance) == 0;

	if(fail){
		collect_backtrace();
		fail = allowlist_check();
	}

	if(fail){
		debug_printf(2, "\nmallocfail: Failing allocation %llu\n", allocation_count);
		print_backtrace();
	}
	return fail;
}


/* --------------------------------------------------
 * Init
 * -------------------------------------------------- */


static void first_run(void)
{
	if(initialised){
		return;
	}
	debug = env_to_long("MALLOCFAIL_DEBUG", 0);
	fail_chance = env_to_long("MALLOCFAIL_FAIL_CHANCE", 1000);
	ignore_initial_count = env_to_long("MALLOCFAIL_IGNORE_INITIAL", 0);
	callstack_max_depth = env_to_long("MALLOCFAIL_CALLSTACK_MAX_DEPTH", 200);
	callstack_min_depth = env_to_long("MALLOCFAIL_CALLSTACK_MIN_DEPTH", 0);
	filename_len = env_to_long("MALLOCFAIL_FILENAME_LEN", 200);
	if(filename_len < 100){
		filename_len = 100;
	}
	function_len = env_to_long("MALLOCFAIL_FUNCTION_LEN", 100);
	if(function_len < 50){
		function_len = 50;
	}
	splitmix64_seed();
	backtrace_init();
	allowlist_init(getenv("MALLOCFAIL_ALLOWLIST"));
	initialised = true;

	debug_printf(1, "mallocfail: allow list=%s\n", getenv("MALLOCFAIL_ALLOWLIST"));
	debug_printf(1, "mallocfail: debug=%ld\n", debug);
	debug_printf(1, "mallocfail: fail chance=1:%ld\n", fail_chance);
	debug_printf(1, "mallocfail: filename length=%ld\n", filename_len);
	debug_printf(1, "mallocfail: function length=%ld\n", function_len);
	debug_printf(1, "mallocfail: ignore initial allocations=%ld\n", ignore_initial_count);
	debug_printf(1, "mallocfail: callstack max depth=%ld\n", callstack_max_depth);
	debug_printf(1, "mallocfail: callstack min depth=%ld\n", callstack_min_depth);
	debug_printf(1, "mallocfail: seed=%llu\n", rnd_state);
	allowlist_print();
}


void mallocfailwrap_init(const void *data, size_t size)
{
	first_run();

	if(size > 0 && data){
		rnd_state = hash_fnv1a(data, size);
	}
}
