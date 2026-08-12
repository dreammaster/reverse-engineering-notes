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
now accounted for.** Remaining unrecovered content is entirely inside
the trailing `_pad_unknown7` gap (`+0xA7F4`..`+0xBF84`, 6032 bytes):
whatever fixed-size arrays the further-derived `GameSetupStruct`
embeds directly (`invinfo[100]`, `spriteflags`, `acroom.h:2890-2917`)
that this build predates `OriGameSetupStruct`'s own layout for -- a
genuinely different, harder kind of lead than anything resolved so far,
since there's no `OriGameSetupStruct` declaration left to anchor
against.

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
