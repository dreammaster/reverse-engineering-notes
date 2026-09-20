# AGS engine reimplementation -- see src/PLAN.md for the overall
# strategy. Step 0: struct headers only, no behavior -- see
# include/ags/*.h. stub.c is the AGS_STUB()/AGS_STUB_VOID() logging
# convention every later milestone's not-yet-implemented functions
# use. M1 (src/PLAN.md): clib.c, the CLIB asset-library reader.

add_library(agsengine STATIC
  src/stub.c
  src/clib.c
  src/loader.c
)

target_include_directories(agsengine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(agsengine PUBLIC allegro)

add_subdirectory(tests)
