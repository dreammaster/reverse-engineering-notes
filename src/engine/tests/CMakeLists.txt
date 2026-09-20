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
