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
