# Struct layouts drift too, not just function boundaries

Checked whether the 2011 reference source's struct layouts can be trusted
for the 2002 Rob Blanc 1 binary, before generating IDA struct definitions
for `apply_structs.py`. Short answer: **mostly no.** Verify every struct
against real evidence (an already-known IDB size, or an allocation-size
constant in the disassembly) before applying it -- do not copy 2011 struct
layouts wholesale.

## Verified DRIFTED (2011 layout is unsafe to apply as-is)

- **`CharacterInfo`**: IDB already knows the real size from usage analysis:
  `0x140` = 320 bytes. The 2011 struct is far bigger -- `short
  inv[MAX_INV]` alone is 602 bytes with `MAX_INV` = 301 (`Common/acroom.h:2478`).
  AGS's inventory-slot limit was almost certainly much smaller in 2002.
- **`SpriteCache`**: IDB known size `0x10` = 16 bytes. The 2011 `class
  SpriteCache` (`Common/sprcache.h`) has ~16 data members (LRU tracking,
  compression, cache-size accounting) -- 60+ bytes. The whole
  caching/eviction subsystem looks like a later addition; 2002 was
  presumably just a couple of raw arrays.
- **`ccInstance`**: no pre-existing IDB size, but `ccCreateInstanceEx`
  (disassembly) allocates it with `push 9A8h ; call malloc` -- a fixed
  `malloc(0x9A8)` = 2472 bytes. Computing the 2011 `Common/CSCOMP.H:242`
  struct's size by hand (`MAX_CALL_STACK`=100, `CC_NUM_REGISTERS`=8) gives
  ~1300 bytes -- roughly 1172 bytes short. The gap is suspiciously close to
  what you'd get if `MAX_CALL_STACK` were ~200 instead of 100 in 2002 (each
  of the three call-stack arrays scales with it), but that's a guess, not
  a finding -- needs the actual constant recovered, not assumed.
- **`GameSetupStructBase`**: IDB known size `0xBF84` = 49028 bytes. Hand-
  computing the 2011 struct (`Common/acroom.h:2816`) -- including
  `gamename[50]`, `options[100]`, `paluses[256]`, `defpal[256]`,
  `messages[MAXGLOBALMES=500]`, etc. -- comes to roughly 3900 bytes.
  49028 vs ~3900 is not a rounding difference; the 2002 struct almost
  certainly embedded large fixed-size arrays directly (fonts, sprites,
  views, etc.) that the 2011 version replaced with pointers to
  dynamically-allocated data. This matches the general pattern seen
  elsewhere in this project: old AGS = monolithic/fixed-size, later AGS =
  modular/dynamic.

## Verified SAFE (matches exactly, or is layout-independent)

- **`roomstruct`**: IDB already has this fully labeled -- `walls`,
  `object`, `lookat`, `regions`, 4 pointers = exactly `0x10` = 16 bytes,
  matching `Common/acroom.h:207` perfectly. Already complete, no action
  needed.
- **`block`** = `BITMAP*`: a pointer typedef. Size (4 bytes on x86) doesn't
  depend on `BITMAP`'s internal layout at all, so it's safe regardless of
  whether `BITMAP` itself is fully known. Applied via `apply_structs.py`.

## Partial field evidence recovered in passing (function-matching work)

Not full struct recoveries, just noting real data points found incidentally
while matching functions, so they're not lost:

- **`GUIMain`** (via `remove_popup_interface`, `Engine/AC.CPP:5339`, matched
  to `sub_40D662`): element size **0x184** (388 bytes) confirmed by an
  `imul reg, 184h` array-index computation; `on` field at **+0x90**;
  a `popupyp`-comparable field at **+0x44**. No known IDB size to cross-
  check against yet (no `malloc`/`push <size>` allocation site found so
  far), so treat as leads for later recovery, not verified-safe.
  Corroborated and extended via three more matched member functions:
  `GUIMain::is_mouse_on_gui` (`sub_407587`) confirms `on` at +0x90 again;
  `GUIMain::mouse_but_up` (`sub_407C58`) adds `mousedownon` at **+0x60**,
  an `objs[]` pointer array at **+0x94**, and virtual method `MouseUp` at
  **vtable slot +0x10**; `GUIMain::mouse_but_down` (`sub_407BCA`) adds
  `mouseover` at **+0x54**, `x` at **+0x28**, `y` at **+0x2C**, and virtual
  method `MouseDown` at **vtable slot +0xC**.

  Running tally for `GUIMain` (2002 binary, not yet a complete struct):
  `+0x28`=x, `+0x2C`=y, `+0x44`=popupyp(?), `+0x54`=mouseover, `+0x60`=
  mousedownon, `+0x90`=on, `+0x94`=objs[] (pointer array), element size
  `0x184`. Vtable (on whatever `objs[]` points to, e.g. GUIObject-derived):
  `+0xC`=MouseDown, `+0x10`=MouseUp.

