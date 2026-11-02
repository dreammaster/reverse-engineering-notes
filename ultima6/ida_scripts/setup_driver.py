"""
First-run set-up for the *.DRV driver databases (raw code blobs): IDA's binary loader makes a
32-bit segment, but the drivers are 16-bit real-mode code that starts with a table of 3-byte
`jmp near` entries (entry n at offset 3 * n, which is how GAME.EXE calls them: it stores 3 * n in
the offset of video_driver_call).  Switch the segment to 16 bits, re-analyse from the entries and
name them entry_NN.

Used by create_drivers.ps1 instead of identify.py.
"""

import idc
import idautils
import ida_auto
import ida_bytes
import ida_funcs
import ida_name
import ida_segment

seg = ida_segment.get_first_seg()
start, end = seg.start_ea, seg.end_ea
ida_bytes.del_items(start, 0, end - start)
seg.bitness = 0                       # 16-bit
ida_segment.update_segm(seg)
idc.set_segm_attr(start, idc.SEGATTR_BITNESS, 0)
idc.set_inf_attr(idc.INF_LFLAGS, idc.get_inf_attr(idc.INF_LFLAGS))

n = 0
while ida_bytes.get_byte(start + 3 * n) == 0xE9:
    ea = start + 3 * n
    ida_bytes.del_items(ea, 0, 3)
    idc.create_insn(ea)
    ida_name.set_name(ea, "entry_%02d" % n, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
    target = (ea + 3 + ida_bytes.get_word(ea + 1)) & 0xFFFF
    ida_funcs.add_func(target)
    n += 1
ida_auto.auto_wait()
print("%d entries; %d functions" % (n, len(list(idautils.Functions()))))
