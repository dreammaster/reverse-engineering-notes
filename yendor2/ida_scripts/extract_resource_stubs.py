"""
Read-only: for every "resource block setup" stub matching the pattern
documented in document_resource_stubs.py (push si / mov si, imm / write
FileEntry.buffer+blockOffset+blockOffsetHi via [bx+N] / retf), extracts:
  - its own address
  - the SI pointer (resolved to a real linear address now that DS is
    fixed) and the dword value stored there (the actual file offset)
  - the block-size value/pattern used
so the offsets can be checked against PICTURES.VGA/WORLD.DAT directly.

    .\run_ida_script.ps1 extract_resource_stubs.py -NoExport
"""
import idautils
import idc
import ida_funcs
import ida_bytes

def matches_pattern(f):
    cur = f.start_ea
    insns = []
    while cur < f.end_ea:
        insns.append((cur, idc.print_insn_mnem(cur), idc.print_operand(cur, 0), idc.print_operand(cur, 1)))
        cur = idc.next_head(cur, f.end_ea)
    text = ";".join(f"{m}:{a}:{b}" for _, m, a, b in insns)
    ok = "[bx+4]" in text and ("[bx+0Ah]" in text or "[bx+0A]" in text.replace("h", ""))
    return ok, insns

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

    # DS is fixed to paragraph 0x2D86 (linear 0x2D860) for every segment
    # (see fix_ds_segreg.py) -- si is a plain DS-relative offset into the
    # shared data segment (seg129).
    DS_BASE = 0x2D860
    real_si = DS_BASE + si_ea if si_ea is not None else None
    dword_val = None
    if real_si is not None:
        try:
            dword_val = ida_bytes.get_wide_dword(real_si)
        except Exception:
            pass

    results.append((ea, f.end_ea - f.start_ea, si_ea, real_si, dword_val, size_val))

results.sort()
print(f"{len(results)} resource stubs found\n")
for ea, sz, si_raw, real_si, dword_val, size_val in results:
    si_raw_s = f"{si_raw:#x}" if si_raw is not None else "?"
    real_si_s = f"{real_si:#x}" if real_si else "?"
    dword_s = f"{dword_val:#x}" if dword_val is not None else "?"
    print(f"{ea:#x} (sz={sz}): si_raw={si_raw_s} real_si={real_si_s} "
          f"offset={dword_s} blocksize_expr={size_val}")
