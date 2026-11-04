"""
First-pass setup for the raw sound-driver .BIN files (create_idb.ps1 -Binary).

IDA's "Binary file" loader makes the one segment 32-bit when run
headlessly, but these drivers are 16-bit real-mode code.  This flips the
segment to 16-bit, throws away the 32-bit auto-analysis, and re-seeds it
from the two near-call entry stubs every driver starts with
(offset 0: "call near ptr ...; retf", offset 4: the same), which are the
driver's two entry points as called by the game.
"""

import idc
import ida_auto
import ida_bytes
import ida_segment

seg = ida_segment.get_first_seg()
start, end = seg.start_ea, seg.end_ea

ida_segment.set_segm_addressing(seg, 0)          # 0 = 16-bit
ida_bytes.del_items(start, ida_bytes.DELIT_SIMPLE, end - start)

for ofs, name in ((0, "entry0"), (4, "entry1")):
    ea = start + ofs
    idc.add_entry(ea, ea, name, 1)
    idc.create_insn(ea)
    idc.add_func(ea)

ida_auto.auto_mark_range(start, end, ida_auto.AU_CODE)
ida_auto.plan_and_wait(start, end)
ida_auto.auto_wait()

print("segment %#x-%#x bitness=%d" % (start, end, ida_segment.getseg(start).bitness))
