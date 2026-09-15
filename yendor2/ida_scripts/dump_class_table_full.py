"""
Full dump of the class-name table used by sub_19768, to get a clean
final list (excluding the trailing garbage strings past the real
class count) and to confirm ShowWorldMap's address for a comment fix.

Run via:
    .\\run_ida_script.ps1 dump_class_table_full.py -NoExport
"""
import ida_bytes
import idc

def read_entry(ea, stride=0xB):
    chars = []
    for j in range(stride):
        b = ida_bytes.get_byte(ea + j)
        if b == 0:
            break
        chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    return "".join(chars)

print("table1 (ids 1-9):")
for i in range(9):
    ea = 0x351E2 + i * 0xB
    print(f"  id={i+1}: {read_entry(ea)!r}")

print("table2/3 contiguous (ids 10-28):")
for i in range(19):
    ea = 0x35C94 + i * 0xB
    print(f"  id={i+10}: {read_entry(ea)!r}")

print("ShowWorldMap ea:", hex(idc.get_name_ea_simple("ShowWorldMap")))
print("sub_2BFBC ea:", hex(idc.get_name_ea_simple("sub_2BFBC")))
print("sub_19768 ea:", hex(idc.get_name_ea_simple("sub_19768")))
