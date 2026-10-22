"""
Dump SW.EXE's file descriptor table: records of 19h bytes in seg019 at 08FAh..
{+0 handle, +2 buffer off, +4 ?, +6 record size, +8 record number, +0Ah/+0Ch dword offset, +0Eh ASCIZ name}.
Read-only; prints to the batch log.
"""
import idc, ida_segment
seg = ida_segment.get_segm_by_name("seg019")
base = (seg.start_ea >> 4) << 4
OFFS = [0x8FA, 0x913, 0x92C, 0x945, 0x95E, 0x977, 0x990,       # FILE00..06.16
        0x9A9, 0x9CA, 0x9E9, 0xA0A, 0xA2B, 0xA43, 0xA5A,       # PARTY NPC AREA BATPIC WORLD TOWN MINE
        0xE71, 0xE8B, 0xEA5]                                     # palette files
for off in OFFS:
    ea = base + off
    name = idc.get_strlit_contents(ea + 0x0E, -1, idc.STRTYPE_C)
    w = [idc.get_wide_word(ea + 2 * i) for i in range(7)]
    print(f"{off:#06x} name={(name or b'?').decode('latin1'):24s} buf={w[1]:#06x} w2={w[2]:#06x} recsize={w[3]:5d} recno={w[4]} +0A={w[5]:#06x} +0C={w[6]:#06x}")
