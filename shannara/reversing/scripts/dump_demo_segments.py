"""
Dumps the code/data segments of the open (demo) IDB to seg_<start>.bin plus segs.json (segment list and the
addresses of the Q-codec symbols), for the unicorn-based Q decoder harness (q_emulate.py).

    idat.exe -A -S"reversing/scripts/dump_demo_segments.py" copy-of-Shannara-Demo.idb     (run in an empty directory)
"""
import idc, idautils, ida_bytes, ida_segment, ida_kernwin, json
segs=[]
for s in idautils.Segments():
    st=idc.get_segm_start(s); en=idc.get_segm_end(s)
    segs.append((idc.get_segm_name(s),st,en))
    data=ida_bytes.get_bytes(st,en-st) or b''
    # uninitialised parts come back as zeros / None
    open("seg_%X.bin"%st,"wb").write(data)
names={}
for n in ["_qloop1","_qloop2","_q_qmap_table","dword_18B2BC","dword_18B6BC","_Q_RAW_LABEL1","_Q_RAW_LABEL2","_Q_RAW_LABEL3","_Q_RAW_LABEL4","_Q_RAW_LABEL5","_Q_RAW_LABEL6","_Q_RAW_LABEL7","_qblock_addr_table","_video_buffer","off_18B17C","jpt_1652D3","jpt_1655A3","jpt_166323","jpt_1663E3","sub_1652E0","q_expand","init_addr_tbl","q_open","q_load","_qheader","_qinfo","_qbuffer","_qprefix","_next_qprefix","_composite_buffer","_composite_addr_tbl","dword_18BABC","dword_18B12C","_qaudio_buffer_size"]:
    names[n]=idc.get_name_ea_simple(n)
json.dump({"segs":segs,"names":names},open("segs.json","w"))
if not ida_kernwin.is_idaq(): idc.qexit(0)
