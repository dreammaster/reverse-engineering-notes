#include "Diagnostics.h"

#include <cstdio>

void x_assert(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        std::fprintf(stderr, "assertion failed: %s (%s:%d)\n", expression, file, line);
    }
}
