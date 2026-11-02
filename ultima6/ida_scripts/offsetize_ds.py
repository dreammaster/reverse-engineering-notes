"""
Large-model code passes data pointers as plain immediates (`push ds / mov ax, 57Eh /
push ax`), so IDA shows neither the string nor a reference.  This script converts
such immediates into `offset` operands relative to the data segment (creating the
string literal where needed), which makes the listing readable and gives every
string and table cross references.

An immediate is converted only when it is clearly a pointer:
  * it follows `push ds` (far pointer argument), or
  * it is pushed as the argument of a file_* function (near pointer to a file name),
  and the target is either a NUL-terminated printable string or already named.

Idempotent.  Run after the renames:  .\\run_ida_script.ps1 -Idb u6_game -ScriptName offsetize_ds.py
"""

import idautils
import idc
import ida_bytes
import ida_name
import ida_segment

dseg = ida_segment.get_segm_by_name("dseg")
DS_BASE = dseg.start_ea
FILE_FUNCS = ("file_open", "file_create", "file_delete", "file_rename", "file_read", "file_write")


def is_string_start(ea):
    if ida_bytes.get_byte(ea - 1) != 0:
        return 0
    raw = ida_bytes.get_bytes(ea, 100)
    if not raw:
        return 0
    n = raw.find(bytes([0]))
    if n < 3:
        return 0
    if any((c < 32 or c > 126) and c not in (9, 10, 13) for c in raw[:n]):
        return 0
    return n


def is_named(ea):
    return ida_name.get_name(ea) and not ida_name.is_dummy_name(ida_bytes.get_flags(ea)) if hasattr(ida_name, "is_dummy_name") \
        else idc.get_name(ea, 0) and not idc.get_name(ea, 0).startswith(("byte_", "word_", "dword_", "unk_", "off_", "stru_", "a"))


converted = strings = 0
for fstart in idautils.Functions():
    prev = None
    items = list(idautils.FuncItems(fstart))
    for i, head in enumerate(items):
        mnem = idc.print_insn_mnem(head)
        op = None
        if mnem == "mov" and idc.get_operand_type(head, 1) == idc.o_imm and idc.get_operand_type(head, 0) == idc.o_reg:
            op = 1
        elif mnem == "push" and idc.get_operand_type(head, 0) == idc.o_imm:
            op = 0
        if op is not None:
            v = idc.get_operand_value(head, op) & 0xFFFF      # IDA sign-extends immediates >= 8000h
            ea = DS_BASE + v
            if 0x10 <= v < dseg.end_ea - DS_BASE:
                far_arg = i > 0 and idc.print_insn_mnem(items[i - 1]) == "push" and idc.print_operand(items[i - 1], 0) == "ds"
                file_arg = False
                if not far_arg and mnem == "mov" and i + 2 < len(items):
                    if idc.print_insn_mnem(items[i + 1]) == "push" and idc.print_insn_mnem(items[i + 2]) == "call":
                        file_arg = idc.print_operand(items[i + 2], 0) in FILE_FUNCS
                n = is_string_start(ea)
                if (far_arg or file_arg) and n:
                    if not ida_bytes.is_strlit(ida_bytes.get_flags(ea)):
                        ida_bytes.del_items(ea, 0, n + 1)
                        if idc.create_strlit(ea, ea + n + 1):
                            strings += 1
                    idc.op_plain_offset(head, op, DS_BASE)
                    converted += 1
                elif (far_arg or file_arg) and not n and not idc.get_name(ea).startswith(("byte_", "word_", "dword_", "unk_", "off_", "stru_")) and idc.get_name(ea):
                    idc.op_plain_offset(head, op, DS_BASE)
                    converted += 1
print("converted %d operands to offsets, created %d strings" % (converted, strings))
