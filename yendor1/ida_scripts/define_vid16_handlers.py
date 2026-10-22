"""
VID16.DRV: define the INT 62h service routine in the database.

The ISR starts at seg000:0000 and dispatches on AH through a table of near
offsets at DS:001Bh (DS = seg001's paragraph); the stored offsets are already
IDA's seg000 offsets (at run time the resident image sits at CS:0100h behind its
PSP, and the ISR adds 0100h back). This creates a function at the ISR and at
each handler, named vid_fnXX (XX = AH value) until each is understood.
"""
import idc, ida_segment, ida_funcs, ida_auto, ida_name

seg0 = ida_segment.get_segm_by_name("seg000")
seg1 = ida_segment.get_segm_by_name("seg001")
base = ((seg1.start_ea >> 4) << 4) + 0x1B
org = seg0.start_ea

idc.create_insn(org)
ida_funcs.add_func(org)
ida_name.set_name(org, "int62_isr", ida_name.SN_NOWARN)
seen = {}
for ah in range(0x2E):
    w = idc.get_wide_word(base + ah * 2)
    if w == 0:
        continue
    ea = org + w
    # the byte before many handlers is the shared "add sp,2" prologue that
    # discards the AX saved by the ISR; the table points at it.
    idc.create_insn(ea)
    ida_funcs.add_func(ea)
    name = f"vid_fn{ah:02X}"
    if ea in seen:
        name += f"_alias_{seen[ea]:02X}"
    else:
        seen[ea] = ah
    ida_name.set_name(ea, name, ida_name.SN_NOWARN)
ida_auto.auto_wait()
print("defined", len(seen), "handlers")
