"""
Names the small DOS file-layer wrappers that every Ultima 6 / Martian Dreams /
Savage Empire executable carries (the Origin run-time layer around INT 21h:
open/close/create/delete/rename with a retry hook; read and write also seek, so
they are named from signature matches instead), by the DOS
service each one uses.  Only functions that are still `sub_XXXXX` and use exactly
one of the file services are renamed; the first of each kind gets the plain name,
further ones a numeric suffix (the ea order is stable, so re-running is idempotent).

    .\\run_ida_script.ps1 -Idb u6_u -ScriptName name_dos_wrappers.py
"""

import idautils
import idc
import ida_funcs
import ida_name

SERVICES = {
    0x3D: "file_open",
    0x3E: "file_close",
    0x3C: "file_create",
    0x41: "file_delete",
    0x56: "file_rename",
}
MAX_SIZE = 600

found = {}
for start in idautils.Functions():
    f = ida_funcs.get_func(start)
    if not idc.get_func_name(start).startswith("sub_") or f.end_ea - f.start_ea > MAX_SIZE:
        continue
    used = set()
    ah = None
    for ea in idautils.FuncItems(start):
        m = idc.print_insn_mnem(ea)
        if m == "mov" and idc.get_operand_type(ea, 0) == idc.o_reg and idc.get_operand_type(ea, 1) == idc.o_imm:
            reg = idc.print_operand(ea, 0)
            v = idc.get_operand_value(ea, 1) & 0xFFFF
            if reg == "ah":
                ah = v
            elif reg == "ax":
                ah = v >> 8
        elif m == "int" and idc.get_operand_value(ea, 0) == 0x21 and ah is not None:
            used.add(ah)
    kinds = used & set(SERVICES)
    if len(kinds) == 1 and len(used) == 1:
        found.setdefault(SERVICES[next(iter(kinds))], []).append(start)

for base, eas in sorted(found.items()):
    for i, ea in enumerate(sorted(eas)):
        name = base if i == 0 else "%s_%d" % (base, i + 1)
        ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
        print("%05X %s" % (ea, name))
