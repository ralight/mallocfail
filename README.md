Malloc Fail
===========

This is a shared library that aims to help you test memory allocation failures
in a fairly deterministic manner. It was inspired by a stackoverflow answer:

https://stackoverflow.com/questions/1711170/unit-testing-for-failed-malloc

```
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
```

`mallocfail.so` implements this behaviour for you. It overrides `malloc`,
`calloc`, and `realloc` with custom versions. Each time a custom allocator
runs, the function uses `libbacktrace` to generate a text representation of the
current call stack, and then generates a sha256 hash of that text. It then
checks to see if the new hash has already been seen. If it has never been seen
before, then the memory allocation fails and the hash is stored in memory and
written to disk. If the hash (i.e. the particular call stack) has been seen
before, then the normal libc version of the allocator is called as normal. Each
time the program starts, the hashes that have already been seen are loaded in
from disk.

The magic of the mallocfail library is that it does not require your program to
be modified, as long as debug symbols are available (i.e. no stripped
executables). You simply use the `LD_PRELOAD` linker environment variable to
tell the linker to use the malloc etc. implementations in mallocfail instead of
the libc versions.


Simple Usage
------------

To use mallocfail with your executable you need to set the `LD_PRELOAD`
environment variable.

    LD_PRELOAD=/usr/local/lib/mallocfail.so <your executable>

Note that the location of mallocfail.so may be different on your system,
substitute the path in that you have instead. If you have compiled yourself and
are still in the mallocfail directory, then use

    LD_PRELOAD=./mallocfail.so <your executable>

It is not recommended to set `LD_PRELOAD` by using `export` before running your
executable, because any subsequent command will be affected.

    export LD_PRELOAD=/usr/local/lib/mallocfail.so
    my_executable_test
    ssh myhost # this will certainly fail

Keep running your program until no more entries are being added to the hashes
file. It may take a lot of testing to achieve this.

You may find that your program crashes, or produces unexpected behaviour at
some point. If you want to repeat your test, remove the last line or last few
lines from the hashes file and repeat your program. It may be helpful to set
the `MALLOCFAIL_DEBUG` environment variable to give you more information on
what is happening.

    LD_PRELOAD=/usr/local/lib/mallocfail.so MALLOCFAIL_DEBUG=1 <your executable>

Using the debug option in this manner rather than setting it outright means you
are not bombarded with information on failures that are handled correctly. See
the `Environment Variables` section below for more information on
`MALLOCFAIL_DEBUG`.


Usage in GDB
------------

Start gdb as normal:

     gdb <your executable>

or

     gdb --args <your executable> <your executable arguments>

Then configure `LD_PRELOAD` in the gdb command window:

     set environment LD_PRELOAD /usr/local/lib/mallocfail.so

Then run your program

     run

You can then run multiple times to test all outcomes, debug any crashes and so
on.


Environment Variables
---------------------

You can control the behaviour of mallocfail with some environment variables.

`MALLOCFAIL_FILE` determines the file that hashes will be written to. Defaults
to `/tmp/mallocfail_hashes` if not set.

`MALLOCFAIL_DEBUG` - set to 1 to enable debugging information. This has the
effect of printing to stdout the stack traces of any allocations that are
forced to fail. Enabling this option means the stack trace is generated twice
per failed memory allocation, which may have a small effect on performance -
worth noting if this is critical to you.


Performance Impact
------------------

Allocated memory overhead associated with mallocfail is approximately a
constant 2kB on the stack plus 128 bytes on the heap per stored call stack on a
64-bit machine.


Limitations
-----------

The current implementation will probably fail badly on threaded programs.


Dependencies
------------

Troy D. Hanson's uthash (included in repository)
https://troydhanson.github.io/uthash/

Andrey Jivsov sha3 implementation (included in repository)
https://github.com/brainhub/SHA3IUF

Ian Taylor's libbacktrace
https://github.com/ianlancetaylor/libbacktrace
