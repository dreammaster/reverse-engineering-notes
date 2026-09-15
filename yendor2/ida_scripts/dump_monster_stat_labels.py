"""
Dump all the field-label messages used by sub_141D9 (the F2 Monster
Statistics clue-book detail panel, called from
RunClueBookMonsterCategory but not yet named itself). Read-only --
just documenting what stats the screen shows, without mapping each to
a specific record offset (that would need tracing sub_14833/
sub_148B2/sub_1496B's argument conventions, left for a future round).

Run via:
    .\\run_ida_script.ps1 dump_monster_stat_labels.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=40):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

offsets = [0x88C0, 0x7DBB, 0x890B, 0x7C61, 0x7C6D, 0x8911, 0x8919,
           0x8923, 0x892E, 0x893A, 0x8942, 0x894F, 0x895C, 0x8964,
           0x896D, 0x8978, 0x8982, 0x898A, 0x8993, 0x8999, 0x899F,
           0x89A9]
for off in offsets:
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
