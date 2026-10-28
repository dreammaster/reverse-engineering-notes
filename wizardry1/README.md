# Wizardry I — reverse engineering / reimplementation

Dissecting *Wizardry: Proving Grounds of the Mad Overlord* (DOS "Ultimate
Wizardry Archives" release) and rebuilding it as a standalone C++ program,
en route to a shared ScummVM engine for Wizardry I–V.

Engine-wide reference (the UCSD p-System stack, disk / file formats, the
p-machine, the RNG — all shared with Wiz II/III) lives one level up in
[`../docs/`](../docs/).  This folder holds the Wizardry I-specific work.

**Taking over the project? Start with [`docs/handoff.md`](docs/handoff.md)** (current state, workflow, gaps).
Then [`../docs/engine.md`](../docs/engine.md), [`docs/drawing.md`](docs/drawing.md) and [`docs/roadmap.md`](docs/roadmap.md).

## Layout

| path | what |
|---|---|
| `sources/` | Thomas Ewers' 2014 byte-exact Apple II Pascal reconstruction (reference) |
| `tools/` | Python: UCSD disk reader, p-code disassembler/decompiler, **the p-code oracle** (`wizvm.py`, `conunit.py`), regression scripts (`screens_check.py`, `cmpgrids.py`, `replay_check.py`), decoders, `export_transcript.py` |
| `ida_scripts/` | headless IDA (`idat.exe`) driver + analysis scripts |
| `docs/` | Wiz1-specific findings: `handoff.md`, overview, roadmap, `drawing.md`, combat / maze / town, decoded tables, strings; `history/` = the working conversation + screenshots |
| `engine/` | the standalone C++ engine (`wizcore` lib + `wiz1` CLI); `wiz1 dosgame` plays the DOS-faithful ports |
| `extracted/` | files carved out of the game disks *(gitignored, regenerable)* |
| `*.asm` / `*.idc` | committed IDA analysis backups |

## Quick start

```bash
# list / extract a game disk (adjust path to your Archives install)
python tools/ucsd_disk.py list       "C:/games/wizard15/WIZ1.DSK"
python tools/ucsd_disk.py extractall  "C:/games/wizard15/WIZ1.DSK" extracted/wiz1
```

```powershell
# (re)build the p-machine IDB from the extracted interpreter
copy extracted\wiz1\SYSTEM.INTERP wiz1_interp
& "C:\Program Files\IDA Pro 8.3\idat.exe" -A -p8086 -T"Binary file" `
    -S"ida_scripts\batch_run_and_export.py ida_scripts\setup_interp.py" `
    -o"wiz1_interp.idb" wiz1_interp

# run a later analysis script against it
ida_scripts\run_ida_script.ps1 -Idb wiz1_interp -ScriptName ida_scripts\some_script.py
```
