# KEH.EXE — engine notes

Findings from the first full naming pass over `KEH.EXE` (2026-10-21). All 355
functions now have names, but **most were named from a single read of the
disassembly** — treat names as "probably right" until confirmed by the C++
reimplementation. Fragments IDA mis-split into separate "functions" carry a
`_partN` suffix. Address notes use the IDA linear form (`1008C` = offset in the
code segment); data offsets are dseg-relative as seen in the disassembly.

Source of truth for every rename + justification: `ida_scripts/apply_renames_keh.py`.

## Runtime layout

* 16-bit real-mode MS-C program, **Tandy/EGA-style 320x200x16** presentation:
  all drawing goes to an off-screen 4bpp buffer (160 bytes/scanline, segment
  `_picDecoded_0`), then a *dirty-rectangle* list is blitted to `B800h`
  (`addDirtyRect` → `flushDirtyRects`/`blitDirtyRects`). Text is 40x25 cells of
  8x8 glyphs (`FONT`, 32 bytes/glyph, `writeChar`); map tiles are 16x16 4bpp
  (128 bytes, `drawTile`, tile word low 11 bits, high bits = overlay).
* Input: `getKeypress` returns ASCII (upper-cased), F1–F5 = 3Bh–3Fh,
  arrows `0FDh` up, `0FCh` down, `0FBh` left, `0FAh` right, ESC `0FFh`,
  Enter/Space `0FEh`. F10 toggles sound. While waiting it calls
  `animationTick`.
* Sound: PC speaker only (`playSound`, scripts in a table, stepped per vertical
  retrace by `soundTick`).
* Random numbers: 16-byte additive-carry generator (`rngByte`), range helper
  `randomRange(min,max)`.
* Compression: all big resources are **LZSS** (4096 ring, init pos `0xFEE`,
  12-bit offset / 4-bit length+3) — `lzssDecode`.
* Disk handling: `openGameFile` loops "Please insert %s." until the file opens
  (the original shipped on 4 data disks). Chunked reads use
  `fileChunkIO(file, idx, table, mode, buf)` where `table[idx] = {offset>>4,
  size}`.

## Startup (`_main`)

`gfxInit` (allocs one big arena and slices it into `_picDecoded`, `_services`,
`_archetypesData`, `_hdsPct`, `_fontData`, `_borders`, map buffer, 3 portrait
buffers…) → `loadData` (TILES?, FONT, BORDERS, PACKETS, ARCHTYPE, SERVICES,
HDSPCT, WEAPONS, GLOBALS) → `loadSaveState` (DISK1..3, newest wins) →
`loadMap` → draw frame → command loop:

| Key | Action |
|---|---|
| arrows | `movePartyStep` |
| F1–F5 | `characterInfo` for that member |
| A | `rosterScreen` (F-keys, R reorder, B banish) then `afterRosterScreen` |
| E | force a combat check (`buildCombatants` → `runCombat`) |
| Q | quit menu (1 quit without saving, 3 save+quit) |
| (none) | `endOfTurn(0)` — time passes, monsters move, encounter checks |

## Data files seen by KEH

`DISK1`(save, `Disk1` struct, 0xEC0 bytes) · `FONT` · `BORDERS` (5000) ·
`PACKETS` (loot tables) · `ARCHTYPE` (professions + enemy templates) ·
`SERVICES` · `HDSPCT` (status effects/skill tables, 1180 bytes) · `WEAPONS` ·
`GLOBALS` (item table) · per region: `?MAP`, `?SCR`, `?ANI` and `?PICT`
(regions: Disk1 `KMAP/KSCR/KANI`, Disk2 same names, Disk3 `FMAP/…`, Disk4
`DMAP/DSCR/DANI`; picture sets `DPICT/KPICT/EPICT`; end screens via the table
at dseg+0EFEh). Region record = 5 near pointers (disk prompt, volume label,
map, script, animation file name) at dseg+0F4h, 10 bytes each; map→region table
at dseg+125h.

## Key data structures (offsets verified in code)

### Map (decoded `?MAP` chunk, pointed at by `_map`)

| Off | Meaning |
|---|---|
| +00 | width (u16) |
| +02 | height (u16) |
| +04 | u16; bit15 = "reset when left" (reloaded by `reloadMapIfResettable`); low bits = off-map fill tile |
| +05 | (high byte of the above) bit 80h = *modified* flag (`markMapDirty`) |
| +06 | u8 minutes per step |
| +07 | u8 random-encounter chance % |
| +08 | u8 number of group templates for random encounters |
| +09 | u8 monster count |
| +0B | u8 group count |
| +0D | u8 string index shown when walking off the map |
| +1E | word: pointer to a stored-member block (`retrieveStoreMember`; members are 0x14C-byte copies at +20h…) |
| +404/406/408/40A/40C | offsets of shops / monsters / monsters-end / groups / ground items |
| +40E/410/412 | ground-item count / limit / free gap |
| +414 | cell array, `width*height` cells of 8 bytes, row-major |

