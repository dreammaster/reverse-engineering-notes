# JGMOD -- static library build for the ags/src reimplementation.
# A module-music (.MOD/.XM/.S3M/.IT) player library, built on Allegro.
#
# jgm.c ("For converting MODs to JGM") and jgmod.c ("The player. Just to
# demonstrate how JGMOD sounds.") are both standalone CLI-tool/demo
# sources with their own main()/END_OF_MAIN() -- despite living
# top-level alongside the real library sources, they are not part of
# JGMOD's own reusable library at all. Both define main(), so a naive
# glob silently drags one of their competing entry points into any
# executable that links this library (MinGW's PE/COFF ld tolerates
# duplicate strong symbols across objects with no error, unlike ELF --
# it just keeps whichever definition it processes last, which is not
# necessarily the "real" one).

file(GLOB JGMOD_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
list(REMOVE_ITEM JGMOD_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/jgm.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/jgmod.c"
)

add_library(jgmod STATIC
  ${JGMOD_SOURCES}
)

target_include_directories(jgmod PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(jgmod PUBLIC allegro)
