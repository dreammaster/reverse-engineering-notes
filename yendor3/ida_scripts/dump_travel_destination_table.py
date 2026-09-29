"""
Read-only: Chapter 3's sibling of yendor2's
dump_travel_destination_table.py. TravelToDestination (yendor3.asm:10081)
uses a materially different, 18-byte-stride record at DS:0xBA95 --
2 bytes wider than Chapter 2's, and reorganized: the "travel mode" is a
plain value field (+0xC, copied verbatim to a mode global) rather than
Chapter 2's mutually-exclusive flag bits, and there's no equivalent of
Chapter 2's hardcoded landing-spot override. IsDestinationUnlocked
(yendor3.asm:10142) keeps the same 22-byte gate-table stride as Chapter
2 but is genuinely richer: past the gate table's own flag-check/message
paths, it re-tests the ORIGINAL destination record's own +0xE bits
(0x4000/0x8000) to choose between a silent 3-line deny message and the
same typed-password check Chapter 2 has -- a real per-game structural
difference in when the password path is reached, not just relocated
data.

Exact table length, confirmed the same way as Chapter 2's: dest_base
(0xBA95) + 139*18 == gate_base (0xC45B) exactly -- matching
WorldObjectFlagUnknown2000's own 139 reachable Chapter 3 records.

    .\run_ida_script.ps1 dump_travel_destination_table.py -NoExport
"""
import idc
import ida_bytes
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

DEST_COUNT = 139
DEST_STRIDE = 18
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


dest_base = DS_BASE + 0xBA95
gate_base = DS_BASE + 0xC45B
assert dest_base + DEST_COUNT * DEST_STRIDE == gate_base, "destination table no longer runs exactly up to the gate table"

lines = [f"=== Destination table (0xBA95, 18-byte stride, exactly {DEST_COUNT} entries -- table end == gate table start) ==="]
for i in range(DEST_COUNT + 5):
    ea = dest_base + i * DEST_STRIDE
    worldX = i16(ea)
    worldY = i16(ea + 2)
    facing = u16(ea + 4)
    sound = u16(ea + 6)
    field8 = u16(ea + 8)     # -> ds:0xCF2F, read AFTER RefreshDungeonMapWindow (unlike Ch2's dayMusic slot)
    fieldA = u16(ea + 0xA)   # -> ds:0xCF31, same position
    mode = u16(ea + 0xC)     # -> ds:0xCF33 ("mode" global, read verbatim, not bit-tested)
    field10 = u16(ea + 0x10)  # -> ds:0xCEF9 whole word; bit 0 also gates zeroing ds:0xCF3F
    flags = u16(ea + 0xE)    # gate bits 0xC000 (0x4000|0x8000) tested by TravelToDestination itself
    marker = "  <-- past the real 139" if i >= DEST_COUNT else ""
    lines.append(f"  [{i + 1:3d}] x={worldX:6d} y={worldY:6d} facing={facing:#06x} sound={sound:#06x} "
                 f"+8={field8:#06x} +A={fieldA:#06x} mode={mode:#06x} flags={flags:#06x} +10={field10:#06x}{marker}")

lines.append("\n=== IsDestinationUnlocked's own gate table (0xC45B, 22-byte stride) ===")
for i in range(40):
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

out_path = r"C:\dev\yendor\yendor3\ida_scripts\travel_destination_dump.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
