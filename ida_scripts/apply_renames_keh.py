"""
IDA Pro script: master list of symbol renames, comments and data labels for
KEH.EXE (fod/Fountain of Dreams KEH.idb).

Single accumulating script. Whenever a finding is confirmed, add an entry to
RENAMES / GLOBALS below and re-run. Idempotent: entries already applied are
skipped silently. Each entry carries a justification note; fuller write-ups
live in docs/overview.md.

    powershell -ExecutionPolicy Bypass -File run_ida_script.ps1 -Idb KEH -ScriptName apply_renames_keh.py

Data labels are given as dseg-relative offsets (the form used in the
disassembly operands), converted to EAs via DSEG_BASE.
"""

import idc
import ida_name

DSEG_BASE = 0x21240
DRY_RUN = False

# (code ea, new_name, note)
RENAMES = [
    # -- screen / dirty-rect layer. The screen is a 320x200 4bpp (160 bytes
    # per scanline) offscreen buffer (_picDecoded_0 segment) that is copied
    # to B800h through a list of dirty rectangles. --
    (0x1D9F5, "addDirtyRectFromBounds",
     "loads x/y/w/h from a Bounds struct and calls addDirtyRect"),
    (0x2077F, "addDirtyRect",
     "far; appends a 14-byte entry (screen dest, src, row count, words/row) "
     "to the dirty list at dseg:A251; AX=x, SI=y, DI=w, CX=h"),
    (0x208E2, "flushDirtyRects",
     "far; blitDirtyRects then reset the list head/count"),
    (0x208FC, "blitDirtyRects",
     "copies each queued rectangle from the offscreen buffer to video memory B800h"),
    (0x20772, "clearDirtyRects", "far; resets list head and count only"),
    (0x1DA17, "refreshBounds",
     "queue a Bounds rect and flush it to the screen immediately"),
    (0x1DC28, "refreshCurrentWindow",
     "refreshBounds on the current window's rect (field_1A override if set)"),
    (0x20720, "restoreTextMode",
     "far; INT 10h mode 3, used on exit and error paths"),

    # -- window / text layer --
    (0x1DDFD, "newLine",
     "carriage-return inside the current window; on overflow of the last "
     "line pages output (pauses for a key when byte_22A74 is set) or scrolls"),
    (0x1E18D, "scrollWindowUp",
     "scrolls the window contents up one text row then delays according to "
     "word_22C44 (text speed 1-8, adjusted live by the < > , . keys)"),
    (0x1DB58, "substituteCaret",
     "copies a string to unk_2323A, expanding '^' to the string at word_22C8A"),
    (0x1DBFD, "writeStringFlushRun",
     "word-wrap helper of writeString: emits chars from ptr1 up to ptr3"),
    (0x1DC0B, "writeStringRun",
     "writes CX characters from _writeStringPtr1 via writeChar"),
    (0x1DE81, "writeCharAtWindowPos",
     "writeChar at window-relative (x, y); arg0 = char"),
    (0x1E376, "writeIndexedString",
     "skips N NUL-terminated strings in the table at dseg:A834 and prints the Nth"),
    (0x1E2B8, "invertRect",
     "XORs the pixels of a window rect (4bpp) and optionally refreshes it"),
    (0x1E28A, "winClear", "zero-fills the window rectangle in the offscreen buffer"),

    # -- map view rendering --
    (0x1DC46, "buildMapView",
     "builds the 19x9 tile-word grid at unk_230E4 around the party position "
     "(mapPos - (9,4)) from the loaded map; cell flags 0x40/0x80 add overlay bits"),
    (0x1DD19, "drawMapViewTiles", "draws the 19x9 grid built by buildMapView via drawTile"),
    (0x1DD4C, "redrawMapView", "buildMapView + refresh the map viewport rect"),
    (0x1E098, "drawTile",
     "blits a 16x16 4bpp tile (128 bytes) with an optional masked overlay "
     "tile; AX = tile word (low 11 bits index, high bits overlay)"),
    (0x1E017, "blitTileRaw", "raw 16x16 blit of the current-window-relative cell"),
    (0x1E141, "writeBorderCell", "blits an 8x8 glyph (32 bytes) from fontData at textX/textY"),

    # -- misc runtime --
    (0x1DEE7, "rngByte",
     "16-byte additive-carry PRNG state at unk_22A64; returns next byte in AL"),
    (0x1DEB1, "randomRange",
     "random(min, max) inclusive, by rejection sampling rngByte"),
    (0x1DEDB, "rngByteSaveRegs", "rngByte wrapper that preserves SI/BX/CX/DX"),
    (0x1E08C, "getBiosTicks", "INT 1Ah AH=0, returns the low word of the tick count"),
    (0x1E4D0, "lzssDecode",
     "LZSS: 4096-byte ring buffer at dseg-relative 0xA8DD, start 0xFEE, "
     "12-bit offset / 4-bit length+3, flag bytes consumed LSB first"),
    (0x1E5BD, "lzssDecodeSegs",
     "args set the source/dest segments (word_2BB18/2BB1A) then lzssDecode"),
    (0x21130, "playSound",
     "far; starts PC-speaker sound effect N (<=6) from the table at dseg:-458C; "
     "waits for vertical retrace and steps sound script until finished"),
    (0x2119E, "soundTick",
     "interprets the speaker script one tick: 83h=note, 81h=repeat, 84h=stop"),
    (0x21090, "blitPic",
     "far; copies a width/height-prefixed picture (4-byte header) to the "
     "offscreen buffer at (x, y)"),
    (0x210DD, "blitPicFrame", "far; blitPic of frame N from a picture-set with offset table"),

    # -- game helpers --
    (0x1B452, "redrawGameScreen",
     "resets windows, redraws party list, map view and clock"),
    (0x1B29B, "showMessageWaitKey",
     "prints text in the dialog window (or window7), pages output, waits for key 0xFE"),
    (0x17E09, "markMapDirty", "sets bit 0x80 of the map header flags (dword_22C6E+5)"),
    (0x1073E, "isWithinRadiusOfParty",
     "(radius, x, y) -> true if the point is within a square radius of the party map position"),
    (0x12AB2, "setMapCellHighBits",
     "sets the top 5 bits of the map cell word at (x, y), preserving the low 11 bits"),
    (0x12B93, "checkPartyWipeAndQuit",
     "if no member has con > -15: revive them to -65 (0xFFBF) and run the end handling, then quit(arg)"),
    (0x16F6B, "isMemberActive", "true if member.con (+44h, minus level overflow >15) is positive"),
    (0x2045A, "removeMemberFromParty",
     "removes a member by party slot: clears its roster flag, shifts the party arrays, decrements partyCount"),
    (0x115CB, "drawListMenuPage",
     "draws one page (max 8 rows) of a scrolling list menu with up/down markers; "
     "rows are drawn through a callback"),
    (0x11682, "listMenuSelect",
     "runs a scrolling list menu: keys 1-8, PgUp/PgDn/cursor keys; returns the chosen index"),

    # -- pass 2: startup / data loading / world update (chunk 10010-11400) --
    (0x1008C, "loadMap",
     "arg=map number; if the map's region (table at dseg+125h) changed, loads the "
     "region files, then reads the map (LZSS) and sets the sub-table pointers "
     "(map+404h..40Ch -> dword_2EBF8/2E446/2EC14/2E41E/2EC1A) and the party cell pointer"),
    (0x10022, "loadMapPicSet",
     "reads+LZSS-decodes the current region's picture/animation block into dword_2EE12"),
    (0x101BE, "loadSaveState",
     "reads DISK1 (Disk1 struct), validates against the DISK1..3 save disks, picks the newest, "
     "rebuilds the party member pointer table (0x14C bytes per roster slot at 0BF24h)"),
    (0x103BF, "readFileChunk",
     "args(file, idx, table, mode, dst): opens file (prompting for disk), seeks to table[idx].offset<<4, "
     "reads table[idx].size bytes (mode 1: segmented read via sub_1E448)"),
    (0x10444, "readFileBlock", "args(file, dst, nwords): opens file, reads nwords*4 bytes"),
    (0x169A3, "openGameFile",
     "opens a file read-only, looping on 'Please insert %s.' prompts until it succeeds"),
    (0x10795, "distanceToParty",
     "Chebyshev distance (1..63) from the party map position to (x, y)"),
    (0x107E1, "drawPartyStatus",
     "draws window1 party rows: F-key, name, level, AC, hp/status, ready weapon; arg0 = highlighted row"),
    (0x10A87, "getDirectionKey",
     "reads a cursor key; returns direction 1..5 (up,down?,right,left,space); sets byte_2E441 dir and "
     "target x/y (byte_2E423/2E428) from the delta table at dseg+1DCh, bounds-checked against the map size"),
    (0x10B45, "drawWhichWayPrompt", "'Which way?' prompt with ^ < space > v key hints"),
    (0x10B95, "askDirectionGetCell",
     "prompts for a direction and returns a far pointer to the target map cell (0:0 if cancelled)"),
    (0x10BE7, "getPartyFKey",
     "waits for F1..F5 (3Bh..3Fh) naming a valid party slot; returns slot or -1 on ESC"),
    (0x10C42, "isPartyPosition", "true if (x,y) equals the party's map position"),
    (0x10D19, "markEntityOnMap",
     "if (x,y) is within radius of the party, stores overlay id (arg3<<11) in the map cell word"),
    (0x10D78, "placeEntitiesOnMap",
     "stamps every live monster (0x68-byte records at dword_2E446) and group (0x66-byte records at dword_2E41E) onto the map"),
    (0x10EF7, "moveMapEntities",
     "advances each monster/group movement by its speed accumulator (+64h/+0Ah), calling moveEntityStep"),
    (0x10E29, "moveEntityStep",
     "moves one entity one step toward/away from the party, updating map cell overlay bits"),
    (0x11033, "writeTime", "draws weekday and HH:MM in windowTime"),
    (0x1109D, "setTime",
     "advances the clock by 1 minute (arg!=0) or by map header +6 minutes; rolls hours/weekday; redraws clock"),
    (0x1113F, "passTime",
     "per-tick party upkeep: heal 1 hp every 10 ticks (5 in combat), apply status-effect damage (HDSPCT table), "
     "kill via sub_18FFA, wipe check"),
    (0x112E9, "passTimeStep", "randomRange(1,100) then passTime(0); periodic sub_192D5; wipe check"),
    (0x11339, "moveMonstersAndCheckEncounter",
     "moveMapEntities + placeEntitiesOnMap, then if enemies are adjacent starts combat "
     "('Uh oh! Here comes trouble!') via sub_15691"),
    (0x1138D, "endOfTurn",
     "passTimeStep; if arg==0 also moveMonstersAndCheckEncounter; then redrawMapView"),

    # -- pass 3: map cell helpers, combat resolution (12C0F-14A9E) --
    (0x12C0F, "isCellPassable",
     "false if out of bounds, cell byte+2 bit0 clear, or a monster/group stands on it"),
    (0x12CF5, "setBlastMessageStrings", "sets the 'The blast'/'also pounds' string pointers"),
    (0x12D0A, "pickMapCell",
     "cursor selection on the map: arrow keys move a highlighted cell (flag 40h) until space/ESC; returns the cell pointer"),
    (0x12E76, "playFireSound", "plays sound 3 once per shot when byte_2E442 (fire mode) is 1 or 2"),
    (0x12F44, "getItemWeaponDef", "item id -> weapons table entry (weapons + 2 + 3Ch*globals[item*18h+3Fh])"),
    (0x12F62, "rollAttack",
     "attack resolution: attacker/defender (type 0 member,1 monster,2 group) in the 6-byte attack record; "
     "returns 0 miss,1 ?,2 hit,3 useless,4 kill and accumulates damage through arg0"),
    (0x136B8, "applyMonsterHitReaction", "monster state byte +5Dh handling after being shot (stun/flee counters)"),
    (0x138CA, "printAttackLine", "prints '%Fs %Fs %Fs' attacker/verb/target line"),
    (0x13901, "resetAttackState", "resets burst/fire-mode globals (2E43A..2E444)"),
    (0x13B70, "removeDefeatedFromMap", "after a kill: marks member/monster dead or clears the group bit and map cell"),
    (0x13C03, "printDamage", "prints ' for %d pts damage.'"),
    (0x13C29, "resolveAttack",
     "runs rollAttack and prints/applies the outcome (miss, kill, damage, mutagen exposure, 'useless against')"),
    (0x13E79, "attackMultiShot", "repeats resolveAttack for burst/auto fire, rolling to double damage"),
    (0x13FAA, "pickWeakestInGroup", "chooses the lowest positive hp slot of an enemy group; sets word_2E436/dword_2E42A"),
    (0x140B4, "selectTargetStats",
     "points dword_2E42A at the target's hp and loads xp reward (professions table) and kill-name type"),
    (0x14161, "isWithinWeaponRange", "square range test around the attacker's position (2E43F/2E440)"),
    (0x141B8, "attackNearbyTargets", "applies the attack to other targets in the blast radius"),
    (0x14375, "findNearbyTarget", "finds another combatant within range 1..3 of the attacker"),
    (0x14438, "retargetIfDead", "after a kill, retargets the attack at a living neighbour"),
    (0x14498, "checkWeaponMalfunction", "roll vs weapon jam%; prints 'weapon has jammed/broken'/'fails to detonate'"),
    (0x1459F, "memberAttack", "complete attack action for one party member including ammo use"),
    (0x14788, "drawCombatNames", "writes the three combat name windows"),
    (0x147D8, "refreshCombatScreen", "redraws combat panel windows"),
    (0x1492E, "chooseTarget", "menu of targetable enemies in range; returns combatant index or -1"),
    (0x14A9E, "combatMoveEnemy", "moves an enemy monster/group one step toward the party; prints 'closer.' / 'to a better postion.'"),
    (0x14C43, "chooseEnemyAction",
     "AI for one enemy combatant: sets its command (4 attack, 1 evade, 0Eh none) and picks a living party target"),
    (0x14E61, "addCombatant", "stores a combatant record (12 bytes at 2E6D4+idx*12) and its initiative"),
    (0x14F1A, "buildCombatants",
     "collects live monsters/groups near the party into the combatant table, sorts by speed with qsort; returns count"),
    (0x15390, "sortCombatants", "qsort of the combatant table by speed groups"),
    (0x154B0, "clearCombatCommands", "sets the 5 party command bytes (197Ah) to 0FFh"),
    (0x154D6, "getPartyLevelUnit", "sum of live members' levels / 5, min 1"),
    (0x15528, "chooseFiringMode", "S)ingle / B)urst / F)ull auto prompt; sets member +144h/+146h"),
    (0x1564A, "redrawCombatScreen", "redraws the combat windows and refreshes the screen"),
    (0x15691, "runCombat",
     "main combat loop: per party member choose R)un E)vade A)ttack L)oad/unjam W)eapon U)se; "
     "'Encounter begins...' then executes each combatant's command in initiative order"),
    
    # -- pass 4: input, items, level-up, encounters (1695C-17E37) --
    (0x1695C, "showMessageBox", "plays sound 1 and shows a message (short form in combat); used by the critical-error handler"),
    (0x169E8, "drawCombatCommandRow", "builds the combat command text for roster row y: name + command string (off_21DD0) + target"),
    (0x16B44, "animationTick",
     "runs map/sprite animation scripts on a ~3 tick cadence (force!=0 ignores the timer); frames drawn via blitPicFrame"),
    (0x16D5E, "flushKeyboard", "drains the DOS keyboard buffer"),
    (0x16D6B, "isUniqueItem", "item.globals[+3Eh] & 1Ch == 1Ch (quest/unique item)"),
    (0x16D96, "setUniqueItemFlag", "sets bit globals[item+41h] in disk1.field_7 (unique items obtained)"),
    (0x16DB4, "clearUniqueItemFlag", "clears that bit"),
    (0x16DD4, "tryUnjamWeapon", "skill roll (profession-dependent) to clear the jam flag on the readied weapon; prints success/failure"),
    (0x16F3A, "attrBonus", "returns max(0, value-15): bonus from an attribute/skill above 15"),
    (0x16F9D, "printSkillLevel", "prints '%s now has a %Fs level of %d.'"),
    (0x17015, "partySkillCheck", "best living member's attribute arg2 among those who know skill arg1: random(1,100) < 2*bonus"),
    (0x170F2, "findFreeRosterSlot", "first of 5 roster slots (0x14C bytes each) with empty name"),
    (0x1711E, "refreshPartyDisplay", "redraws party status (in-dialog) or the party name windows"),
    (0x17140, "retrieveStoreMember", "script op 72: 'What do you wish? 1. Retrieve a member 2. Store a member L)eave'"),
    (0x1738A, "showMessageAt", "clears window, writes text at (x,y), waits for a key"),
    (0x173AC, "showShortMessage", "clears window, writes a short string and waits for a key"),
    (0x173C9, "rollGroupHp", "rolls an enemy slot's hit points around the party average, +-arg0 percent, and derives count"),
    (0x17470, "spawnEnemyGroup", "initialises a group record (position, counts, per-slot hp) from its template and stamps the map"),
    (0x17685, "tryRandomEncounter",
     "if map header +8 (encounter rate) passes a d100 roll, picks an adjacent free cell and spawns a group from the template list"),
    (0x17854, "checkTimedEffects",
     "hourly: counts down member timers (+57h, +21h) and triggers scheduled mutation flare-ups ('mutations acting up again')"),
    (0x17AAB, "exposeToMutagens", "chance roll: schedules a mutation onset (member +53h..56h) on a future weekday/hour"),
    (0x17B02, "applyItemEffect", "item side effect on a member (timed buff or status bit from the item definition)"),
    (0x17B3F, "checkLevelUps", "promotes members whose experience reached the next rank: hp gain, skill points, 'Distribute %d points'"),
    (0x17E13, "setCellFlag80", "sets bit 80h in cell byte+2 at (x,y)"),
    (0x17E37, "clearCellFlag80", "clears bit 80h in cell byte+2 at (x,y)"),

    # -- pass 5: ground items, loot, save (17ED0-18880) --
    (0x17ED0, "findGroundItemsAt", "collects indices of ground-item records (6 bytes at dword_2EC1A) at (x,y) into the scratch list; returns count"),
    (0x17F34, "removeGroundItem", "deletes one ground-item record, shifts the table; clears the cell's item flag if none remain"),
    (0x17FA8, "lootCell", "'You find these items:' pickup menu for the ground items at (x,y), giving them to party members"),
    (0x1822E, "addGroundItem", "adds qty of an item to an existing stack at (x,y) or fails (arg2==0 clears the stack)"),
    (0x182CA, "dropPacketLoot", "rolls the loot packet table (PACKETS) at idx for items to drop at (x,y)"),
    (0x184A8, "addScratchItemsAtCell", "appends the scratch item list to the ground-item table at (x,y)"),
    (0x1858A, "compactGroundItems", "movedata compaction of the ground-item table after an insert/remove"),
    (0x18607, "removeMapEntity", "removes a monster (arg4==1) or group record from the map arrays and drops its loot as ground items"),
    (0x187E5, "reloadMapIfResettable", "if the map header flags bit 8000h is set, clears 80h and reloads the map pristine"),
    (0x18827, "writeSaveState", "increments the save counter and writes the Disk1 struct (0EC0h bytes) to the save disk"),
    (0x18880, "loadCombatPortraits", "loads/decodes up to 3 enemy picture sets for the combat window (HDS/KPICT) and queues their animations"),

    # -- pass 6: script helpers (18A56-198E3) --
    (0x18A56, "showEndingScreen", "script op 74: loads ending picture set N from the file table at dseg+0EFEh, displays it; N==3 quits"),
    (0x18B08, "findMonsterById", "returns far ptr to the monster record whose +49h id matches (index via arg2)"),
    (0x18B69, "setCellFields", "packs value/flag bits into a map cell's bytes +2,+3,+4 (script-driven cell edit)"),
    (0x18BBB, "applyCellChange", "applies change kind 19h-1Eh to a map cell: 19h/1Ch tile, 1Ah/1Dh fields, 1Bh/1Eh script word"),
    (0x18C32, "recordMapChange",
     "script ops 28-30: applies a cell change now if on the current map, else appends a 12-byte persistent change record (list at word_21326, max 150)"),
    (0x18E23, "resolveMemberSel", "script member selector: 0 = current member, n = party slot n-1"),
    (0x18E37, "removeItemsByFlag", "removes from a member every item whose globals[+3Eh] has any bit of the mask"),
    (0x18E9C, "removeMemberAmmo", "removes all ammo items matching any carried weapon's ammo type"),
    (0x18F3E, "getScriptString", "returns far ptr to the Nth NUL-terminated string after the script block header"),
    (0x18F63, "printScriptString", "writes script string N at the top-left of the current window"),
    (0x18FFA, "damageMember", "subtracts hp; records the 'last hurt' value; marks the member dead below -60"),
    (0x19073, "applyDamageWithArmor", "random damage in [min,max] reduced by armor class; returns remaining damage"),
    (0x1912F, "damageParty", "area damage to every living member with armor absorption messages ('takes %u pts of damage', 'rendered unconscious')"),
    (0x19298, "healMember", "adds random(min,max) hp to a party slot, clamped at max hp"),
    (0x192D5, "applyRepeatingHazard", "periodic area damage/heal using the stored hazard parameters (2E206/2F018/2EC04)"),
    (0x19335, "getMemberStat",
     "script stat read: 0 level(+4Eh),1 exp(+5Ah),2 hp(+44h),3 max hp(+46h),4 attribute,5 active skill,6 passive skill,7 cash,8 sex(+50h&3),9 profession(+48h)"),
    (0x1947F, "clampLong", "clamps a 32-bit value to [min,max] in place"),
    (0x194C2, "setMemberStat", "script stat write for the same selector set (level, exp (triggers level-up), hp, max hp, attributes, skills, cash)"),
    (0x1969F, "trySkillImprove",
     "increments the use counter of skill N (HDSPCT threshold at +13Ah); on overflow raises the skill if the governing attribute exceeds it; returns 1 if raised"),
    (0x1972A, "combineCondition", "script condition register combine: mode 0 set, 1 AND, 2 OR"),
    (0x19780, "promptYesNo", "writes 'Y)es  N)o' at (x,y) and waits for Y/N"),
    (0x1982E, "memberHasItem", "true if party slot arg0 carries item id arg1"),
    (0x198B1, "setPartyCellScript", "points dword_2E4C6 at the party's current cell and word_2EBF4/6 at its script word"),
    (0x198E3, "setCurrentMember", "word_22C8A = member pointer (target of '^' substitution in strings)"),
    (0x1987A, "showMessage", "shows script message line N and waits for the space key"),

    # -- pass 7: shops, services, medical, character sheet (1B2F0-1CBDF) --
    (0x1B2F0, "showMessageTimed", "short popup in window10 with a name/text, waits for a key then clears"),
    (0x1B360, "showShopHeader", "opens a shop/service screen: loads portrait set, draws title in window10"),
    (0x1B3DF, "restoreAfterShop", "re-draws the main windows after a shop/service screen"),
    (0x1B498, "showCashLine", "window10: '%s, you have $%lu'"),
    (0x1B5FF, "showCashAndRefresh", "showCashLine, refresh and switch to window7"),
    (0x1B621, "shopBuy", "buy menu: lists in-stock shop items, checks cash/inventory space, 'buys 1 %s.'"),
    (0x1B82F, "shopSell", "sell menu: lists member items the shop accepts, 'sells 1 %s.'"),
    (0x1BA26, "shopLoop", "script op 37: shop N loop with B)uy S)ell L)eave and F-key member switching"),
    (0x1BB93, "findFirstMemberBelowState", "first party slot whose getMemberCon is below the given state"),
    (0x1BBC8, "serviceLoop",
     "script op 45: pay-for services from the SERVICES table (levels, exp, hp, attributes, skills, cure); entries are 32 bytes"),
    (0x1BFCE, "printBeyondHelp", "'%s is beyond your help.'"),
    (0x1BFEE, "printEffortsUnsuccessful", "'Your efforts were commendable, but unsuccessful.'"),
    (0x1BFF9, "printSkillGained", "after trySkillImprove: '%s is now a level %d %Fs.'"),
    (0x1C052, "doctorTreat", "doctor skill: heals a downed member or cures an affliction with a skill roll; messages for success/failure"),
    (0x1C42B, "ingestItem", "'Who will ingest this?' consumable use (mutagens/anti-mutation effects, timers +53h..57h)"),
    (0x1C64A, "writeMemberCondition", "'Cond: Afflicted/Unafflicted' (optionally in brackets)"),
    (0x1C84A, "charInfoActiveSkills", "paged list of the member's learned active skills (window13)"),
    (0x1C8ED, "charInfoPassiveSkills", "paged list of passive skills (window14)"),
    (0x1C990, "charInfoInventory", "paged list of carried items (window15)"),
    (0x1CA0B, "charInfoConditions", "lists the member's afflictions with remission hours (window16)"),
    (0x1CAE6, "charInfoPageKeys", "left/right cursor keys switch pages of the character screen"),
    (0x1CB19, "printIncapacitated", "'%s is incapacitated.'"),
    (0x1CB58, "checkMemberConscious", "true if member hp >= 1, else prints incapacitated"),
    (0x1CB80, "selectMemberByFKey", "validates an F1-F5 key for a conscious party member and returns its slot"),
    (0x1CBDF, "chooseCommandTargetCell", "after a combat/field command needing a map target: askDirectionGetCell or pickMapCell"),

    # -- pass 8: roster screen, party editing, misc (1D0EE-1D8CD, 1xxxx leftovers) --
    (0x1D0EE, "drawRosterRow", "draws one roster row: F-key, name, condition, Con/AC, weapon and ammo"),
    (0x1D283, "drawRosterFooter", "roster screen footer: unique-item icons, cash, '(R)eorder roster (B)anish (L)eave'"),
    (0x1D2F9, "reorderRoster", "interactive reordering of the party order"),
    (0x1D4BF, "banishMember", "'Really banish %s?' then removeMemberFromParty"),
    (0x1D5BA, "rosterScreen", "main-loop 'A'/roster screen: F1-F5 character info, R reorder, B banish, L leave"),
    (0x1D6AA, "initItemSlot", "initialises a 6-byte inventory entry for an item id: ammo = weapon clip size if it is a weapon"),
    (0x1D712, "addNpcToParty", "recruits a monster/NPC record into a free roster slot, copying stats and inventory"),
    (0x1D8CD, "showEncounterPortrait", "script op 6 target: shows the monster's picture/name window and marks window state 2ED26=1"),
    (0x1D9A9, "initVideoAndCriticalHandler", "clears dirty list, calls the video-mode init and installs the critical-error handler; clears the screen buffer"),
    (0x1E045, "drawUniqueItemIcons", "draws a 16x16 icon for each bit set in disk1.field_7 (up to 4)"),
    (0x127CC, "afterRosterScreen", "redraws the game screen, applies readied weapon use and updates the map"),
    (0x127EE, "seedRngUntilKey", "calls rngByte while no key is pending (entropy for the PRNG)"),
    (0x11766, "startPendingFight", "after a script requested combat (word_2E418==8): sets up the monster as the target and calls runCombat"),
    (0x11834, "runScriptChain", "script driver: runs doScript repeatedly while word_2E418 requests continuation; handles combat requests"),
    (0x1189E, "useItemScriptOnCell", "runs the item's event script against the target cell; 'That has no effect.' otherwise"),
    (0x11920, "rollWeaponVsObject", "chance-to-hit roll for using a weapon on a map object; returns 0..6 (miss .. destroyed)"),
    (0x11A36, "printMemberUsesWeapon", "'%s %Fs...' line when a member uses a weapon on a cell"),
    (0x11A62, "useUpAmmo", "decrements the ammo (or removes the one-shot item) of a member's item slot and re-selects the readied weapon"),
    (0x11B1A, "printWouldntWork", "'You decide that it wouldn't work.'"),
    (0x11B2E, "useWeaponOnCell", "uses the readied weapon on a map cell (door/obstacle): resolves rollWeaponVsObject and runs the cell script"),
    (0x11CEA, "useCommandOnCell", "dispatch for field commands 0Ah/0Bh/0Eh against a target cell"),
    (0x11D6A, "chooseAmmoToLoad", "builds the list of compatible ammo in inventory and asks 'Load Which Ammo?'; returns slot, -1 cancel, -2 none"),
    (0x11ED8, "giveItemToMember", "moves an item between members ('has too much to carry already.')"),
    (0x11FA6, "askFixJammedWeapon", "'This weapon is jammed. Do you wish to fix it?'"),
    (0x11FB7, "itemActionMenu", "'Do what with your %s: U)se I)ngest D)rop T)rade E)quip' menu (mis-split into sub_1202D/12065/12074)"),
    (0x1251E, "setPartyPosition", "stores the party map position, resets hazard state"),
    (0x1253F, "movePartyStep", "moves/turns the party one step in a direction (arrow key): bounds, passability, scripts, random encounters"),
    (0x12A34, "setListMenuMode", "switches list-menu windows between narrow and wide layouts"),
    (0x10C62, "findFreeCellNear", "finds a free adjacent cell using the direction table, used when an entity is pushed aside"),
    (0x200D8, "directionTowards", "returns a direction table offset (0..21 step 3) from (x,y) towards/away from the party"),
    (0x20182, "isItemEquipped", "true if item id arg1 is any of the member's equipped slots (+4Ah, +4Bh..4Dh)"),
    (0x201C8, "tickMonsterStates", "decays the monster state byte (+5Dh) each tick"),
    (0x2023E, "getCombatantInfo", "sets word_2DFFC/E to the name string and 2E43F/2E440 to range/speed for a combatant record"),
    (0x202F2, "getCombatantPos", "returns hp and x,y (via pointers) of a combatant record"),
    (0x203C9, "getMaxWeaponRange", "max over living party members' ready weapon range"),
    (0x204C4, "applyMapChangeList", "applies the persistent map change records (word_21326) for the map being loaded"),
    (0x2060D, "removeItemSlot", "removes inventory slot (or the ready-weapon reference) from a member and compacts the 6-byte table"),

    # -- pass 9: leftovers, mis-split fragments, runtime --
    (0x103BF, "fileChunkIO", "args(file, idx, table, mode, buf): seeks to table[idx].offset<<4 and reads (mode 0) or writes (mode 1) table[idx].size bytes"),
    (0x10444, "readFileBlock", "args(file, dst, nwords): opens file, reads nwords*4 bytes"),
    (0x1E448, "fileWrite", "DOS write (40h) in 64K-1 chunks across segments; returns 1 on success"),
    (0x1E498, "fileSeek", "DOS lseek (42h): handle, hi, lo, whence"),
    (0x107CB, "lootPartyCell", "if the 'item flag' byte_2EE06 is set, runs lootCell at the party position"),
    (0x13716, "postCombatCleanup", "mis-split head of the loop that finalises groups after combat (calls dropPacketLoot / clears map cells)"),
    (0x1DA8C, "drawBorderSetUnused1", "two vertical border segments (unused)"),
    (0x1DA9B, "drawBorderSetUnused2", "horizontal border segment (unused)"),
    (0x1DAF0, "drawListMenuBorders", "draws the two vertical border segments used by list menus"),
    (0x1DAFF, "drawCombatListBorders", "draws the two vertical border segments used by combat lists"),
    (0x1DDC5, "writeStringRow", "writes a 0FFh-terminated row (x,y,text...) into window3 using attribute 4"),
    (0x1DF22, "readScriptByteFar", "reads a byte at es:di and advances di with segment carry"),
    (0x1DF48, "readScriptRecord",
     "decodes one script record: low nibble of the first byte = event class; if it equals arg2, reads opcode+parameters via the "
     "decoder lists (off_22C2E); else skips opcode+length (table 22BC1)"),
    (0x1DFC6, "setPartyFacing", "sets party facing 1-4 (5=no change) and the matching sprite index (table at dseg+1A36h)"),
    (0x1DFEE, "drawPartySprite", "draws the party icon tile at the centre of the map view (9,4) and refreshes"),
    (0x1E5E2, "criticalErrorMessage", "calls showMessageBox for the DOS critical-error handler"),
    (0x1E887, "_callExitProcs1", "CRT: calls near exit procedures between si and di"),
    (0x1E896, "_callExitProcs2", "CRT: calls far exit procedures between si and di"),
    (0x1EBC0, "__dosmaperr_helper", "CRT: maps a DOS error code to errno"),
    (0x1F838, "_write_flush", "CRT: DOS write helper of _write"),
    (0x1FAF4, "__brk_setblock", "CRT: INT 21h 4Ah memory resize helper of _brkctl"),
    (0x1FBE2, "_toupper", "table-driven toupper (ctype flag 2 = lowercase)"),
    (0x20000, "getWeaponRangeBand", "returns 1+index of the range band (per-distance byte table of the weapon fire mode) used for the target distance"),
    (0x20734, "initVideoScanlines", "far; resets the dirty list, builds the 200-entry scanline offset table (160 bytes/line) and calls the video-mode routine"),
    (0x1DBF9, "writeString_markBreak", "mis-split entry inside writeString: records the current position as the end of the word run"),
]

