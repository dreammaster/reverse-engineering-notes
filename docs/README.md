# Repo layout

This repo covers reverse-engineering and reimplementation work across
the Yendorian Tales games, which span **two unrelated engines**:

- **Book I, Chapter 1** ("Yendor 1") — a distinct, separate game from
  Chapters 2/3, not just a marketing split (confirmed via its own
  narrative backstory references). Different engine, not yet started.
  Its own analysis and reimplementation work will live in
  [`docs1/`](../docs1) and [`docs1/`](../src1) once that begins.
- **Book I, Chapters 2 and 3** ("Yendor 2" and "Yendor 3") — confirmed
  via BinDiff to share the same underlying engine (a large fraction of
  Chapter 3's functions are exact or near-exact structural matches to
  already-named Chapter 2 functions). Their shared engine analysis and
  reimplementation work lives in [`docs23/`](../docs23) and
  [`src23/`](../src23), so the eventual C source can serve both games
  without a later restructure.

## Per-game directories

Each game's own IDA project artifacts — the `.idb`/`.asm`/`.idc`
files, the `ida_scripts/` headless-analysis pipeline, and the original
`game/` files (gitignored) — stay in that game's own top-level folder
(`yendor1/`, `yendor2/`, `yendor3/`), since those are genuinely
per-executable. Only the *understanding* derived from them (docs) and
the *reimplementation* (src) are shared where the engine is shared.

## Where things are

| Folder | Contents |
|---|---|
| `yendor2/` | Chapter 2's IDA project (idb/asm/idc, ida_scripts, game files) |
| `yendor3/` | Chapter 3's IDA project (idb/asm/idc, ida_scripts, game files) |
| `docs23/` | Shared engine docs for Chapters 2 & 3 (`overview.md` dated log, `file-formats.md` living reference, `roadmap.md`, manuals) |
| `src23/` | Shared C reimplementation for Chapters 2 & 3, plus `src23/tests/` |
| `yendor1/` | (future) Chapter 1's IDA project |
| `docs1/` | (future) Chapter 1's own engine docs, once started |
| `src1/` | (future) Chapter 1's own C reimplementation, once started |
