"""Debug helper: write the disassembly of ranges listed in mm2/build/dump_req.txt
('<hex start> <hex end>' per line) to mm2/build/dump_out.txt.  Run with -NoExport."""
import os, idc
b = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")
out = []
for line in open(os.path.join(b, "dump_req.txt")):
    if not line.strip(): continue
    s, e = (int(x, 16) for x in line.split()[:2])
    ea = s
    while ea < e:
        out.append("%05X %s" % (ea, idc.generate_disasm_line(ea, 0)))
        ea = idc.next_head(ea, e)
open(os.path.join(b, "dump_out.txt"), "w").write("\n".join(out))
print(len(out), "lines")
