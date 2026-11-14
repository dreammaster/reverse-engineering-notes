# Candidate ScummVM engine options

Every place where the decompiled/modern build differs from the IBM 1.16 original, or where the original itself has a defect, is
listed here as a possible engine option. **Default = original behaviour** unless noted. Evidence levels:
*binary* = confirmed against the disassembly (see [deviations_review.md](deviations_review.md)); *u5d-match* = the expression is in
the `MATCHING_BUILD` path, which u5d byte-matches against the EXE, so it is what the binary does; *C only* = read from the C, not checked.

## A. Original defects (default: keep; option: fix)

| id | name | original behaviour | optional fix | evidence | source |
|---|---|---|---|---|---|
| `armour_works` | Armour counts | Equipment defence is never added (`cmp si,0FFFFh; jna` always jumps); only the +3 for one tile type applies | Add defence from equipped armour (and add it to the hit calculation in combat) | binary | `6000.c` `ULTIMA_6da8`, `combat.c:1297` (`ENABLE_BUG_FIX`) |
| `equip_def_refresh` | Equipment change updates defence | After equipping, `ULTIMA_6da8` is called and its result ignored (nothing stored) | Store the result (only meaningful together with `armour_works`) | binary (`ULTIMA+6f11`: `call 6da8`, AX not used) | `6000.c:786` |
| `numeric_prompt_empty` | Empty number entry | Returns a value derived from uninitialised stack | Return 0 | binary | `3000.c:764` `ULTIMA_3b9e` |
| `shop_price_overflow` | Shop price formula | Unsigned long division; INT > 33 overflows to huge prices (e.g. 23,593 for INT 40) | Clamp / use signed arithmetic | binary (observed in-game) | shops code (`ULTIMA_*` price routine) |
| `begone_vermin_hostile` | "Begone, vermin!" | `TOWN_10da` gets a stray SI argument; NPC never turns hostile | Pass the real NPC index so qualifying NPCs turn hostile | binary | `talk.c:304` |
| `spell_pass_aborts` | Passing at a spell direction prompt | Spell is still cast with direction 0 | Abort the spell | binary | `cast.c:2098` |
| `combat_map_negative_y` | Combat map y < 0 | Reads bytes before the combat map | Return a dummy tile | binary | `4000.c:243` |
| `look_record_scan` | Look-object scan | Stops at the 0xff terminator (already restored in `src/`) | n/a | binary | `lookobj.c:552` |
| `npc_lookahead` | NPC schedule look-ahead | Tests `arr[si-1]` (reads past the array) | Use `arr[si-5]` | binary; negligible effect | `npc.c:549` |
| `xit_anywhere` | Xit (exit transport) location check | Test `map_id < 0x20 && map_id > 0x29` is always false, so "Not here!" never prints and the command is attempted everywhere | Restrict to the intended map range (`0x20..0x29`; confirm intent first) | binary (`CMDS+0eb4`: `cmp [5893],20; jnc` then `cmp [5893],29; jna` both skip the message) | `cmds.c:927` |
| `xit_vehicle_field` | Xit command: new vehicle record byte `_7` | On dismounting a horse/skiff/carpet the byte stored into the new `D_5c5a[]._7` is an uninitialised stack local (only the ship cases set it; `_7` is the ship hull value) | Store 0 | u5d-match (this is `CMDS_0eb4_XitCmd`, not Board) | `cmds.c:1028` |
| `herbalist_gender` | Herbalist honorific | "milady" if the purchasing party member is female (`[member+9]=='F'`), else "sir". Binary matches the C; the `// BUG` comment is u5d's, behaviour is sensible | none needed | binary (`SHOPPES2+085a`) | `shoppes2.c:559` |
| `conflict_message` | "*** CONFLICT ***" | Printed unconditionally (the `!= 0x80 \|\| != 4` test is always true) | Print only on a real conflict | u5d-match | `6000.c:620` |
| `cmds_always_true` | Command tile filter | `(local_10 & 0xf8) != 0x78 && (local_10 & 0xfc) != 0x2f` is always true | none needed (only a wasted test) | u5d-match | `cmds.c:796` |
| `theft_extra_decrement` | Theft decrement (`TALK_1180`) | The trailing `DecreaseByte(&D_57ad,1)` in the C is unreachable (the retry loop only exits through `return`); the binary has no such code. Theft takes exactly one item | none | binary (`TALK+1180`) | `talk.c:1422` |

## B. Modern-build behaviour (default: original; option: modern)

| id | name | original | modern | evidence | source |
|---|---|---|---|---|---|
| `fm_towns_music` | FM Towns CD music | PC-speaker effects only | Per-map background music | C only | `ENABLE_BGM` (34 blocks), `AUDIO_Dispatch*` |
| `speed_fixes` | Timed delays | CPU-calibrated busy-waits | Sleep-based timing | C only; keep (pacing only) | `ENABLE_SPEED_FIX` (12) |
| `los_edge_guards` | LOS flood at buffer edges | Reads the bytes after the 32x32 combat map (DS:b114..) when a ray leaves it; the extra `local_20c != 0` test is provably equivalent | Skips such rays | instrumented run: the guard fires in ordinary combat (map row 23 + offset 9 = row 32, i.e. one row past the map) so the original really reads out-of-map memory; effect limited to off-map cells, contents depend on neighbouring globals | `5000.c:499-576` |
| `gog_prompts` | GOG-style start-up | "Insert disk / press drive letter" prompts | Skips them (disk-swap only; no achievements involved) | C only; irrelevant for ScummVM | `GOG_BUILD` (8) |
| `move_result_codes` | Movement return values | AX leaks through after "On foot!"/"Attacked at entrance!" | Returns 0 | partly binary: the direction-move path (`MAINOUT+0490`) returns `MAINOUT_00da`'s AX exactly as modern; the "On foot!"/"Attacked at entrance!" exits of `MAINOUT+0790` return whatever AX holds after `PrintString`/`Combat` (indeterminate), modern returns 0 | `mainout.c:360-436, 609-650` |
| `translation` | Translation hooks | none | Alternate string tables | C only | `ENABLE_TRANSLATION` |
| `grap_overlay` | Graphics overlay | none | Debug/overlay drawing | C only | `ENABLE_GRAP_OVERLAY*` |

## C. Return-value conventions (no option needed)

The `// sic` returns (`cast.c` ×6, `cmds.c` ×3, `combat.c` ×2, `mainout.c` ×2, `6000.c:395`) mark spots where the original returns
whatever is left in AX; the C keeps the shape of the original. They affect callers only when the caller tests the value. `cast.c:620`
(`map_id > 0x7f && D_58a3 == 0`) is annotated `sic` too: odd, but it is what the binary compiles.

## Suggested ScummVM option set

1. **Armour fix** (`armour_works` + `equip_def_refresh`) — the most requested fan fix.
2. **Original spell / vermin behaviour** — off by default; on = fixed versions of `spell_pass_aborts`, `begone_vermin_hostile`.
3. **Price fix** (`shop_price_overflow`).
4. **Music** (`fm_towns_music`) as an audio option, not a gameplay one.
5. Everything else stays at original behaviour (crash-avoiding guards on out-of-range reads are applied silently).

## Still to verify against the binary

Whether the out-of-map LOS reads (`los_edge_guards`) can ever change a visible on-map cell (needs a differential run against the original). `gog_prompts` are only the disk-swap / drive-letter prompts (no achievement code exists in the sources). The *u5d-match* rows rely on u5d's byte-matching claim for those functions.
