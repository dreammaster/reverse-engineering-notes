"""Read-only: per-function signature (size + mnemonic sequence hash) to sigs_<root>.txt for cross-binary name matching."""
import os, hashlib
import idc, idautils
root = os.path.splitext(idc.get_root_filename())[0]
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sigs_%s.txt" % root)
with open(out, "w") as fh:
    for fea in idautils.Functions():
        end = idc.get_func_attr(fea, idc.FUNCATTR_END)
        seq = []
        ea = fea
        while ea < end:
            seq.append(idc.print_insn_mnem(ea))
            ea = idc.next_head(ea, end)
        h = hashlib.md5(" ".join(seq).encode()).hexdigest()[:12]
        fh.write("%X\t%s\t%d\t%d\t%s\n" % (fea, idc.get_func_name(fea), end - fea, len(seq), h))
