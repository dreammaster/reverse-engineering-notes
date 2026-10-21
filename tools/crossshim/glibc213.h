// The libstdc++ headers of GCC 8 use a few functions that glibc 2.13 (Debian 7) does not declare; only declared here, to compile
#include <stdlib.h>
#include <time.h>
extern "C" void *aligned_alloc(size_t alignment, size_t size);
extern "C" int timespec_get(struct timespec *ts, int base);
