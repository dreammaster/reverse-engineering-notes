# AGS engine reimplementation -- see src/PLAN.md for the overall
# strategy. Step 0 (this target so far): struct headers only, no
# behavior yet -- see include/ags/*.h. stub.c is the one real .c file
# so far (the AGS_STUB()/AGS_STUB_VOID() logging convention every
# later milestone's not-yet-implemented functions will use).

add_library(agsengine STATIC
  src/stub.c
)

target_include_directories(agsengine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(agsengine PUBLIC allegro)

add_subdirectory(tests)
