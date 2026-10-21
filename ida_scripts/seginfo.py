import idc, ida_segment, ida_bytes, idautils
for i in range(ida_segment.get_segm_qty()):
    seg = ida_segment.getnseg(i)
    code = data = unk = 0
    ea = seg.start_ea
    while ea < seg.end_ea:
        f = ida_bytes.get_flags(ea)
        sz = ida_bytes.get_item_size(ea)
        if ida_bytes.is_code(f): code += sz
        elif ida_bytes.is_data(f): data += sz
        else: unk += sz
        ea += max(sz, 1)
    print("%-10s %#x-%#x class=%s code=%d data=%d unk=%d" % (ida_segment.get_segm_name(seg), seg.start_ea, seg.end_ea, ida_segment.get_segm_class(seg), code, data, unk))
