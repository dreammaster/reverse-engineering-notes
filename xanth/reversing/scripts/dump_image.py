"""
IDAPython: writes the flat memory image of xanth.idb (linear address = segment * 16 + offset, the layout the IDB uses) to a binary file, and the
function table (name -> linear address, size, near/far) to JSON.  Unmapped bytes are 0.  The image is game data: keep it out of git (xanth/game_out).

    idat.exe -A -S"dump_image.py image.bin funcs.json" copy.idb
"""
import json
import idc
import idautils
import ida_bytes
import ida_segment
import ida_funcs
import ida_kernwin

img = bytearray(0x100000)
top = 0
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    if s.end_ea > 0x100000:
        continue
    b = ida_bytes.get_bytes(s.start_ea, s.end_ea - s.start_ea)
    if b:
        img[s.start_ea:s.end_ea] = b
        top = max(top, s.end_ea)
open(idc.ARGV[1], "wb").write(bytes(img[:top]))
funcs = {}
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea)
    sg = ida_segment.getseg(ea)
    funcs[idc.get_func_name(ea)] = {"ea": ea, "end": f.end_ea, "seg": (sg.start_ea >> 4) if sg else 0}
json.dump({"top": top, "funcs": funcs}, open(idc.ARGV[2], "w"))
print("image bytes", top, "functions", len(funcs))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
