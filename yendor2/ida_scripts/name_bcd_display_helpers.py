"""
Names two small, reused "draw a label, then the BCD4 value only if
nonzero" display helpers, both used across ShowClueBookItemDetail
(BASE VALUE/WEIGHT) and ShowClueBookMonsterDetail (Health/Accuracy/
Dexterity/.../Power, 6 call sites each per rank_naming_candidates).

sub_147FF (-> DrawLabeledBCDIfNonzero): writeString(bx=label), then
IsBCDCounterAtLeast(si=value ptr, threshold 0 via word_3293E) -- if
nonzero, FormatAndDrawBCD4 draws it at a fixed x. Caller passes the
value pointer directly in si.

sub_14833 (-> DrawRecordFieldBCDIfNonzero): same shape, but first
copies a 4-byte field from the caller-supplied record (es:[bx]) into
a shared scratch location (word_5104) before the same "if nonzero,
format and draw" logic. Caller passes a record pointer in es:bx
instead of a value pointer directly.

Run via:
    .\run_ida_script.ps1 name_bcd_display_helpers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x147FF: "DrawLabeledBCDIfNonzero",
    0x14833: "DrawRecordFieldBCDIfNonzero",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x147FF,
    "Draws a label (bx=msg) then the BCD4 value at si only if nonzero "
    "(IsBCDCounterAtLeast vs threshold 0). Used by "
    "ShowClueBookItemDetail and ShowClueBookMonsterDetail for several "
    "stat fields.",
    False,
)
ida_bytes.set_cmt(
    0x14833,
    "Draws a label (bx=msg) then, copying a 4-byte field from a "
    "caller-supplied record (es:[bx]) into scratch word_5104, the BCD4 "
    "value only if nonzero. Same role as DrawLabeledBCDIfNonzero but "
    "takes a record pointer instead of a direct value pointer.",
    False,
)
