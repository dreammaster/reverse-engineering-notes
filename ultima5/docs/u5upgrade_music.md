# The Ultima V Upgrade's music, analysed and placed in `src/`

Source: *The Exodus Project: Ultima V Upgrade, Release 1.0* (Michael C. Maggio, 2001), unpacked into `u5upgrade/` (git-ignored, freely
available). It adds MIDI music (XMI files played through a Midpak TSR) to a game that has none, without touching the user interface.
This document records what the package changes in the game binaries, the logic of its driver, and where each music call now sits in
the generated C. The result is `src/audio/u5music.[ch]` plus the call sites listed in [Call sites](#4-call-sites).

## 1. What the package contains

| part | files | used by the engine? |
|---|---|---|
| music | 16 XMI files: `U5THEME BRITLAND HORNPIPE ENGGMNT STONES GREYSON FANFARE MONARCH TRNTLLA HALLS WRLDBLW BLCKTHRN LADYNAN REUNION RULEBRIT AMIGA` (+ `setm.xmi`, a test tune for the sound-card setup) | **yes** - the only data a ScummVM engine needs |
| patched game code | `ULTIMA.EXE` and `INTRO FONT MAINOUT TOWN DUNGEON ENDGAME .OVL` (same sizes as the originals, except `ENDGAME.OVL` +64 bytes) | analysed below |
| driver | `mid.drv` (823 bytes): the game-side glue between the game and Midpak | analysed and ported |
| DOS plumbing | `*.ADV/*.ADD` Midpak sound-card drivers and maps, `?MIDPAK.COM` TSRs, `FAT.OPL`, `setm.exe`, `u5cfg.exe`, `u5data.exe`, `ultima5.com` | not needed (ScummVM provides the MIDI output) |

`u5data.exe` patches `DATA.OVL` once: the unused Tandy video-driver file name (`T1K.DRV`) is overwritten by the name `MID.DRV`, which is how
the game finds the driver. Nothing else in `DATA.OVL` changes.

## 2. Method

1. Byte-diff of each patched file against the files in `C:\games\ultima5` (`tools/patch_sites.py` prints every changed run with the
   disassembly of both versions; `tools/dis_diff.py` shows one window). The patches are tiny: 195 bytes differ in `ULTIMA.EXE` (180 of
   them one new block), 97 in `INTRO.OVL`, 6 in `FONT`, 4 in `MAINOUT`, 17 in `TOWN`, 2 in `DUNGEON`, 14 + 64 appended in `ENDGAME`.
2. Call targets were resolved with the audit's overlay data (`ida/out/stubs.json`, `ovl_bases.json`): an overlay's relative `call T`
   reaches EXE address `(T + base) mod 64K` with base `81c0` (INTRO), `81d0` (MAINOUT, TOWN, DUNGEON), `a290` (ENDGAME), `e1e0` (FONT);
   stubs at `7a16..81ba` name the overlay function (e.g. `7a5e` = `BLCKTHRN_0910_Death`).
3. `mid.drv` was disassembled completely and then **ported and checked against the original machine code**: `tools/verify_midrv.py` runs
   the real driver in a Unicorn 16-bit x86 emulator (INT 66h hooked) and the C port on the same random call sequences and compares every
   Midpak command and both state bytes - 24,000 entry calls agree (`mingw32-make unit` runs a shorter check when `u5upgrade/` exists).
4. The call sites in `src/` were then placed by function (not by address) and the resulting music cues are recorded per scenario in
   `tests/golden_music.json` (`tools/smoke_test.py` compares them like the screenshot hashes).

Only the *music* changes are ported. Not ported (not music): the upgrade replaces `ULTIMA_0de0_DetectVideo` by a stub that forces EGA; adds
Ctrl-E at the main menu and "any key" at the end of the endgame to leave to DOS (a ScummVM engine has its own quit); and the
INTRO.OVL difference at `+1392` is the GOG patch already described in `deviations_review.md`, not part of the upgrade.

## 3. The driver (`mid.drv`)

The game calls the driver with a far call to one of ten jump-table entries (`call far [cs:0ded]`, the offset is the entry number x 3 in DX).
It keeps two bytes: the **current track** (`ff` = none) and an **auto flag**. It talks to Midpak with INT 66h: `070d` load an XMI by name,
`0702` start sequence 0, `0705` stop, `070c` status (1 = playing).

`Play(t)`: if `t` is the current track, the driver only checks the status and restarts the track when it is no longer playing (this is how the
tunes loop); otherwise it stops the music and starts `t` (`ff` = silence).

| entry | what it does | game-side name |
|---|---|---|
| `00` | auto on: track = `TrackForGameState`; auto off: keep the current track; then `Play` | `U5MUSIC_Update` |
| `03` | stop, auto off | `U5MUSIC_Stop` |
| `06` | `Play(TrackForIntroScene(BL))` | `U5MUSIC_PlayIntroScene` |
| `09` | auto off, `Play(THEME)` | `U5MUSIC_PlayTheme` |
| `0c` | `Play(AMIGA)` (auto unchanged) | `U5MUSIC_PlayAmiga` |
| `0f` | auto on | `U5MUSIC_AutoOn` |
| `12` | auto off, `Play(STONES)` | `U5MUSIC_PlayStones` |
| `15` | auto off, `Play(REUNION)`, then the current track is set to RULEBRIT so that a later `1b` starts it once Reunion has ended | `U5MUSIC_PlayReunion` |
| `18` | `Play(TrackForEndingScene(BL))` | `U5MUSIC_PlayEndingScene` |
| `1b` | `Play(RULEBRIT)` | `U5MUSIC_PlayRuleBritannia` |

### Track for the game state (auto on)

Evaluated top to bottom on `D_5893_map_id`, `D_587c_partyTile`, `D_5895_map_level`, `D_58a3`:

| condition | track | where |
|---|---|---|
| map `ff` and `D_58a3 == 0` | ENGGMNT | combat |
| map `ff` and `D_58a3 != 0` | THEME | the final battle |
| party tile `& f8 == 20` | HORNPIPE | aboard a ship |
| map `0`, level 0 | BRITLAND | overworld |
| map `0`, level not 0 | WRLDBLW | underworld |
| map 1-8 | TRNTLLA | the eight cities (`TOWNE.DAT`) |
| map 9-0c | LADYNAN | the first four dwellings |
| map 0d-10 | GREYSON | the other dwellings |
| map 11 | MONARCH | Castle of Lord British |
| map 12 | BLCKTHRN | Palace of Blackthorn |
| map 13-18 | GREYSON | the other castles |
| map 19-1d | LADYNAN | the first five keeps |
| map 1e-20 | FANFARE | the last three keeps (Empath Abbey is one) |
| map 21-28 | HALLS | dungeons |
| anything above 28 | none | e.g. map 40 = "no map loaded" |

Introduction scenes (entry 06): scene 0-7 STONES, 8-0e HALLS, 0f-15 GREYSON. Ending slides (entry 18): 0-3 STONES, 4-7 LADYNAN, else none.

**Quirk of the original driver.** For a map above `28` the auto branch jumps to the end of the routine *past* its own `mov al,0ff`, so it
returns whatever AL the game had at the call (normally garbage; the name table is indexed with it). The port treats the case as "no
music", which is clearly the intent; `verify_midrv.py` sets AL to `ff` before each call for the same reason.

## 4. Call sites

`ULTIMA.EXE` carries the wrappers (code offsets `0de0`-`0e92`, in the space of the replaced video detection); overlays call them by the
patched `call` targets. Each row: where the upgrade changes the binary, and the equivalent place in `src/`.

| upgrade patch | effect | `src/` |
|---|---|---|
| EXE `00ad` (`call` of the intro stub -> `0e36`) | load the driver, run the intro, then auto on | `ultima.c` `main`: `U5MUSIC_Init(); INTRO_0986_Main(); U5MUSIC_AutoOn();` |
| EXE `034c` (`mov ah,4c; int 21` -> `0e2f`) | stop the music before the program ends | `ultima.c` end of `main`: `U5MUSIC_Stop()` |
| EXE `1b5b` (the `call` of `ULTIMA_1d5e_PollKey` in the cursor-animation wait loop -> `0e0e`) | entry 00 before every poll of the main key wait | `1000.c` `MUSIC_POLLKEY()` |
| EXE `606a` (`call` of `COMBAT_0b94_MainLoop` -> `0e49`) | auto on, then the combat loop | `5000.c` `ULTIMA_5f86_SpecialMapHandler` |
| EXE `3e7c` (`call 5f86` -> `0e66`) | Stones, special map (camping in a dungeon), auto on | `3000.c` hole-up code |
| EXE `6373` (`call 5f86` -> `0e5a`) | Stones around camping outdoors | `6000.c` `ULTIMA_6360_Camping` |
| MAINOUT `0968` (`call` of `CAST2_0e76_Shrine` -> `0e50`) | Stones around entering a shrine (the Codex shrine and the other shrine call are *not* wrapped) | `mainout.c` `MAINOUT_08de_EnterCmd`, `TILE_MAP_SHRINE` |
| MAINOUT `0af0`, TOWN `145c`/`1862`, DUNGEON `1014` (`BLCKTHRN_0910_Death` -> stop, death, auto on) | no music during the death sequence | `mainout.c`, `town.c`, `dungeon.c` |
| TOWN `12ca` (`BLCKTHRN_060e_Capture` -> `0e72`) | no music during the Blackthorn capture | `town.c` `TOWN_12ae` |
| INTRO `05b7` (new code `0ad0`: entry 09 before `D_a9bd[1] = 2`) | the theme whenever the title is shown | `intro.c` `INTRO_05b0_DisplayTitle` |
| INTRO `0337`, `0411` (poll -> new code `0adc`: entry 06 with `[bp-8]` = the scene) | story music during the introduction | `intro.c` `INTRO_014e_Introduction`, `MUSIC_POLLKEY_INTRO(local_a)` |
| INTRO `0852` (poll -> `0e0e`) | refresh on the acknowledgements screen | `intro.c` `INTRO_072e_Acknowledgements` |
| INTRO `0fa3`, `0fa8` (new code `0af9`/`0aed`: entry 0c) | the Amiga tune for character creation and for the Ultima IV transfer | `intro.c` main menu, cases `'T'` and `'C'` |
| FONT `0ab4`, `0c9b`, `0d9c` (poll -> `0e0e`) | refresh while the character-creation screens wait | `font.c` |
| ENDGAME `0655` (new code `0aff`: entry 15 before `ULTIMA_2900_UpdateVitalsDisplay`) | Reunion, then Rule Britannia | `endgame.c` `ENDGAME_0648_EndgameMain` |
| ENDGAME `017e`, `01e6` (poll -> new code `0aee`: entry 18 with `[bp-6]` = the slide) | music per ending slide | `endgame.c` `ENDGAME_0000`, `MUSIC_POLLKEY_ENDING(local_8)` |
| ENDGAME `04f9` (the final endless loop), `0ae5` (the "bad ending" endless loop) | Rule Britannia in a loop (and any key leaves - not ported) | `endgame.c` `ENDGAME_0326`, end of `ENDGAME_0648_EndgameMain` |

Note on frame variables: u5d numbers locals like Ghidra (`local_a` is `[bp-8]`), which is why the intro scene is `local_a` and the ending
slide is `local_8`.

## 5. Cues per scenario (from `tests/golden_music.json`)

| scenario | cues |
|---|---|
| intro menus, character creation | THEME, AMIGA (creation), THEME (back at the title) |
| Journey Onward from the overworld | BRITLAND |
| Britain, shops | BRITLAND, TRNTLLA |
| Empath Abbey, talking to a jester | FANFARE |
| overworld fights | BRITLAND, FANFARE (abbey), ENGGMNT |
| sailing | HORNPIPE (BRITLAND on land) |
| shrine of Compassion | BRITLAND, STONES, BRITLAND |
| dungeon | HALLS; camping inside: STONES; fights: ENGGMNT; back to HALLS |
| Lord British's castle | MONARCH |
| Palace of Blackthorn | BLCKTHRN |
| ending | REUNION, STONES, LADYNAN (slides); RULEBRIT follows at the final screens (not reached by the scenario's 14 slides) |

## 6. For the ScummVM engine

* Implement `U5MusicHost` (`Start(track)`, `Stop()`, `IsPlaying()`) with an XMI player (ScummVM's `MidiParser::createParser_XMIDI` and a
  `MidiDriver`); `U5MUSIC_TrackFile(track)` gives the file name. Everything else in `u5music.c` is the original driver's state machine and
  needs no change.
* The XMI files are the only data needed; they are loaded by name from the game folder. The engine option "music" (default on) should
  simply not call the hooks (or install a null host); detection of the files decides whether there is anything to play.
* `IsPlaying()` is what makes the tunes loop; if the host loops XMI natively, return 1 permanently.
* The cue logic is independent of the FM Towns background music (`ENABLE_BGM`, `AUDIO_*`): the two are alternative sources of music. The
  FM Towns hooks sit at different places (and use CD tracks); pick one source per run.
* Quirk worth keeping in mind: the original driver restarts a track only when Midpak reports it is no longer playing, so a track that is
  already current is never restarted by a new cue.
