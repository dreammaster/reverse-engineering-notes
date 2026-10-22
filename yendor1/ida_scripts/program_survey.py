"""
Per-program survey, written to survey_<db>.txt: segment/function counts, the
DOS (INT 21h) and driver (INT 62h) function histograms, other interrupts used,
and every printable string of 4+ characters with its address. Used to classify
what each executable does. Read-only:

    .\run_ida_script.ps1 <name> program_survey.py -NoExport
"""
import collections
import os

import idc
import idautils
import ida_funcs
import ida_bytes

from ida_util import find_ah
from vid_api import VID_FUNCS

DOS = {0x09: "print$", 0x0E: "setdrive", 0x19: "getdrive", 0x1A: "setDTA", 0x25: "setvect", 0x2A: "getdate",
       0x2C: "gettime", 0x30: "version", 0x35: "getvect", 0x3B: "chdir", 0x3C: "creat", 0x3D: "open",
       0x3E: "close", 0x3F: "read", 0x40: "write", 0x41: "unlink", 0x42: "lseek", 0x43: "chmod", 0x47: "getcwd",
       0x48: "alloc", 0x49: "free", 0x4A: "setblock", 0x4B: "exec", 0x4C: "exit", 0x4D: "exitcode",
       0x4E: "findfirst", 0x4F: "findnext", 0x56: "rename", 0x67: "sethandles", 0x31: "tsr", 0x39: "mkdir",
       0x0A: "bufinput", 0x01: "getchar", 0x07: "getch", 0x08: "getchnoecho", 0x0B: "kbhit", 0x0C: "flush+input",
       0x02: "putchar", 0x06: "dirio", 0x2F: "getDTA", 0x62: "getPSP", 0x44: "ioctl", 0x45: "dup", 0x46: "dup2",
       0x36: "diskfree", 0x38: "country", 0x4D: "exitcode"}

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"survey_{stem}.txt")

funcs = list(idautils.Functions())
size = sum(ida_funcs.get_func(f).end_ea - ida_funcs.get_func(f).start_ea for f in funcs)
dos = collections.Counter()
vid = collections.Counter()
other = collections.Counter()
for seg in idautils.Segments():
    for h in idautils.Heads(seg, idc.get_segm_end(seg)):
        if idc.print_insn_mnem(h).lower() != "int":
            continue
        n = idc.get_operand_value(h, 0)
        if n == 0x21:
            ah, _, _ = find_ah(h)
            dos[DOS.get(ah, "ah=%s" % ("?" if ah is None else "%02X" % ah))] += 1
        elif n == 0x62:
            ah, _, _ = find_ah(h)
            vid[VID_FUNCS.get(ah, ("?",))[0] if ah is not None else "?"] += 1
        else:
            other["int %02Xh" % n] += 1

with open(OUT, "w") as fh:
    fh.write(f"{stem}: {len(funcs)} functions, {size} bytes of function code\n")
    fh.write("DOS calls : " + ", ".join(f"{k}x{v}" for k, v in dos.most_common()) + "\n")
    fh.write("other ints: " + ", ".join(f"{k}x{v}" for k, v in other.most_common()) + "\n")
    fh.write("INT 62h   : " + ", ".join(f"{k}x{v}" for k, v in vid.most_common()) + "\n")
    fh.write("strings:\n")
    sc = idautils.Strings()
    sc.setup(strtypes=[idc.STRTYPE_C], minlen=4)
    sc.refresh()
    for s in sc:
        fh.write(f"  {s.ea:#08x} {str(s)!r}\n")
print(f"{stem}: survey written")
