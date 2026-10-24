# Ultima V — decompilation workspace (ScummVM preparation)

* [`u5d-main/`](u5d-main) — the upstream u5d decompilation, untouched.
* [`src/`](src) — our working copy of `u5d-main/src`, plus a native Win32 backend and the fixes found by the audit.
* [`audit/`](audit), [`ida_scripts/`](ida_scripts), [`docs/audit.md`](docs/audit.md) — the audit of `src/` against the original
  `ULTIMA.EXE`/`*.OVL` (results in [docs/audit.md](docs/audit.md)).
* [`tests/`](tests), [`tools/`](tools) — unit tests and the headless test harness.

## Build (MinGW-w64)

Needs MinGW-w64 `gcc` + `mingw32-make` and Python 3 on `PATH`. No SDL required.

```
mingw32-make              # build/u5.exe  (native Win32/GDI window, silent)
mingw32-make run          # copy the game data to run/ (once) and start the game
mingw32-make test         # headless smoke tests (boot, "Journey Onward" path); screenshots land in run/
mingw32-make unit         # unit tests for the C ports of assembly routines
mingw32-make clean
```

Variables: `GAME_DIR=C:/games/ultima5` (the original game; never modified — `run/` is a working copy that also holds the
`SAVEGAME/` folder the game writes to), `DEBUG=1`, `WARN=1` (the decompiled code is very noisy, so warnings are off by default),
`BACKEND=sdl` to build the SDL3 backend instead (needs SDL3 + SDL3_mixer via `pkg-config`).

### Running headless / scripted

The Win32 backend honours these environment variables (used by `tools/smoke_test.py`):

| variable | effect |
|---|---|
| `U5_HEADLESS=1` | no window; the game runs and can still take screenshots |
| `U5_FAST=1` | skip delays longer than 20 ms (animations); keyboard-loop pacing is kept |
| `U5_KEYS="..."` | key script: plain characters, `{enter} {esc} {space} {bs} {tab} {up} {down} {left} {right} {home} {end} {pgup} {pgdn}`, `{wait:N}` (N polls), `{sleep:MS}` (real time), `{shot:file.png}`, `{quit}` |
| `U5_KEYS_EXIT=1` | exit when the key script runs out |
| `U5_KEYS_TRACE=1` | print each key the game receives |
| `U5_SEED=n` | fixed random seed |

F12 in the window saves `screenshot.png`. At the very first key poll `J` is the original "Journey Onward" shortcut; the pristine
`SAVED.GAM` has no character, so that path ends at "No active game. Please create a character...".

## Changes against u5d

* `common/platform.h`: new `BACKEND_WIN32` / `BACKEND_SDL` selection (`TARGET_SDL` still means "modern hosted target").
* New: `graphics/grap_win32.c` (GDI presentation, PNG screenshots), `event/evt_win32.c`, `key/key_win32.c` (keyboard + key
  script), `time/time_win32.c`; `audio/audio.c` leaves audio disabled for this backend; `common/debug.c` message box.
* Audit fixes: `vars.c` `D_1a58` (table was reversed), `6000.c` `ULTIMA_6f1e` (FM Towns semantics replaced by the IBM routine),
  `2000.c` random generator and clock seed (original generator in every build).

## Re-running the audit

```
python audit/run_all.py [--game-dir C:/games/ultima5] [--idat "C:/Program Files/IDA Pro 8.3/idat.exe"] [--skip-ida]
```

Writes `audit/*.md` (kept in git); the JSON, IDA output and the scratch copies of the game binaries (`ida/`) are git-ignored.
