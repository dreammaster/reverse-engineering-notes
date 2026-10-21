"""Read-only: dump every `_window*` Window struct (28 bytes) from dseg as raw words. Run with -NoExport."""
import os
import idc, idautils, ida_bytes, ida_name
root = os.path.splitext(idc.get_root_filename())[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "windows_%s.txt" % root)
with open(out, "w") as fh:
    for ea, name in idautils.Names():
        if name.startswith("_window") or name in ("_windowInfo",):
            words = [ida_bytes.get_word(ea + i * 2) for i in range(14)]
            fh.write("%s %X " % (name, ea) + " ".join("%04X" % w for w in words) + "\n")
