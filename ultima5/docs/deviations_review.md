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
2. **NPC schedule look-ahead** (`npc.c:549`, `NPC_06e4`). *Confirmed in the binary:* the original keeps four pointers into the
   four-entry distance array; the guard `!= 99` is applied through a pointer that is `&arr[si-1]` (which for `si` 5..7 points past the
   array at the function's other locals) while the equality test uses `&arr[si-5]`. So the `MATCHING_BUILD` line (`local_10[local_16 - 1]`)
   is what the binary does. The modern `local_16 - 5` additionally refuses invalid directions (value 99) in the equality test; the two only
   differ when the NPC's current distance is itself 99. Practical effect: negligible.
3. **Spell precondition** (`cast.c:2098-2105`, `CAST_1f60`). *Confirmed in the binary, both halves:* the original pushes `param_1` before
   the call to `CAST2_0306` (stub `80e2`), which takes no arguments and returns with a plain `ret`, so the pushed word is simply leaked
   until the function's `mov sp,bp`; and it stores the returned key in `[bp-10ch]` and goes on to cast regardless. `CAST2_0306` returns 0 for
   Space ("Pass"), so in the original a spell is still carried out when the player passes at the "Direction-" prompt (with direction value 0);
   the FM Towns-derived `if (local_10e != 0)` makes the modern build abort the spell instead. *Original behaviour = no abort.*
4. **Talk with `0xfe` response** (`talk.c:304`). *Confirmed in the binary.* The original calls `TOWN_10da` with **no argument**
   (the call at `TALK:03d3` has nothing pushed), so the callee reads the caller's saved `SI` — a pointer into the NPC table, not an
   index — and `TOWN_08d4` then indexes `D_659e[]` with it, far outside the table (indeterminate memory beyond the data segment).
   In practice the "Begone, vermin!" reply never turns the NPC hostile. The modern build passes the real NPC index, so the NPC does
   turn hostile (`0xfd`, schedule type 3) when it qualifies. *Original behaviour = no state change.*
5. **Empty number entry** (`3000.c:764`, `ULTIMA_3b9e`, the numeric prompt — not a name prompt as I first wrote). With nothing typed
   the original skips the digit loop (`--local_c > 0` is false) and then evaluates the *uninitialised* `local_8[0]`, so it returns
   a value derived from stack garbage; modern returns 0.
6. **Combat map access with y < 0** (`4000.c:243`). *Confirmed:* the original `ULTIMA_4402` is just `0xad14 + y*32 + x` with no check
   (the overworld branch masks with `& 0x1f`; the combat branch does not), so a negative `y` reads the bytes before the combat map.
   Modern returns a dummy byte. Only reachable by an actor at the map edge.
7. **Line-of-sight flood past the edge** (`5000.c:499-576`). Modern skips rays that leave the 32x32 / 11x32 buffers and tests an extra
   condition; the original reads the neighbouring memory. Not verified how the original's neighbours look; edge visibility in combat
   and on the viewport border may differ.
8. **`D_2e90`/`D_2e9a` read past their end** (`dungeon.c:1319`, `1395`). The callers index up to `[12]`; the original reads into the
   following data (`D_2e90[10..12] = 0e 17 32`, `D_2e9a[10..12] = 5a 73 52`). **Fixed in `src/`**: both tables now carry those exact
   original bytes (the previous zero padding was wrong, and index 12 was still out of range on the host).
9. **GOG_BUILD** (`2000.c:271-335`, `intro.c:1125-1177`): skips the "which drive" prompts and the drive-letter echo; matches the
   GOG release, not the 1.16 floppy/hard-disk flow. Not individually checked.
10. **Movement result codes** (`mainout.c:360-436`, `MAINOUT_0490`). Where the original does `if (MAINOUT_00da(x)) return;`, AX still holds
    `MAINOUT_00da`'s non-zero result, which modern returns explicitly (equivalent). The "On foot!" and "Attacked at entrance!" paths
    return whatever AX holds after a call (modern: 0); the caller stores it in `local_a`. Possible difference in whether a turn passes.
11. **Look-object record scan** (`lookobj.c:552`, `LOOKOBJ_07e4`). The original stops at the `0xff` record terminator; the modern
    non-matching code ended only at the end of the 1500-byte scratch buffer, so it kept scanning leftover bytes after the last record
    and could match stale data. **Fixed in `src/`**: the terminator test is back (buffer end kept as a safety stop).

## Checked and equivalent

* **Bare hands (weapon `0xff`) in `COMSUBS_0c52` and the hit-stat selection in `combat.c:1360`.** The modern guard replaces an
  out-of-range table read. The binary reads `D_169c[0xff]` (= `0x73`, the `s` of "Crossbow") and `D_1664` is explicitly special-cased
  to 0 for `0xff`; in the combat stat selection the byte is compared with 8 and is not equal, which selects DEX — exactly what the
  guard does. No behaviour change.
* `ULTIMA_6f1e`, the PRNG and the line clipper were genuine differences and are already fixed (see [audit.md](audit.md)).

## Method note

Items 1–6, 8 and 11 and the "checked and equivalent" entries were confirmed against the disassembly (`ULTIMA_6da8`,
`NPC_06e4`, `CAST_1f60`/`CAST2_0306`, `COMSUBS_0c52`, `TALK_031e`/`TOWN_08d4`, `ULTIMA_4402`, the data bytes in `DATA.OVL`, `LOOKOBJ_07e4`).
Items 7, 9 and 10 are described from the C only.
