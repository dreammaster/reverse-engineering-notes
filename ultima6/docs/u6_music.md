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
