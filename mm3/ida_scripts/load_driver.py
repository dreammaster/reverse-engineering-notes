"""Add the video/draw module (member 8F99h of MM3.CC) to the database as segment `vdrv` at 60000h.

The game loads it at run time and reaches it through 3-byte `jmp near` entries at offsets 00h..2Dh via the wrappers in seg007
(`push cs:word_24F63; push <offset>; retf`).  Needs the environment variable MM3_CC (path to MM3.CC); skipped if unset.
"""
import os, sys
import ida_auto, ida_bytes, ida_funcs, ida_segment, idc
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools"))
import mm3_cc

path = os.environ.get("MM3_CC")
if not path or not os.path.exists(path):
    print("MM3_CC not set; skipping driver load")
else:
    cc = open(path, "rb").read()
    mod = None
    for ident, off, size in mm3_cc.read_toc(cc):
        if ident == 0x8F99:
            mod, _ = mm3_cc.member(cc, off, size)
    base = 0x60000
    if ida_segment.getseg(base) is None:
        ida_segment.add_segm(0x6000, base, base + len(mod), "vdrv", "CODE")
        seg = ida_segment.getseg(base)
        seg.bitness = 0
        ida_segment.update_segm(seg)
        ida_bytes.put_bytes(base, mod)
    idc.set_default_sreg_value(base, "ds", 0x286F)
    for i in range(17):
        a = base + 3 * i
        idc.create_insn(a)
        ida_funcs.add_func(a, a + 3)
        t = idc.get_operand_value(a, 0)
        idc.create_insn(base + t)
        ida_funcs.add_func(base + t)
        idc.set_name(a, "vdrv_api_%02X" % (3 * i), idc.SN_NOCHECK | idc.SN_NOWARN)
    ida_auto.auto_wait()
    print("vdrv loaded:", len(mod), "bytes")
