"""
IDA Pro script: master list of symbol renames (functions + globals) for
Ultima II (DOS).

Single accumulating script instead of one standalone name_*.py file per
finding. Whenever a function's or global's role becomes clear enough to
name confidently, add an entry to RENAMES below and re-run. Safe to
re-run repeatedly -- each entry is checked against the address's
*current* name and skipped if already applied, so old entries are
harmless to leave in place. Keep them rather than deleting: this file
doubles as a dated changelog of "what have we named and roughly why"
that's easy to diff in git, which is the whole point of consolidating
into one file instead of many.

Convention: DRY_RUN is left False (Paul's call, 2026-08-17) -- new
entries take effect the moment they're added and the script is re-run,
no separate dry-run confirmation pass. Sanity-check a new entry's
address and name before adding it, since there's no dry-run safety net
here anymore -- get it right in the list, not via a preview step.
(apply_structs.py hasn't had this same call made yet; treat it as
dry-run-first until told otherwise.)

Scope: plain renames only (idc.set_name on an address that IDA can
already address -- a function start, or an existing named/auto-named
data item). If a finding requires *creating* new data where there was
previously nothing (splitting an array, building a table, adding xrefs)
that's structural surgery, not a rename -- write a dedicated one-off
script for it instead (see resolve_command_jump_table.py and
fix_access_file_calls.py for that pattern). Struct member renames/
additions go in apply_structs.py, not here, since they use a different
IDA API (add_struc_member / set_member_name) and address a struct
definition rather than a single linear address.

For fuller justification of each rename, see the matching section of
docs/overview.md or docs/file-formats.md -- the `note` field here is
just enough to read the list top-to-bottom without cross-referencing.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x10730, "command_jump_table",
     "26-entry A-Z command dispatch table (formerly off_10730). See "
     "docs/overview.md#resolved-a-z-command-jump-table-and-the-tlkxff-loader."),

    (0x122D5, "load_talk_file",
     "loads TLKXFF for the current map; formerly sub_122D5. NOT a "
     "Talk-key handler -- there is no Talk key, called from `enter` on "
     "VILLAGE/TOWN/CASTLE only. See docs/overview.md as above."),

    (0x1154E, "print_indexed_shop_string",
     "given an index in AL, walks the load_talk_file buffer "
     "(word_17886) to the index-th null-terminated string and prints "
     "it via write_character. Called only from `transact`'s shopkeeper "
     "branch -- TLKXFF data turns out to be shop response text, not "
     "walk-up NPC dialogue. See "
     "docs/file-formats.md#consumer-traced-read-out-by-transact-not-a-walk-up-talk."),

    (0x153F4, "print_char",
     "thin wrapper, literally `call write_character; retn`. Called from "
     "sub_10A30 and from print_indexed_shop_string; why the wrapper "
     "exists (vs. calling write_character directly) isn't understood "
     "yet."),

    # -- 8 helpers shared across the 26 A-Z command handlers, ranked by
    # how many different commands call each one (see
    # docs/overview.md#command-handler-shared-helpers-decoded-and-applied
    # for the full evidence trail per entry) --

    (0x15217, "rand_byte",
     "PRNG: combines three running BIOS-timer-driven values "
     "(_timer2/5/6) into _timer1, updates a small shift-register "
     "history buffer, returns the new byte in AL. Called from attack, "
     "fire, get, offer, steal -- the dice roll behind hit chance, item "
     "drops, spell luck, etc."),

    (0x123E6, "find_target_monster",
     "computes the cursor+facing-direction target cell, scans all 32 "
     "monster slots for one active ([di+197h]!=0) and positioned "
     "there, returns its slot index in di (0 if none found). Called "
     "from attack, fire, offer, transact -- the core 'who does this "
     "command act on' lookup."),

    (0x12028, "read_digit_keypress",
     "blocks on keypress until a '0'-'9' digit, echoes it plus a "
     "cursor-control byte, returns the numeric value 0-9 in AX. "
     "Called from magic, offer, ready, wear_armor for 'which "
     "spell/item number?' prompts."),

    (0x1508C, "print_indexed_menu_string",
     "same mechanism as print_indexed_shop_string (find the Nth "
     "null-terminated string, print it) but against a FIXED "
     "compile-time table at cs:4C60h, not the runtime talk buffer. "
     "Called from cast, magic, ready, wear_armor, zstats -- reused "
     "across weapon/armor/spell selection and stats display, so the "
     "table likely holds more than one logical list. Name deliberately "
     "generic -- table contents at cs:4C60h not yet dumped/confirmed, "
     "see docs/roadmap.md."),

    (0x1113A, "alert_town_guards",
     "no-op only on the overworld (player._mapNum2==0 -- see the "
     "_mapNum2 value table in docs/overview.md: 0=overworld, "
     "1-3=village/town/castle, 4=tower, 5=dungeon). Everywhere else: "
     "sets [di+1D7h]=1 for monster slots 0-7 plus the current target "
     "slot. Called from attack, steal -- committing a hostile act "
     "raises an alarm flag on a block of slots, plausibly the town's "
     "reserved guard slots."),

    (0x12052, "try_spend_gold",
     "subtracts a 2-byte BCD amount (byte_17438/_sleepFlag2? as "
     "low/high) from player._gold; on underflow, undoes the "
     "subtraction and prints 'YOU DONT HAVE THAT MUCH!', returns 0 on "
     "failure. Called from offer, transact."),

    (0x15ED7, "play_hit_sound",
     "thin wrapper: calls play_tone_sweep with fixed parameters, "
     "right after a successful melee hit in attack."),

    (0x15DA9, "play_tone_sweep",
     "checks a mute flag (byte_1795D, toggled elsewhere via "
     "'xor ...,0FFh') then sweeps the PC speaker from one frequency to "
     "another over N steps. Generic tone-sweep primitive reused for "
     "multiple sound effects (play_hit_sound, pause?'s sound, etc.), "
     "not attack-specific itself."),

    # -- found while closing out the cast/_readiedSpell trace --

    (0x15FC3, "play_magic_sound",
     "thin play_tone_sweep wrapper, structurally identical to "
     "play_hit_sound but different fixed params. NOT cast-specific "
     "despite being called there (as soon as a cast attempt begins, "
     "before the map-type gate) -- also called from 3 unrelated "
     "trap/curse handlers ('LEGS PARALIZED!', 'ARMS PARALIZED!', "
     "'SLEEP SPELL!') plus a 'MAGIC MISSILE!' trap hitting the player "
     "(distinct from the player's own Magic Missile spell). Generic "
     "'something magical just happened' chime. See "
     "docs/overview.md#sub_15fc3-and-sub_15e83--the-two-sound-effects-traced."),

    (0x15E83, "play_fail_sound",
     "cast's fail buzzer: checks byte_1795D then sweeps a descending "
     "tone via the lower-level primitives directly (sub_15CB2/"
     "sub_15CD2/sub_15CE4), not via play_tone_sweep. Called at all 4 "
     "of cast's failure points ('NEED WAND OR STAFF!', 'NO SPELL!', "
     "both '-FAILED!' cases) -- cast-only, no other callers."),

    # -- found while tracing what end_of_turn2 skips --

    (0x15C95, "play_tick_sound",
     "generic 'turn advances' tick: checks byte_1795D then plays one "
     "flat tone (bx=0x500, cx=0x4000) via sub_15CB2/sub_15CD2/"
     "sub_15CE4 -- same primitives as play_fail_sound but no sweep, "
     "just a single beep. Called only from end_of_turn's own first "
     "instruction; skipped whenever a caller jumps straight to "
     "end_of_turn2 instead (e.g. board's 4 vehicle-boarding "
     "successes). See "
     "docs/overview.md#play_tick_sound--the-turn-tick-sound-traced-and-renamed."),

    # -- found while tracing the visual flash bracketing these sounds --

    (0x14B26, "flash_screen",
     "screen-invert flash: calls xor_invert_cga_bank twice, once per "
     "interleaved CGA framebuffer segment (0xB800/0xBA00). XOR-based, "
     "so the bracketing pattern seen everywhere (call before + after "
     "a sound/effect) inverts then restores. Reused across the 4 "
     "end_of_turn trap handlers, an armor-damage encounter, and "
     "fire's 'killed Minax' branch -- general purpose, not "
     "event-specific. See "
     "docs/overview.md#flash_screen--the-screen-flash-effect-traced-and-renamed."),

    (0x14B35, "xor_invert_cga_bank",
     "given a CGA framebuffer segment in AX, XORs every word from "
     "offset 0 through 0x1900 with 0xFFFF, inverting that memory. "
     "Private helper, only caller is flash_screen."),

    # -- top 5 of a 73-function sub_XXXXX sweep, ranked by call-site
    # reuse (counting both `call` and `jmp`, since this codebase tail-
    # jumps a lot). All 5 are one cohesive CGA point/line-drawing
    # cluster used pervasively by the space-travel screens, sharing
    # byte_17889/byte_178A (current point) + word_178CF/word_178D1
    # (viewport origin) addressing --

    (0x167B3, "draw_line",
     "Bresenham line stepper, 44 static call/jmp sites -- by far the "
     "most-reused unnamed function in the binary. Steps "
     "byte_17889/byte_178A (current point, updated in place) toward "
     "byte_1788B/byte_1788C (target point), calling plot_point once "
     "per pixel step. byte_1788D/byte_1788E are the X/Y step "
     "directions (+1/-1), byte_17893/byte_17894 the |dx|/|dy| "
     "magnitudes -- textbook Bresenham setup (major axis by "
     "cmp |dx|,|dy|, error accumulator in byte_17891)."),

    (0x14B76, "plot_point",
     "converts byte_17889/byte_178A + viewport origin "
     "(word_178CF/word_178D1) to a CGA framebuffer address + 2bpp "
     "pixel mask (same interleaved-bank addressing as "
     "xor_invert_cga_bank/flash_screen), then ORs the mask in -- sets "
     "one pixel. Called once per step from inside draw_line's loop, "
     "plus called directly elsewhere (17 static call sites)."),

    (0x14BAA, "erase_point",
     "identical address/mask computation to plot_point, but ANDs an "
     "inverted mask in instead of ORing -- clears one pixel instead of "
     "setting it. The erase counterpart, 14 static call sites."),

    (0x14B4E, "clear_screen",
     "calls clear_cga_bank once per interleaved CGA segment "
     "(0xB800/0xBA00), same two-bank pattern as flash_screen. Called "
     "early in start_ and reused elsewhere (10 sites) -- generic "
     "screen clear, not intro-specific."),

    (0x14B5D, "clear_cga_bank",
     "given a CGA segment in AX, zeroes 0x1900 (6400) bytes word by "
     "word -- one bank's worth of the sub-full-screen graphics area "
     "(200-line CGA needs 8000 bytes/bank; 6400 covers 160 of 200 "
     "rows, leaving room for a status/text area). Private helper, only "
     "caller is clear_screen."),

    # -- 3 PC-speaker primitives, already understood from tracing "
    # play_hit_sound/play_tone_sweep/play_fail_sound/play_tick_sound "
    # (which call these directly) but never renamed themselves --

    (0x15CB2, "speaker_on",
     "sets PIT (8253) timer channel 2 to square-wave mode (out 43h, "
     "0B6h), loads the initial frequency divisor from BX, saves the "
     "current port 61h state, and (unless byte_1795D mute flag is set) "
     "enables the speaker gate bits. Called once at the start of a "
     "sound effect."),

    (0x15CD2, "hold_tone",
     "reloads the PIT frequency divisor from BX, then busy-waits via a "
     "nested countdown (CX outer, a fixed inner loop). Called once per "
     "step by play_tone_sweep with a changing BX each time -- the "
     "'set new pitch and hold briefly' primitive a sweep is built from."),

    (0x15CE4, "speaker_off",
     "restores the port 61h state speaker_on saved (cs:byte_15C90) -- "
     "silences the speaker. Called once at the end of a sound effect."),

    # -- ship-marker draw/erase pair, found while tracing the dense
    # sub_16xxx hyperwarp/star-map cluster --

    (0x16D83, "draw_ship_marker",
     "draws a '+'-shaped crosshair centered on byte_1788B/byte_1788C "
     "(the ship's current on-screen position during hyperwarp/space "
     "flight): 6 plot_point calls stepping byte_1788A (vertical "
     "stroke), then 6 more stepping byte_17889 (horizontal stroke, "
     "with a 1-pixel gap at center where the vertical stroke already "
     "covers it)."),

    (0x16CC4, "erase_ship_marker",
     "identical shape to draw_ship_marker (same 6+6 point pattern) but "
     "calls erase_point instead of plot_point -- the standard 'erase "
     "old position before redrawing at the new one' sprite-move "
     "companion."),

    (0x168C0, "get_dungeon_tile_at_player",
     "reads a tile byte using _playerX/_playerY with a 16-row stride, "
     "banked by byte_17435 (dungeon depth level) into map_ptr -- "
     "distinct from get_player_tile's 64-row overworld stride (16x16 "
     "dungeon/tower grid vs. 64x64 overworld). Called 3x from "
     "sub_16000 (itself called from end_of_turn) with _playerX/Y "
     "temporarily offset to probe center/up/down, feeding the "
     "already-named _tilePlayerCenter/_tilePlayerUp/_tilePlayerDown."),

    (0x150D8, "sign_byte",
     "classic sign(): al==0 -> 0, al negative (0x80-0xFF) -> 0xFF "
     "(-1), else -> 1. Called twice from canMoveToTile, presumably on "
     "a movement delta."),

    (0x150EF, "abs_byte",
     "classic abs() for a signed byte: al<0x80 unchanged, else "
     "two's-complement negate (XOR 0FFh, +1). Called twice from "
     "end_of_turn."),

    # -- the two remaining pieces of the dense hyperwarp starfield
    # cluster, completing draw_ship_marker/erase_ship_marker above --

    (0x16B3D, "animate_starfield",
     "draws the ship marker (draw_ship_marker), then loops all 64 "
     "star slots (byte_178DD[si]/byte_1791D[si] = each star's screen "
     "X/Y): computes the star's new position relative to ship motion, "
     "boundary-checked via the byte_16CAC/byte_16CBC lookup tables, "
     "erase_point at the old position + plot_point at the new one, "
     "respawning (via next_star_coord) any star whose boundary check "
     "fails (scrolled off-screen). Also plays a tick via hold_tone "
     "periodically. The per-frame hyperwarp starfield animation, "
     "called from hyperwarp's main loop."),

    (0x16D68, "next_star_coord",
     "lightweight PRNG step (new = old_low + 9 + old_high-ish, byte "
     "pair byte_16996/byte_16997), distinct from the game's main "
     "rand_byte. Only caller is animate_starfield, called twice back "
     "to back to generate a fresh X then Y (masked to 0-127) when "
     "respawning a star that's scrolled off-screen."),

    # -- resolves what sub_129D2 (only caller: view) actually does,
    # previously described only as "a pure map-drawing routine" during
    # the TEXT_STRINGS location-descriptor dead-data investigation --

    (0x129D2, "draw_world_map_overview",
     "view's \"VIEW WITH MAGICAL HELM!\" implementation. _circleDeltaX "
     "increments 0-63 as the inner loop, _circleDeltaY 0-63 as the "
     "outer loop (both masked &3Fh) -- a full 64x64 scan of the "
     "overworld map, NOT a circle despite the variable names (reused "
     "scratch globals here, unrelated to their name). For each cell, "
     "reads the tile via get_player_tile and, for several tile-type "
     "categories (0x10, 0x78-0xF0 range, 4/8/0xC/0x70), draws a small "
     "2-4-point glyph icon via plot_map_icon_point instead of the real "
     "tile graphic -- a simplified world-map overview, since a real "
     "64x64-tile render wouldn't fit legibly on a 320x200 screen."),

    (0x12B65, "plot_map_icon_point",
     "converts a small sub-cell offset (di/si, values 0-3) plus the "
     "current scan position (_circleDeltaX*4, _circleDeltaY*2) into "
     "screen coordinates and plots one point via plot_point -- one "
     "pixel of a per-tile-type glyph icon. Called repeatedly (18 "
     "sites) by draw_world_map_overview, different di/si combinations "
     "per tile-type case forming each glyph's distinct point pattern."),

    (0x12362, "read_direction_keypress",
     "busy-waits for a directional keypress (checks NORTH_KEYCODE and "
     "similar), sets _circleDeltaX/_circleDeltaY to the chosen "
     "direction's delta (reused as generic scratch delta vars here, "
     "same as in draw_world_map_overview -- not actually circle-"
     "related). Idle timeout (~64K-iteration double counter, "
     "byte_1742F/byte_17430) falls through to 'PASS' and end_of_turn. "
     "Called from attack/fire's 'which direction?' prompts, parallels "
     "the already-named read_digit_keypress."),

    (0x1068F, "longPauseScreen",
     "identical to pauseScreen (mov dx,N; call delayMilli?) but a "
     "longer delay constant -- 0A000h vs pauseScreen's 6000h."),

    (0x120AB, "compute_item_price",
     "shop item-price calculator, discounted by the player's combined "
     "intelligence+charisma: computes di = bit-length(_intelligence + "
     "_charisma) via a shift-count loop, then builds the final price "
     "in a 2-byte BCD accumulator (_attribPoints/points_to_distrubte, "
     "reused as scratch here -- unrelated to their character-creation "
     "names) via di repeated additions of a per-item constant "
     "(_sleepFlag2?, also reused as scratch). All 6 call sites are "
     "inside transact's shop-purchase branches (food, horse, etc.) -- "
     "a 'smooth talker gets a better price' haggling mechanic."),

    (0x16D5B, "seed_star_prng",
     "sets next_star_coord's PRNG state (byte_16996/byte_16997) to a "
     "fixed constant (3Bh/67h) -- every hyperwarp launch gets the "
     "same starfield sequence. Called from hyperwarp's startup."),

    (0x15FA6, "play_bump_sound",
     "one-shot play_tone_sweep call with fixed, unusual params "
     "(ax=2F8h bx=320h cx=0FFFFh dx=50h bp=0F00h), called from "
     "normal_movement. Best guess given the calling context is a "
     "'can't move there' bump sound -- not independently confirmed, "
     "lower confidence than most of this sweep's other entries."),

    (0x15D09, "play_melody",
     "generic table-driven melody player: bx selects a tune from "
     "off_15D9B (pointer table), then walks a duration/frequency note "
     "list at that pointer (0FFFFh frequency = rest) via hold_tone/"
     "speaker_off, until a 0 duration ends the tune. Gated by the "
     "byte_1795D mute flag."),

    (0x15EC7, "play_step_tick",
     "thin play_melody wrapper, always tune 0 (bx=0). Called across "
     "play_game's idle-wait loop, canMoveToTile, normal_movement's "
     "walking loop, and launch's cleanup -- a generic step/tick sound "
     "tied to animation frames rather than any one specific event."),

    (0x153A0, "clear_text_row",
     "clears the current text row: sets text_x=0, writes 40 (28h) "
     "space characters via write_character. Uses whatever text_y the "
     "caller already set -- doesn't take/set a row parameter itself."),

    (0x1538F, "clear_caption_rows",
     "calls clear_text_row for text_y=17h(23) and 18h(24) -- the "
     "exact two text_y values used by the demo sequence's captions "
     "(BATTLE STRANGE CREATURES/etc, see pic??? writeup in "
     "file-formats.md). Clears old caption text before the next one."),

    (0x15EAC, "play_cannon_sound",
     "fixed play_tone_sweep params (ax=300h bx=1000h cx=2 dx=200h "
     "bp=8). Clearest calling context is fire's 'FIRE DIRECT-' "
     "sequence, right after the direction is confirmed and "
     "_playerTileId==24h (Ship, TileId 18 x2) -- the ship-cannon fire "
     "sound. Also called twice from end_of_turn in a less-clear "
     "context (possibly a shared boom/impact effect)."),

    (0x16F7F, "read_animated_digit_keypress",
     "same digit-prompt shape as read_digit_keypress (loops until "
     "'0'-'9', echoes via print_char, returns the numeric value) but "
     "calls animate_starfield every loop iteration so the starfield "
     "keeps moving while waiting -- used for hyperwarp's XENO=/YAKO=/"
     "ZABO= coordinate digit entry."),

    (0x17289, "speaker_off_once",
     "guards speaker_off behind an 'already silenced' flag "
     "(byte_16998): calls it and sets the flag only if not already "
     "set, so repeated calls only silence once. Used in "
     "hyperwarp-adjacent cleanup paths."),

    (0x16FA3, "draw_hyperwarp_hud",
     "displays 4 numbers via write_number: player._triLithium (fuel), "
     "then byte_178DA/178DB/178DC (the XENO=/YAKO=/ZABO= hyperwarp "
     "destination coordinates). The hyperwarp navigation HUD."),

    (0x12199, "check_monster_collision",
     "canMoveToTile's monster-blocks-movement check -- entirely "
     "bypassed if _negateTimeDuration is nonzero (jumps past the "
     "whole (_mapMonsters+60h)[di] monster-type dispatch), i.e. you "
     "can walk through monsters while Negate Time is active. "
     "Otherwise dispatches on monster type (0x2C/0x48/>=0x78 vs "
     "byte_1742F==0x5C) to decide collision behavior."),

    (0x1767A, "_negateTimeDuration",
     "confirmed via 3 sites: initialized to 0 in play_game alongside "
     "_flag3/_sleepFlag (paralysis/sleep duration counters), "
     "decremented once per turn in the exact same end_of_turn spot as "
     "those two, and set to 0x14 by negate_time (\"YOU RUB A "
     "COIN...\", spends _strangeCoin). While nonzero, "
     "check_monster_collision skips its monster-blocking check "
     "entirely -- the Negate Time spell/item effect's remaining-"
     "duration counter. Formerly _flag1, a plain global (not a "
     "Savegame struct member)."),

    (0x17030, "check_hyperwarp_sun_collision",
     "checks if all 3 hyperwarp destination coordinates "
     "(byte_178DA/178DB/178DC, XENO=/YAKO=/ZABO=) equal 4 -- if so, "
     "\"YOU HIT THE SUN!\" and clear_screen. Called from hyperwarp."),

    (0x1720B, "patch_map_filename",
     "patches byte offset 3 (the 'X' placeholder) of 3 file-loading "
     "templates (aMapxff/'MAPXFF', byte_122C9/'MONXFF', aTlkxff/"
     "'TLKXFF') with the character in AL -- dynamic map/monster/talk "
     "file selection (e.g. building 'MAP0FF' for a specific numbered "
     "planet/dungeon) rather than a fixed filename."),

    (0x17276, "speaker_on_once",
     "mirror of speaker_off_once, same guard flag (byte_16998) but "
     "opposite condition/action: only calls speaker_on if the flag is "
     "0FFh (already-silenced marker), then clears it."),

    (0x15FE0, "play_attack_sound",
     "fixed play_tone_sweep params (ax=400h bx=1000h cx=1 dx=200h "
     "bp=80h), called twice from attack (early in the sequence and "
     "again later) -- likely the weapon-swing sound, distinct from "
     "play_hit_sound which is specifically on a successful hit."),

    # -- Ultima II's dungeon first-person wireframe corridor view,
    # called from end_of_turn -- the "sub_16xxx cluster" flagged in an
    # earlier roadmap note as "dense, likely one subsystem, not yet
    # confirmed which". 4 structurally-confirmed pieces; the 10
    # individual wall-segment-drawing helpers sub_16144 dispatches to
    # (sub_16425/16594/1660C/166CB for the right wall, sub_16377/164E2/
    # 16291 for the left, sub_163CE/1653F/16304 for straight ahead) are
    # understood collectively (each draws a specific perspective wall
    # segment for a specific precomputed bit pattern) but not
    # individually pinned yet -- flagged as a follow-up in
    # docs/roadmap.md rather than forcing weak names onto all 10. --

    (0x16000, "render_dungeon_view",
     "top-level dungeon view step, called from end_of_turn. Always "
     "computes _tilePlayerCenter/_tilePlayerUp/_tilePlayerDown via "
     "get_dungeon_tile_at_player (needed for movement/combat logic "
     "regardless of rendering); if byte_17436 is set, also triggers "
     "the full corridor render (precompute_dungeon_corridor -> "
     "draw_dungeon_corridor -> draw_dungeon_monster). Note: reuses "
     "_mapLeft/_mapTop as scratch facing-direction deltas here, "
     "unrelated to their more common 'viewport scroll origin' "
     "meaning elsewhere -- same reused-global pattern as "
     "_circleDeltaX/Y in draw_world_map_overview."),

    (0x16078, "precompute_dungeon_corridor",
     "for each of 8 steps ahead along the facing direction "
     "(byte_17893/17894 = the reused _mapLeft/_mapTop deltas), reads "
     "3 dungeon tiles via get_dungeon_tile_at_player and builds 3 "
     "parallel 8-entry arrays: left-wall type ([di+48Fh], high "
     "nibble), right-wall type + monster-presence flag ([di+497h] "
     "high nibble / [di+4AFh] low 3 bits -- same low-3-bits monster "
     "flag as loc_11451's dungeon-cell accessor), and straight-ahead "
     "wall type ([di+49Fh] high nibble)."),

    (0x16144, "draw_dungeon_corridor",
     "the actual corridor renderer: clear_screen, then iterates depth "
     "0-7, checking bit patterns of the 3 precomputed wall-type bytes "
     "at each depth to dispatch to one of 10 wall-segment-drawing "
     "helpers (right wall: sub_16425/sub_16594/sub_1660C/sub_166CB; "
     "left wall: sub_16377/sub_164E2/sub_16291; straight ahead: "
     "sub_163CE/sub_1653F/sub_16304 -- not individually named yet, "
     "see docs/roadmap.md), stopping early once a wall blocks further "
     "view (sign bit set on the right-wall byte). Classic first-person "
     "wireframe dungeon crawler rendering."),

    (0x168ED, "draw_dungeon_monster",
     "runs after draw_dungeon_corridor: scans the precomputed "
     "monster-presence flags ([di+4AFh] from precompute_dungeon_"
     "corridor) up to whatever depth (byte_17892) the wall render "
     "loop stopped at, and if one's nonzero, looks up a screen "
     "position via byte_1697E[di] and draws a monster sprite from "
     "monsters_ptr at that depth in the corridor view."),

    # -- end_of_turn's random trap-encounter handlers. The item-flags
    # that gate these (_bootsOwned/_cloakOwned/_idolOwned) were already
    # named from tracing play_magic_sound/play_fail_sound's callers in
    # an earlier session, but the handler functions themselves were
    # never named -- see
    # docs/overview.md#player0xa30xa40xae--magical-item-protection-flags-traced --

    (0x10E70, "leg_paralysis_trap",
     "\"LEGS PARALIZED!\" -- flash_screen + play_magic_sound, "
     "_bootsOwned + rand_byte()>=0x40 (~75%) resists (\"SAVED BY "
     "MAGICAL BOOTS!\"), otherwise sets player_paralyzedFlag (0-15 "
     "random duration)."),

    (0x10EC2, "arm_paralysis_trap",
     "\"ARMS PARALIZED!\", same shape as leg_paralysis_trap but "
     "_cloakOwned-gated (\"SAVED BY MAGICAL CLOAK\") and sets _flag3 "
     "on failure instead -- the duration counter attack's "
     "\"PARALIZED!\" gate checks. Legs vs. arms is a real mechanical "
     "distinction: player_paralyzedFlag vs. _flag3 are different "
     "globals with different consequences (movement vs. combat)."),

    (0x10F12, "magic_missile_trap",
     "\"MAGIC MISSILE!\" -- flash_screen + play_magic_sound, no "
     "protection item, always wastes 2 turns (byte_17430/"
     "_commandWaitCtr both +2) and adds 0x40 (BCD) to byte_17432."),

    (0x10F48, "sleep_trap",
     "\"SLEEP SPELL!\", same shape as the paralysis traps but "
     "_idolOwned-gated (\"SAVED BY IDOL!\") and sets _sleepFlag on "
     "failure. proc far, not proc near -- missed by the initial "
     "sub_XXXXX sweep grep for this reason."),

    (0x10F8E, "minax_curse_trap",
     "\"MINAX CRIES: DIE FOOL!\" -- a cursed-tile encounter, called "
     "from canMoveToTile (not end_of_turn like the others above). "
     "Draws and erases a circle via xorDrawCircle (XOR flash "
     "bracketing pattern) then does 1 HP damage (BCD), death check on "
     "underflow."),

    (0x10FCD, "random_item_loss_trap",
     "end_of_turn random encounter, ~25% chance (rand_byte()<0x40): "
     "picks a random treasure-array slot (rand_byte()&0xF indexes "
     "player._ringOwned[]) and decrements it by 1 if owned -- an "
     "unprotected item-loss encounter, no resist mechanic like the "
     "paralysis/sleep traps above."),

    (0x112DB, "consume_food",
     "per-turn food consumption: decrements player._foodTurnCtr, and "
     "on borrow decrements the 2-byte player._food (BCD). Death check "
     "on underflow -- starving to 0 food kills the player. Called "
     "from end_of_turn."),

    (0x1143C, "check_monster_on_level",
     "gates map_get_monster_at?/loc_11451 by dungeon level: if "
     "byte_17435 (current depth) doesn't match the monster's "
     "(_mapMonsters+80h)[di] level field, returns 0FFh (not found on "
     "this level) instead of falling through to read its tile."),

    (0x1147F, "spawn_dungeon_monster",
     "wandering-monster spawner: loops all 32 monster slots looking "
     "for an inactive one ((_mapMonsters+60h)[di]==0), then assigns a "
     "random position (retrying via map_get_monster_at? if occupied), "
     "monster type, and display tile, writing both the monster-data "
     "arrays and the dungeon map tile itself. Called from end_of_turn."),

    (0x1152E, "compute_monster_direction_to_player",
     "sign(_mapX - _mapMonsters[di]) -> _circleDeltaX, sign(_mapY - "
     "(_mapMonsters+20h)[di]) -> _circleDeltaY (reused scratch "
     "direction-delta globals, same pattern as elsewhere in this "
     "sweep) -- the direction vector from a monster toward the "
     "player, monster AI movement targeting. Called from end_of_turn."),

    (0x126F9, "update_patrol_marker",
     "gated by player._disableSave. Cycles a map marker through 4 "
     "fixed waypoints (a table at cs:[+2798h]) every 8 turns "
     "(player+39h countdown, player.field_38 waypoint index 0/2/4/6), "
     "preserving/restoring the underlying terrain tile "
     "(player+3Ah) as it moves off/onto each waypoint, writing tile "
     "0C0h to mark its current position. Functional name -- possibly "
     "Minax's overworld movement per Ultima II lore, but not "
     "independently confirmed which entity this represents."),

    (0x1330C, "find_cursor_target_monster",
     "cast's monster-finder (already referred to this way in earlier "
     "docs/overview.md notes, never actually renamed): scans all 32 "
     "monster slots for one active, on the current dungeon level "
     "(byte_17435), and at the cursor-targeted position. Distinct "
     "from the already-named find_target_monster (facing-direction- "
     "based, used by attack/fire/offer/transact) -- this one is "
     "cursor-based and cast-only."),

    (0x1361C, "clear_picked_up_tile",
     "only runs above ground (player._mapNum2<4): reads the tile at "
     "_mapX/_mapY via get_player_tile then overwrites it with tile "
     "value 8 (plain ground). Called twice from get -- clears a "
     "picked-up item's tile off the overworld map."),

    (0x14BE0, "draw_sprite_row",
     "low-level sprite drawer: bp selects a byte mask from a "
     "glyph-indexed table (cs:[bp+48D4h]), ax/bx give screen X/Y -- "
     "same table-driven CGA addressing style as plot_point, but "
     "writes a full byte (not a 2-bit pixel) across 2 rows (di and "
     "di+50h) and both interleaved framebuffer banks per call. Only "
     "caller is draw_dungeon_monster_sprite."),

    (0x168A3, "draw_dungeon_monster_sprite",
     "called directly from draw_dungeon_monster: byte_1788F (a "
     "monster glyph ID) selects the sprite via bp, _mapOffsetX/"
     "_mapOffsetY (x4) give the screen position, then draw_sprite_row "
     "does the actual write -- the monster-sprite draw for the "
     "dungeon corridor view."),

    (0x14F11, "compute_monster_direction_scaled",
     "same _circleDeltaX/_circleDeltaY output as "
     "compute_monster_direction_to_player, but takes sign_byte() of a "
     "x4-scaled delta instead of the raw delta -- called from "
     "different sites (end_of_turn+D4, canMoveToTile-13A7) than the "
     "x1 version (end_of_turn+764). Exact semantic distinction from "
     "the x1 version not fully traced -- named for the structural "
     "difference (scaling) rather than an unconfirmed purpose."),

    (0x14F36, "compute_monster_delta",
     "same delta computation as the two direction helpers above "
     "(_mapX/_mapY minus a monster's position) but no sign_byte() "
     "call -- stores the raw signed difference in _circleDeltaX/Y "
     "instead of just its direction. Single caller, end_of_turn:"
     "loc_10AA7 -- likely feeds a distance/range check rather than a "
     "movement direction, not independently confirmed."),

    (0x15E3B, "play_trap_sound",
     "fixed play_tone_sweep params, called right after \"ARGH! A "
     "TRAP!\" (rand_byte() < byte_17435 -- deeper dungeon levels are "
     "more dangerous), clears byte_17436 (the dungeon-view-refresh "
     "flag render_dungeon_view checks) and pauses twice. A dungeon "
     "pit/trap encounter."),

    (0x16999, "setup_rocket_launch_display",
     "launch's post-rocket-launch setup: setPalette, positions the "
     "ship marker at screen center (0x80,0x40), loads the 3 "
     "destination coordinates from a table (indexed by "
     "player._disableSave, reused as scratch here), decrements "
     "player._triLithium (fuel), and starts drawing the FUEL=/XENO=/ "
     "YAKO=/ZABO= HUD labels (continues into unnamed code after the "
     "proc boundary)."),

    (0x16A79, "space_travel_command_loop",
     "space flight's main per-turn loop, parallel to play_game's "
     "overworld loop: draw_hyperwarp_hud, \"CMD: \" prompt, "
     "animate_starfield while waiting for a keypress, then dispatches "
     "on the key (NORTH_KEYCODE and others) to movement/hyperwarp "
     "commands."),

    (0x16D3E, "init_starfield",
     "seed_star_prng then initializes all 64 star positions via "
     "next_star_coord (X then Y, masked to 0-127, si=63 downto 0). "
     "Run once when entering the hyperwarp/space-travel view."),

    (0x17242, "play_star_twinkle_sound",
     "periodic chime scheduled from inside animate_starfield's "
     "countdown (word_16994): plays a short random-frequency tone via "
     "speaker_on_once/hold_tone, or silences via speaker_off_once if "
     "byte_16998 is already set -- the ambient twinkling-star sound "
     "during hyperwarp flight."),

    (0x172A0, "minax_death_sequence",
     "\"MINAX IS DEAD!! ALL HER WORKS SHALL DIE!\", called from "
     "attack+13E -- a victory/destruction animation: repeated "
     "play_hit_sound calls bracketing randomized _playerX/_playerY "
     "positions (function chunk continues past the proc boundary)."),

    # -- the dungeon-view wall-segment cluster, the last 12 unnamed
    # functions in the sub_XXXXX sweep. Precisely decoded (not
    # guessed) by cross-referencing draw_dungeon_corridor's bit-check
    # dispatch logic against the already-documented dungeon tile
    # format (docs/file-formats.md#dungeontower-format-mapx-number-ends-4-5:
    # 0x00 Floor, 0x10 Ladder up, 0x20 Ladder down, 0x30 both, 0x40
    # Chest/Tri-Lithium, 0x80 Wall, 0xC0 Door, 0xE0 Secret door -- all
    # multiples of 0x10 in the high nibble, sign bit set = Wall/Door/
    # SecretDoor, clear = Floor/Ladder/Chest). Every mapping below
    # cross-checked against each function's actual CODE XREF caller
    # site in draw_dungeon_corridor, not just the bit logic alone. --

    (0x16425, "draw_corridor_wall_segment",
     "right-position wall-type case (sign bit set: Wall/Door/Secret "
     "door). Also called unconditionally once before the render loop "
     "starts, as the initial depth-0 frame. Draws the converging "
     "perspective side-wall lines via draw_line, indexed by depth "
     "(byte_17892) through lookup tables."),

    (0x1648D, "draw_dungeon_door",
     "reached only when the right-position tile is specifically a "
     "Door (0xC0: sign bit set, 0x40 set, 0x20 clear) -- draws the "
     "door icon itself, after draw_corridor_wall_segment already drew "
     "the surrounding wall frame. Only caller: "
     "draw_dungeon_corridor:loc_16178."),

    (0x16594, "draw_ladder_down",
     "right-position tile has the 0x20 bit set (Ladder down 0x20, or "
     "Ladder up+down 0x30 -- which also triggers draw_ladder_up in "
     "that case). Draws the ladder-down icon via draw_line, calls "
     "draw_ladder_rail for the shared central-rail piece."),

    (0x1660C, "draw_ladder_up",
     "right-position tile has the 0x10 bit set (Ladder up 0x10, or "
     "Ladder up+down 0x30). Same shape as draw_ladder_down (different "
     "Y lookup tables), also calls draw_ladder_rail."),

    (0x166CB, "draw_chest_icon",
     "right-position tile is specifically a Chest (0x40: sign bit "
     "clear, 0x20 clear, 0x10 clear, 0x40 set). Draws the chest icon "
     "via draw_line."),

    (0x16684, "draw_ladder_rail",
     "shared sub-step called identically from both draw_ladder_down "
     "and draw_ladder_up: fixed screen-center X (7Fh/80h, same "
     "convention as plot_map_icon_point), draws the ladder's central "
     "rail via draw_line, depth-indexed lookup tables."),

    (0x16377, "draw_left_wall_segment",
     "left-position wall-type case (sign bit set: Wall/Door/Secret "
     "door) -- mirrors draw_corridor_wall_segment for the left probe "
     "position ([di+48Fh])."),

    (0x164E2, "draw_left_door",
     "reached only when the left-position tile is specifically a "
     "Door -- mirrors draw_dungeon_door for the left position."),

    (0x16291, "draw_left_open",
     "left-position tile has the sign bit clear (Floor/Ladder/Chest, "
     "i.e. nothing blocking) -- the 'open' counterpart to "
     "draw_left_wall_segment, no further bit dispatch (ladders/chests "
     "aren't distinguished on the side positions, only on the right)."),

    (0x163CE, "draw_ahead_wall_segment",
     "straight-ahead wall-type case (sign bit set: Wall/Door/Secret "
     "door) -- mirrors draw_corridor_wall_segment for the "
     "straight-ahead probe position ([di+49Fh])."),

    (0x1653F, "draw_ahead_door",
     "reached only when the straight-ahead tile is specifically a "
     "Door -- mirrors draw_dungeon_door for the ahead position."),

    (0x16304, "draw_ahead_open",
     "straight-ahead tile has the sign bit clear (Floor/Ladder/Chest) "
     "-- the 'open' counterpart to draw_ahead_wall_segment."),

    (0x17655, "_fileFCB",
     "renamed from _picData -- misleadingly named after one file type "
     "(PIC*) when it's actually the single shared FCB instance used "
     "for every access_file call regardless of file type (PLAYER/"
     "MAPXFF/MONXFF/TLKXFF/PIC* alike). Found while renaming the "
     "auto-named segments, 2026-08-18."),
]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
