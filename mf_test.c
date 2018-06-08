#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void c2(char *argv)
{
	char *s = strdup(argv);
	printf("call 3a: %p\n", malloc(10));
	printf("call 3b: %p\n", realloc(NULL, 10));
	printf("call 3c: %p\n", s);
}


void c1(char *argv)
{
	printf("call 2a: %p\n", malloc(10));
	printf("call 2b: %p\n", realloc(NULL, 10));
	printf("call 2c: %p\n", strdup(argv));
	c2(argv);
}


int main(int argc, char *argv[])
{
	printf("call 1a: %p\n", malloc(10));
	printf("call 1b: %p\n", realloc(NULL, 10));
	printf("call 1c: %p\n", strdup(argv[0]));
	c1(argv[0]);
	c1(argv[0]);
	return 0;
}

