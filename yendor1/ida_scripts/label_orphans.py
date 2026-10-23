"""Give a name to every instruction that is not reached by falling through from the previous one and has no name of its own.

Such code is reached only through jump tables (`jmp ax` / `call [bx]` over a table in the data segment), and without a label
the exported .asm gives src1/tools/asm2cpp.py no address for it.

    .\run_ida_script.ps1 prog4 label_orphans.py
"""
import idaapi
import idautils
import idc
import ida_bytes
import ida_name

count = 0
for seg_ea in idautils.Segments():
    if idc.get_segm_attr(seg_ea, idc.SEGATTR_TYPE) != idc.SEG_CODE:
        continue
    for ea in idautils.Heads(idc.get_segm_start(seg_ea), idc.get_segm_end(seg_ea)):
        flags = ida_bytes.get_full_flags(ea)
        if not ida_bytes.is_code(flags):
            continue
        if ida_bytes.has_user_name(flags) or ida_name.get_name(ea):
            continue
        if ida_bytes.is_flow(flags):
            continue
        if ida_name.set_name(ea, "loc_%X" % ea, ida_name.SN_NOWARN | ida_name.SN_NOCHECK):
            count += 1

print("label_orphans: named %d instructions" % count)
