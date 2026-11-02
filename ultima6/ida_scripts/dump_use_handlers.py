import idc, idautils, ida_bytes, ida_funcs
T = 0x35AF7
BASE = 0x2F690
out = open(r"C:\dev\ultima\ultima6\u6\use_handlers.txt", "w")
seen = {}
for k in range(85):
    t = ida_bytes.get_word(T + 2 * k)
    h = ida_bytes.get_word(T + 170 + 2 * k)
    ea = BASE + h
    out.write("USE type %03X handler %05X\n" % (t, ea))
    if ea in seen:
        out.write("   same as type %03X\n" % seen[ea])
        continue
    seen[ea] = t
    cur = ea
    n = 0
    while n < 60:
        out.write("   %05X %s\n" % (cur, idc.generate_disasm_line(cur, 0)))
        m = idc.print_insn_mnem(cur)
        if m in ("retf", "retn"):
            break
        nxt = idc.next_head(cur)
        if m == "jmp" and idc.get_operand_value(cur, 0) in (0x35F4D + 0, ) :
            break
        cur = nxt
        n += 1
out.close()
