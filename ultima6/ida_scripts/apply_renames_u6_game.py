"""
Master list of symbol renames for Ultima VI's GAME.EXE (u6_game.idb, built from
the unpacked GAME_UNP.EXE).  Single accumulating script: when a finding is
confirmed add an entry and re-run (it is idempotent).  Justification for each
entry is in docs/u6_game.md.

    .\\run_ida_script.ps1 -Idb u6_game -ScriptName apply_renames_u6_game.py

Addresses are IDA linear addresses.  The data segment starts at 0x3D850, so a
`ds:XXXX` offset seen in the code is DS_BASE + XXXX.
"""

import idc
import ida_name

DRY_RUN = False
DS_BASE = 0x3D850


def ds(off):
    return DS_BASE + off


# (ea, new_name, note)
FUNCTIONS = [
    # -- DOS file layer (INT 21h wrappers; every failure calls the error hook at
    #    off_3DB7E with the error kind in word_3DB84: 1 open, 2 close, 5 read/seek) --
    (0x1020A, "file_open", "AH=3Dh read/write; arg = ds offset of the ASCIZ name; returns handle or -1"),
    (0x10249, "file_close", "AH=3Eh; arg = handle"),
    (0x1027D, "file_create", "AH=3Ch; arg = ds offset of the name; returns handle or -1"),
    (0x102BD, "file_delete", "AH=41h; arg = ds offset of the name"),
    (0x102FB, "file_read", "file_read(dest far, count dword, offset dword (-1 = continue), handle); reads in 0FFF0h chunks; returns bytes read"),
    (0x103E1, "file_write", "same arguments as file_read, writes"),
    (0x104A6, "file_rename", "AH=56h; args = two ds offsets"),
    # -- memory --
    (0x10CB1, "mem_alloc_zeroed", "mem_alloc_zeroed(size dword, far pointer variable *): carves size bytes off the block taken at start-up "
                                  "(dword_40A7E = next free, dword_40A82 = bytes left), zero-fills it; exits with the driver's shutdown call when exhausted"),
    # -- start-up and data loading --
    (0x1118B, "load_tile_attribs", "loads ANIMDATA (194 bytes) and TILEFLAG (0x800 + 0x800 + 0x400 + 0x800 bytes) via mem_alloc_zeroed buffers"),
    (0x11474, "load_palette", "loads U6PAL (0x300 bytes, VGA only) and sets it through the video driver"),
    (0x14A6A, "load_world_data", "loads SCHEDULE, BASETILE, savegame\\objlist (all arrays, layout in docs/u6_game.md) and CHUNKS"),
    (0x14EDF, "save_objlist", "writes savegame\\objlist: same arrays in the same order as load_world_data"),
    (0x3A947, "require_disk", "prompts to insert disk N (4 = program disk, 8 = save disk, 5 = ...) if it is not the current one (word_406EA)"),
    (0x1D0DB, "load_look_data", "loads LOOK.LZD (the object description table)"),
    (0x1D15C, "get_object_name", "copies the description of an object type from the look data ('Nothing' when none); 31 callers"),
    # -- text console --
    (0x17C83, "scroll_printf", "prints a formatted message to the text scroll; 141 callers (directives %d %s %> %/ ...)"),
    (0x17A13, "scroll_print_wrapped", "word-wrapping text output, breaks at ' ,.:;!?\\'-\"\\n'"),
    # -- commands (identified from their message strings) --
    (0x13C64, "command_loop", "main command loop: 'Restore game? ', 'Help ', calls use_object"),
    (0x35809, "cmd_use", "the Use command: dispatcher with 44 object handlers"),
    (0x302F7, "cmd_look", "the Look command: 'Thou dost see ...', weight, damage/defense"),
    (0x30031, "search_here", "'Searching here, you find ...'"),
    (0x30A4E, "ask_quantity", "'How many? '"),
    (0x1E69B, "cmd_ready", "'Ready-'"),
    (0x1E988, "cmd_unready", "'Unready-%s'"),
    (0x3A1DD, "cmd_rest", "'Rest', 'How many hours? ', 'Who will guard? '"),
    (0x259E6, "cmd_cast", "'No magic at this time!', 'Not enough magic points.', 'No Reagents.'"),
    (0x31F33, "trigger_trap", "'Acid!', 'Poison!', 'Bomb!', 'Gas!'"),
    (0x32661, "use_torch", "'Torch is lit.'"),
    (0x343D6, "use_fishing_pole", "'Didn't get a fish.'"),
    (0x34656, "use_shovel", "'You dig a hole.', water fountain, gold nugget"),
    (0x34FC5, "use_sextant", "'At latitude=', 'longitude='"),
    (0x355D3, "use_food", "'You eat the food.'"),
]

