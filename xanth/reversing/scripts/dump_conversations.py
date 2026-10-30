"""
IDAPython: dumps Companions of Xanth's compiled-in conversation tables (Conversation_Show / Conversation_GetEntry) from the IDB to JSON.

Layout (read from Conversation_Show 0x4935x and Conversation_GetEntry; all in the exe's data, not in a data file; linear = segment*16 + offset):

  convId < Conversation_Count (0x40C): "simple" line.  record at seg069:dword_3B314[convId*4] = {u16 strIdx; u16 next}; enabled flag = bit 0 of
      Conversation_ENTRIES1[convId].  Shows strIdx in a speech bubble and continues with `next` (0xFFFF = end).
  convId >= 0x8000 (id & 0x7FFF < Conversation_MinusCount 0x190): menu.  record at Conversation_LOOKUPS[(id & 0x7FFF)*6] = {u8 count; u8 b1; u16 first;
      u16 callback}; option j (0..count-1) = {u16 strIdx; u16 next} at unk_3C858[(first+j)*4], enabled flag = bit 0 of Conversation_ENTRIES2[first+j];
      a per-menu flag lives in Conversation_ENTRIES3[id & 0x7FFF].  Choosing option j calls dword_3DCA4[callback](convId, first+j) when callback != 0,
      then continues with that option's `next`.
  strIdx < 0x8000: seg068[strIdx*4] = u32 SHANSTR message id 0xF0BBIIII (block BB, index IIII; XANTHSTR.DAT); strIdx >= 0x8000: far pointer to a function that returns the text (seg069[(strIdx-0x8000)*4]).

    idat.exe -A -S"dump_conversations.py out.json" copy.idb
"""
import json
import idc
import ida_segment
import ida_kernwin

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "conversations.json"
S68 = ida_segment.get_segm_by_name("seg068").start_ea
S69 = ida_segment.get_segm_by_name("seg069").start_ea
ea = idc.get_name_ea_simple
LOOK, E1, E2, E3 = ea("Conversation_LOOKUPS"), ea("Conversation_ENTRIES1"), ea("Conversation_ENTRIES2"), ea("Conversation_ENTRIES3")
ENT, SIMPLE, CB = ea("unk_3C858"), ea("dword_3B314"), ea("dword_3DCA4")
if CB == idc.BADADDR:
    CB = ea("conversation_callbacks")   # renamed by apply_xanth_handlers.py
NSIMPLE, NMENU = idc.get_wide_word(ea("Conversation_Count")), idc.get_wide_word(ea("Conversation_MinusCount"))
w, b = idc.get_wide_word, idc.get_wide_byte


def far(addr):
    return (w(addr + 2) << 4) + w(addr)


def text(idx):
    if idx < 0x8000:
        v = w(S68 + idx * 4)
        hi = w(S68 + idx * 4 + 2)          # 0xF0BB: SHANSTR message id, block BB, index v
        return {"s": idx, "msg": [hi & 0xFF, v], "tag": hi >> 8}
    t = far(S69 + (idx - 0x8000) * 4)
    return {"s": idx, "func": "%x" % t, "name": idc.get_func_name(t).replace("_stub", "")}


res = {"simple": [], "menus": []}
for i in range(NSIMPLE):
    r = SIMPLE + i * 4
    res["simple"].append({"id": i, "flag": b(E1 + i), "next": w(r + 2), **text(w(r))})
for i in range(NMENU):
    r = LOOK + i * 6
    count, b1, first, cb = b(r), b(r + 1), w(r + 2), w(r + 4)
    opts = []
    for j in range(count):
        o = ENT + (first + j) * 4
        opts.append({"flag": b(E2 + first + j), "next": w(o + 2), **text(w(o))})
    m = {"id": 0x8000 + i, "flag": b(E3 + i), "b1": b1, "first": first, "options": opts}
    if cb:
        t = far(CB + cb * 4)
        m["callback"] = {"index": cb, "func": "%x" % t, "name": idc.get_func_name(t).replace("_stub", "")}
    res["menus"].append(m)
json.dump(res, open(out, "w"))
print("conversations: simple %d, menus %d" % (NSIMPLE, NMENU))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