## GUIMain: now applied as a partial struct

Unlike everything above, the `GUIMain` field evidence gathered while
matching member functions (`remove_popup_interface`, `is_mouse_on_gui`,
`mouse_but_down`, `mouse_but_up`) was read directly off real disassembly
instructions for *this* binary -- not copied from 2011 source and hoped
for the best. That makes it safe to apply, unlike the structs above. Now
defined (partially) in `reversing/scripts/apply_structs.py`:

```
+0x28  x
+0x2C  y
+0x54  mouseover
+0x60  mousedownon
+0x90  on
+0x94  objs[60]   (void*, length inferred from total size, not independently confirmed)
size   0x184 (388 bytes, confirmed via imul reg,184h in remove_popup_interface)
vtable (on whatever objs[] points to): +0xC = MouseDown, +0x10 = MouseUp
```

Everything else in the 388 bytes is unnamed padding (`_pad0`..`_pad3`) --
genuinely unknown, not guessed. Extend this as more `GUIMain`/`GUIObject`
methods get matched, following the same "only what's directly evidenced"
discipline.

## CharacterInfo: 5 fields recovered from real access evidence

Same methodology as `GUIMain` above, but done as a dedicated pass rather
than a byproduct of function matching: `reversing/scripts/find_struct_accesses.py`
scans the whole disassembly for every `mov reg, playerchar` followed by a
`[reg+NNh]` dereference, groups by offset, and lists which (already-matched)
functions touch each one -- so the busiest/most-corroborated offsets can be
tackled first.

Result for `playerchar` (a `CharacterInfo*`), 49 access sites across 9
distinct offsets:

| offset | field | confidence | evidence |
|---|---|---|---|
| +0x08 | `wait` | medium-high | `update_stuff` decrements it exactly like source's `if (chi->wait>0) chi->wait--;` lip-sync countdown (`AC.CPP:6676`) |
| +0x0C | `room` | high | `SetPlayerCharacter` saves it, switches `playerchar`, and calls `NewRoom(playerchar->room)` only if it changed -- matches source's logic exactly |
| +0x14 | `x` | high | exact arg-order match: `mainloop` calls `get_hotspot_at(playerchar->x, playerchar->y)`, and the disassembly pulls args from `playerchar+0x14`/`+0x18` in that order |
| +0x18 | `y` | high | see `x` above |
| +0x34 | `activeinv` | high | `SetActiveInventory(-1)` sets `[playerchar+0x34] = -1` verbatim, matching source's `player.activeinv = -1;` |

Also confirmed in passing: `SetPlayerCharacter` does `imul ecx, 140h` when
indexing the global `game_chars` array -- independent cross-check that
`sizeof(CharacterInfo)` really is `0x140`, matching the IDB's pre-existing
size inference.

**Important:** none of these offsets match their 2011 counterparts (2011's
offset 0 is `int defview`; 2002's offset 0 is something else entirely, and
2011's `x`/`y` would be at +0x14/+0x18 too *if* the earlier fields hadn't
grown -- the match here is coincidental, not because the layout survived).
Field *names* were chosen because the *behavior* observed matches a known
2011 field's role, not because of positional correspondence -- do not
assume any other offset lines up just because these five happened to.

**Open contradiction, left unresolved:** the pre-existing IDB annotation
called `+0x00` a `word` (`field_0`). But `update_stuff` reads it with a
full 32-bit `mov edx, [ecx]` (not `movzx`/`movsx` from a word), which
wouldn't make sense if it were genuinely a 2-byte field. Flagged rather
than silently kept or overridden -- needs a closer look before trusting
either interpretation.

