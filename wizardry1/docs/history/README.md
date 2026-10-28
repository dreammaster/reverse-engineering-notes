# Working history — conversation transcripts and screenshots

This folder is the complete record of how the Wizardry I work was done with Claude Code, exported on
2026-10-28 so a new contributor can see *why* things are the way they are.  The current state and
workflow are in [`../handoff.md`](../handoff.md); read that first and dip in here for rationale.

## Contents

| file | what |
|---|---|
| `session1-part01…04.md` | first session, 2026-09-02 → 09-03: project kick-off through the early engine (UTC timestamps) |
| `session2-part01…09.md` | second session, 2026-09-03 → 10-28: starts with a compaction summary of session 1's tail (so the two overlap on 09-03 00:27–02:41), then the rest of the Apple-derived engine, RNG validation, the DOS-faithful rewrite and the handoff request |
| `session*-images.tsv` | every embedded image: file, timestamp, who supplied it (`user` = a screenshot the project owner pasted; `tool output` = something rendered during the work, e.g. an oracle or engine screen) |
| `screenshots/` | all of those images (`session2-014.png` etc.) |
| `reference-screenshots/` | the ten DOSBox screenshots the project owner supplied as references, copied out with descriptive names (see below) |

How the transcripts were produced: `tools/export_transcript.py <session.jsonl> <outdir> <prefix>` (the raw
`.jsonl` files live in `~/.claude/projects/…` and are 17–35 MB each because they carry every tool output).
In the Markdown, user and assistant text is complete; tool calls are one-line summaries (`› Bash: …`),
tool output is cut to ~240 characters (`↳ …`), and the model's private reasoning is omitted.  Pre-compaction
summaries (long "This session is being continued…" user messages) are kept in full — they are the densest
account of decisions, formats discovered and conventions, and are the best place to start reading.

Two screenshots the owner pasted on 2026-09-02 (≈12:38 and 12:45 UTC, while the monster-portrait driver was being
mapped) were not stored by the transcript, only as `[Image: …]` placeholders; they are not recoverable.

## The reference screenshots

DOSBox 0.74 captures of the real game, supplied 2026-09-04.  **They validate; they do not define.**  Early in
the project layouts were derived from them and turned out wrong in structure, which led to the instruction
recorded on 2026-10-25 (session2-part06, 20:43): *"I gave the screenshots as a reference, but I don't want you
to base your work solely on them … identify all the drawing code in the PC executable and use that as the
main source for all user interface rendering."*  That is the basis of the oracle approach.

| file | shows |
|---|---|
| `title-scenario-disk-prompt.png` | title screen: "YOU CANNOT PLAY WITHOUT MAKING A SCENARIO DISK! S)TART GAME / M)AKE SCENARIO DISK" |
| `castle-hub.png` | CASTLE hub menu over the party roster (6 characters) |
| `tavern-menu.png` | Gilgamesh's Tavern menu |
| `temple-of-cant.png` | Temple of Cant prompt (roster partly covered by the dialog) |
| `boltacs-welcome.png` | Boltac's "WELCOME … WHO (#) WILL ENTER?" |
| `roller-enter-name.png` | Training Grounds: enter a name |
| `roller-choose-alignment.png` | Training Grounds: choose alignment |
| `roller-point-allocation.png` | Training Grounds: bonus-point allocation |
| `maze-corridor-side-passages.png` | maze view with side passages (menu bar `C)AMP S)TATUS I)NSPECT O)FF A-W-D K`) |
| `maze-corridor-ahead.png` | maze view of a corridor with a wall ahead |

## Timeline (commit dates are local time; transcript timestamps are UTC)

| date | milestone | where to read |
|---|---|---|
| 09-02 | Kick-off. Discover the `.DSK` files are UCSD p-System volumes; p-machine opcode table; `SCENARIO.DATA` container; p-code disassembler; `ASCII.KRN` string-pool cipher; global map; engine skeleton; ROLLER rules; RNG reversed; platform + text-grid layers; title and monster formats | session1-part01–03 |
| 09-02/03 | Apple-derived port of the town (Castle, Tavern, Inn, Boltac's, Temple), the maze (RUNNER, DRAWMAZE, HUD), COMBAT family, rewards, SCNMSG, CAMP, EQUIP | session1-part03–04, session2-part01–03 |
| 09-03 | Save/restore, body recovery, SETTIME, FRIENDLY, breath/run/yell, groups, camp spells, "Phase 3 complete"; RNG validated bit-exact against a live DOSBox capture (the owner drove the DOSBox debugger) | session2-part02–04 |
| 09-04 | CHK4WIN; `SYSTEM.INTERP`'s CONUNIT driver disassembled; monster-portrait tile geometry pinned and rendered in combat; first UI "border pass" and maze wireframe, then reworked against the owner's DOSBox screenshots | session2-part04–06 |
| 10-25 | **The pivot.** The owner points out that screenshot-driven layout is not enough: use the executable's own drawing code. Drawing inventory (`drawing.md`), then the oracle (`wizvm.py`/`conunit.py`) | session2-part06–07 |
| 10-26 | `wiz::Console` (bit-exact compositor); CASTLE, SHOPS, title, ROLLER, RUNNER ported and verified; `DosFlow` + `wiz1 dosgame` | session2-part07–08 |
| 10-27 | CAMP, SPECIALS, COMBAT family, REWARDS, UTILITIE camp side ported; 50 oracle scenarios | session2-part08–09 |
| 10-28 | Handoff request → documentation review, `docs/handoff.md`, this folder | session2-part09 |

## Reading tips

* Search for a segment or proc name (`CAMP:41`, `UTILITIE:23`, `REWARDS`) to find where it was ported; the
  assistant's text before each commit explains the non-obvious decisions.
* The long "This session is being continued…" summaries (session1 part02 and part03; session2 part01, 03, 05, 06, 07
  and 08 — search for that phrase) restate the project goal, constraints, key technical concepts, errors hit
  and how they were fixed.  The one in session2-part08 covers the DOS-faithful port from the oracle up to REWARDS.
* Everything of lasting value was also moved into code comments (each routine cites its p-code proc), the
  docs in `../`, and the repo-level `../../../docs/`.
