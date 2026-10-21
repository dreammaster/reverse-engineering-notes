# One engine for Fountain of Dreams and Wasteland 1? — assessment

State of the two code bases (October 2026 snapshot of this repository):

| | Fountain of Dreams (`engine/src`, library `fod`) | Wasteland 1 (`engine/wl`, `engine/tools/recomp`) |
|---|---|---|
| Original program | KEH.EXE, compiled C, data driven | WL.EXE, hand written 8086 assembly, register calling convention |
| Game logic | bytecode script VM (76 opcodes), combat, shops, party: ported to readable C++ | 843 routines (map actions, combat, UI): machine translated, verified, not yet ported by hand |
| Data formats | LZSS archives (table of contents), TILES, `?MAP` chunks, PICT/ANI pictures | MSQ blocks, rotating XOR + Huffman, vertical XOR pictures, 5 bit text, `GAME1/2` map blocks |
| Video | 320x200, EGA 16 colours, back buffer | EGA mode 0Dh planar, write mode/latch pipeline, `ALLHTDS` tiles, mono + colour fonts |
| Sound / input | PC speaker effects, keyboard | PC speaker (timer 2), BIOS keyboard, F-key macros |
| Size | ~5100 lines, complete game loop | ~2700 lines of hand written ports + 30k lines of generated code |

## What is really shared

* **Platform needs**: a 320x200 16 colour framebuffer with the EGA palette, a keyboard queue in BIOS key codes (the FoD
  front-end already maps to its own codes), an 18.2 Hz timer, case-insensitive access to the game directory, a speaker
  tone generator. `platform/win32_main.cpp` (FoD) and `platform/win32_wl.cpp` (WL) duplicate roughly 150 lines of window and
  paint code; in ScummVM all of that is the `OSystem` backend.
* **Fonts and window frames**: both games are Interplay/Michael Cranford descendants and draw 8x8 text cells inside
  bordered windows; `wl::Ui` and `fod::Screen` do the same job against different assets.
* **Rules lineage**: skill/attribute checks, dice (WL `rollDice` 2d6 explosive vs FoD KEH `rngByte` style generators),
  party/inventory layouts are similar in spirit but not byte compatible.

## What is not shared

The game logic. FoD is interpreted bytecode (the scripts live in the data files); WL's logic is code in the executable
(the "action class" handlers, combat round at 0x12D84, ...). There is no common script format, no common map format and no
common save format. A single "interpreter" for both is not possible without rewriting one game's logic in the other's
framework, which would change behaviour.

## Recommendation

1. **One engine module, two game back-ends** (what a ScummVM `wasteland` engine can reasonably look like):
   * `engine/common`: framebuffer + palette, input queue, timer, speaker, file system, text/window helpers.
   * `engine/fod`: the current `fod` library (already free of platform code behind the `Platform` interface).
   * `engine/wl`: the Wasteland back-end.
   * detection entries for both games (file sets: FoD `KEH.EXE`/`FOD.EXE` + data; WL `WL.EXE`, `GAME1`, `GAME2`, ...).
2. **Wasteland back-end strategy**: ScummVM cannot ship code machine-translated from the original executable, so the
   recompiler is a *development scaffold*, not the product:
   * keep `wlplay` (generated code) as the always-correct reference build and test oracle;
   * replace generated functions by hand written ports one group at a time, exactly as the `wl` library already does for
     the renderers, text engine, rules and party accessors; `wltest` (per routine) and `wlrctest` (whole program) prove
     each replacement against the original;
   * the remaining coverage is tracked by the list of `f_XXXX` still used at run time (see `tools/recomp/entries.txt`).
   What is left is most of the 843 routines: movement and the map action classes, combat, the shop/party/roster screens
   and the disk/menu code (the latter partly obsolete on a modern system).
3. **Do not merge the two games' state and logic layers.** Share only the platform/helper layer; give each game its own
   `Engine::run` loop. This keeps the verified FoD engine untouched.

## Risks

* Legal: generated code and the committed IDA exports are derived from copyrighted programs; they must stay out of any
  upstream submission (the repository keeps them out of git: `wl_image.bin`, `wl_gen.cpp`).
* Wasteland 1 behaviour depends on timer ticks and on the number of keyboard polls (the RNG is stirred on each poll); a
  hand written port has to reproduce the poll structure or accept non-identical random sequences.
* The `wl` hand written code relies on byte-exact data segment offsets (`globals.h`); a clean rewrite will replace those
  with real structures while keeping the oracle comparisons at the boundaries.
