Malloc Fail - Wrap Edition (mallocfailwrap - MFW)
=================================================

This is a static library that aims to help you test memory allocation failures
in a random but deterministic manner. It is designed specifically for use in
fuzzers.

By linking MFW into your fuzzer with the appropriate linker options, memory
allocations will randomly fail based on a random number generator. The seed for
the RNG can be set directly at runtime, generated based on the hash of some
input data (recommnded for fuzz input), or left unset in which case a random
value is used.

The behaviour of MFW can be modified as described in the `Environment
Variables` section.


Simple Usage
------------


Environment Variables - Operation
---------------------------------

You can control the behaviour of mallocfailwrap with some environment variables.

`MALLOCFAIL_ALLOWLIST` is a string that is the path to a file containing a list
of functions, one per line, that if detected in the callstack should have
allocations always succeed. For example, applications using openssl will
definitely want all allocations in `OPENSSL_init_ssl` to succeed.

`MALLOCFAIL_FAIL_CHANCE` is an integer that sets the chance that a given
allocation will fail. The default is 1000 which means that there is a 1 in 1000
chance of failure.

`MALLOCFAIL_IGNORE_INITIAL` is an integer value which is the count of
allocations that MFW will always allow, meaning some setup allocations can be
made safe. Defaults to 0.

`MALLOCFAIL_DEBUG` is an integer that controls the debug output. Set to 0, the
default, to give no output at all. Increasing the value gives more debug
output. Current output levels produce:

1. Information on configuration and state on initialisation.
2. Callstack of when an allocation is failed.

`MALLOCFAIL_RNG_SEED` is an integer value that is used as the seed for the
internal random number generator. This allows runs to be repeated with the same
seed to get the same failure pattern.


Environment Variables - Tuning
------------------------------

These variables allow various internal parameters to be tuned.

`MALLOCFAIL_CALLSTACK_MAX_DEPTH` is an integer that sets the maximum callstack
depth that is allowed. Defaults to 200. Any functions beyond this depth will
not be recorded and printed if `MALLOCFAIL_DEBUG` is set to 2 or greater. Note
that this value affects the memory pre-allocated for storing callstack frames.
The total memory usage is roughly `MALLOCFAIL_CALLSTACK_MAX_DEPTH *
(MALLOCFAIL_FILENAME_LEN + MALLOCFAIL_FUNCTION_LEN + sizeof(uintptr_t) +
sizeof(int))` bytes.

`MALLOCFAIL_CALLSTACK_MIN_DEPTH` is an integer that sets the minimum callstack
depth where functions should start being printed when `MALLOCFAIL_DEBUG` is set
to 2 or greater. This allows functions below main such as
`__libc_start_call_main` to be removed from the callstack output. It is
entirely acceptable to set this value so that `main` or application functions
are suppressed. Defaults to 0.

`MALLOCFAIL_FILENAME_LEN` is an integer that sets the maximum number of bytes
that can be in a filename reported in a callstack. Defaults to 200. Characters
beyond this limit will be truncated. Set to a higher value if your path
requires it, at the expense of an increase in the pre-allocated memory needed.

`MALLOCFAIL_FUNCTION_LEN` is an integer that sets the maximum number of bytes
that can be in a function name reported in a callstack. Defaults to 100.
Characters beyond this limit will be truncated. Set to a higher value if your
function names require it, at the expense of an increase in the pre-allocated
memory needed.


Wrapped Functions
-----------------

* malloc
* calloc
* realloc
* strdup


Dependencies
------------

Ian Taylor's [libbacktrace](https://github.com/ianlancetaylor/libbacktrace)
