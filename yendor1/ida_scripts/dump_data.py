"""
Dump byte ranges of the open database's data segment (DS-relative), as requested in dump_request.json:
  {"seg": "seg021", "ranges": [{"name": "iconTable", "off": "0x10EF", "len": 128}, ...]}
Writes dump_<db>.txt: for each range, "name off len" then a hex line, then (when the bytes look like strings) the ASCIZ strings.
Read-only:   .\run_ida_script.ps1 <name> dump_data.py -NoExport
"""
import json
import os

import idc
import ida_segment

here = os.path.dirname(os.path.abspath(__file__))
stem = os.path.splitext(os.path.basename(idc.get_idb_path()))[0]
req = json.load(open(os.path.join(here, "dump_request.json")))
seg = ida_segment.get_segm_by_name(req["seg"])
base = (seg.start_ea >> 4) << 4
with open(os.path.join(here, f"dump_{stem}.txt"), "w") as fh:
    for r in req["ranges"]:
        off = int(r["off"], 0)
        n = int(r["len"])
        data = idc.get_bytes(base + off, n) or b""
        fh.write(f"{r['name']} off={off:#x} len={n}\n{data.hex()}\n")
print(stem, "dumped", len(req["ranges"]), "ranges")
