"""
First-load setup for SYSTEM.INTERP -- the DOS Wizardry x86 UCSD p-machine.

Load the raw 16 KB image with the Binary-file loader (base 0, 8086), then run
this once via the driver:

    copy extracted\\wiz1\\SYSTEM.INTERP wiz1_interp
    idat.exe -A -p8086 -T"Binary file" ^
        -S"ida_scripts\\batch_run_and_export.py ida_scripts\\setup_interp.py" ^
        -o"wiz1_interp.idb" wiz1_interp

Byte 0 is `E9 18 00` (jmp) into the real entry; the code then relocates itself
(`mov ax,cs; sub ax,0x1000; mov ss,ax; ...`). We analyze it flat: seg000 at
base 0, offset == file offset.
"""

import ida_segment
import ida_bytes
import ida_auto
import ida_entry
import ida_funcs
import idautils
import idc

END = 0x4000

seg = ida_segment.getnseg(0)
print(f"seg000: {seg.start_ea:#x}-{seg.end_ea:#x} bitness={seg.bitness}")
ida_segment.set_segm_name(seg, "INTERP")
ida_segment.set_segm_addressing(seg, 0)  # 16-bit

# Entry: the initial jmp at offset 0.
ida_bytes.del_items(0, ida_bytes.DELIT_SIMPLE, 3)
idc.create_insn(0)
ida_entry.add_entry(0, 0, "interp_start", 1)

# Follow the jmp target explicitly, then let auto-analysis sweep.
tgt = idc.get_operand_value(0, 0)
print(f"jmp target = {tgt:#x}")
if 0 < tgt < END:
    idc.create_insn(tgt)
    ida_funcs.add_func(tgt)
    idc.set_name(tgt, "interp_reloc", idc.SN_CHECK)

ida_auto.plan_and_wait(0, END)

funcs = list(idautils.Functions(0, END))
print(f"functions: {len(funcs)}")
for ea in funcs[:40]:
    print(f"  {ea:#06x}  {idc.get_func_name(ea)}")

# Rough coverage: how many bytes are code vs data/undefined.
code = sum(1 for ea in range(END) if ida_bytes.is_code(ida_bytes.get_flags(ea)))
print(f"code bytes: {code}/{END} ({100*code//END}%)")
