"""
Read-only: Chapter 3's sibling of yendor2's dump_trigger_list_table.py.
IsPositionInTriggerList's own table (DS:0xB71F, 20-byte stride --
genuinely wider than Chapter 2's 8-byte record, matching the richer
teleport-branch field population ApplyMapTriggerEffect shows for
Chapter 3, see file-formats.md's "ApplyMapTriggerEffect" section).
0xFFFF-terminated, same convention as Chapter 2.

    .\run_ida_script.ps1 dump_trigger_list_table.py -NoExport
"""
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF


def u16(ea):
    return ida_bytes.get_word(ea)


def i16(ea):
    v = u16(ea)
    return v - 0x10000 if v >= 0x8000 else v


base = DS_BASE + 0xB71F
STRIDE = 20
lines = [f"=== IsPositionInTriggerList's own table (0xB71F, {STRIDE}-byte stride) ==="]
for i in range(300):
    ea = base + i * STRIDE
    coord = u16(ea)
    if coord == 0xFFFF:
        lines.append(f"  [{i:3d}] terminator (0xFFFF) at ea={ea:#x}")
        break
    flags = u16(ea + 2)
    words = [i16(ea + 4 + 2 * k) for k in range((STRIDE - 4) // 2)]
    wordsStr = " ".join(f"{w:6d}" for w in words)
    lines.append(f"  [{i:3d}] coord={coord:6d} flags={flags:#06x} rest=[{wordsStr}]")

out_path = r"C:\dev\yendor\yendor3\ida_scripts\trigger_list_dump.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
