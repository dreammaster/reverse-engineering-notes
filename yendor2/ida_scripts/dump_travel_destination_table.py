"""
Read-only: dumps TravelToDestination's own destination table
(DS:0xD40B, 16-byte stride) -- embedded statically in the EXE's own
data segment (not WORLD.DAT), same pattern as g_trapEffectDefs/the XP
threshold table. Resolves this project's long-open "region/town
password" question (see docs23/roadmap.md's "Open questions" section):
these are exactly the party's teleport/fast-travel destination
coordinates, reached from `start`'s own WorldObjectFlagUnknown2000
branch (a previously-untested world-object flag, worldobjects.h) via
`ProbeFacingTile` -> `TravelToDestination(ax=[si+4], the object's own
value field)`.

Confirmed the real table runs to roughly 187 entries before the data
stops looking like coordinates (facing stops being one of the 4
SaveFacing bits, values go out of any plausible world-map range) --
not yet pinned to an exact IDA-declared boundary, since no length
constant or terminator sentinel was found; this dump is deliberately
generous (well past 187) so a future session can see exactly where it
degrades rather than trusting a guessed cutoff.

    .\run_ida_script.ps1 dump_travel_destination_table.py -NoExport
"""
import idc
import ida_bytes

DS_BASE = 0x2D860


def u16(ea):
    return ida_bytes.get_word(ea)


def i16(ea):
    v = u16(ea)
    return v - 0x10000 if v >= 0x8000 else v


dest_base = DS_BASE + 0xD40B
lines = ["=== Destination table (0xD40B, 16-byte stride) ==="]
for i in range(200):
    ea = dest_base + i * 16
    worldX = i16(ea)
    worldY = i16(ea + 2)
    facing = u16(ea + 4)
    sound = u16(ea + 6)
    w8 = u16(ea + 8)
    wA = u16(ea + 0xA)
    wC = u16(ea + 0xC)
    flags = u16(ea + 0xE)
    lines.append(f"  [{i + 1:3d}] x={worldX:6d} y={worldY:6d} facing={facing:#06x} sound={sound:#06x} "
                 f"+8={w8:#06x} +A={wA:#06x} +C={wC:#06x} flags={flags:#06x}")

lines.append("\n=== IsDestinationUnlocked's own gate table (0xDFBB, 22-byte stride) ===")
gate_base = DS_BASE + 0xDFBB
for i in range(60):
    ea = gate_base + i * 22
    destId = u16(ea)
    if destId == 0xFFFF:
        lines.append(f"  [{i:3d}] terminator (0xFFFF) at ea={ea:#x}")
        break
    flagPtr = u16(ea + 2)
    mask = u16(ea + 4)
    hasMsg = u16(ea + 8)
    lines.append(f"  [{i:3d}] destId={destId:3d} flagPtrRaw={flagPtr:#06x} mask={mask:#06x} hasMsg={hasMsg:#06x}")

out_path = r"C:\dev\yendor\yendor2\ida_scripts\travel_destination_dump.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
