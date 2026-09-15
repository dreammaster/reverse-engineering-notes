"""
Sets the default DS segment-register value for every code segment to
paragraph 0x2D86 (the single shared data segment), matching what every
one of the old 8.3-era database's ~130 SegDefReg(ea,"ds",0x2D86) calls
recorded -- confirmed uniform across every segment in the backed-up IDC
(ida_scripts/backups/yendor2.idc.pre-8.2-rebuild.bak). This piece of the
old analysis was deliberately NOT replayed along with the names/comments
(replaying Bytes()/SegRegs() wholesale was the fragile whole-IDC-replay
approach that lost almost everything -- see apply_prior_annotations.py's
docstring) -- but it's small, mechanical, and uniform enough to redo
directly and safely.

Without it, IDA can't resolve "ds:XXXXh"-style data operands (the DS
register's value is untracked/undefined at those addresses) to their
real linear address, so no data xref forms and the operand displays as
a bare, unnamed hex offset instead of a symbol -- this is a big part of
why the string-cross-reference approach came up empty earlier this
session, and why globals like the error-exit path's "ds:50D0h" pending
error code couldn't be named/verified directly.

After setting it, forces a full reanalysis so already-disassembled
operands actually get re-resolved with the new segment register info
(not just future analysis).

Run via:
    .\run_ida_script.ps1 fix_ds_segreg.py
"""
import ida_segment
import ida_auto
import idc

DS_PARA = 0x2D86

n = ida_segment.get_segm_qty()
ok, fail = 0, 0
for i in range(n):
    seg = ida_segment.getnseg(i)
    res = idc.set_default_sreg_value(seg.start_ea, "ds", DS_PARA)
    if res:
        ok += 1
    else:
        fail += 1
        print(f"  FAILED seg {i} at {seg.start_ea:#x}")

print(f"set_default_sreg_value(ds={DS_PARA:#x}): {ok} ok, {fail} failed, {n} segments total")

import idc as _idc
print("spot check before reanalysis:", _idc.generate_disasm_line(0x28988, 0))

print("reanalyzing whole database (this will take a while)...")
ida_auto.plan_and_wait(0, idc.BADADDR)
ida_auto.auto_wait()
print("reanalysis done")
print("spot check after reanalysis:", _idc.generate_disasm_line(0x28988, 0))
