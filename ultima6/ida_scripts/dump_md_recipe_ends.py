import idc, ida_funcs

names = ("5987C 5A7BF 5AAA1 5AE03 5A9C5 5AFA9 5E0F2").split()
out = open(r"C:\dev\ultima\ultima6\md\recipe_ends2.txt", "w")
for n in names:
    ea = int(n, 16)
    f = ida_funcs.get_func(ea)
    if not f:
        out.write("%s NO FUNCTION\n" % n)
        continue
    words = [idc.get_wide_word(f.end_ea + 2 * i) for i in range(48)]
    out.write("%s %X %X %s\n" % (n, f.start_ea, f.end_ea, " ".join("%04X" % w for w in words)))
out.close()
