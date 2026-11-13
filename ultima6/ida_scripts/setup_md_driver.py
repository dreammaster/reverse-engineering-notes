"""
First-run set-up for the Martian Dreams video driver blocks (tools/extract_md_drivers.py): raw 16-bit code that begins with a table of near entry offsets (word 0 is a flag the code sets
while it runs, word n the entry n; the game copies the entries into its far pointers dword_28126...).  IDA's binary loader makes a 32-bit segment: switch it to 16 bits, name the table and
start the analysis from the entries.  The names say which pointer of MARTIAN.EXE each entry becomes (sub_200F0 reads TM, sub_2024B SUPERTM).
"""

import idc
import idautils
import ida_auto
import ida_bytes
import ida_funcs
import ida_name
import ida_segment

TM = {2: 'word_287A4', 4: 'word_2879C', 6: 'word_287A0', 8: 'dword_2812E', 10: 'dword_28132', 12: 'dword_28136', 14: 'dword_2813A_draw_shape', 16: 'dword_2813E', 18: 'dword_28142',
      20: 'word_28146', 22: 'dword_2814A_present', 24: 'dword_2814E_clear', 26: 'dword_28152', 28: 'dword_28156', 30: 'word_2815A', 32: 'word_2815E', 34: 'word_28162', 36: 'dword_28166',
      38: 'word_2816A', 40: 'word_2816E', 42: 'dword_28172_circle', 44: 'word_2818A', 50: 'word_28176'}
SUPERTM = {2: 'dword_2817A_draw_rotated', 4: 'word_2817E', 6: 'dword_28182', 8: 'word_28186', 10: 'word_2818E'}

seg = ida_segment.get_first_seg()
start, end = seg.start_ea, seg.end_ea
ida_bytes.del_items(start, 0, end - start)
seg.bitness = 0                       # 16-bit
ida_segment.update_segm(seg)
idc.set_segm_attr(start, idc.SEGATTR_BITNESS, 0)

names = SUPERTM if (end - start) > 25000 else TM  # (SUPERTM_0 is 30053 bytes, TM_0 19935)
entries = []
for off, name in sorted(names.items()):
    target = ida_bytes.get_word(start + off)
    if 0 < target < end - start:
        ida_funcs.add_func(start + target)
        entries.append((start + target, name))
ida_auto.auto_wait()
for ea, name in entries:
    ida_name.set_name(ea, 'drv_' + name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN | ida_name.SN_FORCE)
ida_name.set_name(start, "reentry_flag", ida_name.SN_NOCHECK | ida_name.SN_NOWARN | ida_name.SN_FORCE)
if names is SUPERTM:
    # the entry 0x15E9 jumps through the table at 0x4AC (0x4EC for the routine at 0x7B0 that only measures the box): index = quarter-turn code (0 exactly 0 degrees, 1 above 0 and below 90, 2 exactly 90,
    # 3 above 90 and below 180) + 4 if the size is 256 or more + 8 or 16 for the forms the flags argument mirrors + 24 when the angle is 180 or more; each target is reached by an indirect jump
    for table, tag in ((0x4AC, "draw"), (0x4EC, "measure")):
        ida_name.set_name(start + table, "jump_%s" % tag, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
        for i in range(32):
            target = ida_bytes.get_word(start + table + 2 * i)
            if 0 < target < end - start:
                ida_funcs.add_func(start + target)
                ida_name.set_name(start + target, "%s_%02d" % (tag, i), ida_name.SN_NOCHECK | ida_name.SN_NOWARN | ida_name.SN_FORCE)
ida_auto.auto_wait()
print("%d functions" % len(list(idautils.Functions())))
