import idc, ida_funcs, re
names = "1BAD6 1AD09 1B486 1B5FA 1C1C3 1C246 2E5D2 1A0C7 14264 17FC8 17534 1E046 18858 18AB9 2B429 242E4 24EBC 15D66 19D2B 1AB94 1ECD1 2DBD3 111F5 1B701 1B59D 1AC65 18C09 1B26F 1A62D 2E7FA".split()
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

out = open(r"C:\dev\ultima\ultima6\se\se_use_helpers.txt", "w")
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
