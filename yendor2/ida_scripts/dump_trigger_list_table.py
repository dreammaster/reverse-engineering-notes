"""
Read-only: dumps IsPositionInTriggerList's own table (DS:0xD1C9,
8-byte stride, 0xFFFF-terminated) -- the general-purpose "special
map cell" table feeding both ApplyMapTriggerEffect (called once per
movement step) and IsRestingAllowedHere (the 'R rest' eligibility
check, already reimplemented as gameClockRestAllowed in
src23/gameclock.c, which currently takes isInTriggerList as an
already-resolved input since this table wasn't extracted yet).

IsPositionInTriggerList itself (yendor2.asm:17681) only ever reads
[di]/[di+2] (the coordinate value and a match-selector flag, bit
0x8000 choosing x vs y) -- but it leaves di pointing at the matched
entry on success (never restored before retn), and its only real
caller, ApplyMapTriggerEffect (yendor2.asm:17458), reads two more
words off that same di (+4/+6) depending on which of [di+2]'s other
bits are set. So the real per-entry record is the full 8 bytes, not
just the 4 IsPositionInTriggerList itself touches -- see
file-formats.md's "ApplyMapTriggerEffect" section (roadmap.md
candidate 10) for the full dispatch this table drives.

    .\run_ida_script.ps1 dump_trigger_list_table.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860


def u16(ea):
    return ida_bytes.get_word(ea)


def i16(ea):
    v = u16(ea)
    return v - 0x10000 if v >= 0x8000 else v


base = DS_BASE + 0xD1C9
lines = ["=== IsPositionInTriggerList's own table (0xD1C9, 8-byte stride) ==="]
for i in range(300):
    ea = base + i * 8
    coord = u16(ea)
    if coord == 0xFFFF:
        lines.append(f"  [{i:3d}] terminator (0xFFFF) at ea={ea:#x}")
        break
    flags = u16(ea + 2)
    w4 = i16(ea + 4)
    w6 = i16(ea + 6)
    axis = "y" if (flags & 0x8000) else "x"
    lines.append(f"  [{i:3d}] coord({axis})={coord:6d} flags={flags:#06x} +4={w4:6d} +6={w6:6d}")

out_path = r"C:\dev\yendor\yendor2\ida_scripts\trigger_list_dump.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
