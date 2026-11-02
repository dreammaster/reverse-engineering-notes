import idc
out=open(r"C:\dev\ultima\ultima6\u6\use_fix.txt","w")
def words(a,n):
    idc.del_items(a,0,2*n)
    for i in range(n): idc.create_word(a+2*i)
    out.write("words at %05X: %s\n"%(a," ".join("%04X"%idc.get_wide_word(a+2*i) for i in range(n))))
def code(a,b):
    cur=a
    while cur<b:
        idc.del_items(cur,0,4); k=idc.create_insn(cur) or 1
        out.write("%05X %s\n"%(cur, idc.generate_disasm_line(cur,0)))
        cur+=k
words(0x32F7E,8)
code(0x32F8E,0x32FB8)
# strings
for ea in range(0x3E62C,0x3E650,2):
    p=idc.get_wide_word(ea)
    s=""
    a=0x3D850+p
    while idc.get_wide_byte(a): s+=chr(idc.get_wide_byte(a)); a+=1
    out.write("%05X -> %04X %r\n"%(ea,p,s))
out.close()
