"""
One-off fix for mm2.idb, which IDA's MZ loader left with (a) no DGROUP contents
(the "trailing data") and (b) 14 empty carve-outs where the overlays load.

  1. Load DGROUP from the MM2.EXE tail into seg019, applying its 2 relocations.
  2. Replace IDA's per-relocation overlay carve-outs (seg005..seg018) with the two
     real overlay windows OVL_A / OVL_B (selector 1000h = the code segment).
  3. Name segments and make DS default to DGROUP for the game code.

Safe to re-run.
"""
import os, sys
import ida_segment, idc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

lay = layout()
dg = lay.dgroup
body, relocs = lay.read_segment(dg)
assert len(body) == DGROUP[1] - DGROUP[0], (len(body), DGROUP)

# 1. DGROUP contents -------------------------------------------------------
load_bytes(DGROUP[0], body)
print(f"loaded DGROUP {len(body):#x} bytes at {DGROUP[0]:#x}")
if idc.get_wide_word(DGROUP[0] + relocs[0][0]) < 0x1000:      # not yet relocated
    print("applied", apply_relocs(relocs, DGROUP[0], 0), "relocs", relocs)

# 2. overlay windows -------------------------------------------------------
for ea in [s.start_ea for s in (ida_segment.getnseg(i) for i in range(ida_segment.get_segm_qty()))
           if OVL_A[0] <= s.start_ea < OVL_B[1]]:
    ida_segment.del_segm(ea, ida_segment.SEGMOD_KILL)
for name, (a, b) in (("OVL_A", OVL_A), ("OVL_B", OVL_B)):
    if not idc.add_segm_ex(a, b, CODE_SEL, 0, idc.saRelByte, idc.scPub, 0):
        print("add_segm failed", name)
    idc.set_segm_name(a, name)
    idc.set_segm_class(a, "CODE")

# 3. names / DS default ----------------------------------------------------
idc.set_segm_name(DGROUP[0], "DGROUP")
idc.set_segm_class(DGROUP[0], "DATA")
idc.set_segm_name(STACK[0], "STACK")
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    if s.start_ea < OVL_B[1] and s.start_ea not in (0x177D0, 0x17D30):   # not the Plink86 stub
        idc.set_default_sreg_value(s.start_ea, "ds", DGROUP_SEL)
print("done")

# 4. Plink86 overlay thunks (12 bytes each) -------------------------------------
#    9A <off16 seg16>   call far Plink86 loader        (5)
#    dw <idx>           8000h | overlay record number   (2)
#    EA <off16 seg16>   jmp far real target             (5)
import ida_ua

for off, idx, toff in lay.thunks():
    ea = BASE + off
    owner = lay.thunk_owner(idx)
    for a in (ea, ea + 5, ea + 7):
        idc.del_items(a, idc.DELIT_SIMPLE, 5 if a != ea + 5 else 2)
    idc.create_insn(ea)
    idc.create_word(ea + 5)
    idc.create_insn(ea + 7)
    tag = owner.name.split(".")[0] if owner else "res"
    idc.set_name(ea, f"thk_{tag}_{toff:04X}", idc.SN_NOCHECK | idc.SN_NOWARN)
    idc.set_cmt(ea + 5, f"thunk index {idx:#06x}: " + (owner.name if owner else "resident"), 0)
print("thunks defined:", len(lay.thunks()))

# 5. Plink86 runtime names ---------------------------------------------------
PLINK = {
    0x17A52: ("plink_load_segment_far", "far entry, CX = 1-based segment-table row to load; AH=0 -> fatal on error"),
    0x17A57: ("plink_load_segment", "loads row CX (and any rows it chains to) via plink_read_segment"),
    0x17AB4: ("plink_thunk_handler", "reads the thunk index word after the far call's return address, loads that overlay, returns to the thunk's jmp"),
    0x17B14: ("plink_thunk_entry", "target of every 9A far call in the 12-byte overlay thunks (cs:6D8Ah..77CEh)"),
    0x17981: ("plink_read_segment", "opens the row's file, seeks, reads the body to its load paragraph, applies relocs"),
    0x17A06: ("plink_apply_relocs", "reloc entries are (offset, segment) words; word at seg:offset += load base"),
    0x178BA: ("plink_seek", "DOS lseek to (row.filePara + DX) paragraphs"),
    0x178E3: ("plink_read", "DOS read of CX paragraphs into BX:DX in <=0FFF0h chunks"),
    0x1785F: ("plink_open_file", "opens the file whose name is at segtable:row.nameOff (MM2.EXE or an .OVL)"),
    0x1783B: ("plink_close_file", ""),
    0x177F5: ("plink_evict_overlapping", "clears the 'loaded' bit (8000h) of every row whose window overlaps row DI"),
    0x17B90: ("plink_find_segment_for_address", "AX = cs offset paragraph -> row number of the loaded overlay covering it"),
    0x17D24: ("plink_abort", "closes files and exits with code 0FFh"),
    0x177D0: ("plink_nullsub_1", ""),
}
for ea, (name, cmt) in PLINK.items():
    idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN)
    if cmt:
        idc.set_func_cmt(ea, cmt, 0)
idc.set_name(0x16BF0, "plink_segtable", idc.SN_NOCHECK | idc.SN_NOWARN)
idc.set_cmt(0x16BF0, "Plink86 segment table: 16-byte rows, see mm2/docs/exe-layout.md", 0)
print("done (plink)")

# 6. re-run operand analysis now that DS is known
idc.plan_and_wait(0x10000, 0x17D30)
print("reanalysed")
