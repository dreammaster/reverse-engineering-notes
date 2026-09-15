"""
Read-only: dumps disassembly (with comments) for a hardcoded address
range, for manual reading. Edit START/END below and rerun.

    .\run_ida_script.ps1 dump_range.py -NoExport
"""
import idc
import ida_funcs

START = 0x2A11B
END = 0x2A11B + 120

ea = START
while ea < END:
    fn = ida_funcs.get_func(ea)
    if fn and fn.start_ea == ea:
        print(f"; ---- {idc.get_func_name(ea)} @ {ea:#x} ----")
    line = idc.generate_disasm_line(ea, 0)
    cmt = idc.get_cmt(ea, 0) or idc.get_cmt(ea, 1) or ""
    print(f"  {ea:#x}  {line}" + (f"  ; {cmt}" if cmt else ""))
    nxt = idc.next_head(ea, END)
    if nxt <= ea:
        break
    ea = nxt
