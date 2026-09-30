"""
Read-only: investigates the long-open "word_332D8-33306 encoded effect
descriptor cluster" write-site mystery (see project memory /
file-formats.md's "A follow-up investigation..." note). Checks the
segment/address facts needed to test one hypothesis: that
LoadClueBookSpellEntry's own copy destination (es:0x5A5A, es=word_2E4AA)
might physically coincide with the word_332D0.. cluster's own address
range, which is restored to `seg129` right after the copy.

    .\run_ida_script.ps1 investigate_encoded_effect_cluster.py -NoExport
"""
import idc
import ida_bytes
import ida_name
import ida_segment
import ida_xref

names = ["word_332D0", "word_332D2", "word_332D4", "word_332D6", "word_332D8",
         "word_332DA", "word_332DC", "word_332DE", "word_332E0", "word_33300",
         "word_33302", "word_33306", "word_2E4AA", "word_2E548"]

lines = []
for n in names:
    ea = ida_name.get_name_ea(idc.BADADDR, n)
    if ea == idc.BADADDR:
        lines.append(f"{n}: NOT FOUND")
        continue
    seg = ida_segment.getseg(ea)
    segname = ida_segment.get_segm_name(seg) if seg else "?"
    lines.append(f"{n}: ea={ea:#x} seg={segname} (seg base={seg.start_ea:#x} sel={seg.sel:#x})")

# seg129's own base, for comparison against word_2E4AA's likely runtime value
seg129 = ida_segment.get_segm_by_name("seg129")
if seg129:
    lines.append(f"\nseg129: start_ea={seg129.start_ea:#x} sel={seg129.sel:#x} para={seg129.sel:#x}")

# Every xref (read AND write) to word_332D8 specifically, using IDA's own xref database
# rather than a text grep, to catch anything a plain-text search might miss.
target_names = ["word_332D8", "word_332DA", "word_332DC", "word_332DE"]
lines.append("\n=== Full xref dump (IDA's own database, not text grep) ===")
for n in target_names:
    ea = ida_name.get_name_ea(idc.BADADDR, n)
    if ea == idc.BADADDR:
        continue
    lines.append(f"--- {n} ({ea:#x}) ---")
    xb = ida_xref.get_first_dref_to(ea)
    while xb != idc.BADADDR:
        is_write = ida_xref.get_first_dref_from(xb) != idc.BADADDR  # rough heuristic, refine if needed
        fn = ida_name.get_name(xb)
        lines.append(f"  dref from {xb:#x} ({fn})")
        xb = ida_xref.get_next_dref_to(ea, xb)

out_path = r"C:\dev\yendor\yendor2\ida_scripts\encoded_effect_cluster_investigation.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))
print(f"wrote {out_path}")
