"""
Parse the overlay call-stub table in ULTIMA.EXE.

Every function an overlay exports is reached through a 12-byte stub in the EXE:
    9A ec 02 xx xx     call far <overlay loader>
    NN 00              overlay number (1-based, order of the OVL name table at file 0x8113)
    EA lo hi 00 00     jmp far  <target>   (target offset = overlay offset + the overlay's load-area shift)
A call to any overlay function (from the EXE or from another overlay) therefore lands on a stub address.

Writes ida/out/stubs.json: {"order": [overlay names], "stubs": {stub_addr: [overlay, offset]}}
"""
import json
import os
import struct

import audit_functions as a

EXE = os.path.join(a.GAME_DIR, "ULTIMA.EXE")
ORDER = ["TOWN", "MAINOUT", "DUNGEON", "INTRO", "FLAMES", "NPC", "COMBAT", "BLCKTHRN", "LOOKOBJ", "DNGLOOK", "OUTSUBS",
         "SHOPPES", "ENDGAME", "SJOG", "CMDS", "CAST", "TALK", "CAST2", "ZSTATS", "COMSUBS", "SHOPPES2", "SHOPPES3", "FONT",
         "DATA"]

data = open(EXE, "rb").read()
hdr = struct.unpack_from("<H", data, 8)[0] * 16
img = data[hdr:]
bases = json.load(open(os.path.join(a.OUT, "ovl_bases.json")))
stubs = {}
for off in range(0x7000, 0x81d0):
    if img[off] == 0x9A and img[off + 1:off + 3] == b"\xec\x02" and img[off + 7 - 1 + 1 - 1] is not None:
        n = img[off + 5]
        if img[off + 7] == 0xEA and 1 <= n <= len(ORDER) and img[off + 6] == 0:
            tgt = struct.unpack_from("<H", img, off + 8)[0]
            m = ORDER[n - 1]
            if m in bases:
                stubs[off] = [m, (tgt - bases[m]) & 0xFFFF]
print(len(stubs), "stubs;", "range %04x-%04x" % (min(stubs), max(stubs)))
per = {}
for off, (m, o) in stubs.items():
    per.setdefault(m, []).append(o)
print({m: len(v) for m, v in per.items()})
json.dump({"order": ORDER, "stubs": stubs}, open(os.path.join(a.OUT, "stubs.json"), "w"))
