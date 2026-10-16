# The demo's complete script (the four `FlixEntry` arrays) and its data files

Decoded from `Shannara Demo.idb` (`_logosvga`, `_market`, `_still_pics?`, `_morph?` at `0x189FD4`, `0x18A034`, `0x18A134`, `0x18A274`) and checked
against the files in `demo/`. This is everything the non-interactive demo does after its text-mode setup (see `entry_and_init_flow.md`).

## Record format (correction of the earlier struct)
`FlixEntry` is 16 bytes: `u16 frame` (tick at which it fires), `u16 opcode`, `u16 arg1`, `u32 param` (pointer to a file name or function), `u16 arg2`,
`u16 arg3`, `u16 pad`. (The IDB struct had `frame` as a dword and `arg1` after it; the real layout is `+0 frame`, `+2 op`, `+4 arg1`, `+6 param`,
`+0xA arg2`, `+0xC arg3`.) Ticks advance with the video frames (10 per second for the demo's Q files). Opcodes (callees in `flix_play`):

| Op | Meaning | Operands |
|---|---|---|
| 0 | call `param` as a C function | |
| 1 / 12 | load a still picture (1 = GIF/DIB, 12 = PCX) | `param` = file name |
| 2 | palette fade | `arg2` 2 = fade in, 3 = fade out |
| 3 | open a Q video (`q_open`), clears the screen when `arg2` is 1-4 | `param` = file name, `arg2` = read-buffer hint |
| 4 | close the Q video | |
| 5 / 7 | `start_music(arg1)` / `stop_music(arg1)` | track in `Demo001.mus` |
| 6 | `fade_music(arg1, arg3 ms)` | `arg3` 2000 = 2 s |
| 8 | `make_sound(arg1)`, then `set_sound_style` | `arg1` = sound id in `Demo001.voc`, `arg2` = volume, `arg3` = 1 |
| 9 | `set_sound_style` | |
| 10 | `stop_sound(arg1)` | |
| 11 | end of script (waits until `frame`) | |

## The timeline (what `main` plays, in order)
1. **Logo** (`flix_play(_logosvga, 6)`): open `LOGOSVGA.Q` (79 frames), fade in (tick 0), start music 1 (tick 1), fade out at 85, close at 90, end;
   `main`'s own entry 6 fades the music out over 2 s. The Q animation is the Legend shield logo (verified by decoding).
2. **"Market" / Brona intro** (`flix_play(_market, 16)`): fade music, open `BRONA_2.Q` (191 frames, the skeletal sorcerer and the Sword of Shannara), fade in,
   sounds: id 1 at tick 5 (vol 80, stopped at 62), id 4 at 72 (vol 200), id 2 at 119 (vol 300), music 4 at 147 (after stopping music), sound 3 at 192
   (vol 100); fade out at 287, fade music 2 s, close at 307, stop music 4, start music 2, end at 307.
3. **Stills** (`flix_play(_still_pics?, 20)`): `SCREEN00.PCX` (fade in, sound 5 at vol 130), then every 30 ticks fade out / load / fade in:
   `BRONA.PCX` (30), `SCREEN02.PCX` (60), `SHADE1.PCX` (90), `DEAD.PCX` (120); fade out at 150, stop sound 5, fade music 2 s, stop music 2 at 170, end.
4. **Hold** (`flix_play(_morph?, 3)`): `DEMO3.PCX` shown for 400 ticks.
`main` repeats 1-4 while the INI `LOOPING` key is present and no key is pressed (any key also aborts a running `flix_play`; Esc is pushed back).

## Demo data files (all in `demo/`)
`Logosvga.q` (321,496 B) and `Brona_2.q` (5,662,299 B): Q videos, 640x480, 10 fps, no embedded audio (decoded in full, see `q_codec_spec.md`);
`Screen00/Screen02/Shade1/Brona/Dead/Demo3.pcx`: standard PCX stills; `Legend.gif`: a GIF (title); `Demo001.mus`: 15 XMIDI tracks (section 25 of
`retail_game_layer.md`); `Demo001.voc`: sound effects (6 raw 8-bit PCM clips, 256-byte-block directory); `*.mdi`, `*.dig`: Miles sound-driver files;
`Demo.exe`: the DOS/4GW + `DEMO.LE` executable analysed here; `Sample.ad/.opl`, `512.gus`: instrument banks.
