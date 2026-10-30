"""
Exports the open IDB as a text listing (what "Shannara Demo.asm" / "Shannara.asm"
are). Run unfold_functions.py first so no function bodies are collapsed.

    idat.exe -A -S"reversing/scripts/export_asm.py out.asm" copy-of.idb

Pass a relative output path (msys /c/... paths are not translated inside -S).
"""
import idc, ida_loader, ida_diskio, ida_kernwin, ida_ida

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "export.asm"
ida_loader.gen_file(ida_loader.OFILE_ASM, ida_diskio.fopenWT(out), 0, idc.BADADDR, 0)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
