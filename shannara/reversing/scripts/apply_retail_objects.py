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
     "print_obj(flags, obj[, verb]) cdecl: flags low 3 bits = form (the/a/he/your...), 0x10 capital, 0x20 period, 0x40 conjugated verb, 0x80 print now. Name text comes from "
     "object_get_name?(obj)."),
]

# Mis-named by library matching; verified from the code.
FORCE += [
    (0x14010, "combat_loop?",
     "WAS 'capture_keyboard?' (a demo-name guess that does not fit). Sets game flag 8, stores the caller's hook in "
     "_old_keyboard_vector, sends events 0x10/0x40, iterates the 23-slot combatant table dword_CCA44 (sub_14C50 / sub_14C94) "
     "- the combat UI/turn loop. 576 bytes, 18 callees."),
    (0x14B34, "combat_hook_notify?",
     "WAS 'release_keyboard'. Calls the combat hook (_old_keyboard_vector) with code 7; on failure clears word +40h of the "
     "combatant and re-adds it via sub_1535C."),
    (0x1A530, "object_get_prop_dword?",
     "WAS '_L1OutCode_' (false library match). (obj=ax) -> dword at +11h (type 2) / +14h (type 3) / +15h (type 5), "
     "0 for types 1,4 / invalid. Used as a boolean-ish object property test in handlers."),
    (0x5C714, "get_gr_op?",
     "WAS 'set_gr_op' (demo name copied wrongly): body is `xor eax,eax; mov al,_gr_op?; retn` - a getter."),
]

# Mis-named by library matching (false demo match): must override the existing name
FORCE += [
    (0x15B08, "combat_stats_set_a?", "WAS 'reset_bevel_style?' (false demo match). Rewrites party template 0 (0xABB7C): +4 ptr unk_A8948, +8=0x519, +0Ah=0x46, +0Ch=0x2D, +0Eh=0x1E. Chosen by start_encounter? when object 0x2F is inside object 3; else combat_stats_set_b?."),
]


