import idc
import ida_segment

# Savage Empire cmd_use (sub_35E46): the whole function with addresses, and the two tables of its switch (81 type words at cs:8965, 81 handler offsets at cs:8965 + 0xA2)
out = open(r"C:\dev\ultima\ultima6\se\se_use.txt", "w")
ea = idc.get_name_ea_simple("sub_35E46")
seg = ida_segment.getseg(ea)
base = 0x36537 - 0x8965  # (cs:0; the segment of IDA starts one byte higher)
out.write("sub_35E46 %05X, segment base %05X\n" % (ea, base))
types = [idc.get_wide_word(base + 0x8965 + 2 * i) for i in range(81)]
hands = [idc.get_wide_word(base + 0x8965 + 0xA2 + 2 * i) for i in range(81)]
for t, h in zip(types, hands):
    out.write("type %03X handler %04X linear %05X\n" % (t, h, base + h))
out.write("=== code\n")
end = idc.get_func_attr(ea, idc.FUNCATTR_END)
cur = ea
while cur < end:
    if not idc.is_code(idc.get_full_flags(cur)):
        idc.del_items(cur, 0, 4)
        n = idc.create_insn(cur) or 1
    else:
        n = idc.get_item_size(cur)
    out.write("%05X %s\n" % (cur, idc.generate_disasm_line(cur, 0)))
    cur += n
out.close()
