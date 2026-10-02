"""Turn `mov r16/m16, imm` / `push imm` whose immediate is the DGROUP offset of a defined string into a
proper offset operand (creates the data xref so strings show up in the listing and in reports).
Run on every code segment; safe to re-run."""
import idautils, ida_bytes, ida_offset, idc

DG = 0x286F0
DGEND = 0x37840
n = fixed = 0
for s in idautils.Segments():
    if not idc.get_segm_name(s).startswith(("seg0", "ovl")):
        continue
    for ea in idautils.Heads(s, idc.get_segm_end(s)):
        if not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            continue
        if idc.print_insn_mnem(ea) not in ("mov", "push"):
            continue
        for op in (0, 1):
            if idc.get_operand_type(ea, op) != idc.o_imm:
                continue
            v = idc.get_operand_value(ea, op)
            if not 0x40 <= v < DGEND - DG:
                continue
            t = DG + v
            if not ida_bytes.is_strlit(ida_bytes.get_flags(t)):
                # the game's text strings start with control bytes (03h centre, 0Dh, ...) that IDA leaves
                # out of the string item: re-create the string from the referenced address
                k = 1
                while k <= 3 and not ida_bytes.is_strlit(ida_bytes.get_flags(t + k)):
                    k += 1
                if k > 3 or not all(0 < ida_bytes.get_byte(t + i) < 0x20 for i in range(k)):
                    continue
                if ida_bytes.get_byte(t - 1) != 0 and not ida_bytes.is_strlit(ida_bytes.get_flags(t - 1)):
                    pass
                ida_bytes.del_items(t, 0, k)
                if not idc.create_strlit(t, t + k + 1):
                    continue
                fixed += 1
            if ida_offset.op_plain_offset(ea, op, DG):
                n += 1
print("string offsets resolved:", n, "strings extended:", fixed)
