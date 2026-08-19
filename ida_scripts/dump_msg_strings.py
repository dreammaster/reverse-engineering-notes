"""
Read-only helper: resolves small numeric "message id" immediates -- as
seen pushed directly to writeString/writeStringNewline in MONDAIN.EXE,
e.g. `mov ax, 24Ch / push ax / call writeString` -- into actual ASCII
text.

DS is fixed at the data segment (dseg) for the whole program, so a bare
immediate offset X used as a near pointer means the string lives at
(dseg base + X). Confirmed against known strings ("Board?", "Get (Gem)",
"Hit Mondain! ", "THE UNIVERSE IS DOOMED!", etc.) during the MONDAIN.EXE
renaming pass -- see docs/overview.md and apply_renames_mondain.py.

Edit OFFSETS below and re-run whenever a new batch of message ids turns
up during future analysis.

    .\\run_ida_script.ps1 -Idb ultima1_mondain -ScriptName dump_msg_strings.py -NoExport
"""

import ida_segment
import ida_bytes

seg = ida_segment.get_segm_by_name("dseg")
base = seg.start_ea
print(f"dseg base = {base:#x}")

OFFSETS = [
    0x24C, 0x251, 0x256, 0x25D, 0x260, 0x264, 0x276, 0x26D, 0x27F, 0x287,
]

for off in OFFSETS:
    ea = base + off
    s = ida_bytes.get_strlit_contents(ea, -1, 0)
    if s is None:
        raw = bytearray()
        p = ea
        for _ in range(200):
            b = ida_bytes.get_byte(p)
            if b == 0:
                break
            raw.append(b)
            p += 1
        s = bytes(raw)
    print(f"{off:#05x} (ea={ea:#x}): {s!r}")
