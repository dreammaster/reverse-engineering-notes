import idc, ida_funcs, sys
names = {"sub_2220D":0x2220D,"sub_2C20F":0x2C20F,"sub_2B5A4":0x2B5A4,"sub_37940":0x37940,"sub_241CC":0x241CC,"sub_1AC65":0x1AC65,"sub_1DF45":0x1DF45}
out=open(r"C:\dev\ultima\ultima6\u6\use_funcs2.txt","w")
for n,ea in names.items():
    out.write("=== %s %05X\n"%(n, ea))
    f = ida_funcs.get_func(ea)
    if not f:
        idc.create_insn(ea); ida_funcs.add_func(ea); f=ida_funcs.get_func(ea)
    if not f: continue
    cur=f.start_ea
    while cur<f.end_ea:
        if not idc.is_code(idc.get_full_flags(cur)):
            idc.del_items(cur,0,4); k=idc.create_insn(cur) or 1
        else: k=idc.get_item_size(cur)
        out.write("%05X %s\n"%(cur, idc.generate_disasm_line(cur,0)))
        cur+=k
out.close()
