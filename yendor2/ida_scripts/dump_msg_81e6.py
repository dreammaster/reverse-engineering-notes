"""
Dump the message at DS:0x81E6 (sub_19140's ineligibility rejection,
cx=2 lines) -- checking whether it's "I CAN NOT REPAIR THAT" as
suggested by the nearby message-bank dump from last round.

Run via:
    .\\run_ida_script.ps1 dump_msg_81e6.py -NoExport
"""
import ida_bytes

ea = 0x81E6 + 0x2D860
ctx = bytes(ida_bytes.get_byte(ea + i) for i in range(100))
print(f"0x81e6 -> {ea:#x}: {ctx!r}")
