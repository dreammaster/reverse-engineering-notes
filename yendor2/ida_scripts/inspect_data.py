"""
Read-only: inspects a data symbol -- address, type/size, and a sample of
its xrefs.

    .\run_ida_script.ps1 inspect_data.py -NoExport
"""
import idc
import idautils
import ida_bytes

NAME = "fe"

ea = idc.get_name_ea_simple(NAME)
print(f"{NAME} @ {ea:#x}")
print("flags:", hex(ida_bytes.get_flags(ea)))
print("size:", ida_bytes.get_item_size(ea))

refs = list(idautils.XrefsTo(ea))
print(f"{len(refs)} xrefs, sample:")
for x in refs[:20]:
    fn = idc.get_func_name(x.frm)
    print(f"  {x.frm:#x} ({fn})  {idc.generate_disasm_line(x.frm, 0)}")
