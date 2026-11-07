import idc, ida_funcs

names = ("52F40 59480 59641 598F6 59964 59A5E 59B41 59CC9 59D15 59E49 59EE7 59FD7 5A01D 5A17C 5A27E 5A39C 5A5C5 5A7BF 5A93C 5A9C5 5AAA1 5ADA9 5AE03 5AFA9 5B0A0 5B412 5B490 5B6F7 5B83A 5B993 "
         "5BC07 5C2E3 5C418 5C518 5C6C7 5C7C4 5C8D9 5C9F8 5CAFA 5CC45 5CE18 5CE4D 5CF05 5D046 5D298 5D313 5D357 5D4B0 5D5A7 5D62E 5D8CA 5D93E 5DA12 5DB79 5DECB 5E0F2 5E445 5E690 5E7E8 5E988 "
         "5EC84 5ED76 5EDA3 5EE03 5EF0C 5EFB0 5EFC8 5F06C 5F0DE 5F166 5F25E").split()
out = open(r"C:\dev\ultima\ultima6\md\recipe_ends.txt", "w")
for n in names:
    ea = int(n, 16)
    f = ida_funcs.get_func(ea)
    if not f:
        out.write("%s NO FUNCTION\n" % n)
        continue
    words = [idc.get_wide_word(f.end_ea + 2 * i) for i in range(48)]
    out.write("%s %X %X %s\n" % (n, f.start_ea, f.end_ea, " ".join("%04X" % w for w in words)))
out.close()
