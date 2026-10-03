"""Apply mm3/names/mm3.tsv (see export_names.py) to the open database; safe to re-run."""
import os
import ida_bytes, ida_funcs, idc

path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "names", "mm3.tsv")
n = c = 0
for line in open(path, encoding="utf-8"):
    line = line.rstrip("\n")
    if not line.strip() or line.startswith("#"):
        continue
    p = line.split("\t")
    ea, name, cmt = int(p[0], 16), p[1] if len(p) > 1 else "-", p[2] if len(p) > 2 else ""
    thunk = None
    if name != "-" and idc.get_segm_name(ea).startswith("stub") and idc.print_insn_mnem(ea) == "jmp":
        # overlay thunk (jmp far sel:off): the name belongs to the real routine, the thunk becomes j_<name>
        thunk = ea
        ea = (idc.get_wide_word(ea + 3) << 4) + idc.get_wide_word(ea + 1)
    if name != "-":
        if ida_bytes.is_code(ida_bytes.get_flags(ea)) and not ida_funcs.get_func(ea):
            ida_funcs.add_func(ea)
        ok = idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN)
        if not ok and not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            # inside a larger data item: split it so the address becomes an item head
            ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)
            ok = idc.set_name(ea, name, idc.SN_NOCHECK | idc.SN_NOWARN)
        if ok:
            n += 1
        else:
            print("name failed:", hex(ea), name)
    if cmt:
        f = ida_funcs.get_func(ea)
        (idc.set_func_cmt(ea, cmt, 0) if f and f.start_ea == ea else idc.set_cmt(ea, cmt, 0))
        c += 1
# thunks in the stub segments get j_<name of the routine they jump to>
import idautils
t = 0
for seg in idautils.Segments():
    if not idc.get_segm_name(seg).startswith("stub"):
        continue
    for ea in idautils.Heads(seg, idc.get_segm_end(seg)):
        if idc.print_insn_mnem(ea) == "jmp" and idc.get_operand_type(ea, 0) == idc.o_far:
            tgt = (idc.get_wide_word(ea + 3) << 4) + idc.get_wide_word(ea + 1)
            nm = idc.get_name(tgt)
            if nm and ida_bytes.has_user_name(ida_bytes.get_flags(tgt)) and not nm.startswith("j_"):
                idc.set_name(ea, "j_" + nm, idc.SN_NOCHECK | idc.SN_NOWARN)
                t += 1
print("thunks named:", t)
print("applied %d names, %d comments" % (n, c))
