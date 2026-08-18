"""
IDA Pro script: investigates (read-only) the extent of seg002's
mis-disassembled garbage between the real `start` bootstrap proc and
the confirmed-uninitialized `byte_18FE0` tail. Prints boundaries, a
byte-level summary, and any printable-ASCII runs found. Does not
modify the IDB.
"""

import ida_bytes
import ida_funcs
import idc

start_ea = idc.get_name_ea_simple("start")
uninit_ea = idc.get_name_ea_simple("byte_18FE0")

pfn = ida_funcs.get_func(start_ea)
gap_start = pfn.end_ea
gap_len = uninit_ea - gap_start
raw = ida_bytes.get_bytes(gap_start, gap_len)

print(f"Gap: {gap_start:X} .. {uninit_ea:X} ({gap_len:#x} = {gap_len} bytes)")

# Find runs of >=5 printable ASCII bytes.
def is_printable(b):
    return 0x20 <= b < 0x7F

runs = []
run_start = None
for i, b in enumerate(raw):
    if is_printable(b):
        if run_start is None:
            run_start = i
    else:
        if run_start is not None and i - run_start >= 5:
            runs.append((run_start, i))
        run_start = None
if run_start is not None and len(raw) - run_start >= 5:
    runs.append((run_start, len(raw)))

if runs:
    print(f"\n{len(runs)} printable-ASCII run(s) of length >=5:")
    for s, e in runs:
        text = raw[s:e].decode("latin-1", "replace")
        print(f"  {gap_start+s:X}: {text!r} ({e-s} bytes)")
else:
    print("\nNo printable-ASCII runs of length >=5 found.")

# Dump the first 96 bytes as hex for manual inspection regardless.
print("\nFirst 96 bytes (hex):")
for row in range(0, min(96, gap_len), 16):
    chunk = raw[row:row+16]
    hexpart = " ".join(f"{b:02X}" for b in chunk)
    asciipart = "".join(chr(b) if is_printable(b) else "." for b in chunk)
    print(f"  {gap_start+row:X}: {hexpart:<48} {asciipart}")
