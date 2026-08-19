Malloc Fail
===========

This is a project that contains various ways to introduce memory allocation
into your C programs, with the aim of ensuring that no failed allocation can
cause a problem.

mallocfailwrap - wrap
---------------------

This is a static library that replaces memory allocation functions by using the
linker function wrapping capability.

It fails an allocation randomly with a 1 in N chance, using its own pseudo
random number generator. The seed for the PRNG can be randomly generated at
start, set by an environment variable, or by using an initialisation function
which hases some input data.

The hashed input data method is suitable for use with fuzzers, with the fuzzer
generated input data. This has the effect that the allocation failures are
deterministic based on the fuzzer input and so will not confuse the fuzzer.

For more details see the [README](wrap/README.md)

mallocffailpreload - ld_preload
------------------------------

This is a shared library that replaces memory allocation functions by using
`LD_PRELOAD`, which ensures that the implementation of `malloc` etc. from the
library are used in preference to the ones provided by the C library.

Each time an allocation is requested, the library checks the callstack. If
this is the first time the given callstack has been seen, the allocation
fails, otherwise it succeeds. The library stores the callstacks in a file so
that then can be correctly accounted for over multiple runs.

In principle this allows full coverage of all memory allocations. In practice
it is very hard to achieve and takes a long time.

It can still be useful however.

For more details see the [README](ld_preload/README.md)

failgrind
---------

This is not a project hosted here, but is worth mentioning. This is an
experimental valgrind tool that operates in a very similar manner to
`mallocfailpreload`, but with many more options for controlling where
allocation failures can occur, with valgrind macros to obtain information about
the current state of the tool - potentially allowing a loop that exercises a
full test, and the ability to fail syscalls.

See https://github.com/ralight/valgrind-snap for more details.
