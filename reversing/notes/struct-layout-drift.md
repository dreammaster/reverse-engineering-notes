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
  modular/dynamic. **Field-level recovery started -- see its own section
  below.**
- **`ExecutingScript`**: confirmed total size `0x6C` = 108 bytes (via a
  `rep movsd` bulk-copy of exactly 27 dwords in `post_script_cleanup`,
  independently matching the `imul` stride used throughout the same
  function). The 2011 struct (`Common/acruntim.h:700`) is ~725 bytes,
  dominated by the `postScriptActions[MAX_QUEUED_ACTIONS=5]` queue
  system (`postScriptSaveSlotDescription[5][100]` alone is 500 bytes).
  That whole subsystem is almost certainly a later addition -- 2002's
  struct is barely bigger than just `inst` + a small `run_another`-style
  block. **Field-level recovery started (`inst`/`forked` confirmed,
  first/last fields) -- see its own section below.**

## SpriteCache: field-level recovery was already sitting in the live IDB, just never formalized

The "verified DRIFTED" entry above (size `0x10`, presumed "a couple of
raw arrays") turned out to be an incomplete picture -- checking the live
IDB directly (rather than just its recorded size) found the struct was
**already fully field-recovered**, with a proper `SpriteCache struc`
definition and 4 named fields, predating even this project's
`apply_structs.py`/`matches.json` tracking (part of the original
pre-existing manual work from before the AI-assisted phase, like the
535 pre-existing function names). It had just never been pulled into
`apply_structs.py` for re-runnability, so a `.asm` re-export from a fresh
IDB would have silently lost it.

Confirmed fields, exactly filling the known `0x10` (16-byte) size:
`offsets` (`long*`, `+0x00`), `elements` (`long`, `+0x04`), `images`
(`void**`, `+0x08`), `ff` (`FILE*`, `+0x0C`) -- all four independently
confirmed via `SpriteCache::initFile` (`Common/sprcache.cpp:631-643`,
already matched): `"for (vv=0; vv<elements; vv++) { images[vv]=NULL;
offsets[vv]=0; } ff=clibfopen(filnam,\"rb\");"` matches the disassembly's
loop-bound/array-write/fopen-assignment pattern exactly, field for field.

This **confirms** (not just "presumably") the earlier guess: 2002's
`SpriteCache` really is just two raw parallel arrays plus a count and a
file handle -- none of 2011's LRU-eviction bookkeeping (`mrulist`,
`mrubacklink`, `liststart`, `listend`, `lastLoad`, `maxCacheSize`,
`lockedSize`), per-sprite metadata (`sizes`, `flags`,
`spritesAreCompressed`), or cache-size accounting (`cachesize`) exists in
this build at all -- the whole discardable-cache/eviction subsystem is a
later addition. Since the struct is fully accounted for byte-for-byte
(4 fields x 4 bytes = 16 bytes exactly, matching the known size with zero
slack), no further field-level work is expected here -- this one is
**done**, not just "safer than assumed."

**Lesson for future struct work**: when a struct is flagged "drifted, not
recovered" in old notes, double-check the live IDB itself before
re-deriving from scratch -- earlier manual work (especially the original
pre-project-tracking annotations) may already have solved it, just
without a matching entry in `apply_structs.py`/`matches.json` to make
that visible from the repo alone.

## GameSetupStructBase: first 4 fields recovered (out of 30+), 1330 of 49028 bytes

Started the real field-level recovery of `GameSetupStructBase` -- much
bigger in scope than any struct tackled so far (known size `0xBF84` =
49028 bytes vs the largest previous target, `ccScript` at `0x1C50` =
7248 bytes). The global instance's base address is confirmed directly:
it's literally named `game_gamename` in the IDB (via
`load_ac2game_dta`'s `fread(&game_gamename, sizeof(GameSetupStructBase),
1, file)` -- a single bulk read of the whole struct at once, typical for
a little-endian platform, which is why 2011's own `ReadFromFile` method
is `#ifdef ALLEGRO_BIG_ENDIAN`-only and doesn't apply here at all).

**Confirmed fields** (all high confidence):
- `gamename[30]` @ `+0x00` -- the struct's own base address. DRIFT: 2011
  has `char gamename[50]` (`acroom.h:2817`); only 30 bytes here.
- `options[20]` @ `+0x1E` (right after gamename) -- confirmed via
  `GiveScore`'s (already-matched) sound-effect check: `if (amnt>0 &&
  options[OPT_SCORESOUND]>0) PlaySound(options[OPT_SCORESOUND]);`, where
  the accessed byte (`byte_513337`) sits exactly 1 byte after gamename's
  end, matching `OPT_SCORESOUND`'s value of 1 (`acroom.h:2707`) exactly.
  DRASTIC DRIFT: 2011 has `int32 options[100]` (400 bytes); this build
  uses single BYTES (not 4-byte ints) and has room for only 20 entries --
  both a type change and a ~20x size cut. The highest option constant
  that fits in 20 slots is index 19 (`OPT_FADETYPE`), consistent with
  this build predating later UI/rendering options like
  `OPT_DIALOGNUMBERED`(22), `OPT_MOUSEWHEEL`(21), `OPT_ANTIALIASFONTS`(25).
- `paluses[256]` @ `+0x32` -- confirmed via a shared loop in `main`
  (already matched): `for (ee=0; ee<256; ee++) if (paluses[ee]!=2)
  palette[ee]=defpal[ee];` -- the disassembly's `cmp [ee],100h` loop
  bound and `cmp edx,2` (`PAL_BACKGROUND`) check match this exactly.
  Matches 2011's `unsigned char paluses[256]` (`acroom.h:2819`) with
  **zero drift** -- same size, same position (right after options).
## RETRACTED: `defpal[256]` was wrongly asserted, corrected same session

Originally also claimed `defpal[256]` @ `+0x132` (as packed 4-byte
values) with "zero drift" high confidence, reasoning that the SAME loop
which confirmed `paluses` (`for (ee=0; ee<256; ee++) if (paluses[ee]!=2)
palette[ee]=defpal[ee];`) also confirmed `defpal` at the same 256-element
extent, since both are indexed by the same `ee`.

**This was wrong, and caught by mechanically re-counting the raw bytes**
rather than trusting the loop bound alone. Writing a small script to sum
byte counts through the actual `.data` declarations (handling `dup()`
counts and `align` directives properly, rather than eyeballing hundreds
of lines) found that only ~51 dwords (204 bytes) of confirmed space
exist after `dword_51344A` (the field's start) before running into
`g_interface` -- a totally unrelated global (AGS's legacy
script-exported `"interface"` object, registered via
`scAdd_External_Symbol`, just placed memory-adjacent by the linker, not
a struct field at all). 206 bytes isn't even a clean multiple of 4, so
it can't be pinned to a specific element count either.

**The lesson: a matching loop bound in source is necessary but not
sufficient evidence for a field's size.** The loop's `ee<256` condition
proves the *code* is willing to read up to 256 elements, not that the
*struct* actually allocates 256 elements worth of space. Most likely
explanation here: 2002's `defpal[]` is genuinely much smaller than 256
(consistent with the fixed-capacity-shrinkage pattern seen everywhere
else in this project), and the loop's out-of-bounds reads for higher
`ee` simply never trigger in practice, because this game's actual
`paluses[]` data never sets a non-background flag past whatever
`defpal`'s true (smaller) capacity is -- a latent, never-triggered
out-of-bounds read in the original 2002 code, not a bug that needs
fixing here.

`defpal` has been **removed from the struct declaration** in
`apply_structs.py` pending a size that can be independently confirmed
(e.g. an allocation-size constant, or a second, more tightly-bounded
access site) -- position alone (`+0x132`, right after `paluses`) is not
sufficient to assert a size. Do not re-add it on loop-bound evidence
alone.

**Broader methodology finding, worth remembering for the rest of this
struct**: this `.data` region already had many individual byte/dword
labels from IDA's own heuristics or earlier pre-project work (e.g.
`byte_513337`, `dword_51344A`), each with real `DATA XREF`s -- but their
**declared sizes are not reliable field-boundary evidence by
themselves either**. IDA only carves out a label as far as it sees a
*direct* reference land; a label showing, say, 51 dwords does not prove
the true field is only 51 dwords long -- it may continue into what IDA
separately labeled as a "different" symbol right next to it (untouched
by any single instruction). Combined with the `defpal` retraction above,
the full picture is: **neither a pre-existing label's declared extent,
nor a loop bound alone, is sufficient evidence for a field's true size
-- verify with both, or with an independent allocation-size constant.**

**Scope reality check**: confirmed fields so far span only `0x132` (306
bytes) of the known `0xBF84` (49028) total -- under 1%. The
`apply_structs.py` declaration pads the remainder with an explicit
`_pad_unrecovered[0xBE52]` trailer to preserve the struct's known-correct
total size (critical: unlike dynamically-allocated structs elsewhere in
this project, this one has a fixed global instance whose size was
already confirmed before this recovery began -- omitting the padding
would shrink the applied type and could corrupt downstream type info).
Remaining unrecovered: `defpal` (size TBD), `numviews`, `numcharacters`,
`playercharacter`, `totalscore`, `numinvitems`,
`numdialog`/`numdlgmessage`, `numfonts`, `color_depth`, `target_win`,
`dialog_bullet`, `hotdot`/`hotdotouter`, `uniqueid`, `numgui`,
`numcursors`, `default_resolution`, `default_lipsync_frame`,
`invhotdotsprite`, `messages[]`, `dict`, `globalscript`, `chars`,
`compiled_script` (all base-class fields, per `acroom.h:2821-2843`) --
plus whatever the derived `GameSetupStruct` embeds directly as
fixed-size arrays (`fontflags`, `fontoutline`, `spriteflags`, `invinfo`,
`mcurs`, etc., `acroom.h:2890-2917`), which is almost certainly where
most of the remaining ~48.7KB actually lives, going by the general
"2002 = fixed arrays inline, 2011 = pointers to dynamic allocation"
pattern already established for every other struct in this project.

## Three more fields confirmed: chars, playercharacter, numinvitems -- and the field ORDER itself is proven to differ from 2011

Picked up the already-labeled globals flagged above as next-round leads,
using the same offset-counting script (relative to `game_gamename`'s
base address) to get precise positions before trusting anything:

- **`chars`** (`CharacterInfo *`) @ `+0x263C` -- confirmed via
  `SetPlayerCharacter` (already matched): `game_chars + newchar*0x140`,
  where the `0x140` stride is an exact match for this project's own
  independently-confirmed `CharacterInfo` struct size. Further confirmed
  via `scAdd_External_Symbol("character", game_chars)`, registering the
  script-exposed `character[]` array pointer -- matches source's
  `chars` field identity precisely.
- **`playercharacter`** (`int`) @ `+0x7CFC` -- confirmed via
  `GetPlayerCharacter` (already matched, script-exported): the entire
  function body is just `return game_playercharacter;`. Also written by
  `SetPlayerCharacter`. Zero type drift from 2011's `int32
  playercharacter`.
- **`numinvitems`** (`short`) @ `+0x8538` -- confirmed via its `movsx`
  (sign-extend from a 2-byte word) use as the inventory-index upper
  bound across several already-matched functions (`GetInvName`,
  `update_invorder`, `SetInvItemPic`, and others). Zero type drift from
  2011's `short numinvitems` -- both are the same signed 16-bit field.

*(Offsets in this section were corrected +8 bytes from their originally
recorded values -- see the dedicated "OFFSET CORRECTION" section further
down for why; the evidence/reasoning below was always sound, only the
absolute positions computed by an early offset-counting script were
off.)*

**Important finding: the field ORDER itself differs from 2011, not just
sizes/types.** 2011 declares `chars` near the very end of the base
struct (after `messages[]`, `dict`, `globalscript` -- acroom.h:2842),
but here it sits at only `+0x263C`, far *before* `playercharacter`
(`+0x7CFC`), which 2011 declares near the very *start* (acroom.h:2823).
This rules out the working assumption used for `gamename`/`options`/
`paluses` (that confirmed fields land in the same relative order as
2011, just with different sizes) -- from this point on, each field is
being anchored independently via its own evidence, not assumed adjacent
to whatever 2011 declares nearby. This also means the huge gaps between
confirmed points (`+0x132` to `+0x263C`, 9482 bytes; `+0x2640` to
`+0x7CFC`, 22204 bytes; `+0x7D00` to `+0x8538`, 2104 bytes) cannot be
assumed to contain the 2011 fields "in between" `chars` and
`playercharacter` in source order -- they could contain almost anything,
including large chunks of the derived `GameSetupStruct`'s fixed-size
arrays (`fontflags`, `spriteflags`, `invinfo`, `mcurs`, etc.), consistent
with this build likely flattening base+derived into one struct rather
than the clean 2011 split.

**Scope update**: confirmed anchor points now span `+0x00` to `+0x853A`
(34106 bytes, ~70% of the struct's total *extent*) -- but this is
extent, not recovered content: roughly 33836 of those bytes remain
completely opaque padding between the anchors, only the anchors
themselves (gamename/options/paluses/numcharacters/chars/
playercharacter/numinvitems, a handful of scalar fields and one pointer)
are actually known.

`game_paused` and `game_file_name` (the two remaining leads flagged
above) turned out to be **false leads, not struct fields at all** --
both are genuinely standalone global variables in AGS, matching 2011
source exactly: `Engine/AC.CPP:4989` has bare `game_paused++;` (a plain
global int, not `game.paused`), and `AC.CPP:716` declares `char*
game_file_name=NULL;` directly. The `game_` prefix on their IDB names
was coincidental/heuristic, not evidence of struct membership -- a
useful caution for reading any future `game_*`-prefixed label the same
way: verify against real 2011 source usage before assuming it's a
`GameSetupStructBase` field.

**One more field found via a different technique** (source usage ->
candidate function -> disassembly verification, rather than scanning
pre-existing labels): `numcharacters` (`int`) @ `+0x2638` (corrected
+8, see the OFFSET CORRECTION section further down), confirmed via
`is_valid_character` (`Engine/AC.CPP:3820`, newly matched this round --
"if ((newchar<0) || (newchar>=game.numcharacters)) return 0;"), itself
found by checking `SetCharacterBaseline` (already matched)'s callee
against source's `if (!is_valid_character(obn)) quit(...)`. Notable:
`numcharacters` sits **directly adjacent to `chars`** (ends at exactly
`+0x263C` with zero gap), even though 2011 declares them far apart
(`acroom.h:2822` vs `:2842`) -- this build apparently pairs the
character count immediately before the character array pointer, a
natural "count-then-pointer" idiom that 2011's later refactor separated
out. This is now the *second* piece of evidence for 2002's field order
differing from 2011's (the first being `chars` vs `playercharacter`'s
relative position, noted above).

With the "scan pre-existing labels" technique now exhausted (a targeted
scan of the three remaining gap ranges for any other non-generic labels
turned up nothing new), further progress on this struct needs the
slower source-usage-first cycle demonstrated by `numcharacters` above:
pick a 2011 field, find its usage in `Engine/`, locate an already-matched
caller, and trace the callee. Good next candidates by the same method:
`totalscore`, `numviews`, `numdialog`, `numfonts`, `color_depth`,
`uniqueid`, `numgui`, `numcursors` -- all heavily used as loop bounds or
validation checks throughout the engine, similar to how `numinvitems`
and `numcharacters` were found.

## numviews found -- and it was hiding behind a pre-existing generic label

Chased a whole cluster of tiny one-line 2011 getter functions
(`Game_GetFontCount`, `Game_GetMouseCursorCount`, `Game_GetGUICount`,
`Game_GetViewCount`, etc. -- all in `Engine/AC.CPP:16700-16723`,
literally back-to-back `return game.numXXX;` one-liners) hoping several
would already be matched in the disassembly. None were (these particular
script-property getters may not exist yet in this build, or just aren't
named). Fell back to `numviews`'s other usage sites and found
`SetObjectFrame` (already matched) checks it directly: `"if (viw>=
game.numviews) quit(\"!SetObjectFrame: invalid view number used\");"`
(`AC.CPP:14653`) -- the disassembly does `viw--; cmp viw,ElementCount;
jl <ok>`, an exact match.

**`ElementCount` turned out to BE `numviews`.** This is one of the two
labels the earlier gap-scan already found and dismissed as generic
noise (alongside `g_interface`, which really was unrelated) -- worth
remembering: a generic-looking pre-existing label (`ElementCount`,
`dword_51344A`, etc.) isn't automatically noise just because its *name*
looks auto-generated. Some of them are real fields that IDA simply
never got a chance to name meaningfully, because the ONE place it's used
happens to reuse a common parameter-name pattern (`ElementCount` here is
also independently used, unrelated, as a genuine `fread`/`fwrite`
parameter name elsewhere in the binary -- a coincidental name collision,
not evidence either way about this specific occurrence). Position:
`+0x2540`, sitting inside what had been treated as one large unknown gap
-- now split into two smaller gaps on either side of it.

**A promising but NOT YET confirmed lead surfaced alongside this**:
`dword_515854`, sitting just 4 bytes before `numcharacters` (at
`+0x253C`-ish), is used as a loop bound over `0x334` (820)-byte structs
inside `load_ac2game_dta` (already matched) -- plausibly `numdialog`
(`DialogTopic` entries are the right shape/size for this), but not
independently verified against source this round. Flagged in
`apply_structs.py` as `_pad_unknown1b`'s neighbor rather than asserted.

## numdialog found -- and the dword_515854 lead above was a dead end, not numdialog

Followed up on `numdialog` directly rather than the `dword_515854` lead.
Checking 2011's `DialogTopic` struct size first: `optionnames[30][150]`
alone is 4500 bytes, plus `optionflags`/`entrypoints`/scalars, totaling
~4696 bytes per topic -- nowhere near the `0x334` (820)-byte stride
`dword_515854` was bounding. Reading that loop's actual body more
closely confirmed the mismatch: it indexes `byte_513B7C`/`byte_513B7D`
(a *different*, far-away region around `+0x864`, unrelated to
`GameSetupStructBase`'s scalar-field area) -- almost certainly more of
the same "legacy interface" system that `g_interface` turned out to
belong to, not dialog data at all. The `dword_515854` lead is dropped
*as a `numdialog` candidate* -- correctly so, but a much later round
("numiface found") revisits this exact global and identifies what it
actually IS (`numiface`), once `OriGameSetupStruct`'s existence made
that a field worth checking for at all. See that section for the
resolution; don't treat this paragraph as the final word on
`dword_515854`.

The real `numdialog` was found the same way `numcharacters` was: `int32
numdialog` is checked in `RunDialog(int tum)` (already matched,
`AC.CPP:16065-16067`) with the identical bitwise-OR idiom seen before --
`"if ((tum<0) | (tum>=game.numdialog)) quit(\"!RunDialog: invalid topic
number specified\");"` matches disasm's `cmp arg_0,0; setl al` / `cmp
ecx,dword_51D2E4; setnl dl` / `or eax,edx` exactly. Position: `+0x9FCC`
(corrected +8, see the OFFSET CORRECTION section further down), well
beyond `numinvitems` (`+0x8538`), splitting what had been one large
trailing "unrecovered" pad into two.

**Process note, worth remembering going forward**: this evidence was
initially added as a brand-new `matches.json` entry for `RunDialog`,
which was a mistake -- `RunDialog` already had an original match entry
from when it was first identified. `apply_matches.py` replaces a
function's *entire* comment block on each run (by design, so re-runs
stay idempotent), so a second entry for the same `asm_name` later in the
file would silently **overwrite** the original match's evidence instead
of adding to it. Caught and fixed by merging the new field-discovery
evidence into the *existing* entry instead of leaving a duplicate.
**When a struct-field discovery hangs off an already-matched function,
merge into its existing `matches.json` entry -- never append a second
entry for the same `asm_name`.**

## numfonts found -- adjacent to numdialog, matching 2011's declared order for once

Followed `numfonts` through `SetSpeechFont`/`SetNormalFont` (both already
matched, `AC.CPP:13463-13472`), two near-identical sibling functions with
the exact same check: `"if ((fontnum<0) || (fontnum>=game.numfonts))
quit(...)"`, matching disasm's `cmp fontnum,0; jl <quit>; cmp
eax,dword_51D2EC; jl <ok>` in both. `dword_51D2EC` @ `+0x9FD4` -- exactly
8 bytes after `numdialog`'s start (`+0x9FCC`), i.e. immediately adjacent
modulo a 4-byte gap. That gap is suspiciously exactly one `int` wide, and
sits precisely where 2011's comma-declared `"int32 numdialog,
numdlgmessage;"` (`acroom.h:2826`) would put `numdlgmessage` -- a
plausible lead, flagged in `apply_structs.py` but NOT asserted without
its own independent access-site confirmation.

Notable: this is the **first** case since `chars`/`playercharacter`
where a confirmed field sits adjacent to another confirmed field in a
way that *does* match 2011's declared relative order (`numdialog` then
`numfonts`, matching `acroom.h:2826-2827`'s sequence) -- the field-order
divergence found earlier isn't total scrambling, just some fields (like
`chars`) got moved around while others (like this `numdialog`/
`numdlgmessage`/`numfonts` run) stayed put.

## numgui found; numcursors turns out to be a genuine 2002-vs-2011 behavioral gap, not just layout

`numgui` confirmed cleanly via `InterfaceOn`/`InterfaceOff` (both already
matched, `AC.CPP:18596-18619`), the same paired-sibling-function pattern
that found `numfonts`: identical bitwise-OR checks `"if ((ifn<0) |
(ifn>=game.numgui)) quit(...)"`, both against `dword_51DB04`. Position
`+0xA7EC` (corrected +8, see the OFFSET CORRECTION section further
down), zero type drift from 2011's `int32 numgui`.

`numcursors` (2011's very next declared field) was chased the same way
and hit a real wall, not just a missing-lead problem: **every single
cursor-related check in this build is hardcoded to the literal `10`,
never a runtime field lookup.** Checked `ChangeCursorGraphic`,
`ChangeCursorHotspot` (both `cmp curs,0Ah`), and a cursor-coordinate-
scaling loop in `main` (`cmp [ee],0Ah`) -- all three, independently,
compare against the same fixed `10`, not any global. Also checked
`load_ac2game_dta` directly for a per-file cursor-count `fread` (which
would have to read a real count from disk regardless of what runtime
code does) and found none -- no `ElementSize=0x18` (`sizeof(MouseCursor)`
in this build, confirmed via `ChangeCursorGraphic`'s `imul eax,18h`
stride) `fread` call anywhere in that function. This strongly suggests
2002 simply doesn't have a runtime-configurable cursor count at all --
`MAX_CURSOR` (2011: `acroom.h:2699`, value 20) may have been a
fixed-and-unchecked constant (10, not 20 -- a genuine value drift too)
in this build, with no corresponding struct field actually read or
validated anywhere. Not chasing this further; flagged in
`apply_structs.py` as "possibly absent as a runtime field" rather than
left as an open lead to revisit, since there may be nothing to find.

## Best round yet: 6 fields in one pass, via a single exact 16-byte arithmetic fit

Went looking for `uniqueid` next and found it via `init_translation`
(already matched): the translation-file compatibility check `"if
((uidfrom != game.uniqueid) || (strcmp(wasgamename, game.gamename) !=
0)) {...quit...}"` matches disasm's `cmp eax,dword_51D300; jnz <fail>;
push offset game_gamename; ...strcmp...` exactly -- and as a bonus,
independently re-confirms `gamename`'s own identity via the same
instruction sequence. Position: `+0x9FE8` (corrected +8, see the
OFFSET CORRECTION section further down).

That position turned out to be the real prize: it sits **exactly 16
bytes** after `numfonts` ends (`+0x9FD8`). 16 bytes is *precisely*
`color_depth`(4) + `target_win`(4) + `dialog_bullet`(4) + `hotdot`(2) +
`hotdotouter`(2), in 2011's exact declared order (`acroom.h:2828-2832`)
-- zero drift, zero slack. Rather than just assert the fit, checked each
predicted address individually against real usage:

- **`color_depth`** @ `+0x9FD8`: `FadeOut` (already matched) gates a
  hi-color-only fade path with `cmp dword_51D2F0,1; jle <skip>`,
  matching `"if (game.color_depth > 1) {...}"` exactly.
- **`target_win`** @ `+0x9FDC`: has **zero references anywhere** in the
  disassembly -- plausible for a build/editor-time-only flag the runtime
  engine never checks. Included at medium confidence (positional only,
  boxed in with zero slack by the four confirmed neighbors) since there
  was nothing to independently verify against.
- **`dialog_bullet`** @ `+0x9FE0`: `sub_41D7F7` (called from
  `do_conversation`, already matched, not itself renamed) does `cmp
  dword_51D2F8,0; jle <skip>; ...spriteset[dword_51D2F8]...`, matching
  2011's documented semantics exactly ("0 for none, otherwise slot num
  of bullet point", `acroom.h:2830`) -- used directly as a
  `SpriteCache::operator[]` index.
- **`hotdot`/`hotdotouter`** @ `+0x9FE4`/`+0x9FE6`: `SetMouseCursor`
  (already named, but had no `matches.json` entry at all -- another
  original pre-project name, now retroactively documented) gates an
  inventory-cursor hotspot-dot marker specifically for `MODE_USE`(4),
  then an immediately-following "outer ring" color lookup right after --
  matching 2011's "hotdot, hotdotouter; // inv cursor hotspot dot"
  (`acroom.h:2831`) exactly, in the exact same relative order.

Every one of the four independently-checkable fields confirmed cleanly
at its predicted address -- a strong validation of trusting the
arithmetic fit for `target_win` too, even without its own access site.
6 fields confirmed in a single round, the best yield so far.

## Diminishing returns: totalscore, default_resolution, default_lipsync_frame all hard to isolate; invhotdotsprite likely absent

Tried the remaining candidates and hit real difficulty with each, for
different reasons:

- **`totalscore`**: every usage is either deep inside a huge already-
  inlined `main`/`init_game_settings` block (hard to pinpoint the exact
  instruction without much more tracing) or behind an indirect macro-
  constant lookup (`AC.CPP:7133`'s `"%totalscore%"` handler resolves to
  `MAXSCORE`, not a direct field read). No clean isolated site found.
- **`default_resolution`**: every usage site is embedded in large,
  complex graphics/room-loading functions (`AC.CPP:4089-4135`,
  `27742-27790`) with no small standalone validator function like the
  `is_valid_character`/`RunDialog`/`InterfaceOn` pattern that worked so
  well for the scalar count fields.
- **`default_lipsync_frame`**: its cleanest candidate function,
  `GetLipSyncFrame` (`AC.CPP:6096`), isn't matched in the disassembly
  yet, and its other usage site is deep inside a large lip-sync-timing
  function.
- **`invhotdotsprite`** looks like it may not exist in this build at
  all: `SetMouseCursor`'s already-confirmed body (see the `hotdot`/
  `hotdotouter` writeup above) only implements 2011's plain pixel-dot
  branch (`putpixel_compensate`-style), with no trace of the
  sprite-based `invhotdotsprite>0` branch or `draw_sprite_support_alpha`
  call that source has alongside it (`AC.CPP:5079-5097`) -- consistent
  with this being a later addition, same pattern as `numcursors`.

## GetLipSyncFrame chased directly -- conclusively appears absent from this build too

Tried to match `GetLipSyncFrame` as a standalone task (it would have
unlocked `default_lipsync_frame` the usual way, via
`is_valid_character`-style callgraph tracing). Its two most distinctive
calls are `strnicmp` (dynamic-length case-insensitive compare inside a
nested loop, `AC.CPP:6110`) and `strchr(tptr,'/')` (`AC.CPP:6107-6108`,
called twice per outer iteration). Checked **every** call site of both
in the entire disassembly:

- All 5 `_strnicmp` calls: one belongs to `sub_417088` (interface-click
  text-parser dispatch, already known from earlier `#unhandled_event`
  investigation) comparing against fixed strings ("hotspot"/"object"/
  "character"/"inventory"); the rest are unrelated fixed-string
  comparisons in the same cluster.
- All 6 `strchr` calls: one pair belongs to `FileOpen` (already matched,
  checking for `/`/`\`/`:` path-separator characters for file-access
  validation); the other pair belongs to `csetlib` (already matched,
  library-path resolution, also checking `\`/`/`). Neither is
  GetLipSyncFrame's `'/'`-as-phoneme-delimiter search.

**Zero candidates match.** Combined with the `invhotdotsprite` finding
above (the sibling `MODE_USE` sprite-cursor feature also missing from
`SetMouseCursor`), this is now fairly solid evidence that the *entire*
automatic-lip-sync-from-phoneme-text system (`MAXLIPSYNCFRAMES`,
`lipSyncFrameLetters[]`, `GetLipSyncFrame`, `update_lip_sync`) is a
later AGS addition with no counterpart in this 2002 build at all --
not a search failure, a genuine absence. `default_lipsync_frame` is
being retired as an open lead for the same reason `numcursors` and
`invhotdotsprite` were: there's very likely nothing here to find.

**Remaining open leads for a future round**, if `GameSetupStructBase`
work resumes: `totalscore` and `default_resolution` need a slower
full-body trace through `main`/large graphics functions rather than the
fast "small standalone validator" pattern that carried most of this
struct's recovery so far -- no shortcut found for either.

## MAJOR FINDING: this struct isn't a drifted GameSetupStructBase -- it's OriGameSetupStruct, AGS's own oldest ancestor struct, still sitting in the 2011 header

Went back for `totalscore` with a different technique: instead of
chasing its two `Engine/AC.CPP` usage sites directly (both previously
dead ends -- one behind a macro-constant lookup, one buried in a huge
inlined initializer), searched all of `Common/` for the bare string
`totalscore` and found something unexpected: `Common/acroom.h:3017`,
`void ConvertOldGameStruct(OldGameSetupStruct *ogss, GameSetupStruct
*gss)` -- a compatibility shim that converts AGS's ORIGINAL (oldest)
game-struct layout into the modern one, field by field. This function's
sequence, `gss->totalscore = ogss->totalscore; gss->numinvitems =
ogss->numinvitems; gss->numdialog = ogss->numdialog; ...`, reads almost
exactly like this project's own confirmed-field list, in the same
relative order.

Followed the inheritance chain up: `OldGameSetupStruct : public
OriGameSetupStruct2`, and `OriGameSetupStruct2 : public
OriGameSetupStruct` (`acroom.h:2769`, `2802`, `2810`) -- three
generations of the SAME struct, kept in the 2011 header purely so old
save/game files can still be loaded and upgraded. `OriGameSetupStruct`
is the very first, original layout.

**Checking `OriGameSetupStruct`'s declared fields against every anchor
already confirmed in this struct was a near-perfect match**, explaining
away nearly all of the "drastic drift" found in earlier rounds as one
single fact (this build predates the ENTIRE struct-evolution chain, not
scattered independent divergence) rather than dozens of unrelated
coincidences:

- `gamename[30]` -- OriGameSetupStruct declares exactly 30, matching
  this build with ZERO drift (2011's `GameSetupStructBase` widened it
  to 50).
- `options[20]` (single bytes) -- OriGameSetupStruct declares `char
  options[20]` exactly, matching with ZERO drift (2011 widened to
  `int32 options[100]`).
- `paluses[256]` -- matches with ZERO drift in both versions (unchanged
  across the whole evolution).
- `numcharacters` immediately followed by `chars` (zero gap) --
  OriGameSetupStruct declares `int numcharacters; OldCharacterInfo
  *chars;` back to back (`acroom.h:2779-2780`) -- EXACT adjacency
  match.
- `chars` sitting far before `playercharacter` -- OriGameSetupStruct's
  order is `..., chars, __charcond[50], __invcond[100],
  compiled_script, playercharacter, ...` -- `chars` genuinely does
  precede `playercharacter` by a huge margin here, unlike
  `GameSetupStructBase`'s later reordering. This is the exact
  divergence flagged as unexplained two rounds ago -- now explained.

**Independently verified with a mechanical byte-counting script** --
rather than trusting the field-order hypothesis alone (same discipline
as the `defpal` retraction). Wrote a parser
(`reversing/scripts/count_data_offsets.py`) that walks the raw `.data`
declarations (`db`/`dw`/`dd`, `dup()` counts, `align` directives) from
`game_gamename` forward, summing byte sizes to compute every label's
exact offset -- parsed **zero unhandled lines** across the ~34KB,
~60000-line span from `game_gamename` through past `numgui`. The
computed offsets landed on all 8 previously-confirmed anchors
(`numviews`/`ElementCount`@+0x2540, `numcharacters`, `chars`/
`game_chars`, `playercharacter`/`game_playercharacter`, `numinvitems`/
`game_numinvitems`, `numdialog`, `numfonts`, `numgui`) -- but this
FIRST version of the script had a real bug that wasn't caught until a
later cross-check (see "OFFSET CORRECTION" below): every one of those
apparent "exact matches" past `numviews` was actually off by +8 bytes
in a way that happened to reproduce whatever value had already been
recorded in this file from an EARLIER session's own (equally buggy)
offset-counting pass -- two independently-written scripts agreeing is
NOT the same as either being correct. The numbers below are the
CORRECTED ones (see the dedicated section further down for the full
story and how it was caught) -- do not trust a struct-offset script's
output just because it reproduces a previously-recorded number.

### Five new fields confirmed this round, using the parser plus real access-site evidence

- **`compiled_script`** (`ccScript *`) @ `+0x7CF8` -- `dword_51B010`.
  The 22200-byte gap between `chars` and this field's start turned out
  to be a PERFECT zero-slack fit for `EventBlock __charcond[50]` +
  `EventBlock __invcond[100]` (OriGameSetupStruct's declared order,
  `acroom.h:2781-2782`): independently computing `EventBlock`'s own
  size from its declaration (`int list/respond/respondval/data[
  MAXCOMMANDS=8]` + `int numcmd` + `short score[8]` = 148 bytes exactly)
  gives `148*150 = 22200`, matching the gap to the byte -- but this is
  arithmetic fit only, NOT independently confirmed field-by-field, so
  `__charcond`/`__invcond` themselves are documented as a hypothesis in
  `apply_structs.py`'s comments, not asserted as typed fields.
  `compiled_script` itself IS independently confirmed two ways: (1)
  `load_ac2game_dta` (already matched) does `cmp dword_51B010,0; jz
  skip; call fread_script(stream)` -- `fread_script`
  (`Common/CSRUN.CPP:2029`, already matched) deserializes a compiled
  `ccScript` blob, gated by this field as a presence flag; (2)
  `restore_game_data` (already matched) explicitly SAVES this field to
  a local before re-`fread`-ing the whole struct from a save file, then
  RESTORES it afterward -- a "preserve compiled game assets across a
  savegame restore" idiom, since a save file carries game *state*, not
  a fresh copy of the compiled script.
- **`globalscript`** (`char *`) @ `+0x2634` -- `dword_51594C`. Found
  via the SAME save/restore-preservation pattern in `restore_game_data`
  as `compiled_script` (saved and restored in the same instruction
  block, alongside `chars`/`numcharacters`). No independent access site
  of its own, but the position is a perfect zero-slack fit: sits
  immediately before `numcharacters` (ends exactly at `+0x2638`),
  reproducing OriGameSetupStruct's declared triple "`char *globalscript;
  int numcharacters; OldCharacterInfo *chars;`" with zero drift across
  all three fields at once. Medium-high confidence (positional +
  categorical, not a fully independent semantic confirmation).
- **`totalscore`** (`int`) @ `+0x8534` -- `dword_51B84C`. Boxed in with
  zero slack between `playercharacter`+`__old_spriteflags[2100]`
  (OriGameSetupStruct's next declared field, `acroom.h:2785`) and
  `numinvitems` -- the exact 2104-byte gap decomposes to `2100 + 4`
  with nothing left over. PLUS direct disassembly confirmation: `main`'s
  giant inlined play-struct-initializer (matching `Engine/AC.CPP:26320-
  26349`'s long chain of `play.x = ...;` assignments) does `mov
  edx,dword_51B84C; mov dword_4EEB30,edx` -- an exact transliteration
  of `"play.totalscore = game.totalscore;"` (`AC.CPP:26348`), sitting
  in the middle of a dozen other already-recognizable sibling
  assignments in the same block. This was the field flagged as "hard,
  needs a slower full-body trace" two rounds ago -- the exact-fit
  position from the byte-counting script is what made the single
  matching instruction findable inside that huge block.
- **`numdlgmessage`** (`int`) @ `+0x9FD0` -- `dword_51D2E8`. The
  4-byte gap flagged as "plausible but unconfirmed" in the previous
  round is now confirmed directly: `load_ac2game_dta` does `cmp
  dword_51D2E8,7D0h(2000); jle <ok>; ...quit("Error in game file: too
  many dialog lin[es]...")`, then loops `malloc(0x1F4=500)` once per
  entry up to that count -- exactly the concept of a global-dialog-
  message count with a compile-time cap. Sits immediately after
  `numdialog` with zero gap, matching 2011's comma-declared `"int32
  numdialog, numdlgmessage;"` (`acroom.h:2826`) exactly.
- **`dict`** (`WordsDictionary *`) @ `+0xA7F0` -- `dword_51DB08`.
  Confirmed via `load_ac2game_dta`: right after the whole-struct
  `fread`, `cmp dword_51DB08,0; jz skip; malloc(0xBB84); dword_51DB08
  =result; call sub_4039AB(stream,result)`. `sub_4039AB` reads an
  `int` (`num_words`) into `[buf+0]`, then loops that many times
  reading a 30-byte string per entry (matching `MAX_PARSER_WORD_LENGTH
  =30`, `acroom.h:337`) into `[buf+4+i*30]`, PLUS a 2-byte short into a
  SECOND fixed-offset array `[buf+0xAFCC+i*2]` -- an exact structural
  match to `WordsDictionary`'s `num_words`/`word[]`/`wordnum[]` fields
  (`acroom.h:341-344`). DRIFT: 2011 uses two separately-sized
  allocations (`char **word`, `short *wordnum`); this build flattens
  both into one fixed-capacity blob behind a single pointer that
  doubles as an on-disk presence flag -- the same "flag becomes
  pointer" idiom as `compiled_script`. Sits immediately after `numgui`
  with zero gap, matching `OriGameSetupStruct2`'s declared adjacency
  `"int numgui; WordsDictionary *dict;"` (`acroom.h:2805-2806`)
  exactly.

**Every one of the five sits at a position matching OriGameSetupStruct's
(or OriGameSetupStruct2's) own declared field order exactly, with zero
slack.** Combined with the earlier `gamename`/`options`/`paluses`/
`numcharacters`+`chars` matches, this struct's true identity is now
about as settled as it can get without a byte-for-byte full recovery:
**Rob Blanc 1's `game_gamename` struct IS `OriGameSetupStruct`**, not a
custom/independently-drifted layout. The struct total is still
confirmed at `0xBF84` (49028 bytes) via the same `fread` call site as
before, and every field added this round keeps that total exact (byte
arithmetic double-checked with a script before committing, same
discipline as `ExecutingScript`).

**One dead end worth recording (fully resolved later -- read on)**:
initially hoped the ~9230-byte gap between `paluses` and `numviews`
would decompose cleanly into `defpal[256]` (as packed `RGB`, 1024
bytes) + `InterfaceElement iface[10]` (computed at exactly 820 bytes/
`0x334` each, matching the `dword_515854` stride from an EARLIER round
that was dismissed as an unrelated "legacy interface" lead) +
`numiface`. The byte-counting script disproved the CLEAN version of
this decomposition: the real labeled content in that span (`g_interface`
1636 bytes, `byte_513B7C` 1 byte, `byte_513B7D` 7383 bytes,
`dword_515854` 4 bytes) doesn't match the predicted sizes exactly. This
turned out to be a temporary dead end, not a permanent one -- see the
"numiface found" and "defpal + iface[10]" sections below for the full,
eventual resolution of every field this paragraph originally hoped for:
`numiface` was confirmed via the same `dword_515854` stride this
paragraph already noticed, `defpal[256]` was reconfirmed via a SECOND,
more thorough access site, and `iface[10]` was confirmed via exact
field-level evidence once `g_interface` turned out to not be a
genuinely separate global at all (its own address falls inside
`defpal`'s confirmed memory) -- resolving the apparent size mismatch
that originally killed this hypothesis.

## OFFSET CORRECTION: the byte-counting script had an alignment bug -- every field from globalscript onward was +8 bytes too low

While going deeper on `default_resolution` (still unresolved -- see
below), tried to find `game_gamename`'s TRUE absolute base address a
third, independent way, to cross-check the value used throughout this
round (`0x513318`, already derived twice: once from `byte_512F2C`'s own
address plus its declared size, once from the retracted `defpal`
label's address minus its assumed `+0x132` offset -- both agreed). This
third check was: `numcharacters`'s real disassembly symbol,
`dword_515950`, embeds its own absolute address in its IDA-generated
name. Direct subtraction (`0x515950 - 0x513318 = 0x2638`) should equal
whatever offset this file already had recorded for `numcharacters`.

**It didn't.** This file said `+0x2630`. An 8-byte mismatch.

Traced it to the ONE `align 10h` directive in the whole struct
(sitting right before the still-unrecovered `mcurs[10]`-shaped region,
between `numviews` and `globalscript`). `align N` directives align the
TRUE ABSOLUTE memory address to a multiple of N -- but
`count_data_offsets.py`'s first version (and, it turns out, whatever
ad-hoc script an EARLIER session used to originally establish
`numcharacters`/`chars`/`playercharacter`/etc.'s positions) rounded the
running RELATIVE offset from `game_gamename` instead. Those two only
agree if `game_gamename`'s own base address happens to already be a
multiple of the alignment. It isn't: `0x513318 mod 0x10 == 8`. So the
"align to 16" step landed 8 bytes short of where the real assembler
would have placed it, and every field after that point inherited the
same 8-byte deficit.

**Verified the scope of the damage with pure hex-address arithmetic**,
independent of any script (immune to the bug by construction): for
every label in the struct whose IDA name embeds its own real address
(`dword_51594C`, `dword_515950`, `dword_51B010`, `dword_51B84C`,
`dword_51D2E4`, `dword_51D2E8`, `dword_51D2EC`, `dword_51D2F0`,
`dword_51D2F8`, `word_51D2FC`, `word_51D2FE`, `dword_51D300`,
`dword_51DB04`, `dword_51DB08` -- 14 labels total), computed
`address - 0x513318` directly and compared against the script's output.
All 14 disagreed with the buggy script by exactly +8, and all 14
agreed with each other and with a CORRECTED version of the script
(which now takes the true base address as a parameter and aligns the
absolute address, not the relative offset) to the byte. `numviews`
itself, and everything before it, was unaffected (the earlier `align 4`
in the `options` region doesn't matter, because `0x513318` already is
a multiple of 4 -- alignment-relative-vs-absolute only diverges when
the base isn't already a multiple of the specific alignment used).

**Every offset for `globalscript` through `dict` recorded earlier in
this file, and originally in `apply_structs.py`, has now been
corrected by +8** (already fixed in both files by the time you're
reading this -- the numbers throughout the "Five new fields" section
above are the corrected ones). This was caught and fixed BEFORE the
struct was ever applied to the live IDB, so no permanent damage --
`apply_structs.py`'s `_pad_unknown1b` field grew from `0xE8` to `0xF0`
(240, not 232, bytes) to absorb the missing 8 bytes, every downstream
padding field's SIZE stayed the same (only its absolute position
shifted), and the struct's grand total is still exactly confirmed at
`0xBF84` (49028 bytes) -- verified with a script before committing, the
same discipline as every other struct edit in this project.

**Process lesson, worth remembering permanently**: a struct-offset-
counting script's output agreeing with a previously-recorded value is
NOT independent confirmation of that value -- if both were computed the
same (buggy) way, they'll agree while both being wrong. The only
trustworthy cross-check is a METHOD-independent one: here, direct
hex-address subtraction using IDA's own auto-generated label names,
which don't go through any alignment logic at all. Whenever a script
computes a struct offset, cross-check at least one result against raw
address arithmetic on a literally-named label before trusting the rest
of its output -- exactly the same "don't trust one technique alone"
discipline as the `defpal` retraction (access pattern) and the
`iface[10]` dead end (arithmetic fit) above, just applied to tooling
instead of evidence.

## default_resolution: confirmed ABSENT, via ConvertOldGameStruct-style techniques -- this build uses a completely different, player-side resolution model

Picked this back up using the same technique that cracked `totalscore`
open: instead of chasing `game.default_resolution`'s own usage sites in
`Engine/AC.CPP` again (already tried twice, both dead ends -- large
inlined functions with no small isolated check), went looking for how
AGS's own OLD-format-compatibility code (`ConvertOldGameStruct`,
`acroom.h:3017`, the same function that revealed this struct's
`OriGameSetupStruct` identity) treats the field when upgrading an old
game.

**It doesn't set it at all.** Read `ConvertOldGameStruct`'s full body
this time (previously only skimmed for `totalscore`): it explicitly
copies over every field `OldGameSetupStruct` actually has (`gamename`
through `numgui`, plus the fixed arrays), and for the handful of fields
`OldGameSetupStruct` does NOT have, it does one of two things --
either leaves them untouched (implicitly whatever the destination
struct was already initialized to), or gives them an explicit
hardcoded fallback. Only ONE field gets the explicit-fallback
treatment: `gss->numcursors = 10;` -- an exact, independent confirmation
of the "every cursor check in this build is hardcoded to 10" finding
from an earlier round, straight from AGS's own upgrade-path source.
`default_resolution`, `default_lipsync_frame`, and `invhotdotsprite`
get NEITHER treatment -- they're simply absent from the assignment
list entirely, left at whatever the destination struct's own default
is (almost certainly zero, matching 2011's own documented semantics
"`default_resolution`: 0=undefined").

**Confirmed what "undefined" (0) actually does at runtime**: `Engine/
AC.CPP:27739-27782` is the full resolution-selection block inside
`main`'s startup sequence -- `usetup.base_width=320;
usetup.base_height=200;` first, then a cascading `if
(game.default_resolution>=5) {...1024x768-ish...} else if (==4||==3)
{640x400} else if (==2||==1) {320x200} else {scrnwid=usetup.base_width;
scrnhit=usetup.base_height;}` -- the `else` (undefined/0) branch is
just plain 320x200, no scaling. This is 2011's fallback for OLD games
that never had this field at all.

**Checked the disassembly's actual equivalent of this block and found
something conclusively different, not just "matches the fallback
branch".** `main`'s own resolution-setup code (`CODE XREF: main+DA6`,
confirmed via the surrounding `_atexit`/`install_allegro`/`set_gfx_mode`
call chain) is:
```
mov usetup_base_width, 320
mov usetup_base_height, 200
mov ecx, usetup_screenres
add ecx, 1
mov screenresIdx, ecx
cmp usetup_screenres, 1
jnz short loc_423001
mov usetup_base_width, 640
mov usetup_base_height, 400
loc_423001:
cmp usetup_screenres, 2
jnz short loc_42301E
mov usetup_base_width, 960
mov usetup_base_height, 600
loc_42301E:
mov scrnwid, usetup_base_width
mov scrnhit, usetup_base_height
... current_screen_resolution_multiplier_x = usetup_base_width / 320
... current_screen_resolution_multiplier_y = usetup_base_height / 200
```
This branches on **`usetup_screenres`**, not any `game.*` field --
`usetup_*` is AGS's PLAYER/config-setup namespace (matching 2011's own
`usetup.` struct, e.g. `usetup.base_width`), i.e. a runtime
player-chosen display-scale preference (0/1/2 selecting 320x200/
640x400/960x600), read from the game's config file, completely separate
from any field baked into the compiled game data at authoring time. Also
notable DRIFT: this build tracks **separate X/Y resolution multipliers**
(`current_screen_resolution_multiplier_x`/`_y`) where 2011 has a single
unified `current_screen_resolution_multiplier` -- another later
simplification, not a reduction.

**Conclusion, three independent lines of evidence agreeing**: (1)
`OriGameSetupStruct` never declares this field at all; (2)
`ConvertOldGameStruct`'s upgrade path leaves it unset (no hardcoded
fallback, unlike `numcursors`); (3) the actual disassembly implements
resolution selection via an entirely different, player-side mechanism
with no `game.*` field read anywhere in the equivalent code path.
`default_resolution` is retired as an open lead for the same reason
`numcursors`/`invhotdotsprite`/`default_lipsync_frame` were: the whole
"game author declares a native resolution, engine scales to fit"
feature is a later AGS addition that postdates this 2002 build, which
instead exposes resolution purely as a player/config-side window-scale
choice. **`GameSetupStructBase` recovery has no more open leads** --
every field flagged as "genuinely open" in earlier rounds is now either
confirmed present (22 fields, later 24 -- see below) or confirmed
absent (`numcursors`, `default_lipsync_frame`, `invhotdotsprite`,
`default_resolution`); what remains is unrecovered CONTENT in the
large gaps (`defpal`'s true size, `messages[]`, and the derived
`GameSetupStruct`'s fixed arrays), not open field-identity questions.

## EventBlock / __charcond / __invcond: the largest remaining gap, fully confirmed -- promoted from arithmetic-fit hypothesis to a completely typed struct

Picked back up the one hypothesis flagged as "arithmetic fit only, NOT
independently confirmed" throughout this whole struct's recovery: the
22200-byte gap between `chars` and `compiled_script`, suspected to be
`EventBlock __charcond[50]` + `EventBlock __invcond[100]`
(`OriGameSetupStruct`'s declared order, `acroom.h:2781-2782`).

The lead: `Engine/AC.CPP:756` has a dead, commented-out declaration --
`//void run_event_block(EventBlock*,int,int=-1, int=-1);` -- proof this
function used to exist and take an `EventBlock*` directly, before 2011
replaced the whole system with `InteractionScripts`. Its two natural
callers, `RunCharacterInteraction` and `RunInventoryInteraction`, are
STILL exported in 2011 with the same script names, so their disassembly
counterparts were locatable directly by name/string search rather than
by dead code -- exactly the same "the modern source names still act as
a directory of the removed system" trick that made `run_event_block`'s
own prototype findable in the first place.

**`RunCharacterInteraction` was already matched** (a real function,
line 43208 of the .asm) and its body was a complete surprise in how
directly it confirmed everything at once:
```
mov     ecx, [ebp+cc]
imul    ecx, 94h
add     ecx, offset unk_515958
push    ecx
call    sub_417088
```
`unk_515958`'s absolute address (`0x515958`) lands EXACTLY on the
zero-slack arithmetic-fit prediction for `__charcond`'s start
(`game_gamename` base `0x513318` + `0x2640`, immediately after the
confirmed `chars` field with no gap) -- and the `imul ecx,94h` stride
is EXACTLY the independently-computed `EventBlock` size (148 bytes).
Two confirmations in one instruction sequence.

**`sub_417088`'s own body then confirmed EVERY field of `EventBlock`
directly**, not just its overall size -- by reading through what turns
out to be a genuinely large, real dispatcher function (881 lines):
- Loops `for(i=0; i<[this+0x80]; i++)` -- confirms **`numcmd`@+0x80** as
  the loop bound.
- Reads `[this+i*4]` and compares it against the caller-supplied
  event-code parameters -- confirms **`list[8]`@+0x00**.
- Reads `[this+i*4+0x20]` and switches on it (1 = a no-op branch, 2 =
  `call StopMoving(playerchar)`, 3 = `call run_on_event(3,data[i])`,
  5 = a further branch) -- confirms **`respond[8]`@+0x20** as a
  response-TYPE code per entry.
- Reads `[this+i*4+0x40]` and passes it straight to the already-matched
  `DisplayMessage` as `msnum` -- confirms **`respondval[8]`@+0x40** as
  the response VALUE.
- Reads `[this+i*4+0x60]` for extra-data comparisons (gating a
  `LoseInventory` call) and as `run_on_event`'s `wparam` argument --
  confirms **`data[8]`@+0x60**.
- Reads/writes `[this+i*2+0x84]` (2-byte stride), passes it to the
  already-matched `GiveScore` if nonzero, then immediately zeroes it --
  confirms **`score[8]`@+0x84** as a one-time per-entry score award.

Every field, at its exact 2011-declared offset, with zero drift in
type, order, or position (`Common/acroom.h:239-246`). `sub_417088`
renamed to `run_event_block` on the strength of this (4-argument
signature matching the dead prototype exactly, both known callers
passing an `EventBlock*` as the first argument, and this complete
internal-layout match).

**`__invcond[100]` confirmed the same way, via a DIFFERENT already-
matched function** (`run_event_block_inv`, `sub_40D8A9`) -- its own
body is a near-direct forward to `run_event_block`:
```
mov     String1, offset aInventoryD ; "inventory%d"
push    0FFFFFFFFh
...
call    sub_417088
```
matching `Engine/AC.CPP:5412`'s `evblockbasename="inventory%d";` almost
verbatim (bonus finding: `String1` is `evblockbasename`). Its own
callers, inside the already-matched `check_controls`, build the pointer
as `offset unk_517640 + iit*0x94` -- `unk_517640`'s address
(`0x517640`) lands EXACTLY at `__charcond`'s computed end (zero gap),
matching `OriGameSetupStruct`'s declared adjacency
`"EventBlock __charcond[50]; EventBlock __invcond[100];"` exactly.

**Net result**: the single largest remaining gap in `GameSetupStructBase`
(22200 of the struct's ~49KB) is now FULLY typed and confirmed, not
just arithmetically boxed in -- `__charcond[50]` @ `+0x2640`,
`__invcond[100]` @ `+0x4328`, both `EventBlock[N]` with every one of
`EventBlock`'s 6 fields independently confirmed via real access sites.
`GameSetupStructBase` is now at 24 confirmed fields (up from 22).

## mcurs[10]: hiding in plain sight -- generic padding was actually MouseCursor's own array, all along

Went looking for the next candidate and found something almost
embarrassingly simple: `MouseCursor`'s own struct was FULLY recovered
several rounds ago (`pic`/`hotx`/`hoty`/`view`/`name`/`flags`, stride
confirmed at exactly `0x18`/24 bytes via `ChangeCursorGraphic`/
`SetMouseCursor`/`__GetLocationType`/a cursor-precache loop in `main`)
-- but nobody had ever checked whether that array's own base address,
`dword_51585C`, happens to fall INSIDE `game_gamename`'s own memory
range. It does: `dword_51585C - 0x513318 = 0x2544` -- exactly where
`_pad_unknown1b` (a 240-byte generic-padding placeholder, sitting
right after `numviews`) used to start.

Checked with the same zero-slack triple confirmation used throughout
this struct: (1) start matches `dword_51585C` exactly; (2) `10*0x18 =
0xF0` matches the pad's declared size exactly; (3) end (`+0x2634`)
matches the already-confirmed `globalscript`'s start with zero gap.
Cross-checked element-by-element too: `dword_5158BC`/`word_5158C0`/
`word_5158C2` (individually labeled in the .asm, likely because cursor
index 4 = `MODE_USE` gets special-cased hotdot-marker handling in
`SetMouseCursor`) sit EXACTLY at `(0x25A4-0x2544)/0x18 = 4` -- element
index 4's `pic`/`hotx`/`hoty` offsets -- independently confirming the
24-byte stride holds across the whole array, not just element 0.
Matches `OriGameSetupStruct`'s declared `MouseCursor mcurs[10];`
(`acroom.h:2777`) exactly, in both position (right after `numviews`)
and identity.

**Bonus clarification, while re-checking this region**: the byte span
`+0x254E`..`+0x2558` (element 0's unreferenced `name[10]` field) is
where IDA's `.asm` export emits a spurious `align 10h` directive. This
is NOT a genuine compiler-inserted alignment gap -- it's IDA's own
heuristic mislabeling of real (if never-referenced) struct content:
`name[10]` sits at exactly that offset within `MouseCursor`'s own
declared layout (`pic`(4)+`hotx`(2)+`hoty`(2)+`view`(2)=10 bytes in,
then `name[10]`, then `flags` at `+0x14`), and the resulting absolute
address for `flags` (`0x515870`) happens to be 16-byte-aligned purely
by coincidence -- IDA appears to default to representing unreferenced-
yet-aligned gaps as `align` filler rather than raw bytes. This does
NOT change anything about the separately-verified OFFSET CORRECTION
(the +8-byte fix documented earlier in this file) -- that correction's
byte-count was independently verified via direct hex-address
subtraction and remains exactly right regardless of how this one gap
is semantically interpreted; it just explains WHY that particular
`align` directive exists in the source `.asm` in the first place.

**Struct-layout implication**: since `MouseCursor mcurs[10]` sits
directly inside `GameSetupStructBase` (not in some separate global),
this is further confirmation that this build flattens whatever 2011
splits into base+derived (`GameSetupStructBase`+`GameSetupStruct`)
into one struct, matching `OriGameSetupStruct`'s own flat declaration
exactly. `GameSetupStructBase` is now at 25 confirmed fields (up from
24).

**Process note**: while chasing this, found that 6 `matches.json`
entries (`FadeOut`, `SetSpeechFont`, `RunDialog`, `InterfaceOn`,
`init_translation`, `SetMouseCursor`) still cited their fields'
PRE-correction relative offsets from before the OFFSET CORRECTION round
-- that round fixed `apply_structs.py`/`struct-layout-drift.md`/
`CLAUDE.md` but never swept `matches.json` itself for the same stale
numbers. Fixed all 6 in place (each now also carries an explicit
"[Offset corrected...]" note explaining why the number changed, so a
future reader isn't confused by the discrepancy against an old
`.asm` export's comments). Worth remembering: an offset-correction
pass needs to check EVERY file that cites the corrected numbers, not
just the ones actively being edited at the time -- `matches.json`
entries reference struct offsets just as much as the struct
declaration's own comments do.

## numiface found -- an old dead-end lead, revisited with fresh eyes after OriGameSetupStruct's discovery

Went back to the one remaining unresolved region before `numviews`
(`_pad_unknown1a`, `+0x132`..`+0x2540`, 9230 bytes) and reconsidered
`dword_515854` -- the SAME global that an earlier round ("numdialog
found") investigated as a `numdialog` candidate and correctly ruled
OUT (its indexed array target, `0x334`-byte-stride data, didn't match
`DialogTopic`'s size). That round dropped the lead entirely without
asking what `dword_515854` might otherwise BE -- reasonable at the
time, since `OriGameSetupStruct`'s identity (and therefore its
declared `int numiface; int numviews;` adjacency) wasn't known yet.

**Position, checked fresh**: `dword_515854 - game_gamename_base =
0x253C`, ending at `0x2540` -- EXACTLY where `numviews` (confirmed
independently, `+0x2540`) begins, zero gap. The exact same "count field
immediately before the next field" idiom already seen for
`numcharacters`/`chars`, `numgui`/`dict`, and `numdialog`/
`numdlgmessage`/`numfonts`.

**Usage, re-read with the InterfaceElement stride now known**: inside
`load_ac2game_dta` (already matched), `cmp ecx,dword_515854; jge <done>`
gates a loop indexing `byte_513B7C[i*0x334]`/`byte_513B7D[i*0x334]` --
and `0x334` (820 bytes) is EXACTLY `InterfaceElement`'s size,
independently computed several rounds ago while investigating
`__charcond`/`__invcond`'s `EventBlock` stride. The earlier round noted
this exact loop and its `0x334` stride, but dismissed the WHOLE thing
as "more of the same legacy interface system... not dialog data" --
correct about it not being dialog data, but it never circled back to
ask whether the LOOP BOUND itself might be a real, nameable field. It
is: `dword_515854` = `numiface`, matching `OriGameSetupStruct`'s
declared `InterfaceElement iface[10]; int numiface; int numviews;`
sequence (`acroom.h:2774-2776`) in both position and semantics
(count-of-populated-interface-elements).

**`iface[10]` itself remains an open, intriguing-but-unconfirmed lead.**
The three labeled chunks preceding `numiface` -- `g_interface` (1636
bytes), `byte_513B7C` (1 byte), `byte_513B7D` (7383 bytes) -- don't
individually divide evenly by `0x334`, but their SUM does, exactly:
`1636+1+7383 = 9020 = 11 * 0x334`. That's a striking coincidence if
`g_interface` really is the "genuinely unrelated global" it was
originally assumed to be (a totally separate global shouldn't make the
combined total land on a clean multiple of an unrelated struct's size)
-- but it doesn't cleanly confirm `iface[10]` either, since 2011
declares a 10-element array, not 11, and there's no evidence yet for
WHERE within this 9020-byte span the boundary between two adjacent
`InterfaceElement`s (as opposed to the arbitrary point IDA happened to
split `g_interface` from `byte_513B7C`) actually falls. Also still
unresolved: `defpal`'s true size (the `+0x132`..`+0x200`-ish gap before
this region even starts, only ~206 bytes -- nowhere near 1024 bytes for
packed RGB, echoing the original retraction). Flagged as a documented
hypothesis in `apply_structs.py`, not asserted -- per this project's
now well-established rule that a suggestive arithmetic fit alone isn't
sufficient without independent per-field confirmation (the `defpal`
retraction, the `iface[10]`-vs-`g_interface` dead end, and the OFFSET
CORRECTION all taught this same lesson in different ways).

`GameSetupStructBase` is now at 26 confirmed fields (up from 25).

## defpal + iface[10]: the LAST major gap resolved -- g_interface was never a separate global at all

Went back to the two open leads flagged in the section above: `defpal`'s
true size, and `iface[10]`'s exact internal boundaries. Both turned out
to have a single, shared resolution.

**`defpal`, re-examined**: the original retraction (several rounds ago)
checked ONLY IDA's own declared label extent for `dword_51344A` (~204
bytes before hitting `g_interface`) -- it never checked whether the
*code itself* reads further than that boundary. It does. `Engine/
AC.CPP:26196-26198`'s `"for(ee=0;ee<256;ee++) if(paluses[ee]!=
PAL_BACKGROUND) palette[ee]=defpal[ee];"` has a SECOND, independent copy
inside `main` (distinct from the copy inside `load_new_room`, which
turns out to read from a totally different global, `dword_51F69C` --
easy to confuse the two, don't). `main`'s copy is `"cmp
byte_51334A[ecx],2; jz <skip>; mov ecx,dword_51344A[eax*4]; mov
palette[edx*4],ecx"` -- an UNCONDITIONAL 4-byte-stride read reaching all
the way to index 255, addressing a full 1024 bytes starting at
`dword_51344A`. Matches 2011's `color defpal[256]` (`acroom.h:2773`) in
type AND count with zero drift.

**The key realization**: that 1024-byte addressable range extends well
past where IDA's own `g_interface` label begins (`+0x200`, only 206
bytes into the array). Since `game_gamename` is ONE single contiguous
allocation (confirmed via its own `sizeof()`-based `fread` call), a
genuinely SEPARATE global cannot have an address that falls inside
already-allocated struct memory. `g_interface` (and the `byte_513B7C`/
`byte_513B7D` labels right after it) were never independent globals at
all -- they're simply IDA's own arbitrary sub-labeling of bytes that
are really part of `defpal`'s (and, further along, `iface[10]`'s) own
storage. The original "collided with a totally unrelated global"
framing from the retraction was backwards: there was no collision,
because there was no separate global.

**`iface[10]`, resolved the same way, with an extra piece of luck**:
`defpal[256]` ends at `+0x532` -- 2 bytes short of 4-byte alignment.
`InterfaceElement`'s own int-heavy layout needs 4-byte alignment, so a
natural compiler pad brings the next field to `+0x534`. From there,
`10 * sizeof(InterfaceElement) = 10 * 0x334 = 0x2008` bytes lands
EXACTLY on the already-confirmed `numiface`'s start (`+0x253C`) --
zero slack, matching `OriGameSetupStruct`'s declared `"InterfaceElement
iface[10]; int numiface;"` adjacency (`acroom.h:2774-2775`) exactly.
This is the SAME arithmetic fit flagged as "intriguing but unconfirmed"
in the section above (there, the fit looked like 11 elements because
`g_interface` was still being counted as separate space alongside
`iface[10]` rather than recognized as part of `defpal`'s own memory --
once that double-counting is removed, the fit is exactly 10 elements,
matching 2011 precisely).

**Confirmed with real field-level evidence, not just the arithmetic
fit**: `byte_513B7C`'s own address sits at offset `0x330` within this
newly-established `iface[]` array -- computed `InterfaceElement`'s
declared field layout (`x,y,x2,y2`, `bgcol/fgcol/bordercol`,
`vtextxp/vtextyp/vtextalign`, `vtext[40]`, `numbuttons`, `button[20]`
at `InterfaceButton`'s own computed `0x24`-byte stride, `flags`,
`reserved_for_future`, `popupyp`) lands `popup` at EXACTLY offset
`0x330` and `on` at `0x331` -- matching `byte_513B7C`/`byte_513B7D`'s
positions to the byte. And the disassembly's own usage matches the
semantics: `load_ac2game_dta` checks `iface[i].popup == 2` and sets
`iface[i].on` to 0 or 1 accordingly -- consistent with 2011's
constructor default (`"on = 1;"`, `acroom.h:318`) being conditionally
overridden during game-data load.

**What's still NOT independently confirmed**: `InterfaceElement`'s
other ~13 fields (everything except `popup`/`on`) have no access-site
evidence of their own yet, and `InterfaceButton` (embedded 20x inside
each `InterfaceElement` via `button[20]`) has none at all -- both are
left as opaque padding in `apply_structs.py` rather than asserted
field-by-field, the same discipline used for `EventBlock` before ITS
fields were individually confirmed via `run_event_block`. The overall
`iface[10]` array boundary and its `InterfaceElement` element size are
now high-confidence; the STRUCT's internal field layout beyond
`popup`/`on` is inherited from 2011 by positional inference only.

`GameSetupStructBase` is now at 28 confirmed fields (up from 26) --
`defpal`/`iface[10]` was the last major unresolved span in this struct.

## messages[500] + the whole uniqueid-to-numgui tail: the entire remaining OriGameSetupStruct2 gap resolved in one pass

With `defpal`/`iface[10]` resolved, the only sizeable unconfirmed span
left was `_pad_unknown6` (`+0x9FEC`..`+0xA7EC`, 2048 bytes, between the
already-confirmed `uniqueid` and `numgui`). Checked whether it could
hold `OriGameSetupStruct`'s ENTIRE declared tail after `uniqueid` --
`reserved[2]`, `numlang`, `langcodes[MAXLANGUAGE=5][3]`,
`messages[MAXGLOBALMES=500]` -- plus `OriGameSetupStruct2`'s own
additions right before `numgui`, `fontflags[10]`+`fontoutline[10]`
(`acroom.h:2795-2799`, `2803-2804`).

**Summing the predicted sizes with proper alignment** (`messages[]` is
a pointer array and needs 4-byte alignment; `langcodes`'s 15-byte
extent lands 3 bytes short) gives EXACTLY 2048 bytes -- zero slack,
matching `_pad_unknown6`'s size to the byte:
`reserved[2](8) + numlang(2) + langcodes[5][3](15) + align(3) +
messages[500](2000) + fontflags[10](10) + fontoutline[10](10) = 2048`.

**Confirmed with real evidence, not just the sum**: `messages[500]`'s
predicted address (`+0xA008`, i.e. `dword_51D320`) turned out to be an
already-labeled global with real `DATA XREF`s, inside `load_ac2game_dta`
(already matched): `"for(i=0;i<0x1F4(500);i++) { if(dword_51D320[i*4]
==0) continue; else { malloc(0x1F4=500); dword_51D320[i*4]=result;
fread(...) into it } }"` -- a per-slot conditional message-string
loader. Immediately after this loop ends, the code falls straight into
the already-confirmed `set_default_glmsg` chain for the 12 built-in
messages (`MSG_RESTORE=984` etc.) -- conclusive confirmation this is
genuinely the global-messages-override system, not a coincidentally-
similar loop shape. Matches 2011's `char *messages[MAXGLOBALMES]`
(`acroom.h:2799`) in type (pointer array) and count with zero drift.

This makes the fit "over-determined" rather than a simple two-endpoint
arithmetic argument (the kind this project has repeatedly cautioned
against trusting alone): `messages[500]`'s own address is independently
confirmed via real disassembly evidence, sitting exactly where the
PREDICTED reserved/numlang/langcodes/alignment total places it, which
in turn is exactly `0x800` bytes before the independently-confirmed
`numgui` once `fontflags`/`fontoutline` are added on the other side --
three independent constraints (two confirmed endpoints, one confirmed
interior anchor) all agreeing to the byte.

**What's still not independently confirmed**: `reserved[2]`,
`numlang`, `langcodes[5][3]`, `fontflags[10]`, `fontoutline[10]`
themselves have no access-site evidence of their own. Checked and came
up empty for a reason: 2011's own source has essentially abandoned
these fields by 2011 -- `numlang`/`langcodes`'s only remaining mention
in `Engine/AC.CPP` (`init_language_text(game.langcodes[0])`, line
26230) is ITSELF commented-out dead code, the same "system predates or
was superseded before 2011" pattern already seen for `run_event_block`.
`fontflags`/`fontoutline`'s 2011 reader (`AC.CPP:11655-11656`) lives in
a structurally divergent, much-later game-loading routine that also
reads `guid`/`saveGameFileExtension`/`saveGameFolderName` -- all
confirmed-later additions with zero counterpart anywhere in this 2002
build, so there's no 2011 anchor to guide a disassembly search for
this build's own access site (if one even exists). These five fields
are included at medium confidence, positionally boxed in by the
exact-fit arithmetic, following the same precedent as `target_win`
earlier in this struct's recovery.

`GameSetupStructBase` is now at 34 confirmed fields (up from 28) --
**every field `OriGameSetupStruct`/`OriGameSetupStruct2` declares is
now accounted for** (at the time this was written -- see the
`invinfo[100]` section immediately below for a correction: it turned
out to ALSO be declared directly in `OriGameSetupStruct` itself, not
just the further-derived `GameSetupStruct` as first assumed here).
Remaining unrecovered content is entirely inside the trailing
`_pad_unknown7` gap (`+0xA7F4`..`+0xBF84`, 6032 bytes): whatever
fixed-size arrays the further-derived `GameSetupStruct` embeds directly
(`spriteflags`, `acroom.h:2890-2917`) that this build predates
`OriGameSetupStruct`'s own layout for -- a genuinely different, harder
kind of lead, since there's no `OriGameSetupStruct` declaration left to
anchor against for THAT gap specifically.

## invinfo[100]: found while surveying for a NEW struct to tackle -- the strongest zero-drift match in the project

Asked to switch focus to a genuinely different structure, surveyed
`Common/acroom.h`'s full struct list against what's already confirmed.
Two promising leads stood out, both tied to already-confirmed count
fields: `DialogTopic` (`numdialog`) and `InventoryItemInfo`
(`numinvitems`).

**`DialogTopic` checked first, hit a real wall**: `Engine/AC.CPP:567`
declares `DialogTopic *dialog;` as a genuine pointer, dynamically
`malloc`+`fread`'d in `load_ac2game_dta` with an explicit per-element
size of `0x484` (1156 bytes) -- found by searching for the
already-confirmed `numdialog` (`dword_51D2E4`) as a `fread` `ElementCount`
argument. But `1156` bytes is nowhere near 2011's declared
`DialogTopic` size (`optionnames[30][150]` alone is 4500 bytes, ~4696
total) -- and unlike `CharacterInfo`/`GameSetupStructBase`, `acroom.h`
preserves NO older ancestor declaration for `DialogTopic` to check
against. Reconstructing it would mean starting from zero evidence, not
even an old reference struct to anchor a hypothesis against -- shelved
as a harder, more novel task for a future round.

**`InventoryItemInfo` picked up instead, and paid off immediately.**
`SetInvItemPic` (already matched, script-exported) writes its `piccy`
parameter into `dword_51B870[item*0x44]` -- `0x44` (68 bytes) matching
`InventoryItemInfo`'s own computed total size from its 2011 declaration
almost exactly (a promising sign before even chasing the field itself).
Tracing `dword_51B870` through every caller confirmed FOUR fields at
once, each landing at its EXACT 2011-declared offset:

- **`pic`** @ element-relative `+0x1C`: confirmed via `SetInvItemPic`
  (writes it), `GUIInv::Draw` (already matched -- reads it as a
  sprite-cache index for icon rendering), AND `sub_40CF16` (an unnamed
  helper called from both `SetInvItemPic` and `SetMouseCursor` -- reads
  it to set the MODE_USE cursor's picture, and again as a sprite-index
  for an auto-center-hotspot fallback when no explicit hotspot is set).
  Three independent callers, one field, all consistent.
- **`hotx`**/**`hoty`** @ `+0x24`/`+0x28`: confirmed via `sub_40CF16`
  (applies them to the already-confirmed `mcurs[MODE_USE].hotx`/`.hoty`)
  AND via `main`'s startup sequence (scales them by
  `current_screen_resolution_multiplier_x`/`_y`) -- both read/write
  them as FULL DWORDS, matching 2011's declared `int hotx, hoty`
  (`acroom.h:2627`) with ZERO type drift. Worth noting explicitly:
  `MouseCursor`'s OWN `hotx`/`hoty` ARE `short` in this build --
  two different, independently-confirmed types for similarly-named
  fields in two different structs, don't conflate them just because
  the names match.
- **`flags`** @ `+0x40` (the struct's LAST field): confirmed via
  `main`'s startup sequence -- `"movsx eax,byte_51B894[ee*0x44]; and
  eax,1; jz <skip>; ...playerchar->inv[ee]=1..."`, checking bit 0 and
  giving the player 1 starting copy of the item if set. Matches 2011's
  `IFLG_STARTWITH=1` (`acroom.h:2623`, "start the player off with this
  item") exactly.

Four fields, four exact matches to 2011's declared offsets -- `pic`
(+0x1C), `hotx` (+0x24), `hoty` (+0x28), `flags` (+0x40) -- with ZERO
drift in any of them. This is genuinely the strongest confirmation-to-
effort ratio found in this project alongside `MouseCursor`'s own "rare
full match" case. The one open question: 2011 declares a SEPARATE
`cursorPic` field at `+0x20` (between `pic` and `hotx`) that no traced
code path in this build reads or writes -- every cursor-picture code
path found (`sub_40CF16`) uses `pic` directly instead. Left as an
unconfirmed (not asserted, not denied) gap in `apply_structs.py`: there's
no NEGATIVE evidence the field is absent (unlike `numcursors` etc.,
which have explicit absence proof), just no code path that happens to
touch it in what's been traced so far.

**And the array's own position resolves what used to be
`GameSetupStructBase`'s `_pad_unknown4` gap entirely**: `dword_51B870`'s
absolute address, minus its own `+0x1C` intra-element offset, lands
EXACTLY 2 bytes after the already-confirmed `numinvitems` ends -- a
natural alignment pad (a `short` field needing to reach 4-byte
alignment before an int-heavy array). And `100 * sizeof
(InventoryItemInfo) = 100*0x44 = 0x1A90` bytes from there lands EXACTLY
on the already-confirmed `numdialog`'s start, zero slack. Matches
`OriGameSetupStruct`'s own declared adjacency `"short numinvitems;
InventoryItemInfo invinfo[100]; int numdialog, numdlgmessage;"`
(`acroom.h:2787-2789`) exactly -- meaning `invinfo[100]` was ALREADY
part of the ancient ancestor struct's own declaration all along, not
something added later by `GameSetupStruct`'s derived-class extensions
as the previous section's closing paragraph assumed before this was
traced through properly. `GameSetupStructBase` is now at 35 confirmed
fields (up from 34).

## DialogTopic: a genuinely NEW struct with no 2011 reference to lean on -- EVERY field eventually confirmed, one field proven ABSENT

Picked back up the `DialogTopic` lead shelved in the previous round.
Unlike every other struct tackled in this project, `Common/acroom.h`
preserves no older ancestor declaration for `DialogTopic` -- no
`OldDialogTopic`, no `DialogTopic272`, nothing. 2011's own declaration
(`acroom.h:2673-2696`) is the only reference available, and it's
~4696 bytes -- over 4x this build's already-confirmed 1156-byte
(`0x484`) per-topic size. This is the first struct this session where
"check against the reference and expect broad agreement" simply
doesn't apply; every field needed independent disassembly confirmation
with no size/shape hint to guide the search.

**The global itself**: `Engine/AC.CPP:567` declares `DialogTopic
*dialog;` as a genuine standalone pointer (NOT embedded in
`GameSetupStructBase` or anywhere else) -- found by searching for the
already-confirmed `numdialog` (`dword_51D2E4`) as a `fread`
`ElementCount` argument inside `load_ac2game_dta`: `"imul edx,
numdialog; add edx,5; malloc(edx); dword_4EDA48=result;
fread(result,ElementSize=0x484,ElementCount=numdialog,stream)"`. The
`+5` beyond the raw array size is presumably a small safety margin or
sentinel, not itself a meaningful field. `dword_4EDA48` should be
renamed to `dialog` (typed `DialogTopic *`) manually in IDA -- this
project's `matches.json`/`apply_matches.py` machinery only covers
FUNCTION renames, there's no automated path for a raw data global like
this one.

**Six fields confirmed, each from a different already-matched caller**:

- **`optionflags[15]`** @ `+0x41C` (60 bytes) -- the strongest evidence
  of the round: `SaveGameSlot` (already matched) has "`push 0Fh
  /* ElementCount */; push 4 /* ElementSize */; lea eax,[dialog+i*0x484
  +0x41C]; call fwrite`" -- an explicit, LITERAL `0x0F` (15) constant,
  not an inferred gap size. Independently cross-confirmed by
  `restore_game_data` (already matched)'s matching `fread` with the
  identical `0x0F`/`4`/`+0x41C` triple -- a genuine save/restore
  round-trip of the topic's mutable per-option ON/OFF state (option
  text/scripts are presumably static per-game-file data that doesn't
  need saving, which is why only `optionflags` gets round-tripped).
  Type/role also confirmed via `SetDialogOption` (already matched,
  script-exported): bitwise `AND ~1`/`OR 1` and `AND 2`/`OR 2`
  manipulation matches 2011's documented `DFLG_ON=1`/`DFLG_OFFPERM=2`
  semantics exactly. **DRIFT**: `MAXTOPICOPTIONS=15` here, not 2011's
  declared 30 -- a genuine capacity halving, matching the "smaller
  fixed capacity" pattern already seen throughout this project
  (`ExecutingScript`'s `MAX_QUEUED_SCRIPTS=2`, `numcursors` hardcoded
  to 10, etc.).
- **`optionscripts`** @ `+0x458` -- confirmed via `load_ac2game_dta`:
  `"cmp [dialog+i*0x484+0x458],0; jz <skip>; Size=word[dialog+i*0x484
  +0x47C]+0xA; malloc(Size); [dialog+i*0x484+0x458]=result;
  fread(result,ElementSize=word[...+0x47C],1,stream)"` -- an on-disk
  presence-flag-that-becomes-a-real-pointer idiom, the SAME pattern
  already confirmed for `GameSetupStructBase.compiled_script`/`.dict`.
  Matches 2011's `unsigned char *optionscripts` (compiled per-option
  dialog bytecode) in identity and position.
- **`entrypoints[15]`** @ `+0x45C` (30 bytes) -- MEDIUM confidence, not
  independently confirmed via its own access site: boxed in with zero
  slack between the confirmed `optionscripts` and `startupentrypoint`,
  and 15 elements at 2 bytes each matches the SAME independently-
  confirmed `MAXTOPICOPTIONS=15` capacity as `optionflags` exactly --
  not just a coincidental gap fit.
- **`startupentrypoint`** @ `+0x47A` -- confirmed via `do_conversation`
  (already matched): `"movsx edx,word[parmtr+0x47A]; push edx; push
  parmtr; call run_dialog_request"` -- passed alongside the dialog
  topic pointer itself to `run_dialog_request` (already matched),
  matching 2011's role ("initial dialog-script entry point") exactly.
- **`codesize`** @ `+0x47C` -- confirmed via the SAME `load_ac2game_dta`
  site as `optionscripts` above: used directly as the malloc/fread
  size for the compiled bytecode blob, matching 2011's declared `short
  codesize` in type and role exactly.
- **`numoptions`** @ `+0x480`, ending EXACTLY at the struct's own
  confirmed total (`0x484`) -- confirmed via `SetDialogOption`
  (already matched): `"cmp opt,[dlg*0x484+dialog+0x480]; setnle bl;
  ...quit(\"!SetDialogOption: Invalid option number\")"`, an
  option-index range check matching 2011's role and position exactly.

**Architectural finding, mirroring `ExecutingScript`'s `+0x64` field
discovery**: `numoptions` landing exactly at the struct's own confirmed
end leaves NO ROOM for 2011's `int topicFlags;` (2011's declared LAST
field, right after `numoptions`) -- `topicFlags` is CONFIRMED ABSENT
from this build, the zero-slack arithmetic proves it, not merely
"unfound."

**`optionnames` -- the one open question -- resolved immediately in the
very next round**, by directly following `do_conversation`'s (already
matched) option-display code rather than searching for `get_translation`
by name first (that search came up empty -- its distinctive error
string, `"!Null string supplied to CheckForTranslations"`, doesn't
exist anywhere in this disassembly, meaning 2011's `get_translation`
either predates this build under a different name or genuinely didn't
exist with that exact error-handling shape yet). Instead, searched for
the STRING already cited in `do_conversation`'s own match evidence
(`"!DoDialog: all options have been turned off"`, from `show_dialog_options`,
inlined into `do_conversation` per an earlier round) and read straight
through the surrounding code. Found the option-text-rendering loop
directly: `"movsx eax,[disporder_buf+var_50]; imul eax,46h; mov
ecx,parmtr; add ecx,eax; push ecx; call GetTranslation"` -- passing
`dialog[dlgnum] + chosenOption*0x46` to a function ALREADY correctly
named `GetTranslation` in the IDB (capitalized differently from 2011's
`get_translation`, but otherwise unchanged) -- reading its body
confirmed a clean, simplified match to 2011's `get_translation`
(`AC.CPP:2041-2072`): `strlen` + a `transtree` lookup + fallback to the
original string, missing only 2011's later plugin-hook and
`"&12 text"`-prefix-stripping additions.

The `imul eax,46h` is the answer: **`0x46` (70 decimal) is
`optionnames`'s per-entry stride**. `15 * 0x46 = 0x41A` (1050 bytes),
leaving an exact 2-byte alignment pad before the already-confirmed
`optionflags` at `+0x41C` -- zero slack, resolving the entire region in
one shot. DRIFT: `optionnames` is `char[15][0x46]` in this build, not
2011's declared `char[30][150]` -- BOTH the element count and the
per-option text length are smaller, the same "reduced fixed capacity"
pattern as everything else found in this struct.

**Net result: EVERY field of `DialogTopic` is now confirmed** --
`optionnames`, `optionflags`, `optionscripts`, `entrypoints`,
`startupentrypoint`, `codesize`, `numoptions` all located and typed,
plus `topicFlags` confirmed absent. `MAXTOPICOPTIONS=15` is now
independently confirmed THREE separate ways (the `optionnames` fit,
the `optionflags` fit, and the literal `0x0F` constant in `SaveGameSlot`/
`restore_game_data`) -- about as solid as evidence gets in this
project. A struct that started with zero reference material to lean on
ended up as complete as `MouseCursor`/`InventoryItemInfo`'s "exact
zero-drift match" cases, just via a different route (real access-site
evidence for every field instead of a clean match to a still-existing
2011 declaration, since none exists for this one). The lesson: when a
name-based search for the "obvious" 2011 function comes up empty
(`get_translation`'s error string wasn't in the binary), following an
ALREADY-matched caller's actual code -- rather than searching for the
callee by name first -- found the same function under a
differently-capitalized name in one pass.

## GameSetupStructBase: FULLY MAPPED -- spriteflags[6000] closes the very last gap, every byte of the struct now accounted for

Went back to `_pad_unknown7` (`+0xA7F4`..`+0xBF84`, 6032 bytes), the
last remaining span in `GameSetupStructBase` and the one flagged as
genuinely harder than everything resolved so far -- no
`OriGameSetupStruct` declaration exists to anchor a search against it,
since it's entirely inside the further-derived 2011 `GameSetupStruct`'s
own additions (`spriteflags[MAX_SPRITES=30000]`, `invinfo[MAX_INV]`
again, audio clips, custom properties, GUIDs, room lists -- almost all
clearly much-later features this build predates).

**Checked what's already referenced inside that address range first**,
rather than guessing at which 2011 field might apply. Computed the
range's real addresses (`game_gamename`'s base `+0xA7F4` through
`+0xBF84` → `0x51DB0C`..`0x51F29C`) and grepped for any already-
labeled global inside it. Found exactly one: `byte_51DB2C`, referenced
from `prepare_characters_for_drawing` (already matched) at relative
offset `+0xA814` (right after the small `+0x20` gap past the
already-confirmed `dict`).

**`byte_51DB2C` is `spriteflags`**, confirmed three independent ways in
the SAME function:
1. `"xor eax,eax; mov al,byte_51DB2C[edx]; and eax,2; test eax,eax; jz
   <skip>; ...var_C=final_col_dep..."` -- reads a per-sprite flags
   byte and checks bit 1 to gate hi-color-specific rendering behavior,
   sitting right alongside an already-confirmed sprite-width lookup
   (`dword_4CD2E8[index*4]`) -- matching 2011's `SPF_HICOLOR`-style
   per-sprite flag semantics exactly.
2. A direct sanity-clamp bounds check in the same function: `"cmp
   index,0; jl <clamp>; cmp index,1770h; jl <ok>; clamp: index=0"` --
   `0x1770` is **6000 decimal**, a LITERAL constant, not an inferred
   fit.
3. `+0xA814` plus 6000 bytes lands EXACTLY on `GameSetupStructBase`'s
   own independently-confirmed total size (`0xBF84`, from the
   `fread`/`fwrite` `sizeof()` constant used consistently throughout
   `load_ac2game_dta`/`SaveGameSlot`/`restore_game_data`) with zero
   slack -- `spriteflags` is the struct's own FINAL field, the
   remaining space wasn't a coincidence, it's exactly what was left.

**DRIFT**: `MAX_SPRITES=6000` in this build, not 2011's declared 30000
(`acroom.h:2698`) -- a 5x capacity reduction, the same "smaller fixed
capacity" pattern as `ExecutingScript`'s `MAX_QUEUED_SCRIPTS`,
`DialogTopic`'s `MAXTOPICOPTIONS`, `numcursors`'s hardcoded 10, and
just about everything else in this project.

**The small `+0x20` (32-byte) gap between `dict` and `spriteflags`
also resolved itself for free**: it's an EXACT match for
`OriGameSetupStruct2`'s own declared LAST field, `int reserved2[8]`
(`acroom.h:2807`, `8*4=32` bytes) -- no access-site evidence of its
own (consistent with genuinely unused reserved space), but boxed in
with zero slack between two independently-confirmed neighbors. This
completes `OriGameSetupStruct2`'s ENTIRE declared field list with zero
remaining gaps -- literally every field either of `OriGameSetupStruct`
or `OriGameSetupStruct2` declares is now accounted for, plus one
further-derived `GameSetupStruct` field (`spriteflags`) that this
build apparently added early, ahead of the rest of that later class's
additions.

**`GameSetupStructBase` is now FULLY MAPPED**: every byte from `+0x00`
through `+0xBF84` (49028 bytes total) is accounted for, closing out
what has been by far the largest and longest-running single-struct
effort in this project -- 37 fields confirmed across dozens of rounds,
spanning a genuine mid-investigation retraction (`defpal`), a real
tooling bug found and fixed (the alignment-rounding `OFFSET
CORRECTION`), a completely novel from-scratch struct reconstruction
that hung off it (`DialogTopic`), and a handful of fields independently
proven ABSENT rather than merely unfound (`numcursors`,
`default_lipsync_frame`, `invhotdotsprite`, `default_resolution`).
Nothing further remains to recover in this struct's own layout.

## MoveList: another complete, zero-drift match -- found by surveying for the next struct after GameSetupStructBase closed out

With `GameSetupStructBase` fully mapped, surveyed `InterfaceElement`
first (only `popup`/`on` confirmed of its ~15 fields) but hit a wall:
no already-matched function references ANY of its other fields
anywhere in the disassembly -- the whole "legacy interface" subsystem
(`g_interface`'s only live code reference is a single
`scAdd_External_Symbol("interface", ...)` script-export registration,
AC.CPP:59954-ish) appears to be genuinely rarely touched by the
compiled game logic traced so far. Shelved, no further evidence
available without a slower, from-scratch investigation.

**Pivoted to `MoveList` (`mls[]`) instead**, and it paid off
immediately. `SaveGameSlot`/`restore_game_data` (both already matched)
already had a literal `fwrite`/`fread` with `ElementSize=0x200`(512),
`ElementCount=0x3C`(60) for the global `mls` array -- and the live IDB
ALREADY had a PARTIAL `MoveList` type applied to it (from before this
project's own tracking began), with only `pos`/`numstage` named via
dot-notation access (`mls.pos[eax]`, `mls.numstage[eax]`); every OTHER
field was still accessed via raw offset arithmetic
(`(mls+1E4h)[eax]` etc.), meaning nobody had extended that partial
type since.

**Every remaining field confirmed, all in already-matched functions,
all at 2011's exact declared offset**:
- `pos[40]`/`numstage` (already named in the IDB) re-confirmed via
  `find_route` (`Engine/routefnd.cpp:766`): `numstage` is set to the
  computed route's stage count, immediately followed by a direct
  `memcpy(&mls[movlst].pos, Src, numstage*4)` populating `pos[]` --
  matching both fields' declared roles exactly.
- `fromx`/`fromy`/`onstage`/`onpart`/`lastx`/`lasty`/`doneflag` (7
  fields) all confirmed in the SAME `find_route` function, a later
  branch doing a clean "start of move" reset: `fromx`/`fromy` set from
  a computed coordinate, `onstage`/`onpart` zeroed, `lastx`/`lasty` set
  to `-1` (a "not yet drawn" sentinel), `doneflag` zeroed -- all via
  literal offsets (`+0x1E4` through `+0x1FC`) that match 2011's
  declared layout (`Common/acroom.h:3082-3090`) to the byte, zero
  drift in any of them.
- `walk_character` (already matched) independently confirms the
  array's own `0x200`(512)-byte stride via a `shl eax,9` (2^9=512)
  scaling, and reads `pos[0]`/`pos[1]` to short-circuit a route with
  only one distinct waypoint.
- `xpermove[40]`/`ypermove[40]`/`direct` remain MEDIUM confidence
  (positionally boxed in with zero slack between confirmed neighbors,
  no independent access site found this round) -- consistent with the
  project's usual "include at lower confidence, don't assert past the
  evidence" treatment for the small remainder of an otherwise
  fully-confirmed struct.

Total size (`512` bytes, including a natural 2-byte trailing alignment
pad after the 1-byte `doneflag`+`direct` pair) matches the ALREADY-
confirmed `fwrite`/`fread` `ElementSize` exactly. `MoveList` joins
`MouseCursor`/`InventoryItemInfo` as one of the cleanest zero-drift
matches in this project -- and as a nice bonus, this round's field
definitions can be applied via `apply_structs.py` to UPGRADE the live
IDB's already-existing partial `MoveList` type into a complete one,
converting several raw-offset accesses into proper struct-member
references.

## ExecutingScript: FULLY mapped, zero unaccounted bytes -- individual dedicated fields where 2011 later unified into a generic queue

Picked up from an old open lead: `post_script_cleanup`'s own match
evidence (from much earlier in this project) already mentioned an
"index into a 0x6Ch-sized-element array (`dword_4CC848`, matching the
`ExecutingScript` `scripts[]` array)" without pursuing the field-level
detail. Went back and read the body properly.

The function pops the top script off the call stack: `mov esi,
dword_523150 (num_scripts); sub esi,1; imul esi,6Ch; add esi,offset
dword_4CC848` computes a pointer to `scripts[num_scripts-1]`, then a
`rep movsd` with `ecx=0x1B` (27 dwords) bulk-copies **exactly 108
bytes** out of it into a local buffer -- independently confirming the
element stride at `0x6C` (108 bytes), matching the `imul` used
everywhere else in the function.

### Round 1: inst/forked (first/last fields)

- **`inst`** (`ccInstance *`) @ `+0x00`: `dword_4CC848[idx]` (no added
  offset) is passed straight to the already-matched `ccFreeInstance`.
  Matches 2011's declared FIRST field exactly.
- **`forked`** (`char`) @ `+0x68`: `byte_4CC8B0[idx]` -- and
  `0x4CC8B0 - 0x4CC848 = 0x68`, exactly the offset within the same
  108-byte stride -- gates the `ccFreeInstance` call as a boolean.
  Matches 2011's declared LAST field (`acruntim.h:711`) exactly, in both
  *position* (last field) and *semantics* (a forked-instance cleanup
  gate -- `if (forked) ccFreeInstance(inst);` is close to a direct
  transliteration of the disassembly).

**The size drop is the real finding here.** 2011's `ExecutingScript` is
~725 bytes, almost entirely because of `postScriptActions
[MAX_QUEUED_ACTIONS=5]` -- a deferred-action queue whose
`postScriptSaveSlotDescription[5][100]` member alone is 500 bytes. None
of that exists in this 2002 build: the *entire* struct is only 108
bytes, `inst` to `forked` inclusive.

### Round 2: run_another chain, via ExecutingScript::run_another (sub_425500)

`ExecutingScript::run_another` (`Engine/AC.CPP:351-363`) is called from
the already-matched `run_on_event` (`curscript->run_another("#on_event",
...)`), operating on the same 108-byte `scripts[]` struct. Its body maps
cleanly onto the disassembly:

- `if (numanother<MAX_QUEUED_SCRIPTS) numanother++;` matches `cmp
  [this+0x60],2; jge skip; [this+0x60]++` -- **DRIFT**: capacity compared
  against `2` here, not 2011's declared `MAX_QUEUED_SCRIPTS=4`
  (`acruntim.h:698`).
- `thisslot=numanother-1; strcpy(script_run_another[thisslot],namm);`
  matches `imul thisslot,0x1E(30); strcpy([this+thisslot*0x1E+0x14],
  namm)` -- confirms **`script_run_another[2][30]` @ `+0x14`..`+0x50`**
  (60 bytes), inner dimension (30 bytes/entry) exactly matching 2011's
  declared size (`acruntim.h:707`) with zero drift.
- `run_another_p1[thisslot]=p1; run_another_p2[thisslot]=p2;` matches
  `[this+thisslot*4+0x50]=p1; [this+thisslot*4+0x58]=p2` exactly --
  confirms **`run_another_p1[2]` @ `+0x50`..`+0x58`** and
  **`run_another_p2[2]` @ `+0x58`..`+0x60`** (8 bytes each).
- **`numanother`** @ `+0x60` (4 bytes) -- zero slack between
  `run_another_p2`'s end and `numanother`'s start, independently pinning
  the array capacity at 2.

### Round 3: the rest of the middle section, via decoding IDA's own pre-existing local-variable names

`post_script_cleanup` bulk-copies the `scripts[]` slot into a local
stack buffer with `lea edi,[ebp+var_6C]; rep movsd` -- so the buffer
starts at `[ebp+var_6C]`, meaning any local at `[ebp+X]` maps onto the
struct via `buffer_offset = X - (-0x6C)`. IDA had **already** assigned
semantic names to several of these locals, long before this project's
tracking began (`newnum`, `ooo`, `dlgnum` -- not generic `var_NN` names)
-- decoding them via that arithmetic, then confirming each against its
actual usage in the function body (never trust the name alone: a later
grep for `[ebp+ooo]` also turned up an unrelated, coincidental reuse of
the same auto-generated name inside a completely different function
using `"agssave.%03d"` -- always verify by usage and position, not name
matching):

- **`newnum`** (`int`, sentinel -1) @ `+0x04` (`-0x68` in the frame):
  `cmp [newnum],0; jl skip; ...call new_room(newnum,playerchar)` (or, if
  other scripts are still running, deferred onto the *next* slot instead
  via `dword_52314C[+4]=newnum`). Matches 2011's `ePSANewRoom` case
  (`acruntim.h:687`).
- **invscreen flag** (`int`/bool, `+0x08`, `-0x64` in the frame, no
  pre-existing IDA name): `cmp [+0x08],0; jz skip; call sub_41FEA9`,
  where `sub_41FEA9` (not independently renamed -- a single-purpose
  gate, same treatment as other small helpers like `sub_41D7F7` for
  `dialog_bullet`) unconditionally calls the already-matched
  `__actual_invscreen()`. Matches 2011's `ePSAInvScreen` case
  (`acruntim.h:688`).
- **`ooo`** (`int`) @ `+0x0C` (`-0x60` in the frame): compared against
  sentinel `0x3E8` (1000) -- if equal, calls `RestoreGameDialog`;
  else if `>=0`, calls `sub_409A9C` (frees/resets every currently-
  executing script: iterates `dword_4CC848[]` up to `dword_523150`,
  frees each via `sub_42C270`/`sub_42C24C` depending on its `forked`
  flag, then zeroes `dword_523150`) followed by `restore_game_data(ooo,
  nametouse=0)`. Matches 2011's **combined** `ePSARestoreGame`/
  `ePSARestoreGameDialog` pair (`acruntim.h:689-690`) folded into *one*
  field plus a magic sentinel -- 2011 keeps these as two separate enum
  cases.
- **`dlgnum`** (`int`, sentinel -1) @ `+0x10` (`-0x5C` in the frame):
  `cmp [dlgnum],0; jl skip; push dlgnum; call do_conversation(dlgnum)`.
  Matches 2011's `ePSARunDialog` case (`acruntim.h:691`).
- **restart-game flag** (`int`/bool, `+0x64`, `-0x8` in the frame --
  IDA's generic `var_8`, sitting between `numanother` and `forked`): at
  the very end of the function, `cmp [+0x64],0; jz done; call
  sub_409A9C; call RestartGame`. Matches 2011's `ePSARestartGame` case
  (`acruntim.h:693`).

### Cross-confirmation: ExecutingScript::init (sub_424A00)

A separate, previously-unmatched function turned out to be the
constructor-equivalent: exactly 8 field-initialization writes through
`this`, in struct-offset order, matching every field found above with
the sentinel each usage site expects as "none": `[this+0]=0` (inst),
`[this+4]=-1` (newnum), `[this+8]=0` (invscreen flag), `[this+0xC]=-1`
(ooo), `[this+0x10]=-1` (dlgnum), `[this+0x60]=0` (numanother),
`[this+0x64]=0` (restart-game flag), `[this+0x68]=0` (forked). This is
the clinching evidence for the whole struct -- an independent function
touching precisely these 8 offsets, in this exact order, with sentinels
that match every usage site's "none"/default case.

### Architectural finding

2002 gives 5 of 2011's 9 `PostScriptAction` enum cases (`acruntim.h:
686-695`) -- `NewRoom`, `InvScreen`, `RestoreGame`+`RestoreGameDialog`
(combined via sentinel), `RunDialog`, `RestartGame` -- their **own
dedicated struct field**. `SaveGame`, `SaveGameDialog`, and `RunAGSGame`
have **no** dedicated field anywhere in this 108-byte struct, so in this
build they're either handled immediately (not deferred past script
execution) or the concept postdates 2002 entirely. 2011's generic
`postScriptActions[MAX_QUEUED_ACTIONS]` enum+data queue array is
therefore a **later unification** of what were originally independent
fields -- not a reduced/shrunk version of an already-existing queue, the
usual drift pattern seen elsewhere in this project (e.g. `MouseCursor`'s
fixed array sizes). Struct is now complete: `0x00`..`0x6C` (108 bytes),
zero unaccounted bytes.

## Verified SAFE (matches exactly, or is layout-independent)

- **`roomstruct`**: IDB already has this fully labeled -- `walls`,
  `object`, `lookat`, `regions`, 4 pointers = exactly `0x10` = 16 bytes,
  matching `Common/acroom.h:207` perfectly. Already complete, no action
  needed.
- **`block`** = `BITMAP*`: a pointer typedef. Size (4 bytes on x86) doesn't
  depend on `BITMAP`'s internal layout at all, so it's safe regardless of
  whether `BITMAP` itself is fully known. Applied via `apply_structs.py`.
- **`MouseCursor`** (`game.mcurs[]`, `Common/acroom.h:2455`): found
  incidentally while investigating `GameSetupStructBase`'s `hotdot`/
  `hotdotouter` fields, and the best confirmation-to-effort ratio of any
  struct in this project. `pic`(int,+0x00), `hotx`(short,+0x04),
  `hoty`(short,+0x06), `view`(short,+0x08) all independently confirmed
  via already-matched functions (`ChangeCursorGraphic`,
  `ChangeCursorHotspot`, `SetMouseCursor`, `__GetLocationType`), with
  each address delta between them matching its predicted field size
  exactly. `flags`(char,+0x14) confirmed via a not-yet-renamed helper
  (`sub_40D2F4`, called from `SetNextCursorMode`/`SetCursorMode`)
  checking bits `2`/`4`, matching 2011's `MCF_DISABLED=2`/
  `MCF_STANDARD=4` (`acroom.h:2451-2452`) exactly. Only `name[10]`
  (+0x0A) lacks direct evidence -- zero references anywhere in the
  disassembly, included at medium confidence purely because it's boxed
  in with zero slack between the confirmed `view` and `flags` fields.
  Struct stride confirmed at exactly `0x18` (24 bytes) via a consistent
  `imul reg,18h` indexing pattern across every caller. **Matches 2011's
  declared layout with ZERO drift in every field found** -- both size
  and order -- a rare, clean result after so many drifted structs
  elsewhere in this project.

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

## GUIMain: resolving sub_407541 corrects the objs[] array length

`sub_407541` had been an unmatched mystery since the `check_controls`
investigation many rounds ago -- a member function taking one index arg,
bounds-checking it against `[this+0x3C]`, and returning the high 16 bits of
a packed value at `[this+idx*4+0x10C]`. It kept turning up as a callee of
unrelated script API entry points (`SetTextBoxText`, `SetSliderValue`),
which was itself a clue: something that widely-used isn't tied to one
control type.

It's `GUIMain::get_control_type(int)` (`Engine/acgui.cpp:1135`): `if
(indx<0 || indx>=numobjs) return -1; return (objrefptr[indx]>>16)&0xffff;`
-- exact match, and the `>>16 & 0xffff` bit pattern is the same
packed-type-extraction idiom already confirmed via `GUIMain::rebuild_array`
several rounds ago (`(objrefptr[ff]>>16)&0xffff`).

This adds `numobjs@+0x3C` and `objrefptr[]@+0x10C` -- and in the process
**corrects** the original `GUIMain` struct: `objrefptr[]` starts exactly
where `objs[]` was assumed to end (`+0x10C`), meaning the original
`objs[60]` guess (made purely from `(0x184-0x94)/4`, before `objrefptr[]`
was known to exist) silently assumed `objs[]` was the *only* thing filling
that space. It wasn't -- `objs[]` and `objrefptr[]` are separate parallel
`MAX_OBJS_ON_GUI`-sized arrays (`Common/acgui.h:684-685`), and the correct
split is `objs[30]@0x94..0x10C` + `objrefptr[30]@0x10C..0x184`. Notably,
`MAX_OBJS_ON_GUI=30` matches 2011's value exactly (`Common/acgui.h:654`) --
one of the few fixed-capacity constants in this whole project that *hasn't*
drifted.

**Lesson for future struct work:** inferring an array's length purely from
"total struct size minus known start offset" is only safe once you're
confident nothing else occupies the remaining space. Prefer confirming the
far end independently (as this correction did) before trusting that kind
of arithmetic.

## GUIObject base size confirmed generalizable: exactly 0x20 across three classes

Reading `SetTextBoxText`'s body (an already-matched script API function,
not a class method) for its `GUITextBox`-specific field writes turned up
more than expected:

- **`GOBJ_TEXTBOX = 5`** confirmed directly (`cmp eax,5` on the
  `get_control_type` result), matching `Common/acgui.h:659` exactly. Free
  corroboration for the other `GOBJ_*` constants too (`BUTTON=1, LABEL=2,
  INVENTORY=3, SLIDER=4, LISTBOX=6`) since they're declared in the same
  enum-like block.
- **`GUITextBox::text` at `+0x20`** -- matches source's `char text[200];`
  being the class's first own field, declared immediately after the
  `GUIObject` base (`Common/acgui.h:339`).
- Reconfirms `objs[]@+0x94` and `numobjs@+0x3C` yet again (this function
  does its own inline validation rather than delegating to a class method).
- The `strlen > 0xBE` (190) "text too long" bound check turned out NOT to
  be drift at all: `Engine/AC.CPP:19057` has the exact same hardcoded `190`
  in 2011's `TextBox_SetText`, unrelated to `sizeof(text)` in either build
  (`SetLabelText` uses the identical constant too -- it's a shared
  validation limit, not a capacity reflection).

**This means `GUIObject`'s base size is now confirmed at exactly `0x20`
(32 bytes) across three independent classes** -- `GUIButton::text@+0x20`,
`GUIListBox::items[]@+0x20`, and now `GUITextBox::text@+0x20`. Worth
treating as a solid, generalizable fact for any future `GUIObject`-derived
class: its own fields will start at `+0x20`.

## GUITextBox and GUILabel vtables pinned; GUIObject's x/y/wid/hit resolved

`GUITextBox::KeyPress` was found directly: `Engine/acgui.cpp:444`'s
`if (strlen(text) >= 199) return;` compiles to a distinctive
`cmp eax, 0C7h` (199) right after a `strlen` call, and grepping the
disassembly for `0C7h` turned up exactly one hit in that shape --
`sub_406058` (`rob_blanc_1.asm:10708`). Its own `DATA XREF:
.rdata:004AD568` pins it to slot 5 (`+0x14`) of the 9-slot vtable at
`off_4AD554`, which immediately identifies the whole table as
`GUITextBox`'s: slot 6 (`sub_405EAA`) is `Draw`, slot 7 (`sub_405DE1`) is
`WriteToFile`, slot 8 (`sub_405E39`) is `ReadFromFile` -- all confirmed by
reading their bodies against `Engine/acgui.cpp:378-424`.

While mapping that table, its immediate neighbor `off_4AD4E8` (9 slots
earlier in the same `.rdata` vtable cluster, `MouseMove`/`KeyPress` both
using the shared empty-stub) turned out to be **`GUILabel`'s** vtable, not
another `GUITextBox`-shaped table as first suspected -- `GUILabel::KeyPress`
is declared empty in source (`Common/acgui.h:315`, `void KeyPress(int){}`)
so its slot 5 uses the same shared stub as `MouseMove`, unlike
`GUITextBox`'s real override. Slots 7/8 (`sub_4059A7`/`sub_4059FF`) read/
write the exact same `text[200]`+3-int-tail shape as `GUITextBox` -- which
at first looked like a mismatch against 2011's `GUILabel` (`char *text` +
`textBufferLen`, a dynamically-sized field) until checking the 2011 source
directly: it still contains the *old* fixed-array `fwrite` as a dead
commented-out line (`Engine/acgui.cpp:276`) and its `ReadFromFile`
explicitly branches `if (version<113) textBufferLen=200;`
(`Engine/acgui.cpp:289`) -- both confirm this 2002 build predates that
refactor and legitimately still has a fixed `char text[200]` inline array,
not a bug in the match. Slot 6 (`sub_405B28`, `Draw`) was accepted at
medium confidence (positional + a plausible `check_font(&font)` first call)
rather than fully traced statement-by-statement like the other three.

**Bonus generalizable finding**: `GUITextBox::Draw`'s
`wrectangle(x, y, x+wid-1, y+hit-1)` call precisely resolves four of
`GUIObject`'s previously-opaque base fields: `x@+0x08`, `y@+0x0C`,
`wid@+0x10` (independently reconfirmed via `KeyPress`'s
`wgettextwidth(...) > wid-(6+...)` bound check), `hit@+0x14`. Since these
are base-class fields under the C++ ABI's single-inheritance layout, they
apply identically to every `GUIObject`-derived class -- this **corrects**
`GUIButton`'s struct, whose `+0x08..+0x1C` span had been left as opaque
padding pending exactly this kind of evidence.

**Lesson for future struct work:** a `DATA XREF` on an already-identified
function is often a faster, more mechanical path to pinning a whole vtable
than trying to reason about which shared empty-stub pattern "should"
belong to which class -- work backward from one confidently-matched
function's own address reference rather than forward from vtable-slot
shape guessing.

## off_4AD578 misidentified as GUISlider by shape, corrected to GUIListBox

Continuing the vtable sweep, `off_4AD578` looked -- purely from slot shape
(non-empty `MouseMove`/`MouseDown`, empty `KeyPress`) -- like a plausible
match for `GUISlider` (also has non-empty `MouseMove`/`MouseDown`/`MouseUp`,
empty `KeyPress`). Reading the actual body of slot 0 (`sub_424490`) before
committing anything caught the mistake: `GUISlider::MouseMove`
(`Common/acgui.h:252`) starts with `if (mpressed==0) return;` and does
floating-point ratio math against `min`/`max`/`wid`/`hit`; `sub_424490`
does neither -- it's just `[this+0x1BC]=arg_0-[this+8];
[this+0x1C0]=arg_4-[this+0xC];`, an exact byte-for-byte match for
`GUIListBox::MouseMove`'s `mousexp=nx-x; mouseyp=ny-y;`
(`Common/acgui.h:408-409`) instead. `GUISlider` itself remains
unidentified -- still an open lead for a future round.

This was independently confirmed (not just plausible-looking) by
`GUIListBox::WriteToFile`'s 44-byte bulk `fwrite` at `+0x1B0`: interpreting
it as the 11-int field block source declares at `acgui.h:388-390`
(`numItems, selected, topItem, mousexp, mouseyp, rowheight,
num_items_fit, font, textcol, backcol, exflags`) places `mousexp`/`mouseyp`
at exactly `+0x1BC`/`+0x1C0` -- matching `MouseMove`'s offsets exactly,
via a completely independent piece of evidence. Full derived block:
`numItems@0x1B0, selected@0x1B4, topItem@0x1B8, mousexp@0x1BC,
mouseyp@0x1C0, rowheight@0x1C4, num_items_fit@0x1C8, font@0x1CC,
textcol@0x1D0, backcol@0x1D4, exflags@0x1D8`. Drift: 2011's three trailing
fields (`selectedbgcol, alignment, reserved1`) aren't part of this bulk
write.

`off_4AD50C` was pinned as `GUIInv` the same way: `MouseOver`/`MouseLeave`/
`MouseUp` all read or write a single field at `+0x20` (`isover`, GUIInv's
own first declared field, landing right after the GUIObject base per the
now-familiar `+0x20` pattern), and `MouseUp`'s `if (isover) activated=1;`
reconfirms `activated@+0x1C` for a fifth class. Its `WriteToFile`/
`ReadFromFile` turned out to touch *only* the GUIObject base block --
no `charId`/`itemWidth`/`itemHeight`/`topIndex` at all -- which lines up
with 2011's own version-gated fallback (`if (version>=109) {...} else
{charId=-1; ...}`, `acgui.h:486-497`): this build simply predates that
format version, so this is expected absence, not a missed field.

**Lesson reinforced:** don't commit a vtable-to-class mapping on slot-shape
resemblance alone, even when it looks clean -- always read at least the
one distinguishing method's actual body first. The GUISlider mixup would
have silently mislabeled an entire class's vtable and fields if the
`mousexp`/`mouseyp` body hadn't been checked before writing anything down.

## GUISlider finally found: it was the table sitting right between GUIInv and GUITextBox

After the `off_4AD578` correction above, `GUISlider` was still unaccounted
for. Re-scanning the `.rdata` vtable cluster's labels caught the table at
`off_4AD530` -- sitting directly BETWEEN the already-pinned `GUIInv`
(`off_4AD50C`) and `GUITextBox` (`off_4AD554`) tables -- which had been
skipped over during the earlier sweep. Its shape (real `MouseMove`/
`MouseDown`/`MouseUp`, empty `MouseOver`/`MouseLeave`/`KeyPress`) matches
`GUISlider` exactly, and this time the bodies were checked before
committing anything: `mpressed@+0x2C` is confirmed three independent
ways -- `MouseDown` sets it 1, `MouseUp` clears it 0, `MouseMove` guards
`if (mpressed==0) return;` before its drag-ratio floating-point math
(`fild`/`fidiv`/`fimul`/`_ftol`) -- and `WriteToFile`'s 16-byte bulk
`fwrite` at `+0x20` (covering `min@0x20, max@0x24, value@0x28,
mpressed@0x2C`) agrees with all three exactly. Drift: 2011's three
trailing fields (`handlepic, handleoffset, bgimage`) aren't part of this
build's bulk write, same pattern as every other `GUIObject`-derived class
found so far.

**This completes the `GUIObject` hierarchy** -- all six derived classes
(`GUIButton`, `GUIListBox`, `GUITextBox`, `GUILabel`, `GUIInv`,
`GUISlider`) now have their full 9-slot vtables identified and a struct
recovered from real disassembly evidence, plus the shared `GUIObject` base
layout (`x/y/wid/hit/activated`, own-fields-start-at-`+0x20`) confirmed
across all of them.

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

(Note: this "Takeaway" was written early in the project, during the
GUIButton vtable round. It reads as overly pessimistic in hindsight --
`GameSetupStructBase`, `DialogTopic`, `MoveList`, and `ViewStruct272`
(below) all turned out to be efficient, high-yield investigations once
the "follow an already-matched caller's actual code" technique matured.
Left in place as a historical snapshot, not current guidance.)

## ViewStruct272: this build's per-view animation data -- an "over-determined fit" across three independent strides, with a genuine architectural drift (no per-loop `loopflags[]`)

Surveyed for the next struct after `MoveList` closed out. `numviews`
(`GameSetupStructBase`, already confirmed) pointed at an obvious next
target: the animation "views" data itself, 2011's old-format ancestor
being the already-old `ViewStruct272` (`Common/acroom.h:2421-2448`,
itself superseded by a modern pointer-based `ViewStruct`/`ViewLoopNew`
by 2011 -- the same "ancestor struct preserved for old-save
compatibility" pattern as `OriGameSetupStruct`, though here it's the
build's *live* runtime format, not just a save-compat relic).

**The global itself**: `load_ac2game_dta` (already matched) does `imul
eax, ElementCount(numviews), 8D4h; add eax, 14h; call malloc; ...;
mov dword_52313C, eax; ...; fread(dword_52313C, 8D4h, numviews,
stream)` -- confirming `dword_52313C` = `views`, a genuine standalone
global pointer (dynamically allocated, NOT embedded in
`GameSetupStructBase`, mirroring the earlier `dialog`/`DialogTopic`
discovery), with a confirmed per-view stride of `0x8D4` (2260 bytes).
Computing 2011's full `ViewStruct272` size (16 loops x 20 frames of
28-byte `ViewFrame` each, plus headers) gives roughly 9060 bytes --
meaning this 2002 build's format is *even more reduced* than the
already-old 2011 ancestor it's named after.

**Cracking the internal layout** took reading through the 60+ usage
sites of `dword_52313C` scattered across the binary. The first
productive cluster was inside an unmatched function, `sub_40C3E0`
(called from another unmatched function, `sub_40C75E`): a record from a
DIFFERENT global, `dword_4E45C8` (stride `0x20`/32 bytes -- at the time
guessed to be "a per-character runtime animation-state array, NOT the
same as `CharacterInfo`/`chars`"; **CORRECTED in the very next round,
see the `RoomObject` section below** -- it's actually `croom->obj`, the
current room's `RoomObject` array, completely unrelated to characters;
IDA's own `chat`/`chaa` loop-variable names were misleading, not real
evidence) supplied a view number and loop number, combined via `imul
eax, 8D4h` (view stride, already known) then `imul eax, 118h` -- the
first sighting of a SECOND stride, `0x118` (280 bytes), applied
per-loop within a view.

That per-loop stride was independently corroborated TWICE more, both
inside the already-matched `update_stuff` (the per-frame main-loop
update function -- the same function `MouseCursor`/`ExecutingScript`
evidence came from in earlier rounds). Two clean, complementary
patterns emerged:

1. **Frame-advance check**: `viewbase = views + curview*0x8D4; cmp
   curframe, word[viewbase + curloop*2 + 2]; jl <keep-animating>` --
   reading `numframes[curloop]` to decide whether to advance to the
   next frame or move on to the next loop. This confirms a `short
   numframes[]` array starting at view-relative `+0x02`, 2-byte stride
   per loop -- matching 2011's exact declared field ORDER (`short
   numloops; short numframes[16];`, `acroom.h:2421-2422`) even though
   the reference to `numloops` itself (at `+0x00`) is only a positional
   inference (boxed in with zero slack, no direct read found).

2. **Loop-mirroring fallback**: when the frame-advance check fails
   (`curframe >= numframes[curloop]`), a second path reads the SAME
   `numframes[]` array one loop index further back -- `word[viewbase +
   curloop*2]` (no `+2`) is exactly `numframes[curloop-1]` -- then
   computes that PREVIOUS loop's frame-block address (`(curloop-1)*
   0x118 + 0x14`) and checks its LAST frame's first field against the
   `-1` sentinel: `cmp dword[loopbase + (numframes[curloop-1]-1)*0x1Ch],
   0FFFFFFFFh`. This is the classic AGS "a loop with no frames of its
   own mirrors the previous loop" behavior. It independently confirms
   THREE things at once: the `0x118` per-loop stride (corroborating
   `sub_40C3E0`'s earlier finding), a `0x1C`(28-byte) per-frame stride
   (via several `imul reg, 1Ch` sites sitting right alongside, matching
   2011's declared 28-byte `ViewFrame` size), and that the frame's
   FIRST field (`pic`, a 4-byte int/sprite-index) uses `-1` as an
   "unused frame slot" sentinel -- matching 2011's own `pic==-1`
   convention (`acroom.h:2270`-adjacent usage in `AC.CPP`) exactly.

**The result is an "over-determined fit"**: three independently-found
strides -- total view size (`0x8D4`, from `load_ac2game_dta`'s
malloc/fread), per-loop block size (`0x118`, from three separate
disassembly sites), and per-frame size (`0x1C`, from multiple `imul`
sites) -- all reconcile with ZERO slack simultaneously: `0x14`(header)
`+ 8*0x118`(loop blocks) `= 0x8D4` exactly, and `0x118 / 0x1C = 10`
frames per loop exactly. No other loop-count/frame-count combination
closes all three at once, which is strong evidence even without a
direct "loop count" or "frames per loop" literal constant found
anywhere.

**Confirmed layout**: `short numloops`(MEDIUM, positional -- **upgraded
to HIGH in the next round**, see the `RoomObject` section below, where
`SetObjectView` was found reading it directly) @ `+0x00`,
`short numframes[8]`(HIGH) @ `+0x02`, 2-byte alignment pad, then
`ViewFrame272 frames[8][10]` @ `+0x14` through the confirmed end
`+0x8D4`. Only `ViewFrame272.pic` @ `+0x00` has independent access-site
evidence (the `-1` sentinel check above); the remaining `ViewFrame272`
fields (`xoffs`/`yoffs`/`speed`/`flags`/`sound`/`reserved_for_future[2]`)
are carried over from 2011's declared `ViewFrame` layout
(`acroom.h:2268-2291`) as an unverified structural assumption, included
only to give the confirmed 28-byte stride named fields instead of
opaque padding -- flagged accordingly in `apply_structs.py`, not to be
cited as confirmed without new evidence.

**DRIFT, and a genuine architectural finding**: capacity is 8 loops x
10 frames/loop (80 total frame slots) here, vs. 2011's declared 16
loops x 20 frames (320 slots) -- a 4x reduction, consistent with this
project's repeated "smaller fixed capacity" pattern. More interesting:
the header ends immediately after `numframes[8]` (`+0x02 + 0x10 =
+0x12`, +2 pad = `+0x14`, landing exactly on the loop-block start with
zero slack) -- leaving NO ROOM for 2011's declared `int
loopflags[MAXLOOPSPERVIEW]` (`acroom.h:2423`) as a separate array.
Per-loop flags (e.g. `LOOPFLAG_RUNNEXTLOOP`) are CONFIRMED ABSENT from
this build's per-view header, not merely unfound -- consistent with the
project's other "later AGS feature, confirmed absent" findings
(`numcursors`, `default_lipsync_frame`, `invhotdotsprite`,
`DialogTopic.topicFlags`).

Two functions remain unmatched but now documented for the struct-field
evidence they contributed: `sub_40C3E0` (first sighting of the `0x118`
per-loop stride) and its caller `sub_40C75E` (not yet individually
examined). `dword_4E45C8` (stride `0x20`, holding view/loop/frame/
counter/flag fields at `+0x10`/`+0x12`/`+0x14`/`+0x16`/`+0x1A`) was
flagged as a promising lead for a future round -- that round happened
immediately next, see below.

## RoomObject: correcting a misattribution -- `dword_4E45C8` is `croom->obj`, not a character array

Picked up the `dword_4E45C8` lead flagged at the end of the
`ViewStruct272` round above. The first step, tracing `dword_4E45C8`'s
own single write site, immediately overturned the earlier "per-character"
guess: `load_new_room` (already matched) does

```
mov eax, roomstats
add eax, newnum*1390h      ; sizeof(RoomStatus)
mov dword_523128, eax      ; (or, for an out-of-range room number,
                            ;  a fixed fallback: dword_523128 =
                            ;  offset dword_4EF3A0)
...
mov eax, dword_523128
add eax, 8
mov dword_4E45C8, eax
```

`roomstats` is already a confirmed global -- `RoomStatus *roomstats`
(`Engine/AC.CPP:493`) -- so `dword_523128 = &roomstats[newnum]`,
matching 2011's `croom=&roomstats[newnum]` (`AC.CPP:4259`) exactly.
`RoomStatus`'s own declared field order (`Common/acruntim.h:94-97`) is
`int beenhere; int numobj; RoomObject obj[MAX_INIT_SPR];` -- meaning
`dword_4E45C8 = dword_523128 + 8` is simply `&roomstats[newnum].obj[0]`.
**`dword_4E45C8` is `croom->obj` -- AGS's `objs[]` room-object array --
nothing to do with characters at all.** The `chat`/`chaa` loop-variable
names IDA had assigned around its use sites (which drove the original
"per-character" guess) turn out to be generic placeholder names, not a
genuine hint.

The SAME `load_new_room` block independently confirms `RoomStatus`'s
two leading fields: `beenhere`@`+0x00` (`cmp dword ptr[croom],0`, an
"already visited, don't re-run first-time-enters-room setup" gate) and
`numobj`@`+0x04` (set from a default count, then used as the loop bound
for a per-object initialization loop) -- both matching 2011's declared
order with zero drift. `RoomStatus` itself is not being formalized as
its own `apply_structs.py` struct yet -- its total size and `obj[]`'s
actual capacity in this build (2011 declares `MAX_INIT_SPR=40`,
`acroom.h:59`, not verified here) haven't been independently confirmed,
and this project's own discipline says don't guess a capacity without
evidence. Only the two leading fields plus `obj[]`'s start address are
confirmed.

**`RoomObject` itself, however, closed out completely**, all in
already-matched functions:

- **`x`@`+0x00`, `y`@`+0x04`** (both `int`): confirmed via `GetObjectAt`
  (already matched, script-exported `GetObjectAt(int xx,int yy)`),
  read directly as the object's on-screen position. Matches 2011's
  declared first two fields (`acruntim.h:39`) exactly.
- **`num`@`+0x0C`** (`short`, sprite slot number): confirmed THREE
  independent ways -- `GetObjectAt` uses it as an index into
  `dword_4E787C[]` (a sprite-dimension lookup table) to compute the
  object's bounding box; `SetObjectView` and `SetObjectFrame` (both
  already matched) each set it from the low 16 bits of the relevant
  `ViewFrame272.pic` field right after changing the object's view/loop/
  frame -- the same "narrow read" pattern seen with `InventoryItemInfo`
  earlier in this project. Matches 2011's declared field
  (`acruntim.h:46`) exactly.
- **`baseline`@`+0x0E`** (`short`): confirmed via `GetObjectAt`: `cmp
  var,1; jge <use-as-is>; else var=y` -- an exact match to 2011's
  `RoomObject::get_baseline()` ("if (baseline<1) return y; return
  baseline;", `acruntim.h:66-70`), both in logic shape and semantic
  role.
- **`view`@`+0x10`, `loop`@`+0x12`, `frame`@`+0x14`**: confirmed via
  `SetObjectView`/`SetObjectFrame` (both already matched), which set
  them directly from their `viw`/`vii`/`lop`/`fra` parameters. Crucially,
  `SetObjectView` also reads `loop`@`+0x12` back afterward and compares
  it *directly* against `ViewStruct272.numloops` (`views[vii]`, offset
  `+0x00`, no added offset) -- the direct read that upgrades
  `ViewStruct272.numloops` from MEDIUM (positional-only) to HIGH
  confidence, resolving the one open question left over from the
  previous round.
- **`wait`@`+0x16`, `moving`@`+0x18`**: confirmed via `update_stuff`'s
  frame-advance/loop-mirroring logic (see the `ViewStruct272` section
  above -- now correctly understood to be operating on `RoomObject`
  records, not characters) for `wait`, and, even more conclusively for
  `moving`, `update_stuff` passing `&obj[+0x18]` *by address* to
  `do_movelist_move` -- matching 2011's own call shape exactly:
  `do_movelist_move(&objs[aa].moving,&objs[aa].x,&objs[aa].y)`
  (`Engine/AC.CPP:6438`). This one call site independently nails down
  BOTH the field identity (`moving`) AND the global's own identity
  (`objs[]`) at once. (This build's actual call site only pushes the
  one `mlnum` pointer, not all three from 2011's declared signature --
  a reduced calling convention, not investigated further this round.)
- **`cycling`@`+0x1A`** (`char`): confirmed via `SetObjectView` and
  `SetObjectFrame`, both of which clear it to 0 immediately after
  changing the object's view/frame -- matching 2011's "is it currently
  animating?" role (explicitly setting a frame stops any in-progress
  animation).
- **`on`@`+0x1C`, `flags`@`+0x1D`** (both `char`): confirmed via
  `GetObjectAt`'s hit-test gate, `cmp byte[obj+1Ch],1; jz <continue>`
  then `and byte[obj+1Dh],1; jz <continue>` -- an object must be
  visible (`on==1`) AND not have the interaction-disabling bit set in
  `flags` to be clickable. Matches 2011's declared fields
  (`acruntim.h:52-53`) in position and semantic role exactly.
- **`transparent`@`+0x08`** (`int`): MEDIUM confidence -- a real access
  site exists (`load_new_room`'s per-object initialization loop copies
  a value from the compiled room file's own object-data table,
  `word_51FF5C[chaa*0xA]`, into this offset), but the field's semantic
  *role* is inferred from 2011's declared field order rather than
  independently confirmed via a script-API read/write (no
  `SetObjectTransparency`/`GetObjectTransparency` disassembly examined
  this round).
- **`overall_speed`@`+0x1B`** (`char`): MEDIUM confidence, boxed in
  with zero slack between the confirmed `cycling` and `on` fields, no
  independent access site found.

Total size `0x20` (32 bytes) is high confidence, confirmed by the
consistent `shl reg,5` stride scaling used by every function above.
Field order matches 2011's declared `RoomObject` (`acruntim.h:38-54`)
**exactly**, with 2011's `transparent`-adjacent tint fields
(`tint_r/g/b/level/light`), `last_zoom`, `last_width`/`last_height`,
and trailing `blocking_width`/`blocking_height` all CONFIRMED ABSENT
from this build (the header runs straight from `y`@`+0x04` to
`transparent`@`+0x08` to `num`@`+0x0C` with no gap for the tint/zoom
fields, and the struct ends immediately after `flags`@`+0x1D` with no
room for the trailing blocking-box fields) -- consistent with this
project's repeated "later AGS feature, confirmed absent" pattern, just
applied to a script-visible object struct instead of an internal one
this time.

Also documented in passing: `sub_4256E0`, an unmatched helper called
from both `SetObjectView` and `SetObjectFrame` to validate an object
number (`return arg_0<0 || arg_0>=croom->numobj`), which independently
cross-confirms `RoomStatus.numobj`@`+0x04`.

**Methodology note worth repeating**: this whole correction happened
because a loop-index variable IDA itself had named `chat`/`chaa`
*looked* like it meant "character," and that assumption went
unchallenged for a full round of struct work before the array's actual
allocation/pointer-chain evidence was traced back to its source. The
fix, as always in this project, was to follow the pointer to its real
origin (`load_new_room`'s `croom=&roomstats[newnum]` assignment) rather
than trust a suggestive-but-unverified name.

## RoomObject: closing the last two MEDIUM-confidence fields, and an unexpected ViewFrame272 bonus

Went back to `RoomObject`'s two remaining non-HIGH fields
(`transparent`@`+0x08`, `overall_speed`@`+0x1B`) to see if a dedicated
script-API function would confirm them directly, the same way
`GetObjectAt`/`SetObjectView`/`SetObjectFrame` had closed out
everything else. Both paid off immediately.

**`SetObjectTransparency`** (already matched, script-exported) is an
exact instruction-for-instruction match to 2011's implementation
(`Engine/AC.CPP:14679-14688`): `trans==0` sets the field to `0`,
`trans==100` sets it to `255`, otherwise `((100-trans)*25)/10` -- all
three branches target `dword_4E45C8[obn*0x20+8]`, i.e.
`RoomObject.transparent`. This upgrades it from MEDIUM (a real access
site existed, but only from `load_new_room`'s room-file initialization,
with the field's *role* inferred rather than confirmed) to HIGH.

**`AnimateObject`** (already matched, script-exported,
`AnimateObject(int obn,int loopn,int spdd,int rept)`) turned out to be
a small goldmine, confirming or reconfirming FIVE fields in one pass:

- `.cycling`@`+0x1A` = `rept+1` -- not just "cleared to 0" as the
  `SetObjectView`/`SetObjectFrame` evidence alone suggested, but the
  fuller picture: cycling holds `0` when idle and `repeat-count+1` while
  animating, matching 2011's exact encoding convention.
- `.loop`@`+0x12` = `loopn`, `.frame`@`+0x14` = `0` -- reconfirms the
  existing evidence via a third independent function.
- `.overall_speed`@`+0x1B` = `(char)spdd` -- direct confirmation,
  upgrading it from MEDIUM (boxed-in-only) to HIGH.
- `.wait`@`+0x16` = `spdd + views[view].loops[loopn].frames[0].speed` --
  this is the interesting one: it's simultaneously the confirmation for
  `RoomObject.wait` (already HIGH) AND the FIRST independent access-site
  evidence for `ViewFrame272.speed`@`+0x08` (loop-block-relative
  `+0x08`, i.e. frame 0's own `+0x08`), which had been sitting at
  UNCONFIRMED/borrowed-from-2011-only status since the original
  `ViewStruct272` round. Upgraded to HIGH.

With this round, **every field of `RoomObject` is now HIGH confidence**
-- `x`, `y`, `transparent`, `num`, `baseline`, `view`, `loop`, `frame`,
`wait`, `moving`, `cycling`, `overall_speed`, `on`, `flags`, spanning
`+0x00` through `+0x1D` with the natural 2-byte pad closing out `0x20`.
It joins `MouseCursor`/`InventoryItemInfo`/`MoveList` as one of the
project's fully-confirmed, zero-guesswork structs.

One more thread surfaced but not pulled this round: `AnimateObject`
branches to the already-matched `animate_character` when `obn>=99`
(this build's convention for aliasing object/character ID ranges), and
is itself called from `sub_40C3E0` at `+0x1E7` -- the same still-
unmatched function that first revealed `ViewStruct272`'s `0x118`
per-loop stride. `sub_40C3E0` calling a script-exported API function
directly (rather than through the bytecode interpreter) is an
interesting shape worth investigating for its own identity in a future
round.

## RoomObject.flags bit values, and a shelving decision for ViewFrame272's remaining fields

Continued past the previous round's HIGH-confidence sweep to look for
the last two things worth chasing: real bit-level meaning for
`RoomObject.flags` (so far just "some bits gate some behavior"), and
any evidence at all for `ViewFrame272`'s still-unconfirmed
`xoffs`/`yoffs`/`flags`/`sound` fields.

**`RoomObject.flags` now has two confirmed bit values.** Reading
`prepare_characters_for_drawing` (already matched) in full revealed
that -- despite its name -- it ALSO prepares ROOM OBJECTS for drawing,
not just characters: it loops over `dword_523128[+4]`
(`RoomStatus.numobj`) indexing the confirmed `dword_4E45C8`
(`RoomObject[]`) array, gated by `.on`@`+0x1C`. Partway through, `and
edx,2` against `.flags`@`+0x1D` selects between two different draw
paths (a walk-behind-aware pixel-masking sort, `sub_410631`/
`sub_410C6A`, vs. a simpler direct blit) -- matching 2011's declared
`OBJF_NOWALKBEHINDS`(`2`, `Common/acroom.h:793`) exactly. Combined with
`GetObjectAt`'s existing bit-0 check (`OBJF_NOINTERACT`, `1`,
`acroom.h:792`), `flags` now carries two independently confirmed bit
meanings instead of just "an opaque interactability-adjacent byte."
(The same function also re-confirms the `get_baseline()` fallback
pattern -- "if `baseline`<1, use `y` instead" -- via a second, separate
call site from `GetObjectAt`'s original one.)

**`ViewFrame272`'s `speed`@`+0x08` picked up a THIRD independent
confirmation**, from an unexpected direction: `GetLocationType`
(already matched) contains the mouse cursor's own idle-animation logic
-- gated by a `MouseCursor.flags` bit (`byte_515870[cur_cursor*0x18] &
8`, "is this cursor animated"), it does `wait = frames[frame].speed +
5` when advancing to the next frame, then reloads the sprite via
`SpriteCache::operator[]`. This is useful beyond just re-confirming
`speed` -- it proves `ViewStruct272`/`ViewFrame272` is a genuinely
SHARED animation-data format used by mouse cursors, room objects, AND
characters alike (via the identical `0x8D4`/`0x118`/`0x1C` stride math
applied to `MouseCursor.view`, with loop fixed at 0), not something
room-object-specific.

**`xoffs`/`yoffs`/`flags`(frame-mirroring)/`sound` remain
unconfirmed.** Every already-matched frame-consuming function examined
across this and the previous two rounds --
`prepare_characters_for_drawing`, `AnimateObject`,
`SetObjectView`/`SetObjectFrame`, `update_stuff`, and now
`GetLocationType`'s cursor code -- touches only `pic` and/or `speed`.
2011's `VFLG_FLIPSPRITE` mirroring check (`views[...].frames[...].flags
& VFLG_FLIPSPRITE`) lives inside `AC.CPP`'s object/character sprite
compositing code, which in the 2011 source is entangled with `tint`/
`objcache`/hardware-acceleration logic that this build's `RoomObject`
has already been shown to lack entirely (no tint fields at all) --
strongly suggesting this build's actual draw-compositing code is
differently shaped and not a profitable place to keep searching by
analogy with 2011. Shelving these four fields at the same status as
`InterfaceElement`'s remaining fields: real, but not yet reachable from
any already-matched caller. Revisit if a new lead (a mirrored-sprite
draw path, or a frame-linked sound-effect trigger) surfaces some other
way.

## EventBlockCmd: characterizing sub_40C3E0/sub_40C75E -- a pre-NewInteraction command-list format, no 2011 source to anchor to

Pivoted away from `ViewStruct272`'s exhausted remaining fields to chase
a lead flagged twice already: `sub_40C3E0`, the still-unmatched
function that first revealed `ViewStruct272`'s `0x118` per-loop stride
and later turned out to call `AnimateObject`. Reading it in full (and
its sole caller, `sub_40C75E`) turned into a genuinely different kind
of investigation than usual for this project: there is no living 2011
source AND no dead-commented declaration to anchor either function to
-- the first time that's been true for something this substantial.

**The shape, established solidly**: `sub_40C75E(list, startIndex)`
loops `for (i=startIndex; i<list[+0xF0]; i++) sub_40C3E0(&list[i*0x18])`
-- an outer loop over a command list with a confirmed `0x18`(24-byte)
per-record stride. `sub_40C3E0` is the per-record dispatcher: it
decodes a `target` field (`+0x0C`) using this build's now-familiar
object/character selector convention (`<10`=room object, `==99`=player
character, `>=100`=character index+100 -- the same convention seen in
`AnimateObject`'s `obn>=99` branch to `animate_character`), then
switches on a `type` byte (`+0x14`): `0` is an explicit error
("!undefined animation command"), `1` routes to `SetObjectView` or
`SetCharacterView` depending on what `target` resolved to, `2` routes
to `AnimateObject` or an inline animate-character equivalent (packing a
value into `CharacterInfo+0x3E`, optionally blocking via
`do_main_cycle` if a secondary flag byte at `+0x15` is set). Further
`type` values reach `move_object` and `walk_character` per their own
`CODE XREF` comments, but weren't individually read this round.

**Tracing the caller chain the rest of the way** was the interesting
part: `sub_40C75E` is called from `run_event_block` (already matched)
at exactly one call site, `+0x548`. `run_event_block`'s own evidence
(from an earlier round) already documents its `EventBlock.respond[i]`
switch handling values `1`, `2` (`StopMoving`), `3` (`run_on_event`),
and `5` -- notably skipping `4`. The natural hypothesis is that
`sub_40C75E` is exactly what fires for `respond[i]==4`, giving
`EventBlock` a way to attach a richer "do several
move/animate/set-view things" command list to a single response slot,
rather than the simple fixed actions the other respond values trigger.
This is a strong, coherent hypothesis but **not independently confirmed
this round** -- it would take reading `run_event_block`'s own
disassembly around `+0x548` directly (not yet done) to nail down the
exact `respond` value.

**Why no 2011 match exists**: `run_event_block` itself was matched
earlier via a *dead, commented-out* prototype declaration -- "the whole
EventBlock/interaction-scripts system was replaced" by 2011. Whatever
`sub_40C75E`/`sub_40C3E0` process didn't survive even as a dead
comment. The command *set* they dispatch to (set view, animate, move
object, move character, set/release character view) maps closely onto
2011's later `NewInteractionCommand`/`run_interaction_commandlist`
(`Common/acroom.h:600`, `Engine/AC.CPP:21449` -- cases 14/17/18/19/27/28
cover almost exactly the same actions), which is a reasonable thing to
notice given AGS's later interaction system is a documented descendant
of the EventBlock-era one. But the STRUCT LAYOUT doesn't match at all:
2011's `NewInteractionCommand` inherits from a virtual base
(`NewInteractionAction`, giving it a vtable pointer) and carries 5 typed
`data[]` slots (each a 12-byte `NewInteractionValue`), totaling 70+
bytes -- nothing close to this build's flat 24-byte POD record. The
most defensible reading is that this build's format is a genuine,
simpler ANCESTOR of `NewInteractionCommand` from before the
`NewInteractionAction` virtual-base refactor, not a corrupted or
misread version of the 2011 struct.

**Naming decision**: given neither function has ANY 2011 source to
derive a name from (living or dead), both are documented in
`matches.json` with `new_name: None` rather than forcing an invented
name that would misleadingly imply a real correspondence. The new
struct, however, gets a project-assigned name --
**`EventBlockCmd`** -- clearly flagged in its own comment as
project-invented, not source-derived, since a completely nameless
struct in `apply_structs.py` would be less useful going forward than an
honestly-labeled provisional one. Several of its fields are MEDIUM
confidence (single-`type`-case evidence only, e.g. `data1`/`data2`'s
roles are inferred from just the `type==1`/`type==2` paths, not
cross-checked against the `move_object`/`walk_character`-routing type
values), and `+0x00` (4 bytes) has no observed access site at all.
Treated the same way as `InterfaceElement`'s shelved remainder: a real,
partially-mapped lead, explicitly left open rather than forced closed.
This is a good next thread to pull on -- reading `sub_40C3E0`'s
remaining `type` branches (the `move_object`/`walk_character` routes)
would very likely fill in `data1`/`data2`'s roles more completely and
might turn up the mysterious `+0x00` field's purpose too.

## EventBlockCmd resolved completely: reading the remaining `type` branches closed every open question

Continued straight on from the previous round's provisional
`EventBlockCmd` writeup by reading the rest of `sub_40C3E0`'s body --
the `type` branches beyond 1 and 2 that hadn't been examined yet. This
closed out every open question from that round in one pass.

**The full `type` enum, 0 through 5**:
- `0`: explicit error, `"!undefined animation command"`.
- `1`: `SetObjectView` (object target) or, for a character target,
  `SetCharacterView`/`ReleaseCharacterView` UNIFIED into a single type
  -- `data2==0` means "release", nonzero means "set to this view". 2011
  later SPLIT this into two separate command types (`case 27`/`28`).
- `2`: `AnimateObject` (object) or an inline animate-character
  equivalent (character, packing a value into `CharacterInfo+0x3E`).
- `3`/`4`: both route to `move_object`/`walk_character`, differing only
  in the `ignwal` flag (`3`=respect walls, `4`="move direct") -- again
  one flag on a shared type where 2011 later has a separate
  `MoveObjectDirect`-style distinction.
- `5`: set the target's `x`/`y` directly with no movement at all --
  writes straight into `RoomObject.x`/`.y` (already HIGH confidence)
  for an object, or `CharacterInfo+0x14`/`+0x18` (already independently
  confirmed as `x`/`y`) for a character. A clean THIRD confirming
  caller for both.
- Anything else: a SECOND, distinct error, `"unknown animation
  encountered"` -- proving the switch is exhaustive and there is
  nothing left to find in this dimension.

**This resolved the two things left open last round.** The mystery
`+0x00` field turned out to be `data0`, the move/set-position target X
coordinate (used only by types 3/4/5, hence invisible in the type-1/2
evidence examined previously). And `+0x04` -- tentatively called
`flags` last round because only a single bit had been observed -- is
actually the SAME kind of generic reusable slot: a full Y-coordinate
int for types 3/4/5, but reinterpreted as a single repeat-flag bit for
types 1/2. That dual role is itself good evidence for how this struct
works: it's a flat set of 4 generic 4-byte argument slots
(`data0`/`data1`/`data2`/`data3`) whose meaning is entirely
`type`-defined, precisely mirroring 2011's `NewInteractionCommand.data[
MAX_ACTION_ARGS]`/`IPARAM1`-`IPARAM5` convention -- just without the
12-byte `NewInteractionValue` wrapper or the vtable, both later
additions.

With every byte from `+0x00` through `+0x15` now positively identified
(and `+0x16`..`+0x18` a natural trailing pad), `EventBlockCmd` is
promoted from "provisional, partially explored" to essentially
complete -- as solid as any struct in this project despite having zero
2011 source to lean on. `sub_40C3E0`/`sub_40C75E` remain intentionally
unnamed (no 2011 identifier exists for either), but are now among the
most thoroughly documented *unnamed* functions in the whole codebase.

One thread still open: confirming that `sub_40C75E` genuinely fires on
`EventBlock.respond[i]==4` specifically (a hypothesis from last round,
not yet checked against `run_event_block`'s own disassembly around
`+0x548`).

## GameAnimation: confirming EventBlock.respond[i]==4 uncovers a whole undocumented game resource

Went back to the one thread left dangling from the `EventBlockCmd`
rounds: does `sub_40C75E` really fire for `EventBlock.respond[i]==4`?
Read `run_event_block`'s own disassembly directly around its call to
`sub_40C75E` (rather than continuing to infer from the outside) and
found much more than expected.

**The hypothesis is confirmed directly**: `cmp dword ptr
[respond_base+i*4+0x20], 4; jnz <next-check>` gates the whole branch --
an exact, literal `respond[i]==4` check sitting right before the call.
But the branch itself does a lot more than just call `sub_40C75E`
blind. It:

1. Bounds-checks `EventBlock.data[i]` against `0Ah`(10), erroring
   `"!run_animate: undefined animation was r[un]"` if out of range --
   giving a hard capacity of 10 for whatever `data[i]` indexes into.
2. Checks a parallel global table, `dword_52033C[data[i]*0xF4]`, for
   nonzero, erroring `"!Run_animate: empty animation was run"`
   otherwise -- a "this slot hasn't been populated" gate. `dword_52033C`
   itself is confirmed to have a `0xF4`(244-byte) stride by this access
   alone; its own per-slot contents weren't explored further this
   round.
3. Finally calls `sub_40C75E(&unk_52024C[data[i]*0xF4], 0)`.

**`unk_52024C` is a whole table of `EventBlockCmd` LISTS** -- this
build's entirely undocumented (in 2011) "Animations" resource system.
Old AGS Editor versions had a distinct "Animations" entry in the
project resource tree (separate from Views, Characters, Rooms) letting
the game author define a reusable, room-independent sequence of
move/animate/set-view commands, triggerable from ANY interaction's "Run
Animation" response. That feature is long gone by the 2011 reference
build -- consistent with `run_event_block`'s own note that "the whole
EventBlock/interaction-scripts system was replaced" extending to this
resource type specifically, not just the command format itself.

**The per-slot struct, formalized as `GameAnimation`**, closes with an
unusually clean confirmation: `sub_40C75E`'s own already-established
"list[+0xF0] = numCommands" behavior (from the `EventBlockCmd` rounds)
applied to `unk_52024C[slot]` means `EventBlockCmd command[10]`
(`10*0x18=0xF0`) plus a trailing `int numCommands` lands EXACTLY on the
`0xF4` stride independently confirmed by `run_event_block`'s own
`data[i]*0xF4` scaling -- two completely separate pieces of evidence
(one from the caller's indexing math, one from the callee's own loop
bound) landing on the same total with zero slack.

This closes out the entire `sub_40C3E0`/`sub_40C75E` investigation
thread cleanly: both functions are now fully characterized (all 6
command types known, exact caller-side trigger condition confirmed),
and it surfaced a genuine "new resource type" finding on top -- not
just a struct-layout detail, but a whole game-authoring feature this
2002 build has that the 2011 reference source has no trace of at all,
not even a dead declaration. `dword_52033C`'s own contents (beyond the
single non-zero gate check) remain a loose end for a future round.

## GameAnimation follow-up: `dword_52033C` resolved -- it was never a second table

Went back to close the one loose end left from the `GameAnimation`
round: what is `dword_52033C`, actually? A quick address check settled
it immediately -- `dword_52033C - unk_52024C = 0xF0` exactly, which is
precisely `GameAnimation.numCommands`'s own offset within each slot.
`dword_52033C` isn't a second parallel table at all; it's simply
`&unk_52024C[0].numCommands`, reached through a differently-computed
address that IDA's auto-analysis didn't recognize as overlapping with
`unk_52024C`. `run_event_block`'s "!Run_animate: empty animation was
run" check is therefore nothing more exotic than `GameAnimation[data[i]
].numCommands != 0` -- the exact same field `sub_40C75E`'s own loop
bound already reads, just checked once early as a friendlier error path
before the (otherwise silently-no-op) iteration would run. This is also
a second, fully independent confirmation of `numCommands`@`+0xF0`,
arrived at via address arithmetic rather than disassembly reading --
worth remembering as a technique: when two IDA symbols in the same
data region look suspiciously related, checking their raw address
delta against an already-confirmed struct's field offsets is a quick,
cheap way to test whether they're actually the same object.

Nothing else remains open in this thread.

## RoomStatus: pivoting to a fresh struct after the EventBlockCmd thread closed out

Surveyed for the next thread and picked up `RoomStatus` -- already
known to have `beenhere`@`+0x00` and `numobj`@`+0x04` confirmed
(discovered incidentally during the `RoomObject` round), but never
pursued further. `SaveGameSlot`/`restore_game_data` (both already
matched) turned out to be an efficient way in, since save/restore code
naturally has to touch every field that survives across a save file.

**The bulk of the struct saves/restores as one raw blob** -- both
functions do a single `fwrite`/`fread` of the ENTIRE `roomstats` array
with a literal `ElementSize=0x1390`(5008 bytes), which independently
reconfirms the struct's total size (already known from `load_new_room`'s
`newnum*0x1390` scaling) and, as a bonus, the array's own capacity:
`restore_game_data`'s restore loop is bounded by `cmp roomIndex,0x12Ch`
-- `MAX_ROOMS=300`, matching 2011's declared value (`Common/
acroom.h:789`) with ZERO drift, unusual for this project where most
capacities are reduced.

**`tsdatasize`/`tsdata` get separate, individual handling** -- exactly
what you'd expect for a heap-allocated pointer field that can't survive
a raw blob copy. Both functions gate on `tsdatasize@+0x168 > 0`; the
writer does a size-prefixed `fwrite(tsdata@+0x16C, tsdatasize, 1,
Stream)`, the reader frees any existing buffer, `malloc(tsdatasize+5)`,
and `fread`s it back in. This is about as clean a confirmation as this
project gets -- both the save AND restore sides independently agree on
both fields' exact offsets and roles, matching 2011's declared
`tsdatasize`/`tsdata` (`Common/acruntim.h:99-100`) exactly.

**`obj[]`'s capacity and `flagstates[]`'s existence were closed via
arithmetic**, not direct access-site evidence (no already-matched
function reads them field-by-field). `obj[]`'s own start (`+0x08`) was
already established during the `RoomObject` round; `tsdatasize`'s
confirmed position (`+0x168`) gives the other boundary. The 352 bytes
between them only close cleanly if `flagstates[]` matches 2011's
declared `MAX_FLAGS=15` (`acroom.h:801`) EXACTLY (`15*2=30`, +2 align
pad = 32 bytes) -- which leaves `320` bytes for `obj[]`, dividing
EXACTLY by `RoomObject`'s own confirmed `0x20`-byte stride into `10`
objects. A clean round number falling out of an otherwise-arbitrary
subtraction, anchored at one end by a zero-drift 2011 constant, is
convincing evidence even without a direct access site -- but flagged at
MEDIUM confidence per this project's usual standard for arithmetic-only
fits. DRIFT: 10 objects/room here vs. 2011's declared
`MAX_INIT_SPR=40` (`acroom.h:59`) -- a 4x reduction, the usual pattern.

**Everything from `+0x170` onward (4640 bytes) is unexplored.** 2011
declares this space as `intrHotspot`/`intrObject`/`intrRegion`/
`intrRoom` (all `NewInteraction`-based) plus several `enabled`/
`walkbehind_base`/`interactionVariableValues` arrays -- but this build
has already been shown, repeatedly and conclusively (`EventBlockCmd`,
`GameAnimation`, `run_event_block`), to predate `NewInteraction`
entirely and use `EventBlock` instead. 2011's declared layout for this
region cannot be assumed to apply at all, and `SaveGameSlot`/
`restore_game_data` give no field-level evidence here since they only
touch it as part of the single raw blob transfer. Left as an opaque
tail, honestly labeled unexplored rather than guessed at -- a good next
thread if a caller ever turns up (most likely candidates: whatever
this build's own room-level `EventBlock`s for hotspots/regions/the room
itself are called and stored, and walk-behind-related code, which
would need `walkbehind_base`-equivalent data from somewhere in this
range).

## WordsDictionary formalized, plus three clean new function matches along the way

Picked up the last item on the "next candidates" list from `CLAUDE.md`:
`dict`'s own internal layout had been worked out in an earlier round
(the `num_words`/`word[]`/`wordnum[]` fields, with a `sub_4039AB`
placeholder for the reading function) but never turned into a real
`apply_structs.py` struct or given `sub_4039AB` a proper match entry.
Revisiting it turned up three clean function identifications for free.

**`sub_4039AB` = `read_dictionary`** (`Common/acroom.h:1552-1560`) --
an exact shape match: read `num_words` via `getw()`, then loop that
many times reading a word (via a helper) and a `wordnum` short. Its own
helper, **`sub_403969` = `read_string_decrypt`** (`acroom.h:1541-1550`)
-- `newlen=getw(); fread(sss,1,newlen); sss[newlen]=0;
decrypt_text(sss);`, matching the disasm exactly (this build omits
2011's later-added corruption sanity check, `newlen<0 ||
newlen>5000000`, consistent with predating that hardening). And ITS
helper, **`sub_40390F` = `decrypt_text`** (`acroom.h:1483-1497`) --
confirmed beyond doubt by the disassembly literally referencing the
string `"Avis Durgan"` at its decryption-key table, matching 2011's
`static char *passwencstring = "Avis Durgan";` (`acroom.h:1481`) --
AGS's well-known text-encryption Easter egg. The loop shape (subtract
key byte, break on null, advance index mod 11) matches
instruction-for-instruction.

**`WordsDictionary` itself formalized** with capacity confirmed via a
satisfying double-arithmetic check: the boundary between `word[]` and
`wordnum[]` (`0xAFCC`) and the boundary between `wordnum[]` and the
struct's own end (`0xBB84`, the confirmed `malloc` size from
`load_ac2game_dta`) both independently divide out to EXACTLY 1500 words
-- `(0xAFCC-4)/30 = 1500` and `(0xBB84-0xAFCC)/2 = 1500`, two
completely separate calculations landing on the same integer with zero
remainder either way. The per-word stride itself (30 bytes) matches
2011's `MAX_PARSER_WORD_LENGTH=30` (`acroom.h:337`) with zero drift --
though the outer 1500-word table capacity itself has no 2011 analogue
at all, since 2011's version is dynamically sized with no fixed
ceiling. This is the same "flatten a dynamic 2011 double-allocation
into one fixed blob behind a single presence-flag pointer" pattern seen
elsewhere in this build (`compiled_script`).

Also note for the record: an attempt to push `InterfaceElement`'s
remaining fields a third time (this session, before pivoting here)
found zero new evidence -- a direct address search for every one of
its unconfirmed fields' computed absolute addresses (`x`/`y`/`x2`/`y2`/
`bgcol`/`fgcol`/`bordercol`/`vtextxp`/`vtextyp`/`vtextalign`/`vtext`/
`numbuttons`/`button`/`flags`/`reserved_for_future`/`popupyp`, derived
from `popup`'s own confirmed address minus `0x330`) across the ENTIRE
disassembly turned up not one single reference anywhere. This is
stronger negative evidence than the previous two "no already-matched
caller touches it" shelvings -- it now looks like Rob Blanc 1 simply
doesn't use the old icon-bar interface system at all (a plausible
authoring choice for a game from the GUI-era transition), making this
a genuine dead end rather than a not-yet-found lead. Recommend not
re-attempting without a fundamentally different technique (e.g.
disassembling the room-file/game-file format itself to see whether
`iface[]` data is even present in Rob Blanc 1's compiled `.crm`/`.dta`
resources, independent of runtime code).

## RoomStatus.hotspot_enabled recovered -- a script-API lead deep in the previously-opaque tail

Went back to `RoomStatus`'s unexplored `+0x170` tail with a targeted
technique: rather than searching blindly, looked for AGS's well-known
`DisableHotspot`/`EnableHotspot`/`GetHotspotAt` script API functions,
already matched (or trivially matchable) by name, on the theory that
per-room hotspot state has to live SOMEWHERE and these are the obvious
functions to touch it.

**`DisableHotspot`/`EnableHotspot`** (both script-exported, already
named): both bounds-check their `hsnum` argument to the range 1
through 19 inclusive, then write `0`/`1` respectively to
`croom[+0x135C+hsnum]` -- a direct, unambiguous hit deep inside the
previously-opaque tail. **`get_hotspot_at`** (already matched) reads
the same offset to gate hotspot hit-testing ("if the flag is zero,
treat as if there's no hotspot here"). And -- a satisfying callback --
the exact same `[reg+135Ch]` access pattern had ALREADY been read once
before, much earlier in this project's `RoomObject` investigation
(inside `load_new_room`'s room-entry initialization loop, "for
cc=0..19: `croom[+0x135C+cc]=1`"), just not recognized as anything in
particular at the time since neither `RoomStatus` nor this offset had
been mapped yet. All FOUR sites agree exactly: this is
`RoomStatus.hotspot_enabled[20]` @ `+0x135C`.

**A genuinely archaeological capacity finding**: the bounds check
caps `hsnum` at 20, matching 2011's own code comment documenting
`MAX_HOTSPOTS`'s history -- `Common/acroom.h:65`: "v2.62 increased
from 20 to 30; v2.8 to 50". This build's `20` isn't an ad-hoc
reduction like most of this project's other drift findings -- it's
2011's own documented ORIGINAL value, meaning Rob Blanc 1 genuinely
predates BOTH of AGS's later hotspot-capacity increases. A rare case
where the drift finding lines up with an explicit version-history
comment in the reference source rather than just an inferred "smaller
capacity" pattern.

**A related search came up empty, and that's informative too**:
looked for `DisableRegion`/`EnableRegion` (2011's region equivalents,
`Engine/AC.CPP:17484-17494`) the same way, expecting to find
`region_enabled` similarly. Neither function's name NOR its distinctive
error string (`"!DisableRegion: invalid region specified"`) appears
anywhere in this binary at all -- unlike `DisableHotspot`/
`EnableHotspot`, which are both cleanly present. Regions THEMSELVES
aren't absent (per-region tinting/light-level code is already
confirmed elsewhere), just this specific pair of enable/disable script
commands. Not conclusive on its own, but a real lead: it's plausible
this build predates the "toggle a region on/off" script API even
though the region concept and its light-level data already exist.
Logged as an open question, not asserted as confirmed-absent (unlike
`OBJF_*`-style bit findings elsewhere, this doesn't have the same
"total absence across the whole cascade" strength of evidence).

**Struct updated**: the previous single `_tail_unexplored[0x1220]`
opaque blob is now split into three pieces --
`_pad_unexplored1[0x11EC]` (`+0x170..+0x135C`, still opaque),
`hotspot_enabled[20]` (`+0x135C..+0x1370`, HIGH confidence), and
`_pad_unexplored2[0x20]` (`+0x1370..+0x1390`, still opaque, where 2011
declares `region_enabled`/`walkbehind_base`/
`interactionVariableValues`). Total size unchanged and still lands
exactly at the confirmed `0x1390`.

## RoomStatus.walkbehind_base closes the tail, and proves region_enabled is genuinely absent

Immediate follow-up to the `hotspot_enabled` round: checked for AGS's
`SetWalkBehindBase` script function the same way, on the theory that
2011 declares `walkbehind_base[]` as `hotspot_enabled`'s very next
neighbor.

**`SetWalkBehindBase`** (already correctly named in the IDB via linker
map, though with a pre-existing label typo -- `SetalkBehindBase`,
missing the `W`, not introduced by this project) bounds-checks its `wa`
argument to `1` through `14` inclusive, then writes a 2-byte value to
`croom[+0x1370+wa*2]` -- landing EXACTLY at `RoomStatus.hotspot_enabled`'s
confirmed end (`+0x1370`), with ZERO gap between them.

**That zero gap is the interesting part.** 2011 declares
`region_enabled[MAX_REGIONS]` between `hotspot_enabled` and
`walkbehind_base`. If it existed in this build at all, there would have
to be room for it right there -- and there isn't any. Combined with the
already-noted total absence of `DisableRegion`/`EnableRegion`'s names
AND error strings anywhere in this binary (unlike `DisableHotspot`/
`EnableHotspot`, both cleanly present), this upgrades last round's
tentative "open lead, not confirmed absent" to a genuine CONFIRMED
ABSENT finding -- the same standard used for `numcursors`,
`default_lipsync_frame`, `DialogTopic.topicFlags`, and others
throughout this project. Rob Blanc 1 most likely predates per-region
enable/disable as a scriptable concept entirely, even though per-region
tint/light-level DATA already exists (`SetAreaLightLevel`, already
matched) -- the data and the on/off toggle for it appear to have been
added to AGS at different times.

**The struct's final 32 bytes are now fully closed too**:
`walkbehind_base[15]` (`+0x1370..+0x138E`, 30 bytes) plus a natural
2-byte trailing pad lands EXACTLY on the confirmed `0x1390` total with
zero slack -- which ALSO proves 2011's declared trailing field,
`interactionVariableValues[MAX_GLOBAL_VARIABLES]` (100 ints = 400
bytes), is CONFIRMED ABSENT here as well: there's simply no room left
for it. DRIFT: capacity 15 (indices 0-14, with 0 conventionally
reserved the same way `hotspot_enabled` treats its own index 0) vs.
2011's declared `MAX_OBJ=16` -- a one-less reduction, though unlike
`MAX_HOTSPOTS` there's no version-history comment in the reference
source to say whether this is an "original value" or an ad-hoc
reduction.

**Where this leaves `RoomStatus`**: `beenhere`, `numobj`, `obj[10]`,
`flagstates[15]`, `tsdatasize`, `tsdata`, `hotspot_enabled[20]`, and now
`walkbehind_base[15]` are all confirmed, plus TWO fields conclusively
proven absent (`region_enabled`, `interactionVariableValues`). One
real gap remains: `+0x170` through `+0x135C` (4588 bytes, where 2011's
enormous `NewInteraction`-based `intrHotspot`/`intrObject`/
`intrRegion`/`intrRoom` block would sit, already proven irrelevant
here) -- still genuinely unexplored, no already-matched caller found
touching it yet. A good candidate for a future round if a new lead
surfaces.

## RoomStatus FULLY MAPPED: the last gap was this build's own EventBlock-based room interactions

Went back to `RoomStatus`'s one remaining gap (`+0x170` through
`+0x135C`, 4588 bytes) with a specific hypothesis: 2011 fills this
region with `NewInteraction`-based `intrHotspot`/`intrObject`/
`intrRegion`/`intrRoom` (already proven entirely absent from this
build), but 2011's OWN source has a suggestive clue sitting right next
to that declaration -- three lines, commented out:

```c
/*  EventBlock hscond[MAX_HOTSPOTS];
  EventBlock objcond[MAX_INIT_SPR];
  EventBlock misccond;*/
```

This is exactly the EventBlock-based ancestor layout this build has
already been shown to use throughout (`GameSetupStructBase.__charcond`/
`__invcond`, `run_event_block` itself). The hypothesis: this build's
`RoomStatus` still has these three fields live, in this exact gap,
matching the dead comment almost verbatim.

**Confirmed directly, all three, no arithmetic guessing required.**
Searching for `run_event_block`'s remaining unread call sites (9 total
in the whole binary) turned up two already-matched functions that go
straight to the point:

- **`RunHotspotInteraction`**: `croom + hotspothere*0x94 + 0x170` --
  `hscond[20]`, starting IMMEDIATELY after the confirmed `tsdata`
  field.
- **`RunObjectInteraction`**: `croom + aa*0x94 + 0xD00` -- `objcond[10]`.
- A previously-unmatched helper (`sub_40C335`, called only from
  `new_room`, with `String1="room"`): `croom + 0x12C8` -- `misccond`, a
  single instance (not an array), the room-level "Player Enters/Leaves
  Screen" event handler.

**The arithmetic converges from three independent directions at
once**, which is the strongest kind of evidence this project gets:
`hscond`'s capacity (20) independently matches the ALREADY-confirmed
`hotspot_enabled[20]` (a totally different access site, from the
`DisableHotspot`/`EnableHotspot` round); `objcond`'s capacity (10)
independently matches the ALREADY-confirmed `RoomObject obj[10]`
(again, a totally different access site, from `SaveGameSlot`/
`restore_game_data` arithmetic); and `hscond[20] + objcond[10]`
(`20*0x94 + 10*0x94 = 0x1728`, from `+0x170`) lands EXACTLY on
`misccond`'s own independently-confirmed start (`+0x12C8`), which in
turn ends EXACTLY on `hotspot_enabled`'s own independently-confirmed
start (`+0x135C`). Every boundary in this entire 4588-byte region is
now pinned down by at least two unrelated pieces of evidence agreeing
with zero slack.

The remaining unread `run_event_block` call sites are all accounted
for too: 2x `RunCharacterInteraction` (character-level, uses a SEPARATE
global array, not part of `RoomStatus`), 1x `run_event_block_inv`
(inventory-level, also separate), and 1x inside `process_event`'s
generic dispatcher (routes to a pointer resolved elsewhere, not a new
struct lead).

**`RoomStatus` is now FULLY MAPPED** -- every byte from `+0x00` through
the confirmed `+0x1390` total is accounted for: `beenhere`, `numobj`,
`obj[10]`, `flagstates[15]`, `tsdatasize`, `tsdata`, `hscond[20]`,
`objcond[10]`, `misccond`, `hotspot_enabled[20]`, `walkbehind_base[15]`
(8 confirmed fields), plus `region_enabled` and
`interactionVariableValues` CONFIRMED ABSENT. A genuinely satisfying
result: this build's `RoomStatus` turns out to be almost a direct,
still-live implementation of what 2011's source keeps around only as a
commented-out historical footnote.

## CharacterInfo = OldCharacterInfo: a whole struct's remaining gaps close via 2011's other save-compat ancestor

Surveyed for a fresh target after `RoomStatus` closed out, and found
one sitting in plain view: `CharacterInfo` had two small unexplored
gaps (`+0x24..+0x2C`, `+0x30..+0x34`) and one large one (`+0x48..
+0x140`, 248 bytes) left over from much earlier rounds, plus a
previously-flagged caution note on its `inv` field ("named after
2011's `short inv[MAX_INV]` array, but this 2002 struct is far too
small to hold a 301-element array").

**The key realization**: this project already has precedent for a
2011 struct having an OLDER, smaller save-compatibility ancestor
declaration sitting alongside the modern one (`GameSetupStructBase` /
`OriGameSetupStruct`, `RoomStatus`'s own dead-commented `EventBlock`
fields). `CharacterInfo` has exactly this too --
`Common/acroom.h:2599`, `struct OldCharacterInfo`. Laying out its
declared fields by hand and comparing to what's ALREADY confirmed in
this build's `CharacterInfo` was immediately striking: every single
already-independently-confirmed field (`defview`, `talkview`, `view`,
`room`, `prevroom`, `x`, `y`, `wait`, `flags`, `idletime`, `idleleft`,
`activeinv`, `loop`, `frame`, `walking`, `animating`, `walkspeed`,
`animspeed`) lands at EXACTLY the offset `OldCharacterInfo`'s own
declared field order predicts -- and the struct's OWN total size
(`0x140`) matches `OldCharacterInfo`'s computed total exactly too.

**Confirmed the gaps directly, not just by arithmetic**, the same
standard as every other struct in this project:

- **`following`@`+0x24`, `followinfo`@`+0x26`**: `FollowCharacterEx`
  (already matched, script-exported) sets both directly -- `following`
  from its `tofollow` parameter, `followinfo` as a packed byte pair
  (`(distaway<<8)|eagerness`).
- **`idleview`@`+0x28`**: `SetCharacterIdle` (already matched) sets it
  to `iview-1`. The same function also reconfirmed `idletime`/
  `idleleft`/`wait`/`flags`, all at their already-known offsets.
- **`transparency`@`+0x30`**: `SetCharacterTransparency` (already
  matched) -- the EXACT same `((100-trans)*25)/10` formula already
  confirmed for `RoomObject.transparent` earlier this session, just
  applied to a different struct.
- **`baseline`@`+0x32`**: `SetCharacterBaseline` (already matched)
  sets it directly from its `basel` parameter.

**The `inv` caution note is resolved, not just closed**: `main`'s
already-known startup loop (previously cited only for
`InventoryItemInfo.flags`/`IFLG_STARTWITH`) does `mov word ptr
[playerchar+ee*2+0x44], 1` -- a 2-BYTE write, not the 4-byte write the
old "int inv" annotation implied. This is `OldCharacterInfo`'s declared
`short inv[100]` (`acroom.h:2616`), not a mis-sized attempt at 2011's
301-entry array as the caution note worried. The earlier annotation
had the right offset but the wrong type/size the whole time.

**The remaining tail** (`actx`, `acty`, `name[30]`, `scrname[16]`, `on`
-- everything from `+0x10C` onward) is filled in at MEDIUM confidence,
positional-only: no already-matched function was found touching them
this round, but they close the struct's own confirmed total size
(`0x140`) with zero slack once `inv[100]`'s corrected type/size is
accounted for. A good candidate for a future round (character name/
script-name lookups are common enough operations that a caller
probably exists, just not found yet).

**`CharacterInfo` is now essentially fully accounted for** -- every
byte from `+0x00` through `+0x140`, mixing HIGH-confidence directly-
confirmed fields (everything through `+0x44`) with MEDIUM-confidence
positional fields for the tail, following the exact same "OLD
save-compat ancestor, still live in 2002" pattern already established
for `GameSetupStructBase` and `RoomStatus`.

## CharacterInfo.name/.on upgraded to HIGH confidence -- the predicted tail lead paid off immediately

Picked up the thread flagged at the end of the previous round: find a
caller touching `CharacterInfo`'s still-positional-only tail
(`actx`/`acty`/`name[30]`/`scrname[16]`/`on`). Character name lookups
turned out to be exactly as findable as expected.

**`GetLocationName`** (already matched, the classic AGS "what's under
the cursor" script function) has a character-hover branch: `lea
ecx,[game_chars+idx*0x140+0x110]; push ecx; call GetTranslation` --
passing `CharacterInfo.name` straight to the already-matched
`GetTranslation`, then `strcpy`-ing the result out, matching 2011's
`strcpy(tempo,get_translation(game.chars[onhs].name));` exactly.
Upgrades `name`@`+0x110` from MEDIUM (positional) to HIGH.

**`GetCharacterAt`** (already matched) delegates the actual pixel-hit-
testing to an internal helper (`sub_417ECD`, undocumented until now).
Its per-character filter loop checks four things before doing
sprite-level hit-testing: `room`@`+0x0C` matches the current room,
`flags`@`+0x20` doesn't have a `CHF_NOINTERACT`-style bit set, `view`
@`+0x08` is a valid (non-negative) view number, and -- the new find --
`on`@`+0x13E` is nonzero. Upgrades `on`@`+0x13E` from MEDIUM to HIGH,
and gives it a clear semantic role ("is this character
visible/clickable") matching 2011's declared last `OldCharacterInfo`
field exactly.

Still open: `actx`/`acty`/`scrname[16]` remain positional-only (MEDIUM
confidence) -- no caller found yet for those three specifically. Not
pursued further this round; a reasonable place to stop given how much
of `CharacterInfo` is now HIGH confidence (everything through `+0x44`,
plus `name` and `on` in the tail).

## CharacterInfo.scrname confirmed too -- actx/acty checked and shelved as a likely later addition

Continued chasing `CharacterInfo`'s last few positional-only fields.

**`scrname`@`+0x12E` confirmed** via `compile_room_script` (already
matched): "`if (game.chars[aa].scrname[0]==0) continue; ...
strcat(temphdr,game.chars[aa].scrname);`" -- building `"#define cEgo
0\r\n"`-style macros that map each character's script name to its
numeric index, so room scripts can reference characters by their
in-editor name. This is about as foundational an AGS feature as they
come (room-script compilation needing name-to-index resolution), and
the disassembly match is exact: `scrname[0]` checked at `+0x12E`, then
the whole field passed to `strcat` at the same offset. Upgraded from
MEDIUM to HIGH.

**`actx`/`acty` checked and NOT found this round** -- worth recording
why, since it wasn't for lack of trying. 2011's ONLY usage site for
either field (`Engine/AC.CPP:8525-8526`, `chin->actx=atxp+offsetx;`)
sits deep inside hardware-accelerated sprite drawing code --
`gfxDriver`, `actspsbmp`, `SetTint`, `SetLightLevel` -- an abstraction
layer this build has already been shown, repeatedly and independently
(`RoomObject`'s confirmed-absent tint/zoom fields, the total absence of
`region_enabled`, etc.), to predate entirely. Grepped the whole
`Engine/` tree for `actx`/`acty` and found nothing outside that one
site. Plausibly these fields track "last drawn position for hardware-
accelerated sprite caching," a concept that simply didn't exist yet in
2002 -- not asserted as CONFIRMED ABSENT (unlike fields with harder
proof, e.g. `RoomStatus.region_enabled`), just shelved as unlikely to
be found via this technique.

**`CharacterInfo` status after this round**: every field through
`+0x44` is HIGH confidence, plus `name`, `on`, and now `scrname` in the
tail. Only `actx`/`acty` remain MEDIUM (positional-only, likely-absent-
feature). About as complete as this struct is going to get without a
fundamentally different technique.

## GUIMain: a fresh survey target, and a rare case matching 2011's CURRENT layout instead of an old ancestor

Surveyed for a genuinely fresh target after `CharacterInfo` wound
down, and found one hiding in plain sight: `GUIMain` had been sitting
with only 8 fields confirmed (`x`, `y`, `numobjs`, `mouseover`,
`mousedownon`, `on`, `objs[30]`, `objrefptr[30]`) and five large opaque
padding gaps, unlike almost everything else in this project, its
padding had never been revisited with a hypothesis to test.

**The realization**: laying out 2011's CURRENT `GUIMain` declaration
(`Common/acgui.h:664-687` -- notably NOT an old ancestor struct, this
one doesn't have one) by hand and comparing to the already-confirmed
offsets was immediately striking, the same way `OldCharacterInfo` was:
`x`@`+0x28`, `y`@`+0x2C`, `numobjs`@`+0x3C`, `mouseover`@`+0x54`,
`mousedownon`@`+0x60`, `on`@`+0x90`, `objs`@`+0x94`, `objrefptr`@`+0x10C`
-- EVERY one of these lands exactly where 2011's declared field order
predicts, with zero drift. Unlike most other structs in this project
(`GameSetupStructBase`, `RoomStatus`, `CharacterInfo`, all matching an
OLD save-compat ancestor), `GUIMain` apparently matches 2011's LIVE,
current layout almost unchanged -- a genuinely rare case, alongside
`MouseCursor`/`InventoryItemInfo`'s own "zero drift" wins.

**The five previously-opaque gaps all close simultaneously** once
2011's declared fields are laid into them: `vtext[4]`+`name[16]`+
`clickEventHandler[20]` = 40 bytes, exactly filling `+0x00..+0x28`;
`wid`+`hit`+`focus` = 12 bytes, exactly filling `+0x30..+0x3C`;
`popup`+`popupyp`+`bgcol`+`bgpic`+`fgcol` = 20 bytes, exactly filling
`+0x40..+0x54`; `mousewasx`+`mousewasy` = 8 bytes, exactly filling
`+0x58..+0x60`; `highlightobj`+`flags`+`transparency`+`zorder`+
`guiId`+`reserved[6]` = 44 bytes, exactly filling `+0x64..+0x90`. Five
independent gaps, each closing with zero slack against ONE internally
consistent declaration, is about as strong an over-determined fit as
this project produces without individual access-site evidence for
every field.

**One field got real access-site confirmation too**: `SetGUIBackgroundPic`
(already matched, script-exported) writes its `slotn` parameter
straight to `+0x4C` -- exactly `bgpic`'s predicted position, upgrading
it to HIGH confidence and giving the whole arithmetic-fit hypothesis a
concrete anchor rather than resting on positional inference alone.

**Checked and NOT found this round**: `SetGUIClickable` (`flags`/
`GUIF_NOCLICK`), `SetGUITransparency`, and `SetGUIZOrder`/
`GUI_SetZOrder` -- none of these function names or their distinctive
error strings appear anywhere in this binary, consistent with (but not
conclusive proof of) this project's repeated "later API surface, not
yet added" pattern. `flags`/`transparency`/`zorder` remain at MEDIUM
confidence (positional-fit only) as a result.

Total confirmed size (`0x184`) also matches 2011's own declared
save-file read/write size EXACTLY -- `GUIMain::ReadFromFile` reads a
40-byte header plus `27 + 2*MAX_OBJS_ON_GUI` ints, deliberately
EXCLUDING `drawOrder[MAX_OBJS_ON_GUI]` (regenerated at load time, never
persisted) -- matching this build's own confirmed total exactly,
including that same exclusion.

## GUIMain follow-up: GetGUIAt confirms four more fields in one pass

Immediate follow-up to the `GUIMain` round: checked `GetGUIAt`
(already matched, script-exported "find the GUI at this screen
location") for more of the arithmetic-fit-only fields, since its
2011 source (`Engine/AC.CPP:16085-16098`) touches `on`, `flags`,
`x`, `y`, `wid`, and `hit` all in one small function.

The disassembly matches instruction-for-instruction: `on`@`+0x90`
checked `>=1` (reconfirming existing evidence), `flags`@`+0x68` bit 0
checked (`and edx,1; jz <continue>`) matching 2011's `GUIF_NOCLICK=1`
exactly, and -- the new finds -- `wid`@`+0x30` and `hit`@`+0x34`, both
used to compute the bounding box's right/bottom edge (`x+wid`, `y+hit`)
for the point-in-rectangle test, matching 2011's `xx<=guis[aa].x+
guis[aa].wid` / `yy<=guis[aa].y+guis[aa].hit` exactly. Four fields
upgraded to HIGH confidence in one function (`flags`, `wid`, `hit`,
plus a clean reconfirmation of `x`/`y`/`on`).

`transparency`/`zorder`/`popup`/`bgcol`/`fgcol` (and the rest of the
still-MEDIUM fields) remain positional-only -- no `GUIMain::draw`
equivalent has been matched yet in this project, which is the most
likely place to find `transparency`/`zorder` touched. A reasonable
next lead for a future round.

## GUIMain.zorder: chased further, and the negative result got stronger

Kept pulling the `transparency`/`zorder` thread from the previous
round. `GUIMain::draw`/`draw_at` (2011's most obvious place to expect
`transparency` -- it composites the GUI's background/border using
`fgcol`/`bgcol`/`bgpic`) turns out NOT to touch `transparency` at all
in 2011's own source -- alpha/transparency compositing there is
delegated to `GUIObject::IsDisabled()`/`IsVisible()` virtual calls, a
much later hardware-acceleration-era abstraction already shown absent
from this build repeatedly. Not a profitable lead; `transparency`
remains unconfirmed, no new evidence found.

`zorder` fared better, though still not to a direct confirmation.
2011's `zorder` machinery is small and traceable: `SetGUIZOrder`/
`GUI_SetZOrder` write it, and `update_gui_zorder()` (`Engine/
AC.CPP:8624-8644`) sorts `play.gui_draw_order[]` by it via a simple
insertion sort, called both from `GUI_SetZOrder` and once during game
data loading. All three names were searched for and found nowhere in
this binary -- consistent with, but not stronger than, the "later API"
absence pattern already established for several other GUI functions.

**The stronger piece of evidence came from re-reading `GetGUIAt`'s own
loop structure closely**: 2011's version reads `aa =
play.gui_draw_order[ll]` -- an INDIRECTION through the z-order-sorted
array -- before using `aa` to index `guis[]`. This build's `GetGUIAt`
has no such indirection at all: its loop counter (`var_4`, counting
down from `numgui-1` to `0`) is used DIRECTLY as the `guis[]` array
index, with no intermediate lookup array in sight anywhere in the
function. This is real, positive evidence (not just an absent name)
that z-order-aware GUI iteration genuinely isn't wired into this
build's hit-testing code -- strengthening the "this build predates GUI
z-ordering as a feature" hypothesis considerably, even though `zorder`
itself as a struct FIELD can't be ruled in or out (it may still exist,
inertly, matching the struct's own confirmed total size, just
unconnected to anything that reads or writes it).

`GUIMain` status unchanged in terms of confirmed field COUNT this
round, but the confidence picture around `zorder` specifically is now
much better supported than a plain "not found" -- logged as a genuine
finding, not a dead end.

## GUIMain.popup/popupyp confirmed -- and a discipline note: check matches.json's own history before writing MEDIUM

Kept working the `GUIMain` thread, this time via `check_controls`
(already matched, the input-handling function that decides when
"popup" GUIs should show/hide based on mouse position) and its own
callee `remove_popup_interface`.

**`popup`@`+0x40` confirmed**: `check_controls` does `cmp
[guis+ev2*0x184+0x40], 1; jz <continue>` -- a literal exact-match
check against `1`, matching 2011's declared `POPUP_MOUSEY=1` (`Common/
acroom.h:277`) precisely. This gates the whole "should this GUI
auto-show when the mouse nears its trigger line" branch that follows.

**`popupyp`@`+0x44` confirmed, from two angles**: `check_controls`'s
own trigger check (`cmp mouseY,[guis+ev2*0x184+0x44]; jge <not-yet>`,
only proceeding while `mouseY < popupyp`) and `remove_popup_interface`
(already matched)'s mouse-repositioning logic when auto-hiding the
popup. Both HIGH confidence.

**A small process note worth recording**: `remove_popup_interface`'s
own `matches.json` entry ALREADY cited `+0x44` as "a popupyp-related
field" from a round that predates this session's tracked history --
but the `apply_structs.py` field itself still said "MEDIUM confidence:
positional/arithmetic fit only" right up until this round. The
evidence existed, it just hadn't been propagated from the function
match's prose into the struct file. Worth remembering for future
survey rounds: before assuming a field is genuinely unconfirmed, it's
worth grepping the ALREADY-matched functions that touch its struct for
any prose mention of the offset, not just re-deriving from scratch.

`GUIMain` status: `x`/`y`/`numobjs`/`mouseover`/`mousedownon`/`on`/
`objs`/`objrefptr`/`flags`/`wid`/`hit`/`bgpic`/`popup`/`popupyp` are
all HIGH confidence now -- 14 of the struct's ~24 named fields.
Remaining MEDIUM: `vtext`/`name`/`clickEventHandler`/`focus`/`bgcol`/
`fgcol`/`mousewasx`/`mousewasy`/`highlightobj`/`transparency`/`zorder`/
`guiId`/`reserved[6]`.

## ccInstance correction: the call-stack-array hypothesis was wrong -- nested calls use native recursion instead

Fresh survey pivot after `GUIMain` wound down: picked up `ccInstance`'s
own long-parked 2400-byte unexplored region (`+0x1C..+0x97C`), which
had carried a standing hypothesis since early in this project --
"almost certainly" 2011's `callStackLineNumber[100]`/`callStackAddr
[100]`/`callStackCodeInst[100]`/`callStackSize` arrays (the
`PUSH_CALL_STACK`/`POP_CALL_STACK` macro targets, `Common/
CSCOMP.H:259-262`).

That hypothesis is now disproven, not just still-unverified. Reading
the interpreter's SCMD_CALL handler (inside `sub_42B394`, the
still-unnamed function documented at length in `reversing/notes/
csrun-interpreter-evolution.md`) in full shows nested script-to-script
calls are handled via NATIVE C RECURSION -- `pc`@`+0x99C` gets saved
into a plain local stack variable, the interpreter calls itself
directly, and on return restores `pc` from that same local variable.
There's no array write, no counter increment, nothing that could
overflow/underflow in 2011's sense -- and indeed, 2011's `"Call stack
overflow (recursive call error?)"`/`"Call stack underflow -- internal
error"` strings are both completely absent from this binary. The four
`"stack overflow"` strings that DO exist in this function turned out to
guard something else entirely -- the VM's data/operand stack pointer
against `stacksize`, unrelated to call-nesting depth.

Full writeup, including the methodology lesson it prompted (a
control-flow oddity noticed early on eventually falsifying an
unrelated struct-field guess, once actually read to completion), is in
`reversing/notes/csrun-interpreter-evolution.md`. `apply_structs.py`'s
`ccInstance` comment updated accordingly -- the 2400-byte region is
back to genuinely unknown, not a settled guess.

## ccScript.fixuptypes/fixups/numfixups confirmed -- found while chasing ccInstance's gap, not its target

Kept reading `ccCreateInstanceEx` past the point that resolved the
`ccInstance` call-stack question, looking for anything else in the
struct's own unexplored `+0x1C..+0x97C` region. Didn't find that --
but the SAME function turned up a clean, unrelated win on `ccScript`
instead: a fixup-processing loop that had gone unread in earlier
rounds.

`ccScript`'s `+0x18..+0x24` region had been sitting as a single
TENTATIVE, positional-only guess since an early round -- "matches the
exact combined size of 2011's `fixuptypes`+`fixups`+`numfixups`,
IF that adjacency holds, not independently verified." It now is:
`ccCreateInstanceEx`'s relocation loop reads a fixup TYPE byte through
a pointer at `+0x18` (`fixuptypes`, `char*`), a fixup INDEX through a
pointer at `+0x1C` (`fixups`, `long*`), and bounds the loop against a
plain int at `+0x20` (`numfixups`) -- matching 2011's declared types
(pointers to separately-allocated arrays, not embedded ones) and names
exactly (`Common/CSCOMP.H:208-210`).

The switch on fixup type is a bonus confirmation of its own: type `1`
adds the new instance's `globaldata`(`+0x04`, already confirmed) to
`code[fixups[i]]`, type `3` adds `strings`(`+0x14`, already confirmed)
-- matching 2011's declared `FIXUP_GLOBALDATA`/`FIXUP_STRING` constants
(`CSCOMP.H:165-170`) exactly, both by VALUE and by which already-
confirmed `ccInstance` field each one targets. Type `4` resolves a
system import via the already-matched `SystemImports::is_script_import`
-- `FIXUP_IMPORT`.

Genuinely a "wrong turn, right destination" round: the actual target
(`ccInstance`'s 2400-byte gap) is still unresolved, but reading the
function in full to look for it turned up this instead. Consistent
with a pattern noticed a few times this session -- reading an
already-matched function's FULL body, not just the part relevant to
the current question, keeps paying off.

## ccInstance.line_number and a cluster of adjacent globals -- closing the last small gap before the big one

Still chasing `ccInstance`'s 2400-byte `+0x1C..+0x97C` gap, but taking
a different approach this round: instead of reading one function start
to finish, grepped `sub_42B394`'s (the interpreter loop's) entire body
for every offset it touches on the instance pointer. The interpreter
never once reaches into the big gap -- but it DOES touch `+0x9A0`
exactly once, a small offset that had been sitting as `_pad_9A0[0x04]`
(a leftover 4-byte gap between `pc`@`+0x99C` and `instanceof_`@`+0x9A4`,
never independently investigated on its own).

The access is in jump-table case 36 of the interpreter's opcode
switch -- `SCMD_LINENUM` ("debug info - source code line number",
`Common/CSCOMP.H:303`) in this build's own numbering, which matches
2011's numbering exactly. The handler is a one-liner:
`[ecx+9A0h] = edx; dword_5347F4 = edx` -- writing the same value into
both the instance field and a global in a single place. This matches
2011's `case SCMD_LINENUM: inst->line_number = arg1; currentline =
arg1;` (`CSRUN.CPP:1334-1336`) instruction for instruction, closing
`_pad_9A0` as `line_number` (high confidence) AND identifying
`dword_5347F4` as the global `currentline` (`cscommon.cpp:21`) in the
same stroke.

That identification cascaded into three more globals, all found by
reading the functions that already reference `dword_5347F4`:

- **`cc_error`** (`sub_42A400`, already matched) ends with `dword_5347F8
  = 1; ecx = dword_5347F4; dword_5347FC = ecx` -- matching
  `cscommon.cpp:59-60`'s `ccError = 1; ccErrorLine = currentline;`
  exactly. Identifies `dword_5347F8` as `ccError` and `dword_5347FC` as
  `ccErrorLine` (both high confidence, `cscommon.cpp:22-23`).
- **`sub_42AAA1`**, a previously-untouched 3-instruction function
  (`push ebp; mov ebp,esp; mov eax,dword_534800; pop ebp; retn`),
  called from the already-matched `quit`, is a trivial getter for a
  FOURTH adjacent global, `dword_534800`. This matches 2011's
  `ccInstance *ccGetCurrentInstance() { return current_instance; }`
  (`CSRUN.CPP:770-772`) exactly -- both the triviality and the global
  returned. New match, high confidence.
- `dword_534800`/`current_instance` gets two more independent
  confirmations: `sub_42B394`'s own SCMD_RET and CALLEXT/CALLAS return
  paths restore `dword_534800 = arg_0` (the instance pointer) after a
  call completes, and `ccCallInstance` (`sub_42BF84`, already matched)
  zeroes it at top-level entry/exit alongside `pc=0` -- matching 2011's
  `ccInstance* currentInstanceWas = current_instance; ... current_instance
  = currentInstanceWas;` (`CSRUN.CPP:1949,1962`) save/restore pattern in
  shape, just always resetting to 0 rather than restoring a saved prior
  value (consistent with this build's native-recursion design needing no
  explicit re-entrancy bookkeeping, per the earlier finding in
  `reversing/notes/csrun-interpreter-evolution.md`).

Incidentally, all four globals sit in four consecutive dwords in this
build's `.data` section (`0x5347F4`/`F8`/`FC`/`534800`), though only the
first three (`currentline`/`ccError`/`ccErrorLine`) match
`cscommon.cpp`'s own declaration order -- `current_instance` is declared
separately, in `CSRUN.CPP`. Likely just link-order coincidence (adjacent
translation units laid out back to back), not evidence the four form a
real struct.

`ccInstance`'s big 2400-byte gap is still completely unexplored -- this
round only closed the small 4-byte pad next to it and picked up four
global-variable identifications along the way.

## ccInstance.exportaddr[600] closes the entire 2400-byte gap -- ccInstance is now FULLY MAPPED

Immediately after the `line_number`/global-cluster round, went back to the
one remaining unresolved thing in `ccInstance`: the 2400-byte
`+0x1C..+0x97C` gap left behind once the call-stack-array hypothesis was
disproven. This time the approach was to reread `ccCreateInstanceEx` a
THIRD time, specifically hunting for anything the earlier fixup-loop and
line_number rounds had walked past without registering.

It was sitting right after the fixup loop the whole time: 2011's export-
address resolution step (`CSRUN.CPP:933-948`, `cinst->exportaddr =
(char**)malloc(sizeof(char*) * scri->numexports)`) has a disassembly
counterpart that does NOT malloc a separate array at all -- it writes
computed addresses straight into `[cinst + idx*4 + 0x1C]`, i.e. an array
embedded directly in the struct, starting exactly where the gap starts.

Three independent pieces of evidence converge on this with zero slack:

1. **The loop bound** is the already-confirmed `ccScript.numexports`
   @`+0x1C48` on the SOURCE script.
2. **The per-entry computation** matches source exactly for both export
   types: `EXPORT_FUNCTION`(1) computes `&cinst->code[eaddr]` (`lea
   edx,[eax+ecx*4]` where `eax`=the new instance's own `code`@`+0x0C`,
   already confirmed), `EXPORT_DATA`(2) computes `cinst->globaldata +
   eaddr` (`eax=[edx+4]` then `add eax,[var_14]`) -- both written to
   `[cinst+idx*4+0x1C]`, matching `CSRUN.CPP:940`/`942` line for line.
3. **The auto-import loop right after** reads the SAME array back
   (`[cinst+idx*4+0x1Ch]`) as the third argument to the already-matched
   `SystemImports::add`, matching source's `simp.add(scri->exports[i],
   cinst->exportaddr[i], cinst)` (`CSRUN.CPP:959`) exactly.

The capacity fits with the same "zero slack" precision this project has
repeatedly found elsewhere: 600 entries -- matching `ccScript.
export_addr[600]`'s own already-confirmed capacity, the natural upper
bound on `numexports` -- times 4 bytes = 2400 = `0x960`, landing exactly
on `stack`@`+0x97C` with no remainder in either direction.

This also retroactively explains a loose thread from the earlier
call-stack-array disproof round: the interpreter (`sub_42B394`) was found
to never touch any offset in this region at all. That's now fully
explained rather than just observed -- `exportaddr` is populated once, at
instance-creation time, by `ccCreateInstanceEx`, and never read or written
by the bytecode interpreter loop itself (script calls resolve import/
export addresses through `SystemImports`, not by re-deriving them from
`exportaddr` at call time).

**`ccInstance` is now FULLY MAPPED** -- every byte from `+0x00` through
its confirmed `+0x9A8` total size is accounted for. The struct's own
history is a good illustration of this project's repeated lesson: the
first hypothesis for a big unexplained region (call-stack arrays, by
direct analogy to 2011's declared fields) was entirely plausible and
entirely wrong, while the actual answer (an embedded fixed-capacity
array, replacing a 2011 dynamic-pointer field with the SAME name and
role) was sitting in a function that had already been read twice before,
just not to the very end.

## GameState (`play`): a fresh survey target, and a genuine structural puzzle left open

A new fresh-survey round, picking the single biggest remaining prize: 2011's
`GameState play;` (`Common/acruntim.h:465-618`), the runtime-state struct
touching nearly every subsystem in the engine (150+ fields by 2011,
accumulated over 9 years -- easily bigger than `GameSetupStructBase` was).
Unlike most structs tackled so far, `play` is a plain global, not malloc'd,
so there's no allocation-size site to anchor a total size from.

**The starting lead** came from `check_skip_cutscene_keypress` (already
matched), whose existing `matches.json` evidence already said "matches the
`dword_4EEB50` >0 / !=3 guard" against `play.in_cutscene` -- a fact that had
never been formalized into an actual struct. Grepping the whole disassembly
for every `dword_4EEB*` global turned up something excellent: a big cluster
of globals from `0x4EEB08` through `0x4EEB74`, MOST of them still unnamed,
but the run starting right before it (`0x4EEA18` through `0x4EEB04`, labeled
`play`, `play_usedmode`, `play_disabled_user_interface`,
`play_gscript_timer`, `play_debug_mode`, `play_globalvars`,
`play_messagetime`, `play_usedinv?`, `play_inv_top?`, `play_inv_numdisp`,
`play_inv_numorder`) is **already named directly in the live IDB**, from
manual work predating this project's own tracking -- the same
"already-recovered, just needs formalizing" situation as `SpriteCache`.

`play_globalvars`'s pre-existing `32h dup(?)` declaration is a strong
corroborating signal on its own: 0x32 = 50, landing EXACTLY on 2011's
`MAXGLOBALVARS=50` (`acruntim.h:21`) with zero drift -- a real, deliberately-
sized array, not coincidental padding, sitting right where 2011 still
declares it (if now marked "obsolete").

**New confirmations this round:**

- **`score`**@`+0x00` -- via a brand-new match, `replace_macro_tokens`
  (`sub_41024B`, `AC.CPP:7104`), the GUI label "@SCORE@"/"@TOTALSCORE@"/
  "@SCORETEXT@"/"@GAMENAME@"/"@OVERHOTSPOT@" macro-substitution routine.
  It reads the `play` global at exactly two call sites, matching source's
  two separate `play.score` reads (`stricmp(macroname,"score")` and
  `stricmp(macroname,"scoretext")`) instruction for instruction. Called
  from `GUILabel__Draw` (already matched), matching source's role exactly.
- **`globalscriptvars[300]`**@`dword_4EEB74` -- via `SetGlobalInt` (already
  matched): its bounds check (`cmp [ebp+index],12Ch`) and flat-array write
  (`dword_4EEB74[edx*4]=eax`) match 2011's `if (index>=MAXGSVALUES)
  quit(...); play.globalscriptvars[index]=valu;` (`AC.CPP:13978-13987`)
  exactly in shape, including the verbatim error string. DRIFT: capacity 300
  here vs. 2011's declared `MAXGSVALUES=500`.
- **`wait_counter`/`mboundx1`/`mboundx2`/`mboundy1`/`mboundy2`** -- via
  `SetMouseBounds` (already matched): four consecutive `dw` writes in
  source order match `play.mboundx1=x1; play.mboundx2=x2; play.mboundy1=y1;
  play.mboundy2=y2;` (`AC.CPP:3877-3880`) exactly, sitting immediately after
  an already-named `play_wait_counter` with zero gap -- matching 2011's own
  declared adjacency `short wait_counter; short mboundx1,mboundx2,mboundy1,
  mboundy2;` (`acruntim.h:548-549`) with zero drift in type, order, AND
  stride.

**The puzzle**: naively treating all of this as one contiguous struct
(`play`'s base at `0x4EEA18`, computed from the confirmed zero-gap prefix)
would put `in_cutscene` at `+0x138`, `wait_counter` at `+0x148`, and
`globalscriptvars` at `+0x15C`. But the confirmed-unrelated global `ifnum`
(a genuine standalone variable -- in 2011 it's the default-value argument
of `draw_text_window(..., int ifnum=-1)`, `AC.CPP:12533`, meaning this 2002
build most likely still tracks it as a plain global predating that
refactor into a parameter) sits at the naive `+0x110` position -- squarely
INSIDE that range. A single real C struct cannot have a foreign global's
bytes injected into the middle of its own members; that's not a "confidence
downgrade," it's a structural impossibility if `play` is genuinely one
object spanning that whole range.

**Likeliest resolution** (not yet conclusively proven): `in_cutscene`,
`wait_counter`/`mboundx1-y2`, and `globalscriptvars` are INDEPENDENT
STANDALONE 2002 globals -- each individually the ancestor of a field 2011
later folded into ONE `GameState` struct -- rather than already being
members of the same object as `score` et al. in this build. This would be
a variant of a pattern already seen elsewhere in this project
(`ExecutingScript` had 2011 unify 5 dedicated 2002 fields into one generic
array): here the direction is the same (2011 consolidates), just applied to
whole loose globals being folded into a struct rather than parallel arrays
being folded into one array.

Given this is unresolved, `apply_structs.py`'s new `GameState` struct
deliberately STOPS at `inv_numorder`@`+0xEC` (the last field before the
`ifnum` contradiction) rather than asserting a size or gap contents past
that point. `inv_numorder` itself is worth a flag of its own: it's named
WITHOUT 2011's `obsolete_` prefix, and is written by `update_invorder`
(already matched) -- a name implying active use, not vestigial behavior;
plausibly this build predates it becoming obsolete.

One more open thread noted but not chased this round: `dword_4EEB54`,
`dword_4EEB58`, `dword_4EEB5C` sit between `in_cutscene` and `wait_counter`
and are all XREF'd from `EndSkippingUntilCharStops` (already matched) --
but that function turns out to be a much larger, screen-tint/fade-
restoration-inlined routine in this build than 2011's tiny 3-line
`stop_fast_forwarding()`/`EndSkippingUntilCharStops()` pair, so its field
touches don't cleanly map onto source without a much closer read. Left
unidentified rather than guessed.

## GameState follow-up: inv_numinline/inv_item_wid/inv_item_hit confirmed, and inv_numorder proven genuinely live

A direct follow-up on the previous round's `dword_4EEB08` lead (tentatively
"`inv_numinline`", positional only). Read `sub_40D80C` (XREF'd by several
of the confirmed `inv_*` fields) in full, rather than just noting its
touches -- and it turned out to be an instruction-for-instruction
algorithmic twin of 2011's `offset_over_inv(GUIInv*)`
(`AC.CPP:5394-5409`):

```
mover = xoffs / itemWidth;
if (mover >= itemsPerLine) return -1;
mover += (yoffs / itemHeight) * itemsPerLine;
if (mover >= itemsPerLine * numLines) return -1;
mover += topIndex;
if (mover < 0 || mover >= invorder_count) return -1;
return invorder[mover];
```

matching the disassembly's div/mul/bounds-check chain exactly, field for
field:

- `dword_4EEB08` = **`inv_numinline`**@`+0xF0` (`itemsPerLine`) -- the
  divisor-count check itself, matching the field's own 2011 name/role
  precisely.
- `dword_4EEB18`/`dword_4EEB1C` = **`inv_item_wid`**@`+0x100`/
  **`inv_item_hit`**@`+0x104` -- the column/row divisors, matching 2011's
  exact declared adjacent pair (`inv_item_wid,inv_item_hit; // set by
  SetInvDimensions`).
- `play_inv_top`@`+0xE4` (previously "?"-flagged by the prior session) and
  `play_inv_numdisp`@`+0xE8` both get resolved to HIGH confidence via this
  same match (`topIndex` and `itemsPerLine*numLines` respectively).

The gap between `inv_numinline`@`+0xF0` and `inv_item_wid`@`+0x100` is
exactly 3 ints (12 bytes) -- matching the COUNT of fields 2011 declares in
that exact span (`text_speed`, `sierra_inv_color`, `talkanim_speed`) with
zero slack. Filled in as TENTATIVE/positional-only, since none of the
three has an access-site confirmation of its own yet.

`sub_40D80C` itself is deliberately left unnamed: its callers
(`check_controls`, `GetLocationName`) match 2011's `GetInvAt()` callers,
but `GetInvAt`'s own 2011 body (GUI-object dispatch through
`find_object_under_mouse`) looks nothing like this function, while the grid
-math half matches `offset_over_inv` exactly. Most likely this build has
ONE function doing what 2011 later split into two (`GetInvAt` dispatch +
`offset_over_inv` math) -- consistent with this project's repeated
"later split into more functions" pattern -- but the identity of the whole
function is left open rather than forced onto either name.

**A second, independent thread closed in the same round**: `update_invorder`
(already matched) turned out to be a much simpler single-character
predecessor of 2011's per-character-generalized version
(`AC.CPP:7161-7185`) -- no per-character loop, no `OPT_DUPLICATEINV`
duplicate handling, no `MAX_INVORDER=500` bounds check. Its full body:

```
play_inv_numorder = 0;
for (ff = 0; ff < game_numinvitems; ff++) {
    if (playerchar->inv[ff] > 0) {
        play_invorder[play_inv_numorder] = ff;
        play_inv_numorder++;
    }
}
guis_need_update = 1;
```

This directly PROVES `inv_numorder`@`+0xEC` is NOT obsolete in this
build, resolving the open question flagged in the previous round: 2011
keeps `obsolete_inv_numorder` (`acruntim.h:474`) purely as a "backwards
compatibility" mirror of the real per-character count
(`play.obsolete_inv_numorder = charextra[playerchar].invorder_count;`),
but this build's `play_inv_numorder` IS the one true counter, actively
incremented right here. It also surfaces a genuinely NEW global,
`play_invorder` -- a `short[]` array (confirmed via the 2-byte-stride
write `play_invorder[edx*2]=ax`), the single-character predecessor of
2011's per-character `charextra[cc].invorder[]` (living inside
`CharacterExtras`, a struct not yet formalized in this project -- a
natural next target). `playerchar+0x44` (already-confirmed
`CharacterInfo.inv[]`) gets a further cross-confirmation as a bonus.

**Important structural note**: `play_invorder`'s own address (~`0x4EF02C`)
is far from `GameState`'s confirmed range (~`0x4EEA18`-`0x4EEB08`) -- it is
a genuinely separate global, not embedded in `play`, joining `in_cutscene`/
`wait_counter`/`mboundx1-y2`/`globalscriptvars` on the "independent 2002
global, later folded into GameState by 2011" side of the open structural
question from the previous round, rather than resolving it either way.

## GameState "island 2" cracked open: EndSkippingUntilCharStops turns out to secretly be unload_old_room too

Picked up the round's remaining open thread: `dword_4EEB54`/`58`/`5C`/`6C`/
`70`, all XREF'd from `EndSkippingUntilCharStops` (`sub_40AAE3`, already
matched via callgraph position) but left "inconclusive" last round because
that function's disassembly is far bigger (spans ~220 lines) than 2011's
tiny 3-line source function.

Reading it start to finish resolved the mystery: this build's
`EndSkippingUntilCharStops` doesn't just call `stop_fast_forwarding()` at
the end (as already matched -- CORRECTION: this final-call identification
was itself wrong, see the correction round several sections below; the
actual final call is `remove_screen_overlay(-1)`, and `stop_fast_forwarding`
remains unlocated in this binary) -- it does EVERYTHING 2011 splits out into a
separate `unload_old_room()` (`AC.CPP:3585-3653`, called by 2011's
`new_room()` as its own later step) INLINE, before that final call. Three
separate pieces of source-level evidence converge on this:

1. A 3-way screen-transition dispatch (`cmp dword_4EEB6C,1` / `cmp
   play_scren_tint,0` / `cmp dword_4EEB6C,0`) matches 2011's
   `current_fade_out_effect()` (`AC.CPP:3538-3567`) instruction for
   instruction -- `if (theTransition==FADE_INSTANT || play.screen_tint>=0)
   wsetpalette(...); else if (theTransition==FADE_NORMAL) my_fade_out(5);
   else {...}`, using 2011's own `FADE_NORMAL=0`/`FADE_INSTANT=1`
   constants (`acroom.h:2753-2754`). Identifies `dword_4EEB6C` as
   `play.fade_effect`, and reconfirms the already-IDA-named `play_scren_tint`
   as `play.screen_tint` (role confirmed via `TintScreen`; its address was
   later found to have been miscalculated here -- see the correction round
   several sections below, where it's confirmed to be a genuine in-struct
   field, not the standalone global it was believed to be at the time).
2. A `StopAmbientSound` call and a `free`/`malloc`/`memcpy`/`ccFreeInstance`
   sequence match source's ambient-sound-stop loop and
   `save_room_data_segment()`+`ccFreeInstance(roominstFork)`+
   `ccFreeInstance(roominst)` room-script cleanup respectively. (An
   immediate follow-up round read the helper called right before this
   sequence, `sub_409A9C` -- guessed here as a "strong candidate" for
   `save_room_data_segment` -- and found that guess WRONG: it's actually
   `cancel_all_scripts`. See the next section.)
3. Three zero-writes right before the final `destroy_bitmap`/
   `remove_screen_overlay(-1)` calls (the latter originally misread as
   `stop_fast_forwarding` -- see the correction round several sections
   below) match source's `play.bg_frame=0; play.bg_frame_locked=0;
   play.offsets_locked=0;` (`AC.CPP:3624-3626`) exactly in sequence and role.

That third piece resolves the remaining fields: `dword_4EEB70` =
**`bg_frame_locked`** (doubly confirmed -- zeroed matching source, AND
sitting immediately after `fade_effect` matching 2011's own declared
adjacency `int fade_effect; int bg_frame_locked;`,
`acruntim.h:550-551`, with zero drift); `dword_4EEB58` = **`bg_frame`**
(medium-high -- zeroed alongside `bg_frame_locked` matching the source
pairing, though this build's massive field-count drift in the preceding
region means its exact intra-struct position can't be independently
verified by counting, unlike `bg_frame_locked`); and a THIRD global found
by grepping the same zeroing pattern more broadly, `word_4EF236`
(standalone, far from either GameState island) = **`offsets_locked`**,
zeroed immediately after `bg_frame_locked` matching source's exact
adjacent order.

`dword_4EEB54` and `dword_4EEB5C` remain genuinely unidentified -- no
access-site evidence found for either this round.

This is a significant architectural finding in its own right, joining
`sub_42B394`/`cc_run_code` and `offset_over_inv`/`GetInvAt` as another
case of "one big pre-refactor 2002 function, later split into several
smaller 2011 ones" -- specifically, `unload_old_room()` didn't exist as
its own function yet; its entire body lived inside `EndSkippingUntilCharStops`.
Formalized as a documented "GameState island 2" comment block in
`apply_structs.py` (not a formal struct member list, since it's
internally contiguous with itself but still not proven contiguous with
`play`'s own base -- see the previous round's `ifnum` discussion),
alongside `offsets_locked` and (at the time) `screen_tint`, both then
believed to be standalone globals outside GameState's own range -- an
address-calculation mistake corrected several rounds later; see that
correction for the full story.

## A self-correction: sub_409A9C is cancel_all_scripts, not save_room_data_segment -- and it turns out this project had already half-solved it

Immediate follow-up to the previous round, chasing down the "not yet
confirmed" `save_room_data_segment` guess for `sub_409A9C` rather than
leaving it as a loose thread. Reading the function's actual body instead
of just its call position disproved the guess cleanly: it's a loop over
an array with a `0x6C`(108)-byte stride, dispatching to one of two small
helper functions based on a per-element flag, then zeroing another
per-element field -- nothing like `save_room_data_segment`'s single
free/malloc/memcpy pair.

The stride (108 bytes) immediately identified the array as `scripts[]`,
already fully confirmed as `ExecutingScript` in a much earlier round.
Matching each accessed field against that already-known layout, plus
reading the two small dispatched helpers, closed the whole thing out
as an exact algorithmic match to `cancel_all_scripts()`
(`AC.CPP:3078-3091`):

```
for (aa=0; aa<num_scripts; aa++) {
    if (scripts[aa].forked) ccAbortAndDestroyInstance(scripts[aa].inst);
    else ccAbortInstance(scripts[aa].inst);
    scripts[aa].numanother = 0;
}
num_scripts = 0;
```

- `dword_523150` = **`num_scripts`** (new global match) -- both the loop
  bound and the final reset-to-0 target.
- `dword_4CC848[idx*0x6C]` = `scripts[idx].inst` (already-confirmed
  `ExecutingScript.inst`@`+0x00`) -- this also PINS DOWN `scripts[]`'s
  actual base address as `0x4CC848` for the first time.
- `byte_4CC8B0[idx*0x6C]` = `scripts[idx].forked` -- relative offset
  `0x4CC8B0-0x4CC848=0x68`, landing EXACTLY on the already-confirmed
  `ExecutingScript.forked`@`+0x68`, a third independent confirmation of
  that field (after the original struct-completion round and
  `post_script_cleanup`'s own bulk-copy evidence).
- `dword_4CC8A8[idx*0x6C]` = `scripts[idx].numanother` -- relative offset
  `0x4CC8A8-0x4CC848=0x60`, landing EXACTLY on the already-confirmed
  `ExecutingScript.numanother`@`+0x60`, likewise a further independent
  confirmation.
- The two dispatched helpers, `sub_42C270`/`sub_42C24C`, are themselves
  exact matches to `ccAbortAndDestroyInstance`/`ccAbortInstance`
  (`CSRUN.CPP:1991-2003`) -- both trivial one-line-body functions using
  2011's own `INSTF_ABORTED=2`/`INSTF_FREE=4` constants directly against
  the already-confirmed `ccInstance.flags`@`+0x00` and `.pc`@`+0x99C`.

All five identifications are HIGH confidence, and three of them
(`num_scripts`, `cancel_all_scripts`, the two `ccAbort*` functions) are
brand new -- this thread paid off well beyond fixing the original mistake.
Worth flagging as a process note: `sub_409A9C`'s behavior had ALREADY been
described accurately, in passing, inside `post_script_cleanup`'s own
`matches.json` evidence from a much earlier round (as supporting context
for the `ExecutingScript` struct-completion work) -- it just never got its
own dedicated entry, so the wrong `save_room_data_segment` guess didn't
get cross-checked against it before being written down. A quick
`grep -n "sub_409A9C" matches.json` before guessing would have caught this
immediately; worth doing that check FIRST next time a helper function's
identity is being guessed from its call position alone.

## GameState island 2 is now FULLY MAPPED: fast_forward and bg_anim_delay close the last gaps

Closed the two fields left genuinely unidentified across the last two
rounds (`dword_4EEB54`, `dword_4EEB5C`), completing every single dword/word
from `in_cutscene` through the start of `globalscriptvars` with zero gaps.

`dword_4EEB54` is checked at ~14 separate sites throughout the whole
binary -- immediately suspicious of being a heavily-reused flag rather
than something obscure. The first XREF checked (`FadeOut`, already
matched) bails out immediately: `if (dword_4EEB54 != 0) return;` --
exactly AGS's extremely common `if (play.fast_forward) return;`
early-bailout idiom, used 41 separate times in `AC.CPP` alone (e.g.
`write_screen()`'s own identical first-line gate, `AC.CPP:2776-2777`).
The identification is reinforced positionally too: `dword_4EEB54` sits
with ZERO gap immediately after the already-confirmed
`in_cutscene`@`dword_4EEB50`, matching 2011's own declared adjacency
(`int in_cutscene; int fast_forward;`, `acruntim.h:496-497`) exactly.

`dword_4EEB5C`'s lead came from `mainloop` (already matched): a
background-animation-frame-advance block matches 2011's `if
(play.bg_anim_delay>0) play.bg_anim_delay--; else if (play.bg_frame_locked)
; else { play.bg_anim_delay=play.anim_background_speed; play.bg_frame++;
if (play.bg_frame>=thisroom.num_bscenes) play.bg_frame=0; ...}`
(`AC.CPP:25569-25575`) instruction for instruction. This closes
`dword_4EEB5C` as `bg_anim_delay`, and pays off three further ways at
once:

- **Reconfirms `dword_4EEB70`=`bg_frame_locked` a second independent way**
  (now triply confirmed, across `sub_40AAE3`'s zero-write, its own
  positional adjacency to `fade_effect`, and this gate).
- **Upgrades `dword_4EEB58`=`bg_frame` from medium-high to HIGH
  confidence** -- the same source line pair (`play.bg_frame++; if
  (play.bg_frame>=thisroom.num_bscenes) play.bg_frame=0;`) matches the
  disassembly's increment-and-wrap exactly, an independent confirmation
  beyond the previous round's zero-write evidence.
- **Two bonus globals identified**, both standalone (well outside either
  GameState island): `dword_52308C` = `GameState.anim_background_speed`,
  and `dword_523088` = `roomstruct.num_bscenes` (the current room's own
  background-frame count -- a `roomstruct` field, not GameState; noted
  but not chased further this round).

With this, `apply_structs.py`'s "GameState island 2" comment block is
marked FULLY MAPPED -- every field from `in_cutscene` through
`globalscriptvars`'s start is now identified with a real access-site
confirmation, no positional-only guesses remaining in that stretch. The
only field left genuinely open across all of GameState's confirmed
territory is the small 3-dword gap between `inv_numinline` and
`inv_item_wid` (`text_speed`/`sierra_inv_color`/`talkanim_speed`,
positional-only) -- and, separately, the still-unresolved structural
question of whether island 1 (`score`..`inv_item_hit`) and island 2 are
truly the same contiguous struct object or independent 2002 globals
later consolidated by 2011.

## MAJOR CORRECTION: GameState is one contiguous 2404-byte struct after all -- the `ifnum` puzzle resolved

The structural question left open across the last three GameState rounds
-- whether the `score`-prefix ("island 1") and the `in_cutscene`-onward
run ("island 2") are the same contiguous object or independent standalone
2002 globals -- is now resolved, and the earlier conclusion was **wrong**.

The proof came from `SaveGameSlot` (already matched), read while looking
for a size anchor rather than more individual field leads. It writes
`play` directly to the save file with a literal size constant:

```
push ElementCount=1
push ElementSize=964h    ; 2404 bytes
push offset play
call fwrite
```

`0x964` = 2404 bytes. Computing `play`'s base (`0x4EEA18`, established in
the very first fresh-survey round) plus this size lands at `0x4EF37C` --
and that address is EXACTLY where an unrelated global, `String1`, begins
(with 36 bytes of untouched `db ?` filler immediately before it). Zero
slack, from an entirely independent direction than any of the field-level
evidence gathered so far. `GameState` really is one contiguous 2404-byte
object, full stop.

That leaves the original puzzle to explain: how could `ifnum` (address
`0x4EEB28`, which sits squarely inside this proven range at relative
offset `+0x110`) be a genuinely separate global, if nothing can interrupt
a real C struct's own memory? Answer: **it can't, and it isn't.** Reading
`ifnum`'s actual usage sites properly (rather than trusting the pre-
existing label) showed two DIFFERENT things sharing that name in this
disassembly -- a genuine local parameter inside `draw_text_window`
(`ebp+0x20`, matching 2011's actual `int ifnum=-1` parameter), and a
SEPARATE GLOBAL, also displayed as `ifnum` by IDA, that callers read to
decide what to pass as that parameter. The global's own body (inside
`main`, already matched) is:

```
ifnum = game_options[OPT_TWCUSTOM];
if (ifnum == 0)
    ifnum = -1;
```

matching 2011's `play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
if (play.speech_textwindow_gui==0) play.speech_textwindow_gui=-1;`
(`AC.CPP:26389-26391`) exactly. The global mislabeled `ifnum` IS
`GameState.speech_textwindow_gui` -- almost certainly a naming mistake
from whichever earlier pass applied "ifnum" to it, most likely by
association with the same-role 2011 parameter rather than any real
evidence. Its computed offset (`+0x110`) lines up exactly with 2011's
declared position too: 3 fields after `inv_item_hit`
(`speech_text_shadow`, `swap_portrait_side`, `speech_textwindow_gui`),
matching the confirmed 8-byte gap immediately before it with zero slack.

This also means two OTHER fields dismissed as "standalone, far from
GameState's range" in earlier rounds need re-examination, now that a real
total-size anchor exists to check against instead of eyeballing address
distance:

- `play_invorder` (this build's inventory-order array, `update_invorder`'s
  evidence): computed offset from `play`'s base is `+0x614` -- inside the
  proven 2404-byte range, just 8 bytes past `globalscriptvars`'s own end
  (`+0x60C`). Likely genuinely part of `GameState`, contradicting the
  earlier "genuinely separate" call.
- `word_4EF236`/`offsets_locked` (`sub_40AAE3`'s zero-write evidence):
  computed offset `+0x81E` -- also inside the proven range.
- `play_scren_tint`/`screen_tint` (`TintScreen`'s evidence) was believed
  at this point to compute to `+0x10C6C`, genuinely far outside
  GameState's bounds -- that computation turned out to be WRONG (a
  `grep -n` line number mistaken for a memory address, never cross-
  checked against neighboring self-encoding labels the way it should
  have been). Its real address, found and corrected many rounds later,
  is `+0x8AC` -- well within the struct after all. See the dedicated
  correction round further below for the full story.

`apply_structs.py`'s `GameState` struct has been rebuilt as ONE unified
definition reflecting all of this: the previously-separate "island 2"
comment block is now real struct members, `speech_textwindow_gui` closes
another gap, and the struct is padded out to its now-PROVEN total size of
`0x964` bytes with two explicitly-labeled unexplored regions (`+0x114`
-`+0x138`, 36 bytes; `+0x60C`-`+0x964`, 856 bytes -- the second one
containing `play_invorder`/`offsets_locked` at known-but-unverified-
against-neighbors relative positions, not yet promoted to typed struct
members pending further mapping of the territory around them).

A process lesson worth keeping for next time: the original "`ifnum`
proves discontiguity" conclusion was built entirely on an ADDRESS
appearing inconvenient, without ever checking whether a hard total-size
anchor existed to test it against. A `fwrite`/`malloc`/similar literal
size constant, when one exists, is far more authoritative than eyeballing
whether two addresses "look far apart" -- worth actively hunting for one
before concluding two regions are separate objects, not just after.

## Three more fields close in the +0x114..0x138 gap: totalscore, max_dialogoption_width, no_hicolor_fadein

A direct follow-up on the just-closed structural question -- with GameState
now known to be one real object, the previously "genuinely unexplored"
36-byte gap between `speech_textwindow_gui` and `in_cutscene` became worth
mining properly instead of writing off. Checked each of its 9 dwords'
XREF'd callers in turn.

- **`dword_4EEB30` = `totalscore`**@`+0x118`, high confidence: `replace_macro_tokens`
  (already matched) reads it for BOTH its "totalscore" and "scoretext"
  macro branches, matching 2011's `#define MAXSCORE play.totalscore`
  (`acruntim.h:809`) -- 2011's own source at those exact call sites uses
  the macro rather than the field name, but the definition makes this
  unambiguous.
- **`dword_4EEB48` = `max_dialogoption_width`**@`+0x130`, high confidence:
  `do_conversation` (already matched) computes `wii = dword_4EEB48 *
  current_screen_resolution_multiplier_x` inside its is_textwindow-
  equivalent branch, matching 2011's `areawid =
  multiply_up_coordinate(play.max_dialogoption_width);`
  (`AC.CPP:22119`) exactly in role and context.
- **`dword_4EEB4C` = `no_hicolor_fadein`**@`+0x134`, medium-high confidence:
  an unmatched helper (`sub_40A6D8`, called from `FadeIn`/`process_event`,
  both already matched) gates a hi-color-depth-specific fast path on this
  flag, matching 2011's `no_hicolor_fadein` role (`AC.CPP:3489-3501`)
  closely but not as a clean line-for-line structural match -- this
  build dispatches to a distinct helper rather than an early return
  before a shared tail call, hence medium-high rather than high.

Five dwords in this gap (`dword_4EEB34`/`38`/`3C`/`40`/`44`) remain
genuinely unidentified -- their callers (an `_display_at`-adjacent
function, `update_stuff`, `post_script_cleanup`, `check_controls`) were
checked but none gave a clean match this round. `dword_4EEB44` has a
plausible but unconfirmed `skip_display`-role lead (set to `2` under a
`byte_513340==2` gate) worth a closer read later. `dword_4EEB2C`
(negated into a walk-target-adjacent field inside `update_stuff`) is
similarly unresolved. `apply_structs.py` reflects this precisely: two
small `_pad_unknownN` blocks (4 bytes, 20 bytes) bracket the three newly
confirmed fields rather than one undifferentiated 36-byte pad.

## roomscript_finished, used_inv_on, and two bonus globals -- more of the +0x114..0x138 gap closes

Continued mining the remaining 5 unidentified dwords from the previous
round's partial close of GameState's `+0x114..+0x138` gap, reading each
XREF'd caller in turn.

- **`dword_4EEB3C` = `roomscript_finished`**@`+0x124`, high confidence:
  `post_script_cleanup`'s `runnext[0]=='$'` branch does
  `run_text_script_iparam(roominst,...); dword_4EEB3C=1;`, matching 2011's
  `run_text_script_iparam(roominst,&runnext[1],...); play.roomscript_finished
  = 1;` (`AC.CPP:3179-3181`) exactly. Bonus: identifies `dword_523138` as
  the global `roominst`.
- **`dword_4EEB40` = `used_inv_on`**@`+0x128`, high confidence:
  `check_controls`'s `GOBJ_INVENTORY` click branch computes two mouse-
  offset values, calls `sub_40D80C` (the `offset_over_inv` twin from
  earlier rounds), and stores the non-negative result here -- matching
  2011's `mouse_ifacebut_xoffs=...; mouse_ifacebut_yoffs=...; int
  iit=offset_over_inv(...); if (iit>=0) { ...; play.used_inv_on = iit; }`
  (`AC.CPP:5705-5710`) exactly. This is ALSO a further independent
  confirmation that `sub_40D80C` really is this build's `offset_over_inv`
  equivalent -- the third round in a row this exact identification has
  paid off from a completely different caller. Bonus: identifies
  `dword_4B4234`/`dword_4B4238` (the mystery mouse-relative-position
  globals flagged as unidentified two rounds ago, when `sub_40D80C` was
  first characterized) as the standalone globals `mouse_ifacebut_xoffs`/
  `mouse_ifacebut_yoffs` (`AC.CPP:609`).
- **`dword_4EEB34`@`+0x11C`**, MEDIUM confidence only: `_display_main`
  checks this against 0/2/3 in a message-box wait loop, generally
  consistent with `skip_display`'s role and small-int-enum value space,
  but without a clean line-for-line match to a specific 2011 function.
  This RETRACTS a weaker same-role guess for `dword_4EEB44` from the
  previous round (which had no real semantic tie beyond "gets set to
  2 somewhere") in favor of this closer-matching candidate.

`dword_4EEB2C`@`+0x114` and `dword_4EEB38`@`+0x120` remain genuinely
unidentified -- their callers (`update_stuff`, twice, for different
offsets) were read but didn't yield a clean match this round.
`dword_4EEB44`@`+0x12C` is now also back to unidentified, its earlier
weak candidate role withdrawn in favor of `dword_4EEB34` above.
`apply_structs.py` reflects this precisely: three single-dword pad
blocks now bracket the two newly confirmed fields and the medium-
confidence `skip_display` candidate, rather than one larger
undifferentiated pad.

## An important nuance to the "one contiguous struct" finding: the fwrite sweeps in more than just GameState

Chasing `play_invorder`'s exact capacity (left unconfirmed two rounds
ago) turned up something worth being careful about, rather than a clean
further confirmation.

The lead itself paid off cleanly: the raw `.data` bytes immediately
after `play_invorder` run uninterrupted (zero other labels) for exactly
200 bytes before the next labeled global. 200 bytes / 2 bytes-per-short
= 100 entries -- landing exactly on `MAX_INV=100`, the same capacity
already established elsewhere in this project for `game.invinfo[100]`,
with zero drift. `play_invorder[100]` is now a settled capacity.

But the label immediately after that 200-byte span is where the
complication starts: `word_4EF0F4`, written by `prepare_characters_for_
drawing` (already matched). Reading that write site showed it's part of
THREE parallel 50-entry `short[]` arrays (`word_4EF0F4`/`word_4EF158`/
`word_4EF1BC`, each 100 bytes = 50 shorts apart), computing a percent-
scaled X, percent-scaled Y, and zoom percentage per character --
unmistakably a per-frame RENDER-TIME cache (recomputed fresh every draw
call), not anything that would ever need to survive a save/load.

The problem: this render-cache trio's address (`+0x6DC` from `play`'s
base) sits WELL WITHIN the 2404-byte span `SaveGameSlot`'s literal
`fwrite` constant proved two rounds ago -- the finding written up then as
"GameState is one contiguous struct, full stop." That conclusion now
needs a qualification: the fwrite's `0x964` size constant evidently
sweeps in MORE than just the true `GameState` struct. It also captures
adjacent-but-distinct `AC.CPP` file-scope globals (at minimum this
render-cache trio, quite possibly others) that the linker happened to
lay out contiguously right after `play`, most likely because the
original developer's literal size constant was never tightened to an
exact `sizeof(GameState)` as the struct's own declaration evolved.

**This does not undo the earlier fields already confirmed** --
`in_cutscene`, `speech_textwindow_gui`, `totalscore`, and everything else
established in this struct so far were confirmed by matching actual
BEHAVIOR against 2011 source, not merely by falling inside the fwrite's
byte range. Those stand. What it does mean: falling inside the fwrite's
span is necessary but not SUFFICIENT evidence on its own that something
is a genuine `GameState` member. The two remaining positional-only leads
sitting in the unmapped tail -- `play_invorder`@`+0x614` and
`offsets_locked`@`+0x81E` -- should be treated as open questions again,
not settled corrections, until they get real role-based confirmation or
the territory around them gets mapped closely enough to rule out the
same "coincidentally adjacent, not actually GameState" outcome the
render-cache trio turned out to be.

`apply_structs.py`'s tail pad comment has been rewritten to carry this
caution explicitly, so a future round doesn't accidentally promote
`play_invorder`/`offsets_locked` into typed struct members on the
strength of the (now-qualified) span argument alone.

## Correction: the "render-cache trio" is CharacterExtras.width/height/zoom, not scratch memory

An immediate follow-up on the previous round's nuance-finding -- chasing
`CharacterExtras` as a fresh target (flagged as a lead many rounds ago,
never investigated) walked straight back into `word_4EF0F4`/`word_4EF158`/
`word_4EF1BC`, the three arrays just written off last round as "per-frame
render scratch, definitely not save-worthy."

That characterization was too hasty. Reading `prepare_characters_for_
drawing`'s full computation showed `word_4EF1BC[idx]` (the "zoom" array)
is read with a defaulting-to-100-when-zero fallback that matches 2011's
`zoom_level = charextra[aa].zoom; if (zoom_level==0) zoom_level=100;`
(`AC.CPP:8309-8312`) exactly -- and that computed value then scales two
base sprite dimensions into `word_4EF0F4[idx]`/`word_4EF158[idx]`,
matching `scale_sprite_size(sppic, zoom_level, &newwidth, &newheight);
charextra[aa].width=newwidth; charextra[aa].height=newheight;`
(`AC.CPP:8392-8394`) exactly. `word_4EF1BC[idx]` also gets written back
after computation, matching source's own `charextra[aa].zoom=zoom_level;`
reassignment.

These are this build's actual **`CharacterExtras.width`/`.height`/
`.zoom`** fields (`Common/acruntim.h:441-455`) -- real, meaningful,
semi-persistent per-character state that 2011 still maintains today,
not throwaway scratch. The genuine architectural finding is HOW they're
laid out: this build implements what 2011 declares as one
`CharacterExtras charextra[50]` (array-of-structs) as THREE SEPARATE
PARALLEL `short[50]` arrays instead (structure-of-arrays) -- confirmed
via a clean, zero-interruption 100-byte span between each array with no
other labels breaking it. A bonus, medium-high-confidence lead: the two
base dimension arrays feeding the scale computation, `dword_4CD2E8[]`/
`dword_4E787C[]`, are plausibly the well-known AGS globals
`spritewidth[]`/`spriteheight[]`, both also referenced together from an
unmatched `SpriteCache::loadSprite`-adjacent helper.

This does NOT undo last round's core caution -- falling inside
`SaveGameSlot`'s fwrite span still isn't sufficient evidence of GameState
membership by itself, and that lesson is if anything reinforced here: the
span sweeps in not just scratch memory but a WHOLE SEPARATE real struct
(`CharacterExtras`) that happens to sit adjacent to `play` in the same
source file. `apply_structs.py`'s tail-pad comment and the
`prepare_characters_for_drawing` `matches.json` entry have both been
corrected to reflect this -- the render-cache characterization is
explicitly retracted in favor of the `CharacterExtras` identification,
and the new struct is documented as three parallel array declarations
(matching the real memory layout) rather than a single struct type.

## walkable_areas_on closes a real gap, and resolves offsets_locked's reopened question

A further pass over `EndSkippingUntilCharStops`/`unload_old_room`-combined
(`sub_40AAE3`, already matched many rounds ago and re-read several times
since) found one more piece that had gone unremarked in earlier reads:
`memset(&byte_4EF224, 1, 0x10)`, matching 2011's
`memset(&play.walkable_areas_on[0],1,MAX_WALK_AREAS+1);` (`AC.CPP:3623`)
exactly -- `MAX_WALK_AREAS=15` (`acroom.h:250`), so `MAX_WALK_AREAS+1=16=
0x10` with zero drift. `byte_4EF224[16]` is `GameState.walkable_areas_on`.

This closes more than just one field. `walkable_areas_on`'s own confirmed
end (`+0x81C` from `play`'s base) lands EXACTLY 2 bytes before
`offsets_locked`'s already-known address (`+0x81E`) -- matching 2011's
own declared adjacency `char walkable_areas_on[...]; short
screen_flipped; short offsets_locked;` (`acruntim.h:556-558`) with zero
slack for the one intervening field. This directly answers the question
reopened two rounds ago (after the `CharacterExtras` correction cast
doubt on "falls inside the fwrite span" as sufficient evidence):
`offsets_locked` now has TWO independent confirmations -- its own
original zero-write behavioral evidence, AND this new positional
adjacency to a separately, freshly role-confirmed field. That's enough
to treat it as settled again, not just reopened.

A plausible-but-unconfirmed `screen_flipped`@`+0x81C` fills the 2-byte
gap between them (positional-only, matching 2011's exact field count in
that span, no direct access-site evidence of its own).

Bonus, found in the same read: the `destroy_bitmap` call right at this
function's end, gated on `dword_523204` and followed by
`dword_523204=0`, matches source's `if (raw_saved_screen != NULL) {
wfreeblock(raw_saved_screen); raw_saved_screen = NULL; }`
(`AC.CPP:3628-3630`) -- identifies `dword_523204` as the global
`raw_saved_screen`.

`apply_structs.py`'s tail pad is now split into three pieces: a
512-byte pad (`+0x60C..+0x80C`, still containing at least the
`CharacterExtras` trio and possibly more unmapped territory),
`walkable_areas_on`/`screen_flipped`/`offsets_locked` as real fields,
and a final 324-byte pad (`+0x820..+0x964`) closing out the struct's
proven total size.

## GameState's tail closes completely: script_timers, sound_volume, speech_volume, raw_color, and the final field

The most productive single round on GameState's tail yet -- five new
fields, one of them closing the struct completely.

Chasing labeled globals in the second unmapped pad (`+0x820..+0x964`)
turned up `RawSetColor` (already matched, mechanical) doing
`dword_4EF350 = get_col8_lookup(this);`, an exact match to 2011's
`play.raw_color = get_col8_lookup(clr);` (`AC.CPP:14434`). That field
sits with zero gap immediately before the already-IDA-named
`play_filenumbers`, confirming 2011's declared trailing
`raw_modified[MAX_BSCENE]` is absent here -- no room for it at all.

`play_filenumbers`'s own capacity closed via `ListBoxSaveGameList`
(already matched): a sort/swap loop bounded by `cmp [var],14h` (20)
confirms 20 entries, matching `Engine/acdialog.h:870`'s
`MAXSAVEGAMES=20` (not `acruntim.h`'s own separate `MAXSAVEGAMES=50`
definition -- the disasm evidence is unambiguous). And here's the
payoff: `filenumbers[20]` (40 bytes) lands EXACTLY on GameState's own
independently-proven total size (`SaveGameSlot`'s fwrite
`ElementSize=0x964`) with zero remaining bytes. **This is GameState's
last field** -- the struct's tail is now closed completely, end to end,
from `+0x00` through `+0x964`.

Working backward from `script_timers` gave three more, all with the
same "zero gap between confirmed neighbors" over-determined pattern
this project keeps finding when a region is genuinely tightly packed:

- **`script_timers[21]`**@`+0x838`: `update_stuff`'s own OPENING lines
  (already matched, but not fully read until now) loop `for(chat=0;
  chat<0x15;chat++) if(dword_4EF250[chat]>1) dword_4EF250[chat]--;`,
  matching 2011's own opening lines `for (aa=0;aa<MAX_TIMERS;aa++) { if
  (play.script_timers[aa]>1) play.script_timers[aa]--; }`
  (`AC.CPP:6431-6433`) instruction for instruction, loop bound included
  (`MAX_TIMERS=21`).
- **`sound_volume`**@`+0x88C`: an unmatched helper (`sub_4089CC`, called
  from `PlayAmbientSound`/`SetSoundVolume`) computes `vol*sound_volume/
  255`, matching 2011's `ambientvol = (sourceVolume*play.sound_volume)/
  255;` (`AC.CPP:1567`) exactly. Sits with zero gap immediately after
  `script_timers`, matching 2011's exact declared adjacency.
- **`speech_volume`**@`+0x890`: an unmatched helper (`sub_4141B8`,
  called from `_display_at`) passes it as a volume argument to
  WAV-then-MP3 speech-loading helpers, matching 2011's `speechmp3=
  my_load_wave(finame,play.speech_volume,0); ...
  my_load_mp3(finame,play.speech_volume);` (`AC.CPP:13387-13396`)
  exactly. Sits with zero gap immediately after `sound_volume`.

`apply_structs.py`'s tail is now: a 24-byte pad, `script_timers[21]`,
`sound_volume`, `speech_volume`, a 164-byte pad (2011 declares a large
run of fields here -- `normal_font`/`speech_font` through
`parsed_words[]`/`bad_parsed_word[100]` -- almost certainly not all
present, not mapped this round), `raw_color`, and `filenumbers[20]`
closing the struct exactly at its proven total.

Two functions central to this round (`sub_4089CC`, `sub_4141B8`) are
still not matched to specific 2011 names -- the field identifications
riding on them are high confidence regardless, since they come from
exact algorithmic/role matches independent of the caller's own name.

## Six more fields close out both remaining pads: entered_edge, want_speech, stop_dialog_at_end, normal_font, speech_font, key_skip_wait

An immediate follow-up on the previous round, mining the two remaining
unmapped pads (`+0x820..+0x838` and `+0x894..+0x938`) rather than
declaring GameState "done enough." Both pads turned out to be full of
leads -- some genuinely new, some ALREADY sitting in the live IDB as
pre-existing names from prior manual work that had just never been
individually verified or connected to this struct.

**`load_new_room`'s room-entry-edge logic** (already matched, re-read
for a fourth or fifth time) sets a global to `-1` then `0`/`1`/`2`/`3` by
descending threshold checks against another global, matching 2011's
`play.entered_edge = -1; ... if (new_room_pos>=4000) play.entered_edge=3;
... >=1000: entered_edge=0;` (`AC.CPP:4453-4499`) exactly -- same
thresholds, same order. Identifies `entered_edge`@`+0x828` and, as a
bonus, the global `new_room_pos`. A second edge-detection block in the
same function turned up a genuine absence finding: 2011's companion
`play.entered_at_x=forchar->x; play.entered_at_y=forchar->y;`
(`AC.CPP:4539-4540`) has no counterpart here -- this build writes the
same transient value into the shared `tox`/`toy` scratch globals instead
of persisting it in dedicated fields. **`entered_at_x`/`entered_at_y` are
CONFIRMED ABSENT.**

Right next to `entered_edge`, two ALREADY-NAMED IDA globals turned out to
be exactly where they should be: `play_want_speech` (XREF'd from
`SetVoiceMode`, matching `AC.CPP:13500-13503` exactly) confirms
`want_speech`@`+0x82C`. `play_stop_dialog_at_end` (XREF'd from
`RunDialog`/`NewRoom`) is a plausible match for `stop_dialog_at_end`@
`+0x834` by name and role, but its POSITION here contradicts 2011's
declared order -- 2011 places `stop_dialog_at_end` much earlier, next to
`reserved[10]` near the "game."-exposed section boundary
(`acruntim.h:536`), not next to `want_speech`/`entered_edge`
(`acruntim.h:560-561`). Flagged as a genuine, not-yet-explained
architectural difference rather than silently assumed consistent. The
dword between `want_speech` and `stop_dialog_at_end` -- positionally
exactly where 2011 declares `cant_skip_speech` -- has real XREF activity
in `check_controls` but a `0<x<3` range check that doesn't read as a
simple boolean, so it's recorded as a positional candidate only, not
asserted.

**`SetNormalFont`** (already matched, mechanical) closes `normal_font`@
`+0x894` with an exact match including the verbatim error string. The
already-named global right after it, `fontid`, sits exactly where 2011
declares the paired `speech_font` and is XREF'd from text-drawing
functions consistent with that role -- but given this project's prior
experience with a pre-existing name turning out to be a mislabeled
artifact (`ifnum`/`speech_textwindow_gui`, several rounds ago), it's kept
at medium-high rather than high confidence pending direct behavioral
confirmation.

**`play_key_skip_wait`** (already an IDA-named global) gets upgraded from
an unverified name to a real behavioral confirmation via `check_controls`
(already matched): `if (play_wait_counter>0 && play_key_skip_wait>1)
play_wait_counter=0xFFFF;` matches 2011's `else if ((play.wait_counter >
0) && (play.key_skip_wait > 1))` (`AC.CPP:5742`) exactly.

`apply_structs.py`'s GameState struct is now fully broken into small,
precisely-sized pads (8 bytes, 152 bytes) plus real fields -- no
undifferentiated multi-hundred-byte gaps remain anywhere in the struct.

## MAJOR SELF-CAUGHT ERROR: screen_tint's address was a line-number/address mixup, not actually outside GameState -- and fixing it closes four more fields

While mining the last remaining pad, checking a global right next to
`screen_tint`'s (believed) neighbor turned up a serious problem: `play_want_music`
(XREF'd from `IsMusicVoxAvailable`/`PlayMusic`) sat at an address computed
by straightforward sequential byte-counting from several self-encoding
neighbors (`dword_4EF2AC`, `dword_4EF2B8`, `dword_4EF2C0` -- each of
these labels has its own real address literally encoded in its name, a
strong cross-check), landing at `+0x8A4` from `play`'s base -- and
`screen_tint` sits just a few bytes past it in the SAME sequential run.
That directly contradicts the long-standing claim (from the round that
proved GameState is one contiguous struct) that `screen_tint` computes
to `+0x10C6C`, "genuinely far outside GameState's bounds."

Chasing the discrepancy down: `screen_tint`'s address had originally been
found via `grep -n "^play_scren_tint\b" rob_blanc_1.asm`, which returns
`500204:play_scren_tint dd ? ...` -- and `500204` was then used directly
as `0x500204`, the hex memory address. **That number is the grep LINE
NUMBER, not a memory address.** Every other custom-named global in this
project was correctly grounded by cross-referencing its position against
a neighboring label whose name encodes its own real address (IDA's
default `dword_XXXXXX`/`word_XXXXXX`/`byte_XXXXXX` auto-naming, generated
directly from the true address) -- this one specific case skipped that
check and used the grep line number instead, undetected for several
rounds because nothing else depended on being cross-verified against it
until now.

Re-deriving `screen_tint`'s TRUE address properly (sequential byte count
from three independent self-encoding neighbors: `dword_4EF2AC` ->
`fontid` -> `play_key_skip_wait` -> align padding -> `dword_4EF2B8` ->
`play_want_music` -> `dword_4EF2C0` -> `play_scren_tint`) gives `0x4EF2C4`
-- offset `+0x8AC` from `play`'s base, comfortably inside the proven
2404-byte struct. Better still, this lines up EXACTLY with 2011's own
declared field order: `swap_portrait_lastchar; seperate_music_lib;
in_conversation; screen_tint;` (`acruntim.h`) -- and each of those three
preceding fields turned out to have real, independently-confirmable
evidence right there waiting to be read:

- **`swap_portrait_lastchar`**@`+0x8A0` and **`swap_portrait_side`**@
  `+0x10C` (a MUCH earlier field, upgraded from a many-rounds-old
  TENTATIVE guess) both close via the SAME evidence: `_displayspeech`
  (already matched) does `if (dword_4EF2B8 != xx) { if
  (dword_4EEB24==1) dword_4EEB24=2; else if (dword_4EEB24==2)
  dword_4EEB24=1; ...; dword_4EF2B8=xx; }`, matching 2011's `if
  (play.swap_portrait_lastchar != aschar) { ...toggle
  play.swap_portrait_side...; play.swap_portrait_lastchar=ce; }`
  (`AC.CPP:13697-13721`) exactly. 2011 genuinely declares these two
  related fields far apart in the struct, so finding them at two very
  different offsets is not itself surprising -- once each individually
  had real evidence.
- **`seperate_music_lib`**@`+0x8A4` -- and a THIRD mislabeled pre-
  existing global found in this project (after `ifnum`): `play_want_music`
  is a misleading name. `IsMusicVoxAvailable`'s one-line body, `return
  play_want_music;`, matches 2011's `return play.seperate_music_lib;`
  (`AC.CPP:13512-13514`) exactly -- there is no "want_music" field in
  2011's source at all.
- **`in_conversation`**@`+0x8A8` -- `do_conversation` (already matched)
  increments this global as literally its first statement, matching
  2011's `play.in_conversation++;` (`RunDialog`, `AC.CPP:21955`) exactly.

All four fields close with zero gap against each other, exactly matching
2011's declared adjacency across the whole run. `apply_structs.py` has
been corrected throughout: the wrong `+0x10C6C` claim and its "confirmed
absent"/"standalone global" framing are replaced with the real address
and the four newly-confirmed fields; `matches.json`'s
`EndSkippingUntilCharStops` entry carries the same correction.

**Process lesson, stated plainly for next time**: a `grep -n` line
number and a hex memory address can look superficially similar and are
easy to conflate under momentum. Every custom-named (non-auto-generated)
global's address needs to be grounded by cross-referencing its position
against a neighboring label whose name itself encodes a real address --
never trust a bare number pulled from search output without checking
what it actually represents.

## bad_parsed_word closes almost the last remaining pad, and confirms num_parsed_words/parsed_words[] are absent

A direct follow-up on the `screen_tint` correction round -- checking what
sits immediately after the newly-fixed `screen_tint`/`in_conversation`
cluster turned up two more already-IDA-named globals, `comparetonum`
and `compareto`, XREF'd from `ParseText`/`Said` -- core text-parser
state, unrelated to GameState. Initially this looked like a dead end
(another case of adjacent-but-unrelated globals, like the
`CharacterExtras` trio two rounds ago), but reading a bit further along
the same stretch found `byte_4EF2EA`, XREF'd from `SaidUnknownWord`
(already matched, mechanical).

`SaidUnknownWord`'s body matches 2011's `int SaidUnknownWord(char*buffer)
{ strcpy(buffer, play.bad_parsed_word); if (play.bad_parsed_word[0]==0)
... }` (`AC.CPP:18038-18041`) exactly -- `byte_4EF2EA` is the start of
`GameState.bad_parsed_word[100]`. Its confirmed end lands with a clean,
EXPECTED 2-byte compiler-alignment gap before the already-confirmed
`raw_color` (`0x4EF2EA` isn't itself 4-byte aligned, so a 100-byte array
starting there ends 2 bytes short of the boundary an `int` field needs)
-- a strong positional fit reinforcing the role match, the same kind of
"lands exactly where the next confirmed field begins" evidence this
project has repeatedly found compelling.

This also settles a question implicitly: 2011 declares `num_parsed_words`
and `parsed_words[MAX_PARSED_WORDS]` immediately BEFORE `bad_parsed_word`
in its own struct. Since the 34 bytes actually occupying that position
here belong to the unrelated parser globals (`comparetonum`/`compareto`),
those two fields are **CONFIRMED ABSENT** -- there's no room for them,
and whatever storage this build's text parser used for a running word
count/word list, it wasn't embedded in `GameState`.

`apply_structs.py`'s tail pad shrinks accordingly: a 34-byte confirmed-
non-GameState pad, `bad_parsed_word[100]`, a 2-byte alignment pad, then
straight into the already-confirmed `raw_color`/`filenumbers[20]` close.
The only remaining unmapped GameState territory of any real size is now
the `+0x60C..+0x80C` pad (containing `CharacterExtras`, and possibly
more) and the small `+0x114..+0x138` stretch's three still-unidentified
dwords.

## The +0x60C..+0x80C pad is now fully byte-accounted for, even where not every piece is named

Gave the remaining small-gap dwords (`dword_4EEB2C`/`4EEB38`/`4EEB44`)
one more attempted pass this round -- `update_stuff`'s full gating
context for `dword_4EEB38` (a large object-animation-frame-computation
block, `ViewStruct272`-adjacent) and `sub_4141B8`'s tail (a
`dword_4EF220` countdown decremented by 60, feeding into a check on
`dword_4EEB44` gated by a shared mode byte, `byte_513340`) both got
read in full. Neither yielded a clean match to a specific 2011 field --
recorded as genuinely hard cases rather than forced, after four rounds
of attempts on the same three dwords now.

Redirected the round's remaining effort into precisely characterizing
the big `+0x60C..+0x80C` pad instead of leaving it as one
undifferentiated 512-byte unknown. It turns out to be COMPLETELY
byte-accounted for, even though not every piece has a name:

- `+0x60C..+0x614` (8 bytes): `dword_4EF024`/`dword_4EF028`, XREF'd
  from `PlayVideo` and a small helper cluster (`sub_408356`/
  `sub_418E82`) that looks music/video-parameter-related but isn't
  confirmed.
- `+0x614..+0x6DC` (200 bytes): `play_invorder` -- role and capacity
  both confirmed, but GameState membership is genuinely unresolved
  (neither neighbor is itself a confirmed GameState field, so there's
  no positional evidence to lean on the way there was for
  `bad_parsed_word`/`screen_tint`).
- `+0x6DC..+0x808` (300 bytes): `CharacterExtras.width`/`.height`/
  `.zoom`, CONFIRMED NOT GameState (a whole separate struct, already
  fully written up).
- `+0x808..+0x80C` (4 bytes): `dword_4EF220`, the countdown just
  investigated -- unidentified.

`apply_structs.py` now reflects this precisely: four correctly-sized
pad pieces (using neutral `char _pad_X[size]` declarations, not typed
fields with suggestive names, for anything not actually confirmed --
including `play_invorder`, whose earlier draft in this same round
briefly declared it as a real typed array before being corrected back
to a neutral pad to stay consistent with how every other unconfirmed
region in this struct is represented) replace the single
undifferentiated `_pad_unexplored2[0x200]`. Nothing about GameState's
CONTENT changed this round -- this is a clarity/precision improvement,
making explicit exactly how much of the struct's byte range is truly
still open (very little) versus merely unnamed-but-understood.

## text_speed, sierra_inv_color, and talkanim_speed close -- the last purely-positional guesses in GameState's early section

The three fields that had sat as pure positional inference since the
very first GameState round (`+0xF4..+0xFC`, boxed in by nothing more
than "2011 declares exactly 3 fields in this gap") all close this round
with real evidence, via two different but complementary techniques.

**`sierra_inv_color`**@`+0xF8` came from reading `__actual_invscreen`
(already matched) -- an early lead ("wsetcolor"/"wbar" as literal
identifiers don't appear in this disassembly, since they're Allegro/
Wgt2 wrapper calls) turned into a clean win once the actual call
sequence was read: `push dword_4EEB10; call sub_40187F;` right before
the inventory-window background gets drawn, matching 2011's
`wsetcolor(play.sierra_inv_color); wbar(windowxp,windowyp,...);`
(`AC.CPP:23916-23917`) exactly in shape.

**`text_speed`**@`+0xF4` and **`talkanim_speed`**@`+0xFC` both closed
via a technique used sparingly before but worth naming explicitly:
grepping a candidate global for ALL its code XREFs (not just role-
matching one caller) turned up, for both fields, an EXACT MATCH to
2011's own game-startup init literal value (`text_speed=15`,
`talkanim_speed=5`) alongside a separate, independent ROLE-matching use
site. For `text_speed`, the role site is a text-display-duration
calculation (`(strlen/text_speed+1)*fps`) matching 2011's now-more-
complex version stripped down to its 2002 essentials. For
`talkanim_speed`, the role site is more interesting: 2011's OWN source
only ever assigns this field once, at init, and never reads it again --
this build actively reads and uses it (packed into a `CharacterInfo`
field via the classic AGS packed-value idiom, `(speed<<8)|flags`, when
starting a talk animation), another case -- like `inv_numorder` several
rounds ago -- of a field 2011 kept declared but stopped actively using.

This closes out GameState's early `+0xF4..+0xFC` stretch completely --
every field from `+0x00` through `+0x104` now has real behavioral
evidence, none of it purely positional anymore.

## The init block: one ~65-instruction block confirms or closes nearly everything left in GameState

The single most productive block found in this entire GameState
investigation. Chasing exact-init-value confirmations for the remaining
tentative fields (following the pattern that closed `text_speed`/
`sierra_inv_color`/`talkanim_speed` last round) led straight into a
massive sequential initialization block inside `main` -- this build's
version of 2011's separate `init_game_settings()`, inlined directly
into `main` rather than split out (the same "monolithic pre-refactor"
pattern found repeatedly throughout this project). It sets roughly 40
`GameState` fields to literal values in a row, matching 2011's own init
sequence (`AC.CPP:26277-26394`) almost line for line.

**Three fields that had resisted identification across FIVE separate
rounds finally close:**

- `dword_4EEB2C` = **`follow_change_room_timer`**@`+0x114`: set to
  `0x96`(150), matching `play.follow_change_room_timer = 150;`
  (`AC.CPP:26394`) exactly.
- `dword_4EEB38` = **`no_multiloop_repeat`**@`+0x120`: set to `0`, in
  the same sequential init-code position 2011 uses (immediately after
  `skip_display`), matching `play.no_multiloop_repeat = 0;`
  (`AC.CPP:26345`).
- `dword_4EEB44` = **`no_textbg_when_voice`**@`+0x12C`: set to `0`,
  immediately after `roomscript_finished` in the init sequence,
  matching `play.no_textbg_when_voice = 0;` (`AC.CPP:26350`). This
  RETRACTS an earlier round's guess that this field might be
  `skip_display` -- the real `skip_display` is confirmed elsewhere in
  the same block (see below).

**Every remaining tentative field also closes:**

- `dword_4EEB20` = **`speech_text_shadow`**@`+0x108`: set to
  `0x10`(16), matching `play.speech_text_shadow = 16;`
  (`AC.CPP:26338`) -- and independently confirmed via role too (an
  unmatched helper, `sub_413635`, reads it right before a `wtextcolor`-
  equivalent call, matching `wtextcolor(play.speech_text_shadow);`,
  `AC.CPP:12616`/`12621`).
- `word_4EF234` = **`screen_flipped`**@`+0x81C`: set to `0`, matching
  `play.screen_flipped=0;` (`AC.CPP:26331`) -- closing the field that
  had sat as pure positional inference (the 2-byte gap between
  `walkable_areas_on` and `offsets_locked`) since the round that
  confirmed `walkable_areas_on`.
- `fontid` = **`speech_font`**@`+0x898`: set to `1`, matching
  `play.speech_font = 1;` (`AC.CPP:26337`) exactly -- resolving the
  standing caution that this pre-existing name might be another
  mislabeling artifact like `ifnum`/`play_want_music`. It genuinely is
  what its name says.
- `dword_4EEB34` = **`skip_display`**@`+0x11C`: set to `3`, matching
  `play.skip_display = 3;` (`AC.CPP:26344`) exactly -- upgrading this
  field from medium confidence (a plausible but not fully clean role
  match from `_display_main`) to high.
- `dword_4EF248` = **`cant_skip_speech`**@`+0x830` (medium-high, not
  fully closed): set via `movsx ecx, byte_51333D; dword_4EF248=ecx` --
  a COMPUTED value read from a game-options byte, matching the SHAPE of
  `play.cant_skip_speech = user_to_internal_skip_speech(game.options
  [OPT_NOSKIPTEXT]);` (`AC.CPP:26333`) rather than 2011's literal
  conversion-function call being visibly present -- reinforcing but not
  fully closing this one.

**Roughly 25 more already-confirmed fields get exact-value bonus
reconfirmations** in the same block: `sierra_inv_color`=7,
`talkanim_speed`=5, `inv_item_wid`=40, `inv_item_hit`=22,
`messagetime`=-1, `disabled_user_interface`=0, `gscript_timer`=-1,
`inv_top`=0, `inv_numdisp`=0, `inv_numorder`=0, `text_speed`=15,
`bg_frame`=0, `bg_frame_locked`=0, `bg_anim_delay`=0, `wait_counter`=0,
`key_skip_wait`=0, `sound_volume`=255, `speech_volume`=255,
`normal_font`=0, `screen_tint`=-1, `bad_parsed_word[0]`=0,
`swap_portrait_side`=0, `swap_portrait_lastchar`=-1,
`in_conversation`=0, `in_cutscene`=0, `fast_forward`=0,
`roomscript_finished`=0, `no_hicolor_fadein`=0 -- plus one bonus
global: `dword_51B84C` (read right before `totalscore`'s own
assignment) = `game.totalscore`, matching `play.totalscore =
game.totalscore;` (`AC.CPP:26348`) exactly, and `dword_4EEB48`=`0xB4`
(180) reconfirms `max_dialogoption_width` against 2011's literal
180-constant (`get_fixed_pixel_size(180)`, before scaling).

With this, GameState's ENTIRE early section (`+0x00` through `+0x158`)
and most of the tail have real, mostly-exact-value-confirmed evidence.
Genuinely remaining open items are now down to: `play_invorder`'s
GameState membership question, the still-unidentified `dword_4EF024`/
`dword_4EF028`/`dword_4EF220` (though `dword_4EF220`'s init value,
`0xA0`(160), and `dword_4EF028`'s, `1`, are now at least KNOWN even if
their roles aren't), and precisely mapping the remaining large unknown
pads. The struct's field-level identification work is, for all
practical purposes, essentially complete.

## sub_40A6D8 read in full: a genuinely different fade-out technique, not just a refactor

A follow-up on `no_hicolor_fadein`'s helper function (`sub_40A6D8`,
left unnamed several rounds ago) -- reading its full body (rather than
just the flag-gated dispatch at its top) turned up a real architectural
finding rather than a clean rename.

2011's `highcolor_fade_out()` (`Engine/ali3dsw.cpp:632-670`) uses
Allegro's alpha-blending API (`set_trans_blender`/`draw_trans_sprite`)
to composite a progressively-more-opaque black overlay over a captured
screen copy. `sub_40A6D8` does the SAME conceptual job -- a high-color-
depth screen fade-out, gated behind the same `no_hicolor_fadein` flag,
called from the same `FadeIn`/`process_event` context -- but via a
completely different, more primitive technique: a manual pixel-by-pixel
darkening loop over the entire screen, extracting and scaling down each
pixel's R/G/B channels directly, for a fixed 64 steps. This predates
Allegro's alpha-blending API entirely, not merely a different arrangement
of the same primitives.

Consistent with this project's established convention for this exact
situation (`sub_42B394`/`cc_run_code` being the clearest earlier
example), `sub_40A6D8` is deliberately NOT renamed to
`highcolor_fade_out` -- the role match is solid, but the implementation
diverges too much to claim a 1:1 correspondence. Documented in its
`matches.json` entry instead, alongside the already-confirmed
`no_hicolor_fadein` field evidence it was originally found through.

## Fresh survey: ScreenOverlay -- a clean, complete recovery in a single round

Picked a new target after GameState's field-level work wound down.
`add_screen_overlay` (already matched, `AC.CPP:3451-3474`) turned out to
be an ideal candidate: a short, self-contained construction function
that touches every field of a brand-new struct exactly once, in
sequence, with no ambiguity.

Reading its full body (only the error string had been matched before)
revealed this build's complete `ScreenOverlay` layout in one pass:

```
if (numscreenover>=10) quit("too many screen overlays created");
if (type==2) is_complete_overlay++;
if (type==1) is_text_overlay++;
if (type==0x64) { find an unused custom ID via find_overlay_of_type, 0x65..0xC8 }
screenover[numscreenover] = { pic=piccy, type=type, x=x, y=y, timeout=0 };
numscreenover++;
```

matching 2011's `OVER_TEXTMSG=1`/`OVER_COMPLETE=2`/`OVER_CUSTOM=100`
constants and the custom-ID search range (`OVER_CUSTOM+1` to
`OVER_CUSTOM+100`) exactly. The struct itself is a genuine array-of-
structs (unlike the `CharacterExtras` precedent immediately before it in
this file) with a `0x14`(20)-byte stride: `pic`@`+0x00`, `type`@`+0x04`,
`x`@`+0x08`, `y`@`+0x0C`, `timeout`@`+0x10` -- five fields, one per
assignment statement, zero guesswork.

**Confirmed absent** against 2011's current declaration
(`Common/acruntim.h:272-280`): `bmp` (`IDriverDependantBitmap*`, a later
hardware-acceleration abstraction this build predates -- the same
pattern already established for `CharacterInfo.actx`/`.acty`),
`bgSpeechForChar`, `associatedOverlayHandle`, `hasAlphaChannel`,
`positionRelativeToScreen`. This build's `ScreenOverlay` is exactly
2011's first 5 fields and nothing more.

**Drift**: capacity checked against a literal `10`, not 2011's declared
`MAX_SCREEN_OVERLAYS=20` (`acruntim.h:841`) -- the familiar 2x-reduction
pattern.

**Bonus finds in the same read**: `find_overlay_of_type` (new match,
`sub_40A0E9`, an exact instruction-for-instruction match to
`AC.CPP:3443-3449`), and three related globals -- `numscreenover`,
`is_complete_overlay`, `is_text_overlay` (the last one a second,
independent confirmation of a global that had already surfaced
incidentally during an earlier GameState round via `check_controls`,
without being identified at the time).

`ScreenOverlay` joins the small set of structs in this project closed
completely in a single round, with no open questions left -- the whole
struct, every field, confirmed via one function's construction
sequence.

## CreateGraphicOverlay: reconfirms ScreenOverlay, closes spritewidth/spriteheight, and two bonus function matches

A direct follow-up mining the overlay subsystem further. `CreateGraphicOverlay`
(already matched, but only via its own linker symbol -- never read in
full) turned out to be a clean, complete match to 2011's version
(`AC.CPP:13125-13138`) line for line:

```
create_bitmap_ex(final_col_dep, spritewidth[slott], spriteheight[slott]);
wsetscreen(screeno);
clear_to_color(screeno, bitmap_mask_color(screeno));
wputblock(0, 0, spriteset[slott], trans);
int nse = add_screen_overlay(xx, yy, OVER_CUSTOM, screeno, hasAlpha);
wsetscreen(virtual_screen);
return screenover[nse].type;
```

Two payoffs:

- **`dword_4CD2E8`/`dword_4E787C` = `spritewidth[]`/`spriteheight[]`**,
  upgraded from medium-high to HIGH confidence -- these were first
  suspected during the `CharacterExtras` round (width/height scaling by
  zoom percentage), and this is a second, independent usage context
  (passed directly as `create_bitmap_ex`'s height/width arguments)
  confirming the same identity from a completely different angle.
- The function's own tail, `imul ecx,0x14; mov eax,dword_4CD224[ecx]`,
  is a second independent confirmation of `ScreenOverlay.type`@`+0x04`
  (already confirmed via `add_screen_overlay`), matching `return
  screenover[nse].type;` exactly.

**Two bonus function matches** surfaced along the way: `sub_40177C` =
**`wputblock`** (`Common/Wgt2allg.h:413-419`) -- an exact
instruction-for-instruction match, `if (xray) draw_sprite(abuf,bll,xx,yy);
else blit(bll,abuf,0,0,xx,yy,bll->w,bll->h);` -- and, incidentally,
`sub_423E60` = **`draw_sprite`** (a well-known third-party Allegro API
function, not given its own dedicated entry, but its identity falls out
of `wputblock`'s own confirmed branch).

## CORRECTION: sub_409FD4 is remove_screen_overlay, not stop_fast_forwarding

While reading `RemoveOverlay`'s disassembly during the `ScreenOverlay`
round above, noticed it calls `sub_40A0E9` (already matched this round to
`find_overlay_of_type`) and then a SECOND function, passing the resolved
`ovrid` as an argument -- `sub_409FD4`. That second function had already
been "matched" to `stop_fast_forwarding` several rounds ago, but purely on
callgraph POSITION (it's the last thing `EndSkippingUntilCharStops` calls,
matching where 2011's source calls `stop_fast_forwarding()`) -- its own
body was never actually read at the time.

Reading `sub_409FD4` in full shows it is unmistakably `remove_screen_overlay
(int type)` with `remove_screen_overlay_index(int cc)` inlined
(`AC.CPP:3404-3441`), an exact, complete algorithmic match:

- Loops `numscreenover` entries looking for a type match (`type==-1` matches
  everything, matching source's "just remove everything" mode used by
  `unload_old_room()`).
- On match, calls `wfreeblock`/`destroy_bitmap` on `screenover[i].pic` (only
  if not `is_text_overlay`/`is_complete_overlay`, matching source's ownership
  check), then shifts every later entry down by one slot via a `rep movsd`
  with `ecx=5` -- 5 dwords = 20 bytes, which is exactly `ScreenOverlay`'s own
  independently-confirmed total struct size (`pic`/`type`/`x`/`y`/`timeout`,
  4 bytes each) -- and decrements `numscreenover`/loops back to re-check the
  same slot index (source's `bb--;` after the `memmove`).
- Matches `AC.CPP:3404-3441` line for line, including the loop-restart-on-
  removal behavior.

This means every earlier reference in this file to `EndSkippingUntilCharStops`
calling `stop_fast_forwarding()` at the end is **wrong** -- the actual final
call is `remove_screen_overlay(-1)`, matching 2011's `unload_old_room()`
calling `remove_screen_overlay(-1)` at `AC.CPP:3627` exactly (one more piece
of confirmation that `EndSkippingUntilCharStops`/`sub_40AAE3` really is doing
`unload_old_room()`'s job inline, just with the wrong tail-call identity
attached until now).

The REAL `stop_fast_forwarding()` (`AC.CPP:24132`, a tiny function that just
clears the fast-forward flag and stops the fast-forward music-skip state) has
**not** been located in this binary and remains a genuinely open lead --
worth a dedicated search in a future round, most likely as a short, simple,
still-unmatched `sub_*` called from somewhere in the skip-cutscene code path
rather than from `EndSkippingUntilCharStops` itself.

`matches.json` corrected accordingly: `sub_409FD4`'s entry now records
`remove_screen_overlay` (source_line 3432) with the full evidence above
(old evidence preserved in the entry for the historical record), and
`sub_40AAE3`'s (`EndSkippingUntilCharStops`) entry has a "MAJOR CORRECTION"
paragraph appended flagging every internal `stop_fast_forwarding` mention as
superseded. `apply_structs.py` required no change (checked: zero mentions of
`stop_fast_forwarding` anywhere in that file).

## The stop_fast_forwarding lead, resolved: it doesn't exist as a function here

Picked the open lead back up by reading `StartCutscene`/`EndCutscene`
(`AC.CPP:24187-24215`) in full -- both already matched via exact
linker-symbol evidence long ago, but neither had actually been read body-
first before now, the same gap that caused the `sub_409FD4` mismatch in
the first place.

**`EndCutscene`'s disassembly is the decisive piece**:

```
mov     eax, dword_4EEB54      ; retval = play.fast_forward
mov     [ebp+var_4], eax
mov     dword_4EEB50, 0        ; play.in_cutscene = 0
mov     dword_4EEB54, 0        ; play.fast_forward = 0
call    UpdatePalette          ; already matched, exact linker-symbol match
mov     eax, [ebp+var_4]
...
retn
```

2011's `EndCutscene()` is `int retval = play.fast_forward; play.in_cutscene
= 0; stop_fast_forwarding(); return retval;` (`AC.CPP:24205-24214`), and
`stop_fast_forwarding()` itself opens with `play.fast_forward = 0;
setpal();` (`AC.CPP:24134-24135`) before going on to a conditional
`newmusic(play.end_cutscene_music)` and a `MAX_SOUND_CHANNELS`-bounded
per-channel `volAsPercentage`/`originalVolAsPercentage` restore loop and a
final `update_music_volume()` call (`AC.CPP:24136-24151`).

This build's version inlines `play.fast_forward = 0` directly (matching
`stop_fast_forwarding`'s own first statement) and then calls
`UpdatePalette` instead of a separate `stop_fast_forwarding()` function.
`UpdatePalette`'s own 2011 body -- `if (game.color_depth>1)
invalidate_screen(); if (!play.fast_forward) setpal();`
(`AC.CPP:24118-24124`) -- already covers both the `setpal()` from
`stop_fast_forwarding()` AND the `invalidate_screen()` that 2011's
`EndCutscene` calls as its own separate next step (`AC.CPP:24211`) -- so
one `UpdatePalette` call here does the job both of them do separately in
2011.

**`StartCutscene` corroborates from the other direction**: 2011's version
calls `EndSkippingUntilCharStops()` then `initialize_skippable_cutscene()`
between the argument-range check and `play.in_cutscene = skipwith;`
(`AC.CPP:24194-24198`). This build's disassembly has nothing there at all
-- straight from the range check to `dword_4EEB50 = skipwith; return;`.

**Conclusion: `stop_fast_forwarding()` is CONFIRMED ABSENT as a discrete
function in this binary** -- not merely unfound. Its short setpal()-related
job is covered inline plus a call to the already-matched `UpdatePalette`,
and the rest of its 2011 body (the music-fade-back-in and per-channel
volume-restore logic) doesn't exist here at all. This is the same "later
AGS feature, genuinely absent, not just unlocated" pattern found
repeatedly elsewhere in this project (`ScreenOverlay.bmp`,
`CharacterInfo.actx`/`.acty`, `RoomObject`'s tint/zoom fields, etc.) --
here at whole-subsystem scope: `SkipUntilCharacterStops`/
`EndSkippingUntilCharStops`/`stop_fast_forwarding`/
`initialize_skippable_cutscene` are ALL absent together, consistent with
the earlier finding that no `SkipUntilCharacterStops`-related strings
exist anywhere in the extracted string dataset.

Bonus: this closes the loop on `dword_4EEB50`/`dword_4EEB54` with a third
and fourth independent confirmation each (`in_cutscene`/`fast_forward`,
both already high confidence) via two more behaviorally-distinct call
sites. `matches.json` updated: new evidence appended to `StartCutscene`'s
and `EndCutscene`'s own entries, and `sub_40AAE3`'s entry's "worth a
dedicated search in a future round" note updated to point at this
resolution instead of leaving it dangling.

## CharacterExtras revisited: xwas/ywas/invorder confirmed absent, two drift findings as a bonus

Went back to `CharacterExtras`'s remaining open fields (`xwas`/`ywas`/
`tint_r`/`tint_g`/`tint_b`/`tint_level`/`tint_light`/
`process_idle_this_time`/`slow_move_counter`/`animwait`,
`invorder[MAX_INVORDER]`/`invorder_count`), reading `sub_40AAE3`/
`EndSkippingUntilCharStops` (already matched, the same function at the
center of the last two rounds) in full for the first time end to end,
since only fragments of it had been read at any one time before.

**No `charcache`/`xwas` wipe loop exists in this function at all.** 2011's
`unload_old_room()` ends its room-object-moving-reset loop with a second
per-character loop: `for (ff=0;ff<game.numcharacters;ff++) { if
(charcache[ff].inUse) { destroy_bitmap(charcache[ff].image); ...} ...
charextra[ff].xwas = INVALID_X; }` (`AC.CPP:3637-3646`). This build's
version has exactly ONE `destroy_bitmap` call total, at the very end,
gated on `dword_523204` (already confirmed `raw_saved_screen`) -- no loop,
no `charcache` array touched anywhere in the function. This function is
NOT where this build resets per-character move state on room change, if
it does so at all.

**Chased `xwas`/`ywas` a different way: via `char_zoom`'s xref list
directly.** 2011's `xwas`/`ywas` "half a movement step" smoothing pair is
read/written by exactly one function, `wantMoveNow(int chnum,
CharacterInfo *chi)` (`AC.CPP:6349-6399`) -- a zoom-percentage-gated
movement-speed throttle (170/140/115/80/60/30% zoom bands, each mapping to
a different move-every-Nth-frame pattern via `walkwaitcounter % 2` or `%
4`) that falls through to the `xwas`/`ywas` smoothing logic only in the
30-60% zoom band. It necessarily reads `charextra[chnum].zoom` to pick a
band, i.e. it must reference the already-confirmed `char_zoom` global
(`word_4EF1BC`). Grepping `word_4EF1BC`'s COMPLETE xref list across the
entire disassembly turns up exactly two hits, both inside
`prepare_characters_for_drawing` (one read, one write, both already
accounted for by that function's own confirmed zoom-scaling logic) -- zero
xrefs anywhere else. No `wantMoveNow`-equivalent function exists in this
binary; nothing else ever reads this field.

Combined with an earlier round's dedicated search for the `xwas`/`ywas`
sentinel constant itself (`INVALID_X`=30000=`0x7530`), which turned up
only one coincidental unrelated hit (`add_screen_overlay`'s own
overlay-position tracking, reusing the same generic sentinel value in a
completely different subsystem) and zero genuine hits, this is now solid
converging negative evidence from two independent directions --
**`xwas`/`ywas` and the whole `wantMoveNow` scaled-movement-smoothing
mechanism are CONFIRMED ABSENT**, not merely unfound. This build's
movement code most likely doesn't compensate zoomed characters' walk speed
via sub-pixel smoothing at all yet.

**`invorder[MAX_INVORDER]`/`invorder_count` (2011's PER-CHARACTER
inventory-order fields, also part of `CharacterExtras`) are likewise
CONFIRMED ABSENT**, for a reason already on record from several rounds
back but not yet connected to `CharacterExtras` explicitly: this build's
own inventory-order tracking is `play_invorder`, a single GAME-WIDE array
(not per-character), and `update_invorder` (already matched) is a
genuinely simpler predecessor with no per-character loop and no
`MAX_INVORDER` bounds check at all -- the whole per-character-invorder
feature these two fields belong to simply doesn't exist in this build yet.
This also incidentally reinforces (without fully resolving) the
long-standing `play_invorder` GameState-membership question: whatever it
is, it is NOT a `CharacterExtras` member masquerading as something else,
since the per-character feature it would belong to isn't present at all.

**Two bonus findings fell out of reading the whole function**, unrelated
to `CharacterExtras` but worth recording: (1) the ambient-sound-stop step
is a single hardcoded `StopAmbientSound(1)` call, not a loop over channels
`1..MAX_SOUND_CHANNELS-1` like source's `for (ff=1;ff<MAX_SOUND_CHANNELS;
ff++) StopAmbientSound(ff);` (`AC.CPP:3600-3603`, `MAX_SOUND_CHANNELS=8`)
-- DRIFT, this build only ever stops ambient sound on channel 1; (2) the
room-script cleanup only calls `ccFreeInstance` on ONE instance
(`dword_523138`, i.e. `roominst`) where source calls it on BOTH
`roominstFork` and `roominst` (`AC.CPP:3617-3620`) -- no
`roominstFork`-equivalent global is touched anywhere in this function,
consistent with this build's already-established simpler,
non-forking room-script execution model. Also a genuine bonus: the
room-object-moving-reset loop right before all this --
`for(ff=0;ff<croom->numobj;ff++) [dword_4E45C8+ff*20h+18h]=0` -- is a
SECOND independent confirmation of `RoomObject.moving`@`+0x18` (previously
confirmed only via `do_movelist_move`'s call shape in `update_stuff`),
matching source's own `for (ff=0;ff<croom->numobj;ff++)
objs[ff].moving=0;` (`AC.CPP:3597-3598`) exactly.

**Still unresolved (at the time of the round above)**: `tint_r`/`tint_g`/
`tint_b`/`tint_level`/`tint_light`/`process_idle_this_time`/
`slow_move_counter`/`animwait`. A follow-up round closed most of these.

## animwait/walkwait fold into one field, process_idle_this_time becomes a single flag

Went back into `update_stuff` (already matched, `sub_40EF0D`) and read its
per-character walking/animation section start to finish -- the block
indexing `game_chars` via `imul reg,140h`, roughly
`loc_40F43D`..`loc_40FA7B` in the disassembly -- something no earlier
round had done in one continuous pass.

**The TURNING_AROUND branch is the key**: 2011's `if (chi->walking >=
TURNING_AROUND) { if (chi->walkwait > 0) chi->walkwait--; else {...} }`
(`AC.CPP:6526-6528`) matches disasm's `cmp dword[chi+1Ch],0; jle ...;
[chi+1Ch]--;` -- operating on `CharacterInfo.wait`@`+0x1C`, the SAME field
already confirmed (several rounds ago) via `Character_LockView`'s
`chap->wait=0;` and captioned at the time as playing a "lip-sync
decrement" role. It isn't only that -- it's `walkwait` too.

Reading further into the walking-processing block confirms the THIRD
role: 2011's `if (chi->walking<1) { charextra[aa].process_idle_this_time
= 1; ... chi->walkwait=0; charextra[aa].animwait = 0; ...} else if
(charextra[aa].animwait > 0) charextra[aa].animwait--; else { ...
charextra[aa].animwait = views[chi->view].loops[chi->loop].frames[chi->
frame].speed + chi->animspeed; ...}` (`AC.CPP:6626-6653`) matches, field
for field, THREE separate disassembly sites that ALL read/write the same
`[chi+0x1C]`:

1. `mov dword[chi+1Ch],0` inside the walking<1 branch (alongside `mov
   word[chi+3Ah],0` for `chi->frame=0` -- both confirmed fields, matching
   the source statement pair exactly).
2. `cmp dword[chi+1Ch],0; jle ...; [chi+1Ch]--;` in the `else if` branch --
   an exact structural twin of the walkwait decrement found above, just
   reached from a different branch.
3. The clincher: `movsx eax,word[chi+3Ah]; imul eax,1Ch; movsx ecx,
   word[views_frame_base+eax+8]; movsx eax,word[chi+42h]; add ecx,eax; mov
   [chi+1Ch],ecx` -- reading `chi->frame` (`+0x3A`), scaling by
   `ViewFrame272`'s confirmed `0x1C` stride, reading `.speed`@`+8` within
   the frame, adding `chi->animspeed`@`+0x42`, and storing the result into
   `[chi+0x1C]` -- an EXACT match to `animwait = frames[frame].speed +
   chi->animspeed;`.

**Conclusion: this build has ONE consolidated `CharacterInfo.wait` field
doing the job of THREE separate 2011 fields** (lip-sync wait, `walkwait`,
`charextra[].animwait`) -- not a gap in the evidence, a genuine structural
fact matching `OldCharacterInfo`'s own declaration (`acroom.h:2599-2621`),
which has exactly one `wait` field and no separate `walkwait` at all.
`animwait` is CONFIRMED ABSENT as its own field. Bonus:
`CharacterInfo.animspeed`@`+0x42` (previously tentative, inferred only
from positional adjacency to `walkspeed`) is UPGRADED to high confidence
via its direct read in step 3 above.

**`process_idle_this_time` resolved as a genuine identity, but not a
per-character array**: 2011's own gate for the section right after this
one, `else if ((loopcounter%40==0) || (charextra[aa].
process_idle_this_time == 1))` (`AC.CPP:6867`), matches disasm's `cmp
edx,0x28(40); ...; cmp dword_52320C,1; ...; or` combination exactly --
`dword_523120 % 40 == 0` identifies a new global, `loopcounter` (a
single-round modulo-40 match, not independently cross-checked further, so
treat as a reasonable but not exhaustively-verified identification), and
`dword_52320C` is this build's `process_idle_this_time` equivalent.
Grepping ALL of `dword_52320C`'s xrefs (only 3, all inside `update_stuff`)
shows it's set to 1 inside the walking<1 branch of the FIRST per-character
loop and reset to 0 exactly once, right before a SECOND per-character loop
begins -- a single shared flag, not a 50-entry array. This works because
the flag is set and consumed within the same character's own iteration of
the first loop before that loop moves to the next character -- this
build's flatter, single-pass-per-character loop shape makes a shared
scratch flag sufficient where 2011's (evidently more decoupled) structure
needs a genuine per-character array.

**Two fields shelved rather than forced**:

- `slow_move_counter` -- checked 2011's OWN source usage first, and found
  it's written exactly ONCE in the entire file (zeroed at startup,
  `AC.CPP:26259`) and never read or written anywhere else -- dead weight
  even in the reference build itself. No behavioral test could ever
  distinguish "this build has an unused field too" from "this build never
  had it," so this is left genuinely open rather than guessed either way.
- `tint_r`/`tint_g`/`tint_b`/`tint_level`/`tint_light` -- LIKELY ABSENT
  (medium confidence): the `CHF_HASTINT`(`0x2000`) flag test that would
  gate reading them (`AC.CPP:8319-8327`) has zero occurrences of the
  `0x2000` literal anywhere in `prepare_characters_for_drawing`'s ~1000-
  line body -- real negative evidence, in the same style as the
  `xwas`/`ywas` char_zoom-xref proof -- but not fully conclusive, since
  the fallback branch's own callee (2011's `get_local_tint`) hasn't been
  individually identified among this function's remaining ~11 unmatched
  callees (`sub_410D04`, `sub_4106E0`, `sub_410AFA`, `sub_410C6A`,
  `sub_410631`, `sub_4106EF`, `sub_425650`, `sub_40347F`, `sub_4492D0`,
  `sub_425200`, `sub_410937`) to positively confirm none of them touch
  tint fields either. A concrete lead for a future round.

`apply_structs.py`'s `CharacterInfo.wait`/`.animspeed` comments and the
`CharacterExtras` documentation block updated with the full writeup
above; `matches.json`'s `update_stuff` entry extended with the same
evidence.

## tint_r/tint_g/tint_b/tint_level/tint_light upgraded to confirmed absent

Picked up the concrete lead flagged at the end of the previous round: read
`prepare_characters_for_drawing`'s remaining unmatched callees looking for
a `get_local_tint`-shaped function (`AC.CPP:7661-7737`, an 8-argument
call: `get_local_tint(int xpp,int ypp,int nolight,int*amnt,int*r,int*g,
int*b,int*lit,int*lev)`).

The initial attempt at reading those specific callees (`sub_410D04`,
`sub_4106E0`, `sub_410AFA`, `sub_410C6A`, `sub_410631`, `sub_4106EF`)
turned out to be a wrong turn -- their call sites (disassembly lines
~29857-30014) are all inside a COMPLETELY DIFFERENT section of this same
function: the room-OBJECT drawing loop (iterating `dword_4E45C8`=
`croom->obj` via the confirmed `RoomObject` `0x20` stride), not the
character-drawing section at all. `prepare_characters_for_drawing`
apparently draws both room objects and characters in one pass, and these
particular unmatched callees belong to the object half.

Pivoted to tracing the ACTUAL character-drawing control flow directly
instead of guessing which callee might be `get_local_tint`: starting from
the already-confirmed zoom-scaling code (the `word_4EF0F4`/`word_4EF158`/
`word_4EF1BC` writes, `AC.CPP:8307-8317`) and reading forward
instruction-by-instruction through bitmap creation (`create_bitmap_ex`),
`clear_to_color`, the `ViewFrame272.flags&1` mirroring-check branch,
`SpriteCache::operator[]`'s sprite fetch, and the final `render_to_screen`
call -- there is NO tint-related step anywhere in that sequence. No
8-argument call matching `get_local_tint`'s shape, and no call matching
`apply_tint_or_light(...)` (`AC.CPP:7741+`, the function that would
actually apply whatever `get_local_tint` computed) appears between the
scaling code and the final blit. The code goes directly from "compute
scaled width/height" to "allocate scaled bitmap and blit the sprite" with
nothing in between.

This is meaningfully stronger evidence than the earlier round's "zero
`0x2000` literal" finding alone: it's not just the `CHF_HASTINT`-gated
per-character-override branch that's missing, the ENTIRE tint
computation-and-application subsystem is absent from this code path --
both branches of source's `if (chi->flags & CHF_HASTINT) {...} else {
get_local_tint(...); }`, and the `apply_tint_or_light` call that would
consume either branch's result, have no counterpart here at all. Neither
`get_local_tint` nor `apply_tint_or_light` is matched, or even flagged as
an unmatched lead, anywhere else in the whole binary either.
**`tint_r`/`tint_g`/`tint_b`/`tint_level`/`tint_light` are now CONFIRMED
ABSENT**, upgraded from the previous round's "likely absent, medium
confidence." As a side note, this also explains why `GameState`'s own
`rtint_red`/`rtint_green`/`rtint_blue`/`rtint_level`/`rtint_light` fields
(`get_local_tint`'s room-tint-override source, `acruntim.h:583`) have
never turned up in any GameState round -- consistent absence, not a
separate unexplained gap.

With this, every field originally flagged as open at the start of the
`CharacterExtras` investigation has now been resolved one way or another:
`width`/`height`/`zoom` (confirmed, high confidence, from early rounds),
`xwas`/`ywas`/`invorder[]`/`invorder_count` (confirmed absent),
`animwait` (confirmed absent, folded into `CharacterInfo.wait`),
`process_idle_this_time` (confirmed, but as a single global not a
per-character array), `tint_r`/`tint_g`/`tint_b`/`tint_level`/
`tint_light` (confirmed absent, this round), and `slow_move_counter`
(left genuinely open -- unfalsifiable either way, since even 2011's own
source never reads it). `apply_structs.py`'s `CharacterExtras`
documentation block and `matches.json`'s `prepare_characters_for_drawing`
entry updated with the full writeup above.

## music_master_volume closes the dword_4EF220 lead, play_speech gets named

Went back to a cluster flagged two rounds ago and left dangling: the
`dword_4EF220` pad, XREF'd from an unmatched speech-loading helper
(`sub_4141B8`, already the confirmed source of `GameState.speech_volume`)
that decrements it by a literal 60 and then calls another unmatched
helper (`sub_418E82`), with a follow-on check against the already-
confirmed `no_textbg_when_voice`@`+0x12C` gated on a mystery byte
(`byte_513340`) -- at the time captioned "plausibly lipsync/close-mouth-
timing related given the caller, but not confirmed."

**`sub_418E82` turns out to be `update_music_volume`, fused with 2011's
separate `calculate_max_volume()`** (`AC.CPP:12316-12326`/`17632-17674`):
`movsx eax,byte_51FFCD; imul eax,1Eh; mov ecx,dword_4EF220; add ecx,eax`
matches `int newvol=play.music_master_volume +
((int)thisroom.options[ST_VOLUME])*30;` exactly, and the clamp right
after (`cmp var_4,0FFh; ...; cmp var_4,0; ...`) matches `if (newvol>255)
newvol=255; if (newvol<0) newvol=0;` exactly. This build's version does
NOT have 2011's later `if (play.fast_forward) newvol=0;` line -- CONFIRMED
ABSENT, consistent with every other fast_forward-related refinement
already shown to postdate this build. After computing the volume, the
function dispatches it directly to whichever music backend is active
(MOD player / digital driver / a default Allegro `set_volume` call)
rather than 2011's later crossfade-aware channel lookup -- an
architecturally older, simpler design, predating the crossfade rewrite
entirely, with `calculate_max_volume`'s computation fused directly into
the volume-application step instead of kept as two separate functions.
Its callers (`load_new_room`, `update_polled_stuff`, both already
matched) are exactly `update_music_volume`'s real 2011 callers too.

**This identifies `dword_4EF220` as `GameState.music_master_volume`** --
confirmed twice over: via `sub_418E82`'s formula above, AND via
`sub_4141B8`'s own "`dword_4EF220 -= 60`" right before calling it,
matching 2011's "`play.music_master_volume -= play.speech_music_drop; ...;
update_music_volume();`" (`AC.CPP:13417-13420`) -- this build hardcodes
the speech-ducking amount as a literal instead of reading a configurable
`speech_music_drop` field, predating that feature. A clean ZERO-SLACK
positional bonus: the field ends exactly 4 bytes before the already-
confirmed `walkable_areas_on`@`+0x80C`, with no gap at all -- proving
2011's declared `digital_master_volume` (sitting directly between
`music_master_volume` and `walkable_areas_on`, `acruntim.h:554-556`) and
the preceding `cur_music_number`/`music_repeat` pair are CONFIRMED ABSENT
here too, not merely unfound.

**`sub_4141B8` itself is upgraded from an unnamed medium-confidence field
lead to a fully named, high-confidence match: `play_speech`**
(`AC.CPP:13337-13434`). The tail right after the ducking call closes the
loop on the whole original cluster in one motion: "`movsx ecx,byte_513340;
cmp ecx,2; jnz end; cmp dword_4EEB44,0; jle end; mov byte_513340,1; mov
dword_4EEB44,2;`" matches 2011's "`if ((game.options[OPT_SPEECHTYPE]==2)
&& (play.no_textbg_when_voice>0)) { game.options[OPT_SPEECHTYPE]=1;
play.no_textbg_when_voice=2; }`" (`AC.CPP:13428-13431`) exactly --
identifying `byte_513340` as `game.options[OPT_SPEECHTYPE]` and giving
`no_textbg_when_voice` a SECOND, independent, high-confidence confirmation
from a completely different call site than the one that originally found
it. The earlier "plausibly lipsync-related" guess for this whole cluster
was simply WRONG -- the real answer is music-ducking-during-speech and
Sierra-style speech-window mode switching, nothing to do with lipsync at
all. Two genuine DRIFT points recorded alongside the match: this build's
total-load-failure path calls `quit()` (FATAL) where 2011 fails
gracefully (`debug_log(...); return 0;`, `AC.CPP:13404-13408`), and there
is no OGG attempt between the WAV and MP3 attempts (this build predates
OGG speech support). Named despite these differences, consistent with
this project's convention of naming a function when its overall algorithm
shape matches even where individual steps are simplified or hardcoded --
contrast with `sub_42B394`/`cc_run_code` and `sub_40A6D8`, left unnamed
because their core ALGORITHM, not just a few constants, genuinely
diverges.

A loose end noted but not chased further: `sub_408556`/`sub_408623` (the
WAV/MP3 loader helpers `play_speech` calls) have a call shape matching
`my_load_wave`/`my_load_mp3` respectively (3 args with a trailing 0 vs. 2
args), but `sub_408556` is ALSO called from `PlayAmbientSound` -- meaning
it's more likely a shared generic sound-loading helper than literally
`my_load_wave` itself. Left unmatched rather than forced; a candidate for
a future round with its own dedicated read.

`apply_structs.py`'s `music_master_volume` field (replacing the old
`_pad_unknown9`) and `no_textbg_when_voice`'s comment updated;
`matches.json`'s `sub_4141B8` entry renamed to `play_speech` with the
full evidence above, and a new entry added for `sub_418E82` =
`update_music_volume`.

## Closing the loose end: my_load_wave, my_load_mp3, load_sample, play_sample

Picked up last round's explicitly-flagged loose end -- identify
`sub_408556`/`sub_408623`, the WAV/MP3 loader helpers `play_speech`
calls -- by reading both bodies in full instead of guessing from call
shape alone.

**`sub_408623` = `my_load_mp3`** (`acsound.cpp:296-330`) turned out to be
an overwhelming, complete, instruction-for-instruction match: `pack_fopen`
(already matched) into a GLOBAL matching 2011's own `mp3in` being a
file-scope global; a `malloc(0x186A0)` that matches `MP3CHUNKSIZE`'s OLD,
COMMENTED-OUT value of 100000 in source (`//#define MP3CHUNKSIZE 100000` /
the active `#define MP3CHUNKSIZE 32768` right below it) -- a genuine drift
point that happens to match an explicit version-history artifact left
sitting in the source's own comments, not just an inferred fact; `pack_fclose`
on the malloc-failure path; `new MYMP3()` into another global matching
2011's own `thistune`; a constructor call; field sets matching
`thistune->in`/`chunksize`/`done` exactly; the exact same `chunksize`-vs-
`mp3in->todo` clamp as source; `pack_fread`; and a final boolean
(`mp3in->todo<1`) computed exactly matching `almp3_create_mp3stream`'s
third argument shape. **This independent full-body match upgrades
`pack_fopen`/`pack_fread`/`pack_fclose` from medium to high confidence**
-- they were previously found only via `my_load_static_mp3`'s call shape,
with a lingering "the PACKFILE.todo offset doesn't match 4.2.2's
declaration" caveat; a second, unrelated caller confirming the exact same
shape settles that as ordinary struct-layout drift, not an identity
doubt.

**`sub_408556` = `my_load_wave`** (`acsound.cpp:152-170`) matches
similarly cleanly: calls `sub_4444C0` (new match, see below) on the
filename, returns NULL on failure, otherwise allocates a small wrapper
object (`operator new(0x10)`) into a global matching 2011's own
`thiswave`, calls a constructor, stores the loaded sample pointer, then
calls `sub_444AF0` (new match, see below) with `(sample, voll, 128,
1000, loop)` -- an exact match to Allegro's `play_sample(spl,vol,pan,
freq,loop)` signature (`pan=128`=center, `freq=1000`=Allegro's "normal
speed" sentinel). Both of `my_load_wave`'s two real callers make semantic
sense of its `loop` argument in a way that independently corroborates the
match: `play_speech` passes `0` (play once) and `PlayAmbientSound` passes
`-1` (loop forever, matching ambient sound's actual looping behavior).

**Following those two calls down turned up two more clean matches, both
well-known Allegro third-party APIs**: `sub_4444C0` = `load_sample` --
its body is Allegro's real extension-dispatch implementation almost
verbatim, using the already-matched `uconvert` to build `"wav"`/`"voc"`
comparison strings and, on a match, calling the already-matched
`load_wav`/`load_voc` directly (its own only two real loaders, with NULL
as the fallback for anything else). `sub_444AF0` = `play_sample` -- opens
with the already-matched `allocate_voice`, returns early if negative
(matching `if (v<0) return v;`), then a `freq==1000` special case
matching source exactly, with the `else` branch computing the scaled
frequency via a textbook MSVC divide-by-1000 reciprocal-multiplication
magic constant (`0x10624DD3`) -- an unmistakable compiler fingerprint,
not something that could plausibly be a coincidence. Bonus: this also
surfaces 5 candidate Allegro voice-control functions for a future round
(`sub_445260`/`sub_445560`/`sub_445400`/`sub_4451A0`/`sub_445070`+
`sub_445050`, plausibly `voice_set_volume`/`_pan`/`_frequency`/
`_playmode`/`voice_start`) -- not independently confirmed this round,
left unmatched rather than forced.

Three new globals identified along the way, all as a side effect of the
two main matches: `dword_4CCC80`=`thiswave`, `dword_4EF384`=`mp3in`,
`dword_4CD17C`=`thistune` -- all file-scope globals in this build too,
matching 2011's own declarations exactly.

**Bonus, closing a small side-thread from two rounds ago**: `rtint_red`/
`rtint_green`/`rtint_blue`/`rtint_level`/`rtint_light` (`get_local_tint`'s
room-tint-override source) are now CONFIRMED ABSENT outright, not just
"consistent with absence" -- checked their only writer, `SetAmbientTint`
(`AC.CPP:2615-2629`, a script API function distinct from the already-
matched `TintScreen`), and its distinctive error string
(`"!SetTint: invalid parameter..."`) has zero occurrences anywhere in the
extracted string dataset. Neither the reader (`get_local_tint`, shown
absent two rounds ago) nor the writer exists in this build -- `TintScreen`
(writing the separate, already-confirmed `screen_tint` field via direct
palette manipulation) is this build's only tint mechanism, predating the
whole RGB/opacity/luminance-based tint subsystem entirely.

`apply_structs.py`'s `rtint_*` note upgraded to confirmed-absent;
`matches.json` gets 4 new entries (`my_load_wave`, `my_load_mp3`,
`load_sample`, `play_sample`) plus confidence upgrades for `pack_fopen`/
`pack_fread`/`pack_fclose` (565 entries total).

## The full play_sample call chain closes: all 5 remaining voice-control leads confirmed

Picked up the round's other explicitly-flagged lead immediately: the 5
candidate Allegro voice-control functions surfaced while confirming
`play_sample` (`sub_444AF0`) -- `sub_445260`/`sub_445560`/`sub_445400`/
`sub_4451A0`/`sub_445070`+`sub_445050`. Checking Allegro 4.2.2's real
`play_sample()` source first (`sound.c:1190-1209`) showed its full body:
`allocate_voice -> voice_set_volume -> voice_set_pan ->
voice_set_frequency(absolute_freq(freq,spl)) -> voice_set_playmode ->
voice_start -> release_voice` -- a complete 7-call chain, with
`absolute_freq` itself marked `static INLINE` in source (explaining why
its `(spl->freq*freq)/1000` computation appears inlined directly into
`play_sample`'s own disassembly rather than as a separate call, already
noted last round). All 5 remaining candidates map onto this chain one
for one, and every single one checked out on a full body read:

- **`sub_445260` = `voice_set_volume`** (`sound.c:1640-1656`): the
  `_digi_volume`-scaling branch uses a classic MSVC divide-by-255
  reciprocal-multiplication magic constant (`0x80808081`, `sar 7`) --
  another unmistakable compiler fingerprint, matching `voice_set_pan`'s
  own `0x10624DD3` divide-by-1000 constant from last round as the same
  kind of decisive, coincidence-proof evidence.
- **`sub_445560` = `voice_set_pan`** (`sound.c:1817-1830`): identifies
  `dword_537BAC` as Allegro's `_sound_flip_pan` global via the
  `pan=255-pan` conditional swap, matching source exactly.
- **`sub_4451A0` = `voice_set_playmode`** (`sound.c:1592-1604`): the
  `PLAYMODE_BACKWARD` bit test (`test bl,2`, matching `digi.h:167`'s
  `#define PLAYMODE_BACKWARD 2` exactly) gates a second vtable dispatch
  reading `virt_voice[voice].sample->len-1` -- identifying `dword_550040`
  as `virt_voice[].sample`, sitting exactly one dword before the
  already-established `.num` field, matching the `VOICE` struct's
  declared order (`sample; num; autokill; time; priority;`,
  `aintern.h:1096-1103`) exactly.
- **`sub_445070` = `voice_start`** (`sound.c:1491-1500`): an
  unconditional write of a global into `virt_voice[voice].time`
  (`dword_537E8C`) identifies that global as Allegro's well-known
  `retrace_count`.
- **`sub_445050` = `release_voice`** (`sound.c:1478-1482`): a
  one-line body, but the literal value written (`0xFFFFFFFF`) is the
  clincher -- Allegro's own `TRUE` is `#define`d as `-1`
  (`base.h:59`), not `1`, so `virt_voice[voice].autokill = TRUE;`
  really does compile to writing `0xFFFFFFFF`. `dword_550048` (the field
  written) sits exactly one dword after `.num`, matching `VOICE`'s
  declared order for `autokill` too.

Every field in the `VOICE` struct's first 3 members (`sample`@0, `num`@4,
`autokill`@8) is now independently cross-confirmed across three different
functions this round, and the entire `play_sample` call chain -- all 7
calls -- is fully identified with zero remaining unmatched links.
`matches.json` gets 5 new high-confidence entries, plus a follow-up note
on `play_sample`'s own entry recording that the chain is complete (570
entries total). No `apply_structs.py` changes needed -- these are Allegro
internals, not AGS-specific struct fields.

## cur_music_number/music_repeat close the +0x60C pad -- and a self-caught correction

Picked up the last remaining `GameState` pad thread: `dword_4EF024`/
`dword_4EF028` at `+0x60C..+0x614`, right after `globalscriptvars[300]`.
The old lead note pointed at `sub_408356` (an unmatched helper called
from `sub_418E82`) touching `dword_4EF028` in what looked like a
music-volume-adjustment context -- worth chasing directly since
`sub_418E82` is now known to be `update_music_volume` (confirmed two
rounds ago).

**`dword_4EF024` = `GameState.cur_music_number`**, confirmed across SIX
independent functions once actually searched for -- its xref list reads
almost like a checklist of every place 2011 touches `play.
cur_music_number`: `GetCurrentMusic` (`return dword_4EF024;`, matching
`return play.cur_music_number;` at `AC.CPP:17750`), `PlayMusic` (reads it
for an "already playing this track" early-out, then writes the new
number, matching `AC.CPP:17896-17917`), `scr_StopMusic` (writes `-1`,
matching `AC.CPP:17583`), `main` (inits to `-1`, matching `AC.CPP:26327`),
and -- the single most decisive piece -- `restore_game_data` writing the
oddly-specific literal `0x7D0` (2000), matching source's own
`play.cur_music_number=2000; // make sure it gets played` comment at
`AC.CPP:23632` word for word. A literal that specific, in that exact
role, cannot plausibly be a coincidence.

**`dword_4EF028` = `GameState.music_repeat`** is even cleaner:
`SetMusicRepeat` (already matched)'s ENTIRE one-line body --
`dword_4EF028 = loopflag;` -- matches 2011's ENTIRE function body
verbatim, `void SetMusicRepeat(int loopflag) { play.music_repeat=
loopflag; }` (`AC.CPP:17753-17755`). It sits with ZERO gap immediately
after `cur_music_number`, matching 2011's own declared adjacency `int
cur_music_number,music_repeat;` (`acruntim.h:553`) exactly -- one of the
very few pairs in this whole struct that shows NO positional drift
relative to source at all.

**A genuine self-caught correction, surfaced by this find**: two rounds
ago, `music_master_volume`'s own field entry claimed that the SAME
zero-gap argument proving `digital_master_volume` absent (it sits right
after `music_master_volume`, which itself sits with zero gap before
`walkable_areas_on`) ALSO proved `cur_music_number`/`music_repeat`
absent, since 2011 declares them immediately BEFORE `music_master_volume`
in the same struct. That reasoning was WRONG -- a zero-gap argument about
what comes AFTER a field says nothing about what would come BEFORE it;
those are two separate claims that got conflated in the excitement of a
clean positional proof. The correct picture, now that both fields are
independently confirmed: `cur_music_number`/`music_repeat` DO exist in
this build, just as a genuinely standalone pair at `+0x60C` -- nowhere
near `music_master_volume`'s own `+0x808` position. 2011 consolidated
what this build keeps as two separate, non-adjacent globals (this pair,
and `music_master_volume` off on its own) into one contiguous run inside
`GameState` -- the SAME "later consolidation of previously-independent
globals" pattern this project has now seen repeatedly (`in_cutscene`/
`wait_counter`/`globalscriptvars`, `CharacterExtras`, etc.), just not
recognized here until this second look. `digital_master_volume`
remaining confirmed absent is UNAFFECTED by this correction -- that part
of the original argument was sound.

`apply_structs.py`'s `_pad_unknown8` replaced with real
`cur_music_number`/`music_repeat` field declarations, its surrounding
`+0x60C..+0x80C` span comment updated, and `music_master_volume`'s own
comment corrected in place (old wrong claim struck through with a visible
CORRECTION note, not silently removed). `matches.json`'s `GetCurrentMusic`
and `SetMusicRepeat` entries extended with the full evidence above.

## GameState's very last pad turns out to already be solved

One thread remained after the `cur_music_number`/`music_repeat` round:
the `+0x820..+0x828` pad (8 bytes, 2 dwords), sitting between
`offsets_locked` and `entered_edge`. Computing its actual addresses
(`0x4EF238`-`0x4EF23F`, base `0x4EEA18`+`0x820`) and grepping the ENTIRE
disassembly for every one of them turned up zero `dword_4EF238`/
`dword_4EF23C`-style labels anywhere -- meaning literally no code in the
whole binary touches this memory, a stronger and more direct kind of
negative evidence than any role-based absence finding in this project so
far (most "confirmed absent" fields were established via watching a
specific reassignment happen elsewhere, not a total-silence search like
this one).

Checking what 2011 declares at this exact position explained why: `int
entered_at_x,entered_at_y,entered_edge;` (`acruntim.h:559`) -- and
`entered_at_x`/`entered_at_y` were ALREADY confirmed absent, several
rounds ago, during the very first `entered_edge` investigation
(`load_new_room`'s second edge-computation block writes the equivalent
2011 assignment into the shared `tox`/`toy` scratch globals instead of
persistent fields) -- that finding just hadn't been cross-referenced back
into this pad's own comment until now. The zero-xref search this round
doesn't overturn or add a new fact so much as it independently and much
more forcefully reconfirms a conclusion that was already correct: nothing
here, for either reason.

**With this, GameState's field-level investigation is complete in the
fullest sense** -- every byte from `+0x00` through the struct's own
proven `+0x964` total size is now either a confirmed field or an
explicitly-explained, evidenced pad, with no remaining "genuinely
unexplored" territory left anywhere in the struct. `apply_structs.py`'s
`_pad_unknown5` comment rewritten to cite the resolution;
`matches.json`'s `load_new_room` entry gets a short follow-up note
recording the zero-xref search (570 entries, unchanged -- no new function
identity this round, just a documentation close-out).

## Fresh survey: RoomStruct (`thisroom`/`rstruc`) -- round 1

With `GameState` fully closed out, picked a genuinely fresh target:
2011's `roomstruct` (`Common/acroom.h:806`), AGS's current-room data
format (walkable areas, hotspots, regions, background scenes) -- never
formalized in this project despite incidental references in already-
matched functions going back many rounds (`calculate_max_volume`'s
`thisroom.options[ST_VOLUME]`, `unload_old_room`'s
`thisroom.numLocalVars`, etc.).

**The global instance turns out to already be identified**: `load_room`
(already matched) passes `offset rstruc` as its own first argument, and
`rstruc` is a PRE-EXISTING IDA name (not from this project) with FOUR
fields already named too -- `rstruc.walls`/`.object`/`.lookat`/
`.regions` -- from work predating this project's own tracking, the same
"already recovered, just needs formalizing" situation as `SpriteCache`.
All four match 2011's own declared leading order with ZERO drift, an
unusually clean start for this project (most structs' leading fields
drift at least a little).

**A genuine caution surfaced immediately**: IDA's Local Types library
ALSO already has a type literally named `roomstruct` applied to this
global (`"rstruc roomstruct <?>"`). That type is almost certainly either
a blind import of 2011's declared layout or an old placeholder --
UNVERIFIED against this build, and this project has already independently
shown (via `RoomStatus`/`RoomObject`/`GameState` work in earlier rounds)
that this exact struct's own capacity constants drift substantially
smaller here than 2011's declarations (`MAX_INIT_SPR=10` not 2011's
larger value, `MAX_HOTSPOTS=20` not 2011's 50, `MAX_WALK_AREAS=15`). The
usual "never trust a 2011 layout without independent verification"
caution applies doubly hard here, since a pre-existing IDA type makes it
tempting to skip that step -- worth flagging loudly for whoever continues
this work.

**Evidence source**: `load_main_block` (already matched,
`acroom.h:1605`, called from `load_room`) -- AGS's own room-file-loading
code, which inits/reads `roomstruct`'s fields in a strict, traceable
sequence, the same kind of anchor `count_data_offsets.py` used for
`GameSetupStructBase`'s `.data`-section layout. Its opening init/memset
block confirmed 5 fields via exact literal-value matches (`width=320`,
`height=200`, `resolution=1`, `numwalkareas=0`, `numhotspots=0`), plus
two bigger finds:

- **`objbaseline[10]`**@`+0x3858` -- the function's ONLY `0xFF`-valued
  memset (`memset(rst+0x3858,0xFF,0x28)`) is an unambiguous fingerprint
  matching `memset(&rstruc->objbaseline[0],0xff,sizeof(int)*
  MAX_INIT_SPR);` exactly, capacity 10 matching `RoomObject`'s already-
  confirmed array size with zero further drift.
- **`hotspotnames[20][30]`**@`+0x2414`, an ARCHITECTURAL DRIFT finding:
  this build stores hotspot names as a FIXED INLINE `char[20][30]`
  array, not 2011's `char* hotspotnames[MAX_HOTSPOTS]` (individually
  `malloc`'d pointers) -- the hotspot-init loop writes directly into
  computed offsets via `sprintf("Hotspot %d")`/`strcpy("No hotspot")`,
  with a 30-byte stride matching the SAME function's own pre-v28-room-
  format fallback `fread` size later on. This build simply never moved
  past the old fixed-size-name convention for hotspots.

A clean structural bonus fell out of the last find: `hotspotnames`'s
computed end address (`0x2414+20*30=0x266C`) lands with ZERO gap exactly
where the NEXT of three consecutive, zero-gap memsets begins (`0xB90`/
`0x5C8`/`0x94` bytes), and the last of those three ends with zero gap
exactly at `objbaseline`'s own independently-confirmed start (`0x3858`)
-- the whole `+0x266C..+0x3858` span (1628 bytes) is fully byte-
accounted for even though none of its individual field contents are
identified yet, the same "fully accounted for, not every piece named"
status this project has given several other spans (`GameState`'s
`+0x60C..+0x80C` in an earlier round, for instance).

**Left deliberately open for a future round**: the huge `+0x10..+0x1570`
and `+0x1574..+0x23C0` spans before `numwalkareas`/`numhotspots`
(plausibly `pal[256]`, the "obsolete v2.00 action editor" arrays, `left/
right/top/bottom`, `sprs[]`, the `NewInteraction*` pointer arrays --
none independently confirmed), the three unidentified-but-byte-
accounted-for regions between `hotspotnames` and `objbaseline`
(plausibly `hswalkto[]`/`hotspotScriptNames[]`/`shadinginfo[]`/
`walk_area_zoom[]`/`walk_area_light[]`/`objectFlags[]`), and everything
past `resolution`@`+0x3884` -- the struct's own total size isn't
established yet either. A concrete next step: keep following
`load_main_block`'s field-by-field `fread`/`memset` sequence in source
order past where this round stopped, the exact technique that produced
every confirmed field so far.

`apply_structs.py` gets a new `RoomStruct` declaration (9 confirmed
fields/arrays, the rest precisely-sized pads); `matches.json`'s
`load_main_block` entry extended with the full evidence above.

## RoomStruct round 2: hscond/objcond/misccond are still LIVE here, not dead code

Immediate follow-up: `load_main_block` doesn't just init/memset
`roomstruct`'s fields, it also `fread`s most of them straight from the
room file in a real, traceable sequence -- reading that sequence (rather
than just the init preamble read last round) closed every pad flagged as
open.

**`numobj`@`+0x414`** and **`objyval[]`**'s start (`+0x416`, zero gap
after) close cleanly: `fread(rst+0x414,2,1)` / `fread(rst+0x416,2,
[rst+0x414])` match `fread(&rstruc->numobj,2,1,opty);` /
`fread(&rstruc->objyval[0],2,rstruc->numobj,opty);` (`acroom.h:1650`/
`1655`) exactly -- `objyval[]`'s true capacity isn't asserted (the read
count is dynamic, driven by `numobj` itself, not a compile-time
constant), so it's folded into the following pad rather than declared as
a fixed array, consistent with this project's "don't assert an
unconfirmed capacity" convention. `numwalkareas` and `numhotspots` (both
already confirmed via literal init last round) each get a clean SECOND
confirmation via their own `fread`s, and `hotspotnames` gets a second
confirmation too, via a version-gated newer-format read path using the
exact same address/stride/capacity as the init-loop path found last
round.

**The decisive find: `hscond[20]`/`objcond[10]`/`misccond` are still
genuinely live in this build's room-file format.** Three `fread` calls
share the EXACT SAME `ElementSize`, `0x94`(148) -- and that number is not
a guess, it's an EXACT match to `EventBlock`'s own independently-
confirmed total size (a struct fully closed out many rounds ago, in the
`GUIObject`-hierarchy era of this project). `hscond[20]`@`+0x266C`,
`objcond[10]`@`+0x31FC`, `misccond`@`+0x37C4` are `roomstruct`'s own
SOURCE copies of the exact same per-hotspot/per-object/room-wide
`EventBlock` command lists that `RoomStatus.hscond[20]`/`.objcond[10]`/
`.misccond` (confirmed several rounds ago) hold RUNTIME copies of --
this closes the loop architecturally: the compiled room file stores
these command lists in `roomstruct`, and at room-load time they get
copied into the per-save-slot `RoomStatus`. Capacities match `MAX_
HOTSPOTS=20`/`MAX_INIT_SPR=10` exactly, with zero further drift from
`RoomStatus`'s own already-confirmed capacities -- both structs agree
completely, from two totally independent investigation threads several
rounds apart.

This is a genuinely interesting archaeological point worth flagging
explicitly: 2011's own header keeps the `EventBlock`-based fields only as
a DEAD, commented-out declaration (`/* EventBlock hscond[MAX_HOTSPOTS];
EventBlock objcond[MAX_INIT_SPR]; EventBlock misccond; */`, found several
rounds ago while cracking `RoomStatus`'s own last gap) -- but in THIS
2002 build, the room-file-loading code still actively reads and uses
this data on every room load. Rob Blanc 1 isn't just running an old
engine build with vestigial dead fields still declared for save-
compatibility (the `OriGameSetupStruct`/`OldCharacterInfo` pattern seen
repeatedly elsewhere in this project) -- for the `EventBlock`-based
interaction system specifically, it's running the actual LIVE, load-
bearing implementation that 2011 only remembers in a comment.

`hswalkto[20]` (2-short `x,y` pairs, 80 bytes) also closes cleanly via
`fread(rst+0x23C4,4,20)` matching `fread(&rstruc->hswalkto[0],
sizeof(_Point),rstruc->numhotspots,opty);` (`acroom.h:1666`) exactly --
represented as a raw `short[20][2]` in `apply_structs.py` rather than
declaring a new `_Point` type, since nothing else in this project has
needed one yet.

**This closes ALL THREE of last round's "byte-accounted-for but not
individually identified" pad regions** with real field identities in a
single follow-up read -- a notably fast turnaround compared to most
struct-recovery threads in this project, helped enormously by
`EventBlock`'s size already being nailed down precisely from earlier
work. `apply_structs.py`'s `RoomStruct` declaration updated with 6 more
confirmed fields (`numobj`, `hswalkto`, `hscond`, `objcond`, `misccond`,
plus reconfirmations of `numwalkareas`/`numhotspots`/`hotspotnames`);
`matches.json`'s `load_main_block` entry extended with the full evidence
above.

## RoomStruct round 3: the "obsolete v2.00" arrays are genuinely live, and a real field-order surprise

Kept reading `load_main_block` past the `+0x936..+0x1570` gap flagged as
open two rounds ago, and it turned out to hold TWO distinct, fully
resolvable pieces: a version-gated backward-compatibility branch, and
(past that) the real `left`/`right`/`top`/`bottom`/`numsprs`/`sprs[]`
fields this build's own room files actually use.

**The version<9 branch confirms this build's room-file format still
actively reads 2011's "obsolete v2.00 action editor" arrays.** 2011's
header keeps `whataction`/`val1`/`val2`/`otcond`/`points` (all
`NUM_CONDIT+3`-sized) declared but already calls them obsolete in a
comment (`acroom.h:812-818`). This function has a REAL, version-gated
`fread` path for room-file versions 7-8 (`arg_C>=7 && arg_C<9`) that
reads all five directly, plus an even-older sub-v7 conversion path that
transforms them into the newer `hscond`/`objcond` format on the fly.
Their capacity closes with a genuinely satisfying chain: `whataction`'s
computed end lands exactly on `val1`'s start, which lands exactly on
`val2`'s, then `otcond`'s, then `points`'s -- FOUR consecutive zero-slack
boundaries in a row, all consistent with capacity 130 (`NUM_CONDIT+3=130`,
`NUM_CONDIT=127` -- independently matching the same function's own
`ElementCount=0x7F`(127) local used by the sub-v7 path). Whether this
specific game's OWN room files are old enough to exercise this path, or
it's dead-but-still-compiled fallback code, isn't established -- but
either way the array layout itself is now confirmed. This is the SAME
kind of finding as `hscond`/`objcond`/`misccond` two rounds ago (a "dead
in 2011, still load-bearing in 2002" subsystem), just one step further
back in AGS's own version history.

**Past that branch, the version>=9 path (this build's real, actively-
used path) confirms `left`/`right`/`top`/`bottom`/`numsprs`/`sprs[10]`**
via a satisfying double cross-check: the four `fread`s execute in
source's own READ order (top, bottom, left, right, `acroom.h:1704-1707`)
but land on addresses that only make sense in source's DECLARED order
(left, right, top, bottom, `acroom.h:819`) -- both orderings agree on the
same four addresses simultaneously, leaving zero ambiguity about which
is which. `numsprs` follows with zero gap, then a 2-byte gap
(`nummes`, filled positionally but not independently behaviorally
confirmed this round -- flagged honestly as MEDIUM rather than high),
then `sprs[10]` -- a 100-byte memset/`fread` pair matching `sprstruc`'s
own already-confirmed 10-byte size (5 packed shorts) times 10 elements,
`MAX_INIT_SPR=10` again matching this build's now-very-well-established
capacity.

**A genuine surprise fell out of a bonus cross-reference**: the same
version>=9 path also does `fread(rst+0x3858, ElementSize=4,
Count=[rst+0x8CE])` -- reading directly INTO the address already
confirmed, two rounds ago, as `objbaseline` (via the function's only
`0xFF`-fill memset) -- using `numsprs`'s own value as the read count
rather than a fixed `MAX_INIT_SPR` constant. A welcome second
confirmation of that address, but it also surfaces something more
interesting: 2011 declares `objbaseline` immediately after `sprs[]`/
`intrObject[]`/`objectScripts` -- i.e., right around where THIS round's
`sprs[]` sits, `+0x936` -- but this build's actual `objbaseline` is
nowhere near there. It sits at `+0x3858`, on the FAR side of the entire
`numwalkareas`-through-`misccond` hotspot/walkarea block. This is a
genuinely different kind of drift than everything else found in this
struct (and most others in this project) so far -- not a smaller
capacity or an absent later-added field, but the SAME set of fields
arranged in a different ORDER, with an entire unrelated subsystem's
worth of fields interposed between two fields 2011 declares as
neighbors. `intrObject[]`/`objectScripts` themselves haven't turned up
anywhere in the address ranges explored so far -- plausible
confirmed-absent-by-precedent (matching this build's well-established
"predates `NewInteraction`" finding from the `EventBlockCmd`/
`GameAnimation` rounds), but not yet independently verified for this
specific struct.

`apply_structs.py`'s `RoomStruct` declaration extended with 12 more
confirmed/positional fields across this round (`whataction`/`val1`/
`val2`/`otcond`/`points`/`left`/`right`/`top`/`bottom`/`numsprs`/
`nummes`/`sprs`) plus the field-order finding documented inline;
`matches.json`'s `load_main_block` entry extended with the full evidence
above.

## RoomStruct round 4: password through shadinginfo close, and walk_area_zoom/light get individually resolved

Continued straight on from round 3's `sprs[]` find, through to the
version-gated script-loading calls (`load_script_configuration`/
`load_graphical_scripts`, both already matched) -- a long, productive
stretch that closed the entire `+0x936..+0x1570` pad from two rounds ago
field by field, plus resolved the one loose end round 1 left dangling.

**`password[11]`/`options[10]`** close immediately and cleanly: exact-
size `fread`s (`0xB`/`0xA` bytes) with zero gap between them, matching
2011's declared pair exactly.

**`message[]`/`msgi[]`** turned out to be more interesting than a plain
`fread`: `message[]` is populated by a dedicated loop that decrypts each
message string via an as-yet-unmatched helper (`sub_403024`, plausibly
`read_string_decrypt`-adjacent -- a candidate for a future round, not
chased down this round), `malloc`s a buffer sized to fit, and stores the
pointer -- genuinely a `char*[100]` array, matching 2011's declared TYPE
exactly (unlike `hotspotnames`, which drifted to a fixed inline array,
this field did NOT drift). `msgi[]` closes right after with zero gap,
its 2-byte-per-entry stride matching `MessageInfo`'s own packed
2-byte layout precisely, and a per-message `flags|=1` write in the same
loop directly confirms `MessageInfo.flags`'s role. The version<3
fallback path's 200-byte memset (`200/2=100`) independently reconfirms
`MAXMESS=100` a second way.

**`anims[10]`/`numanims`/`shadinginfo[16]`** close via the same
version-gated `fread`-or-`memset` pattern seen throughout this struct --
`FullAnimation`'s own 244-byte internal layout isn't explored this
round, `anims[]` is represented as a raw byte blob, but its own overall
size and `MAXANIMS=10` capacity (the usual smaller-than-2011 pattern)
are solid.

**Round 1's one remaining loose end resolves cleanly**: the "two
`0x20`-byte memsets, ambiguous which is `shadinginfo`/`walk_area_zoom`/
`walk_area_light`" note from the very first round is now fully settled
-- `shadinginfo` turned out to live somewhere else entirely (`+0x154E`,
found this round), and the two memsets near `width`/`height`/
`resolution` are `walk_area_zoom[16]`@`+0x3886` and
`walk_area_light[16]`@`+0x38A6`, each individually confirmed via their
own version-gated `fread`s (driven by a shared `NUMREAD` count local,
bounds-checked against a literal `16` matching `MAX_WALK_AREAS+1`
exactly). They sit with zero gap between each other, which also
confirms 2011's intervening `walk_area_zoom2[MAX_WALK_AREAS+1]`
(`acroom.h:857`) is CONFIRMED ABSENT here -- no room left for it.

This round closes the entirety of the `+0x936..+0x1570` span from round
3 with real field identities (no pads left inside it except two tiny
1-2 byte alignment gaps and one genuinely still-unexplored 32-byte
stretch between `msgi[]` and `anims[]`). `apply_structs.py`'s
`RoomStruct` declaration extended with 9 more confirmed fields
(`password`/`options`/`message`/`msgi`/`anims`/`numanims`/`shadinginfo`/
`walk_area_zoom`/`walk_area_light`) plus `nummes` upgraded from
positional to behaviorally confirmed; `matches.json`'s `load_main_block`
entry extended with the full evidence above.

## MAJOR SELF-CAUGHT CORRECTION: the four leading fields were never actually confirmed

Continuing straight on into the background-picture-loading calls
(`sub_40365D`/`sub_403846`, right after `walk_area_light`) surfaced a
genuine mistake sitting at the very foundation of this struct -- its own
first four fields, the ones round 1 called "unusually clean, zero-drift,
`MouseCursor`/`GUIMain`-caliber."

**The problem**: round 1's evidence for `walls`@`+0x00`/`object`@`+0x04`/
`lookat`@`+0x08`/`regions`@`+0x0C` was "pre-existing IDA field names
(`rstruc.walls` etc.) appearing in `load_room`'s disassembly." That
sounded like independent confirmation at the time, but it wasn't --
`rstruc` already has IDA's own `roomstruct <?>` type applied to it (the
SAME type round 1's own caution note explicitly flagged as "UNVERIFIED
... almost certainly a blind import ... must NOT be trusted wholesale").
Once a type is applied to a global, IDA's disassembly display
automatically renders raw offset arithmetic as `GLOBAL.field` syntax
using THAT type's field names -- so "`rstruc.walls`" in the listing was
never independent evidence about this build at all, it was IDA's own
unverified guess reflected back at me, dressed up as if it were a fact about the
disassembly. Exactly the trap the caution note warned against, walked
into anyway two rounds later.

**The actual independent evidence**: `load_main_block`'s own
`loadcompressed_allegro` calls (`sub_403846`, matching 2011's real 4-arg
signature `FILE*, block* [by ADDRESS], color*, long`) reference their
destination fields via literal hex arithmetic on the `rst` FUNCTION
PARAMETER -- untyped, so it never goes through IDA's struct at all, and
is genuinely independent. Three calls fire unconditionally, one after
another, targeting `rst+0x4`, `rst+0x8`, `rst+0xC` in that exact order
-- matching source's own unconditional trio, `loadcompressed_allegro`
for `walls` then `object` then `lookat` (`acroom.h:1946-1952`), in the
same relative order. A fourth call, gated on room-file version>=8,
targets `rst+0x10` and sits immediately BEFORE that trio in the
disassembly -- matching source's own version-gated `regions` load
coming before the trio in source order too (`acroom.h:1938-1939`).

Every one of the four fields shifts 4 bytes later: `walls`@`+0x04`,
`object`@`+0x08`, `lookat`@`+0x0C`, `regions`@`+0x10`. And a fifth,
previously-unknown field turns out to occupy the REAL `+0x00`:
**`ebscene[0]`**, confirmed via `sub_40365D` (a DIFFERENT function,
matching `load_lzw`'s distinct 3-arg signature, `FILE*, block [by
VALUE], color*`) being called with the value currently AT `+0x00` as its
"block" argument, immediately followed by `[rst] = dword_4EDA3C` --
matching source's `tesl=load_lzw(opty,rstruc->ebscene[0],rstruc->pal);
rstruc->ebscene[0]=recalced;` (`acroom.h:1926-1927`) exactly.
`dword_4EDA3C` is independently confirmed as the real AGS global `BITMAP
*recalced;` (`acroom.h:1333`), used by every `load_lzw` implementation
to hand back its decompressed result.

**A satisfying bonus came along with the fix**: the standing "`pal[256]`
should be 1024 bytes but there's a 4-byte remainder before `numobj`"
puzzle from an earlier round evaporates completely. `pal[256]` is the
SAME shared palette-buffer address passed to every one of the four
picture-loading calls above -- placing it at the corrected `+0x14`
(right after the corrected `regions`@`+0x10`), its 1024-byte span now
lands EXACTLY on `numobj`'s own already-confirmed `+0x414` start with
ZERO remaining slack. The earlier "4 bytes unaccounted for" was never a
separate mystery at all -- it was this same 4-byte leading-field
misalignment, showing up a second time from a different angle,
correctly flagged as unresolved at the time rather than papered over.

**Process lesson, worth restating plainly**: a pre-existing IDA type
being flagged as "unverified, don't trust it" is not enough on its own
-- the SAME type can still smuggle itself into evidence indirectly,
through the disassembly's own symbolic display, unless every citation is
double-checked for whether it's showing a raw address or a name resolved
through that exact type. `apply_structs.py`'s leading four fields
corrected in place with the wrong offsets struck through and explained,
not silently rewritten, plus the new `ebscene[0]`/`pal[256]` fields
added; `matches.json`'s `load_main_block` entry extended with the full
correction.

## RoomStruct round 6: a second correction, this time to this session's own work

Kept reading straight past `load_main_block`'s own end (its final
section decodes `password` two ways -- a version<9 `+=60` fixup and a
version>=9 XOR against the "Avis Durgan" key, both operating on the
already-confirmed `+0x936`, a bonus third confirmation of that address)
and into its CALLER, `load_room`, which runs its own pre-load cleanup
before ever calling `load_main_block`. That cleanup code closed several
new fields -- and immediately overturned something round 5 itself had
just gotten wrong.

**`scripts`@`+0x39F4`** and **`compiled_script`@`+0x39F8`** close
cleanly: `load_room` frees the first via a plain `free()` (matching
2011's `char *scripts;`, a raw buffer) and the second via a DIFFERENT,
as-yet-unmatched destructor helper (`sub_42A4DB`, matching 2011's
`ccScript *compiled_script;`, which needs its own specialized cleanup,
not a plain `free()`) -- the differing destructor choice is itself
confirming evidence of the two fields' different types.

**The correction**: a loop right after, bounded by `[rstruc+0x3A00]`,
destroys `[rstruc+c*4+0x3A0C]` for `c=1..bound-1` -- matching source's
own `ebscene[]` cleanup, `for(c=1;c<num_bscenes;c++) {
destroy_bitmap(rstruc->ebscene[c]); rstruc->ebscene[c]=NULL; }` exactly.
This proves `ebscene[]`'s REAL, PERSISTENT array base is `+0x3A0C` --
directly contradicting round 5's own identification of `+0x00` as
`ebscene[0]` from just one round earlier. Round 5's reasoning wasn't
baseless (`load_main_block` really does read/write `+0x00` in a
sequence that looks exactly like the `load_lzw`/`recalced`/`ebscene[0]`
assignment from source), but it stopped one step too early: `[rst+0x00]`
gets read as `load_lzw`'s input, overwritten with the decompression
result, and then that result gets COPIED into the real destination,
`+0x3A0C` -- a transient holding spot, not `ebscene[0]`'s own persistent
home. `+0x00`'s true role is left honestly unconfirmed rather than
re-guessed a second time.

Immediately after the cleanup loop, `[rstruc+0x3A00]` gets reset to the
literal `1` and `[rstruc+0x3A04]` to the literal `5` -- matching 2011's
own `roomstruct` constructor defaults, `num_bscenes=1;
bscene_anim_speed=5;` (`acroom.h:889-890`), exactly and in the same
declared order -- confirming **`num_bscenes`@`+0x3A00`** and
**`bscene_anim_speed`@`+0x3A04`**. A `memset` right before the whole
cleanup block zeroes a bounded-but-unidentified 300-byte span between
`walk_area_light` and `scripts` (`+0x38C6..+0x39F2`) -- plausible
candidates (`walk_area_top[16]`/`walk_area_bottom[16]`) only account for
64 of those 300 bytes even if both are present, so real unexplored
territory remains even inside this newly-bounded span.

**Process note**: this is the first time in this struct's investigation
that a correction landed on THIS SESSION's own immediately-preceding
work, rather than catching an older mistake from a prior session. The
same discipline applied both times -- keep reading past the point where
a plausible-looking match was declared, and let contradicting evidence
override it rather than working around it.

`apply_structs.py`'s `+0x00` field comment rewritten to record the
correction (the old `ebscene[0]` claim struck through and explained, not
silently deleted); five new confirmed fields added (`scripts`/
`compiled_script`/`num_bscenes`/`bscene_anim_speed`/`ebscene[]`, the
last with an explicitly-flagged UNCONFIRMED capacity assumption of 10);
`matches.json`'s `load_room` entry extended with the full evidence
above.

## RoomStruct round 7: localvars confirmed absent, and a caught unforced error on capacity claims

Closed out this session's pass with two threads: settling
`localvars`/`numLocalVars`'s status, and catching a small but real
sloppiness in how this struct's capacity-drift claims were being made.

**`localvars`/`numLocalVars` are CONFIRMED ABSENT.** 2011 declares these
(`InteractionVariable *localvars; int numLocalVars;`, `acroom.h:868-869`)
behind a version>=19 `getw()` gate. An exhaustive count of every `getw()`
call across `load_main_block`'s ENTIRE body -- already fully read,
piece by piece, across this whole investigation -- finds exactly two,
both long since identified (`_acroom_bpp`'s version>=12 read, the
walk-area-count override's version>=14 read). No third call exists
anywhere for a version>=19 gate. This is independently reinforced by
`unload_old_room`/`EndSkippingUntilCharStops`-combined (itself
exhaustively read across several earlier GameState/RoomStatus rounds)
never containing the matching `thisroom.localvars[]`-to-
`interactionVariableValues[]` copy loop either -- two independent
negative results agreeing with each other, the same cross-confirmation
standard this project applies to every other confirmed-absent finding.

**A small, real mistake caught in passing**: while looking up
`MAX_BSCENE`'s actual value (needed to properly bound `ebscene[]`'s
capacity, left an open assumption since round 6), the same lookup in
`acroom.h` turned up `#define MAXANIMS 10` -- meaning `anims[10]`,
confirmed back in round 4 and captioned at the time as "the familiar
smaller-capacity pattern," is actually a ZERO-DRIFT match. The earlier
round asserted drift without ever having checked what 2011's own
constant actually was, defaulting to this project's general pattern
instead of verifying the specific number -- a real, if minor, lapse in
the same discipline this project otherwise holds itself to. Corrected
in place. The same lookup gives `MAX_BSCENE=5`, replacing round 6's
`10` (which had been guessed purely from the general pattern, the same
unverified-default mistake, just caught before it was stated as fact
rather than after) -- `ebscene[]`'s declared capacity is now `5`,
though, unlike `anims[]`, this specific number still isn't independently
confirmed against this build's own disassembly, just grounded in the
correct reference point instead of a guess.

**Lesson for future rounds, stated plainly this time**: "matches this
project's common smaller-capacity pattern" is not, by itself, evidence
-- it's a plausibility check that still requires looking up what the
2011 constant actually is before citing drift. Two capacity claims in
this same struct turned out to skip that check; both are now fixed.

`apply_structs.py`'s `anims[]` comment corrected to zero-drift; `ebscene[]`
resized from `[10]` to `[5]` with an honest "still unconfirmed for this
build" caveat kept in place; a new paragraph documents `localvars`/
`numLocalVars` as confirmed absent; the struct's fresh-survey intro
comment updated to stop citing the four leading-field IDA names as a
`SpriteCache`-style success story, given round 5's correction.
`matches.json`'s `load_main_block` entry extended with the full evidence
above.

## RoomStruct round 8: a capstone finding -- this build's engine never compiled room-format v15+ at all

Went looking for `bpalettes[]` (a per-`ebscene`-slot palette array, the
natural next field after `ebscene[]`) via `on_background_frame_change`'s
own `memcpy(palette, thisroom.bpalettes[play.bg_frame], 1024)` call --
searched for a matching 1024-byte `memcpy`, a `shl reg,0Ah` (multiply-
by-1024) index computation, and the confirmed already-known
`num_bscenes` global via its computed absolute address (`0x51F688 +
0x3A00 = 0x523088`, a nice bonus: this INDEPENDENTLY reconfirms
`num_bscenes`@`+0x3A00` a third way, via `mainloop`'s own bg-frame-cycle
wraparound check using the same field under yet another guise). None of
those searches turned up `bpalettes[]` itself -- still an open lead.

**But the search for `bpalettes[]` led somewhere more valuable.** While
checking version gates near the palette-copy logic, it became worth
listing EVERY room-file-version comparison `load_main_block` makes,
across its entire body, in one place instead of encountering them
piecemeal round after round. The complete list: `3, 4, 5, 6, 7, 8, 9,
10, 11, 12, 13, 14` -- a clean, gapless run, with NOTHING above 14
anywhere in the function.

This is a genuinely different kind of fact than anything else found in
this struct so far. Every previous "confirmed absent" finding in this
project rests on watching a SPECIFIC feature's code path be missing
(a flag test, a function call, a field write). This one is structural:
**this build's compiled engine never had ANY code for room-file format
version 15 or higher, for anything.** Not this game's room files
happening to be old-format -- the compiled `load_main_block` itself
doesn't contain the branches. One fact, checked once, retroactively
confirms absent everything 2011 gates version>=15 or higher, replacing
what had been a pile of separate per-field guesses and precedent-based
inferences with a single, decisive explanation:

- `walk_area_zoom2[16]`/`walk_area_top[16]`/`walk_area_bottom[16]`
  (version>=18) -- closes the open candidate list for the 300-byte pad
  before `scripts`, definitively RULING THEM OUT rather than leaving
  them as unconfirmed possibilities. That pad's real contents remain
  genuinely unknown -- eliminating the wrong candidates isn't the same
  as finding the right ones.
- `numLocalVars`/`localvars` (version>=19) -- independently reconfirms
  round 7's separately-derived finding a second way.
- `numRegions`/`regionLightLevel`/`regionTintLevel`, and the ENTIRE
  `NewInteraction`-based `intrHotspot`/`intrObject`/`intrRoom`/
  `intrRegion` deserialization block (version>=21) -- upgrades
  `intrObject[]`/`objectScripts` from "plausible by precedent" (round 3's
  own cautious phrasing) to decisively confirmed absent.
- `hotspotScriptNames` (version>=24), `gameId` (version>=25),
  `hotspotScripts`/`objectScripts`/`regionScripts`/`roomScripts`/
  `deserialize_interaction_scripts` (version>=26).

**A small bonus fell out too**: the message-string-reading helper
(`sub_403024`, matched in round 4 but never definitively named) is
necessarily `fgetstring_limit` (2011's version<22 fallback), not
`read_string_decrypt` (version>=22) -- its unconditional call shape
already suggested this, and the version ceiling now makes it the only
path that COULD exist here at all.

Two stale leftover comments got corrected along the way, caught while
cross-checking this finding against the struct's own existing text: the
`+0x1574..+0x23C0` pad still described `left`/`right`/`top`/`bottom`/
`numsprs`/`sprs[]` as *candidates* for that span, even though rounds 3-4
had already located all of them at `+0x8C6..+0x936` instead -- a stale
note from round 1 that never got updated when the real answer was
found two rounds later. Fixed in place.

`apply_structs.py`'s `scripts` field comment gets the full version-
ceiling writeup (the natural anchor point, right where the newly-
resolved 300-byte pad ends); the `intrObject[]`/`objectScripts` and
`+0x1574..+0x23C0` comments corrected; `matches.json`'s `load_main_block`
entry extended with the full evidence above.

## RoomStruct round 9: the block-dispatch loop closes the 300-byte mystery and bpalettes[]

Went looking for `bpalettes[]` by tracing `on_background_frame_change`'s
palette-copy call, and along the way discovered something bigger:
`load_room` doesn't read the room file directly at all. It dispatches on
a per-block TYPE byte (`fgetc`) to one of several handler branches,
matching source's own block-tagged room-file container format --
`load_main_block` (everything mapped so far in this struct) is only ONE
of those handlers, `BLOCKTYPE_MAIN`. The other block types each have
their own small handler, several of which touch `roomstruct` fields
`load_main_block` never does.

**This closes the 300-byte mystery flagged since round 6.**
`BLOCKTYPE_OBJECTNAMES`(5)'s handler does "if (fgetc(Stream)!=numsprs)
quit(...); fread(rstruc+0x38C6, ElementSize=0x1E(30),
Count=numsprs);" -- matching source's `objectnames[]` read
(`acroom.h:2133-2137`) exactly, `MAXOBJNAMELEN=30` matching the element
size with zero slack, capacity 10 (`300/30`) matching this build's
already-established `MAX_INIT_SPR=10`. **`objectnames[10][30]`@`+0x38C6`**
-- the span that survived two rounds of "candidates ruled out, real
contents still unknown" closes cleanly the moment the right evidence
source (a DIFFERENT function's disassembly, not more of
`load_main_block`) got checked.

**`bpalettes[]` turns out to be CONFIRMED ABSENT, and in an unusually
satisfying way.** `BLOCKTYPE_ANIMBKGRND`(6)'s handler loops over
background-scene indices calling `load_lzw`, but passes the SAME shared
`pal[256]`@`+0x14` as the palette argument every single time -- not a
per-index `bpalettes[ct]` address. 2011's own source shows exactly why:
right next to the live per-frame code sits a commented-out predecessor
line, `// fpos = load_lzw(files,rstruc->ebscene[ct],rstruc->pal,fpos);`
(`acroom.h:2162`) -- this build's disassembly matches that OLDER, dead
line verbatim, not the live one below it. The same pattern as the almp3
`MP3CHUNKSIZE` find from several rounds ago: a genuine historical
artifact, preserved as a comment in the reference source, that this
2002 build's actual behavior still matches. This build shares one
palette across every background frame rather than storing one per
frame -- `bpalettes[]` was never needed.

**A second, equally decisive absence closes alongside it.** Reading the
dispatch loop's own structure end to end shows it explicitly handles
ONLY block types 1, 2, 3, 4, 5, 6, 7, and the `0xFF` EOF sentinel --
types 3 and 4 (the old compiled-script formats) both hit an explicit
"old room format, please upgrade" `quit()`, and ANY other type value
(crucially, including 8=`BLOCKTYPE_PROPERTIES` and
9=`BLOCKTYPE_OBJECTSCRIPTNAMES`) falls through to a generic "unknown
block type %d" `quit()`. This is stronger evidence than any "no handler
found" conclusion elsewhere in this struct -- it's not that this build
doesn't populate `objectscriptnames[]`/the `CustomProperties`-based
`objProps[]`/`roomProps`/`hsProps` trio, it's that encountering either
block type in a room file would actively CRASH this build's engine.
Confirmed absent by a stronger standard than usual.

**Bonuses fell out along the way, mostly reconfirmations**: `numsprs`
@`+0x8CE` gets a third independent confirmation (the object-names
block's own inconsistency check); `num_bscenes`/`bscene_anim_speed`/
`ebscene[]` each get a second confirmation via the animated-background
handler; `scripts`@`+0x39F4` gets a third confirmation (`BLOCKTYPE_
SCRIPT`'s own malloc+fread+in-place-decrypt sequence, this build's old
pre-CSCOMP SeeR-era text-script format) and `compiled_script`@`+0x39F8`
a second (`BLOCKTYPE_COMPSCRIPT3`'s `fread_script` assignment, confirming
this build's OWN room files ship modern CSCOMP-compiled scripts, not the
old text format `scripts` exists to handle).

With this, every field either `load_room`'s or `load_main_block`'s own
read/write sequences reference has been mapped or ruled out. Further
progress on this struct needs either a different already-matched
function that happens to touch `rstruc`, or a dedicated round confirming
`ebscene[]`'s real capacity. `apply_structs.py`'s `objectnames[]`
declaration replaces the old 300-byte pad; `scripts`'s own comment
documents the block-dispatch-loop discovery and the two new confirmed-
absent findings; `ebscene[]`'s comment documents the `bpalettes[]`
finding; `matches.json`'s `load_room` entry extended with the full
evidence above.

## RoomStruct round 10: following round 9's own lead to a different function

Took round 9's own closing suggestion literally -- checked whether a
DIFFERENT already-matched function touching `rstruc` had anything left
to say. `load_new_room` (the function that calls `load_room` in the
first place, already matched) turned out to reference `rstruc`'s
absolute address directly in two more places, past its own call into
`load_room`.

**Two reconfirmations, both clean**: `resolution`@`+0x3884` gets a
second confirmation -- `load_new_room` compares it against
`current_screen_resolution_multiplier_x` to decide whether every
`ebscene[c]` bitmap needs resizing after a room change, matching 2011's
own resolution-mismatch handling role exactly. `ebscene[]`@`+0x3A0C`
gets a THIRD and FOURTH confirmation via two separate
`for(c=0;c<num_bscenes;c++)` loops in the same function -- one
converting each entry's color depth after a room change, one
conditionally resizing entries when the resolution check above trips --
both using the same already-confirmed `num_bscenes` bound and array
base address.

**The `+0x00` mystery gets clarified, though not fully closed.** Right
after the resize loop, `load_new_room` does "`[rstruc+0] = ebscene[0]`"
-- an explicit, direct assignment. This doesn't contradict round 6's
conclusion (that `+0x00` isn't `ebscene[0]`'s own persistent home); it
reinforces the alternative theory from that same round -- `+0x00`
functions as a working CACHE of "the currently active background
bitmap," refreshed here after every room load or resize, not a
persistent array slot in its own right. What this field's "official"
2011 identity is (if it maps to any declared field at all, rather than
being scratch state this build layers on top of `roomstruct`) remains
genuinely open.

No new field addresses closed this round -- a legitimate "kept looking,
found reinforcement rather than new territory" result, worth recording
honestly rather than padding out with speculation. `apply_structs.py`'s
`resolution`/`ebscene[]`/`+0x00` comments extended with the new
evidence; `matches.json`'s `load_new_room` entry extended with the full
writeup above.

## Fresh survey: AmbientSound -- a clean, complete recovery in a single round

With `RoomStruct` genuinely exhausted after ten rounds, picked a new
target: 2011's `AmbientSound ambient[MAX_SOUND_CHANNELS+1]`
(`Common/acruntim.h:25-33`, a small per-channel struct: `channel`,
`x`,`y`, `vol`, `num`, `maxdist`). `PlayAmbientSound` (already matched)
had already been read in passing during earlier rounds (the `my_load_wave`
investigation), which made this a fast, well-anchored start.

**The struct-as-array is CONFIRMED ABSENT, in one clean pass.**
`PlayAmbientSound`'s very first check is "`if (channel!=1) quit(...)`"
-- a hard-coded single-channel restriction, unlike 2011's range check
against `MAX_SOUND_CHANNELS`. Every field write that would be
`ambient[channel].FIELD` in 2011 instead targets a bare scalar global
here: `dword_4EDA68`=`num`, `dword_4EDA6C`=`maxdist` (computed inline as
"`((x>width/2)?x:(width-x))-25`", matching `AMBIENCE_FULL_DIST`=25
exactly, `width` read via the already-confirmed `RoomStruct.width`), and
two literally-named globals, `x`/`y`/`vol` (each individually confirmed
via a DATA XREF to a second function, not just inferred from
`PlayAmbientSound` alone). `channel` itself is confirmed absent as a
stored field -- with it hard-locked to `1`, storing it back would be
redundant, and no write site for it exists.

**That second function closes the loop completely.** `sub_4089CC`
(previously an unnamed medium-confidence lead, flagged several rounds
ago only for incidentally confirming `GameState.sound_volume`) turns out
to be `update_ambient_sound_vol` itself -- reading it in full shows a
complete, exact match to source's per-channel distance-based volume
falloff: full volume if the sound has no position, full volume within
25 units of the player, otherwise a linear falloff scaled by `maxdist`.
Every scalar global identified in `PlayAmbientSound` gets a SECOND,
independent confirmation here, including a second read of `maxdist` as
the falloff divisor. Named and upgraded to high confidence in one
round -- unlike most function-identity upgrades in this project, this
one didn't require correcting an earlier mistake, just finishing a lead
that had been left half-investigated.

**A drift note fell out in passing**: `PlayAmbientSound` tries MP3
first (`sub_408811`) then falls back to WAV (`my_load_wave`, already
matched), unlike 2011's later, unified `load_sound_from_path()` call --
consistent with this build's established pattern of separate, per-
format loading logic everywhere audio gets loaded. `sub_408811` itself
is structurally a near-duplicate of the already-matched
`my_load_static_mp3`/`sub_4083FC` (same `pack_fopen`/`malloc`/
`pack_fread`/`pack_fclose`/`almp3_create_mp3` shape) but is a distinct
function with its own global and call site -- left unresolved rather
than assumed identical, a genuine open lead for a future round.

Unlike `RoomStruct`'s long, incremental byte-by-byte mapping, this
target resolved in a single focused round precisely because the answer
turned out to be "the array doesn't exist" rather than "here is its
layout" -- a useful reminder that not every fresh-survey target needs
many rounds; sometimes the fastest path to a complete, confident answer
is proving there's nothing left to map. `apply_structs.py` gets a new
`AmbientSound` documentation block (a comment, like `CharacterExtras`,
not a formal struct declaration, since there's no shared byte layout to
declare -- just independent scalar globals); `matches.json`'s
`PlayAmbientSound` entry extended with the full finding, and
`sub_4089CC`'s entry upgraded to the named, high-confidence
`update_ambient_sound_vol` match.

## CORRECTION: sub_408811 is my_load_static_mp3, not sub_4083FC

Immediate follow-up: chased the open lead from the previous round --
`sub_408811` (`PlayAmbientSound`'s own MP3 loader) had been flagged as
"structurally a near-duplicate" of `sub_4083FC`, which already carried
the `my_load_static_mp3` name. Reading `sub_408811`'s full body settled
it decisively rather than leaving it as a coin-flip.

`sub_408811` matches 2011's real `my_load_static_mp3`
(`acsound.cpp:439-477`) at the level of exact field offsets, not just
overall algorithm shape: `[ecx+0xC]=voll` (matching `thismp3->vol=voll;`),
`[eax+0x10]=0` (matching `thismp3->mp3buffer=NULL;`), `[ecx+0x14]=loop`
as a byte (matching `thismp3->repeat=loop;`), and `almp3_create_mp3`'s
result stored at `[edx+8]` (matching `thismp3->tune=...`) -- every one
of `MYSTATICMP3`'s declared member offsets confirmed independently, a
much stronger standard of evidence than the call-shape-only match
`sub_4083FC` had been carrying.

Re-reading `sub_4083FC` with that stronger standard in mind shows it
does NOT meet it: after calling `almp3_create_mp3`, it stores the RAW
return value directly into `dword_523214` (already established,
several rounds ago, as `PlayMusic`'s own MP3-stream-handle global) --
no `MYSTATICMP3` wrapper object gets allocated, and none of `vol`/
`mp3buffer`/`repeat` get set anywhere. This is a genuinely different
shape from `my_load_static_mp3`'s `SOUNDCLIP`-wrapping return type, not
a smaller variation on the same one. `sub_4083FC` is better understood
as `PlayMusic`'s own dedicated, inlined MP3-stream-preparation
helper -- this build keeps two separate, near-identical loading
implementations, one per caller, rather than one shared reusable
function, consistent with the same "no unified loader yet" pattern
already found repeatedly elsewhere in this project (`play_speech`'s own
separate `my_load_wave`/`my_load_mp3` calls, `PlayAmbientSound`'s own
separate MP3/WAV attempts, etc.).

`sub_4083FC`'s match entry is corrected in place -- the wrong
`my_load_static_mp3` name retracted, left unnamed (consistent with this
project's convention for functions whose role matches but whose actual
implementation diverges too much to claim identity, alongside
`sub_42B394`/`sub_40A6D8`), with the retraction and reasoning kept
visible in the entry rather than silently edited. A new entry records
`sub_408811` = `my_load_static_mp3` at high confidence.
`apply_structs.py`'s `AmbientSound` documentation block updated to drop
the "open lead" hedge and record the settled correction instead.

## Fresh survey: SOUNDCLIP, MYWAVE, MYMP3, MYSTATICMP3 -- consolidating scattered evidence

Rather than opening a brand-new investigation, this round went back and
formalized something that had been accumulating in pieces across
several earlier rounds without ever being written down as an actual
struct: the `SOUNDCLIP`-derived sound-wrapper classes AGS's `my_load_
wave`/`my_load_mp3`/`my_load_static_mp3` (all three already matched)
each construct and populate. `Common/acsound.h` turned out to still be
present in this repo's reference checkout (only `acsound.cpp` had been
consulted before), giving direct access to 2011's own declared
`SOUNDCLIP` base class and, via `acsound.cpp` itself, the exact declared
field order for all three derived classes -- a rare case in this project
of having the COMPLETE reference declaration in hand from the start,
rather than reconstructing it purely from disassembly evidence.

**The headline finding: this build's `SOUNDCLIP` base class is
drastically smaller than 2011's.** 2011's version carries 13 `int`
fields, a `bool`, and a pointer -- roughly 0x40 bytes of shared overhead
(volume-percentage tracking, directional/positional audio, pause/resume
state) before any derived class's own fields even start. Every one of
this build's three derived classes has its own first field sitting at
just `+0x08`, meaning the real base here is `{vtable; int done;}` -- 8
bytes total. This is a different KIND of drift than almost everything
else found in this project: not a smaller array capacity or an absent
later-added field, but an entire abstract base class collapsed to its
bare minimum, predating the whole volume-percentage/positional-audio
feature set that would eventually bulk it out.

**`MYWAVE` (16 bytes) closes with zero drift in its own two fields**:
`wave`@`+0x08` and `voice`@`+0x0C` both match 2011's own declared order
exactly (`SAMPLE *wave; int voice;`) -- the earlier, less careful first
pass at this comment had wrongly suggested these positions drifted, a
mistake caught and fixed by actually reading `acsound.cpp`'s struct
declaration directly instead of reasoning from the constructor-body
assignment order alone. What's genuinely absent is 2011's trailing
`firstTime`/`repeat` pair: the struct's own confirmed total size (16
bytes) ends immediately after `voice`, with no room for them. This
build's `my_load_wave` calls `play_sample` eagerly, right at load time,
storing the resulting voice handle immediately -- rather than 2011's
lazy design (store `vol`/`firstTime`/`repeat`, call `play_sample` later
inside `poll()`/`play()`).

**`MYMP3` (24 bytes) gives a clean example of this project's more
familiar kind of drift, layered on top of the unfamiliar one.**
`stream`@`+0x08`, `in`@`+0x0C`, and `chunksize`@`+0x14` all match 2011's
own declared positions exactly (zero drift) -- but `buffer` sits at
`+0x10`, immediately after `in` with zero gap, where 2011 declares a
`long filesize;` in between. That's a clean, decisive proof `filesize`
is CONFIRMED ABSENT here, the same "no room for it between two
confirmed neighbors" standard used throughout this project. Along the
way, `almp3_create_mp3stream` (`sub_47ED10`) gets identified as a new
function match -- its third argument, `(mp3in->todo<1)`, computed via a
signed less-than test, matches source's own expression precisely.

**`MYSTATICMP3` (24 bytes) shows the base-class shrinkage forcing a real
structural adaptation, not just an absence.** `tune`@`+0x08` matches
2011's own declared position exactly. But `vol` and `repeat` -- fields
2011 doesn't declare locally in `MYSTATICMP3` at all, inheriting them
instead from the bulky `SOUNDCLIP` base -- had to become LOCAL fields in
this build's version, since its own minimal `SOUNDCLIP` base doesn't
provide them. They land at `+0x0C` and `+0x14` respectively, flanking
`mp3buffer`@`+0x10` (itself shifted 4 bytes later than 2011's position
by the newly-local `vol` immediately before it). A genuinely different
kind of finding from anything else in this project: not fields going
missing, but fields moving DOWN the inheritance hierarchy to compensate
for a base class that no longer carries them.

`apply_structs.py` gets four new struct declarations (`SOUNDCLIP`,
`MYWAVE`, `MYMP3`, `MYSTATICMP3`); `matches.json` gets a new
`almp3_create_mp3stream` entry plus consolidated field evidence added to
`my_load_wave`/`my_load_mp3`/`my_load_static_mp3`'s existing entries.

## MAJOR CORRECTION: `EventBlockCmd`/`GameAnimation` are real 2011 structs after all -- `AnimationStruct`/`FullAnimation`

An earlier session drafted `EventBlockCmd` (the 24-byte command record
processed by `sub_40C3E0`/`sub_40C75E`, still unnamed) and `GameAnimation`
(the 244-byte `command[10]+numCommands` table wrapping it, backing this
build's undocumented "Animations" resource system) with an explicit,
confident claim: "unlike every other struct in this file, no living OR
dead-commented 2011 declaration corresponds to this one." That claim was
wrong -- just never actually tested against `Common/acroom.h` directly,
since the earlier investigation reasoned entirely from `run_event_block`'s
own 2011-absence (correctly established) without checking whether the
underlying DATA FORMAT, independent of who processes it, might still be
declared somewhere.

Re-reading `acroom.h` this round (while chasing an unrelated `RoomStruct`
thread) turned up `AnimationStruct` (lines 218-226) and `FullAnimation`
(lines 228-232):

```c
#define MAXANIMSTAGES 10
struct AnimationStruct {
  int   x, y;
  int   data;
  int   object;
  int   speed;
  char  action;
  char  wait;
  AnimationStruct() { action = 0; object = 0; wait = 1; speed = 5; }
};

struct FullAnimation {
  AnimationStruct stage[MAXANIMSTAGES];
  int             numstages;
  FullAnimation() { numstages = 0; }
};
```

The arithmetic lines up immediately: `AnimationStruct` is 5 ints + 2
chars, naturally padded to a 24-byte (`0x18`) stride -- exactly this
project's own already-confirmed `EventBlockCmd` size, independently
derived purely from `sub_40C75E`'s `for(i=...;i<list[i*0x18])` loop
stride. `10 * 0x18 + sizeof(int) = 0xF4` (244 bytes) then lands EXACTLY
on `GameAnimation`'s own already-confirmed total size, itself
independently triangulated two different ways in an earlier round (the
`dword_52033C`/`unk_52024C` address delta, and `sub_40C75E`'s own
`list[+0xF0]=numCommands` access).

Size arithmetic alone could in principle be coincidence, so the real test
is field-by-field semantics -- and every single field lines up with zero
exceptions, most of them well beyond what arithmetic alone could produce:

| offset | this project's name (pre-rename) | role, independently derived from disassembly | `AnimationStruct` field (`acroom.h`) |
|---|---|---|---|
| `+0x00` | `data0` | target X coordinate (move/set-position) | `x` |
| `+0x04` | `data1` | target Y coordinate (move/set-position); reused as a repeat-flag bit for view/animate types | `y` |
| `+0x08` | `data2` | view/loop number -- generic "data" slot | `data` |
| `+0x0C` | `target` | entity selector (object/character index) | `object` |
| `+0x10` | `data3` | speed parameter | `speed` |
| `+0x14` | `type` | command-type discriminator byte, 0-5 | `action` |
| `+0x15` | `waitUntilDone` | "wait until finished" gate flag | `wait` |

Three of these (`data`, `object`, `speed`) are not just positionally
correct but were independently described, in this project's OWN prior-
round prose, using almost the identical English words 2011 uses as the
literal field name (`object` for "the entity selector", `speed` for "the
speed parameter", `data` for "a generic reusable slot") -- without any
knowledge at the time that those words were about to turn out to be the
real field names. That's a much stronger signal than the arithmetic fit
alone.

The `FullAnimation` connection closes a THIRD way, independently of
either the size arithmetic or the `EventBlockCmd`/`AnimationStruct` field
match: `RoomStruct.anims[10][0xF4]` (confirmed several rounds ago as "244
bytes each, matching 2011's declared `FullAnimation anims[MAXANIMS]` in
POSITION," but left as an unexplored raw byte blob at the time) is
*exactly* one `FullAnimation` per slot. 2011 still declares this field,
unchanged, in the CURRENT `roomstruct`:

```c
#define MAXANIMS      10
...
FullAnimation anims[MAXANIMS];
short         numanims;
```

matching this project's own already-confirmed `anims[10]` capacity and
its immediately-following `numanims` (short) field with zero gap and zero
drift. `RoomStruct.anims` is retyped from a raw byte blob to
`FullAnimation anims[10]` this round as a direct consequence.

**The one place the earlier claim was actually right**: 2011's own room
loader no longer reads the `anims[]` payload for current room versions --
`acroom.h:1897-1908` reads `numanims`, then does `fseek(opty,
sizeof(FullAnimation)*rstruc->numanims, SEEK_CUR)` with the real
`fread(&rstruc->anims[0], ...)` sitting right there, commented out. So
the FORMAT survives to 2011, declared and even still round-tripped in
`numanims`, but the payload itself is functionally dead weight 2011 skips
past on every load. This is the exact same "still fully live here, dead-
but-declared by 2011" pattern already documented for `RoomStatus`/
`RoomStruct`'s `hscond`/`objcond`/`misccond` (`EventBlock`-based room
interaction data) -- not a new pattern, just found again in a different
subsystem. What's still genuinely absent from 2011, and still this
project's own actual discovery rather than a rename, is this build's
specific USE of `FullAnimation` data as a standalone, room-independent,
10-slot global resource table (`unk_52024C[10]`) triggerable from
`EventBlock.respond[i]==4` -- matching the old AGS Editor's long-gone
"Animations" resource-pane concept. Only the underlying struct layout
turned out to have a living 2011 declaration; the processing code
(`sub_40C3E0`/`sub_40C75E`, both still unnamed -- no 2011 counterpart
exists for either) and this build's specific global-table usage of it did
not.

**Renamed** `EventBlockCmd` -> `AnimationStruct` and `GameAnimation` ->
`FullAnimation` throughout `apply_structs.py`, with field names updated to
match 2011 exactly (`data0`->`x`, `data1`->`y`, `data2`->`data`,
`target`->`object`, `data3`->`speed`, `type`->`action`,
`waitUntilDone`->`wait`; `command`->`stage`, `numCommands`->`numstages`)
-- per this project's aim #3 (matching reference signatures/field names to
ease the eventual C reconstruction pass), and per the established "visible
retraction, not silent edit" convention: the old placeholder names are
kept in both structs' own header comments, and `matches.json`'s
`run_event_block`/`sub_40C3E0`/`sub_40C75E` entries each get an appended
correction note rather than having their original (accurately-recorded-
at-the-time) prose rewritten.

## RoomStruct's last big gap: `wallpoints`/`PolyPoints`, plus a whole constructor cluster

Immediately following the `AnimationStruct`/`FullAnimation` correction, the
next round picked the single largest remaining unexplored span in
`RoomStruct`: a `0xE4C`(3660)-byte gap between `numwalkareas`@`+0x1570` and
`numhotspots`@`+0x23C0`, previously left as an undifferentiated unknown
after several rounds of guesses that turned out to be wrong (see the
"STALE NOTE CORRECTED" history already attached to that field).

Checking `acroom.h`'s declared field order between `numwalkareas` and
`numhotspots` showed exactly one field sits there: `PolyPoints
wallpoints[MAX_WALK_AREAS]`. `PolyPoints` (`acroom.h:252-255`: `int
x[MAXPOINTS=30]; int y[MAXPOINTS=30]; int numpoints;`) is 61 ints = 244
bytes, and `MAX_WALK_AREAS=15` (`acroom.h:250`) gives `15*244=3660=0xE4C`
-- landing EXACTLY on the confirmed gap size with zero slack, before any
disassembly evidence was even checked.

The disassembly confirmed it immediately: `load_main_block`'s fread
sequence right after the already-confirmed `numwalkareas` read is
`fread(rst+0x1574, ElementSize=0F4h(244), Count=[rst+0x1570])` --
matching source's `fread(&rstruc->wallpoints[0], sizeof(PolyPoints),
rstruc->numwalkareas, opty);` (`acroom.h:1694`) exactly. This closes the
entire gap in one shot, with `MAX_WALK_AREAS=15` matching zero drift from
2011 (unusual for this project -- most fixed capacities shrink).

Since `wallpoints` has literally zero references anywhere in `Engine/`
(only in `Common/acroom.h`'s room-file I/O code -- walkable-area polygon
vertex data is purely an AGS Editor/room-authoring concern; the runtime
only ever consults the pre-rasterized `walls` bitmap mask), there was no
obvious way to confirm `PolyPoints`'s own internal `x`/`y`/`numpoints`
split at the individual-field level, so the field was initially typed as
a raw `char wallpoints[15][0xF4]` blob -- matching this project's
existing `hswalkto[20]`/`_Point` convention for small utility structs
used only for one whole-array copy.

### A whole constructor cluster, found while closing the last two small pads

Two remaining small RoomStruct pads -- `_pad_unexplored2b[0x20]`
(`+0xBA4..0xBC4`, between `msgi[]` and `anims[]`) and the still-open
`cscriptsize`/`bytes_per_pixel` positions flanking `num_bscenes`/
`bscene_anim_speed` -- were tackled next, and turned into something much
bigger than expected.

`_pad_unexplored2b` closed via arithmetic first: 2011 declares `short
wasversion; short flagstates[MAX_FLAGS];` immediately after `msgi[]`
(`acroom.h:833-834`), and `MAX_FLAGS=15` (`acroom.h:801`) gives
`2+15*2=32=0x20` bytes -- landing exactly on the gap with zero remainder.
`wasversion`@`+0xBA4` then got DIRECT behavioral confirmation from
`load_room` (not `load_main_block` -- the caller, reading the room-file
header before handing off): `fread(&var_8,2,1,Stream);
rstruc.wasversion=var_8; if (rstruc.wasversion<2 ||
rstruc.wasversion>0Eh(14)) quit("Load_Room: Bad packed file. Either the
file requires a newer or older version of...")` matches source's
`rstruc->wasversion=rfh.version; if
((rstruc->wasversion<15)||(rstruc->wasversion>ROOM_FILE_VERSION))
quit("Load_Room: Bad packed file...");` (`acroom.h:2080-2088`) exactly,
including the error string. This build's own enforced bounds (2..14) sit
lower on BOTH ends than 2011's (15..29) -- reinforcing round 8's
capstone "this build's compiled engine never had code for room-format
version 15+" finding from a brand new angle, and additionally pinning
down this build's own `ROOM_FILE_VERSION`-equivalent at exactly 14.
`flagstates[15]` itself stays MEDIUM confidence (arithmetic fit only, no
direct access site -- the same evidentiary status as `RoomStatus.
flagstates[15]`'s own entry, its closest analogue).

Chasing `cscriptsize`/`bytes_per_pixel` led to `load_main_block`'s own
`_acroom_bpp`-driven read (already known from round 7's evidence, never
followed to its actual destination): "if (version>=12) _acroom_bpp =
getw(Stream); else _acroom_bpp=1; if (_acroom_bpp<1) _acroom_bpp=1;
[rst+0x3A08]=_acroom_bpp;" matches source's "if (rfh.version>=12)
_acroom_bpp=getw(opty); else _acroom_bpp=1; if (_acroom_bpp<1)
_acroom_bpp=1; rstruc->bytes_per_pixel=_acroom_bpp;" (`acroom.h:
1641-1649`) line for line, decisively closing `bytes_per_pixel`@`+0x3A08`.

That in turn pointed at a genuinely new lead: a small, previously
uncharacterized function (`sub_424570`) referenced from `sub_4081C5`, not
yet read. It turned out to be `roomstruct::roomstruct()` -- this build's
`RoomStruct` DEFAULT CONSTRUCTOR -- and reading it in full turned into
the single most productive function-read of this entire investigation.
Its body is an almost line-for-line match to source's own constructor
(`acroom.h:878-901`):

- Three C++ `vector constructor iterator` calls default-construct three
  of RoomStruct's own array fields, each independently reconfirming an
  address/stride/capacity ALREADY established from completely different
  (fread-based) evidence: `sprs[10]` (ElementSize=0xA/Count=0xA, callback
  `sub_424750`), `anims[10]` (ElementSize=0xF4/Count=0xA, callback
  `sub_424770`), and -- the headline find -- `wallpoints[15]`
  (ElementSize=0xF4/Count=0xF, callback `sub_4247D0`), landing exactly on
  `wallpoints`'s own already fread-confirmed address/stride/capacity a
  SECOND, fully independent way.
- A long run of literal-value field inits matches source's constructor
  body almost assignment-for-assignment, including a five-in-a-row exact
  match -- "width=320; height=200; scripts=NULL; compiled_script=NULL;
  cscriptsize=0;" (`acroom.h:885-886`) -- that decisively closes
  `cscriptsize`@`+0x39FC`, and a further three-in-a-row match --
  "num_bscenes=1; ebscene[0]=NULL; bscene_anim_speed=5;
  bytes_per_pixel=1;" (`acroom.h:889-890`) -- that reconfirms
  `bytes_per_pixel` a second, independent way.
- `numobj`'s own default value (`0xF`/15) matches source's `numobj =
  MAX_OBJ;` idiom in role, but this build's own `MAX_OBJ`-equivalent
  default is 15, not 2011's declared `MAX_OBJ=16` -- a genuine one-off
  capacity reduction, caught only because the constructor happened to
  encode the constant directly.
- `wasversion`'s default (`0xE`/14) independently reconfirms the
  `ROOM_FILE_VERSION`-equivalent found moments earlier via `load_room`'s
  bounds check -- two completely different code paths agreeing on the
  same build-specific constant.
- The still-open `+0x00` field also gets zeroed here, in the SAME
  instruction group as `walls`/`object`/`lookat` -- interestingly
  mirroring a genuine, still-unexplained redundancy in 2011's OWN source,
  where `ebscene[0] = NULL;` appears twice in the constructor (once in
  the opening group with `walls`/`object`/`lookat`, `acroom.h:879`, and
  again later next to `num_bscenes=1`, `acroom.h:889`). This build
  zeroing `+0x00` specifically in the FIRST group, not the second (where
  the real, persistent `ebscene[]` array gets its own separate zero at
  `+0x3A0C`), is suggestive corroboration -- not proof -- that `+0x00`
  really is this build's counterpart to 2011's first, redundant
  `ebscene[0]=NULL` specifically, and plausibly explains WHY that
  redundancy exists in 2011's source at all: a vestige of a genuine
  transient/duplicate field once occupying this exact role.

### The payoff: three more tiny constructors, and a decisive AnimationStruct confirmation

Reading the three per-element constructor CALLBACKS the main constructor
invokes turned out to be even more valuable than the constructor itself:

- `sub_424750` (`sprstruc::sprstruc()`): a single 2-byte write, `[this+8]
  = 0`, matches source's `sprstruc() { on = 0; }` (`acroom.h:186`)
  exactly -- `sprstruc` (`sprnum, x, y, room, on`, all packed shorts,
  `acroom.h:181-198`) had never been formalized as its own type in this
  project before despite `RoomStruct.sprs[10]`'s 10-byte stride being
  known for many rounds; this pins down its last field directly and
  boxes in the rest with zero slack.
- `sub_424770` (`FullAnimation::FullAnimation()`): its own nested
  `vector constructor iterator` call over `stage[10]`
  (ElementSize=0x18/Count=0xA, callback `sub_4247A0`) independently
  reconfirms `AnimationStruct`'s stride/capacity a THIRD way, then zeroes
  `numstages`@`+0xF0` directly, matching source's `FullAnimation() {
  numstages = 0; }` (`acroom.h:231`) exactly -- a third confirmation of
  that field too.
- `sub_4247A0` (`AnimationStruct::AnimationStruct()`), the payoff: FOUR
  literal writes, `[this+0x14]=0; [this+0xC]=0; [this+0x15]=1;
  [this+0x10]=5;`, matching source's `AnimationStruct() { action=0;
  object=0; wait=1; speed=5; }` (`acroom.h:225`) WORD FOR WORD AND VALUE
  FOR VALUE -- hitting the exact same four fields (`action`, `object`,
  `wait`, `speed`) the previous round's disassembly-only investigation
  had already, independently, matched to those roles via `sub_40C3E0`'s
  dispatcher logic. This is about as strong as confirmation gets in this
  project: a full constructor-literal match on top of an already-solid
  semantic match, definitively closing any remaining doubt about last
  round's `EventBlockCmd`->`AnimationStruct` rename.
- `sub_4247D0` (`PolyPoints::PolyPoints()`): a single 4-byte write,
  `[this+0xF0] = 0`, matches source's `PolyPoints() { numpoints = 0; }`
  (`acroom.h:264`) exactly. This is the missing piece for `wallpoints` --
  the one field this struct's fread-only evidence could never reach
  (walkable-area polygon data being an editor-only concern with zero
  Engine/ usage) is now directly confirmed. `wallpoints` is retyped from
  a raw byte blob to a proper `PolyPoints wallpoints[15]` array; `x[30]`/
  `y[30]` remain boxed in by the confirmed total size and `numpoints`'s
  own confirmed position rather than independently field-confirmed, but
  no other layout is arithmetically possible.

Five new function matches this round (`roomstruct__roomstruct`,
`sprstruc__sprstruc`, `FullAnimation__FullAnimation`,
`AnimationStruct__AnimationStruct`, `PolyPoints__PolyPoints`), two new
formalized structs (`sprstruc`, `PolyPoints`), RoomStruct's last big gap
and both remaining small pads closed, and a decisive reinforcement of the
previous round's `AnimationStruct`/`FullAnimation` rename -- a
genuinely excellent return for following one dangling function reference.

## Two quick follow-ups: constructor completeness, and an exhausted lead

A short follow-up round picked up two loose ends left by
`roomstruct__roomstruct`'s own discovery.

**Constructor completeness closes three more absent-field questions.**
`roomstruct__roomstruct`'s body had already been read in full and ends in
a bare `retn` immediately after `bytes_per_pixel@+0x3A08=1`. 2011's own
constructor (`acroom.h:889-901`) doesn't stop there -- it continues with
`numLocalVars=0; localvars=NULL; lastLoadNumHotspots=0;
lastLoadNumRegions=0; lastLoadNumObjects=0;` and then a `for` loop
initializing `walk_area_zoom2[]`/`walk_area_top[]`/`walk_area_bottom[]`
(the latter three already confirmed absent via round 8's version-ceiling
argument). Since the disassembly's own version of this function is now
known to be read completely, its *failure* to contain any of those
assignments is direct positive evidence, not just an unfound access
site: `lastLoadNumHotspots`/`lastLoadNumObjects`/`lastLoadNumRegions`
(2011-only per-room-reload bookkeeping, `acroom.h:874-876`) are CONFIRMED
ABSENT from this build's `RoomStruct` entirely, and `localvars`/
`numLocalVars` (already confirmed absent twice over in round 7) pick up
a THIRD independent confirmation route. This also narrows what could
still occupy the still-unexplored territory past `+0x3A20`: every
2011-declared field known to specifically live there (`bpalettes`,
`localvars`/`numLocalVars`, and now the three `lastLoadNum*` fields) is
confirmed absent, leaving only `ebpalShared[MAX_BSCENE]` (itself
plausibly a later addition, per 2011's own "used internally by engine
atm" comment) and the already-confirmed-absent `CustomProperties
hsProps[MAX_HOTSPOTS]`/`gameId` as 2011-declared candidates for that
space at all.

**`ebscene[]`'s capacity: an exhausted lead, not an unexplored one.** An
earlier round had flagged this as "a real, well-defined target for a
future round (a bounds-check quit-message or a malloc/array-index
literal would settle it)". Enumerating every single reference to
`rst+0x3A0C` across the ENTIRE disassembly (6 total: `load_room`'s
pre-load cleanup loop, its `BLOCKTYPE_ANIMBKGRND`(6) handler,
`load_new_room`'s two resize/depth-conversion loops, and
`roomstruct__roomstruct`'s own single `ebscene[0]=NULL` write) found
that none of them use a fixed capacity literal -- every loop bound is
the DYNAMIC `num_bscenes` field instead. Checking 2011's own idiom
confirms this isn't a build-specific quirk: `SetBackgroundFrame`/
`GetBackgroundFrame`/etc. (`Engine/AC.CPP:21027` and others) all
bounds-check user input against `thisroom.num_bscenes` -- the room's own
declared count -- never against `MAX_BSCENE`, the array's fixed
capacity. So there was never going to be a literal-5 bounds check to
find here, in EITHER build. This project's usual technique (find a
bounds-check quit-message or an allocation-size literal) simply has no
site to apply to for this specific field. `ebscene[]`'s capacity is
recorded as genuinely unconfirmable by this route -- an exhausted lead
now, not an open one -- and stays sized to 5 purely as 2011's own current
`MAX_BSCENE` constant, without build-specific evidence backing that
number for this build specifically.

## Pivot to GUIMain's remaining fields: `clickEventHandler` confirmed unused, plus a new function match

With RoomStruct's field-level investigation essentially exhausted, this
round surveyed `GUIMain`'s own remaining MEDIUM-confidence fields
(`vtext`, `name`, `clickEventHandler`, `focus`, `bgcol`, `fgcol`,
`mousewasx`/`y`, `highlightobj`, `guiId`, `reserved[6]` -- `transparency`/
`zorder` were already exhaustively checked in an earlier round).

`clickEventHandler` stood out because, unlike `vtext` (2011's own comment
calls it vestigial "for compatibility"), 2011 still actively USES this
field: `process_interface_click` (`Engine/AC.CPP:5355-5391`) opens with
"if (btn<0) { run_text_script_2iparam(gameinst,
guis[ifce].clickEventHandler, (int)&scrGui[ifce], mbut); return; }" --
the "user clicked the GUI's background, not a specific control" case.
`process_interface_click` is already matched in this project
(`sub_40D738`), so its disassembly was worth re-reading specifically for
this field.

The result: this build's `process_interface_click` has NO such branch at
all. Its very first instructions unconditionally decode
`objrefptr[btn]>>16 & 0xFFFF` (the control-type dispatch) with no `btn<0`
check anywhere before or after. Checking the CALL SITE independently
confirms this isn't just an unread code path: `process_event` (already
matched) pushes only 2 arguments (`ifce`, `btn`) for this call, not the 3
(`ifce`, `btn`, `mbut`) 2011's signature needs -- confirmed by the
matching `add esp,8` cleanup immediately after (2 dwords, not 3). Reading
the function's remaining body end to end confirms `mbut` is never
referenced at all. This is decisive: this build's `process_interface_click`
is a genuine 2-argument predecessor that predates the "click GUI
background triggers `clickEventHandler`" feature entirely -- not an
unexercised branch, a never-compiled one. `clickEventHandler`'s own byte
offset stays positional/arithmetic-fit-only (still zero direct access-site
evidence for the field ITSELF), but its associated behavior is now
confirmed absent, which is worth recording in its own right.

The function's OTHER branch (`var_8==2`, `guis[ifce].objrefptr[btn]`
decoded as a control type this project hasn't yet individually
characterized) turned up a genuine new function match along the way:
`sub_409F23`, called there with `(gameinst, "interface_click", ifce,
btn)`, matches source's own fallback call
`run_text_script_2iparam(gameinst,"interface_click",ifce,btn);`
(`AC.CPP:5390`) exactly. Reading its body confirms it thoroughly: it
calls `prepare_text_script` (already matched) then `ccCallInstance`
(already matched) with the two int params, matching source's
`run_script_function_if_exist(sci,tsname,2,iparam,param2)` inlined
directly (this build predates that helper's own extraction into a
separate function -- yet another instance of this project's "one big
pre-refactor function, later split into several" pattern); on error it
builds and quits with the distinctive, exact string
`"run_text_script2: error %d (%s)"`; and it finishes with
`strnicmp(tsname,"interface_click",0xF); if(==0) guis_need_update=1;`
matching source (`AC.CPP:3391-3392`) exactly, down to the literal 15/0xF
length. 2011's OWN first branch in this function -- the `"on_event"`/
`run_claimable_event` special case, a later addition -- is entirely
absent from this build's version, consistent with the broader pattern of
this build predating AGS's claimable-event system. Bonus: identifies
`dword_523134` as `gameinst`, confirmed via `process_interface_click`'s
own call-site push order.

## `GUIMain.fgcol` closes via `wtextcolor`, plus an inlined-and-simplified `adjust_y_for_guis`

Continuing the `GUIMain` field survey, `_display_main` (already matched
-- the core text/speech-display positioning function) turned out to have
an entire chunk of 2011's `draw_text_window_and_bar` (`Engine/AC.CPP:
12573-`) INLINED directly into it, rather than calling it as a separate
function -- the familiar "one big pre-refactor function, later split
apart" pattern seen repeatedly elsewhere in this project (`sub_42B394`/
`cc_run_code`, `offset_over_inv`, `unload_old_room`, and now this).

Two distinct inlined branches were found, matching two different pieces
of source:

1. **An `adjust_y_for_guis`-shaped branch** (source role at `AC.CPP:
   12757-12777`): loops over `guis[]` checking `on@+0x90>=1`, then
   `y@+0x2C<=yy`, then adjusts `yy` when it falls inside `y..y+hit`
   (`hit@+0x34`). This gives `on`/`y`/`hit` further reconfirmations, but
   is a genuinely SIMPLER predecessor of 2011's version -- missing the
   "totally transparent GUI, ignore" check (`bgcol==0 && bgpic<1`) and
   the "full-height GUI down the side, ignore" check (`hit >
   get_fixed_pixel_size(50)`) entirely. So despite superficially matching
   `adjust_y_for_guis`'s ROLE, this specific site doesn't extend to
   `bgcol`/`bgpic` field evidence -- those checks simply aren't compiled
   in here.
2. **The custom-speech-GUI branch** (source role at `AC.CPP:12902-12931`):
   when a custom text-window GUI is configured, does `wtextcolor(guis
   [ifnum].fgcol)` before drawing speech text. The disassembly's exact
   counterpart, `push [guis+ifnum*184h+50h]; call sub_401F62`, matches
   this precisely. `ifnum` here is this build's already-confirmed
   `GameState.speech_textwindow_gui` global (established several rounds
   ago via `main`'s own evidence) picking up a brand new reader -- 2011's
   own version computes an equivalent LOCAL (`usingGui = play.
   speech_textwindow_gui;`) at this exact point rather than reading the
   global directly, another instance of this project's "locals collapsed
   into direct global reads" simplification pattern.

`sub_401F62` itself needed independent confirmation before it could
support the `fgcol` claim, and got a decisive one: `GUILabel__Draw`
(already matched) calls it as `wtextcolor([this+0xEC])`, matching source's
`wtextcolor(textcol);` (`acgui.cpp:354`) exactly, where `[this+0xEC]` is
`GUILabel.textcol`, itself already independently confirmed via
`GUILabel__ReadFromFile`'s own default-value logic. With `sub_401F62`
confirmed as `wtextcolor` (24 call sites total across the disassembly,
consistent with a widely-used simple wrapper), the `_display_main` call
decisively confirms `GUIMain.fgcol`@`+0x50`, upgrading it from MEDIUM to
HIGH confidence -- the first of `GUIMain`'s remaining MEDIUM fields to
close this round. `vtext`/`name`/`clickEventHandler`/`focus`/`bgcol`/
`mousewasx`/`mousewasy`/`highlightobj`/`guiId`/`reserved[6]` remain open.

## `guiId`/`zorder`: two more negative results, plus a `guin`/`objn` architectural lead

A follow-up round checked `focus` first: `GUIMain.focus` ("which object
has the focus") turns out to have ZERO usages anywhere in 2011's own
`Engine/` source -- genuinely vestigial even in the reference build, the
same status as `vtext`. Not worth chasing further in this build, matching
the established "no living code path to find" precedent (`InterfaceElement`,
`vtext`).

`guiId` looked more promising: 2011 writes it in exactly two places --
`GUIMain::rebuild_array()`'s `objs[ff]->guin=this->guiId;` (`acgui.cpp:
1128`) and `read_gui`'s own post-load loop, `guiread[ee].guiId=ee;`
(`acgui.cpp:1487`) -- and both of those CALLERS are already matched in
this project (`GUIMain__rebuild_array`/`read_gui`), making this a
well-scoped lead to re-read both functions in full for the first time.

The result was a clean negative on both counts, but a genuinely useful
one. `GUIMain__rebuild_array`'s loop resolves `objs[ff]` via its already-
documented 6-way type dispatch, but NEVER writes `guin`/`objn` afterward,
and the function ends immediately with no `resort_zorder()` call at the
end at all (source's own trailing statement, `acgui.cpp:1132`) -- both
omissions independently reinforcing (from inside this function's own
body, not just a caller's evidence) the already-established finding that
GUI z-order sorting is absent from this build. `read_gui`'s per-GUI
post-load loop matches source's `hit<2` clamp (`acgui.cpp:1478-1480`)
exactly -- a further reconfirmation of `hit`@`+0x34` -- then calls
`GUIMain__rebuild_array` immediately, with NONE of source's intervening
version-gated defaults (`gver<103` name default, `gver<105` zorder
default) or the unconditional `guiId=ee` assignment present anywhere.
This build's GUI-format version-gating for this specific era of fields
is simply absent, consistent with the "predates this feature entirely"
pattern already established for z-order elsewhere.

A genuine architectural lead fell out of the `guin`/`objn` absence,
worth flagging even though it isn't chased down this round: this
project's own already-independently-confirmed `x`@`+0x08` positioning
for the shared `GUIObject` base (established via `GUIButton`/`GUISlider`/
etc.'s own field recovery rounds) only leaves room for a SINGLE 4-byte
field between the vtable and `x` -- not the THREE fields (`guin`, `objn`,
`flags`) 2011 declares in that exact span (`Common/acgui.h:128-130`).
Combined with `rebuild_array`'s own failure to ever write `guin`/`objn`
on a resolved object, this suggests `GUIObject.guin`/`.objn` may not
exist in this build's base class at all -- plausibly the same kind of
later addition as GUI z-order and the dynamic-GUI script-object system
(`scrGui`/`ccDynamicGUI`), which this build's own `guiScriptObjNames`-
free startup sequence (checked separately this round, see below) is
already consistent with predating entirely. Not formalized as a struct-
level finding since `GUIObject`'s own base layout was never formalized
as its own struct in `apply_structs.py` to begin with -- a candidate for
a future round if `GUIObject` itself becomes a fresh survey target.

One more dead end recorded for completeness: `GUIMain.name`'s one
promising-looking lead, `AC.CPP:11933-11953`'s `guiScriptObjNames`/
`scrGui`/`ccDynamicGUI` startup export loop (which calls
`guis[ee].rebuild_array()` a SECOND time, separately from `read_gui`'s
own load-time call), does not exist in this build at all --
`GUIMain__rebuild_array` has only ONE caller anywhere in the disassembly
(`read_gui`), not two. This is consistent with the broader `guin`/`objn`/
z-order picture: this build predates AGS's GUI-as-scriptable-object
system entirely, not just one isolated feature within it.

## `GUIMain::init()` found, closing nearly every remaining field at once -- with a genuinely new kind of caveat

Chasing `bgcol` further (its two known 2011 reader sites -- the compiled-
out `adjust_x/y_for_guis`, and `draw_gui_for_dialog_options`'s `wbar`-
based rectangle fill -- were both checked and ruled out this round;
`wbar` itself has ZERO occurrences anywhere in this entire 917k-line
binary, confirming the whole custom-dialog-options-GUI background-drawing
path is absent, not just unfound) led to reading the code immediately
surrounding `GUIMain__rebuild_array` and its neighboring static-array-
construction chain (`sub_407356` → `sub_407360`, a `vector constructor
iterator` wrapper for the static `GUIListBox` array) more closely.

Sitting in between `sub_407360`'s `endp` and `GUIMain__rebuild_array`'s
own `proc` is a block of loose instructions with content that is an
almost line-for-line match to source's `GUIMain::init()` (`acgui.cpp:
985-1000`):

```
[this+0x00] = 0   (single byte)      vtext[0] = 0;
[this+0x38] = 0                      focus = 0;
[this+0x3C] = 0                      numobjs = 0;
[this+0x54] = -1                     mouseover = -1;
[this+0x58] = -1                     mousewasx = -1;
[this+0x5C] = -1                     mousewasy = -1;
[this+0x60] = -1                     mousedownon = -1;
[this+0x64] = -1                     highlightobj = -1;
[this+0x90] = 1                      on = 1;
[this+0x50] = 1                      fgcol = 1;
[this+0x48] = 8                      bgcol = 8;
[this+0x68] = 0                      flags = 0;
```

Every field, every value, in almost the exact same order as source --
11 of source's 12 assignments match exactly (only `clickEventHandler[0]
= 0` is missing, discussed below). This single find closes or reconfirms
nearly every remaining `GUIMain` field at once: `focus`, `mousewasx`,
`mousewasy`, `highlightobj`, and the original target `bgcol` all go from
MEDIUM to HIGH confidence; `mousedownon`, `on`, and `flags` (already
HIGH) get further reconfirmation; and `fgcol=1` lines up EXACTLY with
this same round's separate `wtextcolor`-based confirmation of
`fgcol`@`+0x50` -- two completely independent evidence routes landing on
the identical value for the identical field, a nice cross-check.

**A genuinely new kind of caveat, not yet encountered in this project**:
this code has NO formal IDA function boundary at all -- no `proc`/`endp`
pair, no assigned name, no visible `CODE XREF` comment. It's not an
unnamed-but-properly-bounded `sub_XXXXXX` like the dozens of other
unnamed helpers this project has documented -- it's genuinely loose
instructions that IDA's auto-analysis never wrapped into a function
object, most likely because it has zero direct call-site cross-references
(the whole chain it's part of, rooted at `sub_407356`, is itself only
referenced via a DATA XREF from `.data`, consistent with an MSVC C++
static-initializer table entry that runs automatically before `main()`
rather than being called from an identifiable point in the code). This
means it CANNOT be given a normal `matches.json` function-match entry:
`apply_matches.py` resolves every `asm_name` via
`idc.get_name_ea_simple()`, which requires the name to already exist in
the IDB -- there is nothing to look up here. The finding is recorded as
pure field evidence attached to `GUIMain__rebuild_array`'s own entry
instead. A human will need to open the IDB, navigate to immediately after
`sub_407360`'s `endp`, and manually define the function (Alt-P) before it
can be formally named and given its own match entry -- a genuinely new
category of "next step" for this project, distinct from every previous
"unnamed but already-bounded helper" case.

**The one asymmetry worth flagging**: source zeroes BOTH `vtext[0]` and
`clickEventHandler[0]` back to back (`acgui.cpp:987-988`); this build's
version zeroes only `vtext[0]`. Combined with `process_interface_click`'s
already-established total lack of any code path that reads
`clickEventHandler` at all (previous round), this is a second, independent
piece of evidence consistent with `clickEventHandler` not existing as a
distinct field in this build's `GUIMain` -- though not fully decisive on
its own, since a compiler or source revision could in principle zero one
adjacent array and not another for unrelated reasons.

`GUIMain`'s remaining genuinely open fields after this round: `name`
(the version-gated `"GUI%d"` sprintf default was already shown absent
last round, and this constructor doesn't set it either -- consistent
with it being populated purely from the room/game file's own on-disk
data, never a runtime default), `clickEventHandler`'s own byte offset
(behavior confirmed absent, but the offset itself remains positional-
only), and `reserved[6]` (plausible genuinely-unused padding, matching
this project's established `InventoryItemInfo.reserved` precedent).

## Pivot to the shared `GUIObject` base: closing its last unconfirmed pad, `zorder`

With `GUIMain` essentially exhausted (`name`/`clickEventHandler`'s offset/
`reserved[6]` all being genuine, well-documented dead ends now), this
round pivoted to the shared `GUIObject` base class -- the fields common to
all six derived control structs (`GUIButton`/`GUISlider`/`GUILabel`/
`GUITextBox`/`GUIListBox`/`GUIInv`), a target explicitly flagged as a
candidate in an earlier round's `guin`/`objn` architectural note.

Each of the six structs already carried an identical, still-unconfirmed
4-byte pad at `+0x18..0x1C`, sitting between the already-confirmed `hit`
and `activated`. Checking 2011's `GUIObject::WriteToFile`/`ReadFromFile`
(`Engine/acgui.cpp:69-83`) settled it immediately: both read/write the
ENTIRE base class as one bulk block, `fread(&flags, sizeof(int),
BASEGOBJ_SIZE, ooo);`, with `BASEGOBJ_SIZE=7` (`Common/acgui.h:119`) --
exactly 7 consecutive ints starting at `flags`. This build's own already-
confirmed layout has `flags`@`+0x04` through `activated`@`+0x1C`
occupying EXACTLY that same 7-int/28-byte span, with only the one pad at
`+0x18` left unaccounted for -- meaning it MUST hold a real field for the
bulk-block argument (already implicitly relied on to confirm `activated`
in earlier rounds, just never followed all the way through for the
field in the middle) to hold at all. 2011's declared order for this exact
span (`flags, x, y, wid, hit, zorder, activated`, `acgui.h:128-133`) has
only one field between `hit` and `activated`: `zorder`.

Retyped `_pad_unknown[0x04]@+0x18` to `int zorder;` across all six
structs at once (identical text, safe for a single `replace_all`). This
is this build's own per-CONTROL z-order -- a distinct concept from
`GUIMain`'s already-established-unused per-GUI z-order, though plausibly
sharing its fate: 2011's per-control equivalent, `resort_zorder()`, is
likewise never called from this build's `GUIMain__rebuild_array` (already
established two rounds ago), so this field is most likely present in
memory but functionally inert here too -- consistent with, not
contradicting, the broader z-order-absence picture already built up
across this session.

This closes the LAST remaining unconfirmed gap in the shared `GUIObject`
base-class layout: every byte from the vtable through `activated` is now
either a confirmed field (`flags`, `x`, `y`, `wid`, `hit`, `zorder`,
`activated`) or the already-independently-confirmed `guin`/`objn`-absent
finding from two rounds ago. The full `GUIObject` base recovery arc
across this session: `guin`/`objn` shown likely absent (round before
last) -> `zorder` closes the one remaining gap (this round) -- a genuine,
if incidental, capstone to the `GUIObject` class hierarchy work.

## `RoomStruct.objyval[]`: closed by connecting two already-known facts

With `GUIObject`/`GUIMain` now thoroughly exhausted, this round swept the
whole file for any remaining `_pad_*` gaps across every struct, looking
for cases where evidence found in one round might settle a question left
open in an earlier, unrelated round -- a category of easy win this
project hasn't deliberately gone looking for before.

One turned up immediately: `RoomStruct.objyval[]`. Its start address and
element type were confirmed many rounds ago via `load_main_block`'s own
`fread(rst+0x416, ElementSize=2, Count=numobj)`, matching source's
`fread(&rstruc->objyval[0], 2, rstruc->numobj, opty);` (`acroom.h:1655`)
-- but that read is bounded by the DYNAMIC `numobj` field, not a
compile-time capacity, so the array's own fixed size (2011 declares it
`short objyval[MAX_OBJ]`) was left as an unconfirmed 30-byte pad,
explicitly noting "no direct read/write site observed for `MAX_OBJ`'s own
value."

That statement stopped being true several rounds later, just without
anyone going back to check: `roomstruct__roomstruct`'s own constructor
(found while chasing `wallpoints`/`PolyPoints`) sets `numobj`'s default
to the literal `0xF`(15), matching source's `numobj = MAX_OBJ;` idiom and
independently pinning this build's own `MAX_OBJ`-equivalent at exactly
15 -- a fact recorded on `numobj`'s own entry at the time, but never
cross-referenced back against `objyval[]`'s still-open capacity question
sitting right next to it. 15 shorts is exactly 30 bytes, landing with
zero remainder on the already-confirmed pad. Retyped `_pad_objyval_tail
[0x1E]` to `short objyval[15]` accordingly.

Both underlying facts were independently correct the moment they were
found; this round's only contribution is noticing they answer each
other. Worth remembering as a technique for future rounds: periodically
sweep for `_pad_*` fields whose "missing piece" (usually a capacity
constant) might already have been established elsewhere in the file
under a completely different investigation thread.

A pass over `GameSetupStructBase.__old_spriteflags[2100]` (the largest
remaining pad in the file) and `GameState.play_invorder[100]`'s own
long-standing "genuine GameState member, or coincidentally-adjacent
separate global?" question were also revisited this round, but neither
yielded new progress: `__old_spriteflags` has literally zero usages
anywhere in 2011's own source (not just `Engine/`, checked across
`Common/` too) -- a genuine dead end, matching `vtext`/`InterfaceElement`'s
established status, not worth re-flagging as an open lead going forward.
`play_invorder`'s membership question remains genuinely undecidable by
this project's own techniques: `update_invorder` (already matched)
addresses it as a directly-named global array (`play_invorder[edx*2]`),
which is exactly how x86 codegen renders EITHER a standalone global array
OR a member array of a larger global struct -- there is no compiler-level
distinction to exploit here, unlike the "same base register used for a
confirmed neighbor" pattern that has settled genuine ambiguities
elsewhere in this project. Left open, but flagged as likely unresolvable
without a fundamentally different kind of evidence than anything in this
project's existing toolkit.

## `curscript` identified, a small loose end from `run_text_script_2iparam`

A quick follow-up on an open detail noted (but not chased) when
`run_text_script_2iparam` (`sub_409F23`) was matched two rounds ago:
right after calling `prepare_text_script`, the function dereferences
`dword_52314C` ONCE and passes the result as `ccCallInstance`'s first
argument, rather than passing `prepare_text_script`'s own return value
or the raw `sci` parameter directly.

Checking 2011's `run_script_function_if_exist` (the function this build's
version has inlined) settles it immediately: `ccCallInstance(curscript
->inst, tsname, numParam, iparam, iparam2);` (`Engine/AC.CPP:3281`, the
matching 5-argument overload) -- `curscript` is a global `ExecutingScript
*`, set inside `prepare_text_script` itself (`curscript = &scripts
[num_scripts];`, `AC.CPP:3064`) to point at whichever script instance was
just forked/resolved for this call. The single dereference in the
disassembly is exactly `curscript->inst` -- `ExecutingScript`'s own
already-confirmed FIRST field, itself now given a THIRD independent
confirmation route (on top of `post_script_cleanup`'s array-indexed read
and `ExecutingScript::init`'s own zeroing).

`dword_52314C` is identified as `curscript` accordingly -- a small,
previously-unnamed global this project hadn't had occasion to pin down
before, closed in passing while sweeping for loose ends from recent
matches rather than through a dedicated investigation of its own.

## Fresh survey: `MYMIDI` -- confirmed absent, resolved in one round

With `RoomStruct`/`GUIMain`/`GUIObject` all thoroughly exhausted, this
round picked a genuinely fresh target: `MYMIDI` (`Engine/acsound.cpp:
916-1007`), 2011's `SOUNDCLIP`-derived wrapper for MIDI music -- the one
sibling in the `MYWAVE`/`MYMP3`/`MYSTATICMP3`/`MYMIDI` family this
project's earlier SOUNDCLIP-family round hadn't touched.

The question resolved fast, and in the same direction as `AmbientSound`:
this build has NO `MYMIDI` wrapper object at all. `PlayMusic` (already
matched, previously undocumented in `matches.json` despite being
correctly named for a long time -- a "retroactive documentation" case
like `main`/`SaveGameSlot`) tries a MIDI file with `sprintf(Buffer,
"music%d.mid", musicnum)`, then calls Allegro's `load_midi`/`play_midi`
DIRECTLY on the result, with no intervening `new`, vtable, or virtual
dispatch of any kind -- gluing everything together via three bare
globals instead of an object:

- `dword_5231B4` -- the raw `MIDI*` handle itself, matching 2011's
  `MYMIDI.tune` in ROLE, just unwrapped. This single global is checked
  for non-NULL as the "is a MIDI currently active" gate by FIVE
  independent, already-matched functions: `PlayMusic` (sets it),
  `scr_StopMusic` (clears it, after calling `stop_midi()`/`destroy_midi()`
  -- both newly matched this round, in exactly 2011's `MYMIDI::destroy()`
  call order), `IsMusicPlaying`, `GetMIDIPosition`, and
  `SeekMIDIPosition` (the latter three all previously bare
  linker-symbol-matched entries with zero field evidence recorded, now
  filled in).
- `dword_4BD8F8` -- Allegro's own `volatile long midi_pos;` global
  (`allegro/midi.h:110`), read directly by `GetMIDIPosition`/
  `IsMusicPlaying` in place of 2011's `MYMIDI::get_pos()`/`poll()`
  wrapper methods.
- `dword_5231BC` -- set to the music number on successful MIDI load,
  reset to 0 on `PlayMusic`'s two non-MIDI fallthrough paths. Plausibly a
  "currently active MIDI music number" tracker, but WRITE-ONLY
  everywhere else in the disassembly (zero read sites found anywhere) --
  left as a documented hypothesis, not asserted as a confirmed identity,
  consistent with this project's discipline about not forcing a name
  onto evidence that doesn't fully support it.

`GetMIDIPosition`'s own body is a clean example of the "simpler
predecessor" pattern seen throughout this project: `if (dword_5231B4!=0)
return dword_4BD8F8; else return -1;` versus source's four-branch version
(`play.silent_midi`, `current_music_type!=MUS_MIDI`, `play.fast_forward`,
`AC.CPP:8927-8936`) -- the core "is MIDI active, return its position"
logic survives, the later special-case branches don't exist yet.

Five new Allegro function matches this round (`load_midi`, `play_midi`,
`stop_midi`, `destroy_midi`, all confirmed via exact signature/call-shape
matches against `allegro/midi.h`) plus field evidence added to four
already-matched-but-undocumented functions (`scr_StopMusic`,
`IsMusicPlaying`, `GetMIDIPosition`, `SeekMIDIPosition`) and a full
retroactive documentation entry for `PlayMusic` itself. `MYMIDI.
lengthInSeconds` (2011's own addition, used only by `get_length_ms()`)
has no located counterpart in this build -- not exhaustively searched for
absence this round, since no already-matched function was found calling
anything resembling a "get music length" API here.

## Immediate follow-up: `MYMOD` closes the same way

`MYMIDI`'s clean resolution made its immediate JGMOD-based sibling,
`MYMOD` (`Engine/acsound.cpp:1030-1110`, the `#ifdef JGMOD_MOD_PLAYER`
branch -- matching this build's own confirmed module-music library, see
`reversing/notes/third-party-library-identification.md`), an obvious
next check, and it turned out already half-covered: `load_mod`/`play_mod`
had been matched in an earlier Task #10 round, their evidence already
noting (without saying so explicitly) that "the multi-extension-guessing
loop lives in `PlayMusic` directly... rather than a level up" -- exactly
the "no wrapper object" shape, just not yet framed that way or connected
to the rest of the picture.

Re-reading `PlayMusic`'s `.mod`/`.xm`/`.s3m` cascade (right after the
already-documented MIDI attempt) and `scr_StopMusic`'s cleanup branch
confirms it explicitly: `dword_5231B8` is the bare `JGMOD*` handle
(2011's `MYMOD.tune`, unwrapped), checked/cleared by the SAME five-
function pattern already established for MIDI's `dword_5231B4`.
`scr_StopMusic`'s MOD-cleanup sequence -- "if (opts_mod_player!=0 &&
is_mod_playing()) stop_mod(); if (dword_5231B8!=0)
destroy_mod(dword_5231B8); dword_5231B8=0;" -- matches 2011's `MYMOD::
destroy()` ("stop_mod(); destroy_mod(tune); tune=NULL;",
`acsound.cpp:1055-1057`) exactly in role. Three new function matches
fall out of it: `is_mod_playing` (confirmed via two independent call
sites, `scr_StopMusic`'s gate and `IsMusicPlaying`'s own OR-chain),
`stop_mod`, and `destroy_mod` -- all identified by call-shape/role alone
since no JGMOD source tree exists in this repo to check names against,
the same standard already applied to `load_mod`/`play_mod` themselves.

Two SOUNDCLIP-family siblings down (`MYMIDI`, `MYMOD`), both confirmed
absent as wrapper objects in one round each -- `MYOGG`/`MYSTATICOGG`
(Ogg Vorbis support, `acsound.cpp:494-915`) are the two remaining
untouched siblings, a natural next target: OGG is very plausibly an even
later addition than MP3, consistent with this project's own earlier
finding that speech loading here "tries MP3 then WAV... no OGG attempt
exists between WAV and MP3."

## `MYOGG`/`MYSTATICOGG` confirmed absent -- the cleanest result of the four

Checking the last two `SOUNDCLIP`-family siblings took one grep. Every
"ogg" hit (case-insensitive) was searched for across both the full
2727-entry extracted string dataset AND a direct pass over the entire
917k-line disassembly -- zero occurrences either way. This is the same
exhaustive-negative standard already used to rule out `apeg-1.2.1`/
`dumb-0.9.2` in their own dedicated rounds, and it lands just as
decisively here: this build's compiled engine has no Ogg Vorbis support
of any kind. No `MYOGG`/`MYSTATICOGG` wrapper objects, no `my_load_ogg`/
`my_load_static_ogg` functions -- and, going one step further than
`MYMIDI`/`MYMOD` (which are absent as *wrapper objects* but still use
real, present, already-matched underlying libraries, Allegro and JGMOD
respectively), OGG appears to be missing at every level: even the
underlying `vorbisfile`/`ogg.h` third-party dependency itself was
apparently not yet a concept AGS depended on as of this build's
2002-07-21 link date. Consistent with -- and now conclusively closing
the loop on -- this project's much earlier finding that speech loading
"tries MP3 then WAV... no OGG attempt exists between WAV and MP3," OGG
support is most plausibly a later AGS addition than MP3, arriving well
after this binary was built.

With this, all four `SOUNDCLIP`-family siblings surveyed this session
are accounted for: `MYMIDI`/`MYMOD` confirmed absent as wrapper objects
(the underlying libraries are present and already matched, just glued
together via bare globals instead of polymorphic objects), `MYOGG`/
`MYSTATICOGG` confirmed absent as a feature entirely. A clean, complete
close to the `SOUNDCLIP` hierarchy this project opened several rounds
ago with `SOUNDCLIP`/`MYWAVE`/`MYMP3`/`MYSTATICMP3`.

## `InventoryItemInfo.name[25]` closes via `GetInvName`

With the SOUNDCLIP family wrapped up, this round swept other structs'
remaining `_unconfirmed`/positional-only fields for anything with an
obvious, not-yet-traced reader -- `InventoryItemInfo.name[25]` fit that
description exactly: its own struct comment already named the likely
candidate, "presumably read/written by GetInvName/SetInvItemName-
equivalent functions not yet traced," but nobody had gone and traced it.

`GetInvName` turned out to already be correctly named in the IDB (a
bare linker-symbol match, `kind: "function"`, with zero field evidence
recorded) -- reading its body settles the question immediately: after
the usual bounds check, it does `imul eax,44h; add eax,offset
byte_51B854; push eax; call GetTranslation; ...;
strcpy(Destination,result);` -- indexing into `invinfo[]` with this
struct's own already-confirmed `0x44`(68)-byte stride and reading
starting at the array's BASE address (`byte_51B854`, i.e. element-
relative offset 0), then handing the result straight to `GetTranslation`.
This matches 2011's own `GetInvName`
(`strcpy(usebuf,get_translation(game.invinfo[indx].name));`) almost
certainly verbatim, and confirms `name[25]`@`+0x00` directly -- upgrading
it from an unconfirmed positional pad to a real, behaviorally-confirmed
field.

A useful bonus: this also cross-confirms `byte_51B854` as `invinfo[]`'s
own base address a THIRD independent way (previously anchored via
`pic`@`+0x1C`'s own confirmed address, `dword_51B870`) -- `byte_51B854 +
0x1C` lands EXACTLY on `dword_51B870` with zero slack, the two facts
mutually reinforcing each other. `InventoryItemInfo.cursorPic`@`+0x20`
remains the one still-open field in this struct (no access site found
yet that reads a value distinct from `pic` for cursor-picture purposes)
-- everything else is now confirmed.

## `MoveList.xpermove`/`ypermove` close via `do_movelist_move`

Same sweep, same technique, immediate next hit: `MoveList.xpermove[40]`/
`ypermove[40]` had been sitting at MEDIUM confidence (boxed in with zero
slack between `numstage` and `fromx`, but no access site of their own)
since the round that closed out the rest of `MoveList` via `find_route`.
`find_route` computes the ROUTE once; the per-frame CONSUMER of these
two fixed-point fields is a different, already-matched function,
`do_movelist_move` (`Engine/AC.CPP:17327`) -- called every frame by
`update_stuff` for each active move-list.

Its opening block is about as clean a confirmation as this project gets:
"`edx=cmls[+0x1EC]` (`onstage`); `ecx=cmls[edx*4+0xA4]`" immediately
followed by the identical pattern at `+0x144` -- matching source's
single line, "`fixed xpermove=cmls->xpermove[cmls->onstage],
ypermove=cmls->ypermove[cmls->onstage];`" (`AC.CPP:17331`), EXACTLY,
confirming both fields from one source statement. The same block goes on
to re-derive `pos[onstage+1]` (`cmls[onstage*4+4]`) and unpack it into
two 16-bit halves via `shr`/`and`, matching source's `short
targetx=short((cmls->pos[cmls->onstage+1]>>16)&0xffff); short
targety=...&0xffff);` just as exactly -- a bonus reconfirmation of
`pos[]`/`onstage` from a completely different function than the one that
originally confirmed them.

`MoveList` -- already one of this project's cleanest zero-drift matches
-- is now fully behaviorally confirmed field by field, with no remaining
positional-only fields at all.

## `sub_41D49B` is `run_dialog_script`, not `run_dialog_request` -- a self-caught correction

Continuing the sweep, `DialogTopic.entrypoints[15]` was next: its own
struct comment already predicted a `run_dialog_script`-shaped call site
reading it, so `do_conversation` (already matched) was re-read looking
for that access. It was there -- but the function it fed the value into
turned out to be a MISIDENTIFICATION discovered along the way, not
merely an unread caller.

`sub_41D49B` had been matched to `run_dialog_request` (`AC.CPP:21797`,
a small function whose whole body is "set `stop_dialog_at_end`, run a
`"dialog_request"` text-script hook, then interpret the result") on the
strength of a genuine, DATA-XREF-confirmed `"dialog_request"` string
inside its body, plus a structural argument about where `do_conversation`
calls it. Reading the FULL function this round shows that reasoning
stopped one step short: the `"dialog_request"` string sits inside just
ONE branch (`opcode==7`) of a much larger byte-code dispatch loop that
decodes one opcode byte at a time from a buffer starting at `dtpp->
optionscripts + offse` -- unmistakably `run_dialog_script`'s own
interpreter (`AC.CPP:21824`), not `run_dialog_request` at all.

Opcode 7's body is a decisive, line-for-line match to `run_dialog_request`'s
ENTIRE function ("`play.stop_dialog_at_end=DIALOG_RUNNING;
run_text_script_iparam(gameinst,"dialog_request",parmtr); if
(play.stop_dialog_at_end==DIALOG_STOP) return -2; if
(play.stop_dialog_at_end>=DIALOG_NEWTOPIC) {...}`", including the exact
`DIALOG_NEWTOPIC`(100)/`DIALOG_STOP`(2) literal comparisons) -- meaning
`run_dialog_request` has NO separate existence in this build at all,
its entire logic FUSED into `run_dialog_script`'s own opcode dispatch as
one case. The same "one big pre-refactor function, later split into
several" pattern already found repeatedly this project (`sub_42B394`/
the script interpreter, `offset_over_inv`, `unload_old_room`) -- this
function's OWN pre-existing evidence comment (for a neighboring helper)
had already namechecked this exact pattern ("same pattern as
call_function/cc_run_code and do_conversation/show_dialog_options")
without anyone noticing it applied to the enclosing function's own
identity too.

Both of `do_conversation`'s two call sites into this interpreter confirm
the correction and, as a bonus, both `DialogTopic` fields at once:
the STARTUP call passes `[parmtr+0x47A]` (`startupentrypoint`, already
confirmed) alongside the topic pointer, and the CHOSEN-OPTION call
passes `[parmtr+chose*2+0x45C]` (`entrypoints[chose]`, now newly
confirmed) the same way -- both matching 2011's real `run_dialog_script`
call shape (`dtpp`, `offse`) in role, just with this build's own
2-parameter predecessor signature (no separate `dialogID`/`optionIndex`
-- the option-index-driven `optionflags` bit manipulation 2011 does via
a separate call happens INSIDE this interpreter instead, as opcode 6,
reading its own index straight from the bytecode stream).

Renamed `sub_41D49B` from `run_dialog_request` to `run_dialog_script` in
`matches.json`, with the old (wrong) name and reasoning kept visible in
the entry per this project's "visible retraction" convention. The
interpreter's full opcode set is only partially traced (0/6/7
characterized so far) -- left as an open thread for a future round
rather than force complete coverage in one pass.

## The full `DCMD_*` opcode table -- and a bonus `CharacterInfo.talkview` confirmation

An immediate follow-up read `run_dialog_script`'s remaining ~150 lines
to completion, and it turned into one of this project's cleanest
"declared-but-dead-by-2011" finds. The interpreter's opcode byte is not
an invented enum -- it is 2011's own `DCMD_*` dialog-script byte-code
opcode set, `Common/acroom.h:2653-2669`, STILL DECLARED in the reference
source even though 2011's engine no longer uses them at all (its own
`run_dialog_script` now just compiles each topic into real script
bytecode and calls it via `run_text_script_iparam(dialogScriptsInst,
"_run_dialogN",...)` -- the dedicated mini-VM is entirely gone from the
live 2011 engine, only the enum survives as an artifact). The exact same
"declared but dead by 2011, still fully live here" pattern already found
with `EventBlock`/`AnimationStruct`.

Every opcode this build's interpreter handles matches its `DCMD_` name
and role exactly:

| opcode | `DCMD_*` name | this build's behavior |
|---|---|---|
| 1 | `SAY` | reads charID+msgID (2 bytes each); charID==999 -> `Display()` (narrator); else `GetTranslation`+`_displayspeech` (character says) |
| 2 | `OPTOFF` | `optionflags[idx] &= ~1` (clear `DFLG_ON`) |
| 3 | `OPTON` | `optionflags[idx] |= 1`, UNLESS `DFLG_OFFPERM`(2) is already set |
| 4 | `RETURN` | returns -1 |
| 5 | `STOPDIALOG` | returns -2 (`RUN_DIALOG_STOP_DIALOG`) |
| 6 | `OPTOFFFOREVER` | `optionflags[idx] = (optionflags[idx]&~1)|2` (clear ON, set OFFPERM) |
| 7 | `RUNTEXTSCRIPT` | the `run_dialog_request`-fused branch (see above) |
| 8 | `GOTODIALOG` | reads a 2-byte dialog number, returns it directly |
| 9 | `PLAYSOUND` | reads a 2-byte sound number, calls `PlaySound()` |
| 10 | `ADDINV` | reads a 2-byte item number, calls `add_inventory()` |
| 11 | `SETSPCHVIEW` | reads a 2-byte view number, decrements by 1, writes `game_chars[charID*0x140+4]` |
| 12 | `NEWROOM` | reads a 2-byte room number, calls `NewRoom()`, returns -2 |
| 0xFF | `ENDSCRIPT` | returns -1 |

Any opcode byte outside this set hits an explicit `"unknown dialog
command"` `quit()` -- proving the switch is EXHAUSTIVE, not just
unobserved-past-this-point. This is direct positive evidence (this
build would crash on encountering them) that the four `DCMD_*` constants
NOT handled -- `SETGLOBALINT`(13), `GIVESCORE`(14), `GOTOPREVIOUS`(15),
`LOSEINV`(16) -- are later additions to the dialog-script command set
that this build's compiled dialog scripts never emit and its
interpreter has no code path for at all.

The `SETSPCHVIEW`(11) handler is a genuine bonus: it decisively confirms
`CharacterInfo.talkview`@`+0x04`, previously sitting as a TENTATIVE,
purely positional guess since a much earlier round ("`defview`/`view`/
`room` landed at `+0x00`/`+0x08`/`+0x0C`, matching 2011's declared
adjacency -- `talkview` is the field that WOULD fall at `+0x04` if that
holds"). The guess turns out to have been correct all along, now with
real behavioral evidence: `game_chars[charID*0x140+4]` is exactly
`CharacterInfo.talkview`, written directly by the "change this
character's talking-head view" dialog command.

## `CharacterInfo.prevroom` closes, following up the `ags-archives/` detour

Returning to struct work after the `ags-archives/` detour, the natural
next target was the one field that detour left explicitly open:
`CharacterInfo.prevroom`@`+0x10`. The archive's own `TECHINFO.TXT` only
labels this offset "[used internally by AGS]" (not a confirmation of
which field), and `CHANGES.TXT`'s 2.15 entry ("Fixed prevroom text
script variable for following characters") confirms the OFFICIAL NAME
and era but not the exact disassembly access site -- so this was a
genuinely well-scoped, named lead to go chase directly, rather than
another round of pure inference.

2011's own `load_new_room` (`Engine/AC.CPP:4429-4432`) does exactly
`offsetx=0; offsety=0; forchar->prevroom=forchar->room;
forchar->room=newnum;` right near the top of the function, gated on
`forchar!=NULL`. This build's own `load_new_room` (already matched)
has the identical shape at the identical position: `offsetx=0;
offsety=0;` immediately followed by `eax=[forchar+0Ch]; [forchar+10h]
=eax; edx=[newnum]; [forchar+0Ch]=edx` -- reading `room`@`+0x0C` into a
register, writing it to `+0x10`, THEN overwriting `room`@`+0x0C` with
the new room number. A complete, multi-instruction, order-preserving
match to source, closing the loop this round's earlier detour opened.

`CharacterInfo` is now fully confirmed field by field except `actx`/
`acty`@`+0x10C`/`+0x10E` -- already checked and shelved as a plausible
later addition (2011's only usage site sits inside hardware-accelerated
drawing code this build has repeatedly been shown to predate) -- and
`loop`@`+0x38`, sitting at MEDIUM confidence pending a more direct
access site than the shared direction-lookup-table write already found.

## `CharacterInfo.loop` closes too, with a complete "turning around" match

That last remaining field didn't stay open long. `update_stuff`'s
"turning around before walking" branch (a 2.3-era feature, per
`ags-archives/ags230/docs/CHANGES.TXT`: "Added option to make characters
turn to face the new direction before starting to walk") is gated on
`walking >= 0x3E8`(1000, `TURNING_AROUND`) -- already-confirmed
`walking`@`+0x3C` -- and turns out to be a complete, decisive,
multi-field match to source's `AC.CPP:6526-6558`:

- Reads `loop`@`+0x38` as the SOLE argument to a previously-unnamed
  helper (`sub_40EB43`), incrementing the result by 1 -- matching
  source's `int wantloop = find_looporder_index(chi->loop) + 1;`
  exactly. `sub_40EB43` is accordingly matched to `find_looporder_index`.
- Clamps `wantloop` to the range `[0,7]` in a loop, validating each
  candidate against `views[view].numLoops` (via the already-confirmed
  `view`@`+0x08` and the `views` global's own confirmed `0x8D4` stride)
  and `CHF_NODIAGONAL`(8) against the already-confirmed `flags`@`+0x20`
  -- both matching source's validation conditions exactly.
- The candidate values themselves come from a previously-unidentified
  global 8-entry table, `dword_4B42C8` -- now identified as
  `turnlooporder[8] = {0, 6, 1, 7, 3, 5, 2, 4}` itself
  (`Engine/acchars.cpp:19`), the exact literal array this project's
  much earlier round had already found being WRITTEN somewhere without
  yet finding the table's own address.
- Finally: `dx = word[dword_4B42C8[wantloop*4]]; [this+0x38] = dx;` --
  `chi->loop = turnlooporder[wantloop];` -- as the sole, unambiguous
  write target. This closes `loop`@`+0x38` at HIGH confidence.

The same block goes on to decrement `walking` by `TURNING_AROUND` and
take the remainder modulo `TURNING_BACKWARDS` (`0x2710`/10000) --
matching this project's own much earlier "modular-1000/10000
arithmetic" observation on `walking` exactly, finally explained in
full -- and copies `animspeed`@`+0x42` into `wait`@`+0x1C`
(`chi->walkwait = chi->animspeed;`), reconfirming four other already-
established fields in the same pass. `CharacterInfo` is now fully
confirmed field by field except `actx`/`acty`, a genuinely shelved dead
end rather than an open lead.

## Three more small wins: `ViewStruct272.numloops`, `MoveList.direct`, and a big `RoomStatus.obj[]` upgrade

With `CharacterInfo` closed, this round swept the file's remaining
MEDIUM-confidence fields once more.

**`ViewStruct272.numloops`@`+0x00` closed almost by accident.** The
`update_stuff` evidence used to confirm `CharacterInfo.loop` the
previous round already contained the answer: the "turning around"
branch reads `views[view]+0` (no added offset) to compare against
`turnlooporder[wantloop]`, matching source's `turnlooporder[wantloop]
>= views[chi->view].numLoops` -- `views[view]+0` IS `numloops` itself.
Upgraded to HIGH confidence by cross-referencing evidence already on
record rather than fresh investigation.

**`MoveList.direct`@`+0x1FD` got a real but incomplete answer.** 2011's
`move_object` sets `mls[mslot].direct=ignwal;` right after
`objs[objj].moving=mslot;`; this build's own `move_object` (already
matched, already fully read for other fields) ends immediately after
the `moving` assignment with NO further write. That's genuine negative
evidence for THIS call site, but 2011 has at least one other write site
(`NewRoom`'s own "nasty hack" edge case, `AC.CPP:20028`) and a presumed
third for `MoveCharacterDirect` (not yet matched in this build) that
weren't checked -- recorded as a real, useful data point without
claiming the field itself is confirmed absent.

**`RoomStatus.obj[10]`@`+0x08` got the biggest upgrade** -- and it was
hiding in plain sight. `RoomObject.transparent`'s own entry already
cited `load_new_room`'s first-time "beenhere==0" initialization loop for
ONE field write, but nobody had gone back and asked what ELSE that same
loop does. Reading it in full: it writes NINE separate `RoomObject`
fields per iteration -- `x`, `y`, `transparent`, `num`, `view`, `loop`,
`frame`, `wait`, `moving`, and a conditionally-overwritten `baseline` --
matching 2011's own `croom->obj[cc].FIELD=...` initialization block
(`Engine/AC.CPP:4282-4298`) field for field, same order, same defaults,
even reading from the already-confirmed `RoomStruct.sprs[]`/
`objbaseline[]` globals as its own data source. This is no longer an
arithmetic fit (total-size subtraction) -- it's direct, exhaustive,
per-field behavioral proof that `obj[]` genuinely starts at `+0x08`
within `RoomStatus`, upgraded from MEDIUM to HIGH accordingly. The
lesson repeats from a few rounds ago (`objyval[]`/`MAX_OBJ`): evidence
already sitting in the file, cited for one purpose, can answer a
completely different open question if someone goes back and reads the
whole source of that evidence rather than just the one line it was
originally quoted for.

## `RoomStatus.flagstates[]` closes the same way, right next door

Reading a little further past the `obj[]` initialization loop (the same
`load_new_room` code this round's earlier `obj[]` upgrade came from)
turned up the last open `RoomStatus` field for free. Immediately after
the object-init loop ends, the very next code is `for(chaa=0; chaa<0xF
(15); chaa++) [croom+chaa*2+0x148]=0` -- a direct, literal loop clearing
`RoomStatus.flagstates[15]`@`+0x148`, matching 2011's `for
(cc=0;cc<MAX_FLAGS;cc++) croom->flagstates[cc]=0;` (`Engine/AC.CPP:
4308`) exactly. This confirms both the exact POSITION and the exact
CAPACITY in one shot -- no longer an arithmetic remainder computed from
`obj[]`'s own end and `tsdatasize`'s own start, but a literal `0xF`
loop bound sitting right there in the disassembly.

The code immediately after THAT does three more `rep movsd` block
copies of 148-byte (`0x94`) `EventBlock` records, all from `RoomStruct`-
side source data into this build's already-confirmed `RoomStatus`
fields: one record into `misccond`@`+0x12C8`, a 20-element loop into
`hscond[20]`@`+0x170`, and a 10-element loop into `objcond[10]`@`+0xD00`
-- reconfirming all three via the exact same evidence pass, for free.

With this, `RoomStatus` has no remaining MEDIUM-confidence fields at
all -- every field from `beenhere` through `walkbehind_base` is now
either directly behaviorally confirmed or confirmed absent by positive
evidence, joining `RoomObject`/`MoveList`/`MouseCursor`/
`InventoryItemInfo` as one of this project's fully closed structs.

## `MoveList.direct` confirmed absent, and `RoomStruct.flagstates`'s hypothesis corrected

Two follow-up threads closed this round.

**`RoomStruct.flagstates[15]`'s standing hypothesis needed correcting.**
Its own comment had guessed it was "most plausibly copied into the
per-save-slot `RoomStatus.flagstates` on first visit," mirroring the
already-established `hscond`/`objcond`/`misccond` source-copy-to-
runtime-copy relationship. Now that `RoomStatus.flagstates`'s own
populating code IS known (last round's finding: `load_new_room`
unconditionally RESETS it to zero, `for(chaa=0;chaa<15;chaa++)
croom->flagstates[chaa]=0;`), that hypothesis doesn't hold -- there is
no copy to find, because no copy happens. A check of 2011's own
`Engine/` source reinforces this from a different angle:
`thisroom.flagstates`/`rstruc->flagstates` has ZERO usages anywhere in
the reference build either -- this field is genuinely dead weight even
in 2011, the same "declared but never read" status as `MouseCursor.name`/
`GameSetupStructBase.target_win`. The field stays at MEDIUM confidence
(position/arithmetic fit is still solid), just with the wrong
hypothesis retracted rather than silently dropped.

**`MoveList.direct`@`+0x1FD` closes as CONFIRMED ABSENT.** The previous
round found `move_object` doesn't set it, but flagged two more of
2011's three known write sites as unchecked. Both got checked this
round: `MoveCharacterDirect` turns out to be a thin wrapper -- it just
calls `walk_character(cc,xx,yy,ignwal=1,autoWalkAnims=0)`, meaning
"MoveCharacter" and "MoveCharacterDirect" are unified through the same
function in this build (mirroring `move_object`'s own `ignwal`-parameter
unification). Reading `walk_character`'s own body in full, plus its
post-`find_route` helper (`sub_40EB7B`, ~420 lines, called for
multi-stage routes), finds zero writes to `mls[mslot]+0x1FD` anywhere.
`NewRoom` (2011's third site, a "nasty hack" edge case gated on
`inside_script`) was also read in full: this build's own `inside_script`
branch is a genuine simpler predecessor that just stores `nrnum` into
the already-confirmed `ExecutingScript.newnum`@`+0x04` -- the entire
`mls[playerchar->walking].direct=1;`/`StopMoving` hack doesn't exist
here. With all three of 2011's own write sites checked and none present
in this build, `direct` is confirmed absent by the same "exhaustive
multi-site check" standard already used for `DCMD_*` opcodes and other
findings elsewhere in this project -- this build's `MoveObject`/
`MoveCharacter` unify the direct/non-direct distinction entirely through
the `ignwal` parameter passed to `find_route`, with no separate
persistent per-movelist flag needed.

## `GUIButton`'s long-standing "minimum size" caveat closes

`GUIButton` had carried an explicit caveat since the original vtable-
recovery round: its field list (`vtbl` through `rclickdata`, ending at
`+0x80`) was recovered entirely from `ReadFromFile`/`WriteToFile`'s
three bulk fread/fwrite calls, but the struct's own comment flagged this
as a MINIMUM size only -- 2011 declares more fields after `rclickdata`
(`textAlignment`, `reserved1`, `eventHandlers[]`) that get read/written
individually rather than through a bulk call, so their absence couldn't
be assumed just because the three known calls didn't reach them.

Reading both functions (`sub_406A9C`/`sub_406A4A`, already matched) in
full end to end, rather than just the fread/fwrite call sites already
used to recover the field list, resolves this cleanly. Both are tiny,
completely linear functions with a single early-exit branch (a
`textcol==0` default-fixup in `ReadFromFile`) and no other control flow:

```
GUIButton__ReadFromFile:
    fread(this+4,  7, 4, Stream)   ; 28 bytes -- GUIObject base fields
    fread(this+0x54, 12, 4, Stream) ; 48 bytes -- pic..rclickdata block
    fread(this+0x20, 50, 1, Stream) ; 50 bytes -- text[50]
    if (*(this+0x70) == 0) *(this+0x70) = 0x10;  ; textcol default
    return;
```

`WriteToFile` is an exact mirror -- same three offsets, same sizes,
`fwrite` instead of `fread`, no fixup branch, immediate return. Neither
function contains a fourth call, and neither ever computes or
dereferences any offset past `+0x80`. This is a stronger result than
"the three known calls don't reach further" (which was already true and
already reflected in the caveat) -- it's a *complete* account of
everything the load/save routines do, positively proving there is
nothing else to read or write. `sizeof(GUIButton) == 0x84` (132 bytes)
is now confirmed, not just a lower bound, and 2011's `textAlignment`/
`reserved1`/`eventHandlers[]` trailing fields are CONFIRMED ABSENT here
-- the same "later AGS addition, not yet present in this 2002 build"
pattern found repeatedly elsewhere in this project. A quick check of the
other five `GUIObject`-derived structs (`GUISlider`/`GUILabel`/
`GUITextBox`/`GUIListBox`/`GUIInv`) confirms none of them carry an
equivalent open-tail caveat -- `GUIButton` was the only one left with
unfinished business here, and it's now closed.

## `InventoryItemInfo.cursorPic` confirmed absent -- the source comment gave it away

`cursorPic`@`+0x20` had sat in an unusual limbo since the round that
closed `pic`/`hotx`/`hoty`: unlike every genuinely-absent field found
elsewhere in this project, it had explicitly NO negative evidence
either way, just an unconfirmed positional gap -- every cursor-picture
code path traced read `pic` directly, but nothing proved `cursorPic`
itself didn't exist unread somewhere in memory.

Re-reading `SetInvItemPic` (already matched) end to end this round
settled it, but the real smoking gun turned out to be sitting in the
2011 *reference source itself*, not the disassembly. 2011's own
`set_inv_item_pic` (`Engine/AC.CPP:5262-5278`) contains an explicit,
almost too-convenient comment right where the two fields would diverge:

```c
if (game.invinfo[invi].pic == game.invinfo[invi].cursorPic)
{
  // Backwards compatibility -- there didn't used to be a cursorPic,
  // so if they're the same update both.
  set_inv_item_cursorpic(invi, piccy);
}
game.invinfo[invi].pic = piccy;
```

2011's own source is documenting, in plain English, exactly the
scenario this project keeps finding evidence for elsewhere: a field
that "didn't used to" exist, kept in sync only for save-compatibility
with games built before it existed. This build's own `SetInvItemPic` is
even simpler than the backwards-compatibility branch the comment
describes -- it does ONE unconditional write (`dword_51B870[ecx]=
piccy`, i.e. `pic`@`+0x1C` alone), with no `pic==piccy` check and no
second-field sync logic at all. That's consistent with never having had
a second field to keep in sync in the first place, not just with the
sync branch being untraced. Reinforcing this further: an exhaustive
search finds no `set_inv_item_cursorpic`/`InventoryItem::
SetCursorGraphic`/`InventoryItem::GetCursorGraphic`-equivalent function
anywhere in the binary, and no `"SetCursorGraphic"`-family export
string either -- the entire API surface 2011 built around the
cursorPic/pic split doesn't exist yet. `cursorPic` is now confirmed
absent, with `pic` serving both roles in this build -- closing
`InventoryItemInfo`'s last open field.
