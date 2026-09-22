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
# no sound/speech assets at all). M11 (long tail, GUI rendering
# slice): gui_loader.c (read_gui/GUIMain::rebuild_array, plus
# loader.c's own new messages[500]/DialogTopic/dlgmessage steps
# needed to REACH the real on-disk GUI data) + gui_render.c
# (GUIMain::draw_at plus a scoped Button/Label Draw() subset -- see
# ags/gui_render.h's own file-level comment for the real-vs-stubbed
# split). dialog_run.c (M11 long-tail, dialog system slice):
# run_dialog_script's own full DCMD_* byte-code interpreter -- see
# ags/dialog_run.h's own file-level comment for the complete opcode
# table and evidence. dialog_menu.c (M11 long-tail, options-menu UI):
# a real, data-driven do_conversation options-menu renderer -- see
# ags/dialog_menu.h's own file-level comment for the deliberate scope
# decision vs. a byte-exact do_conversation port. inventory.c (M11
# long-tail, inventory): the shared add_inventory/LoseInventory/
# update_invorder/SetActiveInventory core (ags/inventory.h).
# invscreen.c: a real, data-driven __actual_invscreen stand-in (ags/
# invscreen.h) -- see that header's own scope decision vs. the
# original's pixel-exact 1133-line legacy screen. saveload.c (M11
# long-tail, save/restore): a real, scoped SaveGameSlot/
# restore_game_data port -- see ags/saveload.h's own file-level
# comment for the exact real file format and scope decision.

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
  src/gui_loader.c
  src/gui_render.c
  src/dialog_run.c
  src/dialog_menu.c
  src/inventory.c
  src/invscreen.c
  src/saveload.c
)

target_include_directories(agsengine PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(agsengine PUBLIC allegro)

add_subdirectory(tests)
