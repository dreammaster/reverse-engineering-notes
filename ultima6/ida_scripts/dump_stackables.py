import idc, ida_segment
out = open(r"C:\dev\ultima\ultima6\build\stackables.txt", "a")
s = ida_segment.get_segm_by_name("dseg")
base = s.start_ea
name = idc.get_idb_path()
if "md_game" in name:
    off, n = 0xDD6, 0x15
elif "se_game" in name:
    off, n = 0x806, 0x30
else:
    off, n = 0, 0
out.write("%s dseg %05X off %X n %d\n" % (name, base, off, n))
out.write(" ".join("%X" % idc.get_wide_word(base + off + 2 * i) for i in range(n)) + "\n")
out.close()
