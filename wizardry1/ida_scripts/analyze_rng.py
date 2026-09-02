"""Force-disassemble the console-unit / RANDOM native code in SYSTEM.INTERP.
The RANDOM p-code (WIZARDRY proc 34) is UNITREAD(CONUNIT, buf, 0, 10, 0, 0);
CSP 5's handler (loc_2CCF, ~0x2CCE) branches through a table at 0x2CF7 to
native code around 0x2D00-0x2DB0 that auto-analysis left as data.

Run via the driver, then read wiz1_interp.asm around 0x2CCE-0x2DC0.
"""
import ida_bytes
import ida_auto
import idc

# wipe the whole muddled region
for ea in range(0x2CCE, 0x2E00):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)

# 0x2CF7: a 13-entry word table (limit word + 12 handler offsets)
for i in range(13):
    idc.create_word(0x2CF7 + 2 * i)

# the three little trampolines just before the table
for ea in (0x2CEE, 0x2CF1, 0x2CF4):
    idc.create_insn(ea)

# native handlers the tables point at
for ea in (0x2CCE, 0x2D11, 0x2D13, 0x2D19, 0x2D1C, 0x2D1F, 0x2D28,
           0x2D5D, 0x2D8B, 0x2D94, 0x2D9F, 0x2DA8, 0x2DB1, 0x2DB3):
    idc.create_insn(ea)

ida_auto.plan_and_wait(0x2CCE, 0x2DC0)
print("done; dump:")
ea = 0x2CCE
while ea < 0x2DC0:
    print(f"{ea:#06x}  {idc.GetDisasm(ea)}")
    ea = idc.next_head(ea, 0x2DC0)

# --- the real RNG at 0x221E (unit 13, subfn 10) ---
for ea in range(0x221E, 0x22A0):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)
idc.create_insn(0x221E)
ida_auto.plan_and_wait(0x221E, 0x22A0)
print("\n=== RNG 0x221E ===")
ea = 0x221E
while ea < 0x229D:
    print(f"{ea:#06x}  {idc.GetDisasm(ea)}")
    ea = idc.next_head(ea, 0x229D)
