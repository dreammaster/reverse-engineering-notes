"""
Some functions in these IDBs were left "collapsed" (Numpad- in the IDA
GUI, i.e. ida_funcs.FUNC_HIDDEN set) by whatever earlier manual session
touched them -- a purely cosmetic GUI display state, but it also means
ida_loader.gen_file(OFILE_ASM) prints a one-line
"[XXXX BYTES: COLLAPSED FUNCTION Name. PRESS NUMPAD+ TO EXPAND]"
placeholder for that function instead of its real instructions in the
exported .asm -- silently hiding its body from every grep/Read of the
.asm from this pipeline. Found this tracing vocab_load in gatemain.idb
(wanted to read it to cross-check VOCAB.DAT's on-disk format, per
Paul's pointer to the installed game at c:\\games\\gw, and hit a wall).

This clears FUNC_HIDDEN on every collapsed function in the current IDB
so the next export shows real disassembly everywhere.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName unfold_functions.py
    .\\run_ida_script.ps1 -Idb gate -ScriptName unfold_functions.py
"""

import idc
import idautils
import ida_funcs

count = 0
names = []
for ea in idautils.Functions():
    flags = idc.get_func_attr(ea, idc.FUNCATTR_FLAGS)
    if flags & ida_funcs.FUNC_HIDDEN:
        idc.set_func_attr(ea, idc.FUNCATTR_FLAGS, flags & ~ida_funcs.FUNC_HIDDEN)
        count += 1
        names.append(idc.get_func_name(ea))

print(f"unfolded {count} collapsed functions")
for n in names:
    print(f"  {n}")
