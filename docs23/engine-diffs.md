# Chapter 2 vs. Chapter 3 engine differences

Living reference for confirmed behavioral differences between
`yendor2.idb` (Book I Chapter 2, the fully-documented baseline) and
`yendor3.idb` (Book I Chapter 3, confirmed via BinDiff to share the
same engine). Unlike `overview.md`, this file can be edited/corrected
in place as understanding improves — it's a reference, not a session
log. See `overview.md` for the dated narrative of how each entry was
found.

**Methodology note**: BinDiff similarity scores are a starting filter,
not a verdict — spot-checks during the initial >=0.95-similarity bulk
import already found two real differences at 0.99 and 0.98 similarity
(see below), so a high score only means "almost certainly the same
function," not "identical." Real verification needs pulling the
called-function sequence for each match (mnemonic + call-target diff,
filtering out call targets that only differ because the *other*
function hasn't been renamed yet) and reading anything that doesn't
fully explain away as compiler/register-allocation noise.

## Confirmed differences

### `ProcessMonsterAttackTurn` (BinDiff similarity 0.99)

Chapter 3's version has one extra call, to a new function (yendor3
`sub_286D8`, not present in yendor2) not found anywhere in Chapter 2's
version. That function: tests a flag bit (`ds:0xCF63` bit `0x8`) and,
if set, calls a far function pointer stored at `ds:0xFA00` with `bx=8`.
Also called from Chapter 3's alchemy-refinement-yield function
(yendor3 `sub_1C13E`, matched at only 0.05 similarity to
`ComputeAlchemyRefinementYield` — itself a low-confidence match worth
its own review). Reads like a new hook/callback dispatch mechanism
added in Chapter 3 (an event-type selector via `bx`, gated by a flag),
not yet identified further. **Open question**: what is at `ds:0xFA00`
(a function-pointer variable) and `ds:0xCF63` (a flags word) — likely
new globals with no Chapter 2 counterpart.

### `ApplyEncodedItemEffect` (BinDiff similarity 0.98)

Chapter 3's version (1691 disassembly lines vs. Chapter 2's 1642) adds
calls to `WaitForSoundDriverIdle` — a function that exists in *both*
games (100%-matched, so not itself new), but in Chapter 2 is only
called from `TryPlaySoundCueAlt`. Chapter 3 calls it from within the
item/spell effect dispatcher too, at least twice. Not yet traced which
specific effect-type branch(es) added this — needs a closer read once
`ApplyEncodedItemEffect`'s ~19-branch dispatch is worth revisiting in
detail for Chapter 3 specifically.

## Review status

- 68 functions bulk-imported at BinDiff similarity >=0.95
  (`yendor3/ida_scripts/apply_bindiff_high_confidence.py`) — only a
  handful spot-checked so far (2 of which surfaced the differences
  above); the rest are unverified beyond the similarity score.
- 52 functions at similarity 0.70-0.95: not yet reviewed.
- 77 functions at similarity <0.70: not yet reviewed. Many of the
  lowest scores (well under 0.3) may not be genuine matches at all —
  treat the suggested yendor2 name as a weak hint, not a starting
  assumption, when reviewing these.
