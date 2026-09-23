"""
Read-only, yendor3 port of yendor2/ida_scripts/extract_resource_stubs.py:
for every "resource block setup" stub (push si / mov si, imm / write
FileEntry.buffer+blockOffset+blockOffsetHi via [bx+N] / retf), extracts
its SI pointer (resolved to a real linear address) and the dword value
stored there (the actual WORLD.DAT file offset), plus the block-size
value/pattern used.

    .\run_ida_script.ps1 extract_resource_stubs.py -NoExport
"""
import idautils
import idc
import ida_funcs
import ida_bytes
import ida_segment

def matches_pattern(f):
    cur = f.start_ea
    insns = []
    while cur < f.end_ea:
        insns.append((cur, idc.print_insn_mnem(cur), idc.print_operand(cur, 0), idc.print_operand(cur, 1)))
        cur = idc.next_head(cur, f.end_ea)
    text = ";".join(f"{m}:{a}:{b}" for _, m, a, b in insns)
    ok = "[bx+4]" in text and ("[bx+0Ah]" in text or "[bx+0A]" in text.replace("h", ""))
    return ok, insns

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

results = []
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea)
    if f.end_ea - f.start_ea > 40:
        continue
    ok, insns = matches_pattern(f)
    if not ok:
        continue

    si_ea = None
    size_val = None
    for cur, m, a, b in insns:
        if m == "mov" and a == "si":
            si_ea = idc.get_operand_value(cur, 1) & 0xFFFF
        if m == "mov" and "[bx+6]" in a:
            size_val = b

    real_si = DS_BASE + si_ea if si_ea is not None else None
    dword_val = None
    if real_si is not None:
        try:
            dword_val = ida_bytes.get_wide_dword(real_si)
        except Exception:
            pass

    results.append((ea, f.end_ea - f.start_ea, si_ea, real_si, dword_val, size_val))

results.sort()
print(f"{len(results)} resource stubs found (DS_BASE={DS_BASE:#x})\n")
for ea, sz, si_raw, real_si, dword_val, size_val in results:
    si_raw_s = f"{si_raw:#x}" if si_raw is not None else "?"
    real_si_s = f"{real_si:#x}" if real_si else "?"
    dword_s = f"{dword_val:#x}" if dword_val is not None else "?"
    print(f"{ea:#x} (sz={sz}): si_raw={si_raw_s} real_si={real_si_s} "
          f"offset={dword_s} blocksize_expr={size_val}")
