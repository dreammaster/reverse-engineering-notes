"""
VIDP16.DRV is a flat 16-bit code blob (no MZ header, no entry point of its own):
its first instruction is `jmp 02C8h`. Create_idb.ps1 loads it with the Binary
loader, so nothing is marked as code yet. This seeds the analysis at offset 0
and names the entry; run via run_ida_script.ps1 so the result is exported.

    .\run_ida_script.ps1 vidp16 seed_vidp16.py
"""

import idc
import ida_auto
import ida_entry
import ida_funcs

ida_entry.add_entry(0, 0, "start", True)
idc.create_insn(0)
ida_funcs.add_func(0)
ida_auto.auto_wait()
print(f"functions: {sum(1 for _ in __import__('idautils').Functions())}")
