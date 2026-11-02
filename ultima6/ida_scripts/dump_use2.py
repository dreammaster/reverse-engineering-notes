import idc, ida_bytes, idautils
out=open(r"C:\dev\ultima\ultima6\u6\use_handlers2.txt","w")
ea=0x35C4B
idc.del_items(ea, 0, 0x35CE0-ea)
cur=ea
while cur<0x35CE0:
    n=idc.create_insn(cur)
    if not n: n=1
    out.write("%05X %s\n"%(cur, idc.generate_disasm_line(cur,0)))
    cur+=n
out.close()
