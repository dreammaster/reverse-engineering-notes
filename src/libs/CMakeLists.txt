# Third-party libraries actually linked into Rob Blanc 1's own binary
# (see reversing/notes/third-party-library-identification.md for the full
# evidence trail). apeg-1.2.1 and dumb-0.9.2 are deliberately NOT here --
# both are conclusively ruled out (released after the binary's own
# 2002-07-21 link date). aastr-0.1.1/hq2x are also omitted: no evidence
# anywhere in the disassembly that either is actually used by this game's
# engine build.
#
# All of it is 2002-era C, written against compiler defaults that have
# since changed in ways that matter here (GCC 10 flipped -fcommon to
# -fno-common by default -- old C commonly relies on tentative
# definitions being merged by the linker as COMMON symbols, which
# -fno-common instead makes a "multiple definition" LINK error; GCC 14
# promoted implicit-function-declaration/incompatible-pointer-types/
# int-conversion from warnings to hard COMPILE errors by default).
# Reverting both to their pre-GCC-10/pre-GCC-14 behavior lets this
# codebase build against a modern toolchain without a patch for every
# individual call site; genuine correctness issues (wrong signatures,
# real linkage conflicts) are still fixed in the source itself as
# they're found. Scoped to this directory tree only (not src/engine/
# -- our own fresh code gets the compiler's normal, un-relaxed error
# strictness). -fgnu89-inline is a separate matter and applies
# globally -- see the top-level CMakeLists.txt.
add_compile_options(
  -fcommon
  -Wno-error=implicit-function-declaration
  -Wno-error=incompatible-pointer-types
  -Wno-error=int-conversion
  -Wno-error=implicit-int
)

add_subdirectory(allegro-4.0.2)
add_subdirectory(jgmod)
add_subdirectory(almp3-2.0.5)
add_subdirectory(libcda-0.4)
