# Implementing MM2 in ScummVM — where each piece is documented

A suggested split of an `engines/mm2` (or `mm/mm2`) module, with the document and the Python reference
(`tools/`) that pins down each part.  Python tools are executable specifications: port them rather than the
assembly where possible.

## Data layer (all verified against the shipped files)

| Class | Job | Reference |
|---|---|---|
| `LzwDecoder` | 9-12 bit LSB-first LZW, `u32` size header, 100h clear / 101h end | `tools/mm2_lzw.py`, file-formats.md |
| `MapFile` | `MAP.DAT` (60 chunks, 256 wall bytes + 256 flag bytes), `ATTRIB.DAT` (64-byte blocks) | file-formats.md |
| `EventFile` | `EVENTSI/O.DAT` chunks: triggers, scripts, messages | events.md, `tools/mm2_data.py` |
| `StringFile` | `STR.DAT` (`+1Ch` obfuscation) | file-formats.md |
| `MonsterTable`, `ItemTable`, `SpellTable` | `MONSTERS.DAT` (26 B), `ITEMS.DAT` (20 B), `SPELLS.DAT` (2 B) | file-formats.md, spells.md |
| `ImageBank` | `*.16` (4 bpp, EGA palette) / `*.4` (2 bpp, CGA), masks, `MONSTERS.*` banks | `tools/mm2_gfx.py`, `tools/mm2_monsters.py` |
| `SaveFile` | `ROSTER.DAT` = 48 characters (`82h` bytes) + 2052-byte state block | file-formats.md, save-format.md |

Sound is PC-speaker tones only (`pc_speaker_tone`); no digitised audio files exist in the game.

## Game logic

| Module | Notes |
|---|---|
| Event interpreter | 50 opcodes, `cond` register, trigger table; events.md. Opcode 14 is the "enter location" switch (shops, temples, special caves events) |
| Party / characters | classes, levelling, training costs, attributes: classes.md; rest, search, treasure: party-commands.md |
| Combat | flow, speed ordering, to-hit/damage, monster AI, touch effects, rewards: combat.md |
| Spells | 96 spells, usage flags, effect dispatch: spells.md |
| Town buildings | inn/tavern, temple, guilds, blacksmith (prices and stock tables): shops.md, party-commands.md |
| Time | `g_day_fraction`, 10 eras with their own calendars (save-format.md); era travel events |

## Presentation

| Module | Notes |
|---|---|
| First-person view | indoor: view.md (algorithm + all coordinates; `tools/mm2_view.py` is a working renderer). Outdoor: outline only |
| Windows/text UI | 20-byte window slots, text windows on a 40x24 character grid over the 320x200 screen, fonts in the driver; game-library.md, drivers.md |
| Monster pictures | animated 96x96 frames, EGA/CGA: file-formats.md |
| Video drivers | not needed: ScummVM draws itself; the ABI (drivers.md) shows which primitives the game uses (draw image/mask, rect fill, page copy, plot) |

## Suggested order of work

1. Data layer + a debug viewer (maps, banks, monsters).
2. Indoor view + movement + event interpreter (towns first: maps 0-4).
3. Party screens, shops, inn; save/load.
4. Combat and spells; outdoor view; remaining special events (caves, era travel).

## Known unknowns

See the open list in overview.md.  Anything marked *(check)* in the docs was only skimmed; everything else was
read from the code, and the data formats were checked against the shipped files.