Cell (8 bytes): `+0 u16` tile (low 11 bits = tile index, high 5 bits = overlay
entity id drawn on top), `+2 u8` flags (bit0 passable, bit6 40h cursor
highlight, bit7 80h "has items/seen"), `+3 u8`, `+4 u16`, `+6 u16` **script
index** (0FFFFh = none).

Monster record (0x68): shares the first 0x54 bytes of `PartyMember`
(`addNpcToParty` copies them). Notable: `+1A` speed attr, `+44/46` hp/max,
`+48` profession, `+49` roster id, `+4A` readied weapon slot, `+52` item count,
`+53…` item ids, `+5B/5C` map x,y, `+5D` state/morale byte, `+5F` aggro radius,
`+60/61` target x,y, `+63` portrait, `+64` movement accumulator, `+65` overlay id.

Group record (0x66): `+0/1` x,y (template spawn), `+2/3` current x,y, `+4/5`
size/…, `+8` sub-slot count, `+9` active-slot bitmask (low nibble) / flags,
`+0A` movement accumulator, `+0B` speed, `+0D` spawn delay, `+0E…` up to 8
slots of 0x16 bytes (`+0` enemy template id, `+2…` per-member hp words,
`+0F` live count, `+10/11` target x,y, `+12` spread%).

Ground item (6 bytes): `u16 qty, u8 x(+2), u8 y(+3)` flag bit 7 of `+2` =
hidden, `+4` item id.

Combatant (12 bytes, `_combatants`): `+0` type (0 party, 1 monster, 2 group),
`+1` index, `+2` sub-slot, `+3..5` target (type, idx, sub), `+6` command,
`+7` speed, `+8` range-to-target… sorted by speed with `_qsort`.

PartyMember (0x14C): see the IDB struct. Confirmed extra fields: `+1A` speed,
`+1B` toughness (hp bonus above 15), `+21` remission timer, `+22/23` last
skill-up pair, `+44/46` hp/max, `+4A` readied weapon slot, `+4B..4D` armor
slots, `+4E` rank, `+50` sex(2 bits)/flags, `+52` item count, `+53..56` pending
mutation (type, mask, weekday, hour), `+57` mutagen timer, `+58` status bits
(bit i = affliction i, bit7 = mutation), `+5A` exp (u32), `+5E` exp-to-next
(u32), `+62` 32 inventory slots of 6 bytes `{item, loaded-weapon idx, ammo,
flags}`, `+122` last-hurt hp, `+124/+134` skill-use counters, `+144` combat
command, `+145/146/147` command args, `+148` far ptr target cell.

Disk1 (save): `+0` u32 save counter, `+4` region id, `+6` gfx mode, `+7`
unique-item bitmask, `+8/9` hour/min, `+0A/0C` party map x/y, `+0E` cash (u32),
`+12/1C/26` per-level map no./x/y stacks (10 deep), `+30` stack depth,
`+31` party count, `+32` roster ids, `+37` weekday, `+38` facing/sprite,
`+3A` roster 5x0x14C.

### Items (`GLOBALS`, 0x18 bytes each, header: +0 `u8` scale A, +1 `u8` scale B, +2 count, +4/+6 w/h… )

`+0..` name at `+2Ah` (17 chars), `+3Eh` flags (1 = weapon-capable, 2 = dead?,
1Ch bits = unique), `+3Fh` weapon-def index / AC value, `+41h` unique-flag bit.

### Weapon defs (`WEAPONS`, 0x3C each + fire modes 0x12 each)

`+18h` flags (1 = ammo-using, 2 = ranged/consumable), `+1Ah` jam %, `+3Ah`
fire-mode table offset (count at +3Ah of the entry header), per fire mode:
`+0Ah/0Ch` damage min/max, `+0Eh` ammo-type, `+0Fh` clip size, `+10h` burst
shots, per-distance to-hit bytes at `+(dist-1)`.

## Combat

`runCombat` — planning phase: for each living member prompt **R**un / **E**vade /
**A**ttack / **L**oad-unjam / **W**eapon / **U**se; commands stored at member
`+144h`:

