"""Create functions at every resident thunk target (they are only reached from overlays,
so IDA's auto-analysis of the main database never finds them), and name each thunk after
its target once the target has a real name (thk_<name>)."""
import os, sys
import ida_funcs, idc, idautils
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

lay = layout()
made = 0
targets = sorted({BASE + toff for off, idx, toff in lay.thunks() if lay.thunk_owner(idx) is None})
for i, ea in enumerate(targets):
    idc.create_insn(ea)
    if ida_funcs.get_func(ea) and ida_funcs.get_func(ea).start_ea == ea:
        continue
    if ida_funcs.add_func(ea):
        made += 1
print("resident thunk targets:", len(targets), "new functions:", made)

# rename thunks to follow their target's name
renamed = 0
for off, idx, toff in lay.thunks():
    if lay.thunk_owner(idx) is None:
        n = idc.get_name(BASE + toff)
        if n and not n.startswith(("loc_", "sub_")):
            idc.set_name(BASE + off, "thk_" + n, idc.SN_NOCHECK | idc.SN_NOWARN)
            renamed += 1
print("thunks renamed after target:", renamed)
