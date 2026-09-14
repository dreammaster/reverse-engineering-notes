# File formats

On-disk data formats used by "Yendorian Tales Book I: Chapter 2"
(`SW.EXE`), cross-referenced against the disassembly as they're decoded.
**Nothing below is a confirmed format yet** — this is a placeholder with
the observational starting points from the initial setup session
(2026-09-14). See [overview.md](overview.md) for the full session notes
and [roadmap.md](roadmap.md) for priority order.

## `CURGAME` / `SAVGAME1` (and presumably `SAVGAMEn`)

77,509 bytes each, identical size. `SAVGAMEX` appears in `SW.EXE`'s
string table as a templated filename (`X` = slot digit), and the
in-EXE error string `"Problem with CURGAME."` / `"Problem with a SAVED
GAME file."` confirms `CURGAME` is the active/working copy distinct from
the numbered save slots.

First 32 bytes of `CURGAME` (hex-decoded):
```
53 4D 49 54 48 57 41 52 45 20 50 41 52 54 59 00   SMITHWARE PARTY\0
57 41 52 45 00 20 20 20 00 00 00 00 00 00 00 00   WARE\0   \0...
```
`"SMITHWARE PARTY\0WARE\0"` — identifies the developer as **SmithWare**
(matches `SW.EXE`'s name) and suggests a header/magic string rather than
game data proper. Not parsed further yet.

## `WORLD.DAT`

1,761,397 bytes. Referenced by (unverified names, see overview.md)
`loadWorldDat1` through `loadWorldDat5` and `WorldDat_setBlock1` through
`WorldDat_setBlock6`. First 8 bytes read as repeating
`00 00 00 00 01 00 00 00` pairs in a quick raw peek — consistent with a
table of `(flag/type, count-or-offset)` pairs or similar, but this is a
guess from 64 bytes, not a traced format. Error strings suggest it holds
at least: text data, NPC data, conversation data (per `"Problem
retreiving text/NPC/conversation data."`), and presumably maps.

## `PICTURES.VGA`

12,550,618 bytes. Referenced by the in-EXE error string `"Problem with
PICTURE.VGA."` (note: singular in the string, plural on disk). Far too
large to be a single image — almost certainly a sprite/tile/frame atlas,
likely paged through EMS given the game's mandatory-expanded-memory
requirement (see overview.md). Not decoded at all yet.

## Not yet examined

- `SBFMDRV.COM` — third-party(?) Sound Blaster FM driver, likely not
  worth reverse-engineering in detail (not game logic).
