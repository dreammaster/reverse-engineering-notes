"""
Read-only: locates the "PICTURES.VGA" filename string in the binary and
its xrefs, to find the actual picture-loading code path (as opposed to
the WORLD.DAT-offset resource stubs at 0x27B42-0x2801A, which turned out
to read from WORLD.DAT given their offsets top out around 1.75MB,
matching WORLD.DAT's exact size, not PICTURES.VGA's 12.5MB).

    .\run_ida_script.ps1 find_pictures_vga.py -NoExport
"""
import idautils
import idc
import ida_bytes
import ida_funcs

for ea in idautils.Strings():
    try:
        s = str(ea)
    except Exception:
        continue
    if "PICTURE" in s.upper():
        print(f"{ea.ea:#x}  {s!r}")
        for x in idautils.XrefsTo(ea.ea):
            f = ida_funcs.get_func(x.frm)
            fn = idc.get_func_name(f.start_ea) if f else "?"
            print(f"    xref from {x.frm:#x} ({fn})")
