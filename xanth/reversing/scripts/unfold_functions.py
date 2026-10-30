"""
Clears FUNC_HIDDEN ("collapsed" in the IDA GUI) on every function and every
function tail chunk, so ida_loader.gen_file(OFILE_ASM) prints real
disassembly instead of '[N BYTES: COLLAPSED FUNCTION ...]' placeholders.
Retail Shannara.idb had 257 such placeholders (found 2026-10-12 while reading
the save-state code); they silently hid function bodies from every grep.

    idat.exe -A -S"reversing/scripts/unfold_functions.py" Shannara.idb
Idempotent. Exits idat explicitly when run headless.
"""
import idc
import idautils
import ida_funcs

n_func = n_chunk = 0
for ea in idautils.Functions():
    f = ida_funcs.get_func(ea)
    if f.flags & ida_funcs.FUNC_HIDDEN:
        f.flags &= ~ida_funcs.FUNC_HIDDEN
        ida_funcs.update_func(f)
        n_func += 1
    it = ida_funcs.func_tail_iterator_t(f)
    ok = it.first()
    while ok:
        ch = ida_funcs.get_fchunk(it.chunk().start_ea)
        if ch and ch.flags & ida_funcs.FUNC_HIDDEN:
            ch.flags &= ~ida_funcs.FUNC_HIDDEN
            ida_funcs.update_func(ch)
            n_chunk += 1
        ok = it.next()

with open(idc.get_idb_path() + ".unfold.log", "w") as lf:
    lf.write(f"unfolded {n_func} functions, {n_chunk} chunks\n")
idc.save_database(idc.get_idb_path(), 0)

import ida_kernwin
if not ida_kernwin.is_idaq():
    idc.qexit(0)
