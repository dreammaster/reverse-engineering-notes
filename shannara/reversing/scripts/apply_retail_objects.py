"""
Names the retail object model helpers and the 640 object handlers / 79 default
verb handlers in `Shannara.idb`. Source: reversing/analysis/retail_game_layer.md
and the two TSV tables next to it (retail_object_handlers.tsv,
retail_verb_handlers.tsv).

Headless:  idat.exe -A -S"reversing\\scripts\\apply_retail_objects.py" Shannara.idb
Idempotent. Never overwrites an existing non-default name unless listed in FORCE.
"""
import os
import idc

HERE = os.path.dirname(os.path.abspath(__file__))
ANALYSIS = os.path.join(os.path.dirname(HERE), "analysis")
RESULTS = []

# (ea, name, comment) - renamed even if a previous working name exists
FORCE = [
    (0x1C0E8, "print_obj",
     "REAL NAME (debug string 'print_obj: not enough storage'). print_obj(flags_and_obj, actor): builds "
     "article/pronoun + object name (+ plural 'es'/'s') into a rotating 128-byte buffer (unk_CD69C, index "
     "dword_ADE34); flag 0x80 prints it via wprintf instead, 0x20 appends '.', sets object attr 6 afterwards. "
     "Upper 16 bits of arg_0 = object id, low 3 bits = form (the/a/he/your...). Name text comes from "
     "object_get_name?(obj)."),
]

# Mis-named by library matching; verified from the code.
FORCE += [
    (0x1A530, "object_get_prop_dword?",
     "WAS '_L1OutCode_' (false library match). (obj=ax) -> dword at +11h (type 2) / +14h (type 3) / +15h (type 5), "
     "0 for types 1,4 / invalid. Used as a boolean-ish object property test in handlers."),
    (0x5C714, "get_gr_op?",
     "WAS 'set_gr_op' (demo name copied wrongly): body is `xor eax,eax; mov al,_gr_op?; retn` - a getter."),
]

