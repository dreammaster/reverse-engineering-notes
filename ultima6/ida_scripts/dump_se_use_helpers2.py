import idc, ida_funcs, re
names = "1C70E 1ABE3 14746 1B3BF 1ACDD 1A72C 1A96A 17FC8 1E046 1BC8E 2E645 2E61C 33DFF 33EF8 2EC00 1ECD1 1B5FA 1B486 15D66 242E4".split()
DS = 0x3FA60
S50 = 0x3F650

def cstr(ea):
    s = b""
    for i in range(160):
        b = idc.get_wide_byte(ea + i)
        if b == 0:
            break
        s += bytes([b])
    return s.decode("latin1").replace(chr(10), "<NL>")

out = open(r"C:\dev\ultima\ultima6\se\se_use_helpers2.txt", "w")
for n in names:
    ea = int(n, 16)
    f = ida_funcs.get_func(ea)
    if not f:
        out.write("=== sub_%s NO FUNCTION\n" % n)
        continue
    out.write("=== sub_%s %05X-%05X\n" % (n, f.start_ea, f.end_ea))
    cur = f.start_ea
    lines = []
    while cur < f.end_ea:
        if not idc.is_code(idc.get_full_flags(cur)):
            idc.del_items(cur, 0, 4)
            k = idc.create_insn(cur) or 1
        else:
            k = idc.get_item_size(cur)
        lines.append((cur, idc.generate_disasm_line(cur, 0)))
        cur += k
    for i, (a, t) in enumerate(lines):
        t = t.split(";")[0].rstrip()
        if re.match(r"(push\s+cs|pop\s+cx)\s*$", t):
            continue
        note = ""
        m = re.match(r"mov\s+ax, ([0-9A-F]+)h$", t)
        if m and any("sub_18AB9" in l[1] for l in lines[i + 1:i + 4]):
            prev = " ".join(l[1] for l in lines[max(0, i - 2):i])
            base = S50 if "seg050" in prev else DS if "push    ds" in prev else None
            if base:
                note = '   ; "%s"' % cstr(base + int(m.group(1), 16))
        out.write("%05X %s%s\n" % (a, t, note))
out.close()
