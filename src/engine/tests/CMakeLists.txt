# One small executable per milestone (see src/PLAN.md). Step 0 gets
# test_struct_sizes -- no real behavior yet, just proof every struct
# header lays out exactly as struct-layout-drift.md says it should.

add_executable(test_struct_sizes test_struct_sizes.c)
target_link_libraries(test_struct_sizes PRIVATE agsengine)

# Auto-generated from apply_structs.py -- see the generator note at the
# top of test_field_offsets.c. Every field's offsetof() checked against
# the original evidence directly, not just each struct's total size.
add_executable(test_field_offsets test_field_offsets.c)
target_link_libraries(test_field_offsets PRIVATE agsengine)

# M1 (src/PLAN.md): the CLIB manifest reader. Output format matches
# reversing/scripts/parse_clib_manifest.py's own exactly, for a direct
# diff against that already-verified Python prototype.
add_executable(test_clib_manifest test_clib_manifest.c)
target_link_libraries(test_clib_manifest PRIVATE agsengine)

# M2 (src/PLAN.md): the ac2game.dta header + GameSetupStructBase
# loader. Field list matches reversing/scripts/
# dump_gamesetup_from_data.py's own for a direct comparison.
add_executable(test_gamesetup test_gamesetup.c)
target_link_libraries(test_gamesetup PRIVATE agsengine)

# M3 (src/PLAN.md): "Meet the cast" -- walks past WordsDictionary/the
# compiled script/ViewStruct272[]/a second skip to reach and decode
# the real CharacterInfo array. Matches reversing/scripts/
# dump_characters_from_data.py's own (corrected) output.
add_executable(test_characters test_characters.c)
target_link_libraries(test_characters PRIVATE agsengine)

# M4 (src/PLAN.md): "Run the interpreter on nothing" -- reads the real
# compiled global script (ags_cc_read_script), creates an instance, and
# calls its game_start() export through the real 38-opcode VM
# (ags/interp.h), logging every native call the script makes via
# AGS_STUB.
add_executable(test_interpreter test_interpreter.c)
target_link_libraries(test_interpreter PRIVATE agsengine)

# M5 (src/PLAN.md): "Open a room" -- load_room/load_main_block's
# block-type dispatch, real LZW/RLE decompression, printing room 1's
# width/height/hotspot names/object count from the real .crm file.
add_executable(test_room test_room.c)
target_link_libraries(test_room PRIVATE agsengine)

# M6 (src/PLAN.md): "See the room" -- opens a real windowed Allegro
# graphics mode, applies the room's own palette, and blits its
# decompressed background onto the visible screen; saves a screenshot
# for after-the-fact visual verification.
add_executable(test_see_room test_see_room.c)
target_link_libraries(test_see_room PRIVATE agsengine)

# M7 (src/PLAN.md): "Meet the room's people" -- decodes ViewStruct272[]
# for real (ags_load_views), looks up the player character's current
# view/loop/frame sprite number, loads it (ags/sprite_loader.h) and
# their own real starting room, and draws their static pose at their
# own room position.
add_executable(test_meet_people test_meet_people.c)
target_link_libraries(test_meet_people PRIVATE agsengine)

# M8 (src/PLAN.md): "The world starts moving" -- a real mainloop with
# live keyboard/mouse polling (click to walk, ESC to quit), plus an
# automatic walk across room 1's own real walkable-area mask for
# scripted verification. See ags/walk.h's own file-level comment for
# this milestone's scoping decisions (straight-line movement only, no
# gradual-turning animation).
add_executable(test_walk_room test_walk_room.c)
target_link_libraries(test_walk_room PRIVATE agsengine)

# M9 (src/PLAN.md): "Say something" -- get_hotspot_at/
# RunHotspotInteraction/run_event_block's full respond[] dispatch
# (ags/interaction.h), showing a real DisplayMessage text box when a
# real hotspot with real interaction data is clicked.
add_executable(test_say_something test_say_something.c)
target_link_libraries(test_say_something PRIVATE agsengine)