| Code | Meaning |
|---|---|
| 0 | get closer (everyone advances) |
| 1 | evade |
| 2 | shriek (mutation skill, skill[0Dh]*15%: scares later combatants, cmd := 0Eh) |
| 3 | stun (mutation skill [0Eh]) |
| 4 | attack |
| 5 | reload (swap ammo from item slot `cmdArg`) |
| 6 | unjam |
| 7 | switch weapon |
| 8 | (character-info open, internal) |
| 9 | (ingest, internal) |
| 0Ah/0Bh | use item / skill on target |
| 0Ch | trade item |
| 0Dh | drop weapon |
| 0Eh | none/acted |
| 0Fh | run (chance from party level-unit and speed bonus) |

Execution order = combatant table sorted by command code ascending (`cmpCombatantsByCommandAsc`), ties by speed descending (second half of `runCombat`). Attack
resolution: `memberAttack`/enemy AI → `resolveAttack` → `rollAttack` (to-hit
from armor class × scale vs weapon accuracy + attributes, per-shot loop for
bursts/auto; damage reduced by armor; kills grant profession xp).
`checkWeaponMalfunction` handles jam/break/dud. After combat the encounter
cleanup removes dead groups (`removeMapEntity`) and drops loot packets.

## Script VM (`doScript`)

Every map cell has a script index; scripts live in `?SCR` (decoded, table
`dword_2EE12`). A script block: `+0 u16 event-class bitmask`, `+2 u16` offset of
the string area, `+4…` records. Record = `u8 head` (low nibble = event class,
high nibble = parameter signature), `u8 opcode`, params. `readScriptRecord`
decodes the parameters through the signature's decoder list (`off_22C2E`);
records whose class ≠ the requested event are skipped using the per-opcode
parameter-byte table at dseg+0F81h:

```
op: 00-0F  4 2 1 1 3 3 2 2 0 3 2 2 4 2 0 1
    10-1F  1 3 1 4 3 2 0 1 3 2 4 2 5 7 5 1
    20-2F  2 0 3 2 2 1 0 1 0 4 2 2 2 1 0 3
    30-3F  0 0 2 0 4 2 1 1 2 1 3 3 0 2 1 3
    40-4B  2 2 2 0 0 1 3 4 0 3 1 1
```

Event classes seen (`doScript` arg4): 0 = walk-on/enter, 1/2/3 = item-use,
combat and skill events, 4 = skill check context, 6 = fight monster, 7 = hire
request (NPC dialog menu "H)ire A)ttack U)se L)eave").

Opcodes (handler address in IDA; confidence H/M/L):

| Op | Effect |
|---|---|
| 01 | accumulator arithmetic: p0 0 +=reg,1 +=imm,2 -=reg,3 -=imm,4/5 divide(M),6/7 multiply(M) |
| 02 | accumulator/register move (0 reg=acc,1 acc=reg,2 reg=0,3 reg+=acc,4 reg-=acc) |
| 03 | acc = imm |
| 04 / 05 | `getMemberStat` → acc / `setMemberStat` (levels, exp, hp, attributes, skills, cash) |
| 06 / 07 | max of a stat over the party / min over living members |
| 08 | acc = cash |
| 09 | cond = member has item |
| 0A | cond = unique-item flag set / any member has item |
| 0B | item housekeeping (remove items by flag/id, 4 modes) |
| 0C | change map ("Enter new location?"): push/pop/jump on the map stack |
| 0D / 0E / 43h(67) | gosub (push return + member), next-member loop, return |
| 0F / 10h | text input prompt / compare typed word with script string |
| 11h/46h(70) | damage party now (46h ignores armor) |
| 12h | heal every member by random(min,max) |
| 13h | compare (acc vs value, relation = p0 mod 6: = > < ≤ ≥ ≠) / flag combinations |
| 14h/15h/16h | IF / ELSE / ENDIF with nested condition stack |
| 17h | print string with paging |
| 18h | drop loot packet at coordinates |
| 19h–1Bh | edit current cell: tile / fields / script word |
| 1Ch–1Eh | same on any map location, persisted in `_mapChangeList` |
| 1Fh | `playSound` |
| 20h | acc = random(p0, p1) |
| 22h | skill/attribute check (p0 <0Ch attribute, <1Ch active skill, else passive), improves skill |
| 23h/24h | condition: readied weapon / used-item match |
| 25h | `shopLoop` |
| 26h | hide party sprite |
| 27h | Yes/No prompt |
| 28h | restore party position |
| 2Ah | set monster state byte |
| 2Bh/2Ch | party-member presence / left-party tests |
| 2Dh | `serviceLoop` |
| 2Eh | acc = hour*100+minute |
| 2Fh/30h/31h | open NPC menu / leave / return |
| 32h | reveal hidden ground items at coordinates |
| 34h/4Bh | move or remove a monster by id |
| 35h | spawn enemy group |
| 36h | NPC joins party (`addNpcToParty`) |
| 37h | condition = last key == p0 |
| 38h | give item (`initItemSlot`) |
| 39h | `showMessage` |
| 3Ah | set monster hp/state |
| 3Bh | remove item |
| 3Ch | acc = weekday |
| 3Dh | condition: group alive |
| 3Eh | push party one step |
| 3Fh/40h/41h | test / set / clear status bit |
| 42h | cond = member has a free inventory slot (counts used slots, < 32) |
| 44h | end of script |
| 45h | set party facing |
| 48h | `retrieveStoreMember` |
| 49h | set cell high bits |
| 4Ah | `showEndingScreen` |

