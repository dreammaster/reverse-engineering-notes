# Might and Magic III compared with Xeen (Clouds / Dark Side / World of Xeen)

Sources of the comparison: the ScummVM Xeen engine (`engines/mm/xeen`, `engines/mm/shared/xeen`), the Xeen disassembly `mm45/xeen_dat.asm`,
and what this directory decoded from `MM3.EXE` and its data.  Statements marked **(checked)** were verified by comparing the actual
tables / code of both games; the rest come from reading both code bases.

**Summary:** MM3 and Xeen are two stages of one engine.  The container format, the text engine, the character record, the combat and time
rules and the character tables are the same or nearly so.  Xeen added a second world, 88 awards, a few new spells and monsters, a
bit-packed map flag word, "flipped/resized" sprite flags and a larger AdLib instrument format, and organises the inventory, the spell
lists and the outdoor maps differently.

## Container and resources

| | MM3 | Xeen |
|---|---|---|
| archive | `.CC`: u16 count + 8-byte entries, **same cipher** (`rol2(c)` plus `0xAC + 0x67*i`, which is MM3's `-(0x54 + 0x99*i)` mod 256) **(checked against `BaseCCArchive::loadIndex`)** | identical |
| entry id | 16-bit rotate-add name hash | identical (`convertNameToId`) |
| entry payload | **LZHUF** (`fill, fill, u16 BE size, stream`) | stored; data are XOR 0x35 (`cc_archive.cpp`) |
| sprites | u16 frame count, per frame two u16 offsets (**two layers**) | identical (`_offset1`, `_offset2`) |
| saves | `MM3.CUR` is itself a CC archive (maps, events, roster, party) | `.sav` / `.cur` style, serializer |
| external drivers | `*.DRV` (video module + sound drivers) loaded from the archive | integrated in the engine |

## Character record and rules

* Same enums **(checked against `character.h`)**: ten classes in the same order (Knight, Paladin, Archer, Cleric, Sorcerer, Robber, Ninja, Barbarian, Druid, Ranger), seven attributes in the same order, 16 conditions in the same order (cursed, heart broken, weak, poisoned, diseased, insane, in love, drunk, asleep, depressed, confused, paralysed, unconscious, dead, stone, eradicated), 18 skills in the same order (Thievery ... Danger Sense), two-valued attributes (permanent, temporary).
* **Race order differs:** Xeen Human, Elf, Dwarf, Gnome, Half-Orc; MM3's name table Human, Elf, **Gnome, Dwarf**, Half-Orc.
* Tables identical **(checked against `xeen_dat.asm`)**: `STAT_VALUES`, `STAT_BONUSES`, `BASE_HP_BY_CLASS`, `CLASS_EXP_LEVELS` (1500, 2000, 2000, 1500, 2000, 1000, 1500, 1500, 1500, 2000), `AGE_RANGES` and `AGE_RANGES_ADJUST`.
* Formulas identical **(checked against `Character::getMaxHP` and the experience functions)**: `HP = max(1, base + statBonus(endurance) + race + bodybuilder) * level + itemScan(7)`; experience `M << shift` with 1,024,000 per level above 12; stored experience is the part above the level base.
* Award storage: Xeen `_awards[128]` (88 used, `AWARDS_TOTAL`), MM3 **26 award bytes** (the first five are the guild memberships, a few are counters).  The inventory: Xeen four categories of nine slots, MM3 eighteen slots as parallel byte arrays 19 bytes apart with 1-based item ids (Xeen ids are 0-based).
* Spell flags: Xeen `SPELLS_PER_CLASS = 39` per school, MM3 36 flags indexed by position in the class's school list.

## Items

* Weapons **(checked)**: `WEAPON_DAMAGE_BASE` and `WEAPON_DAMAGE_MULTIPLIER` are identical for all 33 MM3 weapons (ids 1-33 = Xeen ids 1-33: same dice, same names order); MM3's armour strengths follow with one extra armour type.
* Materials **(checked)**: `METAL_DAMAGE`, `METAL_DAMAGE_PERCENT`, `METAL_LAC` and `ATTRIBUTE_BONUSES` are identical to Xeen's but stored **1-based** in MM3 (index 0 is "none"); MM3 has the same 22 metals and 10 attribute-enchant tiers per group.
* Random items: Xeen `makeItem` uses `MAKE_ITEM_ARR2` (the tier tables `(1,3)(2,5)(3,6)(4,7)(5,8)(8,8)` ...), MM3's `generateItem` uses the same shaped tables for element/attribute/metal (`rules.md`, `items.md`).

## Spells (checked)

* Xeen has 76 spells, MM3 77.  **65 are shared** by name; MM3-only: Acid Stream, Create Rope, Detect Magic, Disintegrate, Dragon Breath, Duplication, Feeble Mind, Half for Me, Immobilize, Nature's Gate, Paralyze, Silence; Xeen-only: Beast Master, Clairvoyance, Day of Protection, Day of Sorcery, Dragon Sleep, Golem Stopper, Hypnotize, Insect Spray, Item to Gold, Poison Volley, Shrapmetal.
* **Spell point costs are identical for all 65 shared spells**, including the "n x level" encoding (`65534` = 2/level in Xeen's word table, `-2` in MM3's).  **Gem costs are identical for 58 of the 65**; the seven that differ: Cure Wounds 1 (MM3) vs 0, Prot. from Elements 2 vs 1, Blessed 0 vs 1, Holy Bonus 0 vs 1, Time Distortion 3 vs 0, Etherealize 8 vs 10, Sun Ray 10 vs 20.
* Both have three schools (Clerical, Wizardry, Druidic).  Xeen stores 39 spells per school as an alphabetical index list (`SPELLS_ALLOWED`), MM3 36/36/29 spells in **learning order grouped by spell level** with per-level counts; the sets overlap (clerical 32 of 36, wizardry 29 of 36, druidic 17 of 29).
* Xeen casting is the same dialog-then-effect structure: the exploration-only spells (Jump, Teleport, Lloyd's Beacon, Town Portal, Super Shelter, Time Distortion, Etherealize) check a per-map permission; in MM3 these are separate header bytes, in Xeen the restriction bits of `MazeFlags`.

## Combat and monsters

* `hitMonster`, `getWeaponDamage` and the shape of `doMonsterTurn` are the same functions (BinDiff similarity >= 0.9); MM3 stores its 90 monsters as one column file per field (`MON*.DAT`, 22 files).
* **Damage types 0-7 are identical** (physical, magical, fire, electrical, cold, poison, energy, sleep) **(checked)**; beyond that MM3 has Immobilize (8), Feeble Mind (9), Paralyze (10), Finger of Death (11), Holy Word (12), Mass Distortion (13), Turn Undead (14), Disintegrate (15), Silence (16) where Xeen has Finger of Death (8), Holy Word (9), Mass Distortion (10), undead (11), Beast Master, Dragon Sleep, Golem Stopper, Hypnotize, Insect Spray, Poison Volley, Magic Arrow.
* **Special attack numbering differs** (MM3 16 values: poison, sleep, insane, disease, confusion, curse, break armour, weaken, SP drain, paralyse, age, knock-out, eradicate, stone, death, curse items; Xeen 22 values starting with the elements).
* Monster targeting: `MONATTP` classes (cleric, sorcerer, druid, paladin, race) correspond to Xeen's `_hatesClass`; Xeen adds `HATES_PARTY`/`HATES_NOBODY` and the dwarf class 12.
* MM3 specifics (not compared with Xeen): combat is started from `drawView` when monsters stand within three cells, monsters move on a 32x32 occupancy grid (`moveMonsters`), and a fleeing character leaves the fight individually.

## Maps, events and scripts

* Same map model **(checked against `MazeData`)**: 16x16 cells with wall data + a cell flag byte, neighbour map ids for the four directions, and a header of per-map rules; MM3 stores them as 64 header bytes (the neighbour ids at `08`-`0B`, run chance `07`, door/box unlock difficulties `11`/`12`, three bash difficulties `1B`-`1D`, trap damage `1E`, tavern tip pointer `10`, one permission byte per teleport-type spell `14`-`1A`, plus lit/rest/save/dismiss flags `0C`-`0F`), Xeen as `MazeDifficulties` (`_chance2Run`, `_unlockDoor`, `_unlockBox`, `_bashDoor`, `_bashGrate`, `_bashWall`), `_trapDamage`, `_tavernTips`, a flag word with restriction bits (`RESTRICTION_TELPORT`, `_TOWN_PORTAL`, `_SUPER_SHELTER`, `_TIME_DISTORTION`, `_LLOYDS_BEACON`, `_ETHERIALIZE`, `_REST`, `_SAVE`), `_mazeFlags2` (outdoors/dark) and `_wallTypes`/`_surfaceTypes` tables.
* MM3 additionally pages a 32x32 world from four 16x16 pages (`Maze_curSlot`) with a world-wrap flag for the overland maps; Xeen outdoor maps are single 16x16 maps joined by the same "surrounding mazes" ids.
* **Script opcodes are the same table**: MM3 uses the Xeen numbering 1-33 (Display1 ... WhoWill) with the same operand style; opcode 33 is `TakeOrGive` in MM3 and `RndDamage` in Xeen; Xeen continues to ~58 (MoveWallObj, AlterCellFlag, IfMapFlag, GiveEnchanted, FlipWorld, PlayCD ...).
* **`ifProc` value modes share their numbers** **(checked against `Scripts::ifProc`)**: 3 sex, 4 race, 5 class, 8 HP, 9 SP, 10 AC, 11-13, 15 award, 16 experience, 17, 18 condition, 19 spell, 20 flag, 21 item, 25 minutes, 34 gold, 35 gems, 37-43, 44, 45-65 attributes/resistances, 69-73, 76-79, 81, 84-94, 99.  MM3-only: 6 alignment, 23 (event byte flags / hireling flags), 66 random item, 67 learn spell, 74/82 party-effect and damage modes, 95-98 (counters in the award bytes); Xeen-only: 102-107.
* **Modes 78 and 81 have opposite sense**: in Xeen `Scripts::ifProc` returns 1 when current HP (SP) is **at or below** the maximum; MM3 returns 1 when it is **above** the maximum **(checked against both disassemblies)**.

## Text and drawing

* The text engine's control codes are the same **(checked against `FontSurface::writeString`)**: 01/02 font size, 03 + letter justification, 04 box/bar of a given width, 06 non-breaking space, 07 background colour, 08 outlined glyph, 09 x, 0A newline, 0B y, 0C colours.  Code 05 is **a hook in both**: MM3's video module runs a draw list (`view.md`), the ScummVM font code ignores it (`c == 5: continue`) because ScummVM draws the scene itself.
* Scene drawing: both games build a flat list of (sprite sheet, x, y, flags, frame) records, one writer per viewing distance, and draw it back to front.  Xeen's flags: mode field `0xF00` (drawers 1-7), flip `0x8000`, bottom/scene clipping, resize `0x10000`; MM3's: scale `100h`/`200h`/`300h`, mirror bit 0, variant bit 1, enlarge bit 15.  MM3's wall artwork is one 13-to-31-frame sheet per viewing distance and environment (`view.md`).
* Sound: the same command-per-byte AdLib sequencer (`high nibble = command, low nibble = channel`) with wait/instrument/note/volume/slide commands; MM3's instruments are 14 bytes (music) and 11 bytes (effects), Xeen's 26.  MM3's effect streams are stored inside the driver file (`music.md`).

## Time and economy

* Identical time model **(checked against `Party::addTime` / `changeTime`)**: 1440 minutes a day, a 100-day year, a condition tick whenever the 480-minute boundary is crossed.
* The town building set (bank, smithy, guild, inn, tavern, temple, training) is the same in both games; MM3's night closing (`Town_closed`), its 10-day interest and restock cycle and the roster rule were not compared with Xeen.

## What exists only in MM3

Hirelings as roster slots 20-29 with event-controlled availability, the Arena map, the "stored at this inn" roster rule, the 32x32 paged overland, the twelve MM3-only spells (Immobilize, Paralyze, Feeble Mind, Silence, Disintegrate, ...), the Sound Blaster driver's timer-driven PCM and the intro samples `S1.S`-`S7.S` (as far as the comparison above went).

## Practical use

Because the tables are shared, the MM3 tables in `names/mm3.tsv` can be cross-checked against `xeen_dat.asm` by name (the BinDiff matches that seeded this database already did so for the functions); wherever a Xeen routine has a ScummVM C++ version (`Character`, `Party`, `Combat`, `Scripts`, `Spells`), the MM3 routine is a good candidate for the same structure, with the differences listed above.
