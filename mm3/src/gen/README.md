# Generated code

`view_gen.c` is a **mechanical translation** of the original 3D view code (`prepareIndoorView`, `renderIndoorView`, the wall /
sprite / HUD list writers, the object and monster scanners and the maze accessors under them) from `../../mm3.asm`, made by
`../../tools/asm2c.py` (`make regen`).  It keeps the original's registers, flags and stack frames and addresses the game's data
segment (DGROUP) as a 64 KB array `DG[]`, so its behaviour is the original's, bit for bit.

* It is **not meant to be read or edited**.  The hand-written part is `../view_host.c` (the C-runtime, video and sound calls the
  original made, and the hook that receives the finished draw list) and, later, the code that fills `DG[]` from the game state.
* **It is checked against the original**: `make viewcheck` runs both this code and the original machine code (in the Unicorn CPU
  emulator, `tools/mm3_emu.py`) on random positions of the indoor maps and compares the draw lists record by record.
* The intention is to replace parts of it by readable hand-written C over time (`drawWallFaces` is a flat occlusion/priority chain
  that maps cleanly to a table); the same check proves each replacement equivalent.

Naming: routine names are those of `names/mm3.tsv`; data is addressed by DGROUP offset (see `../README.md`).

## Bundles

* `view_gen.c` -- the 3D view (`make regen`); verified by `make viewcheck`.
* `rules_gen.c` -- character rules and event conditions (`make regen_rules`); verified by `make rulescheck`, which runs the overlay code
  in the emulator (`tools/mm3_emu.py` loads the 13 Borland overlays at IDA's addresses and emulates the overlay manager's `int 3Fh`).

Translator lessons recorded so far: stack locals keep their declared byte/word size; jump tables may hold several entries per line;
`jmp $+2` is a no-op; far calls to host routines push no return address (arguments start at sp); uninitialised stack locals read as 0.

## Computed jumps through `cs:` tables
`jmp cs:[bx+K]` dispatch tables (e.g. the 36-entry command table of `exploreLoop`) point at handlers IDA left
unlabelled.  `asm2c.py` reads the real table from `data/IMAGE.BIN`, disassembles the function with `objdump` to get
each instruction's address, and adds `ul_<addr>` labels at the handlers.  `make regen_game` therefore needs
`data/IMAGE.BIN` (derive it with `tools/mm3_image.py`) and binutils.
