"""
IDA Pro script: master list of symbol renames (functions + globals) for
MONDAIN.EXE (ultima1_mondain.idb).

Single accumulating script, mirroring apply_renames_out.py /
apply_renames_space.py's convention. Whenever a function's or global's
role becomes clear enough to name confidently, add an entry below and
re-run. Safe to re-run repeatedly -- each entry is checked against the
address's *current* name and skipped if already applied.

Context: BinDiff-matching MONDAIN.EXE against ultima1_out (OUT.EXE)
transferred names onto 133 of 191 functions before this pass started
(1/191 -> 134/191), since the two executables statically link the same
CRT and share most of the game engine. The 57 functions BinDiff left
unmatched turned out to be almost entirely one self-contained cluster:
the actual "confront Mondain" encounter logic, reached when OUT.EXE's
`board` function chains to "mondain.exe" (see aMondain_exe xref) after
the player boards at the special location "Mondain's Gate to Hell".

Message text was decoded by resolving the small numeric immediates
passed directly to writeString/writeStringNewline (e.g. `mov ax, 24Ch /
push ax / call writeString`) as near-pointer offsets into `dseg`, i.e.
dseg_base + offset -- confirmed against known strings ("Board?",
"Get (Gem)", "Hit Mondain! ", "THE UNIVERSE IS DOOMED!", etc.) via
ida_scripts/dump_msg_strings.py and dump_msg_strings2.py. That let the
whole "letter command" dispatch table in mondainMainLoop (formerly
sub_11F85) be identified precisely by the prompt text each stub prints,
not just by structural guessing.

One pre-existing BinDiff-transferred name was actively wrong and is
fixed here too: `viewChange` (0x11240 candidate collided -- see below)
turned out to be the "Transact" [T] command stub ("Transact...Mondain
will / not negotiate!"), not a view-toggle -- BinDiff matched it to
OUT.EXE's real viewChange purely by code shape (write msg + beep),
not by content. The real "View?" [V] stub is a separate, previously
unnamed function, now named cmdViewDisabled.

For fuller justification of each rename, see the MONDAIN.EXE section
of docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_mondain -ScriptName apply_renames_mondain.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x11F85, "mondainMainLoop",
     "far proc, the sole callee of start2 (the whole executable exists "
     "to run this). Loads the special map/room data via 3 readFile "
     "calls, seeds the RNG, shows the instructions (showInstructions) "
     "and initial redraw, then loops: read a keypress, compare it "
     "against a packed (letter,jump-target) table at cs:[si+20B1h] "
     "(4 bytes/entry, si counts down from 0x78 i.e. up to 30 letters), "
     "dispatch to the matching command handler (or moveOrMeleeAttack "
     "for arrow keys via the fallthrough default), then run "
     "updateMondainState + mondainTakeTurn each iteration until either "
     "player or Mondain's HP hits 0, at which point it falls into "
     "endEncounter."),

    (0x10696, "moveOrMeleeAttack",
     "default per-tick movement handler (direction 0-4 via "
     "dirDeltaX/dirDeltaY): if the destination cell is empty, moves the "
     "player and redraws both tiles (playFX 4); if occupied by Mondain "
     "(tile state 6), does nothing/prints 'nothing'; otherwise treats it "
     "as a hit -- prints 'Burned! '/' damage!', subtracts from "
     "mondainHits, updates the status line, and (via isAdjacentToMondain) "
     "sets combatActiveFlag once player and Mondain are next to each "
     "other."),

    (0x10831, "scanAndDamageAlongDirection",
     "walks up to arg_range cells from the player's position in a given "
     "direction (dx,dy from dirDeltaX/dirDeltaY), stopping at map edge, "
     "an occupied cell, or Mondain's own tile; on a hit rolls "
     "getRandomNumber(1,100) against a hit-chance argument and, if it "
     "connects, subtracts a damage argument from mondainHits and prints "
     "'Hit Mondain! <N> damage!', else 'Missed!'. Shared by "
     "cityCastleAttackDir (physical weapon) and spellEffectMagicMissile "
     "(magic missile spell)."),

    (0x10A0D, "attackWithWand",
     "fires the currently-equipped wand (equippedWandIndex): always "
     "calls combat with the special dir=7 sentinel (cityCastleAttackDir's "
     "magic-attack branch, powered by equippedWandIndex's charge/damage "
     "table) plus a caller-supplied tier/round value. 5 separate call "
     "sites in mondainMainLoop's dispatch table pass -1/3/4/2/1 for that "
     "value -- exact letter-key mapping not confirmed, but all 5 clearly "
     "share this one 'attack with the equipped wand' action."),

    (0x10AD9, "spellEffectNone",
     "applySpellEffect case 0: prints '' then unconditionally 'Hmmm...no "
     "effect!' (guarded by `cmp ax, 618h` against a hardcoded nonzero "
     "constant -- always true, i.e. dead/always-taken branch) and "
     "returns success. Effectively a no-op spell slot; unreachable "
     "in practice since useSelectedItem only ever dispatches indices "
     "{3,7,8,9,10}."),

    (0x10B07, "spellEffectPlaceBarrier",
     "applySpellEffect case 8: after rollSpellSuccess and "
     "promptForDirectionKey, if the target cell is empty and not "
     "Mondain's own tile, marks it occupant-state 5 and draws tile 5 "
     "there ('Done.'). Paired with spellEffectRemoveBarrier."),

    (0x10BE4, "spellEffectRemoveBarrier",
     "applySpellEffect case 9: mirror of spellEffectPlaceBarrier -- if "
     "the target cell's occupant-state is 5 (the barrier "
     "spellEffectPlaceBarrier placed), clears it and redraws tile 0 "
     "('Done.')."),

    (0x10CAC, "spellEffectInterficioNunc",
     "applySpellEffect case 10, the killing spell: prints the Latin "
     "incantation '\"INTERFICIO-NUNC!\"' ('I kill now'); if the player "
     "is within isWithinRange7 of Mondain, doubles mondainHits (capped "
     "at 0x3A98) instead of killing him -- 'The spell doth seem to make "
     "him stronger!'. The backfire-instead-of-kill behavior strongly "
     "implies a precondition (gemDestroyedFlag? exact range/phase?) that "
     "isn't met yet is required to make this spell actually work -- not "
     "fully traced, see docs/roadmap.md."),

    (0x10D08, "spellEffectMagicMissile",
     "applySpellEffect default case (indices 1-6, but useSelectedItem "
     "only ever reaches this via index 3): after rollSpellSuccess and "
     "isWithinRange7 gate, computes power via getMagicWeaponPower and "
     "fires it through scanAndDamageAlongDirection (range 5, base "
     "chance 5/500)."),

    (0x10D88, "spellEffectTeleport",
     "applySpellEffect case 7: repeatedly picks a random in-bounds cell "
     "until an empty one is found, moves the player there (redrawing "
     "old/new tiles), prints 'Done.'."),

    (0x10E56, "applySpellEffect",
     "11-case jump-table dispatcher keyed by the spell/item index passed "
     "in (word_179F4 forwarded by useSelectedItem): 0->spellEffectNone, "
     "1-6 default->spellEffectMagicMissile, 7->spellEffectTeleport, "
     "8->spellEffectPlaceBarrier, 9->spellEffectRemoveBarrier, "
     "10->spellEffectInterficioNunc. Prints 'Failed!' + fail beep if the "
     "chosen effect function returns 0."),

    (0x10ED3, "useSelectedItem",
     "the 'Cast' [C] command: prints 'Cast <spell name>' (name + charge "
     "table both indexed by selectedSpellIndex), 'You've used up that "
     "spell!' if charges are 0, decrements charges, and only actually "
     "dispatches (via applySpellEffect) if selectedSpellIndex is one of "
     "{3,7,8,9,10} -- any other index (including 0, the sentinel for "
     "'nothing selected') prints 'Failed!' and does nothing."),

    (0x10F7B, "isTileOccupantAdjacentDelta",
     "true if the map cell at (player.x+dx, player.y+dy) is in bounds "
     "and its occupant-state byte equals 6 (Mondain's tile state). "
     "Helper for isAdjacentToMondain."),

    (0x10FC7, "isAdjacentToMondain",
     "calls isTileOccupantAdjacentDelta for all 8 neighboring offsets; "
     "true if Mondain occupies any of them."),

    (0x11057, "attackGem",
     "the 'Get' [G] command, prompt 'Get (Gem)': requires "
     "isAdjacentToMondain (else \"..'tis nothing here!\"); on success "
     "sets combatActiveFlag/forceRedrawFlag/gem-in-progress state, deals "
     "3/4 of mondainHits as damage (message '... <N> damage!'), and once "
     "mondainPhaseTimer reaches 30 sets gemDestroyedFlag=... wait see "
     "mondainDefeatedFlag note -- sets the win-trigger checked by "
     "mondainMainLoop."),

    (0x11124, "inspectMondainAura",
     "the 'Inform and Search' [I] command: if gemDestroyedFlag is set, "
     "prints \"Mondain's magical aura doth seem substantially diminished "
     "in the absence of the gem.\"; otherwise \"...it looks as if he is "
     "creating the evil gem!\" -- this is the status check for whether "
     "Mondain is still protected."),

    (0x1117E, "writeFlavorMessageAndBeep",
     "helper: writeStringNewline(msg) + playFX(1) (the fail/blocked "
     "sound used throughout this cluster). Shared by every disabled "
     "single-line command stub below."),

    (0x11194, "cmdBoardDisabled", "'Board?' [B] -- unusable here."),
    (0x111A2, "cmdDropDisabled", "'Drop?' [D] -- unusable here."),
    (0x111B0, "cmdEnterDisabled", "'Enter?' [E] -- unusable here."),
    (0x111BE, "cmdFireDisabled", "'Fire?' [F] -- unusable here."),
    (0x111CC, "cmdHyperjumpDisabled",
     "'Hyperjump?' [H] -- unusable here, though its caller in "
     "mondainMainLoop still sets word_15B00 = 1 afterward for unclear "
     "reasons (word_15B00 is otherwise mondainHitAnimFrame -- possibly "
     "an unused/vestigial leftover from copy-pasting another command's "
     "dispatch case). Not fully explained, see docs/roadmap.md."),
    (0x111DA, "cmdKlimbDisabled", "'Klimb?' [K] -- unusable here."),
    (0x111E8, "cmdOpenDisabled", "'Open?' [O] -- unusable here."),
    (0x111F6, "cmdQuitDisabled",
     "'Quit...is not allowed!' [Q] -- distinct from the ESC-key `escape` "
     "handler; this is the explicit Q command, permanently refused."),
    (0x11204, "cmdStealDisabled",
     "'Steal...he's watching, / That would be foolish!' [S] -- two-line "
     "variant of the disabled-command stub."),
    (0x11232, "cmdUnlockDisabled", "'Unlock?' [U] -- unusable here."),
    (0x11240, "cmdViewDisabled",
     "'View?' [V] -- unusable here. Not to be confused with the "
     "pre-existing (and, it turns out, mislabeled) `viewChange` -- see "
     "that entry below."),
    (0x1124E, "cmdXitDisabled", "'Xit?' [X] -- unusable here."),

    (0x113A3, "redrawMapAndActors",
     "redraws every one of the 19x9 (0x13 x 9) map cells from the "
     "occupant-state array, then redraws Mondain's tile at "
     "(mondainMapX,mondainMapY) using mondainPhase+mondainPhaseAnimOffset "
     "as the tile id, and latches mondainHitAnimFrame/word_15B02 to 1 "
     "if either was already nonzero. Called after every command that "
     "changes the map."),

    (0x11415, "dirDeltaX",
     "direction index (0-4) -> x delta lookup: 0,1 -> -1; 2 -> +1; "
     "3,4 -> 0."),
    (0x11447, "dirDeltaY",
     "direction index (0-4) -> y delta lookup: 0,4 -> +1; 1,2 -> 0; "
     "3 -> -1. Combined with dirDeltaX: 0=SW, 1=W, 2=E, 3=N, 4=S."),

    (0x11477, "animateTick",
     "per-frame animation update, called every idle poll iteration: "
     "advances projectileAnimFrame along a fixed path when active "
     "(reaching frame 6 clears the target tile -- a spell-effect "
     "impact), then draws Mondain's hit-reaction frame "
     "(mondainHitAnimFrame/word_15B02, tiles 6-9) if active, then "
     "unconditionally redraws Mondain's idle sprite using "
     "mondainPhase(+something for the 2/0xB phase cases)+"
     "mondainPhaseAnimOffset."),

    (0x11565, "randomSign",
     "returns sgn(getRandomNumber(1,255) - 128), i.e. roughly a "
     "coin-flip -1/+1 (rarely 0)."),

    (0x11589, "tryMoveMondain",
     "attempts to move Mondain by (dx,dy): if the destination is "
     "in-bounds, empty, and not the player's own cell, clears Mondain's "
     "old tile, moves mondainMapX/mondainMapY, and redraws him there. "
     "Returns whether the move succeeded."),

    (0x1161E, "damagePlayer",
     "applies a damage amount to the player: prints 'You are hit for ' "
     "(word_179B6-colored) + number + ' damage!', subtracts from "
     "playerHits, playFX(2), writeStatusHits."),

    (0x116E0, "mondainSpecialAttack",
     "Mondain's special-ability roll (called from mondainTakeTurn): "
     "1-in-3 chance of one of three effects -- (0) ~50% chance of a "
     "small damagePlayer hit; (1) ~30% chance of draining/weakening a "
     "7-entry resource array at dseg+0x206E (likely the player's "
     "readied-item charge table) by ~1/8 or 2 per entry, 'The Gem is "
     "DESTROYED!'-style multi-line flavor text; (2) plays sub_14031/"
     "restoreNormalPalette-bracketed screen flash, calls "
     "restoreNormalPalette, and (~70% chance) damagePlayer with "
     "damage scaled by the player's current HP. Prints 'Missed!' if the "
     "chosen sub-effect's own roll failed."),

    (0x1186F, "mondainTakeTurn",
     "Mondain's per-tick AI, only active once combatActiveFlag is set: "
     "occasional random no-op ('nothing happens'), HP-regen while in "
     "phase 4 (+25 mondainHits), else if adjacent to the player and in "
     "phase 20 calls dungeonAttack (melee/ranged damage-the-player "
     "attack), else ~12.5% chance in phase 20 calls mondainSpecialAttack, "
     "else in phase 40 (wounded) tries to reposition via tryMoveMondain "
     "(random.Sign-derived direction), regenerating +5 HP if it can't "
     "move."),

    (0x119C8, "updateMondainState",
     "per-tick state machine, only active once combatActiveFlag is set: "
     "occasionally (36% chance, capped at 100 tries) spawns a decoy tile "
     "(state 0xE) at a random empty cell not on Mondain or the player; "
     "then updates mondainPhase from mondainHits -- >500 HP -> phase 2 "
     "(healthy), 1-500 HP -> phase 0xB (wounded, timer 40), <=0 HP with "
     "phase timer != 30 -> death: prints 'THOU ART DEAD!'-style closing "
     "line depending on gemDestroyedFlag (sets mondainDefeatedFlag if it "
     "was set), phase 4 (dying), timer 30."),

    (0x11B46, "pollPlayerCommand",
     "reads+uppercases one keypress (non-blocking), calls animateTick, "
     "and inserts extra wait() ticks (plus a waitVerticalRetraces(4) "
     "flash when mondainHitAnimFrame is active) when no key was ready, "
     "so the idle animation keeps advancing while waiting for input. "
     "Returns the key (0 if none)."),

    (0x105DB, "promptForDirectionKey",
     "blocking variant of pollPlayerCommand: loops calling "
     "animateTick + wait(1 or 3) until a nonzero key is read, then "
     "returns it raw (no _toupper). Used wherever a spell/attack needs "
     "an explicit direction (combat's dir==0xFFFF sentinel, "
     "spellEffectPlaceBarrier, spellEffectRemoveBarrier, "
     "spellEffectMagicMissile)."),

    (0x11B9B, "drawInstructionsPage1", "first of 4 instruction-dialog pages shown by showInstructions."),
    (0x11CAF, "drawInstructionsPage2", "second instruction-dialog page."),
    (0x11D53, "drawInstructionsPage3", "third instruction-dialog page."),
    (0x11D94, "drawInstructionsPage4", "fourth and last instruction-dialog page."),
    (0x11DE6, "showInstructions",
     "shows all 4 instruction pages in sequence, each followed by a "
     "viewportPagingWait ('press space to continue'-style pause). "
     "Called once at the start of mondainMainLoop."),

    (0x11E03, "drawFullscreenImage",
     "generic packed 1-bit-per-pixel bitmap renderer: readFile's a "
     "0x1A40-byte image into a scratch buffer, then walks it byte by "
     "byte (8 pixels/byte) calling drawPenPoint across a 320-wide "
     "canvas. Used by playMondainDefeatCutscene for the victory "
     "picture; generic enough it could in principle be reused "
     "elsewhere, but this is its only call site in this executable."),

    (0x11EA8, "showMondainDeathDialog",
     "draws Mondain's tile at his last position, a dialog box with the "
     "death message, 3 beeps, a short wait, then blocks (up to 10000 "
     "poll iterations) for a keypress before returning. Precedes "
     "playMondainDefeatCutscene."),

    (0x11F31, "playMondainDefeatCutscene",
     "the victory sequence: showMondainDeathDialog, clear the screen, "
     "drawFullscreenImage the closing picture, wait, print the closing "
     "status line. Reached from mondainMainLoop once mondainDefeatedFlag "
     "is set."),

    (0x1138D, "endEncounter",
     "zeroes playerHits and playerFood, refreshes both status widgets, "
     "and falls into writeInUseAndExit (the disk-swap-then-chain-back-"
     "to-OUT.EXE sequence) -- reached from mondainMainLoop's loop-exit "
     "condition (playerHits <= 0 or playerFood <= 0), i.e. this is what "
     "runs whether the player died or ran out of food, not specifically "
     "a win path (the win path runs playMondainDefeatCutscene first, "
     "then presumably reaches this same exit -- not fully confirmed, "
     "see docs/roadmap.md)."),

    (0x1047D, "fillDialogArea",
     "fillRect's the standard dialog background rect then calls "
     "drawDialogBox -- the common setup shared by drawDialog and all "
     "4 drawInstructionsPageN helpers."),
    (0x104BE, "drawDialogBox",
     "draws the 3-line dialog border (drawLine + 2x drawLineTo) and "
     "homes the cursor to (4,5) for the caller to write into."),

    (0x1064A, "isWithinRange7",
     "true if the Manhattan-ish distance (|dx|+|dy|) between the player "
     "and Mondain's positions is < 7. Gates spellEffectMagicMissile and "
     "spellEffectInterficioNunc."),
    (0x10618, "rollSpellSuccess",
     "getRandomNumber(1,249) against a threshold derived from "
     "word_179EA(+0xA0), or unconditional success if word_179DA==1. "
     "Gates all 4 direct-target spell effects (place/remove barrier, "
     "magic missile, teleport) before they prompt for a direction."),

    (0x13CF1, "waitVerticalRetraces",
     "polls port 3DAh (video status) to wait for N full vertical "
     "retraces (arg_0 = count); no-op if arg_0 is 0. Same technique as "
     "waitVerticalRetrace in ultima1_space/ultima1_out, but this copy "
     "takes an explicit repeat count instead of waiting for exactly "
     "one."),
    (0x13FF9, "restoreNormalPalette",
     "sets the CGA/EGA palette back to the default scheme (border "
     "black, palette 1) via INT 10h/AH=0Bh (CGA path) or AH=10h,AL=2 "
     "with a 17-byte register list at dseg+0xC4C (EGA path, gated on "
     "word_163EC). Paired with setHitFlashPalette."),
    (0x14031, "setHitFlashPalette",
     "same shape as restoreNormalPalette but with a different palette "
     "(border white/0Fh, EGA register list at dseg+0xC5E) -- a brief "
     "flash used to punctuate a hit, called just before "
     "restoreNormalPalette in both mondainSpecialAttack and "
     "moveOrMeleeAttack."),
]

# Fixing a BinDiff-transferred name that turned out to be wrong: matched
# to OUT.EXE's real viewChange purely by code shape (writeStringNewline +
# writeFlavorMessageAndBeep, same as several other 2-line command stubs
# here), not by string content. The actual text it prints is the
# "Transact" [T] command's refusal message. Looked up by current name
# since (unlike the sub_XXXXX entries above) its address isn't encoded
# in its current name.
RENAME_FIXES = [
    ("viewChange", "cmdTransactDisabled",
     "was `viewChange` (BinDiff-transferred from OUT.EXE by code shape, "
     "not content). Actually prints 'Transact...Mondain will' + "
     "'not negotiate!' -- the 'T' command's refusal message, unrelated "
     "to view-switching. The real 'View?' stub is the separate, "
     "previously-unnamed function now named cmdViewDisabled (0x11240)."),
]

# (ea, new_name, note) -- globals identified with high confidence from
# the message-string decoding + call-site analysis above.
GLOBAL_RENAMES = [
    (0x179DE, "playerHits",
     "the player's HP pool in this encounter -- 'hits' matches this "
     "game's established attribute-naming (see GEN.EXE's "
     "_savegame._hits). Zeroed by endEncounter, decremented by "
     "damagePlayer, displayed via writeStatusHits."),
    (0x179F0, "playerFood",
     "zeroed alongside playerHits by endEncounter and refreshed via "
     "writeStatusFood immediately after -- the player's food/rations "
     "stat, same as the overworld game's."),
    (0x15A96, "playerMapX", "player's column in the 19x9 (0x13 x 9) encounter map."),
    (0x15A98, "playerMapY", "player's row in the 19x9 encounter map."),
    (0x15A9A, "mondainMapX", "Mondain's column in the encounter map (his tile's occupant-state is 6)."),
    (0x15A9C, "mondainMapY", "Mondain's row in the encounter map."),
    (0x15A9E, "mondainHits",
     "Mondain's HP pool -- damaged by scanAndDamageAlongDirection "
     "(ranged/wand attacks), attackGem ('Get'), and (backfiring, "
     "doubling it) spellEffectInterficioNunc; drives updateMondainState's "
     "phase transitions and the win condition."),
    (0x15AA0, "projectileAnimFrame",
     "-1 when idle; while active, counts up each animateTick call along "
     "a fixed on-screen path (tiles dseg+0xE.. via drawTile) until it "
     "reaches 6, at which point the target cell is cleared -- a spell "
     "projectile's flight/impact animation."),
    (0x15B00, "mondainHitAnimFrame",
     "0 when idle; latched to 1 by moveOrMeleeAttack the moment the "
     "player successfully lands a hit while adjacent to Mondain, then "
     "oscillates 1-4 (see word_15B02) each animateTick call to select "
     "his hit-reaction sprite (tiles 6-9)."),
    (0x15B02, "word_15B02",
     "oscillation direction (+1/-1) for mondainHitAnimFrame -- left "
     "unrenamed pending clearer understanding of the exact frame timing, "
     "see docs/roadmap.md."),
    (0x15BB0, "mondainPhase",
     "Mondain's behavioral phase, also doubling as his idle sprite's "
     "base tile id: 2 = healthy, 0xB = wounded, 4 = dying/regenerating. "
     "Set by updateMondainState, read by mondainTakeTurn and "
     "animateTick."),
    (0x15BB2, "mondainPhaseAnimOffset",
     "small idle-sway offset added to mondainPhase to pick Mondain's "
     "exact idle tile; also reused as a random 0/1/2 toggle in "
     "mondainTakeTurn depending on phase."),
    (0x15BB4, "mondainPhaseTimer",
     "countdown/marker associated with the current mondainPhase (20 for "
     "healthy-combat, 40 for wounded, 30 for dying) -- gates whether "
     "mondainTakeTurn/updateMondainState act this tick."),
    (0x15BB6, "combatActiveFlag",
     "0 until the player is first adjacent to Mondain (set by "
     "moveOrMeleeAttack or attackGem); gates mondainTakeTurn and "
     "updateMondainState from running at all."),
    (0x15BB8, "gemDestroyedFlag",
     "set once attackGem's damage crosses its threshold; read by "
     "inspectMondainAura ('Inform and Search') and "
     "updateMondainState's death-message branch."),
    (0x15BBA, "mondainDefeatedFlag",
     "the actual win trigger -- checked in mondainMainLoop's loop "
     "condition; when set, breaks out to playMondainDefeatCutscene."),
    (0x179F2, "equippedWandIndex",
     "index into the wand name/charge tables, read by attackWithWand "
     "and cityCastleAttackDir's dir==7 branch. Presumably set by the "
     "pre-existing `ready` [R] command (not re-examined in this pass)."),
    (0x179F4, "selectedSpellIndex",
     "index into the spell name/charge tables, read+decremented by "
     "useSelectedItem and forwarded to applySpellEffect. 0 = nothing "
     "selected (always fails); only {3,7,8,9,10} are actually usable."),
    (0x179BC, "escapeRequestedFlag",
     "set just before calling `escape` on an unrecognized keypress "
     "(the dispatch table's default/ESC case) in mondainMainLoop."),
    (0x179BE, "forceRedrawFlag",
     "set by showStats and attackGem; checked each mondainMainLoop "
     "iteration to trigger an extra redrawMapAndActors call."),
]


def apply_rename(ea, new_name, note):
    if ea == 0:
        return
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


def apply_rename_by_current_name(cur_name, new_name, note):
    ea = idc.get_name_ea_simple(cur_name)
    if ea == idc.BADADDR:
        print(f"[!] {cur_name!r} not found -- already renamed? skipping")
        return
    apply_rename(ea, new_name, note)


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    for cur_name, new_name, note in RENAME_FIXES:
        apply_rename_by_current_name(cur_name, new_name, note)
    for ea, new_name, note in GLOBAL_RENAMES:
        apply_rename(ea, new_name, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
