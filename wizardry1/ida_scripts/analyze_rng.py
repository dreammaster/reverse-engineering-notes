"""One-off: force-disassemble the console-unit / RNG native code in
SYSTEM.INTERP that auto-analysis left as `db` (reached only through a data
jump table at 0x2CEE). Run via the driver, then read wiz1_interp.asm around
0x2CEE-0x2E60.
"""
import ida_bytes
import ida_auto
import idc

# the jump table itself (words) and the handlers it points at
for ea in range(0x2CEE, 0x2E60):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)

for ea in (0x2CF1, 0x2CF4, 0x2D13, 0x2D5D, 0x2D82, 0x2D94, 0x2D9A,
           0x2DA6, 0x2DB1, 0x2DB9, 0x2E37, 0x2E45):
    idc.create_insn(ea)

# table at 0x2CEE: first word is a count(0x0d), then 12 word pointers
idc.create_word(0x2CEE)
for i in range(12):
    idc.create_word(0x2CF0 + 2 * i)
    idc.op_plain_offset(0x2CF0 + 2 * i, 0, 0)

ida_auto.plan_and_wait(0x2CEE, 0x2E60)
print("rng region reanalyzed")
for ea in (0x2D13, 0x2D94, 0x2D9A):
    print(f"{ea:#06x}: {idc.GetDisasm(ea)}")
