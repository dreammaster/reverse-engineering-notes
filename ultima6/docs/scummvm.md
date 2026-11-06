# ScummVM integration (goal 5)

The stand-alone engine in `src/` is copied into a ScummVM tree by `tools/sync_scummvm.py <scummvm dir>` as the sub-engine `ultima6new` of the `ultima` engine
(`engines/ultima/ultima6/`). `src/` stays the only place to edit: every run regenerates the copy.

## What the script does
- include paths get the `ultima/ultima6/` prefix, the namespace `Ultima6` moves inside `Ultima` (so the classes are `Ultima::Ultima6::...`, the per-game ones `Ultima::Ultima6::Ultima6::...`);
- `engine/std_headers.h` (the std headers, then `common/scummsys.h`) is the first include of every file, because ScummVM's `forbidden.h` rejects what the std headers use;
- the host-specific files are replaced: `scummvm/overlay/files/data_file.cpp` (files through `SearchMan`, saves through the save file manager under the `@save/` prefix), `overlay/engine/game_id.cpp` (ScummVM's detection tells the game), the window backend and the PNG writer are left out;
- `scummvm/glue/ultima6_engine.*` is the `Engine` subclass: `ScummBackend` (screen with palette, keys as the stand-alone window gives them, mouse clicks, `SoundStream` = `MusicPlayer` + sound effects through the mixer), `MusicSwitch`, the front end / game loop / ending as in `src/main.cpp`;
- `module.mk`, `configure.engine` and `metaengine.cpp` of the ScummVM tree are patched between `BEGIN generated` / `END generated` markers.

## Building
`ultima6new` is off by default and cannot be built together with `ultima6` (Nuvie): both use the namespace `Ultima::Ultima6` (metaengine.cpp has an `#error`).
```
mkdir build && cd build
../scummvm/configure --disable-all-engines --enable-engine=ultima6new ...
```
All three games are routed to the new engine, the plain and the `_enh` detection entries alike. Martian Dreams and Savage Empire are flagged `ADGF_UNSTABLE` in the detection tables, so ScummVM asks to confirm before starting them (`enable_unsupported_game_warning=false` in the `[scummvm]` section of the ini silences it).

## Test hook
`ultima6_keys` (plain characters, `{up} {down} {left} {right} {enter} {esc} {space} {bs}`) and `ultima6_shot` (a BMP path) in the game's section of `scummvm.ini` press the keys, write the screen and quit, like `--keys` / `--shot` of the stand-alone program. `tools/bmp2png.py` makes a PNG of the BMP.

## Checked (2026-11-07)
Ultima VI (menu, then the game from the menu), Martian Dreams and Savage Empire start and draw identically to the stand-alone program.

## Open
- sound and music in ScummVM have been compiled but not listened to; the mixer volume/mute options are not used;
- saves live under `<target>-<name>` in ScummVM's save folder and are not listed in its save/load dialogs (no `loadGameState` / `saveGameState`);
- no keymapper entries (raw keys only), no options widget, no `ultima6new` entries of its own in the detection tables;
- the tree this was built in lacks `shared/engine/messages.cpp`, `shared/engine/view.cpp`, `shared/gfx/gfx_surface.cpp` and `shared/gfx/view.cpp` that its `engines/ultima/module.mk` lists (empty stubs were added there to build).
