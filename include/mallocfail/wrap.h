#ifndef MALLOCFAIL_WRAP_H
#define MALLOCFAIL_WRAP_H

#ifdef __cplusplus
extern "C" {
#endif

void mallocfailwrap_init(const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
