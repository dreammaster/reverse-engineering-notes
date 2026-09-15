"""
Read-only: scopes how many functions match the "resource block setup"
pattern seen around 0x27CB0-0x27FFF (push si / mov si, imm / write
FileEntry.buffer+blockOffset+blockOffsetHi via [bx+N] / retf), across the
whole binary, to decide whether a consistent group name is worthwhile.

    .\run_ida_script.ps1 scope_resource_stubs.py -NoExport
"""
import idautils
import idc
import ida_funcs

def matches_pattern(f):
    cur = f.start_ea
    insns = []
    while cur < f.end_ea:
        insns.append((idc.print_insn_mnem(cur), idc.print_operand(cur, 0), idc.print_operand(cur, 1)))
        cur = idc.next_head(cur, f.end_ea)
    text = ";".join(f"{m}:{a}:{b}" for m, a, b in insns)
    return "[bx+4]" in text and ("[bx+0Ah]" in text or "[bx+0A]" in text.replace("h", ""))

matches = []
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea)
    if f.end_ea - f.start_ea > 40:
        continue
    if matches_pattern(f):
        matches.append(ea)

print(f"functions matching the [bx+4]/[bx+0Ah] FileEntry-block-setup pattern: {len(matches)}")
print("range:", hex(min(matches)) if matches else None, "-", hex(max(matches)) if matches else None)
unnamed = [e for e in matches if idc.get_func_name(e).startswith("sub_")]
print(f"  of which unnamed: {len(unnamed)}")

# Are they contiguous / how are they distributed across segments?
import ida_segment
segs = {}
for e in matches:
    s = ida_segment.getseg(e)
    name = ida_segment.get_segm_name(s) if s else "?"
    segs[name] = segs.get(name, 0) + 1
print("by segment:", segs)
