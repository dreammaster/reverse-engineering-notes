# PROG5's mode 4 (and the end of mode 3) is reached only through the dispatch table at DS:0A93h, so IDA left it as data. Decode it.
import idc
import ida_ida

end = idc.get_name_ea_simple("loc_10AB7")
start = end - (0xAB7 - 0x7C9)
print("decode range %x..%x" % (start, end))
idc.del_items(start, idc.DELIT_SIMPLE, end - start)
ea = start
n = 0
while ea < end:
    ln = idc.create_insn(ea)
    if ln == 0:
        print("cannot decode at %x (byte %02x)" % (ea, idc.get_wide_byte(ea)))
        idc.create_byte(ea)
        ln = 1
    ea += ln
    n += 1
print("decoded %d instructions" % n)
idc.add_func(start) if False else None
