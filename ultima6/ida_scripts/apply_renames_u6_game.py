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
    (0x3A8B8, "disk_present", "disk_present(n): 1 if the marker file U6.n can be opened (U6.1-U6.8 identify the floppy disks)"),
    (0x3A947, "require_disk", "prompts to insert disk N (4 = program disk, 8 = save disk, 5 = ...) if it is not the current one (word_406EA)"),
    (0x1D0DB, "load_look_data", "loads LOOK.LZD (the object description table)"),
    (0x1D15C, "get_object_name", "copies the description of an object type from the look data ('Nothing' when none); 31 callers"),
    # -- map and objects --
    (0x17F17, "load_map_region", "load_map_region(z, y, x): reads the 2 x 2 superchunks around (x, y) of MAP into map_buffer, or the dungeon level"),
    (0x18B37, "get_chunk_index", "get_chunk_index(z, y, x): 12-bit chunk number of a tile position"),
    (0x17E4F, "chunk_cache_get", "chunk_cache_get(chunk): slot number in the 16-slot LRU cache, reading 64 bytes of CHUNKS on a miss"),
    (0x18146, "build_terrain_cache", "fills terrain_cache from the chunks around the player and places the actors in object_grid"),
    (0x1838C, "refresh_map_view", "load_object_blocks + load_map_region + build_terrain_cache for the player position"),
    (0x1C636, "update_object_blocks", "update_object_blocks(z, y, x): loads the object blocks near the player, saving those left behind"),
    (0x1AE6A, "unload_distant_objects", "unload_distant_objects(z, y, x): drops objects more than 20 tiles away"),
    (0x1C2AF, "load_surface_objblock", "load_surface_objblock(n): reads savegame\objblk<col><row> (the .tmp copy when objblk_dirty)"),
    (0x1C566, "load_dungeon_objblock", "load_dungeon_objblock(level 1-5): reads objblk<level>I"),
    (0x1C098, "add_block_objects", "add_block_objects(count): turns the records in objblk_buffer into loaded objects (free list, container fix-up, sorted insert)"),
    (0x1BBE2, "write_objblock", "write_objblock(n, count): writes count and count * 8 bytes of records to the .tmp working file and marks the block dirty"),
    (0x1BA62, "collect_block_objects", "'Error-too many objects in region!'"),
    (0x1BD48, "save_surface_objblock", "collects and writes a surface block"),
    (0x1C473, "save_dungeon_objblock", "collects and writes a dungeon level"),
    (0x1BBA2, "delete_objblk_tmp_files", "deletes the .tmp working copies"),
    (0x1C88A, "commit_objblk_tmp_files", "replaces each dirty block's file with its .tmp copy"),
    (0x10D8E, "load_file", "load_file(far dest or 0 = allocate, name): reads a whole file; when word_3D9E4 is set the file starts with its size and is LZW compressed"),
    (0x3AC50, "lzw_load", "lzw_load(handle, offset dword, ...): positions the file and decompresses"),
    (0x3AEDC, "lzw_read_stream", "reads the size dword, then lzw_decode"),
    (0x3ACB4, "lzw_decode", "the LZW decoder: 9-12 bit codes, 100h reset, 101h end, 3-byte dictionary entries"),
    # -- text console --
    (0x17C83, "scroll_printf", "prints a formatted message to the text scroll; 141 callers (directives %d %s %> %/ ...)"),
    (0x17A13, "scroll_print_wrapped", "word-wrapping text output, breaks at ' ,.:;!?\\'-\"\\n'"),
    # -- commands (identified from their message strings) --
    (0x13C64, "command_loop", "main command loop: 'Restore game? ', 'Help ', calls use_object"),
    # -- the actors' turn (found in run 6) --
    (0x2AB7A, "run_actor_turns", "after every command: runs the schedule walkers, then actors act by moves left (dex per minute; first with full moves else best moves/dex) until the player-controlled member's turn; refills moves and calls advance_time(1) when nobody can act"),
    (0x13305, "advance_time", "advance_time(minutes): timers, per-actor status updates, lit torches/rings/cloak, hourly schedules and magic points"),
    (0x29BDA, "actor_worktype_act", "one action of an actor chosen by run_actor_turns: switch on its worktype (table at cs:3F1B) to the handlers below"),
    (0x2E454, "wt_combat_front", "worktype 3"),
    (0x2E4B9, "wt_combat_rear", "worktype 4"),
    (0x2E7E2, "wt_combat_flank", "worktype 5"),
    (0x2EC2A, "wt_combat_berserk", "worktype 6"),
    (0x2EEC2, "wt_combat_retreat", "worktypes 7, 9 (shy), 0x13 and 0x9A"),
    (0x2DF2E, "wt_combat_assault", "worktypes 8 (assault/wild) and 0x9B (attack party)"),
    (0x2F06F, "wt_combat_like", "worktype 0xA"),
    (0x2F1A7, "wt_combat_unfriendly", "worktype 0xB"),
    (0x2954B, "wt_wander", "worktypes 0xC (animal wander) and 0x8F (wander around)"),
    (0x2F4E6, "wt_tangle", "worktype 0xD (tangle vine)"),
    (0x2F2DE, "wt_immobile", "worktype 0xE"),
    (0x29723, "wt_work_12", "worktype 0x12"),
    (0x295ED, "wt_walk_path", "worktypes 0x83-0x85 (follow a path)"),
    (0x28AA7, "walk_to_schedule_place", "worktype 0x81/0x82 handling: walk towards the schedule position or put the actor there"),
    (0x29134, "wt_work", "worktypes 0x90 (work) and 0x94"),
    (0x2DD5E, "wt_0x99", "worktype 0x99 helper"),
    (0x26CC4, "spend_moves", "spend_moves(actor, cost): halved for a horse (type 0x1AF), minimum 1, subtracted from moves[actor]"),
    (0x28F67, "party_step_cost", "step cost of the square an actor leaves: 5 + the high nibble of TILEFLAG1 of the terrain and of each object there that is not impassable; then spend_moves"),
    (0x2C8BF, "combat_hit_check", "combat_hit_check(weapon, defender, attacker): hit when rnd(1..30) >= (defender dex + 30 - attacker str-or-dex) / 2"),
    (0x2C97F, "print_wound_text", "print_wound_text(actor): quarter of max hp -> critical / heavily / lightly / barely wounded"),
    (0x2C0D4, "weapon_damage", "weapon_damage(object): creature table damage for actors, else the weapon table (-1 if none)"),
    (0x2BA04, "weapon_range", "weapon_range(object): table ds:1AD5, default 1"),
    (0x2B3D7, "adjusted_strength", "strength, -3 (min 1) when the object status has 0x40 (cursed)"),
    (0x2B401, "adjusted_dexterity", "dexterity, -3 when cursed, 1 when asleep or under time stop (non-party)"),
    (0x1D8B1, "object_weight", "weight of an object: TILEFLAG block 3 x quantity, /10 for the nine types at ds:081F, 255 -> 10000"),
    (0x1D957, "contents_weight", "weight of what is inside an object"),
    (0x1D998, "compute_carried_weight", "word_4BF47 = unworn weight, word_4BF44 = worn weight of an actor"),
    (0x2FCAE, "print_article", "prints 'a ', 'an ' or 'the ' from the top two bits of TILEFLAG block 4"),
    (0x2FED1, "print_object_quantity_or_article", "prints '%d ' for stackable types or the article; returns the count"),
    (0x1B687, "stackable_kind", "4 = gold (word quantity), 2 = one of the 21 types at ds:0765 (byte quantity), 0 = not stackable"),
    (0x25D7F, "can_actor_enter", "passability check, see docs/u6_game.md"),
    (0x2787E, "party_move", "walking command: target square, diagonal rule, passability, steps"),
    (0x2DA99, "cmd_attack", "Attack command"),
    (0x2D13D, "perform_attack", "one attack with its hit check, miss redirection and projectiles"),
    (0x25495, "cast_spell_effect", "works a chosen spell"),
    (0x12CCD, "apply_tile_damage", "apply_tile_damage(tile, actor): switch table of the harmful tiles (swamp 2-5, fire 220-223, 890, 1124-1131, 1164, 1193, web 564, field 1165/1167, traps 731/732, saw 1010, 1458-1463)"),
    (0x2AED5, "update_actor_schedules", "hourly: for each actor 2-255 finds the schedule entry whose hour (low 5 bits of byte 0) equals the game hour and whose day of week (high 3 bits) matches, stores its index and sets worktype 81h"),
    (0x394A9, "load_portrait", "load_portrait(actor, dest far): PORTRAIT.Z for the Avatar, else A (numbers below 98) or B"),
    (0x2FE1F, "read_book", "read_book(object): book number = the object's quality byte; BOOK.DAT item number - 1 is read into the conversation script buffer and printed"),
    (0x395D0, "load_conversation", "load_conversation(actor, dest far): picks CONVERSE.A/B and the item (rules in docs/u6_conversation.md), LZW or plain"),
    (0x20AD3, "cmd_talk", "the Talk command: 'Not on screen.', 'Funny, no response.', 'Talking to yourself?'"),
    (0x1F1BE, "conv_read_value", "conversation interpreter: reads and evaluates the next value expression of the script"),
    (0x201EB, "conv_execute_command", "conv_execute_command(opcode): switch over opcodes 9Ch-0DFh through jpt_20207; 'Unknown command.' by default"),
    (0x1EA97, "conv_init_variables", "fills the conversation variables (slot v at 48F31h + 2v) from the game state"),
    (0x20144, "conv_skip_data_ref", "helper of DPRINT: reads a u32 offset and positions/skips in the script"),
    (0x1ED3A, "conv_resolve_npc", "maps the script's npc operand (0EBh = the current speaker) to an actor"),
    (0x208F5, "conv_print_char", "'Try to print out a control char'"),
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
    (0x34FC5, "use_crystal_ball", "'Enter degrees followed by N, S, E or W.', 'At latitude=', 'longitude=' (the dispatch maps the crystal ball, type 09Bh, here)"),
    # -- more handlers, named from their message strings (the action is inferred from the text) --
    (0x23C5A, "spell_create_food", "'Create %d food.'"),
    (0x23CBD, "spell_summon_daemon", "'Hostile daemon summoned.'"),
    (0x25CEF, "check_spellbook", "'No spellbook is readied.'"),
    (0x30F85, "cmd_get", "'Stop Thief!!!', 'Stealing!!!', 'The total is too heavy.', 'No free hand'"),
    (0x320D4, "use_key_on_door", "'Can't (Un)lock an opened door', 'locked!', 'magically locked!', 'unlocked!'"),
    (0x3224C, "use_door", "door/chest state messages: closed, opened, locked, magically locked, unlocked"),
    (0x3241E, "use_key_or_lockpick", "'Key broke.'"),
    (0x32886, "extinguish_light", "fireplace, brazier, candelabra, campfire, candle: ' is doused.'"),
    (0x32AB5, "bury_object", "'buried.', 'Cannot be buried here!'"),
    (0x32CF1, "fill_bucket_water", "'You need a bucket.', 'The bucket of water is too heavy for you.'"),
    (0x32D77, "fill_bucket_milk", "'You need a bucket.', 'The bucket of milk is too heavy for you.'"),
    (0x330C5, "use_cannon", "'Fire!'"),
    (0x331E9, "light_powder_keg", "'Powder lit!'"),
    (0x33271, "blow_up_door", "'The door is blown up!'"),
    (0x339CD, "operate_drawbridge", "'Close the drawbridge.', 'Open the drawbridge.', 'No correspondent drawbridge'"),
    (0x33B09, "use_lever", "'Switch the lever, ', 'can't close the portcullis.'"),
    (0x33D02, "use_switch", "'Operate the switch, '"),
    (0x33E70, "use_scroll", "'You study the scroll!'"),
    (0x34228, "use_shrine", "'Mantra: ', 'Wrong mantra!'"),
    (0x3452B, "use_fountain", "'Make a wish?', 'Wish for: ', 'You got food' (type 0EAh, fountain)"),
    (0x34B93, "use_horse", "'Not boardable!', 'You're already on a horse!'"),
    (0x34C80, "rear_horse", "'Rear horse???'"),
    (0x34E19, "use_orb_of_the_moons", "'Where: ', 'a red moon gate appears.' (type 057h; moonstones, 049h, are buried by bury_object)"),
    (0x35482, "operate_passthrough", "'Open the passthrough.', 'Close the passthrough.'"),
    (0x3A194, "show_hull_strength", "'Hull Strength: %d%%'"),
    (0x3AAC1, "disk_full_dialog", "'Disk full. Do you want to Save the game and continue, or Restore your last saved game?'"),
    (0x2536C, "use_sextant", "type 05Dh: prints the position in degrees ('%d{')"),
    (0x3563C, "use_vortex_cube", "type 03Eh: 'The Codex has vanished!'"),
    (0x329EA, "play_instrument", "play_instrument(n): harp, harpsichord, lute, panpipes (types 09C, 09D, 09E, 099)"),
    (0x32DFD, "use_churn", "type 0B5h: 'You need some milk.'"),
    (0x32E63, "use_beehive", "type 0B6h: 'You need a honey jar.'"),
    (0x32EC2, "use_potion", "type 113h (blue potion)"),
    (0x35766, "use_fan", "type 0D4h: 'You feel a breeze.'"),
    (0x3574D, "use_rubber_ducky", "type 0A9h: 'Squeak!'"),
    (0x3525F, "use_silver_horn", "type 139h: 'Silver snakes are generated!'"),
    (0x35127, "use_telescope", "type 09Ah"),
    (0x3298A, "open_secret_door", "type 14Eh"),
    (0x186DE, "climb_ladder", "type 131h"),
    (0x3282F, "use_gems", "type 04Dh"),
    (0x32A1D, "ring_bell", "types 0ECh (bell) and 1A3h (pull chain)"),
    (0x355D3, "use_food", "'You eat the food.'"),
    (0x1166B, "make_config_string", "builds the hardware settings string passed to END.EXE (video letter, M/K, sound letter, 3 digits)"),
    (0x116CE, "start_ending", "computes the time played (game date minus 4th day, 7th month, year 161), formats years/months/days and execl's END.EXE with them"),
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
    (0x48C67, "object_weights_ptr", "far ptr to u8[1024] (TILEFLAG +0x1000): the weight of each object type; the conversation weight operator uses it"),
    (0x48C3F, "tileflags3_ptr", "far ptr to u8[2048] (TILEFLAG +0x1400)"),
    (0x44074, "basetile_ptr", "far ptr to BASETILE: u16[1024], first tile of each object type"),
    (0x4647C, "palette_ptr", "far ptr to U6PAL 0x300 bytes"),
    # SCHEDULE
    (ds(0xB421), "schedule_header", "SCHEDULE first 0x202 bytes (u16[256] index + count)"),
    (0x48E71, "schedule_count", "last word of the header: number of 5-byte entries"),
    (0x46478, "schedule_entries_ptr", "far ptr to count * 5 bytes"),
    # objlist (offsets are file offsets within savegame\objlist)
    (ds(0xA0AB), "obj_status", "u8[0xC00]: object status/location bits (mask 18h: 0 map, 8 container, 10h inventory, 18h readied); the first 256 entries (the actors) are objlist +0x0000"),
    (ds(0x6828), "obj_pos", "3 bytes per object (x 10 bits, y 10 bits, z 4 bits) [0xC00]; first 256 = objlist +0x0100; a contained object keeps its container's id in the x word"),
    (ds(0x3548), "obj_type_frame", "u16[0xC00] = type (10 bits) | frame << 10; first 256 = objlist +0x0400"),
    (ds(0x4EE4), "obj_quantity", "u16[0xC00] quantity (byte 6 of the block record) and quality (byte 7); first 256 = objlist +0x0600"),
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
    (0x46CE1, "actor_schedule_index_ptr", "objlist +0x10F1, u8[256]: index of the schedule entry in force (set by the hourly schedule check sub_2AED5)"),
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
    (ds(0x2CCA), "avatar_gender", "objlist +0x1C71: 1 = female"),
    (ds(0x2CCB), "avatar_portrait_plus1", "objlist +0x1C72: portrait + 1, 0 = no character yet; _main refuses to start without one"),
    (ds(0x2C4A + 0x08), "karma", "objlist +0x1BF9 (the new game starts at 75, set by U.EXE)"),
    (0x425A0, "conv_script_ptr", "far ptr to the loaded conversation script"),
    (0x4BFFB, "conv_script_pos", "offset of the next script byte"),
    # map and objects
    (ds(0xBDDA), "obj_next", "u16[0xC00]: next object in the sorted list of loaded objects (-1 ends; index 100h is the list head)"),
    (0x4AE2A, "obj_free_head", "head of the free-object list (linked through obj_next)"),
    (0x4BF30, "obj_free_count", "number of free object slots"),
    (0x47689, "objblk_buffer_ptr", "far ptr: scratch buffer for object block records (0xC00 * 8 bytes) followed, at +6000h, by an id map"),
    (ds(0x065C), "objblk_dirty", "u16[69]: 1 when block n (0-63 surface, 64-68 dungeon levels 1-5) has a .tmp working copy"),
    (ds(0x0709), "objblk_name", "'savegame\objblkaa': letters 15 and 16 are patched with the block's column and row"),
    (ds(0x071B), "objblk_tmp_name", "'savegame\objblkaa.tmp'"),
    (0x48FF4, "map_buffer_ptr", "far ptr: the 4 resident surface superchunks (4 * 0x180 bytes) or one dungeon level (0x600)"),
    (ds(0x05D2), "map_loaded_superchunks", "u16[4]: superchunk numbers resident in map_buffer (base, +1, +8, +9, modulo 64)"),
    (0x3DE20, "map_loaded_level", "dungeon level in map_buffer (0 = surface)"),
    (0x3DE1E, "map_file_handle", "handle of MAP while it is being read"),
    (0x3DE1C, "chunks_file_handle", "handle of CHUNKS, kept open"),
    (0x3DE1A, "map_coord_mask", "3FFh on the surface, 0FFh in a dungeon"),
    (ds(0xB784), "chunk_cache_age", "u16[16] LRU ages of the chunk cache"),
    (ds(0xB7A8), "chunk_cache_id", "u16[16] chunk number in each cache slot"),
    (ds(0xB7C8), "chunk_cache_tiles", "16 * 64 tile numbers"),
    (ds(0x8E51), "terrain_cache", "u8[40][40] terrain tiles around the player (5 x 5 chunks)"),
    (ds(0xD8E7), "object_grid", "u16[40][40] the object on each cache tile"),
    (0x49418, "terrain_cache_x", "world x of terrain_cache column 0 (multiple of 8)"),
    (0x4941A, "terrain_cache_y", "world y of terrain_cache row 0"),
    (0x404A6, "player_x", "party x position (word_404A6)"),
    (0x404A8, "player_y", "party y position"),
    (0x404AA, "player_z", "party level (0 = surface, 1-5 dungeon)"),
    (0x4049E, "game_day", "objlist +0x1BF5: day of month 1-28"),
    (0x4049F, "game_month", "objlist +0x1BF6: 1-13"),
    (0x404A0, "game_year", "objlist +0x1BF7, word"),
    (0x35AF7, "cmd_use_types", "u16[85] usable object types, sorted (a copy of the table at ds:1C30)"),
    # runtime
    (0x40A7E, "mem_next_free", "far ptr: next free byte of the start-up block"),
    (0x40A82, "mem_bytes_left", "dword"),
    (0x4C508, "video_driver_call", "far ptr; the offset is set to a driver entry number before each call (3 = shut down, 0x24 = set palette, 0x2A = init ...)"),
    (0x3DA18, "video_mode", "0 VGA, 1 EGA, 2 Tandy, 3 CGA, 4 Hercules (letters v e t c h of CONFIG.U6; see _main's scan-line table set-up)"),
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
