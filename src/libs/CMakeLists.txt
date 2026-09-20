# Third-party libraries actually linked into Rob Blanc 1's own binary
# (see reversing/notes/third-party-library-identification.md for the full
# evidence trail). apeg-1.2.1 and dumb-0.9.2 are deliberately NOT here --
# both are conclusively ruled out (released after the binary's own
# 2002-07-21 link date). aastr-0.1.1/hq2x are also omitted: no evidence
# anywhere in the disassembly that either is actually used by this game's
# engine build.

add_subdirectory(allegro-4.0.2)
add_subdirectory(jgmod)
add_subdirectory(almp3-2.0.5)
add_subdirectory(libcda-0.4)