RENAMES = [
    (0x19FB4, "object_get_name?",
     "(obj=ax) -> char* into the runtime string pool unk_CD9D0 at (record[+0] - 1); 0 if none. The pool is "
     "filled from a game data file at runtime (not present in the exe image). Tries object_special?(obj,16) first."),
    (0x1A03C, "object_get_desc?",
     "(obj=ax) -> char* into the string pool at (record[+4] - 1); second text slot (description)."),
    (0x1A998, "object_get_handler_index",
     "(obj=ax) -> 1-based handler index (1..640) or 0. Record offset by type: 1:+0x10, 2:+0x25, 3:+0x2C, 4:+0x14, "
     "5:+0x2D. Verified over all 744 objects: indices span exactly 1..640 with 640 distinct values."),
    (0x1A254, "object_set_location?", "(obj, loc): writes the +0Ah (types 2,5) / +0Bh (type 3) word; inverse of object_get_location?."),
    (0x1A2A0, "object_get_slot?", "(obj) -> byte at +0Ch (types 2,5) / +0Dh (type 3): which child list of its parent it is in."),
    (0x1A2EC, "object_set_slot?", "(obj, slot): inverse of object_get_slot?."),
    (0x1A348, "object_get_list_head?",
     "(parent=ax, slot=dx) -> first child id of that child list (word array at +0Fh/+10h/+0Ah by type); 0 if slot >= "
     "per-type slot count (byte_20891[(type-1)*3])."),
    (0x1A400, "object_set_list_head?", "(parent=ax, slot=dx, child=bx): inverse of object_get_list_head?."),
    (0x1A49C, "object_get_next_sibling?", "(obj) -> word at +0Dh (types 2,5) / +0Eh (type 3): next object in the same child list."),
    (0x1A4E8, "object_set_next_sibling?", "(obj, next): inverse of object_get_next_sibling?."),
    (0x7BAA4, "get_event",
     "get_event(mask=eax, event*=edx): fills a 16-byte event {u32 type; u32 x; u32 y; u32 data}. Types: 1 key "
     "(data=key), 2 mouse release?, 4 mouse press (data=button), 8 mouse move; 0x10-0x1000 come from the popup/window "
     "event queue (mask 0x1FF0, ring buffer at _popup_window, 8 entries). Returns 1 if an event was stored."),
    (0x1173C, "dispatch_event?",
     "dispatch_event(event*=eax): optional per-window hook (funcs_11780[off_A7E84[window]]) first, then switch on "
     "event type: 1 -> sub_124DC, 2/8 -> sub_12AE4, 4 -> sub_12C78, 0x10/0x20/0x40 -> sub_11054, 0x80 -> sub_10B20, "
     "0x100 -> sub_132A0, 0x200 -> sub_1B9C8, 0x1000 -> sub_1FE8C. Called from the main_ loop right after get_event."),
    (0x11638, "wait_ticks?",
     "wait_ticks(ticks=eax, animate=dl): waits |ticks| clock ticks (0 = single poll), optionally running "
     "update_animation on window 1, returning early with the event code from sub_13A20 if input arrives. "
     "Callers: delay?/delay_animate? wrappers (~185 each)."),
    (0x11720, "delay?", "wait_ticks?(ticks, 0): wait without animating."),
    (0x1172C, "delay_animate?", "wait_ticks?(ticks, 1): wait while animating window 1."),
    (0x1B588, "object_parent_is",
     "(obj=ax, parent=dx) -> 1 if object_get_location?(obj) == parent. ~300 callers."),
    (0x1B5C8, "object_is_within",
     "(obj=ax, ancestor=dx) -> 1 if ancestor is anywhere up obj's location chain (walks object_get_location?)."),
    (0x78544, "set_offsets?", "set_offsets(x=eax, y=edx): stores _xoffset/_yoffset (picture draw offset)."),
    (0x11454, "send_event?", "(type=eax, data=edx): builds a local event record and calls dispatch_event? directly."),
    (0x1146C, "send_event_code?", "(type=eax): send_event?(type, 0)."),
    (0x1BCA0, "disable_pic_regions?",
     "(list=eax, count=edx): for each 12-byte entry {s16 obj; ..; s16 region@+4}, if sub_1BC20(entry)==0 disables the "
     "pic region (or the pic object when region==0)."),
    (0x25B84, "flagbyte_ptr?",
     "(group=ax, index=dx) -> byte* into one of several state-flag byte arrays (unk_C1FA8 table / unk_C1E8E / "
     "unk_BACC2), 0 if out of range; groups >= 0x8000 use a per-group count + offset table."),
    (0x25C24, "flagbyte_test1?", "(group, index) -> bit 1 of flagbyte_ptr?(group,index). ~66 callers."),
    (0x25C48, "flagbyte_set0?", "(group, index, value=bx): sets/clears bit 0 of flagbyte_ptr?(group,index). ~100 callers."),
    (0x1EE10, "go_to_room?",
     "go_to_room(room=ax): validates the id; sends phase 0x0F to the CURRENT room's handler (nonzero = refuse to "
     "leave); stores previous room (dword_CD928), sets the new current room (hi word of dword_ADE94), reparents the "
     "party object (word_ADE98, climbing to its top container via attr 0x11) into the new room with object_move?, "
     "updates lighting (update_room_light?), music (set_room_music?), then sends phase 0x0D (entered) to the new "
     "room's handler and redraws. ~85 callers."),
    (0x2211C, "update_room_light?",
     "(announce=eax): recomputes the room-light flag (word dword_CD91C+2) by scanning the party/room contents via "
     "sub_1B5F0; when announce!=0 prints 'It's light.' / 'It's dark.' on a change."),
    (0x1D324, "get_room_pic_id",
     "(room=ax) -> picture id (hi byte = pic file, lo byte = entry; 0x32 = none/blank). Table word_1CFE4 = 68 x "
     "{u16 room; u32 picId} (_room_pic_table) with hard-coded flag-dependent overrides after it."),
    (0x205BC, "set_room_music?",
     "(room=ax): finds the room in _room_music_table (68 x 6 bytes {u16 room; u32 trackRef}) and starts it via "
     "play_music_entry?."),
    (0x20390, "play_music_entry?",
     "(entry=eax, force=edx): starts/changes the music described by a music record (flags at +10h, fade_music, "
     "msleep(1000)); remembers current entry in dword_AE9F4."),
    (0x11478, "refresh_room?",
     "Redraws the main view: gets get_room_pic_id for the current room, load_pic into stru_CC6FC (purging memory and "
     "retrying on failure), sends phase 0x16 to the room handler (drawing hook; nonzero = it drew), then palette/"
     "display. 54 callers."),
    (0x1E260, "verb_get_name", "(verbId=eax) -> char* from _verb_vocab (13-byte records), '<NULL>' if unknown."),
    (0x1E2D4, "verb_get_flags", "(verbId=eax) -> first byte of the _verb_vocab record, default 1."),
    (0x1E288, "object_get_display_name?", "(obj=ax) -> description if present else name; '<NULL>' if invalid id."),
    (0x1B470, "object_move?",
     "(obj=ax, newParent=dx, slot=ebx): unlinks obj from its current parent's child list (walking the sibling chain), "
     "sets its location/slot, and pushes it onto the head of newParent's list `slot` (newParent==0 only unlinks)."),
]