**Also flagged, not fixed:** the pre-existing `inv` field (dword @ +0x44)
is suspicious -- named after 2011's `short inv[MAX_INV=301]`, but this
2002 struct is only 320 bytes total, nowhere near enough room for that
array. If a per-character inventory array exists here at all, it's a very
different size/shape. Kept as-is (not this pass's finding to begin with)
but worth revisiting.

Applied via `reversing/scripts/apply_structs.py`, which deletes the IDB's
existing skeletal `CharacterInfo` and recreates it whole (parse_decls can't
edit an existing struct in place) -- safe here since nothing yet applies
`CharacterInfo` to live instruction operands, only uses it as a bare type.

## CharacterInfo, round 2: a self-correction, plus two more fields

Went looking for more fields by reading `update_stuff`'s per-character loop
directly (the `chi` pointer, a local `CharacterInfo*`, not the `playerchar`
global -- `find_struct_accesses.py` only greps for accesses off a *named
global*, so this pass had to be done by hand). Found a contradiction with
the earlier round's own conclusion:

**Retracted:** `+0x08 = wait` (medium-high confidence, round 1). Reading
`update_stuff`'s character loop directly shows `chi`'s `+0x08` used as
`imul ecx, 8D4h` -- an array index into a large per-entry data table. That
is an unambiguous "this holds a view number" pattern, not a countdown
timer. **Corrected to `+0x08 = view`** (high confidence) and the earlier
"wait" label removed. Lesson: a single decrement-pattern match (round 1's
evidence for "wait") is weaker than an index-into-a-typed-array pattern --
weight structural specificity, not just "the shape looks similar to what
source does," and be willing to overturn an earlier medium-confidence call
when better evidence turns up.

**New, medium confidence:** `+0x38 = loop` (a `short`). `update_stuff`
writes an 8-entry direction-lookup table value (`dword_4B42C8[idx]`, the
same 8-slot table `sub_40EB43` searches, see the open lead notes) into this
offset -- the classic AGS "walking direction -> loop number" pattern for
8-directional character sprites.

**Flagged, not resolved (at the time):** the pre-existing `walking` label
at `+0x3C` looked suspicious -- `update_stuff` performs repeated modular
arithmetic on it (`cx -= 3E8h` / `1000`, then divides/mods by `2710h` /
`10000`) immediately after writing `loop`, which didn't obviously fit a
simple walking-state field.

## CharacterInfo, round 3: walk_character resolves the round-2 open questions

Read `Engine/acchars.cpp:37`'s `walk_character` (already matched) directly
against its disassembly (`game_chars[chac]`, stored in `var_8` -- referred
to as `chin` in source). It touches most of the still-unknown offsets in
one place, resolving both open leads from round 2 and adding three more:

- **`+0x2C = idletime`, `+0x2E = idleleft`** (both high confidence): source
  has `if (chin->idleleft < 0) { ReleaseCharacterView(chac);
  chin->idleleft=chin->idletime; }` -- disasm matches essentially verbatim:
  a signed `[+0x2E] < 0` check gates a call to the already-matched
  `ReleaseCharacterView`, followed by `[+0x2E] = [+0x2C]`.
- **`+0x3C = walking`, CONFIRMED** (upgraded from "flagged as suspicious"):
  right after computing a path, `walk_character` stores `find_route`'s
  result into this exact offset. That explains the modular-1000/10000
  arithmetic seen in `update_stuff` -- `walking` holds a packed
  route/movelist-derived value here, not a plain boolean, so arithmetic on
  it processing that packed value makes sense. The suspicion was
  reasonable given round-2's evidence alone, but turned out unfounded once
  the assignment site was found -- worth remembering that "doesn't fit my
  mental model of the field" is weaker evidence than an actual assignment
  site.
- **`+0x3E = animating`** (high confidence): `if (chin->animating &&
  autoWalkAnims) chin->animating = 0;` matches a test-and-clear-to-0 on
  this offset, gated the same way.
- **`+0x40 = walkspeed`** (high confidence): `walk_character` reads this
  into a global right where source has `int move_speed_x =
  chin->walkspeed;` -- the very next source line after the `animating`
  check above.
- **`+0x42 = animspeed`** (lower confidence, not independently nailed
  down): inferred from positional adjacency to `walkspeed` (matches 2011's
  `short walkspeed, animspeed;` pair) plus a less-certain read site seen
  during unrelated earlier work. Flagged as weaker than the others above.

Still open: the countdown at `+0x1C` (decrement-if-positive, written from
`+0x42`/animspeed under some threshold) seen during round 2 -- not yet
identified.

## CharacterInfo, round 4: ReleaseCharacterView resolves the field_0 mystery

`ReleaseCharacterView` (already matched, `Engine/acchars.cpp:30`) just
delegates to `Character_UnlockView(&game.chars[chat])`
(`Engine/acchars.cpp:1173`), which touches several fields in one place:

```c
void Character_UnlockView(CharacterInfo *chaa) {
  chaa->flags &= ~CHF_FIXVIEW;
  chaa->view = chaa->defview;
  chaa->frame = 0;
  Character_StopMoving(chaa);
  ...
  chaa->animating = 0;
  chaa->idleleft = chaa->idletime;
  ...
}
```

- **`+0x00 = defview`** (high confidence) -- finally resolves the round-1
  contradiction. The pre-existing IDB annotation calling this a `word`
  (`field_0`) was simply **wrong**: `chaa->view = chaa->defview;` matches
  `[+0x08] = [+0x00]` with a full 32-bit read, not a word/short read. The
  old annotation is discarded.
- **`+0x20 = flags`** (high confidence): `chaa->flags &= ~CHF_FIXVIEW;`
  where `CHF_FIXVIEW = 2` (`Common/acroom.h:2480`) matches disasm's
  `and al, 0FDh` (`~2`) on a field read as a full 32-bit `mov`.
- **`+0x3A = frame`** (high confidence): `chaa->frame = 0;` matches
  `[+0x3A] = 0` exactly -- this was previously 2 bytes of unlabeled
  padding right after `loop` (+0x38).
- `idleleft`/`idletime` (+0x2E/+0x2C) independently re-confirmed by this
  function's own `chaa->idleleft = chaa->idletime;` at its end.

Also confirmed the `loop` (+0x38) lead from round 3 more concretely:
`Engine/acchars.cpp:19` declares `int turnlooporder[8] = {0, 6, 1, 7, 3, 5,
2, 4};` right at the top of the file -- the actual named 2011 source array
for the "walking direction -> loop number" 8-entry table that
`update_stuff` was seen writing into this offset.

**Positional inference (flagged as such, not independently verified):**
with `defview`/`view`/`room` landing at exactly `+0x00`/`+0x08`/`+0x0C` --
the same offsets 2011's source has them -- the two gaps in between
(`+0x04`, `+0x10`) were filled in as `talkview` and `prevroom`
respectively, matching 2011's `defview,talkview,view,room,prevroom` field
order exactly. This is the *only* run of fields in the whole struct where
2002 and 2011 offsets appear to coincide; nothing else should be assumed
to line up just because this stretch does.

## CharacterInfo, round 5: SetCharacterView resolves the +0x1C mystery

`SetCharacterView` (already matched) delegates to `Character_LockView`
(`Engine/acchars.cpp:824`), which includes `chap->wait=0;` right where
disasm has `[+0x1C] = 0` (a dword store). This resolves the open lead from
round 2/3 -- the "decrement-if-positive countdown at +0x1C" spotted back
in round 2 (and originally mis-attributed to +0x08, which round 3
corrected to `view`) genuinely belongs to `wait`, just at a different
offset than first guessed. `update_stuff`'s `if (chi->wait>0)
chi->wait--;` lip-sync pattern (the original round-2 evidence) is real --
it was just pointing at the wrong offset until now.

Also reconfirmed in the same read: `view` (+0x08, `chap->view=vii`),
`loop` (+0x38, clamped to 0 if out of range for the new view -- same
per-view frame-count table as before), `frame`/`walking`/`animating`
(+0x3A/+0x3C/+0x3E, all reset to 0 on a view change), and `flags` (+0x20,
`chap->flags|=CHF_FIXVIEW;` -- the set counterpart to
`ReleaseCharacterView`'s clear).

## CharacterInfo tally after 5 rounds

18 of 320 bytes now named (roughly 64 bytes' worth of fields once you
count sizes, the rest still opaque padding): `defview`, `talkview`
(tentative), `view`, `room`, `prevroom` (tentative), `x`, `y`, `wait`,
`flags`, `idletime`, `idleleft`, `activeinv`, `loop`, `frame`, `walking`,
`animating`, `walkspeed`, `animspeed` (lower confidence), plus the
pre-existing `inv` (flagged as suspect, not re-verified).

## ccInstance: recovered from scratch, and it paid off a stale debt

Unlike `CharacterInfo` (which started with 3 pre-existing fields to build
on), `ccInstance` had nothing in the IDB at all -- just the known malloc
size (`0x9A8` = 2472 bytes, from `push 9A8h; call malloc` in
`ccCreateInstanceEx`). Read `ccCreateInstanceEx` (already matched) start to
finish, since it sequentially initializes every field of a freshly
allocated instance -- the single richest source of struct evidence found
in this project so far.

| offset | field | confidence | evidence |
|---|---|---|---|
| `+0x00` | `flags` | high | set to 0, or 1 (`INSTF_SHAREDATA`) when joining an existing instance's global data |
| `+0x04` | `globaldata` | high | malloc'd + `memcpy`'d from the source `ccScript`'s own `+0x00` |
| `+0x08` | `globaldatasize` | high | drives the above; copied from `ccScript+0x04` |
| `+0x0C` | `code` | high | same malloc/memcpy pattern, size scaled by `shl reg,2` (`sizeof(long)`) |
| `+0x10` | `codesize` | high | the pre-scaling count; copied from `ccScript+0x0C` |
| `+0x14` | `strings` | high | NOT copied -- pointer taken directly from `ccScript+0x10` (shared, read-only pool) |
| `+0x18` | `stringssize` | high | copied directly from `ccScript+0x14`, same non-owning treatment as `strings` |
| `+0x97C` | `stack` | high | malloc'd using the very next field |
| `+0x980` | `stacksize` | high | value observed at this call site: `0x7D0` = 2000 -- **note:** differs from the `CC_STACK_SIZE=4000` macro in 2011 source (`Common/CSCOMP.H:239`); not resolved whether the constant shrank or the units differ |
| `+0x99C` | `pc` | high | reset to 0 right after instance setup |
| `+0x9A4` | `instanceof_` | high | set to the source `ccScript*`; this is what let `ccFreeInstance` (below) be found |

Two things deliberately left as padding rather than asserted: a ~2400-byte
gap between `stringssize` and `stack` (almost certainly the call-stack
bookkeeping arrays, 2011 has three parallel `MAX_CALL_STACK=100` arrays
there, but not verified field-by-field or confirmed that constant is still
100), and a 24-byte zeroed region right after `stack`/`stacksize` setup
that's tentatively `registers[]` by position/purpose but doesn't cleanly
fit 2011's 8-register count (24 bytes only fits 6 longs) -- flagged, not
claimed.

**Bonus: this resolved a previously-abandoned open lead.** Seeing exactly
how `ccCreateInstanceEx` sets `instanceof_` and increments the source
script's own refcount (`arg_0[+0x1C4C]++`) made it immediately obvious that
`sub_42B054` -- abandoned several rounds ago as "a real 2002-vs-2011 logic
difference, not just a renamed function" -- does the *exact inverse*, and
is `ccFreeInstance` (`Common/CSRUN.CPP:1042`), matching line for line. The
original abandonment was reasonable given what was known at the time (the
2011 caller-side code looked simplified in a way that didn't fit), but the
real explanation was simpler: the caller *is* simpler in 2011, the callee
just wasn't being read directly. See
`reversing/notes/open-lead-sub_42B054-forked-instance-refcounting.md` for
the full resolution.

**Side discovery, not yet acted on:** the loop reading `arg_0[+0x984]`-ish
offsets for import/export resolution suggests the source `ccScript`
struct's own 2002 layout is *also* much larger than 2011's compact ~80-byte
version (fixed-size arrays instead of dynamic pointers, same pattern as
`GameSetupStructBase`). Not pursued this round -- flagged for whoever
tackles `ccScript` next.

## ccInstance, round 2: ccCallInstance upgrades registers[] from padding to confirmed

Read `ccCallInstance` (already matched) next, since it *invokes* a
function on an instance rather than just constructing one -- different
code paths, different fields touched.

Mostly reconfirmed existing fields (`pc` reset to 0 after execution,
`flags` checked against `INSTF_ABORTED`(2)/`INSTF_FREE`(4) -- both exact
matches to `Common/CSCOMP.H:236-237` -- driving a conditional call to the
newly-matched `ccFreeInstance`), plus more evidence that `ccScript`'s own
layout has drifted (a `numexports` field at ccScript`+0x1C48`, directly
adjacent to the already-known `instances` at `+0x1C4C`, and an
`export_addr[]`-style packed array at ccScript`+0x12E8` whose high byte is
a type tag -- matches 2011's comment "high byte is type; low 24-bits are
offset" exactly, just at very different offsets than 2011's compact
struct).

**The one new find, and it's a good one:** `[Block+0x988]` gets
initialized from `stack`, incremented by `argcount*4` to push call
arguments, and decremented back by the same amount after the interpreter
call returns -- textbook stack-pointer register behavior. `0x988` sits 4
bytes into the region round-1 had flagged as "tentatively `registers[]`,
not confirmed." Since `SREG_SP = 1` in the 2011 source
(`Common/CSCOMP.H:227`), a `registers[]` array starting at `+0x984` with
4-byte elements puts `registers[1]` at exactly `+0x988` -- confirms both
the array's existence and its base offset in one shot. Upgraded from
`char _pad_regs[0x18]` to `int registers[6]` (24 bytes fits 6 longs, not
2011's `CC_NUM_REGISTERS=8`/32 bytes -- plausible that `SREG_OP` (added
for "member func calls", per the 2011 comment) and `SREG_DX` didn't exist
yet in 2002, consistent with member-function script features being a
later language addition). Only `registers[1]`/SP is independently
confirmed; the other five slots are inferred from the array's existence,
not individually verified.

## ccScript: derived almost entirely from ccInstance's own recovery work

`ccScript` kept surfacing as a side-effect of the `ccInstance` rounds
above (via the `instanceof_` pointer), so it became the next target. Most
of its evidence was already in hand from `ccCreateInstanceEx` and
`ccCallInstance`; `fread_script` (already matched) supplied the missing
total size.

| offset | field | confidence | evidence |
|---|---|---|---|
| `+0x00`/`+0x04` | `globaldata`/`globaldatasize` | high | source for `ccInstance`'s own malloc'd/copied globaldata (see ccInstance rounds above) |
| `+0x08`/`+0x0C` | `code`/`codesize` | high | same pattern, source for `ccInstance`'s code array |
| `+0x10`/`+0x14` | `strings`/`stringssize` | high | copied directly (not malloc'd) into `ccInstance`'s own fields |
| `+0x18` (12 bytes) | *(tentative gap)* | low | matches the combined size of 2011's `fixuptypes+fixups+numfixups` if that adjacency holds -- not independently verified, same caution as `CharacterInfo`'s `talkview`/`prevroom` |
| `+0x24` | `imports[600]` | high | `ccCreateInstanceEx` null-checks entries here in a loop bounded by `numimports` |
| `+0x984` | `numimports` | high | confirmed loop bound for `imports[]` |
| `+0x988` | `exports[600]` | high | `ccCallInstance` `strcmp`s function names against entries here, loop bounded by `numexports` |
| `+0x12E8` | `export_addr[600]` | high | `ccCallInstance` reads a packed value here, high byte masked out as a type tag -- matches 2011's own comment on this field ("high byte is type; low 24-bits are offset") verbatim |
| `+0x1C48` | `numexports` | high | confirmed loop bound for `exports[]`/`export_addr[]`, sits directly before `instances` |
| `+0x1C4C` | `instances` | high | incremented by `ccCreateInstanceEx`, decremented/checked by `ccFreeInstance` -- this pairing is what originally led to identifying `ccFreeInstance` (see the `ccInstance` round above) |

**Total size (`0x1C50`) confirmed via `fread_script`'s `push 1C50h; call
malloc`** -- and it lands *exactly* where the derived layout predicts
(`instances` at `+0x1C4C` + 4 bytes = `0x1C50`), with no fields left over
from 2011's `sectionNames`/`sectionOffsets`/`numSections`/
`capacitySections` group. **Update, now confirmed rather than guessed:**
reading `fread_script`'s actual source body (after matching two more of
its helpers, `fget_long` and `freadstring` -- see below) shows the section-
reading code is gated behind `if (fileVer >= 83)`, with `numSections = 0`
otherwise. So it's not that 2002's `ccScript` structurally can't have those
fields -- it's that this build's compiled script format predates version
83 and the loader skips them entirely. Matches the derived size exactly.

**The three 600-entry arrays are the most satisfying part of this
derivation:** `imports[]`'s length is `(numimports_offset - imports_offset)
/ 4 = (0x984-0x24)/4 = 600`. `exports[]`'s length is
`(export_addr_offset - exports_offset)/4 = (0x12E8-0x988)/4 = 600`.
`export_addr[]`'s length is `(numexports_offset - export_addr_offset)/4 =
(0x1C48-0x12E8)/4 = 600`. Three independent offset pairs, each confirmed
separately via a different already-matched function, all agreeing on
exactly 600 -- this isn't a coincidence, it's a real 2002
`MAX_IMPORTS`/`MAX_EXPORTS`-style fixed-capacity limit, later replaced by
2011's dynamically-sized `importsCapacity`/`exportsCapacity` allocations.
Same "fixed array -> dynamic allocation" evolution pattern seen in
`GameSetupStructBase` and (partially) `ccInstance`'s own call-stack region.

## GUIButton: reading the actual vtable out of .rdata

Followed up on `GUIMain::objs[]` (a `void*[60]` array pointing to
`GUIObject`-derived instances, from the earlier `GUIMain` work) by reading
`GUIButton`'s vtable directly out of `.rdata`, starting at address
`0x4AD4A0` -- the same table `GUIButton::Draw` (already matched) lives in.

```
+0x00  unknown_libname_1    (FLIRT-tagged "Microsoft VisualC runtime" -- likely MouseMove, unconfirmed)
+0x04  sub_4240B0            -> likely MouseOver, unconfirmed
+0x08  sub_4240F0            -> likely MouseLeave, unconfirmed
+0x0C  sub_424120            -> CONFIRMED GUIButton::MouseDown
+0x10  sub_40723F            -> CONFIRMED GUIButton::MouseUp
+0x14  unknown_libname_2     (FLIRT-tagged, likely KeyPress, unconfirmed)
+0x18  GUIButton__Draw       -> already matched (confirms the slot numbering)
+0x1C  sub_406A4A            -> likely IsOverControl (possibly GUIObject's shared/base
                                 implementation, not a GUIButton override), unconfirmed
+0x20  sub_406A9C            -> CONFIRMED GUIButton::ReadFromFile
```

This lines up **exactly** with 2011's declared virtual-method order
(`MouseMove, MouseOver, MouseLeave, MouseDown, MouseUp, KeyPress, Draw,
IsOverControl, WriteToFile/ReadFromFile...`) with **zero drift** -- no
inserted/removed slots, no reordering. That's notable given how much else
in this codebase has drifted; virtual method *order* for this class
apparently hasn't changed even though field layout elsewhere clearly has.

Three of the nine slots are confirmed via direct body-reading, and they
cross-validate the slot numbering independently:
- **`+0xC` (MouseDown)** matches `GUIMain::mouse_but_down`'s already-
  confirmed generic `objs[mouseover]->MouseDown()` call at vtable `+0xC`
  from the earlier `GUIMain` round -- two unrelated investigations agreeing.
- **`+0x10` (MouseUp)** same cross-validation against `GUIMain::mouse_but_up`.
- **`+0x18` (Draw)** was already matched independently (string evidence,
  several rounds ago) -- its position here confirms the whole slot count.

`sub_424120`/`GUIButton::MouseDown` and `sub_40723F`/`GUIButton::MouseUp`
match their 2011 bodies line for line, and in the process gave real
`GUIButton` field offsets: `pushedpic@+0x5C`, `usepic@+0x60`,
`ispushed@+0x64`, `pic@+0x54`, `overpic@+0x58`, `isover@+0x68`,
`activated@+0x1C` (a `GUIObject` base field), `flags@+0x4` (also a
`GUIObject` base field, confirmed via the `IsDisabled()`/`IsClickable()`
bitcheck matching `CHF_*`-style constants).

`sub_406A9C`/`GUIButton::ReadFromFile` is the richest single confirmation:
three sequential `fread` calls map cleanly onto three source statements --
28 bytes at `[this+4]` (`GUIObject::ReadFromFile`'s base fields, ending
exactly at `+0x20`), 50 bytes at `[this+0x20]` (`text[50]`, landing exactly
where the base fields end), and 48 bytes at `[this+0x54]` (the 12-int block
`pic..rclickdata`). The trailing `if (textcol==0) textcol=16;` lands on
`[this+0x70]` exactly, matching `textcol` being the 8th int in that block
(`0x54 + 7*4 = 0x70`) -- the whole `+0x54..+0x80` block matches 2011's
declared order with zero drift, same as the vtable.

**Follow-up round: the two `unknown_libname_*` slots resolved, and a
genuine correction.** Read both bodies directly:

- `+0x00` (`unknown_libname_1`) is `GUIButton::MouseMove` -- an empty
  inline body (`{}`) in source, and the disassembly is exactly that: saves
  `this`, does nothing, `retn 8` (pops 2 stack dwords, matching the 2 int
  params). The "Microsoft VisualC runtime" FLIRT tag was a false positive
  -- almost certainly triggered by matching a generic empty-function
  signature, not genuine library code. This exact byte-identical stub is
  also referenced from a second vtable (`off_4AD4E8`), consistent with the
  linker COMDAT-folding identical empty functions across multiple
  `GUIObject`-derived classes into one shared instance.
- `+0x14` (`unknown_libname_2`) is `GUIButton::KeyPress` -- same story,
  empty body, `retn 4` (1 int param), same COMDAT-folding pattern.

**Correction:** `sub_406A4A` at `+0x1C` is **not** `IsOverControl` as
originally guessed from 2011's declared order -- reading its body shows it's
an exact mirror of the already-matched `ReadFromFile`: three `fwrite` calls
at the identical three offsets, matching `GUIButton::WriteToFile` exactly.
That means `IsOverControl` does **not** occupy a vtable slot between `Draw`
(`+0x18`) and `WriteToFile` (`+0x1C`) in this 2002 build, unlike 2011's
order (`...Draw, IsOverControl, WriteToFile, ReadFromFile`). Most likely
`IsOverControl` was added to `GUIObject` sometime after this build --
2011's version has a real body (not pure-virtual), so the class didn't
*need* to override it before it existed. This is exactly the kind of
"looks right positionally, turns out wrong" case worth remembering: the
9-slot vtable's *order* matched 2011 with zero drift for 8 of 9 slots, but
that doesn't mean the slot *count* matches -- always confirm each slot's
actual body rather than trusting positional prediction alone once you're
this deep into it.

The `GUIButton` vtable (slots 0-8, `0x4AD4A0` to `0x4AD4C0`) is now fully
identified. The `GUIObject` base fields between `+0x08` and `+0x1C` (2011
has `guin`/`objn`/`x`/`y`/`wid`/`hit` there) remain unconfirmed at the
individual-field level -- only the endpoints (`flags@+0x04`,
`activated@+0x1C`) are pinned down.

## Takeaway

Given 4 of the 5 structs checked so far turned out to have drifted
(sometimes drastically), **do not generate IDA struct definitions directly
from the 2011 source without independently verifying size first** (an
existing IDB struct size, or a `malloc`/`push <size>` allocation site in
the disassembly). Where a mismatch is found, the correct (but slower) path
is incremental field-offset recovery from actual access patterns in the
disassembly (`playerchar->x`-style accesses, one field at a time), not
guessing at what changed between versions.

This also means struct-layout work is not the efficient use of time it
looked like at first glance -- most of the effort per struct is in this
verification step, and for genuinely drifted structs, full recovery is
roughly as expensive as the incremental-offset-recovery option that was
available from the start.
