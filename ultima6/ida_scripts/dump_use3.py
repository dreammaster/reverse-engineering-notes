import idc
out=open(r"C:\dev\ultima\ultima6\u6\use_handlers3.txt","w")
def dump(a,b,title):
    out.write("=== %s\n"%title)
    cur=a
    while cur<b:
        if not idc.is_code(idc.get_full_flags(cur)):
            idc.del_items(cur,0,4); n=idc.create_insn(cur) or 1
        else: n=idc.get_item_size(cur)
        out.write("%05X %s\n"%(cur, idc.generate_disasm_line(cur,0)))
        cur+=n
dump(0x34919,0x34AC1,"vehicle 34919")
dump(0x34AC1,0x34B60,"sub_34AC1")
out.close()