# (address, name) - data labels; only applied over auto names
DATA = [
    (0x20918, "_object_table"),        # 744 x {u8 type; u32 recordPtr}
    (0x217A0, "_object_count"),
    (0x20891, "_object_slot_counts"),  # 3-byte stride per type; first byte = child-list slots
    (0x208A0, "_object_attr_desc"),    # 5-byte {u8 bitOffset; u32 typeMask} per attribute
    (0xB0958, "_object_records"),      # type sizes 36/44/48/24/52
    (0xCD9D0, "_text_pool"),           # runtime-loaded name/description strings
    (0xAFF58, "_object_handlers_a"),   # 512 code pointers (handler 1-512)
    (0xB0758, "_object_handlers_b"),   # 128 code pointers (handler 513-640)
    (0x1DAA8, "_verb_vocab"),          # 13-byte {u8 flags; u32 id; char *name; char *prep}
    (0x1DE98, "_verb_default_table"),  # 79 x {u32 verbId; u32 fn}
    (0x1CFE4, "_room_pic_table"),      # 68 x {u16 room; u32 picId}
    (0xAE9FC, "_room_music_table"),    # 68 x {u16 room; u32 trackRef}
    (0xADE94, "_current_room"),        # room object id in the HIGH word
]


def rename(ea, name, cmt, force, create=False):
    cur = idc.get_func_name(ea)
    if not cur and create:
        if idc.add_func(ea):
            RESULTS.append(f"MADE  function at {ea:#x}")
            cur = idc.get_func_name(ea)
    if not cur:
        RESULTS.append(f"SKIP  {ea:#x}: no function")
        return
    if force == "table":
        # entries of the verified handler tables always win over earlier names, except an objh_ name
        force = not cur.startswith("objh_")
    if cur == name:
        RESULTS.append(f"SAME  {ea:#x}: {name}")
    elif cur.startswith("sub_") or force:
        ok = idc.set_name(ea, name, idc.SN_NOWARN)
        RESULTS.append(f"{'OK  ' if ok else 'WARN'}  {cur} -> {name} ({ea:#x})")
    else:
        RESULTS.append(f"SKIP  {ea:#x}: already {cur!r}")
        return
    if cmt:
        idc.set_func_cmt(ea, cmt, 0)


def read_tsv(name):
    with open(os.path.join(ANALYSIS, name), encoding="utf-8") as f:
        next(f)
        for line in f:
            yield line.rstrip("\n").split("\t")


def main():
    for ea, name, cmt in FORCE:
        rename(ea, name, cmt, True)
    for ea, name, cmt in RENAMES:
        rename(ea, name, cmt, False)

    for ea, name in DATA:
        cur = idc.get_name(ea)
        if cur == name or not cur or cur.startswith(("byte_", "word_", "dword_", "unk_", "off_", "stru_", "loc_")):
            idc.set_name(ea, name, idc.SN_NOWARN)
        else:
            RESULTS.append(f"SKIP  data {ea:#x}: already {cur!r}")

    seen = set()
    for h, addr, size, objs, strs in read_tsv("retail_object_handlers.tsv"):
        ea = int(addr, 16)
        if ea in seen:
            RESULTS.append(f"DUP   handler {h} shares {ea:#x}")
            continue
        seen.add(ea)
        cmt = (f"Object handler #{h} (called by call_object_handler? with phase 1-6; returns 1 if it handled "
               f"the command). Objects (id:type): {objs}. Strings: {strs}")
        rename(ea, "objh_%03d" % int(h), cmt, "table", True)

    seen_verb = set()
    for verb, addr, vname, prep, flags in read_tsv("retail_verb_handlers.tsv"):
        ea = int(addr, 16)
        if ea in seen_verb:
            RESULTS.append(f"DUP   verb {verb} shares {ea:#x}")
            continue
        seen_verb.add(ea)
        label = "_" + vname.replace(" ", "_") if vname else ""
        full = (vname + (" " + prep if prep else "")) or "(no menu entry)"
        rename(ea, "verbdef_%02d%s" % (int(verb), label),
               f"Default response for verb {verb} '{full}' (flags byte {flags or '-'}); table dword_1DE98 via "
               f"run_default_verb_handler?. Verb vocabulary table: 13-byte records at 0x1DAA8 "
               f"{{u8 flags; u32 verbId; char* name; char* preposition}}.", "table", True)

    ok = sum(r.startswith("OK") for r in RESULTS)
    skip = sum(r.startswith("SKIP") for r in RESULTS)
    warn = sum(r.startswith("WARN") for r in RESULTS)
    print("\n".join(r for r in RESULTS if not r.startswith("OK")))
    print(f"{ok} renamed, {skip} skipped, {warn} warnings")
    idc.save_database(idc.get_idb_path(), 0)


main()

import ida_kernwin
if not ida_kernwin.is_idaq():
    idc.qexit(0)
