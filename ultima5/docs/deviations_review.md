# Review of the modern build's deviations from the IBM 1.16 code

Companion to the generated [`audit/deviations.md`](../audit/deviations.md) (157 conditional blocks, 162 annotations). The blocks were
read and, where they change behaviour, checked against the disassembly. "Original" below means the IBM 1.16 `ULTIMA.EXE`/`*.OVL`.
For each item the ScummVM engine needs a decision: reproduce the original, keep the fix, or make it an option.

## Summary

| group | blocks | effect on the game | recommendation |
|---|---|---|---|
| Platform / file I/O / disk swap (`!TARGET_DOS16`: `2000.c`, `1000.c`, `savegame.c`, `shoppes.c`, `font.c`, `intro.c`, `ultima.c`) | ~35 | none (host replaces DOS calls, saves go through the verified loader/writer) | n/a for ScummVM |
| Music and effects dispatch (`ENABLE_BGM` 32 blocks, `AUDIO_Dispatch*`, `intro.c` title sfx) | ~40 | adds FM Towns CD music; the IBM game has PC-speaker effects only | optional feature; default to the original sounds |
| Speed fixes (`ENABLE_SPEED_FIX`, 10) | 10 | replace CPU-calibrated busy-waits with timed sleeps | keep (pacing only) |
| Uninitialised variable / `ASSERT` / bounds guards (`local_x = 0`, `ASSERT`) | ~20 | none intended (original reads indeterminate stack data) | keep |
| "Validate return value" fixes (`return local_x;` instead of the original's bare `return;`) | 11 | the original returns whatever is in AX; callers that test it see an arbitrary value | keep, check callers when porting |
| **Behavioural differences from the original** | 11 | see below | decide each |
| Verified equivalent (looks like a change, is not) | 2 | none | keep |

## Behavioural differences (decisions needed)

1. **Armour has no effect in the original.** `ULTIMA_6da8` (player defence) loops over the six equipment slots with
   `cmp si,0FFFFh; jna skip` where `si` is a zero-extended byte, so the jump is *always* taken and nothing is ever added
   (only the `+3` for one tile type remains). The u5d `MATCHING_BUILD` macro `CMP(a) ((a) > -1)` is therefore not what the binary
   does (a C compiler folds it to "always true"); the modern build uses `(s8)a > -1`, i.e. armour counts, and `combat.c:1297`
   (`ENABLE_BUG_FIX`) additionally adds the defence into the hit calculation. This is the well-known DOS armour bug.
   *Original behaviour = defence from equipment is 0.*
2. **NPC schedule look-ahead** (`npc.c:549`). Original: `local_10[local_16 - 1] != 99` (an index that does not match the test
   two lines below it); modern: `local_16 - 5`, marked "BUGFIX?". The `MATCHING_BUILD` branch is the one that equals the binary.
3. **Spell effect precondition** (`cast.c:2098-2105`). Original: `CAST2_0306(param_1)` (an argument the callee ignores; the author
   notes "original code error?") and no check of the result; modern: `CAST2_0306()` and the spell body runs only if it returned
   non-zero (taken from the FM Towns version).
4. **Talk with `0xfe` response** (`talk.c:304`). Original passes an uninitialised `SI` to `TOWN_10da`; modern passes `param_1`.
5. **Empty name on the keyboard prompt** (`3000.c:764`). Modern returns immediately when nothing was typed; the original falls
   into the `while (--local_c > 0)` loop with `local_c == 0` (underflow).
6. **Combat map read with y < 0** (`4000.c:243`). The original computes an address outside the combat map (reads whatever is in
   memory before it); modern returns a dummy byte. Only reachable by actors at the map edge.
7. **Map drawing past the edge** (`5000.c:499-576`). Modern clips reads outside the 32x32 view buffers; the original reads
   neighbouring memory, which can show stray tiles at the border.
8. **`D_2e90`/`D_2e9a` tables padded to 12 bytes** (`vars.c`). Original size 10, indexed out of range by one caller; the modern
   build pads (as the FM Towns version does). The values read past the end in the original are the next variable's bytes.
9. **GOG_BUILD** (`2000.c:271-335`, `intro.c:1125-1177`): skips the "which drive" prompts and the drive-letter echo; matches the
   GOG release, not the 1.16 floppy/hard-disk flow.
10. **Movement result codes** (`mainout.c:360-436`): modern returns `MAINOUT_00da`'s value to the caller; the original tests it and
    returns nothing.
11. **Bounds loop in `lookobj.c:552`**: modern stops at the end of the scratch buffer; the original scans until it finds a zero byte.

## Checked and equivalent

* **Bare hands (weapon `0xff`) in `COMSUBS_0c52` and the hit-stat selection in `combat.c:1360`.** The modern guard replaces an
  out-of-range table read. The binary reads `D_169c[0xff]` (= `0x73`, the `s` of "Crossbow") and `D_1664` is explicitly special-cased
  to 0 for `0xff`; in the combat stat selection the byte is compared with 8 and is not equal, which selects DEX — exactly what the
  guard does. No behaviour change.
* `ULTIMA_6f1e`, the PRNG and the line clipper were genuine differences and are already fixed (see [audit.md](audit.md)).

## Method note

Items 1, 2, 3 and the "checked and equivalent" entries were confirmed against the disassembly of `ULTIMA_6da8`, `COMSUBS_0c52`
and the surrounding code. Items 4–11 were read in the C and their original behaviour inferred from the `MATCHING_BUILD` branch and
the source comments, not individually disassembled; confirm any of them against the binary before relying on it.