# Combat event hooks (renamed from enc_ai_NN? once the hook protocol was understood, see retail_combat.md)
FORCE += [
    (0x15CF8, "enc_hook_00?", "Scripted combat event hook for encounter 0 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x15D08, "enc_hook_01?", "Scripted combat event hook for encounter 1 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x15DF8, "enc_hook_02?", "Scripted combat event hook for encounter 2 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x15F40, "enc_hook_03?", "Scripted combat event hook for encounter 3 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16208, "enc_hook_04?", "Scripted combat event hook for encounter 4 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x165C4, "enc_hook_05?", "Scripted combat event hook for encounter 5 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16728, "enc_hook_06?", "Scripted combat event hook for encounter 6 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16964, "enc_hook_07?", "Scripted combat event hook for encounter 7 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x160E8, "enc_hook_08?", "Scripted combat event hook for encounter 8 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x169C4, "enc_hook_09?", "Scripted combat event hook for encounter 9 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16A8C, "enc_hook_10?", "Scripted combat event hook for encounter 10 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16AC4, "enc_hook_11?", "Scripted combat event hook for encounter 11 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x16BBC, "enc_hook_12?", "Scripted combat event hook for encounter 12 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x17064, "enc_hook_13?", "Scripted combat event hook for encounter 13 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
    (0x170E4, "enc_hook_14?", "Scripted combat event hook for encounter 14 (start_encounter? passes it to combat_loop?, stored in _old_keyboard_vector). Called as hook(combatant, code); returns -1 for the default behaviour, anything else overrides. Codes: 0 with combatant NULL = fight start (edx=0) / round tick (edx=round no.), 1 turn start, 2 choose target, 3 roll damage (return value = damage), 4 combatant damaged, 5 attack starts, 6 defend, 7 idle/pick action, 8 flee attempt, 10 animation finished. Data xref only."),
]

# Default verb property (eighteenth pass: build_command_line? uses property B as the object's default verb)
FORCE += [
    (0x1A7CC, "object_get_default_verb?", "(obj=ax) -> the object's default verb id (was object_get_prop_b?): type2 +1Dh, type3 +24h, type4 +0Ch, type5 +25h; 0 for type 1. build_command_line? uses it to pick the verb for a click on the object."),
    (0x1A834, "object_set_default_verb?", "(obj=ax, verb=edx): setter for the default verb (was object_set_prop_b?)."),
]

# Timed events / turn clock (nineteenth pass)
FORCE += [
    (0x19C40, "schedule_event?", "(eventId=al, delay=dx): schedule timed event eventId (1-8, see fire_timed_event?) delay turns from now in the 16-entry list at byte_CCBB8 {u8 id; i16 countdown} (count dword_ADDEC, part of the savegame); an id already queued is left alone; prints an overflow message when full. Was state_pair_set?."),
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
    (0x1E260, "verb_get_name", "(verbId=eax) -> char* from _verb_vocab (13-byte records {char *name; char *prep; u8 flags; u32 id} from 0x1DAA0), '<NULL>' if unknown."),
    (0x1E2D4, "verb_get_flags", "(verbId=eax) -> flags byte (+8) of the _verb_vocab record, default 1: 0 = no object (Look, Wait), 1 = one object, 2 = two objects (verb + preposition)."),
    (0x1E288, "object_get_display_name?", "(obj=ax) -> description if present else name; '<NULL>' if invalid id."),
    (0x2405C, "save_io_block?",
     "(ptr=eax, size=edx, handle=ebx, op=ecx): the single I/O primitive of the save system. op 2 = fp_write, "
     "10 (0Ah) = fp_read, 3 = memmove INTO a memory snapshot (undo), 11 (0Bh) = memmove OUT of it; the memory "
     "handle holds a cursor. A short file transfer longjmps to unk_CF484 (error)."),
    (0x240D4, "serialize_state?",
     "(op=eax, handle=edx) -> 0 ok / -1 error ('Write error: ...'/'Read error: ...'). Streams, in order: for op 2/0Ah a "
     "0x20-byte header (byte_CF464, save title); every object's record, the first byte_20892[(type-1)*3] bytes (33/41/48/24/49 "
     "for types 1-5); the 92 registered regions at _save_regions (dword_23EC0 count); 4 bytes at dword_ADDEC; 0x30 bytes "
     "at byte_CCBB8."),
    (0x24210, "save_game?",
     "(mode=eax): mode 2 = write JOURNAL.nnn via sprintf('JOURNAL.%03d') in the SAVEDATA dir; 1/4 other targets; calls "
     "serialize_state?."),
    (0x243F4, "load_game?",
     "(mode=eax): 2 = restore a JOURNAL.nnn ('There are no saved games to restore.' if none), 3 = undo from the in-memory "
     "snapshot (dword_C2738), 4 = HELP.DAT, 5 = RESTART.DAT; on success sends PHASE 0x18 to the current room, clears "
     "flag 7 in unk_CD8AC and longjmps to restart_jmp with the mode (main_ then prints '[Undone.]' for 3)."),
    (0x21B60, "load_object_dat?",
     "Opens OBJECT.DAT ('Can't open file OBJECT.DAT' -> game_abort): reads u16 poolSize then poolSize bytes into _text_pool "
     "(NUL-terminated object names/descriptions, referenced by 1-based offsets from the object records)."),
    (0x1AA74, "init_string_file?",
     "Opens SHANSTR.DAT and loads the string-table index: u16 nBlocks; {u16 count; u32 dataBytes}[nBlocks]; per-string "
     "compressed lengths; a Huffman tree (u16 N1 + u16 tree[N1]); a phrase table (u16 K<=128 offsets + u16 poolSize + "
     "pool); then allocates the string cache (0xC00 bytes, shrinking by 0x100 down to 0x400)."),
    (0x1AA18, "read_array?", "(dst=eax, elemSize=edx, count=ebx, fp=ecx): fp_read(count*size); prints '[Disk read error]' on a short read; returns elements read."),
    (0x1AEF0, "decode_string?",
     "(dst=eax, compressedLen=edx, fp=ebx, tree=ecx, K, phrasePtrs, ...): decodes one SHANSTR.DAT string. LSB-first bits, "
     "Huffman tree stored as u16 pairs (negative = leaf, symbol = -v-1); symbols >= 0x80 expand phrase table entry "
     "sym-0x80, others are literal bytes; symbol 0 ends the string."),
    (0x28858, "play_cutscene?",
     "6 KB switch over cutscene ids: plays Autodesk .FLC animations (7700.FLC, 8530.flc ... 9595.flc, OPEN1-5.FLC, SHR_*.FLC) "
     "with fade_in/out_palette, load_music/start_music, make_sound."),
    (0x1E304, "build_command_line?", "Builds/validates the pending command and its on-screen text ('Look at Jak' ...): picks a default verb from the object (object_get_default_verb?, attribute tests), consults the 12-byte override table at dword_ADED4 {verb, obj, obj}, verb_get_flags (0 = no object, 1 = dobj, 2 = dobj+iobj), stores verb/dobj/iobj in dword_CD918 / CD924+2 / CD92C+2, sends PHASE 0x1D to the dobj handler and 0x1E to the iobj handler (return 2 cancels, other non-zero = handled), then lays out the text with get_string_width/set_font_outline."),
    (0x24D34, "save_restore_dialog?", "'Save Game' / 'Restore Game' list dialog: Ok/Cancel/Delete, 'Overwrite the saved game?' / 'Delete the saved game?'."),
    (0x124DC, "handle_key_event?", "dispatch_event? case 1: keyboard handler incl. debug keys ('Version: %s', 'Free memory: %s', 'Animation %sabled')."),
    (0x12C78, "handle_click_event?", "dispatch_event? case 4: mouse press -> region hit-testing / buttons."),
    (0x12AE4, "handle_move_event?", "dispatch_event? cases 2/8: mouse release/move -> region hover."),
    (0x22538, "options_dialog?", "Music / Sound / Voice volume+toggle dialog with Cancel."),
    (0x1CB88, "describe_contents?", "Prints 'You see ... here', 'holding', 'wearing' lists for an object's children."),
    (0x1FDBC, "combat_help_menu?", "'Combat Help' menu: How do I give orders? / pick an opponent? / Keyboard shortcuts / What am I supposed to do?"),
    (0x11054, "popup_window_event?", "dispatch_event? cases 0x10/0x20/0x40: builds a bevelled popup window (make_xwindow, draw_bevel, set_font)."),
    (0x17310, "start_encounter?", "2.4 KB combat setup: 26-case switch on encounter id, clones enemy templates (clone_combatant?) and adds them (add_combatants?); prints 'You have fallen in battle...' on defeat."),
    (0x15758, "clone_combatant?", "Allocates and copies a 0x3F8-byte combatant template, setting word +4Ah = 1."),
    (0x13CDC, "add_combatants?", "(slot, c1, c2, ... 0): inserts combatant blocks into the battle table at dword_CCA44 (23 slots)."),
    (0x667E0, "speak?",
     "(speaker/dialog id=eax, kind=dx, textId=ebx): kinds 0x400-0xC02 select a party character via party_has_member?; if present "
     "show_dialog?(id, kind) (voiced/graphical line), else falls back to printing load_string_(textId). 93 callers (NPC speech)."),
    (0x19548, "party_has_member?", "(n=ax): n==3 always 1; else scans the 24 x 8-byte table dword_ADB84/ADB88 for key n and returns object_parent_is(entry object, 2)."),
    (0x19594, "party_add_member?", "(key): finds the key in the party table and object_move?s its object into object 2, slot 1; refreshes the party UI."),
    (0x195FC, "party_add_member_slot?", "(key, slot): like party_add_member? with an explicit slot; also fixes the selection and posts event 0x10."),
    (0x1F44C, "add_score?", "(points=eax, spaceBefore=edx, newlineAfter=ebx): dword_ADEAC (_score) += points; prints '[%d point%s]'."),
    (0x1F494, "move_to_current_room?",
     "(obj=ax): unless sub_1B3F0(obj, room)==2, object_move?(obj, current room, 0), then sets attrs 5, 10, 6 and clears 7; "
     "returns 1. 25 handlers."),
    (0x1F534, "move_to_current_room_as?", "(obj, who): temporarily sets word_ADE98 (acting character) = who around move_to_current_room?."),
    (0x2200C, "value_in_list?", "cdecl varargs: value_in_list?(v, a, b, ... , -100): 1 if v is one of the listed values (terminator 0xFFFFFF9C)."),
    (0x7529C, "journal_add_entry?",
     "(id=ax): adds one journal/notebook entry (sub_751C0), re-renders the page (journal_render_page?) and refreshes the "
     "table pointer. 65 handlers call it."),
    (0x752F0, "journal_add_entries?", "cdecl varargs: journal_add_entries?(id1, id2, ..., 0): marks each entry then re-renders."),
    (0x751C0, "journal_mark_entry?", "(id=ax): finds the entry (journal_find_entry?) in off_C6688 (22-byte records) and sets its 'seen' bit (+12h bit 0), fixing its list state."),
    (0x75664, "journal_find_entry?", "(key=ax) -> 1-based index of the 22-byte journal record whose key (word +2) matches, 0 if none."),
    (0x74670, "journal_render_page?", "(entry, ...): formats an entry via load_string_(record+4) and the format at off_C6690, draws its picture (load_pic record+6) and text in the journal window."),
    (0x79420, "load_pic_sequence?", "(firstPicId=eax, count=edx, ...): loads `count` consecutive pictures into a handle-backed sequence slot (_next_sequence < 5, 0x18 bytes per Pic); returns slot or -1."),
    (0x1BCE0, "setup_pic_regions?", "(list=eax, count=edx): counterpart of disable_pic_regions?: loads each 12-byte entry's picture (load_pic_in) and enables its hotspot; skipped when byte_ADE24 (UI lock) is set. 48 handlers."),
    (0x1EDB0, "describe_room?", "(room=ax, phase=dx): refreshes the view if the room picture changed (event 0x20) then sends `phase` (9 first visit, 0Ah revisit, 0Bh/0Ch verbosity modes) to the room handler."),
    (0x1A5C8, "object_get_points?", "(obj=ax) -> score value stored in the object (dword +0Ch type 1, +15h type 2, ...); awarded once via add_score?."),
    (0x1A618, "object_set_points?", "(obj, value): inverse of object_get_points? (go_to_room? zeroes it after awarding)."),
    (0x1BC20, "overlay_visible?",
     "(entry=eax) -> 1 if the 12-byte overlay entry {s16 obj; s16 container; s16 picId; s16 attr; void *cond} should be "
     "drawn: cond(obj) != 0 if present; obj in `container` (object_parent_is); obj has `attr`; plain entries defer to "
     "sub_21ED0(obj)."),
    (0x1B470, "object_move?",
     "(obj=ax, newParent=dx, slot=ebx): unlinks obj from its current parent's child list (walking the sibling chain), "
     "sets its location/slot, and pushes it onto the head of newParent's list `slot` (newParent==0 only unlinks)."),
]

# Combat engine (tenth pass, see analysis/retail_combat.md)
RENAMES += [
    (0x132EC, "choice_dialog?",
     "Generic modal button dialog: (promptStringId=eax, option labels=edx...) -> 1-based choice. Used for 'Quit?', 'Restore?', 'Help', 'Combat Help', 'Give orders to...', 'Overwrite/Delete the saved game?', 'You have fallen in battle...'. 15 callers."),
    (0x17310, "start_encounter?",
     "start_encounter?(encounterId 0-25): snapshots the 0x15E-byte character-stat block _char_stats (dword_ADC4C), builds the combatant list with clone_combatant?/add_combatants? (party templates 0xABB7C.., enemy templates 0xA8FD4.. in _combatant_templates, stride 0x3F8), passes the per-encounter AI hook (enc_hook_NN?) to combat_loop?; on result -1 shows 'You have fallen in battle' (Try again / Restore). Returns the combat_loop? result (1 = victory). See analysis/retail_combat.md."),
    (0x13CDC, "add_combatants?",
     "add_combatants?(initNow, c1, c2, ..., 0): puts each combatant block into the first free slot of _combatants (dword_CCA44, 23 slots). initNow != 0 initialises it immediately (combat_init_combatant?, reinforcements); 0 defers it (+4Ch=0) until combat_loop? runs combat_init_all?."),
    (0x15758, "clone_combatant?",
     "clone_combatant?(template=eax) -> heap copy of a 0x3F8-byte combatant template with word +4Ah = 1."),
    (0x14C50, "combat_pick_ready?", "(table=eax, now=edx) -> first combatant in slot order with HP(+3Ah)!=0 and now >= +46h (next-action time); reschedules it to now + (word at +2 = action period). Returns 0 if none."),
    (0x14C94, "combat_init_all?", "(table=eax): runs combat_init_combatant? on every slot of the combatant table."),
    (0x14CBC, "combatant_id_is_party?", "(combatant=eax) -> word +0 < 100 (ids 0-99 = party members indexing _char_stats, >= 100 = monsters)."),
    (0x14CC8, "combat_init_combatant?",
     "Initialises one combatant: clears +44h/+46h/+4Eh, +18h=1; party member (id<100): +38h=0, copies on-screen rects from unk_A7FF8[slot*8] into +1Eh/+26h, HP +3Ah/max +3Ch from _char_stats[id*14+2/+4]; monster: +38h=1, HP 100 default, rects from unk_A7F20; calls combat_hook_notify?, sets +4Ch=1."),
    (0x14BD0, "combat_check_outcome?", "(table=eax) -> 0 = both sides still have living (+3Ah!=0) combatants, -1 = only monsters (+38h!=0) survive (defeat), 1 = no monsters alive (victory)."),
    (0x15B4C, "combat_stats_set_b?", "Alternative to combat_stats_set_a?: template 0xABB7C <- ptr unk_A88AC, +8=0x518, +0Ah=0x32, +0Ch=0x28, +0Eh=0x14."),
    (0x15B90, "combat_stats_set_c?", "Rewrites party template 5 (0xACF54): ptr unk_A8C54, +8=0x51E, +0Ah=0x46, +0Ch=+0Eh=0x3C. Chosen when object 0x39 is inside object 5."),
    (0x15BD0, "combat_stats_set_d?", "Alternative to combat_stats_set_c?: ptr unk_A8BB8, +8=0x51D, +0Ah=0x46, +0Ch=0x3C, +0Eh=0x23."),
]



# (address, name) - data labels; only applied over auto names
RENAMES += [
    (0x19DBC, "advance_time?", "(turns=eax, askContinue=edx): the game clock. For each turn: sends PHASE 0x1B to the current room (turn notification), then walks the timed-event list at byte_CCBB8: positive countdowns are decremented, a countdown of 0 fires fire_timed_event?(id) and removes the entry (a rescheduling event keeps it). Long waits ask 'Do you want to continue...' (sub_254DC). Called by the Wait command ('%s a while.') and by handlers."),
    (0x1F700, "fire_timed_event?", "(id=eax, ...): dispatches timed event 1-8 through funcs_1F709: 2 sub_62A70, 3 sub_4D5F8, 4 sub_4D678 (damages a character with voice lines), 5 sub_61750, 6 sub_5BBBC, 7 sub_5BC10, 8 sub_5BC50; each prints its message and usually reschedules itself with schedule_event?."),
    (0x1F580, "game_over_screen?", "Used by the lethal timed events: background fade, the death/ending screen with restore options (load_game?)."),
]

# Help menu / tutorial (twenty-eighth pass)
RENAMES += [
    (0x1FE8C, "help_menu?", "Event 0x1000 (F1 help): choice_dialog? 'Help' with 5 entries - What is an Adventure game?, On-line tutorial, Keyboard shortcuts, Helpful Hints, OK. Entry 2 starts the tutorial (tutorial_start?('shan000.hLP', 1)); the others page through help texts (SHANSTR block 17, ids 0xF011000D-0xF0110017) with show_help_page?."),
    (0x1F7BC, "tutorial_start?", "(scriptName=eax, mode=edx): mode 1 saves the player's game (save_game?(4) = HELP.DAT), opens the replay script ('r'), loads the RESTART snapshot (load_game?(5)) and arms the replay (dword_AE858). SHAN000.HLP is the recorded-input script, see analysis/retail_game_layer.md section 28."),
    (0x1F984, "show_help_page?", "(messageId=eax): modal window with the help text, waits for a click/key (get_clock, key_hit, mouse_wait)."),
]

DATA = [
    (0x20918, "_object_table"),        # 744 x {u8 type; u32 recordPtr}
    (0x217A0, "_object_count"),
    (0x20891, "_object_slot_counts"),  # 3-byte stride per type; first byte = child-list slots
    (0x208A0, "_object_attr_desc"),    # 5-byte {u8 bitOffset; u32 typeMask} per attribute
    (0xB0958, "_object_records"),      # type sizes 36/44/48/24/52
    (0xCD9D0, "_text_pool"),           # runtime-loaded name/description strings
    (0xAFF58, "_object_handlers_a"),   # 512 code pointers (handler 1-512)
    (0xB0758, "_object_handlers_b"),   # 128 code pointers (handler 513-640)
    (0x1DAA0, "_verb_vocab"),          # 13-byte {char *name; char *prep; u8 flags; u32 id}
    (0x1DE98, "_verb_default_table"),  # 79 x {u32 verbId; u32 fn}
    (0x1CFE4, "_room_pic_table"),      # 68 x {u16 room; u32 picId}
    (0xAE9FC, "_room_music_table"),    # 68 x {u16 room; u32 trackRef}
    (0x23BE0, "_save_regions"),        # 92 x {void *ptr; u32 size} - extra state saved by serialize_state?
    (0x23EC0, "_save_region_count"),
    (0xCD8AC, "_game_flags"),          # 100-byte bitset (800 flags), bitset_test/set/clear
    (0xADEAC, "_score"),               # dword; add_score? adds, prints '[%d point%s]'
    (0xC6688, "_journal_table"),       # pointer to 22-byte journal records
    (0xADE94, "_current_room"),        # room object id in the HIGH word
]

RENAMES += [
    (0x14808, "combat_choose_target?", "(c=eax): hook code 2; default: monsters pick by strategy word +18h (random 1/10/7: 1=nearest living opponent, 2/4=first living opponent, 7=weakest (lowest HP), 8=nearest by rect-centre distance via sub_147B0, 9=the hero's current target dword_ABBCA, 10=strongest); stores the chosen opponent in +4Eh."),
    (0x15200, "combat_roll_damage?", "(attacker=eax) -> damage (0 = miss). hook code 3 can override. Default: hit roll = random-percent(attacker hi word of +8 = +0Ah; 100 = always hits), then target dodge roll = random-percent(target +0Ch, doubled if target is defending +3Eh); damage = attacker word +0Eh + rand()%5. sub_254B8(pct) is the random-percent test."),
    (0x15290, "combat_apply_damage?", "(c=eax, dmg=edx): HP(+3Ah) -= dmg (min 0), redraw HP bar, hook code 4; default: HP 0 -> state +40h=6 (death anim), else state 5 (hit anim)."),
    (0x15300, "combat_step_anim?", "(c=eax) -> flags: bit0 = the animation reached its impact frame (anim+6) this step, bit2 = animation finished (hook code 10 sent); redraws/advances the frame via sub_154A8 / sub_1560C."),
    (0x1535C, "combat_set_anim?", "(c=eax, anim=edx): installs animation record edx as current (+1Ah); handles types 2 (pic sequence), 4 (FLC via +52h state), 8 (Q video, flag bits in +3F6h); plays the animation sound (anim+0Ah)."),
    (0x15448, "combat_stop_anim?", "(c=eax): releases the current animation (FLC player / Q stream) and clears +1Ah."),
    (0x154A8, "combat_draw_anim_frame?", "(c=eax): draws the current frame of the combatant animation (pic sequence / FLC / Q)."),
    (0x1560C, "combat_next_anim_frame?", "(c=eax): advances the current animation frame counter (anim+10h), looping if flag 0x80."),
    (0x15710, "anim_is_finished?", "(c=eax) -> 1 when the combatant has no animation or the current one has played its last frame."),
    (0x15120, "combat_draw_marker?", "(c=eax, n=edx): draws the portrait/selection marker picture 0x515+n (party; n==3 -> 0 wrapped) or [c+8]+n (monsters) at the combatant rectangle through load_display."),
    (0x15178, "combat_refresh_combatant?", "(c=eax): marker 0 if alive else 3, then the HP bar."),
    (0x14ED0, "combat_draw_hp_bar?", "(c=eax): draws the health bar (outline rects via gxSaveColors/gxDirtyDisplay, fill width = percentage, colour from word_A817C); monsters and party use different rectangles."),
    (0x143B4, "combat_begin_turn?", "(c=eax): clears +44h; combatant id 0 (the hero) -> combat_player_input?; others -> hook code 1 then default state 1 (attack) or 4 when strategy +18h == 3."),
    (0x14438, "combat_player_input?", "(c=eax) -> 1 continue / 0 fled: the hero's command UI. Keys a=attack(state 1), d=defend(4), o=orders menu (sub_158AC), r=run (hook code 8); screen buttons 0x501 attack, 0x502 defend, 0x503 orders, 0x504 run; clicking a monster rect selects the target (+4Eh, hook code 2)."),
    (0x14774, "combat_do_defend?", "(c=eax): clears target, hook code 6, state +40h=4, plays animation +58h (index 4)."),
    (0x14B68, "combat_do_attack?", "(c=eax): marker, hook code 5, state +40h=1, plays attack animation 1 + rand()%3 (22-byte animation records at [c+4]: 0 idle, 1-3 attack, 4 defend, 5 hit, 6 death)."),
]

# Video playback and object property accessors (twelfth pass, see analysis/retail_game_layer.md section 13)
RENAMES += [
    (0x22D50, "video_open?", "(vid=eax 0x6D0-byte player state, name=edx, rect=ebx, flags=ecx): tries NAME.Q (Q codec, state type 0x2000 at +0Ch) then NAME.FLC (type 0x1000) via _splitpath_/_makepath_; flag 0x20 saves the background (save_display into vid+10h), flag 1 saves the 0x300-byte palette (vid+28h); sets frame delay (Q: (1000+fps/2)/fps ms, FLC: header speed)."),
    (0x2308C, "video_play?", "(vid=eax, frames=edx (-1 = all), flags=ebx): plays frames with get_clock pacing (0x40 = video_sync, 0x10/0x02 = key-abort behaviour); optional per-frame callback at vid+6CCh. Returns 0 finished, 1 callback stopped, 2 error/bad range, 3 aborted by the player (sub_13A20 input test)."),
    (0x23254, "video_close?", "(vid=eax): closes the FLC/Q stream, restores the saved background and palette (fade_in_palette)."),
    (0x23560, "play_video?", "(name=eax, flags=edx, delayAfter=ebx): flush_events?, video_open?, set up the palette/window, video_play?, optional delay?, video_close?. 24 callers (cutscenes called from handlers)."),
    (0x27FC0, "play_video_wait?", "(name=eax, flags=edx, delay=ebx) -> 1 if the player aborted: _hide_mouse, video_open?, video_play?, delay?, video_close?, restore mouse. 49 callers."),
    (0x28044, "play_video_voiced?", "Like play_video_wait? but with make_voice?/make_sound narration around three video_open?/video_play? segments."),
    (0x1F6E4, "flush_events?", "Drains the event queue (get_event(7,...) until empty)."),
    (0x19BC0, "object_carrier?", "(obj=ax) -> id of the character-table entry (24 x 8-byte records at dword_ADB84, id word at +4) whose id equals the object's location, provided it is the first record or is inside object 2; 0 if none. 72 callers (used to say who carries something)."),
    (0x1A6FC, "object_get_prop_a?", "(obj=ax) -> dword property A: type1 +14h, type2 +19h, type3 +20h, type5 +21h; 0 for type 4."),
    (0x1A768, "object_set_prop_a?", "(obj=ax, value=edx): setter for property A (see object_get_prop_a?)."),
    (0x1A7CC, "object_get_prop_b?", "(obj=ax) -> dword property B: type2 +1Dh, type3 +24h, type4 +0Ch, type5 +25h; 0 for type 1."),
    (0x1A834, "object_set_prop_b?", "(obj=ax, value=edx): setter for property B."),
    (0x1A580, "object_set_prop_dword?", "(obj=ax, value=edx): setter paired with object_get_prop_dword?: type2 +11h, type3 +14h, type5 +15h."),
    (0x1A89C, "object_get_list_slot?", "(obj=ax) -> address of the object's list head in the pool at unk_AF370 (index property: type1 +1Dh, type2 +21h, type3 +28h, type4 +10h, type5 +29h; slot = AF370 + (index-1)*4), or unk_ADDF4 when the index is 0."),
    (0x255B4, "object_list_append?", "(obj=ax, value=edx) -> 1 if appended: finds the end of the object's 0-terminated dword list (object_get_list_slot?) and stores value there if the next slot is free."),
]

# Cutscene script and party stat helpers (thirteenth pass, retail_game_layer.md section 14)
RENAMES += [
    (0x28858, "play_cutscene?", "play_cutscene?(id 0-11): the story-cutscene script - sends phase 0x18 to the current room, picks music from dword_C4614[id] (play_music_entry?), then a 12-case switch (jpt_289D6) that plays clips with play_video_wait?(name, flags) / play_video_voiced?(voiceMsgId, name) (names are 'NNNN.flc' strings; video_open? really tries NNNN.Q first), show_dialog?, make_voice?, party healing/damage and, in case 7, start_encounter?(14). Case 11 is the full-screen (640x480) narrated slideshow. See retail_game_layer.md section 14."),
    (0x196B8, "party_heal_percent?", "(percent=eax): for each of the 24 character-table slots (slot 0 always, others only if inside object 2 = party; character 7 skipped while flag 0x0D is set) HP = min(maxHP, HP + ceil(maxHP*percent/100)). _char_stats record: +2 HP, +4 max HP (14-byte stride)."),
    (0x19778, "character_damage?", "(charObj=ax, amount=edx, redraw=bl) -> new HP or -1: finds the character-table entry whose id == charObj, HP = clamp(HP - amount, 0, maxHP) (negative amount heals); character 7 immune while flag 0x0D is set; optionally refreshes the status display (sub_10DAC)."),
]

DATA += [
    (0xCCA44, "_combatants"),            # 23 x pointer to 0x3F8-byte combatant blocks, 0-terminated
    (0xADC4C, "_char_stats"),            # 0x15E bytes = 25 x 14-byte per-character stat records (save region 10)
    (0xA8FD4, "_combatant_templates"),   # 19 x 0x3F8: 11 enemy templates, then 8 party templates at 0xABB7C
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
            holder = idc.get_name_ea_simple(name)
            if holder != idc.BADADDR and holder != ea:      # the label sits on a wrong address (earlier mistake): free it
                idc.set_name(holder, "byte_%X" % holder, idc.SN_NOWARN)
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
               f"run_default_verb_handler?. Verb vocabulary table: 13-byte records at 0x1DAA0 "
               f"{{char* name; char* preposition; u8 flags; u32 verbId}}.", "table", True)

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