# (dseg offset, new_name, note)
GLOBALS = [
    (0x1A2E, "_map", "far ptr to the loaded map (w,h @0/2; flags @5; cells at +0x414, 8 bytes each)"),
    (0x1A04, "_textSpeed", "scroll delay level 1-8"),
    (0x1834, "_pageOnOverflow", "nonzero: pause at end of window instead of scrolling"),
    (0x1A4A, "_caretSubstitution", "ptr to the string that replaces '^' in writeString"),
    (0x1824, "_rngState", "16-byte PRNG state used by rngByte"),
    (0xA7D0, "_dirtyRectNext", "next free entry in the dirty-rect list"),
    (0xA7CE, "_dirtyRectCount", "number of queued dirty rects"),
    (0xA7D2, "_dirtyRectCursor", "read cursor while blitting"),

    (0xD206, "_monsters", "far ptr: map's monster records (68h bytes each)"),
    (0xD9D4, "_monstersEnd", "near ptr: end of monster records / start of group records"),
    (0xD1DE, "_groups", "far ptr: enemy group records (66h bytes each, up to 8 sub-slots of 16h bytes)"),
    (0xD9DA, "_groundItems", "far ptr: ground-item records (6 bytes: qty, x, y, item)"),
    (0xD9B8, "_shopsBase", "far ptr: shop table for the map"),
    (0xCFC0, "_curShop", "far ptr: current shop record while shopping (2Ch + 10h*items)"),
    (0xD494, "_combatants", "combatant table: 12-byte records (type,idx,sub,...,command,...), count in _combatantCount"),
    (0xDDDA, "_combatantCount", "number of combatants in the current fight"),
    (0xD1EA, "_targetHpPtr", "far ptr to the current target's hit-point word"),
    (0xD1D8, "_scriptRequest", "script engine request code returned to the caller (8 = start combat, 6 = fight scripted monster, -1 = none)"),
    (0xD9B4, "_scriptPtr", "far ptr to the script word being run"),
    (0xD286, "_curCell", "far ptr to the current/target map cell"),
    (0x00E6, "_mapChangeList", "near ptr: persistent map change records (12 bytes each; first word = count)"),
    (0x00EC, "_curRegion", "current region/disk index (selects the file set)"),
    (0x00EA, "_curMapNo", "current map number"),
    (0xDAE6, "_npcDialogMode", "nonzero while a monster/NPC conversation window (window7) is active"),
    (0xBEE6, "_animCount", "number of active map animations"),
    (0xCFC4, "_timeTicks", "minutes counter incremented by setTime"),
]


def apply():
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if cur == name:
            continue
        if DRY_RUN:
            print("[dry] %X %s -> %s" % (ea, cur, name))
            continue
        if not idc.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED rename %X %s -> %s" % (ea, cur, name))
        else:
            idc.set_func_cmt(ea, note, 1)
            print("renamed %X %s -> %s" % (ea, cur, name))
    for off, name, note in GLOBALS:
        ea = DSEG_BASE + off
        cur = idc.get_name(ea)
        if cur == name:
            continue
        if DRY_RUN:
            print("[dry] data %X %s -> %s" % (ea, cur, name))
            continue
        if not idc.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED data %X %s -> %s" % (ea, cur, name))
        else:
            idc.set_cmt(ea, note, 1)
            print("renamed data %X %s -> %s" % (ea, cur, name))


apply()
