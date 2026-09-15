"""
Dump the message at DS:0x7FF7, trying both NUL and '$' terminators
(the engine seems to use both styles in different places), plus wider
context bytes.

Run via:
    .\\run_ida_script.ps1 dump_msg_7ff7.py -NoExport
"""
import ida_bytes

ea = 0x7FF7 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(400))
print(f"0x7ff7 -> {ea:#x} raw 400 bytes: {ctx!r}")
