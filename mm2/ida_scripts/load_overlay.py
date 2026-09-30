"""
Turn a *copy of mm2.idb* into the database for one Plink86 overlay.  The database
file name gives the overlay (mm2/ovl/2PLAY.idb -> 2PLAY.OVL).  The copy already
holds the resident image + DGROUP, so near calls/jumps from the overlay into
resident code and its data references resolve.  What this script does:

  1. loads the overlay body into its window (OVL_A / OVL_B) at its real address,
     applies the overlay's relocations, and shrinks/renames the window segment;
  2. seeds code: every thunk that targets this overlay + every `push bp; mov bp,sp`;
  3. imports names/comments exported from the main database (export_names.py);
  4. tells the driver to export only the overlay's address range.

Re-run only on a fresh copy of mm2.idb -- overlay databases are hand-edited afterwards.
"""
import json, os, sys
import ida_auto, ida_bytes, ida_funcs, ida_segment, idc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
lay = layout()
rec = lay.overlay(stem)
body, relocs = lay.read_segment(rec)
start = rec.ida_start
end = start + len(body)
win = OVL_A if start == OVL_A[0] else OVL_B
print(f"{rec.name}: {len(body):#x} bytes at {start:#x}..{end:#x}, {len(relocs)} relocs")

# 1. load ------------------------------------------------------------------
load_bytes(start, body)
for off, seg in relocs:
    ea = (seg + mm2_layout.IDA_BASE_PARA) * 16 + off
    assert start <= ea < end, hex(ea)
    idc.patch_word(ea, (idc.get_wide_word(ea) + mm2_layout.IDA_BASE_PARA) & 0xFFFF)
seg_name = "ovl_" + stem
ida_segment.set_segm_end(win[0], end, ida_segment.SEGMOD_KEEP)   # shrink to the overlay
idc.set_segm_name(win[0], seg_name)
idc.set_default_sreg_value(win[0], "ds", DGROUP_SEL)

# 2. code seeds ------------------------------------------------------------
seeds = set()
for off, idx, toff in lay.thunks():
    if lay.thunk_owner(idx) is rec:
        seeds.add(BASE + toff)
for i in range(len(body) - 3):
    if body[i:i + 3] in (b"\x55\x8B\xEC", b"\x55\x89\xE5"):
        seeds.add(start + i)
seeds = sorted(seeds)
for i, ea in enumerate(seeds):
    idc.create_insn(ea)
    if not ida_funcs.add_func(ea):          # e.g. the window start abuts other data: give an explicit end
        ida_funcs.add_func(ea, seeds[i + 1] if i + 1 < len(seeds) else end)
print(f"seeded {len(seeds)} function starts")
ida_auto.auto_wait()


def undefined_runs():
    """Yield (start, length) of bytes that are neither code nor a sized data item."""
    ea = start
    while ea < end:
        fl = ida_bytes.get_flags(ea)
        if ida_bytes.is_unknown(fl):
            st = ea
            while ea < end and ida_bytes.is_unknown(ida_bytes.get_flags(ea)):
                ea += 1
            yield st, ea - st
        else:
            ea += max(idc.get_item_size(ea), 1)


# Compiler alignment padding (90h) and code only reachable through jump tables /
# fall-through after a call that never returns are left undefined by the seed
# pass; pick them up until nothing changes.
for _ in range(8):
    changed = False
    for st, n in list(undefined_runs()):
        ea, stop = st, st + n
        while ea < stop and ida_bytes.get_byte(ea) == 0x90 and ea + 1 < end:
            ida_bytes.create_align(ea, 1, 0)
            ea += 1
            changed = True
        if ea < stop and idc.create_insn(ea):
            changed = True
    ida_auto.auto_wait()
    if not changed:
        break
print('jump tables fixed:', fix_jump_tables(start, end))
for _ in range(8):
    changed = False
    for st, n in list(undefined_runs()):
        ea, stop = st, st + n
        while ea < stop and ida_bytes.get_byte(ea) == 0x90 and ea + 1 < end:
            ida_bytes.create_align(ea, 1, 0); ea += 1; changed = True
        if ea < stop and idc.create_insn(ea): changed = True
    ida_auto.auto_wait()
    if not changed: break
print("bytes still undefined:", sum(n for _, n in undefined_runs()))

print('string offsets resolved:', resolve_string_offsets(start, end))

# 3. names from the main database ------------------------------------------
sn = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "build", "shared_names.json")
if os.path.exists(sn):
    d = json.load(open(sn))
    for ea, name in d["names"].items():
        idc.set_name(int(ea), name, idc.SN_NOCHECK | idc.SN_NOWARN)
    for ea, (c, r) in d["cmts"].items():
        if c: idc.set_cmt(int(ea), c, 0)
        if r: idc.set_cmt(int(ea), r, 1)
    print(f"imported {len(d['names'])} names from {sn}")
ida_auto.auto_wait()

exec(compile(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'apply_names.py')).read(), 'apply_names.py', 'exec'))

EXPORT_RANGE = (start, end)
print("functions in overlay:", sum(1 for ea in range(start, end) if (f := ida_funcs.get_func(ea)) and f.start_ea == ea))
