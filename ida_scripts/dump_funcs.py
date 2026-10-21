"""
Read-only: dump every function's disassembly (with addresses, callers and
string contents for data operands) to dump_<root>.txt next to this script,
for reading during analysis. Run with -NoExport.

    .\run_ida_script.ps1 -Idb KEH -ScriptName dump_funcs.py -NoExport
"""

import os
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_ua

out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                        "dump_%s.txt" % os.path.splitext(idc.get_root_filename())[0])


def cstr(ea, maxlen=80):
    s = ida_bytes.get_strlit_contents(ea, maxlen, 0)
    if not s:
        return None
    try:
        s = s.decode("latin-1")
    except Exception:
        return None
    if len(s) >= 3 and all(32 <= ord(c) < 127 or c in "\r\n\t\x00" for c in s):
        return s
    return None


with open(out_path, "w", encoding="utf-8") as fh:
    for fea in idautils.Functions():
        end = idc.get_func_attr(fea, idc.FUNCATTR_END)
        callers = sorted({idc.get_func_name(x) or ("data_%X" % x)
                          for x in idautils.CodeRefsTo(fea, 0)})
        fh.write("\n;==== %s @ %X size=%d callers=%s\n" % (
            idc.get_func_name(fea), fea, end - fea, ",".join(callers)))
        ea = fea
        while ea < end:
            line = idc.generate_disasm_line(ea, 0)
            extra = ""
            for opn in range(2):
                if idc.get_operand_type(ea, opn) in (idc.o_mem, idc.o_displ, idc.o_imm):
                    v = idc.get_operand_value(ea, opn)
                    if idc.get_operand_type(ea, opn) == idc.o_imm and v < 0x2000:
                        continue
                    seg = idc.get_segm_name(idc.get_name_ea_simple("dseg") if False else ea)
                    s = None
                    # strings live in dseg; offsets are dseg-relative
                    dseg_base = idc.get_segm_start(idc.get_name_ea_simple("dseg"))
                    if dseg_base != idc.BADADDR and 0 <= v < 0x10000:
                        s = cstr(dseg_base + v)
                    if s:
                        extra += '  ; "%s"' % s.replace("\n", "\\n").replace("\r", "\\r")
            fh.write("%X  %s%s\n" % (ea, line, extra))
            ea = idc.next_head(ea, end)
print("wrote", out_path)
