"""
IDAPython: dumps the room exit tables of Companions of Xanth to JSON (see sub_1EB3A, the movement routine behind the walk/enter verbs).

Room record (object type 1): +0Eh = number of exits (byte), +10h = 1-based index of its first exit (word).  Exit i (4 bytes) at
(segment word seg_8AA12)*16 + 0x10FA + 4*(i-1) = {u8 direction; u8 kind; u16 arg}.  Kind 1: arg = target room; kind 2: arg indexes the door table at
(segment word seg_8AA14)*16 + 0x84E + 4*arg = {u16 targetRoom; u16 doorObject} (the door must have attribute 0xE = open, otherwise "... isn't open."); kinds 3 and 4 call
off_34D6A[arg](0xF) (56 far pointers via sub_1FB15; the function returns the target room, <= 0 = blocked) - dumped in `exit_functions`.

    idat.exe -A -S"dump_exits.py out.json" copy.idb
"""
import json
import idc
import ida_kernwin

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "exits.json"
w, b = idc.get_wide_word, idc.get_wide_byte
ea = idc.get_name_ea_simple
count = w(ea("object_count"))
tab = w(ea("seg_8A9E4")) * 16 + 0x66
ex = w(ea("seg_8AA12")) * 16 + 0x10FA
door = w(ea("seg_8AA14")) * 16 + 0x84E
rooms = []
for i in range(1, count + 1):
    e = tab + i * 6
    if b(e) != 1:
        continue
    rec = w(e + 4) * 16 + w(e + 2)
    n, first = b(rec + 0xE), w(rec + 0x10)
    exits = []
    for j in range(n):
        a = ex + 4 * (first - 1 + j)
        d = dict(dir=b(a), kind=b(a + 1), arg=w(a + 2))
        if d["kind"] == 2:
            d["door_target"], d["door_obj"] = w(door + 4 * d["arg"]), w(door + 4 * d["arg"] + 2)
        exits.append(d)
    rooms.append(dict(room=i, first=first, exits=exits))
# kinds 3/4: sub_1FB15(arg, 0xF) calls off_34D6A[arg] (56 far pointers; each returns the target room, <= 0 = blocked / default message)
import idautils
import ida_funcs
fns = []
base = ea("off_34D6A") if ea("off_34D6A") != idc.BADADDR else ea("exit_function_table")
for k in range(0x38):
    refs = [r for r in idautils.DataRefsFrom(base + 4 * k) if ida_funcs.get_func(r)]
    t = refs[0] if refs else 0
    nm = idc.get_func_name(t) if t else ""
    stub = None
    if t:
        for r in idautils.CodeRefsFrom(idc.next_head(t), 0):
            if ida_funcs.get_func(r) and ida_funcs.get_func(r).start_ea == r:
                stub = r
    fns.append(dict(index=k, stub="%x" % t if t else "", name=nm, target="%x" % stub if stub else "", target_name=idc.get_func_name(stub) if stub else ""))
json.dump(dict(rooms=rooms, exit_functions=fns), open(out, "w"))
print("rooms", len(rooms))
if not ida_kernwin.is_idaq():
    idc.qexit(0)
