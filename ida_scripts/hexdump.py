"""
Read-only: hex-dump ranges listed in hexdump_args.txt (one per line:
"<dseg-relative offset hex> <length dec> [w]"; trailing w = print as words)
to hexdump_out.txt. Run with -NoExport.
"""
import os
import idc
import ida_bytes
import ida_segment

here = os.path.dirname(os.path.abspath(__file__))
dseg = ida_segment.get_segm_by_name("dseg").start_ea
with open(os.path.join(here, "hexdump_args.txt")) as f, open(os.path.join(here, "hexdump_out.txt"), "w") as out:
    for line in f:
        parts = line.split()
        if len(parts) < 2:
            continue
        off, n = int(parts[0], 16), int(parts[1])
        words = len(parts) > 2 and parts[2] == "w"
        out.write("== %04X (%d)\n" % (off, n))
        data = ida_bytes.get_bytes(dseg + off, n) or b""
        if words:
            for i in range(0, len(data) - 1, 2):
                if i % 16 == 0:
                    out.write("\n%04X: " % (off + i))
                out.write("%04X " % (data[i] | (data[i + 1] << 8)))
        else:
            for i in range(0, len(data), 16):
                chunk = data[i:i + 16]
                out.write("%04X: %s  %s\n" % (off + i, " ".join("%02X" % b for b in chunk),
                          "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)))
        out.write("\n")
