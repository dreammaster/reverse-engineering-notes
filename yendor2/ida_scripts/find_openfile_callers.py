"""
Read-only: lists all callers of FileEntry_OpenFile with a few
instructions of context before the call (to see what filename pointer
they set up), looking for the one that opens PICTURES.VGA (string at
0x3687F, which has no direct xref -- likely referenced indirectly).

    .\run_ida_script.ps1 find_openfile_callers.py -NoExport
"""
import idautils
import idc
import ida_funcs

target = idc.get_name_ea_simple("FileEntry_OpenFile")

for x in idautils.CodeRefsTo(target, 0):
    f = ida_funcs.get_func(x)
    fn = idc.get_func_name(f.start_ea) if f else "?"
    print(f"\n{x:#x} in {fn}:")
    cur = x
    # walk back a few instructions
    lines = []
    p = x
    for _ in range(6):
        p = idc.prev_head(p, f.start_ea if f else 0)
        if p == idc.BADADDR or (f and p < f.start_ea):
            break
        lines.append(p)
    for p in reversed(lines):
        print(f"    {p:#x}  {idc.generate_disasm_line(p, 0)}")
    print(f"    {x:#x}  {idc.generate_disasm_line(x, 0)}")
