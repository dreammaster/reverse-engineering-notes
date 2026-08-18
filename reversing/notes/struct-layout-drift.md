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
the end (as already matched) -- it does EVERYTHING 2011 splits out into a
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
   (a standalone global, address `0x500204`, previously confirmed via
   `TintScreen`) as `play.screen_tint`.
2. A `StopAmbientSound` call and a `free`/`malloc`/`memcpy`/`ccFreeInstance`
   sequence match source's ambient-sound-stop loop and
   `save_room_data_segment()`+`ccFreeInstance(roominstFork)`+
   `ccFreeInstance(roominst)` room-script cleanup respectively. (An
   immediate follow-up round read the helper called right before this
   sequence, `sub_409A9C` -- guessed here as a "strong candidate" for
   `save_room_data_segment` -- and found that guess WRONG: it's actually
   `cancel_all_scripts`. See the next section.)
3. Three zero-writes right before the final `destroy_bitmap`/
   `stop_fast_forwarding` calls match source's `play.bg_frame=0;
   play.bg_frame_locked=0; play.offsets_locked=0;` (`AC.CPP:3624-3626`)
   exactly in sequence and role.

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
alongside the two additional standalone globals (`screen_tint`,
`offsets_locked`) found this round.

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
- By contrast, `play_scren_tint`/`screen_tint` (`TintScreen`'s evidence)
  computes to `+0x10C6C` -- genuinely FAR outside GameState's bounds. That
  one really is a standalone global; the correction doesn't apply to it.

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
