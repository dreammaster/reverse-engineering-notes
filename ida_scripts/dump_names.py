"""
Read-only: writes every named address ("<linear hex> <name>") to wl_names.txt next to this script. Run with -NoExport.
"""
import os
import idautils
import idc
here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, "wl_names.txt"), "w") as f:
    for ea, name in idautils.Names():
        f.write("%X %s\n" % (ea, name))
