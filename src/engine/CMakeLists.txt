# AGS engine reimplementation -- see src/PLAN.md for the overall
# strategy. Step 0: struct headers only, no behavior -- see
# include/ags/*.h. stub.c is the AGS_STUB()/AGS_STUB_VOID() logging
# convention every later milestone's not-yet-implemented functions
# use. M1 (src/PLAN.md): clib.c, the CLIB asset-library reader. M4:
# script_loader.c (a real, decoding fread_script port) + interp.c
# (ccCreateInstanceEx/ccCallInstance/cc_run_code, the 38-opcode
# script VM). M5: lzw.c (LZW/RLE decompression primitives) +
# room_loader.c (load_room/load_main_block's block-type dispatch).
# M6: gfx.c, the thinnest possible wiring onto Allegro's own real
# display pipeline (open a window, apply the room palette, blit the
# decompressed background). M7: sprite_loader.c (SpriteCache::
# initFile/loadSprite's real, uncompressed-sprite path) + loader.c
# gains ags_load_views (decoding ViewStruct272[] for real).

add_library(agsengine STATIC
  src/stub.c
  src/clib.c
  src/loader.c
  src/script_loader.c
  src/interp.c
  src/lzw.c
  src/room_loader.c
  src/gfx.c
  src/sprite_loader.c
)

target_include_directories(agsengine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(agsengine PUBLIC allegro)

add_subdirectory(tests)