(The address-keyed jump table is at dseg:`jpt_19A29`; handlers share code, e.g.
`cases 0,41,71` and `17,70`.)

## IDB structs (second pass)

`ida_scripts/apply_structs_keh.py` defines `MapHeader`, `MapCell`, `Monster`,
`EnemyGroup`, `GroupSlot`, `GroundItem`, `Combatant` (also the 6-byte attack
record), `EnemyDef`, `ItemDef`, `WeaponDef`, `FireMode`, `Shop`, `ShopRow`,
`ServiceRow`, renames `PartyMember`/`ItemSlot` fields, then infers a struct for
each `[reg+disp]` base pointer (stack var / global / party table) from the
displacements used and applies `op_stroff` (~900 operands typed). Ambiguous
pointers are skipped unless listed in `FUNC_HINTS`; the decisions are written to
`ida_scripts/structs_report_KEH.txt`. Remaining untyped accesses are mostly
`[reg+reg+disp]` array indexing and stack variables reused for several types.
Fragment merges are in `ida_scripts/fix_funcs_keh.py` (also recovers the three
qsort comparators at 15335/1534C/15363).

## Pictures and animation (`*PICT`, `*ANI`)

* `TPICT`/`FPICT`/`DPICT`/`KPICT`/`EPICT` are single LZSS streams that decode to 32000 bytes: a 320x200 4bpp
  screen (high nibble = left pixel). `TPICT` is the title; the other four are the ending screens used by script
  op 74 (`showEndingScreen`, file table at dseg:0EFE; ending 3 prints "See you later...!" and quits).
* `KANI` (disk 1/2 maps), `FANI`, `DANI` are 53-entry archives (same `{u16 paragraph offset, u16 size}` TOC as
  the map/script files, LZSS chunks). Chunk index = **picture id**; ids come from `Shop+0x28`, `ServiceHeader+0x28`,
  `Monster+0x63` and the enemy template (`EnemyDef+0x3E`, carried in `Combatant.b11`); ids 0x2D..0x32 are the six
  mutation pictures shown by `checkTimedEffects`.
* A picture chunk is a *picture set*: byte 0 = frame count, bytes 1-2 = offset of the animation script, then a u16
  offset table at +3 (relative to the chunk start). A frame is `{widthBytes, height, xOffset, yOffset}` followed by
  4bpp rows (`blitPicFrame` adds the offsets to the window origin). Frame 0 is the whole 96x88 picture, the others
  are small overlays (eyes, mouths...).
* The animation script is a list of `{count, frame_0..frame_count-1}`; `animationTick` (every 3 BIOS ticks,
  ~165 ms) draws the frames of the next entry; `0xFF` restarts at the offset in the header.
* Picture windows (dseg:1E74 table of Window pointers): left (8,8), middle (112,8), right (216,8), each 96x96.
  Shops/services/NPC talk use the left window with the text window beside it (`window7`, cols 14-38); combat shows
  up to three distinct enemy pictures (1 picture -> middle, 2 -> left+right, 3 -> middle,left,right).
* `loadMapPicSet` is misnamed in the IDB: it reads chunk `mapNo-1` of the region's `*SCR` file (the script bank).

## Hourly effects (`checkTimedEffects`)

Called by `setTime` when the hour changes outside combat. Per living member: `mutagenTimer` (+57h) counts down,
`remissionHours` (+21h) counts down (reaching 0 triggers an event); otherwise the event fires when the member has
`mutPending`/`hasMutation` and `mutWeekday`/`mutHour` equal the clock. The event applies a new mutation (status bit
0x80, attribute `{2,1,3,4,6,0}[m]` -2 if >= 3, luck-slot attribute 5 -1) or just "acting up again" (healing
impaired), and always bumps one of skills `{0Ch,0Dh,0Eh,1Fh}` by one; afterwards `mutWeekday = (weekday+2) % 7`.
