import idc, idautils, ida_funcs, ida_bytes
out = open(r"C:\dev\ultima\ultima6\se\objlist_loader.txt", "w")
ea = idc.get_name_ea_simple("aSavegameObjlis")
out.write("string at %05X\n" % ea)
off = ea - 0x3FA60
hits = []
for f in idautils.Functions():
    for h in idautils.FuncItems(f):
        if idc.print_insn_mnem(h) == "mov" and idc.get_operand_type(h, 1) == idc.o_imm and idc.get_operand_value(h, 1) == off:
            hits.append((f, h))
out.write("offset %X hits %s\n" % (off, [(hex(f), hex(h)) for f, h in hits]))
for f, h in hits:
    fn = ida_funcs.get_func(f)
    out.write("=== func %05X-%05X\n" % (fn.start_ea, fn.end_ea))
    cur = fn.start_ea
    while cur < fn.end_ea:
        if not idc.is_code(idc.get_full_flags(cur)):
            idc.del_items(cur, 0, 4)
            k = idc.create_insn(cur) or 1
        else:
            k = idc.get_item_size(cur)
        out.write("%05X %s\n" % (cur, idc.generate_disasm_line(cur, 0).split(";")[0].rstrip()))
        cur += k
out.close()
