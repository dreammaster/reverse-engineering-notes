"""Read-only report: segments, entry point, and how much of each segment holds bytes."""
import idc, idautils, ida_segment, ida_bytes, ida_nalt

print(f"input file : {idc.get_input_file_path()}")
print(f"entry      : cs={idc.get_inf_attr(idc.INF_START_CS):#x} ip={idc.get_inf_attr(idc.INF_START_IP):#x}")
for i in range(ida_segment.get_segm_qty()):
    s = ida_segment.getnseg(i)
    loaded = sum(1 for ea in range(s.start_ea, min(s.end_ea, s.start_ea + 0x10000), 0x10) if ida_bytes.is_loaded(ea))
    print(f"  {ida_segment.get_segm_name(s):8s} {s.start_ea:#08x}-{s.end_ea:#08x} sel={s.sel:#x} cls={ida_segment.get_segm_class(s)} loaded-paras={loaded}")
print("functions:", sum(1 for _ in idautils.Functions()))
