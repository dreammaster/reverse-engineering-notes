"""
Read-only: resolves the real addresses of the seven door-key strings
(aBrassKey..aGoldKey) and prints them alongside the raw hex message
offsets ShowLockStatus uses for each g_lockStatusFlags tier bit, so the
bit<->tier mapping can be confirmed unambiguously instead of guessed
from declaration order.

    .\run_ida_script.ps1 check_lock_key_strings.py -NoExport
"""
import idc
import ida_name

names = ["aBrassKey", "aBronzeKey", "aCopperKey", "aIronKey", "aSteelKey", "aSilverKey", "aGoldKey"]
for n in names:
    ea = idc.get_name_ea_simple(n)
    print(f"{n}: ea={ea:#x}")

# The exact bx immediates ShowLockStatus loads per flag bit, in the order tested (0x8000 down to 0x200).
bits_and_offsets = [
    (0x8000, 0x7C0E),
    (0x4000, 0x7C18),
    (0x2000, 0x7C23),
    (0x1000, 0x7C2E),
    (0x0800, 0x7C37),
    (0x0400, 0x7C41),
    (0x0200, 0x7C4C),
]
print()
print("DS_BASE-relative message offsets used by ShowLockStatus's dispatch:")
for bit, off in bits_and_offsets:
    print(f"  bit {bit:#06x} -> raw offset {off:#x}")
