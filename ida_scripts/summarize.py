"""
Read-only: one line per function: size, callers, callees, referenced strings.
Writes summary_<root>.txt next to this script. Run with -NoExport.
"""
import os
import idc
import idautils
import ida_bytes
import ida_segment

root = os.path.splitext(idc.get_root_filename())[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "summary_%s.txt" % root)
dseg = ida_segment.get_segm_by_name("dseg").start_ea
NUL = bytes([0])


def cstr(off):
    s = ida_bytes.get_bytes(dseg + off, 70)
    if not s:
        return None
    s = s.split(NUL)[0]
    try:
        s = s.decode("latin-1")
    except Exception:
        return None
    if len(s) >= 4 and all(32 <= ord(c) < 127 or c in "\r\n" for c in s):
        return s.replace("\r", "\r").replace("\n", "\n")
    return None


with open(out, "w", encoding="utf-8") as fh:
    for fea in idautils.Functions():
        end = idc.get_func_attr(fea, idc.FUNCATTR_END)
        callees, strs = [], []
        ea = fea
        while ea < end:
            if idc.print_insn_mnem(ea) == "call":
                for t in idautils.CodeRefsFrom(ea, 0):
                    n = idc.get_func_name(t)
                    if n and n not in callees:
                        callees.append(n)
            for opn in range(2):
                if idc.get_operand_type(ea, opn) == idc.o_imm:
                    v = idc.get_operand_value(ea, opn)
                    if 0x100 <= v < 0xD000:
                        s = cstr(v)
                        if s and s not in strs:
                            strs.append(s)
            ea = idc.next_head(ea, end)
        callers = {idc.get_func_name(x) or "data" for x in idautils.CodeRefsTo(fea, 0)}
        fh.write("%X %s sz=%d callers=%d | calls: %s | str: %s\n" % (
            fea, idc.get_func_name(fea), end - fea, len(callers),
            ",".join(callees), " || ".join(strs)))
