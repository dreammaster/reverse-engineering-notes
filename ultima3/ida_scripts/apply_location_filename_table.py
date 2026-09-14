"""Name the 19-entry per-location filename-pointer table in
ultima_exodus.idb at linear 0x116BB (confirmed via cmdEnter's
[si+16BBh] access, si = word-offset from the LOCATION_TILE_TABLE scan
match). Sits immediately before LOCATION_TILE_TABLE (0x116E1) --
19 words (38 bytes) of filename-pointer, then 19 words of position,
one contiguous struct-of-arrays layout for all 19 named overworld
locations. Each pointer resolves to a real confirmed .ULT filename
string, matching real Ultima III geography exactly (Devil Guard,
Fire/Time dungeons matching the Marks/Time-Lord lore already
confirmed elsewhere this session, etc.)."""
import idc
import ida_bytes

TABLE = 0x116BB
COUNT = 19

ida_bytes.del_items(TABLE, ida_bytes.DELIT_SIMPLE, COUNT * 2)
ok = ida_bytes.create_data(TABLE, ida_bytes.word_flag(), COUNT * 2, idc.BADADDR)
print(f"create_data({TABLE:#x}, {COUNT} words) -> {ok}")
renamed = idc.set_name(TABLE, "LOCATION_FILENAME_TABLE", idc.SN_NOWARN)
print(f"set_name -> {renamed}")
idc.make_array(TABLE, COUNT)

for i in range(COUNT):
    ptr = idc.get_wide_word(TABLE + i * 2)
    linear = 0x10000 + ptr
    s = idc.get_strlit_contents(linear, -1, idc.STRTYPE_C)
    print(f"  [{i:2d}] -> {linear:#x}: {s}")
