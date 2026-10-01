"""
Prepare a database created from a raw *.DRV file (idat -B -c -T"Binary file") as 16-bit code:
a table of 36 near jmps at offset 0 (driver function n = offset 3n), then data tables and code.
Names the entry points drv_fnNN (see docs/drivers.md for what each does) and creates code for
every jmp target plus every `push bp; mov bp,sp` is not used (the drivers are register-based),
so code is seeded from the jmp table and from E8 call targets.
"""
import ida_auto, ida_bytes, ida_funcs, ida_segment, idc, idautils

NAMES = {0: "reset", 1: "select_page", 2: "set_color", 3: "plot_point", 4: "hline", 5: "vline", 6: "line",
         7: "get_pixel", 8: "copy_rect_pages", 9: "copy_page", 10: "page_op", 11: "save_rect",
         12: "restore_rect", 13: "scroll_rect_a", 14: "scroll_rect_b", 15: "load_res_a", 16: "load_image",
         17: "load_res_c", 18: "draw_op12", 19: "draw_image", 20: "draw_image_alt", 21: "put_char",
         22: "monster_setup", 23: "monster_draw"}

seg = ida_segment.getseg(0)
idc.set_segm_attr(0, idc.SEGATTR_BITNESS, 0)           # 16-bit
idc.set_segm_name(0, "drv")
end = idc.get_segm_end(0)
ida_bytes.del_items(0, ida_bytes.DELIT_SIMPLE, end)

# entry table
n = 0
targets = []
while True:
    ea = n * 3
    if idc.get_wide_byte(ea) != 0xE9:
        break
    idc.create_insn(ea)
    tgt = (ea + 3 + idc.get_wide_word(ea + 1)) & 0xFFFF
    targets.append(tgt)
    n += 1
print("jump table entries:", n)

first_code = min(targets) if targets else 0
for i, t in enumerate(targets):
    idc.add_func(t) if idc.create_insn(t) else None
    nm = "drv_fn%02X" % i
    if i in NAMES:
        nm += "_" + NAMES[i]
    idc.set_name(t, nm, idc.SN_NOCHECK | idc.SN_NOWARN)
    idc.set_name(i * 3, "entry_%02X" % i, idc.SN_NOCHECK | idc.SN_NOWARN)
ida_auto.auto_wait()

# call targets that IDA left as data
changed = True
rounds = 0
while changed and rounds < 5:
    changed = False
    rounds += 1
    ea = first_code
    while ea < end:
        if idc.get_wide_byte(ea) == 0xE8 and ida_bytes.is_code(ida_bytes.get_flags(ea)):
            t = (ea + 3 + idc.get_wide_word(ea + 1)) & 0xFFFF
            if first_code <= t < end and not ida_bytes.is_code(ida_bytes.get_flags(t)):
                if idc.create_insn(t):
                    idc.add_func(t)
                    changed = True
        ea = idc.next_head(ea, end) if ida_bytes.is_code(ida_bytes.get_flags(ea)) else ea + 1
    ida_auto.auto_wait()
print("code starts at", hex(first_code), "functions:", len(list(idautils.Functions())))
