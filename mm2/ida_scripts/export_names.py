"""Dump user-assigned names and comments of the *shared* part of an MM2 database
(everything outside the overlay windows) to mm2/build/shared_names.json, so the
per-overlay databases can import them (load_overlay.py does that on build).

Run read-only:  run_ida_script.ps1 -Idb mm2\mm2.idb -ScriptName mm2\ida_scripts\export_names.py -NoExport
"""
import json, os, sys
import ida_bytes, ida_funcs, ida_name, idautils, idc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

names, cmts, funcs = {}, {}, []
for ea, name in idautils.Names():
    if OVL_A[0] <= ea < OVL_B[1]:
        continue
    if ida_bytes.has_user_name(ida_bytes.get_flags(ea)):
        names[ea] = name
for ea in idautils.Heads(0, idc.BADADDR):
    if OVL_A[0] <= ea < OVL_B[1]:
        continue
    c, r = idc.get_cmt(ea, 0), idc.get_cmt(ea, 1)
    if c or r:
        cmts[ea] = [c, r]
for ea in idautils.Functions():
    if not (OVL_A[0] <= ea < OVL_B[1]):
        f = ida_funcs.get_func(ea)
        funcs.append([ea, f.end_ea, idc.get_func_cmt(ea, 0), idc.get_func_cmt(ea, 1)])

out = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "shared_names.json")
os.makedirs(os.path.dirname(out), exist_ok=True)
with open(out, "w") as f:
    json.dump({"names": names, "cmts": cmts, "funcs": funcs}, f)
print(f"exported {len(names)} names, {len(cmts)} comments, {len(funcs)} functions -> {out}")