# (ea, new_name, note)  -- data; DS-relative addresses go through ds()
DATA = [
    # ANIMDATA
    (ds(0xB41F), "anim_count", "ANIMDATA word 0"),
    (ds(0x67E4), "anim_tile_to_animate", "u16[32]"),
    (ds(0x9DF5), "anim_first_frame", "u16[32]"),
    (ds(0x9495), "anim_and_masks", "u8[32]"),
    (ds(0xB3F7), "anim_shift_values", "u8[32]"),
    # TILEFLAG
    (0x48C3B, "tileflags1_ptr", "far ptr to u8[2048] (TILEFLAG +0x0000)"),
    (0x46496, "tileflags2_ptr", "far ptr to u8[2048] (TILEFLAG +0x0800)"),
    (0x48C67, "tileflag_block3_ptr", "far ptr to u8[1024] (TILEFLAG +0x1000)"),
    (0x48C3F, "tileflags3_ptr", "far ptr to u8[2048] (TILEFLAG +0x1400)"),
    (0x44074, "basetile_ptr", "far ptr to BASETILE: u16[1024], first tile of each object type"),
    (0x4647C, "palette_ptr", "far ptr to U6PAL 0x300 bytes"),
    # SCHEDULE
    (ds(0xB421), "schedule_header", "SCHEDULE first 0x202 bytes (u16[256] index + count)"),
    (0x48E71, "schedule_count", "last word of the header: number of 5-byte entries"),
    (0x46478, "schedule_entries_ptr", "far ptr to count * 5 bytes"),
    # objlist (offsets are file offsets within savegame\objlist)
    (ds(0xA0AB), "actor_obj_flags", "objlist +0x0000, u8[256]"),
    (ds(0x6828), "actor_pos", "objlist +0x0100, 3 bytes per actor (x 10 bits, y 10 bits, z 4 bits)"),
    (ds(0x3548), "actor_obj_frame", "objlist +0x0400, u16[256] = obj_n | frame_n << 10; also the table of the first 256 objects"),
    (ds(0x4EE4), "objlist_0600", "objlist +0x0600, 0x200 bytes (purpose not yet established)"),
    (ds(0x9FAB), "actor_status_flags", "objlist +0x0800, u8[256]"),
    (ds(0x8C4A), "actor_strength", "objlist +0x0900"),
    (ds(0x3316), "actor_dexterity", "objlist +0x0A00"),
    (ds(0x3433), "actor_intelligence", "objlist +0x0B00"),
    (ds(0x9BF5), "actor_exp", "objlist +0x0C00, u16[256]"),
    (ds(0x66E4), "actor_hp", "objlist +0x0E00"),
    (ds(0x3236), "party_names", "objlist +0x0F00, 14 * 16 bytes"),
    (ds(0x3533), "party_roster", "objlist +0x0FE0, 16 bytes (actor ids)"),
    (ds(0x8E50), "party_size", "objlist +0x0FF0"),
    (0x4669C, "actor_level_ptr", "objlist +0x0FF1, u8[256] (far ptr)"),
    (0x46CE1, "objlist_10f1_ptr", "objlist +0x10F1, u8[256] (far ptr, purpose not yet established)"),
    (0x40D94, "actor_worktype_ptr", "objlist +0x11F1, u8[256] (far ptr)"),
    (0x47685, "objlist_12f1_ptr", "objlist +0x12F1, u8[256] (combat mode / magic points, far ptr)"),
    (ds(0x9AF5), "objlist_13f1", "objlist +0x13F1, u8[256] (magic points / combat mode)"),
    (ds(0x8D4A), "actor_moves", "objlist +0x14F1, u8[256] movement points"),
    (0x46480, "actor_base_obj_frame_ptr", "objlist +0x15F1, u16[256] (far ptr)"),
    (ds(0xB2EB), "actor_talk_flags", "objlist +0x17F1, u8[256]"),
    (0x40C7F, "objlist_18f1_ptr", "objlist +0x18F1, u8[256] (far ptr, purpose not yet established)"),
    (0x4259C, "actor_movement_flags_ptr", "objlist +0x19F1, u8[256] (far ptr)"),
    (0x46492, "objlist_1af1_ptr", "objlist +0x1AF1, u8[256] (far ptr, purpose not yet established)"),
    (ds(0x2C4A), "objlist_globals", "objlist +0x1BF1, 0x2CCC bytes: quest flag, rest counter, game time, karma, wind, timers, ..."),
    # runtime
    (0x40A7E, "mem_next_free", "far ptr: next free byte of the start-up block"),
    (0x40A82, "mem_bytes_left", "dword"),
    (0x4C508, "video_driver_call", "far ptr; the offset is set to a driver entry number before each call (3 = shut down, 0x24 = set palette, 0x2A = init ...)"),
    (0x3DA18, "video_mode", "0 = VGA/MCGA, 1 = EGA, 2 = CGA, 3 = ?, 4 = Hercules (see _main's scan-line table set-up)"),
]


def apply(items):
    done = skipped = 0
    for ea, name, note in items:
        cur = idc.get_name(ea)
        if cur == name:
            continue
        if DRY_RUN:
            print("would rename %05X %s -> %s" % (ea, cur, name))
            continue
        if ida_name.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            done += 1
        else:
            skipped += 1
            print("FAILED %05X %s -> %s" % (ea, cur, name))
    return done, skipped


f = apply(FUNCTIONS)
d = apply(DATA)
print("functions: %d renamed, %d failed; data: %d renamed, %d failed" % (f[0], f[1], d[0], d[1]))
