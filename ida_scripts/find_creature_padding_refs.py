"""
Read-only helper: exhaustively checks whether Creature.field_A/field_C/
field_E (the 3 unnamed words in the 16-byte Creature struct, used for
Savegame._overworldEntities) are actually referenced anywhere, beyond
what a simple symbolic-name grep of the .asm would catch.

Simple grepping for "Creature.field_A" only catches accesses IDA has
already resolved to a symbol -- exactly the kind of gap that turned out
to hide _hits/_strength's false "mixup" in GEN.EXE (see
apply_structs_gen.py). This script instead computes the raw immediate
value an `[si+CONST]`-style array access would need to reach each field
(dseg's base + _savegame's offset-in-dseg + the field's struct offset)
and scans every instruction's raw operand values for a match, so it
also catches unresolved/unnamed accesses.

Result as of 2026-08-25: zero hits in both OUT.EXE and SPACE.EXE (the
two executables that actually use the Creature array) -- these 3 words
are confirmed unused, not just unreferenced-by-name. Renamed to
_unused1/_unused2/_unused3 in apply_structs_savegame.py.

    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName find_creature_padding_refs.py -NoExport
    .\\run_ida_script.ps1 -Idb ultima1_space -ScriptName find_creature_padding_refs.py -NoExport
"""

import idc, ida_segment, ida_struct, idautils, ida_bytes

dseg = None
for seg_ea in idautils.Segments():
    seg = ida_segment.getseg(seg_ea)
    if ida_segment.get_segm_class(seg) == "DATA":
        dseg = seg
        break
dseg_base = dseg.start_ea
sg = idc.get_name_ea_simple("_savegame")
sg_dseg_off = sg - dseg_base
print(f"dseg base={dseg_base:#x}  _savegame ea={sg:#x}  offset-in-dseg={sg_dseg_off:#x}")

# Creature array starts at Savegame+0xB4, element size 0x10
ARRAY_OFF = 0xB4
ELEM_SIZE = 0x10
FIELD_OFFS = {"field_A": 0xA, "field_C": 0xC, "field_E": 0xE}

for fname, foff in FIELD_OFFS.items():
    raw_val = sg_dseg_off + ARRAY_OFF + foff
    target_ea = raw_val + dseg_base
    print(f"\n{fname}: raw immediate to search for = {raw_val:#x}  (target ea if resolved = {target_ea:#x})")
    # scan the whole executable's code segments for this immediate operand value
    hits = []
    for seg_ea in idautils.Segments():
        seg = ida_segment.getseg(seg_ea)
        if ida_segment.get_segm_class(seg) != "CODE":
            continue
        ea = seg_ea
        end = seg.end_ea
        cur = ea
        while cur < end and cur != idc.BADADDR:
            if idc.is_code(ida_bytes.get_full_flags(cur)):
                for n in (0, 1):
                    try:
                        v = idc.get_operand_value(cur, n)
                    except Exception:
                        v = -1
                    if v == raw_val:
                        hits.append((cur, n))
            cur = idc.next_head(cur, end)
    print(f"  found {len(hits)} instruction(s) with this raw immediate:")
    for ea, n in hits[:20]:
        print(f"    {ea:#x} op{n}: {idc.generate_disasm_line(ea, 0)}")
