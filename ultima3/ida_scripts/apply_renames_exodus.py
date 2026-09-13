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

    # -- party status/turn-processing helpers, found while chasing
    # combat's supporting cast --

    (0x16C14, "isCharacterAlive",
     "identical check to ultima_bootup.idb's isCharacterAlive "
     "(_status == 'G' or 'P') -- same name reused for the same "
     "confirmed logic, per convention."),

    (0x16C29, "checkPartyWipedOut",
     "scans all 4 party slots via isCharacterAlive; if none are alive, "
     "prints 'All Players Out!' and jumps to loc_17252 (not yet "
     "identified -- presumably a game-over sequence)."),

    (0x16CC3, "drawPartyStatusBar",
     "draws the 4-row party status display (Status/Sex/Race/Class "
     "letters, ' M:'+_magicPoints, ' L:'+a level-like byte at +0x1Fh "
     "displayed as value+1 clamped to 99, ' '+_hitPoints, +_food) for "
     "each present party slot -- called both in combat and from "
     "entryFromBootup, so this is the general-purpose status bar, not "
     "combat-specific. **Open discrepancy, not resolved this pass**: "
     "displays +0x1Fh as a standalone 'Level' byte, but "
     "ultima_bootup.idb's RosterEntry struct has +0x1E/+0x1F defined "
     "as a single 2-byte _experience word (from showCharacterDetails's "
     "printHexWord(ax=[bx+1Eh]) call) -- these can't both be right as "
     "written. Left unresolved rather than silently picking one; see "
     "docs/roadmap.md."),

    (0x16FDF, "processPartyTurnEffects",
     "per-turn party upkeep, gated by two countdown timers "
     "(byte_164A2/byte_164A3): for each living party member "
     "(isCharacterAlive), regenerates 1 Magic Point "
     "(regenerateMagicPoint) if below a class-appropriate attribute "
     "threshold -- Wizard capped by _intelligence, Cleric by _wisdom, "
     "Lark/Druid/Alchemist also using _intelligence (matching "
     "castSpell's Wizard-type class grouping) -- and (per this "
     "function's own 'Poisoned!'/'Starving!'-adjacent string table "
     "references, not fully traced this pass) presumably applies "
     "poison/hunger damage-over-time too."),

    (0x171ED, "invertCharacterCell",
     "XORs one on-screen character cell's pixels (same interleaved-"
     "bank CGA addressing as drawCharGlyph/plotPixel2bpp) -- a "
     "highlight-toggle primitive, used e.g. by combatCmdZtats to "
     "highlight the selected player's row."),

    (0x17136, "regenerateMagicPoint",
     "increments a living character's _magicPoints by 1 (BCD) -- the "
     "actual regen step processPartyTurnEffects gates by attribute."),

    # -- THE OVERWORLD MAIN GAME LOOP AND COMMAND TABLE: found while
    # looking for sub_17B54's own command structure (as opposed to
    # combat's, already identified above). This is the top-level "wait
    # for a command, dispatch it" loop that drives the entire game
    # outside combat -- confirms _partyPosition/_currentTransport and a
    # first handful of the 33 overworld commands. Full table documented
    # in docs/overview.md; most of the 33 handlers still need their own
    # pass (see docs/roadmap.md) -- this entry only covers what a first
    # read-through confirmed with real evidence. --

    (0x114BA, "_currentTransport",
     "0x3F = on foot, else a vehicle/mount type index -- matches "
     "docs/file-formats.md's externally-sourced PARTY.ULT 'Transport' "
     "field exactly (0xA=horse, 0xB=ship, 0x3F=foot documented there; "
     "this pass independently confirms the 0x3F=foot convention via "
     "cmdExitVehicle's own logic, not just the external doc)."),

    (0x11887, "OVERWORLD_COMMAND_KEYS",
     "33-entry word array, each a full (scancode:char) AX value "
     "compared via `repne scasw` against the raw getKeypressAndWaitRaw "
     "result -- NOT a byte array of characters, which is why IDA's "
     "auto-analysis originally misdecoded this range as garbage x86 "
     "instructions (fixed via ida_scripts/fix_command_key_tables.py). "
     "Parallel index-for-index with OVERWORLD_COMMAND_TABLE. Verified "
     "via ida_bytes.get_word() against all 33 entries: 4 arrow keys "
     "(Up/Down/Right/Left, scancodes 0x48/0x50/0x4D/0x4B) + Space + "
     "all 26 letters A-Z + 2 additional scancode-only entries (0x1F, "
     "0x2F, char 0 -- likely Alt+S/Alt+V or similar unshifted "
     "combinations, both routed to the same handler as 'V')."),

    (0x1190B, "OVERWORLD_COMMAND_TABLE",
     "33-entry jump table for the main overworld command loop, real "
     "address 0x1190B -- NOT 0x11BD1 as IDA's own auto-generated name "
     "implies (same lesson as every other table in this project: "
     "verify via ida_bytes.get_word(), never trust a name's numeric "
     "suffix). Parallel to OVERWORLD_COMMAND_KEYS. See "
     "docs/overview.md for the full 33-entry letter-to-handler table "
     "and which handlers are confirmed vs. still `loc_XXXXX`."),

    (0x11B7A, "mainGameLoop",
     "the top-level 'wait for a command, dispatch it, repeat' loop: "
     "checkPartyWipedOut, draws a cursor glyph, polls for a keypress "
     "with a ~5-second idle timeout (drives idle animation via "
     "sub_17347 while waiting -- not yet identified), uppercases the "
     "key, looks it up in OVERWORLD_COMMAND_KEYS via repne scasw, and "
     "jumps into OVERWORLD_COMMAND_TABLE at the matching index (or the "
     "shared aWhat/'<-What?' invalid-command trampoline at loc_17DBA "
     "if not found)."),

    (0x11BD5, "mainLoopCommandDone",
     "post-command landing pad: if in combat mode, jumps straight to "
     "combatAdvanceTurn instead of continuing the overworld loop; "
     "otherwise processPartyTurnEffects, re-derives the tile under the "
     "party (getMapTileAt) and dispatches a couple of tile-triggered "
     "side effects (0x88 -- not yet identified, calls sub_15B51/"
     "sub_120AE) before mainGameLoop repeats."),

    (0x11C7D, "cmdMoveNorth",
     "Up arrow (index 0): decrements _partyPosition's Y with 64-wide "
     "wraparound, after checking sub_17233/sub_17254 (not yet "
     "identified -- presumably movement-blocked checks) don't veto it."),
    (0x11C9E, "cmdMoveSouth", "Down arrow (index 2): same shape as cmdMoveNorth, increments Y."),
    (0x11CBF, "cmdMoveEast", "Right arrow (index 3): same shape as cmdMoveNorth, increments X."),
    (0x11CE0, "cmdMoveWest", "Left arrow (index 4): same shape as cmdMoveNorth, decrements X."),

    (0x11C77, "cmdPass",
     "Space (index 5): the simplest overworld command -- prints the "
     "'Pass' message and ends the turn, no other logic."),

    (0x11D01, "cmdBoard",
     "'B' (index 6): checks the tile at _partyPosition for a horse "
     "marker (0x28, 'Mount Horse!' -> _currentTransport=0x0A) or ship "
     "marker (0x2C, 'Board Frigate!' -> _currentTransport=0x0B) -- "
     "confirms _currentTransport's horse/ship values exactly matching "
     "the external PARTY.ULT documentation (0xA=horse, 0xB=ship)."),

    (0x11D69, "cmdEnter",
     "'E' (index 11) -- MAJOR FINDING, fully traced: THE location-"
     "entry command for shrines, dungeons, towns, AND castles. "
     "Shrine branch (gameMode==0xFF, tile==0xF8): calls sub_16366 "
     "(confirmed via its own aShrineImg/aShrineWhoEnter strings). "
     "Otherwise (gameMode==0, on the overworld): looks up "
     "_partyPosition in LOCATION_TILE_TABLE (19 entries) to get an "
     "index into LOCATION_TYPE_TABLE, which holds the location kind "
     "(5=Dungeon, 6=Towne, 7=Castle -- other values presumably other "
     "kinds, not enumerated this pass). On match: saves the overworld "
     "position (_savedOverworldPosition) for later return, sets the "
     "starting in-location position (Dungeon: 0x0101, Towne: 0x2001, "
     "Castle: 0x3E20) and gameMode (1/2/3 respectively), prints "
     "'Dungeon!'/'Towne!'/'Castle!', then loads the location's data: "
     "for towns/castles, a 0x1228-byte (4648, matching SOSARIA.ULT/"
     "town-map size exactly) file into the `start` buffer (the reused-"
     "after-boot code-space trick, same as drawLogoTileGrid's tile "
     "data); for dungeons, BOTH a 0x890-byte (2192, matching the "
     "documented dungeon file size exactly) map into byte_10900 AND a "
     "separate 0x800-byte read of DUNGEON.DAT into `start` -- "
     "confirms DUNGEON.DAT is a distinct auxiliary file loaded "
     "alongside a dungeon's own numbered map file, not the map itself."),

    (0x116E1, "LOCATION_TILE_TABLE",
     "19-entry word array of overworld tile-position values, scanned "
     "by cmdEnter via repne scasw against _partyPosition to find which "
     "of the ~19 named locations (dungeons/towns/castles) the party is "
     "standing on."),

    (0x1259D, "_locationTypeTable",
     "MEDIUM CONFIDENCE: byte, indexed by cmdEnter's LOCATION_TILE_TABLE "
     "match, holding the location kind at that position (5=Dungeon, "
     "6=Towne, 7=Castle -- other values not enumerated this pass). "
     "Named as a scalar-looking single access, but given "
     "LOCATION_TILE_TABLE's 19 entries this may actually be a 19-byte "
     "PARALLEL ARRAY rather than a single byte -- not fully confirmed, "
     "worth double-checking (compare its access pattern against "
     "LOCATION_TILE_TABLE's index, `bx`, rather than a fixed address) "
     "before trusting the current scalar treatment."),

    (0x114C2, "_savedOverworldPosition",
     "the overworld _partyPosition, saved by cmdEnter right before "
     "entering a dungeon/town/castle so it can be restored on exit."),

    # -- THE DUNGEON MAIN LOOP AND ITS COMMAND TABLE: found while
    # investigating jpt_18389, referenced repeatedly throughout this
    # session's finds. Parallel in every way to mainGameLoop/
    # OVERWORLD_COMMAND_TABLE, confirmed via the exact same structural
    # shape (idle-animation poll, key-table lookup via repne scasw,
    # jump-table dispatch) PLUS the "It's dark!" message tying directly
    # to cmdIgniteTorch's own flag -- conclusive, not just structural
    # similarity. --

    (0x18314, "initDungeonState",
     "entered via cmdEnter's dungeon branch right after loading the "
     "dungeon map + DUNGEON.DAT. Resets the lit-torch flag "
     "(byte_115CE=0) and byte_115CF=0 (a second dungeon-state byte, "
     "not yet identified -- possibly a darkness/depth counter), calls "
     "sub_162FD/sub_128C6 (not yet identified), falls into "
     "dungeonMainLoop."),

    (0x18327, "dungeonMainLoop",
     "HIGH CONFIDENCE: the dungeon counterpart to mainGameLoop, "
     "confirmed both structurally (identical idle-poll/key-lookup/"
     "dispatch shape) and semantically -- checks byte_115CE (the "
     "exact flag cmdIgniteTorch sets) and prints 'It's dark!\\n' every "
     "iteration if unlit, the classic Ultima dungeon "
     "need-a-lit-torch-to-see mechanic. Dispatches through "
     "DUNGEON_COMMAND_TABLE via DUNGEON_COMMAND_KEYS instead of the "
     "overworld's tables."),

    (0x1774A, "DUNGEON_COMMAND_TABLE",
     "33-entry jump table for the dungeon command loop, real address "
     "0x1774A (verified via ida_bytes.get_word(), not its "
     "auto-generated name's implied 0x18389). Shares many handlers "
     "directly with OVERWORLD_COMMAND_TABLE (Pass, Cast Spell, "
     "Ignite Torch, Exchange, Toggle Sound, and several still-"
     "unnamed ones) -- these commands work identically in both "
     "contexts. CORRECTED from an earlier note in this same session, "
     "which misread a buggy exploratory dump: indices 16-24 AND 29 "
     "(10 letters total: B, A, E, F, L, Q, T, U, X, S) share ONE "
     "disabled-command stub (cmdDisabledInDungeon) -- Board/Enter/"
     "eXit-vehicle/Quit-and-save/Steal disabled underground all make "
     "immediate sense; Attack/Fire/Locate/Transact/Unlock being "
     "disabled here too is still worth double-checking against actual "
     "gameplay. The remaining 6 entries (K, D, both arrow-turn keys, "
     "both arrow-move keys) are the genuinely dungeon-specific "
     "handlers -- confirmed (not guessed) as cmdKlimb/cmdDescend/"
     "cmdTurnLeft/cmdTurnRight/cmdMoveForward/cmdMoveBackward, a "
     "classic first-person relative-turn dungeon movement system, "
     "distinct from the overworld's compass-direction movement. See "
     "each one's own entry below."),

    (0x158CC, "_facingDirection",
     "0-3, the party's current facing while in a dungeon (classic "
     "Ultima first-person dungeon navigation). Incremented/decremented "
     "mod 4 by cmdTurnRight/cmdTurnLeft; indexes a 4-entry (dx,dy) "
     "delta table (_dungeonFacingDeltaX/Y below) for cmdMoveForward/"
     "cmdMoveBackward."),

    (0x115CF, "_dungeonLevel",
     "current dungeon depth, incremented by cmdDescend and "
     "decremented by cmdKlimb -- matches docs/file-formats.md's "
     "documented dungeon format (levels stored sequentially, depth "
     "increases with level number)."),

    (0x176F0, "_dungeonFacingDeltaX",
     "4-entry byte array, X delta for each of the 4 facing directions "
     "-- indexed by _facingDirection in cmdMoveForward/cmdMoveBackward."),

    (0x176F4, "_dungeonFacingDeltaY",
     "4-entry byte array, Y delta counterpart to "
     "_dungeonFacingDeltaX."),

    (0x183F7, "cmdKlimb",
     "'K' (dungeon index 25): checks the current tile for the Ladder-"
     "up bit (0x10, matching docs/file-formats.md's dungeon tile "
     "encoding exactly), and if present, decrements _dungeonLevel "
     "(guarded by a nonzero check) and calls sub_16458 -- classic "
     "Ultima 'Klimb' a ladder up one level."),

    (0x1841C, "cmdDescend",
     "'D' (dungeon index 26): checks the current tile for the Ladder-"
     "down bit (0x20, matching docs/file-formats.md exactly), and if "
     "present, increments _dungeonLevel -- 'Descend' a ladder down one "
     "level."),

    (0x18438, "cmdTurnRight",
     "Right arrow (dungeon index 27): checks the current tile isn't a "
     "specific blocking type (`cmp al,0A0h; jnb`), then rotates "
     "_facingDirection clockwise mod 4 -- first-person turn right, "
     "does not change position."),

    (0x18455, "cmdTurnLeft",
     "Left arrow (dungeon index 28): mirror of cmdTurnRight, rotates "
     "_facingDirection counterclockwise mod 4."),

    (0x18472, "cmdDisabledInDungeon",
     "10 letters (dungeon indices 16-24 and 29: B, A, E, F, L, Q, T, "
     "U, X, S) share this stub -- beeps (playErrorBeep) and falls "
     "through to the invalid-command path. The dungeon counterpart to "
     "cmdDisabledOnSurface."),

    (0x1847D, "cmdMoveForward",
     "Up arrow (dungeon index 30): computes the tile one step ahead "
     "using _facingDirection to index _dungeonFacingDeltaX/Y, checks "
     "it isn't a Wall (tile==0x80, matching docs/file-formats.md's "
     "dungeon tile encoding exactly), and if clear, updates "
     "_partyPosition -- first-person move forward, the classic Ultima "
     "dungeon-crawler movement primitive."),

    (0x184A9, "cmdMoveBackward",
     "Down arrow (dungeon index 31): mirror of cmdMoveForward using "
     "_facingDirection+2 (mod 4, i.e. facing directly behind), checks "
     "the sign bit of the tile byte (`rol al,1; jb` -- the same Wall/"
     "Door/Secret-door-share-the-high-bit encoding documented in "
     "docs/file-formats.md) rather than an exact Wall match -- move "
     "backward, blocked by any of the 3 blocking tile types rather "
     "than just plain Wall like cmdMoveForward. Worth double-checking "
     "this asymmetry (forward only checks 0x80 exactly, backward "
     "checks the whole sign-bit group) isn't a transcription slip on "
     "this pass's part rather than a real game-behavior difference."),

    (0x1778C, "DUNGEON_COMMAND_LABELS",
     "MEDIUM CONFIDENCE: parallel table to DUNGEON_COMMAND_TABLE, "
     "loaded into si right before the command dispatch jump "
     "(`lea si,DUNGEON_COMMAND_LABELS; mov si,[bx+si]; jmp "
     "DUNGEON_COMMAND_TABLE[bx]`) -- so si is pre-loaded with a "
     "per-command string pointer before every handler's own "
     "`call printGameText`, which is presumably why individual "
     "handlers don't always show their own explicit `lea si,aXxx` "
     "before their first printGameText call. Entries confirmed to sit "
     "inside one contiguous string-literal block (~0x11730-0x11870+) "
     "including 'Descend', 'Klimb', 'Modify order!', 'Ignite a "
     "torch\\n', 'Negate Time!\\n', 'Cast by whom-', 'Quit & Save\\n' "
     "-- one label per command, matching several of this session's "
     "confirmed command identities directly. Not fully mapped index-"
     "by-index against DUNGEON_COMMAND_TABLE this pass."),

    (0x17708, "DUNGEON_COMMAND_KEYS",
     "33-entry word array, same (scancode:char) shape as "
     "OVERWORLD_COMMAND_KEYS, parallel index-for-index with "
     "DUNGEON_COMMAND_TABLE. Verified via ida_bytes.get_word() against "
     "all 33 entries -- did NOT need the misdecoded-as-code fix this "
     "table's counterpart needed (this one was already clean data), "
     "suggesting the earlier fix was specific to how OVERWORLD_COMMAND_"
     "KEYS' surrounding bytes happened to align, not a systemic issue."),

    (0x15CC3, "cmdDisabledOnSurface",
     "'D' and 'K' (indices 15, 26): both just print a message and "
     "jump to the shared invalid-command trampoline (loc_17DBA) -- "
     "these letters do nothing on the overworld. Classic Ultima has "
     "Descend/Klimb as dungeon-only commands, consistent with this "
     "being their overworld no-op stub (the real Descend/Klimb "
     "handlers, if any, would be reached via a different jump table "
     "used while in a dungeon -- jpt_18389, referenced repeatedly "
     "throughout this session's finds but not yet itself identified)."),

    (0x15CC9, "cmdIgniteTorch",
     "'I' (index 19): dungeon-only (gameMode==1) -- prompts 'Whose "
     "torch: ' (player select), spends 1 torch (RosterEntry _torches, "
     "+0x0F, BCD-borrow-checked decrement) and sets a lit-torch flag "
     "(byte_115CE=0xFF). Confirms _torches independently of external "
     "documentation."),

    (0x11E9B, "cmdExchange",
     "'M' (index 13): prompts 2 player selections (sub_16C76 x2) then "
     "swaps their entire 0x40-byte combat records byte-for-byte, "
     "prints 'Exchanged!' -- swap two party members' marching order/"
     "positions."),

    (0x11EFD, "cmdPeer",
     "'P' (index 29): decrements the selected character's _gems "
     "(RosterEntry +0x25, confirmed via this BCD-borrow-checked "
     "decrement -- 'None Left!' on underflow) then, in a dungeon "
     "(gameMode==1), calls sub_12909 (the nibble-coded-border map "
     "renderer read early this session, previously unidentified) -- "
     "'Peer' at the dungeon layout using a gem, classic Ultima "
     "mechanic. On the overworld/towns, calls sub_1259E instead (not "
     "yet identified, presumably an equivalent overworld peer view)."),

    (0x11F25, "cmdQuit",
     "'Q' (index 20): restricted to gameMode==0 (overworld surface --"
     "'Only on surface!' otherwise) -- saves _partyPosition, calls "
     "sub_1A46F and sub_1207D ('Please wait...') -- 'Quit & Save', "
     "matching the classic Ultima restriction that saving only works "
     "on the surface."),

    (0x11F53, "cmdSteal",
     "'S' (index 30): prompts a player and a direction, ~25% chance "
     "of success (stepTimeSeededPrng &3==0, else 'Failed!' and a "
     "guard-alert side effect setting an 0xC0 flag on 'H'-type map "
     "slots -- same alert-guards-on-failed-crime pattern as ultima2's "
     "alert_town_guards), checks the target tile is in a specific "
     "range (0x94-0xE4) then a look-two-tiles-further check for a "
     "'$'(0x24) marker, clears it and calls sub_180D9 (loot-message "
     "printer, confirmed via its own 'and'/'and a ' string fragments) "
     "-- classic Ultima 'Steal' from an NPC/shop."),

    (0x11FD6, "cmdUnlock",
     "'U' (index 18): prompts a direction, checks the target tile is "
     "0xB8 ('locked door'), prompts 'Whose key? ' (player select), "
     "decrements that character's _keys (RosterEntry +0x26, confirmed "
     "via this BCD-borrow-checked decrement), and rewrites the tile "
     "via _locationTypeTable-derived value -- 'Unlock' a door with a "
     "key, confirming the _keys offset independently of external "
     "documentation."),

    (0x1888B, "cmdAttack",
     "'A' (index 1): prompts a direction, checks the target square via "
     "sub_17F96 (a monster/target-presence check, not yet identified), "
     "and if something's there, jumps DIRECTLY into "
     "beginCombatEncounter -- the overworld 'Attack' command manually "
     "triggers the exact same combat-encounter path that "
     "updateMonsterAI's automatic monster-reaches-the-party trigger "
     "uses."),

    (0x15BCF, "cmdFire",
     "'F' (index 24): requires _currentTransport==0x0B (aboard a "
     "Ship) or falls through to the invalid-command path -- prompts a "
     "direction, plays a distinct sound (0FBh), then steps a "
     "projectile up to 3 tiles in that direction (wraparound "
     "position math identical to movement), checking each step via "
     "sub_17F96 for a hit and marking an impact tile (0xF4) if so -- "
     "classic Ultima 'Fire' (ship's cannons), confirmed ship-only."),

    (0x18190, "cmdGet",
     "'G' (index 28): prompts a player selection and confirms they're "
     "alive, then (outside dungeons) checks the current tile is in "
     "range 0x24-0x27 ('$' through treasure markers) and clears it if "
     "so -- classic Ultima 'Get' (pick up gold/treasure off the map)."),

    (0x11E7A, "cmdLook",
     "'L' (index 12): prompts a direction, reads the target tile via "
     "getMapTileAt, derives a small index from it (`shr al,1` x2, "
     "+1) and calls sub_16BFA (a display helper, not yet identified) "
     "-- classic Ultima 'Look' at an adjacent tile."),

    (0x15C73, "cmdJoinGold",
     "'J' (dungeon-shared index, also dungeon index 5): prompts a "
     "player, sums every party member's _gold (BCD, with an overflow "
     "check bailing to an error path), zeroes everyone's _gold, and "
     "gives the selected player the total -- matches "
     "DUNGEON_COMMAND_LABELS' 'Join gold to:' string exactly. Classic "
     "Ultima 'Join' (pool the party's gold into one purse)."),

    (0x15CF8, "cmdNegateTime",
     "'N' (dungeon-shared index, also dungeon index 7): identical "
     "logic to combatCmdNegateTime -- prompts a player, spends 1 "
     "Powder (BCD-borrow-checked decrement) and sets "
     "_negateTimeDuration=0x0A. The overworld/dungeon counterpart to "
     "the combat-only version found earlier this session."),

    (0x17E33, "cmdReady",
     "'R' (dungeon-shared index, also dungeon index 9): prompts a "
     "player, confirms alive, calls sub_17E48 -- the exact same "
     "helper combatCmdReady uses. The overworld/dungeon counterpart "
     "to combat's 'Ready a weapon!'."),

    (0x17FC6, "cmdTransact",
     "MEDIUM-HIGH CONFIDENCE: 'T' (index 17): prompts a player and a "
     "direction ('Direct? ', reusing the tail of the aFcwtpblidardir "
     "string buffer), checks the target tile is in the same 0x94-0xE4 "
     "range cmdSteal checks, then a second tile one step further for "
     "a 0x40 marker -- classic Ultima 'Transact' (trade with a "
     "shopkeeper/merchant tile), though the exact trade mechanics "
     "aren't traced past the tile checks this pass."),

    (0x174D5, "cmdOtherCommand",
     "RENAMED 2026-09-14 from the placeholder guess 'cmdOrder' -- "
     "confirmed via dump_overworld_labels.py's read of the per-command "
     "prompt-string table at DS:18C9h (linear 0x118C9, index 32 = 'O'): "
     "the actual on-screen prompt is 'Other command!\\nWhose action? ', "
     "not anything related to party order (that's cmdExchange/'M', "
     "'Modify order!', a different command entirely -- see below). "
     "'O' (index 32, also dungeon index 8): prompts a player, confirms "
     "alive, prints 'Cmd: ' and reads a 10-character line of typed "
     "text, then looks it up against a table at a computed offset "
     "(`[bx+6540h]`) via sub_1740A/sub_17423 (not yet identified) -- a "
     "typed-text keyword command, matching cmdYell's mechanism almost "
     "exactly (same sub_1740A lookup helper) but against a different "
     "keyword table and without the _marksAndCards bit-check. Likely a "
     "second class of 'say a magic word' interaction, distinct from "
     "Yell's. Purpose past the input mechanism still not fully traced."),

    (0x11D4E, "cmdCastSpell",
     "'C' (index 23): the OVERWORLD cast-spell command (distinct from "
     "combatCmdCastSpell) -- prompts for which party member casts "
     "(sub_16C76, not yet identified -- presumably a player-selection "
     "prompt) then calls castSpell if they're alive."),

    (0x12018, "cmdToggleSound",
     "'V' (index 8) plus 2 additional scancode-only bindings (indices "
     "9, 14 -- likely alternate/Alt-key shortcuts to the same command): "
     "`xor _soundEnabled, 0FFh` then prints 'On!'/'Off!' -- a sound "
     "mute toggle. Shares this handler with a similarly-shaped 'Off!' "
     "case in a DIFFERENT jump table (jpt_1948E case 4, not yet "
     "identified -- presumably a menu/settings screen reusing the same "
     "code)."),

    (0x12035, "cmdExitVehicle",
     "'X' (index 7): writes _currentTransport (shifted to match the "
     "map's tile encoding) onto the tile at _partyPosition, resets "
     "_currentTransport to 0x3F (foot), prints 'Craft' -- confirms "
     "'X-it' as get-off-vehicle, and confirms the 0x3F=on-foot "
     "convention for _currentTransport independently of the external "
     "PARTY.ULT documentation."),

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

    (0x12068, "cmdZtats",
     "'Z' (index 10): calls sub_16DC1 with dh=al+0x30 -- matches "
     "combat's Ztats display pattern exactly (same helper, same "
     "dh=player-index+0x30 convention seen in the combat command "
     "table's Ztats handler). Shows a party member's full stat sheet "
     "outside combat."),

    (0x17458, "cmdYell",
     "'Y' (index 31): prompts 'Word: ', reads up to 9 chars via "
     "readLine, looks up the entered word against a keyword table "
     "(byte_164FA) via sub_1740A/sub_17423. On a successful match, "
     "checks bit 0x40 of the selected character's RosterEntry+0x0E "
     "field (also requires _partyPosition == 0x0A) -- the classic "
     "Ultima III 'Yell' password/keyword mechanic (e.g. yelling "
     "'RAMA' at the correct location). This is the confirming "
     "evidence for RosterEntry's _marksAndCards field (see "
     "create_roster_struct.py) -- matches docs/file-formats.md's "
     "externally-sourced 'Marks/cards bitmask' label exactly, though "
     "individual bit meanings beyond 0x40 are not decoded."),

    (0x17EE4, "cmdWear",
     "'W' (index 22): player-select prompt, alive check, then calls "
     "sub_17EFA (not independently traced past the call). Prompt text "
     "confirmed 2026-09-14 via dump_overworld_labels.py's read of the "
     "prompt table at index 22: 'Wear for # ' -- matches the classic "
     "Ultima III Wear/Wield armour command exactly, upgrading this "
     "from a by-elimination guess to a confirmed identity (the effect "
     "of sub_17EFA itself is still untraced)."),

    (0x11E55, "cmdHandEquipment",
     "'H' (index 16, also dungeon index 3): the last of the 33 "
     "overworld command letters to be identified. Prompt text "
     "confirmed via dump_overworld_labels.py's read of the "
     "per-command prompt table at DS:18C9h (linear 0x118C9): index 16 "
     "= 'Hand Equipment!\\nFrom Player: ', immediately explaining the "
     "otherwise-mysterious double player-select this handler does (the "
     "second prompt, '  To Player: ', is aToPlayer, already used "
     "elsewhere). Selects a 'from' and 'to' player via sub_16C76; if "
     "they're the same player it jumps to the shared loc_17DBA "
     "no-op/failure path, otherwise it makes a `call near ptr "
     "sub_17B54` back into the top-level command dispatcher itself. "
     "sub_17B54 saves/restores every register (including si/di) on "
     "entry/exit, so this is a genuine re-entrant call, not a "
     "parameter-passing trick -- the actual item hand-off almost "
     "certainly happens via a global 'hand mode' state (not yet "
     "located) that a subsequent single-key command (most likely 'W' "
     "for Wear, given the shared item-slot theme) reads to redirect "
     "its normal single-player behaviour into a transfer between the "
     "two globally-remembered players instead. That follow-on "
     "mechanism is NOT traced -- flagged as the next concrete lead "
     "rather than guessed at."),

    # --- Spell tables, 2026-09-14 -----------------------------------
    # castSpell (0x15D83) restricts the typed spell letter to 'A'..'P'
    # (cmp ah,41h / cmp ah,50h at castSpell+89/+92) before subtracting
    # 'A' for a 0-based index -- this, not where a following label
    # happens to sit, is what proves WIZARD_SPELL_TABLE/
    # CLERIC_SPELL_TABLE are exactly 16 entries (32 bytes) each. An
    # earlier attempt at this dump walked byte ranges until the next
    # *named* IDA symbol and silently over-read CLERIC_SPELL_TABLE by
    # 16 bytes into an unrelated, unlabeled table (aStrength/aDexterity/
    # aIntelligence/aWisdom pointers) -- same class of mistake as the
    # dungeon-command-table mixup earlier this session, caught the same
    # way (re-derive from code, not from data layout).
    #
    # Each spell's magic word is directly readable from the binary via
    # a combined 32-entry name-pointer table at linear 0x1590B
    # (`mov si, [si+590Bh]` in castSpell, index = local index + 0/0x10
    # for wizard/cleric) -- see SPELL_NAME_TABLE below. Names here are
    # taken from THAT table (i.e. confirmed present in the binary
    # itself), cross-referenced against C:\games\ultima3\ULTIMA3.TXT's
    # in-game spellbook manual for the associated flavor-text effect
    # description, which is NOT independently verified against each
    # routine's actual code -- see docs/overview.md for the full
    # writeup and the effect-description table sourced from the
    # manual. 6 of the 32 (letter, class) slots share their effect
    # address with another slot in the OTHER class's table (confirmed
    # via check_spell_addrs.py, no other collisions found) -- named
    # after whichever spell reads more naturally as primary, alias
    # noted in each entry.
    (0x1590B, "SPELL_NAME_TABLE",
     "32-entry near-pointer array (wizard spells 0-15, cleric spells "
     "16-31, matching WIZARD_SPELL_TABLE/CLERIC_SPELL_TABLE's own "
     "local indexing) read by castSpell via `mov si, [si+590Bh]` to "
     "print the spell's name before dispatching to its effect "
     "routine."),

    (0x15F9F, "spellRespond",
     "Wizard A, magic word 'Repond' (binary spelling; ULTIMA3.TXT "
     "spells it 'RESPOND'). Manual: dispels Orcs/Goblins/Trolls."),
    (0x15FD4, "spellMittar",
     "Wizard B, 'Mittar'. Manual: a directed magic-missile-style "
     "damage attack."),
    (0x15FE3, "spellLorum",
     "Wizard C, 'Lorum' -- SHARES this effect address with Cleric D "
     "'Luminae'. Manual: a short-duration magic light."),
    (0x15FEE, "spellDorAcron",
     "Wizard D, 'Dor Acron' -- SHARES this effect address with Cleric "
     "F 'Rec Du'. Manual: descend the party one dungeon level "
     "(surface-independent of the Klimb/Descend commands)."),
    (0x1600C, "spellSurAcron",
     "Wizard E, 'Sur Acron' -- SHARES this effect address with Cleric "
     "E 'Rec Su'. Manual: ascend the party one dungeon level."),
    (0x1602F, "spellFulgar",
     "Wizard F, 'Fulgar'. Manual: a fireball-style damage attack."),
    (0x16034, "spellDagAcron",
     "Wizard G, 'Dag Acron'. Manual: teleport the party to a random "
     "location, surface-only."),
    (0x1606A, "spellMentar",
     "Wizard H, 'Mentar'. Manual: an Intelligence-scaled mental "
     "damage attack."),
    (0x1608A, "spellDagLorum",
     "Wizard I, 'Dag Lorum' -- SHARES this effect address with Cleric "
     "J 'Sominae'. Manual: a longer-duration magic light than "
     "Lorum/Luminae."),
    (0x16095, "spellFalDivi",
     "Wizard J, 'Fal Divi'. Manual flavor text claims this grants "
     "access to the Cleric spellbook -- almost certainly narrative "
     "flourish rather than a literal class-unlock; actual effect not "
     "traced in code."),
    (0x160A5, "spellNoxum",
     "Wizard K, 'Noxum'. Manual: 'the first of the multi-pronged "
     "attacks' -- a multi-target damage spell."),
    (0x160BA, "spellDecorp",
     "Wizard L, 'Decorp' -- SHARES this effect address with Cleric M "
     "'Excuun'. Manual: a powerful single-target instant-kill attack."),
    (0x160BF, "spellAltair",
     "Wizard M, 'Altair'. Manual: stops time, matching "
     "combatCmdNegateTime/cmdNegateTime's Powder-based mechanic but as "
     "a spell instead."),
    (0x160CA, "spellDagMentar",
     "Wizard N, 'Dag Mentar'. Manual: a multi-target, "
     "Intelligence-scaled mental damage attack (Mentar's group "
     "version)."),
    (0x160FA, "spellNecorp",
     "Wizard O, 'Necorp'. Manual: another powerful attack spell."),
    (0x1613E, "spellZxkuqyb",
     "Cleric O, 'Zxkuqyb' -- SHARES this effect address with Wizard "
     "slot P. Notably, SPELL_NAME_TABLE's entry for Wizard P resolves "
     "to an empty string and ULTIMA3.TXT's Wizard spell list ends at "
     "O -- 'P' is not a documented Wizard spell, yet castSpell's "
     "letter-range check ('A'..'P') structurally permits selecting it "
     "anyway, and doing so invokes this exact routine (Cleric's most "
     "powerful attack spell, described in the manual as words of "
     "'anti-creation' able to end a target's life outright). Whether "
     "this is a real, exploitable quirk of the shipped game or an "
     "artifact of how the two tables happen to be packed in memory is "
     "not confirmed -- flagged as a concrete, interesting lead rather "
     "than asserted as a discovered bug."),

    (0x16153, "spellPontori",
     "Cleric A, 'Pontori'. Manual: dispels Undead creatures."),
    (0x16188, "spellApparUnem",
     "Cleric B, 'Appar Unem'. Manual: opens a trapped chest safely."),
    (0x161DC, "spellSanctu",
     "Cleric C, 'Sanctu'. Manual: minor healing."),
    (0x161F6, "spellLibRec",
     "Cleric G, 'Lib Rec'. Manual: teleport within a dungeon (blink), "
     "the dungeon-context counterpart to Wizard's surface-only Dag "
     "Acron."),
    (0x16209, "spellAlcort",
     "Cleric H, 'Alcort'. Manual: cures poison."),
    (0x1623C, "spellSequitu",
     "Cleric I, 'Sequitu'. Manual: recalls the party from a dungeon "
     "to the Sosarian surface."),
    (0x1624F, "spellSanctuMani",
     "Cleric K, 'Sanctu Mani' (ULTIMA3.TXT prints it 'SANTU MANI', "
     "missing a C -- likely a manual typo; binary spelling kept as "
     "source of truth). Manual: restores a near-dead character to "
     "full health."),
    (0x16269, "spellVieda",
     "Cleric L, 'Vieda'. Manual: reveals the party's current "
     "surroundings (a Peer-like vision spell, works in dungeon or "
     "surface)."),
    (0x16288, "spellSurmandum",
     "Cleric N, 'Surmandum'. Manual: attempts to resurrect a dead "
     "party member, turning them to ashes on failure."),
    (0x162C4, "spellAnjuSermani",
     "Cleric P, 'Anju Sermani'. Manual: restores an ashed character "
     "to life at a cost of 5 Wisdom points."),

    # --- Combat damage-resolution helpers, 2026-09-14 ----------------
    (0x18E46, "findCombatantAtPosition",
     "Called from updateMonsterAI's attack-resolution chunk and from "
     "fireProjectileAcrossArena. Searches 8 parallel-array combat "
     "slots at a fixed arena base (DS-relative `[si+24C4h]`, si "
     "running 7 down to 0): `+0x98`=occupied flag, `+0x80`=X, "
     "`+0x88`=Y. Given a target (dl,dh)=(X,Y), returns the matching "
     "slot's index (0-7) in bx via a neat trick -- `lea bx, "
     "entryFromBootup` then `sub si, bx` -- entryFromBootup's own "
     "near-offset within this segment (0x24C4) exactly cancels the "
     "array's base, leaving the plain 0-7 loop index; returns "
     "bx=0FFFFh if no occupied slot matches. This is the 8-combatant "
     "arena grid (party+monsters together) that "
     "combatHandleMovement/drawTileGrid's 11x11 rendering operates "
     "on -- distinct from updateMonsterAI's own 32-slot overworld "
     "monster arrays."),

    (0x18E7A, "fireProjectileAcrossArena",
     "Called from combatCmdAttack's chunk (ranged weapons) and from "
     "updateMonsterAI (monster ranged attacks). Steps a projectile "
     "position by a fixed (ch,cl) delta each iteration, redrawing it "
     "via drawLogoTileGrid and checking findCombatantAtPosition after "
     "each step; stops when the position leaves the 11x11 arena "
     "(either axis reaching 0Bh) or a combatant is found at the new "
     "position (bx != 0FFFFh)."),

    (0x18F5A, "applyCombatDamage",
     "Called from updateMonsterAI's melee-hit chunk, from "
     "applyRandomGroupDamage (below), and from at least 2 spell "
     "effect routines (see their notes). Takes bx=arena-relative "
     "combatant pointer (base `[bx+24C4h]`-style, same 8-slot arena "
     "as findCombatantAtPosition) and al=damage amount; subtracts al "
     "from the slot's `+0x98` count/HP field -- PLAIN BINARY, not "
     "BCD (confirmed: no `das` follows the `sub`, and "
     "beginCombatEncounter initializes this same field via `or dl, "
     "0Fh`, which would be an invalid BCD nibble). On reaching 0 (or "
     "going negative/borrowing, checked via the subtract's flags): "
     "prints 'Killed! Exp.+', clears the slot, looks up a BCD "
     "experience award from MONSTER_EXP_TABLE (see "
     "dump_monster_tables.py) indexed by `_conflictMonsterClass & "
     "0xFh` at a computed negative offset (`[bx-79EBh]`, linear "
     "0x18615), and calls addExperienceClamped. Has an unexplained "
     "special case: entirely skipped when `_conflictMonsterClass == "
     "13h` -- flagged, not investigated further (possibly an "
     "indestructible/scripted monster, e.g. a boss)."),

    (0x15EAA, "applyRandomGroupDamage",
     "Called from within spellRespond's body (`seg000:5FCBh` = linear "
     "0x15FCB falls inside spellRespond's 0x15F9F-0x15FD3 range) and "
     "spellNoxum's body (`seg000:60B1h` = linear 0x160B1 falls inside "
     "spellNoxum's 0x160A5-0x160B9 range) -- confirms both are "
     "multi-target effects sharing one implementation. Takes a damage "
     "amount in al, loops all 8 arena combat slots, and for each "
     "occupied slot rolls a 2-bit random value (`and dl,3`) applying "
     "the damage via applyCombatDamage only on a nonzero roll -- "
     "roughly a 3-in-4 chance per occupied slot, not the 1-in-4 "
     "initially estimated from the manual's 'multi-pronged' "
     "description alone. Matches the manual's description of Noxum "
     "('the first of the multi-pronged attacks') and is consistent "
     "with Respond's 'dispel Orcs/Goblins/Trolls' if called with a "
     "damage value large enough to be lethal -- the type-filtering "
     "(only Orcs/Goblins/Trolls) implied by Respond's manual text is "
     "NOT visible in this shared helper itself, so it must happen in "
     "spellRespond's own code before calling this, or the filtering "
     "claim in the manual is inexact; not fully resolved."),

    (0x18605, "MONSTER_HP_TABLE",
     "16-entry table (indexed by `_conflictMonsterClass & 0xFh`), "
     "found via beginCombatEncounter's `[bx-79FBh]` (0x10000-0x79FB = "
     "0x8605, +DS segment = linear 0x18605). Used as the upper bound "
     "passed to stepTimeSeededPrng when rolling a fresh monster's "
     "starting `+0x98` HP-like counter, then OR'd with 0x0Fh (a "
     "PLAIN BINARY value, not BCD -- see applyCombatDamage's note). "
     "Dumped via dump_monster_tables.py: values range 0x20-0xF0, "
     "roughly correlating with expected monster toughness by class "
     "index (not independently confirmed against specific monster "
     "names, since no monster-name string table has been located "
     "yet)."),
    (0x18615, "MONSTER_EXP_TABLE",
     "16-entry table (indexed by _conflictMonsterClass & 0xFh), found "
     "via applyCombatDamage's `[bx-79EBh]` (0x10000-0x79EB = 0x8615, "
     "linear 0x18615 -- exactly 0x10 bytes after MONSTER_HP_TABLE, "
     "consistent with a small monster-class stat block laid out "
     "table-by-table). BCD experience award per kill, values decode "
     "as valid BCD (1-20 XP) for every entry -- dumped via "
     "dump_monster_tables.py."),

    # --- Shrines / dungeon status bar, 2026-09-14 --------------------
    (0x1598B, "FACING_DIRECTION_NAME_TABLE",
     "4-entry near-pointer table ('North', '-East', 'South', '-West' "
     "-- confirmed via dump_small_tables.py), indexed by "
     "`_facingDirection` (0-3) in sub_162FD/drawDungeonStatusBar's "
     "`mov si,[bx+598Bh]` -- the dungeon HUD's 'Head-' heading "
     "display. Sits immediately after CLERIC_SPELL_TABLE's real "
     "16-entry end (0x1598B); an earlier, WRONG version of "
     "dump_spell_tables.py mistakenly attributed this and the next "
     "table to CLERIC_SPELL_TABLE by walking byte ranges instead of "
     "trusting the letter-range bounds check -- see docs/overview.md."),
    (0x15993, "SHRINE_ATTRIBUTE_NAME_TABLE",
     "4-entry near-pointer table (aStrength/aDexterity/aIntelligence/"
     "aWisdom, confirmed via dump_small_tables.py), indexed by "
     "`_partyPosition & 3` in enterShrine (below) to print which "
     "shrine the party is visiting -- Ultima III's 4 shrines "
     "correspond 1:1 to the 4 primary attributes."),

    (0x162FD, "drawDungeonStatusBar",
     "Called from within sub_17B54 (twice). Prints 'LVL:'+"
     "`_dungeonLevel`+1 and 'Head-'+FACING_DIRECTION_NAME_TABLE"
     "[`_facingDirection`] -- the dungeon HUD line."),

    (0x16366, "enterShrine",
     "Called from within sub_17B54 (cmdEnter's shrine branch, per "
     "docs/overview.md's location-type notes). Prompts 'shrine!\\n"
     "Who enters? ' (player select + alive check), loads SHRINE.IMG, "
     "sets `byte_114BC` (game mode) to 4, and prints the shrine's name "
     "via SHRINE_ATTRIBUTE_NAME_TABLE[`_partyPosition & 3`]. Then "
     "prompts 'Offering*100-' (a gold amount via promptForNumberEntry), "
     "rejects an offering above a shrine-specific max ('You can't "
     "cheat the Gods!'), BCD-subtracts the gold cost from "
     "RosterEntry+0x24 (`_gold`), and BCD-adds a computed amount to "
     "one of the character's 4 primary attributes (+0x12-+0x15) "
     "before printing 'Shazam!'. CONFIRMED at this level of detail "
     "only -- the exact formula selecting WHICH attribute is raised "
     "(the code looks up the character's race, `[bx+16h]`, against a "
     "5-entry table `byte_158CD` to compute an index `di`, combined "
     "with the shrine index from SHRINE_ATTRIBUTE_NAME_TABLE's lookup "
     "in a way not fully disentangled) and the per-shrine maximum "
     "offering table (`[bx+di+58D2h]`) are NOT independently verified "
     "-- flagged rather than guessed at further."),

    (0x15B28, "isSpecialEncounterLocation",
     "Called from beginCombatEncounter and updateMonsterAI when "
     "deciding how many monsters to spawn for a new encounter. "
     "Returns true (al=0FFh, via a `cmp al,0FFh` immediately before "
     "`retn` whose flags the caller reads directly with `jz`/`jnz` -- "
     "no separate flag needed) exactly when `_savedOverworldPosition`'s "
     "LOW byte equals a fixed constant (`byte_116E3` = 0Ah, never "
     "written anywhere else -- a true constant, not a variable) AND "
     "either `byte_114BC` (game mode) == 3, or game mode == 80h "
     "(combat) with `byte_158CB` == 3. In beginCombatEncounter, a TRUE "
     "result here means the encounter always spawns a full, "
     "randomly-sized group (up to 8); FALSE falls through to a "
     "separate `byte_158CB`-based check that can instead force a "
     "single monster. Functionally confirmed at this level; WHY "
     "position 0Ah / game mode 3 is special (a specific named "
     "location, e.g. a castle courtyard, is a plausible guess but not "
     "confirmed) is not resolved -- named for the condition it "
     "checks, not for an asserted purpose."),
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
