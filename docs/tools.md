# Tooling

Everything here runs on Linux with Python 3 (`pillow`, `numpy`, `unicorn`),
`ndisasm`, optionally DOSBox + Xvfb. Nothing needs IDA; the IDA exports in
`disasm/` are read-only inputs.

| Tool | What it does |
|---|---|
| `tools/mjm.py` | Decode `mjm0/1/2` files (PKWARE DCL, RNC1). `python3 tools/mjm.py FILE...` prints sizes; import `mjm_unpack` for scripting. |
| `tools/rnc1.py` | RNC method 1 decompressor (used by `mjm.py` and for the Cadaver 2 `.BIN` drivers). |
| `tools/pcman.py` | `PCMAN.DAT` → sprite sheet PNG (`python3 tools/pcman.py cadaver1/DATA/PCMAN.DAT sheet.png`). |
| `tools/things.py` | Dump a level's room lists and things by name (`python3 tools/things.py cadaver1/DATA/1 [--things]`). |
| `tools/lng.py` | `.LNG` language tables → text (`python3 tools/lng.py cadaver1/DATA/1/ENG1.LNG [index]`). |
| `tools/mpsections.py` | Walks the section structure of `LEVn.MP2` and the block layout of `ANDATA.DAT`; flags the truncated Payoff file. |
| `tools/pc1.py` | `*.PC1` → PNG (`python3 tools/pc1.py cadaver1/LOGO.PC1 out.png`). |
| `tools/dosemu.py` | **Emulator harness** (Unicorn): runs an *unpacked* original EXE with a DOS INT 21h subset, keyboard/timer feeding and a planar EGA/VGA model. Lets us run the game's own loaders and renderer and inspect memory/VRAM. |
| `tools/emu_run.py` | Convenience driver around `dosemu.py`: feeds keys, writes a PNG every 25 M instructions, and dumps conventional memory (`*_mem.bin`) and VRAM (`*_vram.bin`) at the end. |
| `tools/dosbox_capture.sh` | Runs the *retail* game in real DOSBox on a virtual X display, sends keys, takes native-resolution screenshots. The independent reference to compare the emulator and the C port against. |
| `tools/renames.py`, `tools/renames_lib.py` | Maintain the rename tables in `renames/` (check / fill / port between the two games / merge renames exported from IDA / stats). See `renames/README.md`. |
| `ida_scripts/apply_renames.py`, `ida_scripts/export_renames.py` | IDA 8.3 scripts (run through `run_ida_script.ps1`): apply a table to your local `.idb`, or dump the names already in an `.idb`. |
| `tools/test_renames.py` | Tests for the above (the apply script is run against a fake IDA API backed by the real binaries). |
| `tools/gen_test_vectors.py` | Regenerates `src/tests/vectors.h` (sizes + FNV hashes from the Python decoders) for the C unit test. |

## Reproducing the in-game frame

```bash
python3 tools/emu_run.py cadaver1 unpacked/cadaver1/CAD.EXE /tmp/g1 400 0231 011b
# 0231 = key '1' (English), 011b = Esc ("don't restore a saved game")
```

After ~150 M instructions the harness shows the same level-1 cavern frame as
DOSBox (`tools/dosbox_capture.sh cadaver1 out cadaver "6:SHOT 4:SHOT 4:SHOT 5:1 …"`).
Throughput is ≈4 M instr/s without video access and roughly 1 M instr/s while
the game is drawing, so a 400 M run takes several minutes.

### How the harness works / caveats

* The whole 1 MB address space is flat RAM except `A0000–AFFFF`, which is an
  MMIO window implementing a planar VGA model: 4×64 KiB planes, latches,
  write modes 0–3 (function select, rotate, bit mask, set/reset), read modes
  0/1, map mask, CRTC start address, attribute-controller palette and the DAC.
* INT 8 is injected every 20 000 instructions when the game has hooked it. Raw
  scancodes queued in `Emu.scan_queue` (make = `sc`, break = `sc|0x80`) are delivered
  one per slice through the game's INT 9 handler and port 60h, so the emulated
  player can be walked around (the object names in `formats.md` §6.4 were captured
  this way: hook `text_get_string`, `1000:75AF`, and print DX and the registers).
  `INT 16h` keys given as `pending_keys` are released only to **blocking**
  reads (AH=0/10h) — the game flushes the keyboard buffer with non-blocking
  peeks before each prompt, so releasing earlier loses the key.
* Only the INT 21h/10h/16h/1Ah calls these games use are implemented; unknown
  ones are logged with `verbose=True`. Sound drivers are loaded (the game
  reads `BEEPER.BIN`) but no audio is produced.
* Useful pattern for finding a decoder without reading the unlabeled disassembly:
  subclass `Emu`, override `on_read` to learn where a file's bytes landed, then
  `uc.hook_add(UC_HOOK_MEM_READ, …)` over that range and histogram
  `(cs, ip)` of the readers. This is how the `.PC1` decoder (`INTRO.EXE
  0B09`) was found in one run.
* Segment values printed by the harness are real load segments (the image is
  loaded at `1000:0`, so `1000:xxxx` equals IDA's `seg000:xxxx`).

## Building the C port

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure   # skips if retail data absent
./build/cadaver -g cadaver1                   # or cadaver2
```
