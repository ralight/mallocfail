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


Environment Variables
---------------------

You can control the behaviour of mallocfailwrap with some environment variables.

`MALLOCFAIL_FAIL_CHANCE` is an integer that sets the chance that a given
allocation will fail. The default is 1000 which means that there is a 1 in 1000
chance of failure.

`MALLOCFAIL_IGNORE_INITIAL` is an integer value which is the count of
allocations that MFW will always allow, meaning some setup allocations can be
made safe. Defaults to 0.


Wrapped Functions
-----------------

* malloc
* calloc
* realloc
* strdup


Dependencies
------------

Ian Taylor's [libbacktrace](https://github.com/ianlancetaylor/libbacktrace)
