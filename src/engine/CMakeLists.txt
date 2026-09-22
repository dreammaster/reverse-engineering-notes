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
# gains ags_load_views (decoding ViewStruct272[] for real). M8: walk.c
# (straight-line walk_character/find_route/calculate_move_stage/
# do_movelist_move/fix_player_sprite, see ags/walk.h's own file-level
# comment for scope). M9: interaction.c (get_hotspot_at/
# RunHotspotInteraction/run_event_block's full respond[] dispatch,
# plus a minimal real DisplayMessage text-box renderer -- see
# ags/interaction.h's own file-level comment for the real-vs-stubbed
# split). M10: audio.c (PlayMusic/PlaySound/PlaySpeech, see
# ags/audio.h's own file-level comment for the real-vs-stubbed split
# -- Rob Blanc 1's own real data ships only two MIDI music files and
# no sound/speech assets at all).

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
  src/walk.c
  src/interaction.c
  src/audio.c
)

target_include_directories(agsengine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(agsengine PUBLIC allegro)

add_subdirectory(tests)
