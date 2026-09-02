# Wizardry I — standalone C++ engine (Phase 3)

A dependency-free reimplementation of the DOS "Ultimate Wizardry Archives"
release, built directly from the reverse-engineering notes in `../docs`.
Structured so the platform-independent core (`wizcore`) can later back a
ScummVM engine; the standalone `wiz1` executable is the driver.

## Status

**Data layer** — reads everything the game needs straight from the original
`WIZ1.DSK`, verified byte-for-byte against the Python tools in `../tools`:

| module | what |
|---|---|
| `wiz/ucsd_volume.h` | mounts the UCSD p-System volume, lists / extracts files |
| `wiz/string_pool.h` | decodes `ASCII.KRN` (range tree + `(raw − 67·(kn%51) − 23·k) mod 256`) |
| `wiz/scenario.h` | `SCENARIO.DATA` TOC + record grid; `MonsterRec` / `ObjectRec` / `ExpTable` views |
| `wiz/types.h` | `WizLong` (base-10000), the enums, little-endian readers |

Not yet: the platform layer (framebuffer + input) and the game logic
(`ROLLER` → `CASTLE`/`SHOPS` → `RUNNER` → combat → …), ported from the
p-code with the Apple Pascal source as a guide.

## Build

Windows / MSVC (uses the CMake + Ninja that ship with VS 2022):

```powershell
engine\build.ps1
```

Or any CMake toolchain:

```bash
cmake -S engine -B engine/build -G Ninja
cmake --build engine/build
```

## Try it

```bash
wiz1 files    /path/to/WIZ1.DSK
wiz1 toc      extracted/wiz1/SCENARIO.DATA
wiz1 monsters extracted/wiz1/SCENARIO.DATA extracted/wiz1/ASCII.KRN
wiz1 items    extracted/wiz1/SCENARIO.DATA extracted/wiz1/ASCII.KRN
wiz1 exp      extracted/wiz1/SCENARIO.DATA
wiz1 str      extracted/wiz1/ASCII.KRN 13010 5001 20355
```
