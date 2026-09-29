"""
Read-only: dumps TravelToDestination's own destination table
(DS:0xD40B, 16-byte stride) and IsDestinationUnlocked's gate table
(DS:0xDFBB, 22-byte stride) -- both embedded statically in the EXE's
own data segment (not WORLD.DAT), same pattern as g_trapEffectDefs/the
XP threshold table. Resolves this project's long-open "region/town
password" question (see docs23/roadmap.md's "Open questions" section):
these are exactly the party's teleport/fast-travel destination
coordinates, reached from `start`'s own WorldObjectFlagUnknown2000
branch (a previously-untested world-object flag, worldobjects.h) via
`ProbeFacingTile` -> `TravelToDestination(ax=[si+4], the object's own
value field)`. The gate table's "no override message" rows
(hasMsg==0) turn out to be a real typed PASSWORD check (see
IsDestinationUnlocked, yendor2.asm:18170/18260): a fixed word/phrase,
compared character-by-character (space = end-of-word wildcard) against
whatever the player typed into a prompt, embedded directly in the gate
table row itself (+0xA, up to 12 bytes) -- this dump reads and prints
that text.

The destination table's exact length is confirmed structural, not
guessed: dest_base (0xD40B) + 187*16 == gate_base (0xDFBB) exactly --
the destination table runs right up against IsDestinationUnlocked's
own table with zero gap, so 187 is the real, exact count (matching
WorldObjectFlagUnknown2000's own 187 reachable Chapter 2 records, 1:1
with the destination table). This dump still reads a few rows past
187 (clearly garbage -- the start of the gate table's own bytes
misread as a destination record) so that's visible directly rather
than just asserted.

    .\run_ida_script.ps1 dump_travel_destination_table.py -NoExport
"""
import idc
import ida_bytes

DS_BASE = 0x2D860

DEST_COUNT = 187
GATE_STRIDE = 22


def u16(ea):
    return ida_bytes.get_word(ea)


def i16(ea):
    v = u16(ea)
    return v - 0x10000 if v >= 0x8000 else v


def read_password_text(ea, max_len=12):
    chars = []
    for i in range(max_len):
        b = ida_bytes.get_byte(ea + i)
        if b == 0x20:  # space = end-of-word wildcard, matches IsDestinationUnlocked's own compare loop
            break
        chars.append(chr(b) if 0x20 <= b < 0x7F else f"\\x{b:02x}")
    return "".join(chars)


dest_base = DS_BASE + 0xD40B
gate_base = DS_BASE + 0xDFBB
assert dest_base + DEST_COUNT * 16 == gate_base, "destination table no longer runs exactly up to the gate table"

lines = [f"=== Destination table (0xD40B, 16-byte stride, exactly {DEST_COUNT} entries -- table end == gate table start) ==="]
for i in range(DEST_COUNT + 5):  # a few past the real end, deliberately, to show the garbage transition
    ea = dest_base + i * 16
    worldX = i16(ea)
    worldY = i16(ea + 2)
    facing = u16(ea + 4)
    sound = u16(ea + 6)
    unused8 = u16(ea + 8)  # never read by Chapter 2's TravelToDestination
    dayMusic = u16(ea + 0xA)   # word_36CB1 -- confirmed day-track id (see 0x28320's day/night music picker)
    nightMusic = u16(ea + 0xC)  # word_36CB3 -- confirmed night-track id, same picker
    flags = u16(ea + 0xE)
    marker = "  <-- past the real 187" if i >= DEST_COUNT else ""
    lines.append(f"  [{i + 1:3d}] x={worldX:6d} y={worldY:6d} facing={facing:#06x} sound={sound:#06x} "
                 f"+8={unused8:#06x} dayMusic={dayMusic:#06x} nightMusic={nightMusic:#06x} flags={flags:#06x}{marker}")

lines.append("\n=== IsDestinationUnlocked's own gate table (0xDFBB, 22-byte stride) ===")
for i in range(60):
    ea = gate_base + i * GATE_STRIDE
    destId = u16(ea)
    if destId == 0xFFFF:
        lines.append(f"  [{i:3d}] terminator (0xFFFF) at ea={ea:#x}")
        break
    flagPtr = u16(ea + 2)
    mask = u16(ea + 4)
    promptId = u16(ea + 6)
    hasMsg = u16(ea + 8)
    if hasMsg:
        lines.append(f"  [{i:3d}] destId={destId:3d} flagPtrRaw={flagPtr:#06x} mask={mask:#06x} "
                      f"promptId={promptId:#06x} hasMsg={hasMsg:#06x} (message-override row, no password)")
    else:
        password = read_password_text(ea + 0xA)
        lines.append(f"  [{i:3d}] destId={destId:3d} flagPtrRaw={flagPtr:#06x} mask={mask:#06x} "
                      f"promptId={promptId:#06x} hasMsg=0 password={password!r}")

out_path = r"C:\dev\yendor\yendor2\ida_scripts\travel_destination_dump.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
