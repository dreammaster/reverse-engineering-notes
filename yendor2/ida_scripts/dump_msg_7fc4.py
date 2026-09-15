"""
Dump the message at DS:0x7FC4, the label ShowMaterialCounterHud draws
next to material counter 0x94B3's value -- checking whether it says
"GOLD" to confirm the new "not enough gold" evidence from sub_18FDA.

Run via:
    .\\run_ida_script.ps1 dump_msg_7fc4.py -NoExport
"""
import ida_bytes

ea = 0x7FC4 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(80))
print(f"0x7fc4 -> {ea:#x}: {ctx!r}")
