import idc, ida_segment
out = open(r"C:\dev\ultima\ultima6\se\probe.txt", "w")
for ea in (0x3FE4C, 0x4B0A2, 0x2DBD3):
    s = ida_segment.getseg(ea)
    out.write("%05X seg %s %05X-%05X sel %X\n" % (ea, ida_segment.get_segm_name(s), s.start_ea, s.end_ea, s.sel))
s = ida_segment.get_segm_by_name("seg050")
out.write("seg050 %05X sel %X\n" % (s.start_ea, s.sel) if s else "no seg050\n")
for a in (0x2324, 0x374, 0x233A):
    for base in (0, ):
        pass
out.close()
