# Ultima VI music

The twelve tracks `ULTIMA.M`, `BOOTUP.M`, `INTRO.M`, `CREATE.M`, `FOREST.M`, `HORNPIPE.M`, `ENGAGE.M`, `STONES.M`, `DUNGEON.M`, `BRIT.M`, `GARGOYLE.M`
and `END.M` are LZW streams (`lzw.h`); the unpacked data is one track of events, handed to the sound driver (`snd_play`, far pointer to the track)
which plays it from its timer interrupt. C++: `src/files/music.*` (`tests/test_music.cpp`). Which file plays where is chosen by name in `GAME.EXE`,
`U.EXE` and `END.EXE` (each lists all twelve names plus `MIDI.DAT` next to the driver names).

## Player (read from the AdLib driver, `u6_snd_adlib`)

`snd_play` (`1C6`) clears 0x12E bytes of state, stores the data pointer, saves the INT 08h vector, programs the PIT to divisor 0x4DAE (1193182 / 19886 =
**60 Hz**) and hooks INT 08h to `snd_timer_isr` (`3A0`, created by `find_driver_isr.py` because nothing calls it). Each tick the handler counts down a delay
and otherwise reads event bytes: `shr al, 3; and bx, 1Eh; jmp [table + bx]` — a **16-entry dispatch on the high nibble of the event byte**. The same shape
holds in the other five sound drivers (their ISRs are at 39D, 466, 442, 366, 385).

## Events

Event byte = command << 4 | channel; layout verified by decoding all twelve tracks from the first byte to the last with no unknown event and every call
target inside the track:

| Command | Meaning | Data |
|---|---|---|
| 0-7 | note off, note on, set pitch, set level, modulation, slide, vibrato, program change | 1 byte |
| 8, channel 1 | call: repeat count u8, offset u16 | 3 bytes |
| 8, channel 2 | delay in ticks | 1 byte |
| 8, channel 3 | load instrument: slot u8 + 11 bytes of OPL instrument | 12 bytes |
| 8, channel 5 / 6 | fade out / fade in | 1 byte |
| 8, other | no data | 0 |
| E | set loop point | 0 |
| F | return: end of a subroutine, or at top level jump back to the loop point | 0 |

The command names follow Nuvie's `MidiParser_M`; the sizes and the instrument count (a track's number of loaded instruments equals the number of entries Nuvie
lists for it in its MT-32 mapping, e.g. 4 for `ULTIMA.M`, 5 for `INTRO.M`) agree with the files. `MIDI.DAT` is a text file explaining how voices are assigned to
MIDI channels for the Roland driver.

## Playing it (`src/engine/music_player.*`, `src/engine/opl2.*`)

`MusicPlayer` is the AdLib driver run by hand, register for register: `tick()` is `snd_timer_isr` (60 Hz; events read until a delay; then the slide / vibrato / fade of each
voice from `loc_5A1`), `play` is `snd_play`, `stop` is `snd_stop` / `sub_383`. The voice of an event is its channel (nine voices; the operator register offsets are the tables at
`cs:0164` and `cs:016D`). Pitch bytes are block (3 bits) | tuning group (2 bits) | note (3 bits), looked up in the table at `cs:0176` (a rest and seven notes per group, three groups a
little apart) and written as the A0 / B0 pair, with 0x2000 for key-on. Note on writes the pitch with the key off first; set level writes `0x40 + carrier`; modulation writes
`0x40 + modulator`; program change writes the eleven bytes of a loaded instrument (modulator 20/40/60/80/E0, carrier 20/40/60/80/E0, then C0); vibrato is a triangle of the
depth (high nibble) with the rate (low nibble) as multiplier; fade out / in add / subtract one from the level every n ticks until 0x3F / 0. `Opl2` is a floating-point model of the
chip (see APPROX 71 in approximations.md). `sub_377C8` / `sub_3773A` choose the track: level 5 GARGOYLE, surface STONES / ULTIMA / FOREST / BRIT by chance, other levels DUNGEON, a ship
or skiff HORNPIPE, combat ENGAGE; chosen again when the level, the vehicle or combat changes and after resting. The track numbers follow the pointer table at `ds:2955` of `GAME.EXE`
(1 ULTIMA, 2 BOOTUP, 3 INTRO, 4 CREATE, 5 FOREST, 6 HORNPIPE, 7 ENGAGE, 8 STONES, 9 DUNGEON, 10 BRIT, 11 GARGOYLE, 12 END). Windows plays it through `waveOut` (mono, 49716 Hz) from a
thread of its own; `U6_NO_MUSIC` in the environment keeps it silent.
