"""Dump the Plink86 layout of MM2.EXE: header, trailing data, segment table, thunks.

    python plink_info.py [path\\to\\MM2.EXE]
"""
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_layout import Layout  # noqa: E402

lay = Layout(sys.argv[1] if len(sys.argv) > 1 else None)
print(f"{lay.exe_path}: {len(lay.data)} bytes")
print(f"MZ header {lay.hdr_size:#x} bytes, image ends at {lay.image_size:#x}, "
      f"trailing data {lay.trailing} bytes ({lay.trailing:#x}), IDA maps {lay.loaded_size:#x}")
print(f"cs:ip={lay.cs:#x}:{lay.ip:#x} ss:sp={lay.ss:#x}:{lay.sp:#x} relocs={lay.nreloc}")
print()
print(f"{'#':>2} {'file':10} {'flags':>6} {'relocs':>6} {'load range (rel para)':>22} {'IDA ea':>8} "
      f"{'file pos':>9} {'body pos':>9} {'body size':>9}")
for r in lay.records:
    print(f"{r.index + 1:>2} {r.name:10} {r.flags:#06x} {r.reloc_count:>6} "
          f"{r.start_para:#8x}-{r.end_para:#06x}   {r.ida_start:#8x} "
          f"{r.file_pos:#9x} {r.body_pos:#9x} {r.body_size:>9}")
print()
th = lay.thunks()
cnt = collections.Counter(t[1] for t in th)
print(f"{len(th)} thunks at cs:{Layout.THUNK_TABLE_REL:#x}; per owner:")
for idx, n in sorted(cnt.items()):
    o = lay.thunk_owner(idx)
    print(f"  {idx:#06x} {'(resident)' if o is None else o.name:12} {n}")
