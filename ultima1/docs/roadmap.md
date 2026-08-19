# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this is tracking against.

## Infra (done, 2026-08-19)

- [x] Set up headless IDA pipeline (`ida_scripts/run_ida_script.ps1` +
      `batch_run_and_export.py`), generalized for 5 IDBs (unlike
      `ultima2`'s single-file driver) by deriving export paths from
      whatever `.idb` `idat.exe` was actually pointed at.
- [x] Wrote `ida_scripts/identify.py`, ran it read-only against all five
      IDBs to catalog current state (root file, segments, function/struct
      naming progress) — see the table in overview.md.
- [x] Verified full export+save round-trip against `ultima1_space.idb`
      (re-ran `identify.py` without `-NoExport`; `.idc` byte-identical,
      `.asm` differs only in incidental ordering/whitespace, not content).
- [x] Fixed `.gitignore` (was still ultima2-specific filenames).
- [ ] Create `docs/file-formats.md` once the first on-disk format
      (savegame, map, etc.) is actually traced — don't stub it empty.

## Executable order

Not yet decided with Paul. Candidates, in order of how much prior work
exists (i.e. how close each is to "fully documented" already):

1. **`ultima1_out` (OUT.EXE)** — furthest along (266/353 functions, 11
   structs). Likely the biggest single module (overworld + towns +
   dungeons) so also the highest payoff to finish first.
2. **`ultima1_space` (SPACE.EXE)** — next furthest (156/210, 16 structs).
   Self-contained minigame, probably the fastest to actually *finish*
   even though it's not the most-complete-by-percentage.
3. **`ultima1_gen` (GEN.EXE)** — 44/113, 5 structs.
4. **`ultima1` (ULTIMA.EXE)** — 39/100, 4 structs. Probably just a
   launcher/title-screen/chainloader — worth confirming that hypothesis
   early since it may be small enough to finish quickly once looked at
   directly.
5. **`ultima1_mondain` (MONDAIN.EXE)** — 1/191, no structs. Essentially
   starting from scratch; role not yet confirmed (best guess: Mondain's
   castle / final confrontation, given the name).

## Per-executable next steps

Not started yet for any executable this session — first task is to
open the chosen starting IDB's existing `.asm` (or a targeted
`ida_scripts` report script) and read through what prior sessions
already established, rather than re-deriving it from scratch.
