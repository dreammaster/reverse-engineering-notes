cmake_minimum_required(VERSION 3.16)
project(rob_blanc_1_reimpl C)

# See src/cmake/toolchain-mingw32.cmake for why this project targets
# 32-bit Windows specifically -- configure with:
#   cmake -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake -S . -B build
if(NOT (CMAKE_SIZEOF_VOID_P EQUAL 4))
  message(WARNING
    "This project's third-party libraries (Allegro 4.0.2/JGMOD/ALMP3/libcda) "
    "assume a 32-bit build throughout, matching the original Rob Blanc 1 "
    "binary. Configure with -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake "
    "to build 32-bit; continuing anyway, but expect trouble.")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS ON)

# -fgnu89-inline applies HERE (top level, affecting libs/ AND engine/):
# Allegro's own AL_INLINE macro uses the classic GNU89 "extern inline"
# idiom (see libs/allegro-4.0.2/include/allegro/internal/alconfig.h),
# meaning "inline this; if you can't, trust some OTHER TU to provide
# the one real definition". GCC 5+ defaults to C99 inline semantics
# instead, where "extern inline" means the opposite -- THIS TU
# provides a real, externally-visible definition -- so ANY .c file
# that includes allegro.h (ours or the third-party libraries') risks
# emitting its own full copy of hline/draw_sprite/dot_product/etc.,
# colliding with "multiple definition" as soon as two such object
# files are linked together. This isn't a 2002-era-code-tolerance
# flag like libs/CMakeLists.txt's own (-fcommon, -Wno-error=...) --
# it's Allegro's own header design requiring gnu89 semantics from
# every consumer, including our own fresh engine/ code, so it's
# scoped globally rather than to libs/ alone.
add_compile_options(-fgnu89-inline)

add_subdirectory(libs)
add_subdirectory(engine)

# Smoke-test executable: links every third-party library and exercises one
# real entry point from each, to prove the whole toolchain/library stack
# actually builds and links before any engine reimplementation code is
# written against it. Not a real test of behavior -- see engine/ (once it
# exists) for that.
add_executable(smoketest smoketest.c)
target_link_libraries(smoketest PRIVATE allegro jgmod almp3 libcda)
