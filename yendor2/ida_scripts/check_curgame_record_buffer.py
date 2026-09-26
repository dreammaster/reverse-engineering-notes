"""
Read-only: resolves what DS:0x556E/0x5570 -- the 4-byte buffer
LoadCurgameRecord/LoadLockState copy their per-record data into -- really
are. Confirms both addresses already have named symbols
(g_lockStatusFlags / word_32DD0) and dumps every cross-reference to
them, closing the "still-undecoded EMS record format" question left
open in src23/lockcatalog.h's "LoadCurgameRecord" note: it's the same
two globals LoadLockState populates for an ordinary lock, just read
through a different loader.

    .\run_ida_script.ps1 check_curgame_record_buffer.py -NoExport
"""
import idautils
import idc
import ida_funcs
import ida_name
import ida_bytes

DS_BASE = 0x2D860

ADDRS = [0x556C, 0x556D, 0x556E, 0x5570, 0x5572]

for off in ADDRS:
    ea = DS_BASE + off
    name = ida_name.get_name(ea)
    cmt0 = idc.get_cmt(ea, 0) or ""
    cmt1 = idc.get_cmt(ea, 1) or ""
    print(f"\n=== DS:{off:#06x} (ea {ea:#x}) name={name!r} ===")
    if cmt0:
        print(f"  cmt0: {cmt0}")
    if cmt1:
        print(f"  cmt1: {cmt1}")
    refs = list(idautils.XrefsTo(ea, 0))
    print(f"  {len(refs)} xrefs:")
    for x in refs:
        fn = ida_funcs.get_func(x.frm)
        fname = idc.get_func_name(fn.start_ea) if fn else "<no func>"
        is_code = ida_bytes.is_code(ida_bytes.get_flags(x.frm))
        kind = "CODE" if is_code else "DATA"
        print(f"    [{kind}] {fname} @ {x.frm:#x} type={x.type}")

print("\n=== word_32DCE / word_32DD0 declared addresses ===")
for nm in ["word_32DCE", "word_32DD0", "word_32DC0", "word_32DC2", "g_lockStatusFlags", "g_lockUnlockedMask"]:
    ea = idc.get_name_ea_simple(nm)
    if ea == idc.BADADDR:
        print(f"{nm}: NOT FOUND")
        continue
    print(f"{nm}: ea={ea:#x}  DS-relative={ea - DS_BASE:#06x}")
