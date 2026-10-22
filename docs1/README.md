# Book I, Chapter 1 ("Yendor 1")

Chapter 1 is a distinct game from Chapters 2/3 (confirmed narratively, not
just a marketing chapter split — see [`docs23/overview.md`](../docs23/overview.md)'s
"Book I backstory recap" note) and uses a different engine, so it gets its own
docs here rather than folding into [`docs23/`](../docs23).

- [`overview.md`](overview.md) — append-only dated session log.
- `file-formats.md` — living reference for confirmed data formats (not started).

Layout:

- `yendor1/game/` — the original game files (gitignored).
- `yendor1/*.idb` / `*.asm` / `*.idc` — one IDA database per executable
  (`.idb` gitignored; the `.asm` listing and `.idc` dump are tracked).
- `yendor1/ida_scripts/` — the headless IDA pipeline (see `overview.md`).
- [`src1/`](../src1) — the C/SDL reimplementation (not started).
