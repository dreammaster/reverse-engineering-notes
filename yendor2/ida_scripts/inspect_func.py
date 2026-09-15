"""
Read-only: prints a function's size, refs, and callers/callees for quick
orientation.

    .\run_ida_script.ps1 inspect_func.py -NoExport
"""
import idautils
import idc
import ida_funcs

TARGET = 0x132F2

f = ida_funcs.get_func(TARGET)
print(f"{idc.get_func_name(TARGET)} @ {TARGET:#x}, size={f.end_ea-f.start_ea}")

callers = set()
for x in idautils.CodeRefsTo(TARGET, 0):
    cf = ida_funcs.get_func(x)
    if cf:
        callers.add(idc.get_func_name(cf.start_ea))
print("callers:", callers)

refs = set()
callees = set()
cur = f.start_ea
while cur < f.end_ea:
    for x in idautils.DataRefsFrom(cur):
        nm = idc.get_name(x)
        if nm:
            refs.add(nm)
    if idc.print_insn_mnem(cur) == "call":
        for x in idautils.CodeRefsFrom(cur, 0):
            cfn = ida_funcs.get_func(x)
            if cfn and cfn.start_ea == x:
                callees.add(idc.get_func_name(x))
    cur = idc.next_head(cur, f.end_ea)
print("data refs:", sorted(refs))
print("callees:", sorted(callees))
