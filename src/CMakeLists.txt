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

# All the third-party libraries under libs/ are 2002-era C, written against
# compiler defaults that have since changed twice in ways that matter here:
#  - GCC 10 flipped -fcommon to -fno-common by default. Old C commonly
#    relies on tentative definitions (a global declared without "extern" in
#    a header, included by several .c files with no matching real
#    definition anywhere) being merged by the linker as COMMON symbols --
#    -fno-common instead makes that a "multiple definition" LINK error.
#  - GCC 14 promoted implicit-function-declaration/incompatible-pointer-
#    types/int-conversion from warnings to hard COMPILE errors by default.
# Reverting both to their pre-GCC-10/pre-GCC-14 behavior lets this codebase
# build against a modern toolchain without a patch for every individual
# call site; genuine correctness issues (wrong signatures, real linkage
# conflicts) are still fixed in the source itself as they're found.
add_compile_options(
  -fcommon
  -fgnu89-inline        # Allegro's AL_INLINE macro uses the classic GNU89
                        # "extern inline" idiom (see include/allegro/
                        # internal/alconfig.h), meaning "inline this; if you
                        # can't, trust some OTHER TU to provide the one real
                        # definition". GCC 5+ defaults to C99 inline
                        # semantics instead, where "extern inline" means the
                        # opposite -- THIS TU provides a real, externally-
                        # visible definition -- so every .c file that
                        # includes allegro.h ends up emitting its own full
                        # copy of hline/draw_sprite/dot_product/etc., which
                        # collide with "multiple definition" link errors as
                        # soon as two such object files are linked together.
                        # This flag restores the original (intended) meaning.
  -Wno-error=implicit-function-declaration
  -Wno-error=incompatible-pointer-types
  -Wno-error=int-conversion
  -Wno-error=implicit-int
)

add_subdirectory(libs)

# Smoke-test executable: links every third-party library and exercises one
# real entry point from each, to prove the whole toolchain/library stack
# actually builds and links before any engine reimplementation code is
# written against it. Not a real test of behavior -- see engine/ (once it
# exists) for that.
add_executable(smoketest smoketest.c)
target_link_libraries(smoketest PRIVATE allegro jgmod almp3 libcda)
