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
