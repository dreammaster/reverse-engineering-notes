"""
Read-only probe for leglib.idb: dump the three int-3Fh resolution tables
the run-time dispatch handler uses, and sanity-check a few entries.

Handler (seg003:7383h, installed via DOS int 21h/AX=253Fh from seg003:~7350h):
  - reads the ordinal byte(s) after `CD 3F` in the caller
  - bare ordinal n   -> routine at  seg003 : word[seg003:73F6h + 2*n]
                        (segment forced to seg003, or seg004 for a small
                         index sub-range that bare ordinals never reach)
  - prefix FF, ord n -> routine at  {seg003|seg004} : word[seg003:75F2h + 2*n]
                        (seg004 when 0x19 <= n < 0x62, else seg003)
  - prefix FE, ord n -> routine at  seg:off from 4-byte entry at
                        seg003:15Ch + 4*n   (full far pointer in-table)
  - then patches the caller's `CALL FAR` operands in place and retf's, so
    each call site self-resolves to a direct far call after first use.

Usage:
    .\run_ida_script.ps1 -Idb leglib -ScriptName probe_rtm_tables.py -NoExport
"""

import idc
import ida_bytes
import ida_segment
import ida_funcs

seg3 = ida_segment.get_segm_by_name("seg003")
seg4 = ida_segment.get_segm_by_name("seg004")
S3 = seg3.start_ea
S4 = seg4.start_ea
print(f"seg003 = {S3:#x}..{seg3.end_ea:#x}")
print(f"seg004 = {S4:#x}..{seg4.end_ea:#x}")


def w(ea):
    return ida_bytes.get_word(ea)


def desc(ea):
    f = idc.get_func(ea) if hasattr(idc, "get_func") else None
    fn = ida_funcs.get_func(ea)
    name = idc.get_name(ea)
    tag = ""
    if fn:
        tag = "FUNC_HEAD" if fn.start_ea == ea else f"mid-func(+{ea - fn.start_ea:#x} of {idc.get_func_name(ea)})"
    else:
        flags = ida_bytes.get_full_flags(ea)
        tag = "code" if ida_bytes.is_code(flags) else ("data" if ida_bytes.is_data(flags) else "UNDEFINED")
    return f"{ea:#07x} {tag} {name}"


BARE = S3 + 0x73F6
FF = S3 + 0x75F2
FE = S3 + 0x15C

print("\n--- bare table @ seg003:73F6, first/again a few known-hot ordinals ---")
for n in [0x00, 0x01, 0x02, 0x30, 0x5E, 0x70, 0x87, 0x98, 0xAF, 0xBC, 0xC2, 0xC3, 0xCD, 0xCE, 0xD1, 0xDE, 0xF0, 0xF4, 0xFD]:
    off = w(BARE + 2 * n)
    tgt = S3 + off
    print(f"  rt_{n:02X}  off={off:#06x}  -> {desc(tgt)}")

print("\n--- FF table @ seg003:75F2 ---")
for n in [0x00, 0x08, 0x0A, 0x19, 0x1F, 0x30, 0x4B, 0x50, 0x61, 0x62, 0x67]:
    off = w(FF + 2 * n)
    base = S4 if 0x19 <= n < 0x62 else S3
    bn = "seg004" if base is S4 else "seg003"
    print(f"  rt_FF{n:02X}  off={off:#06x} base={bn} -> {desc(base + off)}")

print("\n--- FE table @ seg003:15C (4-byte off,seg entries) ---")
for n in [0x00, 0x01, 0x25, 0x26, 0x37, 0x3F, 0x45, 0x53, 0x63, 0x68, 0x6D]:
    off = w(FE + 4 * n)
    seg = w(FE + 4 * n + 2)
    # seg is a load-relative paragraph; IDA base is 0x1000, so linear = (seg+0x1000)<<4 ... but
    # simpler: match against known segment paras. seg003 para 0x12A9 -> stored 0x2A9; seg004 0x1F9C -> 0xF9C
    known = {0x2A9: ("seg003", S3), 0xF9C: ("seg004", S4)}
    if seg in known:
        nm, base = known[seg]
        print(f"  rt_FE{n:02X}  off={off:#06x} seg={seg:#06x}({nm}) -> {desc(base + off)}")
    else:
        print(f"  rt_FE{n:02X}  off={off:#06x} seg={seg:#06x}(?) -> unresolved base")

print("\n--- raw bytes of FE table head (seg003:15C..1A0) ---")
b = ida_bytes.get_bytes(FE, 0x44)
print(" ".join(f"{x:02x}" for x in b))
