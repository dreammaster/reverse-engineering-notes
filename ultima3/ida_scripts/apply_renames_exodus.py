"""
IDA Pro script: master list of symbol renames (functions + globals) for
EXODUS.BIN (DOS) -- Ultima III's overworld/combat/dungeon engine,
chained into from BOOTUP.BIN's handleJourneyOnward via an indirect jump
through a self-describing entry vector (see docs/overview.md). Companion
to apply_renames_ultima.py/apply_renames_bootup.py -- see those files'
docstrings for the full naming/DRY_RUN convention writeup. Run against
ultima_exodus.idb:

    .\\run_ida_script.ps1 -Idb ultima_exodus -ScriptName apply_renames_exodus.py

IMPORTANT: this executable's real entry point (0x124C4, the target of
BOOTUP.BIN's `JMP WORD PTR [1328h]`) was NOT recognized as code or a
function by IDA's own auto-analysis when this IDB was first created --
nothing in EXODUS.BIN's own bytes calls it, so IDA had no reason to
treat that address as anything but data. It had to be manually converted
to code (idc.plan_and_wait) and given a function boundary
(ida_funcs.add_func) before it could even be named -- done once,
directly, before this script existed; the entry here is just for the
changelog/idempotent-skip record.

This is a MUCH larger executable than ULTIMA.COM/BOOTUP.BIN (144
functions vs. 58/73, ~14,600 asm lines) and turns out to be the actual
game-world engine (overworld movement, towns, dungeons, combat, spells,
shops, temples, the ending sequence -- see the string table dump in
docs/overview.md's findings log). This first pass only covers the
shared low-level runtime (confirmed byte-for-byte identical in
structure to the same-named functions in ultima.idb/ultima_bootup.idb,
verified by reading through this whole cluster line-by-line, not
assumed from addresses alone) -- the game-specific logic (a handful of
genuinely massive functions like sub_17B54, the overworld command
dispatcher, spanning thousands of bytes) is a separate, much larger
follow-up pass. See docs/roadmap.md.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x124C4, "entryFromBootup",
     "the real entry point once BOOTUP.BIN's handleJourneyOnward chains "
     "in -- reached via an indirect jump through a vector at file "
     "offset 0x1228 (absolute 0x1328 once loaded over BOOTUP.BIN's "
     "memory), not by falling through to offset 0 the way ULTIMA.COM "
     "and BOOTUP.BIN's own entries work. Had no proc boundary at all "
     "until manually created (see this file's module docstring). Loads "
     "CHARSET.ULT/SHAPES.ULT/PARTY.ULT and continues into the game "
     "proper -- same boot-sequence shape as the other two executables' "
     "entry routines (same INT 23h/24h vector patching, same stack "
     "setup), but no decorative title-screen animation this time -- "
     "goes straight into gameplay."),

    # -- shared low-level runtime, byte-for-byte structurally identical
    # to the same-named functions in ultima.idb/ultima_bootup.idb --

    (0x14F3B, "writeString", "identical to the other IDBs' writeString."),
    (0x14F51, "writeStringPreserveCx", "identical."),
    (0x14F57, "swapCursorPos", "identical (xchg dx,word_12A90, this IDB's _textCursorPos)."),
    (0x14F5C, "writeCharacter", "identical."),
    (0x14F90, "drawCharGlyph", "identical."),
    (0x1504D, "plotPixel2bpp", "identical."),
    (0x15072, "drawTileGrid", "identical."),
    (0x150D3, "clearFramebuffer",
     "clears both CGA banks and resets the text cursor. In the other "
     "two IDBs this exact logic is an anonymous orphan block with no "
     "proc boundary; here it's a properly bounded function, so it gets "
     "a name for the first time -- generic descriptive name since no "
     "specific caller context narrows it further."),
    (0x150FD, "readLine", "identical."),
    (0x1514B, "stepTimeSeededPrng", "identical."),
    (0x15184, "swapAnimTableRows", "identical."),
    (0x151AA, "updateLogoAnimationA", "identical."),
    (0x151F9, "updateLogoAnimationB", "identical."),
    (0x15271, "updateLogoAnimationC", "identical."),
    (0x152B2, "updateLogoAnimationD", "identical."),
    (0x15311, "runIdleAnimationTick", "identical."),
    (0x15334, "pollKeypressAndAnimate", "identical."),
    (0x15347, "getKeypressAndWaitRaw", "identical."),
    (0x153A2, "printHexWord",
     "identical logic to the printHexWord found (as an orphan) in "
     "ultima.idb and (as a proper function) in ultima_bootup.idb -- "
     "here it's a proper function too."),
    (0x153AD, "printHexByte", "identical."),
    (0x153C6, "printHexNibble", "identical."),
    (0x153D8, "accumulateInputDigit", "identical."),
    (0x15462, "getMenuChoice", "identical to ultima_bootup.idb's getMenuChoice."),
    (0x154D9, "promptForNumberEntry", "identical to ultima_bootup.idb's promptForNumberEntry."),
    (0x1552A, "saveFile", "identical."),
    (0x1557A, "loadFile", "identical."),
    (0x155CD, "openFileWithRetry", "identical."),
    (0x15608, "printStringAt", "identical to ultima_bootup.idb's printStringAt."),
    (0x15611, "adjustAnimSpeed", "identical."),
    (0x15634, "drawLogoTileGrid",
     "identical mechanism to the other IDBs' drawLogoTileGrid, but its "
     "11x11 source tile-index buffer is `entryFromBootup` itself (the "
     "boot routine's own code bytes, reused as scratch data once it's "
     "run exactly once at startup) rather than a dedicated byte array -- "
     "a real memory-reuse trick worth remembering if entryFromBootup's "
     "own bytes ever look like they've been 'corrupted': they're "
     "deliberately being read as tile-index data after boot, not a "
     "disassembly error."),
    (0x1564F, "computeAnimTableByte", "identical."),
    (0x15674, "updateWindDisplay", "identical mechanism, verified via ida_bytes.get_word() against all 5 WIND_DIRECTION_TABLE entries."),
    (0x156D9, "playSoundEffect", "identical dispatcher."),
    (0x156C0, "_soundEnabled", "identical role."),
    (0x156C1, "SOUND_EFFECT_TABLE",
     "identical mechanism, verified via ida_bytes.get_word() against "
     "all 12 entries. Real address 0x156C1, NOT what IDA's own "
     "auto-generated array name would suggest -- same lesson as the "
     "other two IDBs' equivalent tables, don't trust an auto-name's "
     "numeric suffix as its address."),
    (0x156F9, "playToneFF", "identical."),
    (0x1571B, "playErrorBeep", "identical role (effect 0FEh) -- confirmed call sites: readLine, promptForNumberEntry, getMenuChoice, openFileWithRetry's 'Wrong Diskette!' prompt."),
    (0x1573D, "playToneFD", "identical."),
    (0x15796, "playToneFC", "identical."),
    (0x157BF, "playToneFB", "identical."),
    (0x157DA, "playToneFA", "identical."),
    (0x157F8, "playToneF9", "identical."),
    (0x15802, "playToneF8", "identical."),
    (0x15828, "playToneF7", "identical."),
    (0x15832, "playToneF6", "identical."),
    (0x1585E, "playToneHelper", "identical -- shared helper for playToneF7/playToneF6, not itself in SOUND_EFFECT_TABLE."),
    (0x1588E, "playToneF5", "identical."),

    (0x12AAD, "aCalmWind", "'\\x10Calm  Wind\\x11\\x00' -- wind-direction display string."),
    (0x12ABA, "aNorthWind", "'\\x10North Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AC7, "aSouthWind", "'\\x10South Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AD4, "aEastWind", "'\\x10East  Wind\\x11\\x00' -- wind-direction display string."),
    (0x12AE1, "aWestWind", "'\\x10West  Wind\\x11\\x00' -- wind-direction display string."),
    (0x1566A, "WIND_DIRECTION_TABLE",
     "identical mechanism to the other IDBs' WIND_DIRECTION_TABLE, "
     "same Calm/North/East/South/West selection order -- verified via "
     "ida_bytes.get_word() against all 5 entries, not assumed."),

    # -- first game-specific function identified, found while looking
    # for the combat command dispatcher the string table pointed at --

    (0x123A5, "updateMonsterAI",
     "CORRECTED from an earlier session note: this is not a IDA-chunk-"
     "merge artifact unrelated to combat -- it's one coherent function "
     "whose per-turn monster movement logic falls straight through "
     "into a full combat encounter (see beginCombatEncounter below) "
     "exactly when a monster's move lands it on _partyPosition (the "
     "`cmp dx, word_115CC` / `jmp loc_188AA` branch in this function's "
     "own main body -- a real, direct control-flow edge, not a "
     "disjoint far-jump artifact). Main-body role: iterates 32 slots "
     "(si=0x1F downto 0) over 4 parallel byte arrays at fixed offsets "
     "from a shared base -- +0x1280 (monster type, 0=empty slot), "
     "+0x12A0 (display tile), +0x12C0 (X position), +0x12E0 (Y "
     "position) -- same parallel-array convention as ultima2's monster "
     "tracking (not an array-of-structs). For each active slot: rolls "
     "movement chance via stepTimeSeededPrng/adjustAnimSpeed (gated by "
     "byte_114BC/_gameMode and the monster's own type-derived movement "
     "class read from [si+1300h]), computes a candidate new position, "
     "checks it via getMapTileAt+canMoveToTile, and either updates "
     "position and redraws the tile, or -- if the new position is "
     "_partyPosition -- falls into beginCombatEncounter instead. "
     "Monster types 't'(0x74)/'<'(0x3C) get special-cased into "
     "sub_1232F -- not yet identified, possibly a specific monster's "
     "unique behavior (thief steal? merchant?)."),

    (0x128DF, "getMapTileAt",
     "LOW-MEDIUM CONFIDENCE: takes a packed coordinate in bx, computes "
     "`(bx>>2) + 0x100`-ish offset and returns the byte there in al -- "
     "i.e. returns a TILE VALUE (not a pointer, despite the "
     "'get X address' shape of similar-looking helpers in other "
     "clusters). Bit-level derivation from the packed coordinate not "
     "fully worked out; named for its evident role (read the map at a "
     "position) in updateMonsterAI's call context, not independently "
     "verified against a known map buffer layout."),

    (0x17AEA, "readDirectionKeypress",
     "waits for an arrow key (scan codes 0x48/0x50/0x4D/0x4B = Up/"
     "Down/Right/Left, printed as North/South/East/West) or Escape "
     "(cancels, ZF set on return via `test dx,dx`), computes and "
     "returns the resulting position (_partyPosition + direction "
     "delta, wrapped `AND 3F3Fh` -- confirms the 64x64 overworld map "
     "wraparound, matching SOSARIA.ULT's documented 64x64 size). "
     "Called from sub_17B54 (overworld) and from updateMonsterAI's "
     "combat-flee handling."),

    # -- CONFLICT/COMBAT ENCOUNTER SYSTEM: a major discovery. What
    # looked like unrelated "function chunks" merged into updateMonsterAI
    # turns out to be a single coherent control flow: updateMonsterAI's
    # main per-turn loop falls through into full combat-encounter logic
    # exactly when a monster's move brings it onto the player's tile
    # (the `cmp dx, word_115CC` / `jmp loc_188AA` branch in
    # updateMonsterAI's own body -- word_115CC is _partyPosition, so
    # this is a real "monster reached the player" trigger, not a
    # disassembly merge artifact as first suspected). Confirms
    # word_115CC as the current overworld/dungeon position and
    # byte_184E1 as which of the 4 party slots is acting. --

    (0x115CC, "_partyPosition",
     "packed (Y<<8|X) current position, read/written throughout "
     "updateMonsterAI's combat-trigger check and readDirectionKeypress. "
     "Wrapped `AND 3F3Fh` on update -- 64x64 map, matching SOSARIA.ULT."),

    (0x184E1, "_currentCombatant",
     "0-based index (0-3) of which party slot is taking its turn "
     "during a combat encounter -- indexes the 0x40-byte-per-character "
     "combat working copies at `[bx*40h + 14CCh]`, same RosterEntry-"
     "compatible layout as ROSTER.ULT/PARTY.ULT (confirmed: reads "
     "`[si+30h]`/`[si+31h]` for weapon index/quantity, `[si+27h]` for "
     "the Powders count spent by Negate Time -- exact offsets match "
     "docs/file-formats.md's ROSTER.ULT layout)."),

    (0x184E2, "_conflictMonsterClass",
     "derived monster-type value (raw type >>2) used to select which "
     "CNFLCT_*.ULT arena file to load for a combat encounter -- see "
     "beginCombatEncounter's arena-selection decision tree, which maps "
     "specific values/ranges of this to all 9 arena files (A/B/C/F/G/"
     "M/Q/R/S)."),

    (0x115D0, "_negateTimeDuration",
     "turn-count duration counter, set to 0x0A by the Negate Time "
     "combat command (combatCmdNegateTime) and cleared to 0 in "
     "combatAdvanceTurn under some condition -- same role/name as "
     "ultima2's identically-purposed global (that project's Negate "
     "Time spell also uses a plain duration-counter global, not a "
     "struct field)."),

    (0x188AA, "beginCombatEncounter",
     "HIGH CONFIDENCE, extensively traced: reached from "
     "updateMonsterAI when a monster's move lands on _partyPosition. "
     "Prints 'Conflict!!', then a decision tree on "
     "_conflictMonsterClass (plus byte_114BA, a map-type/context flag) "
     "selects and loads one of all 9 CNFLCT_*.ULT arena files via "
     "loadFile, saves the current game mode (byte_158CB) and sets "
     "gameMode=0x80 (combat), copies each living party member "
     "(Status 'G'/'P') into per-slot combat display data (position, "
     "weapon glyph via lookupWeaponGlyph, tile via "
     "computeAnimTableByte) or marks dead slots invisible (0xFF "
     "position), randomly places monsters into the 11x11 arena "
     "avoiding collisions (stepTimeSeededPrng), draws the arena "
     "(drawLogoTileGrid) and plays the combat-start sound sequence, "
     "then falls into combatTurnLoop."),

    (0x18AC4, "lookupWeaponGlyph",
     "LOW-MEDIUM CONFIDENCE: looks up a weapon-index byte in a "
     "10-entry table (byte_18625) via linear scan, returns a matched "
     "display character or '?' (0x3F) if not found -- used by "
     "beginCombatEncounter to pick each party member's on-screen "
     "combat glyph based on their readied weapon."),

    (0x18AE6, "printCombatReactionMessage",
     "LOW CONFIDENCE: prints one of several canned reaction phrases "
     "('es'/'s' suffix combinations, 'Thieves') depending on a code in "
     "al/ah and _conflictMonsterClass -- called once during "
     "beginCombatEncounter's setup (al=0FFh) and again later in the "
     "attack-resolution chunk. Exact trigger conditions and full "
     "phrase set not traced -- looked at only enough to place its "
     "role, not its complete behavior."),

    (0x18B52, "combatTurnLoop",
     "per-player combat turn iteration: for each party slot 0-3 "
     "(_currentCombatant), checks the slot is a live, present "
     "combatant (sub_16C14 -- not yet identified, returns ZF), prints "
     "'----Player N', then falls into the per-turn key-read/dispatch "
     "sequence (readCombatCommandKey -> COMBAT_COMMAND_TABLE)."),

    (0x18BD5, "readCombatCommandKey",
     "reads a letter key (uppercased), looks it up in a 33-entry table "
     "(byte_18579 -- presumably the letters shown in the 'F, G, E, W, "
     "A:'/aFcwtpblidardir-style prompts, matching the number of "
     "COMBAT_COMMAND_TABLE cases exactly) via linear scan, and jumps "
     "into COMBAT_COMMAND_TABLE at the matching case -- the combat "
     "menu's actual key dispatcher."),

    (0x18BF8, "COMBAT_COMMAND_TABLE",
     "33-entry (0-32) jump table for the combat per-player command "
     "menu, indexed by readCombatCommandKey. Confirmed cases: 1=Pass, "
     "2-5=movement/flee (North/South/East/West, via "
     "combatHandleMovement), 9=Ready a weapon, 10=Ztats, 11=Negate "
     "Time, 12=Cast Spell, 13=Attack (the large damage-resolution "
     "case), 0 and 14-32=Not usable cmd (catch-all) -- cases 6-8 tail-"
     "jump elsewhere via nested jump tables (loc_11BD1/loc_18389, not "
     "traced further this pass)."),

    (0x18BFC, "combatAdvanceTurn",
     "post-command cleanup: re-checks Negate Time expiry "
     "(clears _negateTimeDuration if sub_15B28 -- not yet identified "
     "-- signals it should), redraws the arena and HUD (drawLogoTileGrid "
     "+ sub_16CC3), advances _currentCombatant, and either loops back "
     "into combatTurnLoop for the next player or exits combat "
     "(loc_1914F/loc_18FB6 -- combat-end paths, not yet traced)."),

    (0x18C3C, "combatHandleMovement",
     "combat commands 2-5 (North/South/East/West): computes the "
     "current combatant's new position (from their combat-display X/Y "
     "at `[si+0A0h]/[si+0A4h]`) and checks it against the 11x11 arena "
     "boundary (`cmp bl/bh, 0Bh`) -- confirms the CNFLCT arena's "
     "documented 11x11 dimension exactly. Movement-in-combat / flee-"
     "attempt handling; full resolution not traced past the boundary "
     "check this pass."),

    (0x18CF1, "combatCmdReady", "'Ready a weapon!' -- combat command 9, calls sub_17E48 (not yet identified, likely the same weapon-selection prompt as the overworld 'ready' command)."),
    (0x18D0B, "combatCmdZtats", "'Ztats' -- combat command 10, shows a party member's stats (calls sub_171ED and sub_16DC1, not yet identified) mid-combat."),
    (0x18D38, "combatCmdCastSpell", "'Cast Spell!' -- combat command 12, sets byte_184E0='<' and calls sub_15D83 (the spellcasting function, confirmed via its own 'Not a mage!'/'Cleric spell-'/'Wizard spell-' strings -- not yet identified/renamed itself, high-value next target)."),
    (0x18D57, "combatCmdNegateTime", "'Negate Time!' -- combat command 11, spends 1 Powder (_currentCombatant's RosterEntry-offset +0x27, BCD-decremented) and sets _negateTimeDuration=0x0A; falls through to the 'no Powders left' path (jumps to the shared aNoneLeft/'<-What?' error trampoline at loc_17DA8, owned by sub_17B54) if the count is already 0."),
    (0x18D83, "combatCmdPass", "'Pass' -- combat command 1, the simplest case: just advances the turn."),
    (0x18D8D, "combatCmdInvalid", "'Not usable cmd!' -- combat commands 0 and 14-32 (the catch-all), beeps (playErrorBeep) and re-prompts."),
    (0x18D9F, "combatCmdAttack",
     "'Attack' -- combat command 13, by far the largest case: prompts "
     "a direction (aAttackDir, ' Attack\\nDir-'), computes the target "
     "square, and (unless the readied weapon is one of 4 special "
     "indices needing a direct hit -- values 3/5/9/0Dh, not yet "
     "identified which weapons those are) calls combat-resolution "
     "helpers (sub_18E46/sub_18E7A, not yet identified) that appear to "
     "check for a monster or player at the target square and apply "
     "damage/consequences, decrementing ammo-style weapon quantities "
     "(`[si+31h]`) when they hit 0. Full damage-formula tracing "
     "deferred -- this pass only confirms the overall shape."),

    # -- spellcasting system, found while tracing combatCmdCastSpell --

    (0x126A9, "printGameText",
     "HIGH CONFIDENCE: the game's main text-output routine, distinct "
     "from writeString/writeCharacter -- adds automatic word-wrap and "
     "scrolling within a fixed text window (checked against "
     "_textCursorPos vs. row>=0x11/col>=0x18 boundaries), with special "
     "handling for embedded newlines (0x0Ah) and apostrophes (0x27h, "
     "checked specially, likely for word-boundary wrap logic). "
     "Despite being attributed to sub_17B54's function-chunk list (an "
     "artifact of IDA's chunk-ownership analysis, not a semantic "
     "claim), this is called pervasively from nearly every subsystem "
     "traced so far (combat, spells, menus) as THE way to print "
     "any in-game message -- this is the real workhorse text printer, "
     "not the lower-level writeString."),

    (0x15D16, "readSpellLetterKey",
     "thin wrapper over getKeypressAndWaitRaw that uppercases a-z -- "
     "used by castSpell for both the Wizard/Cleric type choice and the "
     "actual spell-letter selection."),

    (0x15D2B, "healHitPoints",
     "adds al (BCD) to a character's _hitPoints (word at +0x1A/+0x1B, "
     "with carry into the high byte), then clamps to _maxHitPoints "
     "(+0x1C/+0x1D) if it would exceed it -- confirms both RosterEntry "
     "offsets exactly against real arithmetic, not just display code."),

    (0x15D4B, "addExperienceClamped",
     "adds al (BCD) to _experience (+0x1E/+0x1F), clamping to 9999 on "
     "overflow (`jnb` after the carry-adjust, then hardcodes 9999h) -- "
     "confirms the _experience offset."),

    (0x15D67, "addGoldClamped",
     "adds al (BCD) to _gold (+0x23/+0x24), same 9999-clamp pattern as "
     "addExperienceClamped -- confirms the _gold offset."),

    (0x15D83, "castSpell",
     "'Cast Spell!' entry point (also reachable from combat via "
     "combatCmdCastSpell). Reads the caster's _class (+0x17): Druid/"
     "Ranger ('D'/'R') get to choose Wizard or Cleric spells "
     "('Spell type W/C-'); Cleric/Paladin/Illusionist ('C'/'P'/'I') "
     "are locked to Cleric spells; Wizard/Lark/Alchemist ('W'/'L'/'A') "
     "are locked to Wizard spells; anyone else gets 'Not a mage!'. "
     "Prompts a spell letter (A-P, up to 16 spells per type) via "
     "readSpellLetterKey, computes a BCD magic-point cost "
     "(`(index+1)*5`, BCD-packed via `aam`), checks it against the "
     "caster's _magicPoints (see the new RosterEntry field below; "
     "'M.P. too low!' if insufficient), deducts the cost (BCD "
     "subtraction), prints the spell's name via printGameText, then "
     "dispatches to the specific spell's effect handler through a "
     "per-type jump table (WIZARD_SPELL_TABLE/CLERIC_SPELL_TABLE) -- "
     "the individual spell effects themselves are a natural next "
     "target, one function per spell, now that the dispatch mechanism "
     "is understood."),

    (0x1594B, "WIZARD_SPELL_TABLE",
     "8-entry(ish) jump table of individual Wizard spell-effect "
     "handlers, indexed by castSpell -- entries not yet individually "
     "traced."),

    (0x1596B, "CLERIC_SPELL_TABLE",
     "Cleric-spell counterpart to WIZARD_SPELL_TABLE, same dispatch "
     "mechanism -- entries not yet individually traced."),

    (0x12228, "canMoveToTile",
     "LOW CONFIDENCE, structural guess: takes getMapTileAt's tile "
     "value in al plus a monster-slot context (si), returns al=0xFFh "
     "for passable / 0 for blocked after checking specific tile-type "
     "codes (4/8/0xCh/0x20h look like passable-terrain IDs) and "
     "delegating to sub_17F96 (not identified) for slot types 4 and "
     "0x2C-0x40. Checked 3 times per movement attempt in "
     "updateMonsterAI (straight/horizontal/vertical probes) -- a "
     "movement-legality check almost certainly, exact tile-code "
     "meanings not independently confirmed."),
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
    if not RENAMES:
        print("[no-op] RENAMES is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
