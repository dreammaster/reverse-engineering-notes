# Music (`*.M`) and sound drivers

`*.M` (BANK, CAVES, CITY, COMBAT, CYBER, DUNGEON, EERIE, GROUNDS, GUILD, HONKY, MEDIEVAL, MM3THEME, TEMPLES, TOWNINN, VENTURE) are
songs for the AdLib/OPL driver `ADLIB.DRV` (the other `.DRV`: `ROLAND`, `IBM` (PC speaker), `TANDY`, `BLASTER`, `COVOX`, `DEMO`, `TIMER`).
The format is the ancestor of the one ScummVM implements for Xeen (`engines/mm/shared/xeen/sound_driver*.cpp`) but **not the same**: instruments are
14 bytes instead of 26 and several commands have different operands, so the Xeen driver cannot play these files unchanged.

A song is a stream of commands, `high nibble = command, low nibble = channel/instrument`.  Command set, derived from the dispatch tables of
`ADLIB.DRV` (music table at driver offset `1C5h`, effects table `1E5h`), and verified by walking all 15 songs from the first byte to the last
(`tools/mm3_music.py`):

| Cmd | Name | Operand bytes |
|---|---|---|
| 0 | call subroutine (16-bit offset; return with `FFh`) | 2 |
| 1 | wait (countdown, always 1 byte; ends the current tick loop) | 1 |
| 2 | define instrument `n` (the next 14 bytes are the OPL operator settings) | 14 |
| 3 | no-op (skips a byte) | 1 |
| 4, 5, D, E | no-op | 0 |
| 6 | no-op (skips a byte) | 1 |
| 7 | channel note off (clears the key-on bit) | 0 |
| 8 | channel frequency stop/refresh | 0 |
| 9 | start note on channel (pitch via a note table) | 1 (note) |
| A | set channel volume | 1 |
| B | skip a byte | 1 |
| C | play instrument `arg` on channel | 1 |
| F | `FFh` return from subroutine / `FEh` end of song | 0 |

The effects stream (used for sound effects through the same driver) has its own table: instruments are 11 bytes, `3` sets volume, `6` starts a
frequency without key-on, `D` clears a channel's frequency slide and `E` starts a slide (3 bytes).

`S1.S`-`S7.S` are raw 8-bit unsigned PCM samples (centre 7Fh) for the Covox/Sound Blaster drivers, and `TIMER.DRV` (448 bytes) is the
timer-interrupt helper loaded at start (`aTimerDrv` in `_main`).

## Sound effects

`playSoundEffect(n)` (`1B16B`, formerly `sub_1B16B`) is called with a constant at every sound site (heal 35h/34h, spell animations 0Bh-14h, hit 12h...).
Ids below 97h (151) are sound effects, enabled by the FX flag `byte_36FE8`; 97h and above are gated by `byte_36FE9`.  It calls `soundDriverPlay`
(`26978`), which transfers (push driver segment `cs:2693D`, push 9, `retf`) to API function 9 of the loaded sound driver module -- the same call
convention as the video module (`video-module.md`).  `evt_op06` (event opcode 6, PlayFX) takes the same ids.

### Where the effect data lives (decoded from `ADLIB.DRV`)

The driver's API entry 4 (offset 9, called as `soundDriverPlay`'s "function 9") takes an effect id `n`: ids `FFFFh` and other negative values query/clear the busy flag (`word_F`); ids 21h and 23h are ignored; otherwise the driver loads its effect cursor from the word table at **driver offset `967h` + 2n**, which points to an effect stream stored **inside the driver file itself** (offsets `0AA9h`..`1FE2h`), resets the four channel counters and starts the effects sequencer.
The table has entries for ids 0-150 (151 = the `97h` limit in `playSoundEffect`): 98 distinct streams; 17 ids (0, 5, 6, 11, 19, 23, 26, 27, 32, 33, 37, 39, 49, 50, 56, 59 and others) point at `0B22h`, a short "silent" stream (`20 12 ff ff ff 01 11 ff`).  Some ids deliberately share a stream (for example ids 1 and 4 both use `0B39h`).
Each effect is therefore a few dozen bytes of the effects command stream described above; the other drivers (`ROLAND`, `IBM`, `TANDY`, ...) have their own equivalents of the same table.
