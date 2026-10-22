"""
Dump VID16.DRV's INT 62h dispatch table: DS:001Bh (DS = the driver's data
segment), 0x2E words indexed by AH; the handler is cs:(entry + 0100h), 16-bit
wrap. Read-only.
"""
import idc, ida_segment, idautils

for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    print(ida_segment.get_segm_name(s), hex(s.start_ea), hex(s.end_ea))
seg0 = ida_segment.get_segm_by_name("seg000")
seg1 = ida_segment.get_segm_by_name("seg001")
print("seg001 selector/para:", hex(seg1.start_ea >> 4))
base = ((seg1.start_ea >> 4) << 4) + 0x1B  # DS = seg001 paragraph; IDA's seg start is not para-aligned
for ah in range(0x2E):
    w = idc.get_wide_word(base + ah * 2)
    tgt = (w + 0x100) & 0xFFFF if w else None
    print(f"AH={ah:02X} entry={w:04X} -> seg000:{tgt:04X}" if tgt is not None else f"AH={ah:02X} (unused)")
