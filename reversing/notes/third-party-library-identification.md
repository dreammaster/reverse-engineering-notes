# Third-party library identification

Aim: name the statically-linked library code (Allegro, libcda, apeg, dumb,
aastr, almp3, hq2x under `Engine/libsrc/`) in `rob_blanc_1.asm`. Lower
priority than Engine/Common matches for the "reconstruct Rob Blanc 1" goal,
but worth doing for IDB completeness (see `CLAUDE.md`).

**Status: resumed, still not exhausted.** The original session got
`libcda-0.4` essentially done, made a solid dent in Allegro (window/
thread creation, mouse/keyboard/sound config reading, DirectDraw driver
init, assert/trace/exit-hook plumbing), and conclusively ruled `apeg`
out entirely (see its own section below). A follow-up round resolved
the previously-open "dumb-0.9.2 XM loader" lead completely -- it was
never DUMB at all, it's **JGMOD** (a library with no source tree in
this repo), and separately, **`dumb-0.9.2` itself is now conclusively
ruled out**, same as `apeg`, via a release-date check (see its own
section below). A THIRD round tackled the previously-untouched
`aastr-0.1.1`/`almp3-2.0.5`/`hq2x` group: `aastr` and `hq2x` are
genuine string-matching dead ends (see their own section), but
`almp3-2.0.5` yielded 5 solid new matches via the caller-shape
technique (`my_load_static_mp3`, `almp3_create_mp3`, plus 3 Allegro
`PACKFILE` functions). Roughly 50 new `matches.json` entries total
across all three rounds. To resume further, see the "Next up" section
at the bottom.

## Key finding: `cross_reference.py` already covers `Engine/libsrc/`

`SRC_DIRS = [ROOT/"Common", ROOT/"Engine"]` with `.rglob("*")` recurses into
`Engine/libsrc/` automatically -- no script changes were needed. `leads.json`
just has very few libsrc entries (22 out of 43) because most library
functions are **already named** via FLIRT signatures (`build_leads.py` only
tracks references from still-unnamed `sub_*` functions), not because the
strings aren't there. `string_matches.json` has 60 entries touching libsrc
paths, 31 of them single-file.

`refmap_symbols.json` (the modern local reference build's `acwin.map`) is
much less useful here than for Engine/Common: it only has ~6 *whole-library*
object entries (`alleg_s_crt:allegro.obj`, `almp3.obj`, `libcdaWin.obj`,
`hq2x3x.obj`, `dumbfile.obj`, `AASTR.obj`) rather than per-source-file
granularity, since the modern build links these as prebuilt/bulk objects.
Still useful for exact-symbol-name matches on functions already FLIRT-named
in the disassembly, via the same technique `build_matches.py` uses for
Engine/Common.

## libcda-0.4 (`Engine/libsrc/libcda-0.4/libcdaWin.C`, 199 lines) -- mostly done

Small, self-contained CD-audio wrapper file. All 31 single-file libsrc
leads pointing here got resolved in one pass by reading the whole file
(it's short) and matching each candidate's body/format-string against the
source's `cd_*` functions and the internal static helpers `command()` /
`lengthof()`. `command()` (vararg `mciSendString` wrapper) and the
`CDAudio` interface dispatch table were **already named/matched** before
this round (visible via existing `[reversing] confirmed match` comments
and `CODE XREF: CDAudio+XX` annotations) -- this round's job was naming the
individual `cd_*` wrapper functions that call into them.

**10 confirmed and committed to matches.json** (all high confidence, exact
string/field match): `cd_exit` (sub_4342A2), `lengthof` (sub_4342FD, static
helper), `cd_play_from` (sub_434361), `cd_current_track` (sub_434388),
`cd_pause` (sub_4343DA), `cd_resume` (sub_43440A), `cd_stop` (sub_434464),
`cd_get_tracks` (sub_434480), `cd_eject` (sub_434533), `cd_close`
(sub_43454F).

**6 more identified but NOT yet matched** -- their disassembly bodies are
an exact, unambiguous match to source, but IDA never auto-detected a
function boundary at their start address (no `proc near`/`endp` pair at
all, just raw instructions between two `; ---` separator lines with no
label). `apply_matches.py`'s rename mechanism needs an existing named
symbol to target, so these need a function boundary created in IDA first
(position cursor at the start of the block, press `P` -- IDA's own
analyzer reliably finds the `push ebp; mov ebp,esp` prologue backward from
any address inside the body, so it's safe to invoke from anywhere in the
block, not just the exact first byte). Precise start addresses were
deliberately NOT computed by hand-counting instruction bytes here (x86
immediate-operand encoding length is data-dependent and error-prone to
guess correctly without seeing real IDA bytes) -- use the `CODE XREF` hex
addresses cited below to navigate there instead.

| Body location | Expected name | Source | Evidence |
|---|---|---|---|
| Right after `sub_4342A2` (`cd_exit`) ends, before `sub_4342FD` (`lengthof`) starts (0x4342A2..0x4342FD) | `cd_play` | libcdaWin.C:79-84 | Calls `cd_stop`(sub_434464) then `lengthof`(sub_4342FD) with the arg, `sprintf` with matched string `aUS`/`"%u:%s"` into `byte_536E4C` (`end_pos`), then `command()` with matched string `aPlayCdaudioFro`/`"play cdaudio from %lu to %s"`. `lengthof`'s own `CODE XREF: .text:004342C0` cites the call site inside this block. |
| Right after `sub_4342FD` (`lengthof`) ends, before `sub_434361` (`cd_play_from`) starts (0x434318..0x434361ish) | `cd_play_range` | libcdaWin.C:87-92 | Same shape as `cd_play` but takes 2 args (start,end), uses matched string `aUS_0` and `aPlayCdaudioFro_0`. `lengthof`'s second `CODE XREF: .text:00434324` cites the call site inside this block. |
| Right after `sub_43440A` (`cd_resume`) ends, before `sub_434464` (`cd_stop`) starts | `cd_is_paused` | libcdaWin.C:139-142 | Trivial one-liner: `mov eax, paused; retn` -- exact match for `return paused;`. |
| Right after `sub_434480` (`cd_get_tracks`) ends | `cd_is_audio` | libcdaWin.C:168-173 | `command()` with matched string `aStatusCdaudioT`/`"status cdaudio type track %u"`, then `strcmp(ret,"audio")==0` using matched string `aAudio`. |
| Right after that (`cd_is_audio`) | `cd_get_volume` | libcdaWin.C:176-180 | `if(arg0) *arg0=0x80(128); if(arg1) *arg1=0x80(128);` -- exact match for `if(c0)*c0=128; if(c1)*c1=128;`. |
| Right after that (`cd_get_volume`) | `cd_set_volume` | libcdaWin.C:183-185 | Completely empty body (`push ebp; mov ebp,esp; pop ebp; retn`) -- exact match for the empty `void cd_set_volume(int,int){}`. |

Once these 6 have function boundaries (and therefore `sub_XXXXXXXX` auto
names) in a fresh `.asm` export, they can be matched by address the same
way as everything else.

**Update:** 5 of the 6 got function boundaries created and are now matched
(`cd_play_range`, `cd_is_paused`, `cd_is_audio`, `cd_get_volume`,
`cd_set_volume`) -- the boundary for `cd_play_range` landed at exactly
0x434318, confirming the earlier byte-counted estimate was right, for
whatever that's worth as a sanity check on the general technique. Only
**`cd_play`** (right after `cd_exit`, before `lengthof`) still has no
function boundary -- still open.

**Possible general script improvement for later**: `apply_matches.py`
could support a `create_func_at: "0x..."` field that calls
`ida_funcs.add_func(ea)` before renaming, for exactly this situation. Not
implemented yet -- deliberately deferred rather than risking an
auto-created function boundary being wrong without a human eyeballing it
first, consistent with this project's general caution about scripted
changes to the IDB.

## Allegro `win/wwnd.c` (776 lines) -- window creation/thread, done for its 2 clean leads

Both single-file `leads.json` candidates resolved, plus 2 bonus matches
that fell out of cross-referencing the same window-proc pointer across
both functions:

- `sub_43B890` = `init_directx_window` (wwnd.c:501) -- matched via a chain
  of `RegisterWindowMessageA` calls with matched strings ("Allegro call
  proc", "Allegro window suicide", etc.), then a `SetWindowLongA(...,
  GWL_WNDPROC, sub_43B9B0)` call gated on `user_wnd` being set.
- `sub_43BF20` = `create_directx_window` (wwnd.c:391, static) -- matched
  via the `WNDCLASSA` setup using the matched string `"allegro_icon"` and
  the `IDI_APPLICATION`/`IDC_ARROW` fallback constants. DRIFT: this build
  calls `CreateSolidBrush()` for `hbrBackground` where 2011's source has
  `wnd_class.hbrBackground = NULL;` (wwnd.c:409) -- an actual background
  brush is painted here, not left unset.
- `sub_43B9B0` = `directx_wnd_proc` (wwnd.c:211, static, bonus match) --
  identified purely by construction: it's the one function address
  installed as the window proc in BOTH of the above (source has exactly
  one function, `directx_wnd_proc`, used both ways).
- `StartAddress` (IDA's own auto-name for a `CreateThread`-callback-shaped
  function) = `wnd_thread_proc` (wwnd.c:450, static, bonus match) --
  confirmed via its `DATA XREF: sub_43B890+0xC1` (the `CreateThread` call
  site inside `init_directx_window`) and its body's exact if/else dispatch
  between `wnd_create_proc(directx_wnd_proc)` and `create_directx_window()`,
  matching source's thread-entry logic precisely.

One loose end not chased further: `sub_465C30` (called first inside
`wnd_thread_proc`, presumably `_win_thread_init()`) has no definition
anywhere under `Engine/libsrc/` -- it's only ever called, never defined,
in the scanned source tree, so it must live in a Windows-thread-support
file not included in this checkout (or was inlined/macro'd away in this
reference version). Left unmatched; low priority.

## Allegro `win/wmouse.c` -- one match found despite noisy lead

`sub_464A30`'s lead was noisy (its only single-file string was
`"mouse_accel_factor"`; `"mouse"` alone matched a dozen files) but reading
the body confirmed `mouse_directx_init` (wmouse.c:664) anyway: it pushes
both matched strings through an inline `uconvert_ascii`-shaped helper
(`sub_452700`) then calls a 3-arg `get_config_int`-shaped function
(`sub_460820`) with a default value, matching source's `get_config_int(
uconvert_ascii("mouse",tmp1), uconvert_ascii("mouse_accel_factor",tmp2),
MAF_DEFAULT)` exactly. `sub_452700`/`sub_460820` themselves weren't
independently confirmed as `uconvert_ascii`/`get_config_int` this round --
plausible bonus leads (likely called from MANY other library init
functions given how central they are, so worth chasing next: a
confirmed `get_config_int` would probably unlock several more matches by
association).

## `uconvert`/`get_config_int` confirmed, and where that trail runs out

Following up on the `mouse_directx_init` bonus leads: `sub_452700`
(`uconvert`) and `sub_460820` (`get_config_int`) each have exactly the
right parameter count (5 and 3 dwords respectively, matching
`uconvert(s,type1,buf,type2,size)` and `get_config_int(section,name,def)`),
and grepping for `call sub_460820` turned up **~24 consistent call sites**
across the disassembly, every one following the identical `push offset
<config-key-string>; call sub_452700 (uconvert); ...; call sub_460820
(get_config_int)` shape with real Allegro config key names (`flip_pan`,
`quality`, `sound_dma`, `sound_irq`, `sound_freq`, `sound_bits`,
`sound_stereo`, `digi_volume`, `midi_volume`, `digi_voices`,
`midi_voices`, `mouse`, `key_escape` x7, ...). Upgraded both matches to
high confidence on this corroboration.

This surfaced two clear clusters of config-reading init code:
- **`sub_443500`** (lines ~120298-120697 of the `.asm`): reads
  `flip_pan`/`quality`/`sound_dma`/`sound_irq`/`sound_freq`/`sound_bits`/
  `sound_stereo`/`digi_volume`/`midi_volume`/`digi_voices`/`midi_voices` --
  clearly Allegro's digital+MIDI sound driver config init.
- **`sub_470340`** (around line 203863): reads `key_escape` seven times in
  a row (presumably once per keyboard-layout/scancode-remap table entry)
  -- clearly keyboard driver init, and structurally matches the earlier
  `win/wkeybd.c` lead (`sub_470340` was already flagged there).

**Neither could be given a real name.** The actual Allegro source files
that would implement these (a generic `sound.c`/`digmid.c`-style driver
init, and whatever builds the `key_escape` config keys) are **not present**
in this repo's `Engine/libsrc/allegro-4.2.2-agspatch/` checkout -- only
the Windows-platform `win/*.c` patch files were kept, not the library's
generic/portable core. Grepping the whole repo for `flip_pan`/`sound_dma`/
`digi_voices`/`key_escape` only turns up doc comments in
`Common/libinclude/allegro/sound.h` and `internal/aintern.h`, not any
`.c` implementation to compare against. `key_escape` specifically doesn't
appear literally in `win/wkeybd.c` either -- it may be built dynamically
via string concatenation for per-layout variants in whatever file isn't
checked out here. Logged as a genuine source-availability dead end, not a
technique failure -- revisit only if a fuller Allegro 4.2.2 source tree
becomes available to add to the repo.

## Full Allegro 4.2.2 source added -- the `sound.c`/`config.c`/`unicode.c` gap is closed

The user downloaded the complete upstream Allegro 4.2.2 source tree to
`Engine/libsrc/allegro-4.2.2/` (separate from the pre-existing
`allegro-4.2.2-agspatch/`, which only ever had the Windows-specific
`win/*.c` patch files, not the generic library core). This directly fills
the gap noted above.

**Caution for future leads**: the two trees have overlapping filenames
(both have `win/wwnd.c`, `win/wmouse.c`, `win/wkeybd.c`, etc.) with
**different content** (`agspatch` is AGS's patched fork) -- confirmed via
`diff` that `wwnd.c` differs between the two. Re-running
`cross_reference.py` after adding the new tree means a string literal
present in both versions of the same file will now show 2 candidates
instead of 1, degrading some previously-clean single-file leads into
ambiguous ones. Net effect measured: `string_matches.json` grew from 944
to 1096 matched strings; `leads.json` grew from 43 to 54 `sub_*` leads
but single-candidate-file leads actually dropped slightly, from 22 to 17,
due to this overlap. Worth it anyway -- the genuinely new files
(`sound.c`, `config.c`, `unicode.c`, `misc/pckeys.c`, `allegro.c`, `midi.c`,
no `agspatch` counterpart at all) are pure upside and immediately unlocked
several matches. `Engine/libsrc/allegro-4.2.2/tools/plugins/datgrid.c`
leads are likely false positives -- that's a standalone datgrid build
tool, not code actually linked into the game; deprioritize those.

**6 more matches confirmed this round**, closing out several previously-
open leads:
- `need_uconvert` (sub_452560) -- internal helper called by the
  already-matched `uconvert`, confirmed via exact 3-arg signature
  (`unicode.c:596`).
- `read_sound_config` (sub_443500) -- exact 9-key `get_config_int`
  sequence match (`sound.c:168`); its own caller (`detect_digi_driver`,
  opens with `if (_sound_installed) return 0;`) is identified but still
  has no IDA function boundary, same pending-boundary situation as the
  libcda functions above.
- `read_keyboard_config` (sub_470340, `misc/pckeys.c:666`) -- **this
  resolves the earlier "`key_escape` string doesn't appear literally in
  `win/wkeybd.c`" dead end**: `key_escape` turned out to be a *section*
  name in `misc/pckeys.c` (used for reading accent-key config), not a
  scancode-remap key in `wkeybd.c` at all -- the file-scoping guess in the
  original investigation was simply wrong, not a missing-source problem.
- `get_config_string` (sub_460620, medium confidence -- call-site shape
  only, not body-traced) and `sys_directx_init` (sub_455490,
  `win/wsystem.c:127` -- resolves `init_directx_window`'s previously-open
  caller lead exactly: `"if (init_directx_window() != 0) goto Error;"`)
  and `get_config_text` (sub_461370, `config.c:1304`, matched via the
  unique `"[language]"` config-section string).

## `al_assert`/`al_trace`/`debug_exit` resolved (Allegro `allegro.c`)

Both `_add_exit_func`'s own `CODE XREF: sub_434970+A6` and
`CODE XREF: sub_434B20+94` comments (noted several rounds ago, never
followed up) resolved cleanly once `allegro.c` was available: `al_assert`
(`allegro.c:579`) and `al_trace` (`allegro.c:636`) both match via their
matched `getenv()`/`fopen()` string arguments (`ALLEGRO_ASSERT`,
`ALLEGRO_TRACE`, `allegro.log`) and both call `_add_exit_func(debug_exit)`
-- the same target function address (`sub_434AC0`) in both, giving a
bonus `debug_exit` (`allegro.c:556`) match the same way `directx_wnd_proc`
was found earlier (one function address used identically from two
call sites, source has exactly one candidate for both).

## RESOLVED: the "dumb-0.9.2 XM loader cluster" was JGMOD all along -- and dumb-0.9.2 isn't linked into this binary at all

The lead as previously framed was based on a false premise: both
`sub_47C360`/`sub_47C4C0` reference the matched string `"Extended
Module: "`, which was assumed to be DUMB's XM-file magic-string check
(it's the standard XM format magic, so the string alone doesn't
distinguish which library is checking for it). Reading the FULL
cascade this round, plus the surrounding data, conclusively identifies
the whole thing as **JGMOD**, not DUMB -- and separately, proves
dumb-0.9.2 cannot be linked into Rob Blanc 1 at all.

**`dumb-0.9.2` ruled out on a simple date check**, the same way `apeg`
was: `Engine/libsrc/dumb-0.9.2/RELEASE.TXT` states `"DUMB v0.9.2,
released 2 April 2003"` -- seven months AFTER Rob Blanc 1's binary link
date (2002-07-21, per `CLAUDE.md`). It is chronologically impossible
for this build to contain dumb-0.9.2 code. Stop looking for dumb-0.9.2
matches in this binary; treat `Engine/libsrc/dumb-0.9.2/` the same as
`apeg-1.2.1` -- a feature added to AGS well after this game shipped.

**The real identity: JGMOD**, a MOD/XM/S3M tracker-music library. Not
present anywhere in this repo's `Engine/libsrc/` checkout (no source to
verify exact internal function names against), but its identity is
beyond doubt from the disassembly's own strings alone -- distinctive,
full-sentence, library-specific error messages appear throughout the
whole call tree: `"JGMOD : Not enough memory to setup initialization
sample"`, `"JGMOD : Unable to allocate enough voices"`, `"Can't play a
JGMOD pointer with null value"`, `"JGMOD pointer passed in is a NULL
value"`, `"JGMOD 01 module : "`, and FIVE separate compiled copies of
`"Unable to allocate enough memory for JGMOD structure"` (one per
format-specific loader function, a common pattern when each loader
duplicates its own error-handling boilerplate).

This also lines up perfectly with the 2011 reference source's own
build configuration: `Engine/acsound.cpp:1025` has `#ifdef
JGMOD_MOD_PLAYER` as the ACTIVE branch (with `#include "jgmod.h"`) --
2011's `my_load_mod` calls `load_mod()` and 2011's `MYMOD::play()`
calls `play_mod()`, both genuine JGMOD public API functions. The
alternate `#ifdef DUMB_MOD_PLAYER` branch a few lines later
(`acsound.cpp:1139`) is present in source but NOT the compiled-in
choice, in either the 2011 reference build OR (now confirmed) Rob
Blanc 1's much earlier one.

**Two high-confidence matches added**:
- **`sub_477320` = `load_mod`** (JGMOD's own public API,
  `Engine/acsound.cpp:1116`'s `"JGMOD *modPtr = load_mod((char *)
  filname);"`). Confirmed via caller pattern: `PlayMusic` (already
  matched) builds candidate filenames (`"music%d.mod"`,
  `"music%d.xm"`, `"music%d.s3m"`) and calls this function on each in
  turn, checking for a non-NULL result -- exactly matching
  `my_load_mod`'s "call `load_mod`, check NULL" shape, just with the
  multi-extension-guessing loop living directly in this build's
  `PlayMusic` rather than a level up. Internally does an 8-format
  check-then-load cascade (not 5, as the original framing guessed --
  MOD/XM/S3M/and at least 5 more format pairs), matching JGMOD's own
  multi-format auto-detection concept exactly. The XM-specific pair
  within it is `sub_47C4C0`(check, reads 17 bytes at a fixed offset
  and compares to `"Extended Module: "`) / `sub_47C520`(load, mallocs
  a `0x558`-byte JGMOD structure) -- both use the SAME low-level file
  helpers (`sub_47D670`=open, `sub_47D700`=read-N-bytes,
  `sub_47D720`=read-1-byte) as every other check/load pair in the
  cascade, confirming they're all part of the same library rather than
  two different ones sharing a magic string by coincidence.
  `sub_47C360` (a DIFFERENT function, also checking for `"Extended
  Module: "` but via a byte-by-byte SLIDING-WINDOW scan rather than a
  fixed-offset read, and checking an unidentified 4-byte binary magic
  first) is a SEPARATE format-check pair, not a rival library's
  competing XM detector as originally guessed -- it's simply another
  JGMOD-internal probe, for a format not yet identified (the leading
  4-byte magic, `0xC1 0x83 0x2A 0x9E`, isn't recognizable ASCII and
  wasn't decoded further this round).
- **`sub_477470` = `play_mod`** (`Engine/acsound.cpp:1103`'s
  `"play_mod(tune, repeat);"`), confirmed via the distinctive `"Can't
  play a JGMOD pointer with null value"` string guarding a NULL check
  on its first argument, called from `PlayMusic` immediately after a
  successful `load_mod`.

The internal cascade's other functions (`sub_47D9E0`, `sub_47D4D0`/
`sub_47D5D0`, `sub_47B4E0`/`sub_47B710`, `sub_47AD30`/`sub_47ADB0`,
`sub_47D400`, `sub_47C360`, `sub_47B410`, `sub_47B2B0`, plus the
low-level I/O helpers `sub_47D670`/`sub_47D700`/`sub_47D720`/
`sub_47B360`) are NOT individually renamed -- JGMOD's own source isn't
in this repo to verify exact names against, and guessing plausible
JGMOD API names without a source to check would violate this project's
"verify, don't invent" discipline. If JGMOD source becomes available
(it's open source, historically hosted alongside other Allegro-era
tracker libraries), revisiting this cluster would likely unlock several
more clean matches the same way the Allegro tree did earlier.

**A third public API match, found later via a CORRECTION rather than a
fresh lead: `install_mod(int numVoices)`.** Previously misnamed
`init_mod_player` after a mechanical match against `Engine/
acsound.cpp`'s own trivial 3-line wrapper of that name (`return
install_mod(numVoices);`) -- but this build's actual ~150-line body
(numVoices>64 clamp, malloc'd init-sample struct, per-voice
`voice_set_volume`/`voice_start` setup with `deallocate_voice` cleanup
on failure, three JGMOD-flavored error strings including `"JGMOD : Not
enough memory to setup initialization sample"`) is unmistakably JGMOD's
own `install_mod` doing its real voice-allocation work, called directly
from `main` with no separate AGS-side wrapper existing yet in this 2002
build. Its own two callees (`sub_477CB0`, `sub_47B360` -- the latter
already flagged above as a shared JGMOD-internal I/O/allocation helper)
stay unchased per the scope rule. See
`reversing/notes/struct-layout-drift.md`'s own writeup for the full
correction detail.

**A fourth, this one `install_mod`'s exact mirror image: `remove_mod`
(void)`.** Found immediately after, via a callgraph-ranking re-run --
`quit()` (already matched) calls it directly, and it's ALSO registered
as this build's own `atexit` callback inside `install_mod` itself.
Confirmed via clean symmetry with the just-corrected `install_mod`:
calls the already-matched `stop_mod()` first (matching `acsound.cpp:
1055`'s own `MYMOD::destroy()` sequence), deallocates every entry of
the exact same per-voice array `install_mod` populated, and clears the
exact same `dword_5477F4` "installed" sentinel `install_mod` checks at
its own entry. Same pattern as `install_mod`: 2011's own `remove_mod_
player() { remove_mod(); }` (`acsound.cpp:1132-1134`) is a later-added
thin wrapper this 2002 build predates entirely -- no separate AGS-side
function exists, `quit()` calls JGMOD's own uninstall routine directly.

## `apeg-1.2.1` is NOT LINKED into Rob Blanc 1 at all -- conclusively ruled out

Went looking for `apeg` (the MPEG video/audio decoder) specifically and
found nothing -- which turned out to be the correct, conclusive answer,
not a dead end.

The two leads that originally looked like `apeg-1.2.1/display.c`
candidates (`sub_46E820`, `sub_46EF10`, string evidence "Unsupported
virtual resolution"/"Overlays not supported"/etc.) turned out, once the
full Allegro tree resolved the ambiguity, to actually be Allegro's
Windows DirectDraw graphics driver (`init_directx_ovl` in
`win/wddovl.c`, `init_directx_win` in `win/wddwin.c`) -- both now
matched (see above). The `apeg-1.2.1/display.c` attribution was a false
positive caused by coincidental terminology overlap ("resolution",
"overlay") with Allegro's own DirectDraw messages, only resolvable once
a large enough reference tree existed to disambiguate.

Direct verification: grepping the entire 2727-string `strings.json`
dataset for `"mpeg"` or `"apeg"` (case-insensitive) returns **zero**
hits. None of `apeg`'s ~15 distinctive `apeg_error_jump(...)` error
strings ("Could not open stream", "No video in sequence", "Illegal frame
rate in stream", "Transport streams not supported", etc.) appear
anywhere in the disassembly either.

What Rob Blanc 1 actually uses for video is **DirectShow**, via
`Engine/acwavi.cpp` (already in the main `Engine/` reference tree, not
`libsrc/` -- and already matched, well before this session:
`dxmedia_abort_video`, `RenderFileToMMStream`, and the script-facing
`PlayVideo`/`PlayFlic` functions are all confirmed). The disassembly's
`"Video playing error: %s"` string sits right next to `PlayVideo`'s own
DATA XREF, and `dxmedia_abort_video`'s existing match evidence explicitly
cites DirectShow cleanup. `apeg` was evidently added to AGS as a video
backend at some point **after** 2002 and **before** the 2011 reference
build (which does link it, per `Engine/libsrc/apeg-1.2.1/` existing in
this repo) -- Rob Blanc 1 simply predates that switch.

**Conclusion: stop looking for `apeg` matches in this binary. There are
none to find.** `Engine/libsrc/apeg-1.2.1/` should be treated the same as
a feature the 2011 reference source has that the 2002 binary doesn't --
like the earlier `IsOverControl` vtable-slot drift, just at library scope
instead of a single method.

## `aastr-0.1.1` and `hq2x`: genuine string-matching dead ends

Picked up the last untouched group. `aastr-0.1.1` (anti-aliased
stretch/rotate) and `hq2x` (pixel-magnification filter) both turn out
to have **zero quoted string literals anywhere in their source** --
pure numeric/algorithmic code, no error messages, no debug output, no
assertions. This isn't a search failure; it's *why* `leads.json` never
had single-file candidates for either (already noted in `CLAUDE.md`),
and it rules out this project's most productive technique
(string-matching) categorically for both libraries. No FLIRT-assigned
names exist for their public API either (`aa_stretch`/`aa_rotate`
prefixes turn up nothing in the disassembly).

For `aastr` specifically, there's also a plausible reason its one real
usage site might not even be *reachable*: 2011's only caller
(`scale_and_flip_sprite`, `Engine/AC.CPP:7810`, calling
`aa_stretch_sprite` for antialiased sprite zoom-scaling) is gated by
`IS_ANTIALIAS_SPRITES`, itself gated by an INI setting read via
`INIreadint("misc","antialias",0)` -- and the literal string
`"antialias"` does not appear ANYWHERE in this binary. Separately, two
already-recovered structs that a "sprite zoom" feature would need
fields on (`RoomObject`, fully mapped this session, and `CharacterInfo`,
mapped in an earlier session) show no `zoom`-type field at all --
`RoomObject`'s tint/zoom/last-width/last-height region is explicitly
CONFIRMED ABSENT (see its own struct-layout-drift.md section).
Circumstantial, not as airtight as the date-based `apeg`/`dumb-0.9.2`
rulings (aastr's copyright is 1998-99, so it COULD have been available
to link in 2002 -- this isn't a chronological impossibility, just an
absence of any positive evidence of use), but consistent enough to
treat as low priority: this build most likely doesn't have sprite-zoom
antialiasing wired up at all, whether or not the library itself is
statically linked for some other/future purpose.

`hq2x` has no equivalent circumstantial angle explored this round --
just the flat "no strings, no FLIRT names" wall. Both are logged as
genuine technique dead ends; revisit only with a structural/callgraph
approach (tracing 2011's actual call sites through to already-matched
Engine functions, the same way `almp3` below was cracked) if this
becomes a priority again.

## `almp3-2.0.5`: 5 new matches via the caller-shape technique

Unlike `aastr`/`hq2x`, `almp3` (MP3 playback) has essentially no useful
strings either (checked and confirmed empty), but its usage is
substantial and well-documented in `Engine/acsound.cpp`, and Rob Blanc
1's own `PlayMusic` (already matched) DOES try MP3 first -- the string
`"music%d.mp3"` sits right in its preamble, gated by an already-named
flag (`opts_mp3_player`). This gave a solid caller-side thread to pull.

**`sub_4083FC` = `my_load_static_mp3`** (`Engine/acsound.cpp:~415-477`,
the buffered/non-streaming MP3 loader -- distinct from the nearby
streaming `my_load_mp3`). Called directly from `PlayMusic` right after
the `"music%d.mp3"` filename is built. Its body matches source's
sequence exactly: open the file `"rb"`, read a size field off the
returned handle, `malloc` that size, read the whole file in and close
the handle, then hand the buffer to `almp3_create_mp3`.

**`sub_47E3F0` = `almp3_create_mp3`** (referenced at
`acsound.cpp:465`), confirmed via an exact 2-argument match
(`buffer, size`) called immediately after the file read completes,
result checked for `NULL` exactly as source does. ALMP3's own source
isn't in this repo's `Engine/libsrc/almp3-2.0.5/` checkout at a level
that defines the public API bodies traceably, so this is a
reference-citation match (like `run_event_block`'s dead-declaration
match earlier), not a body-diff -- but the call shape leaves little
room for doubt.

**Three supporting Allegro `PACKFILE` matches** fell out of the same
investigation, at MEDIUM confidence: **`sub_408E49` = `pack_fopen`**
(2-arg, `filename`+`"rb"` mode string, exact match), **`sub_42F8FC` =
`pack_fread`** (3-arg, matching argument ORDER verified via cdecl push
sequence: buffer, size, file-handle), **`sub_42EE9A` = `pack_fclose`**
(1-arg). The confidence is MEDIUM rather than HIGH because the size
field this build reads off the `PACKFILE` handle sits at `+0x10`, which
doesn't match this repo's Allegro 4.2.2 reference declaration of
`_al_normal_packfile_details.todo` (computed at `+0x1C` from the outer
`PACKFILE` struct, `Common/libinclude/allegro/file.h:101`) -- plausible
`PACKFILE`-layout drift between whatever Allegro version this 2002
build actually used and the 4.2.2 reference tree (not independently
resolved this round), consistent with this project's repeated finding
that internal struct layouts drift even when function identities and
call shapes match cleanly. The function IDENTITIES themselves aren't in
doubt -- the argument counts, ORDER, and surrounding call sequence all
line up exactly with `pack_fopen`/`pack_fread`/`pack_fclose`'s
documented signatures.

**One loose end**: `sub_47E7A0`, called right after with a 6-argument
shape (`tune, 0x4000, dword_4B42A0, 0x80, 0x3E8, repeat`) that doesn't
cleanly match any single documented ALMP3 API function's signature
(`almp3_play_mp3(tune,bufsize,vol,pan)` is only 4 args; the literal
`0x4000`=16384 does match `almp3_play_mp3`'s bufsize argument at
`acsound.cpp:404`, but the extra `0x3E8`(1000)/`0x80`(128) arguments
suggest this might be a combined AGS-side wrapper doing BOTH `play_mp3`
and an `adjust_mp3`-style volume/pan/speed set in one call, or a
different ALMP3 function entirely). Not matched this round -- logged
as a promising lead for a future pass.

## JGMOD's cascade characterized: 4 more tracker formats identified by magic string

Went back to `load_mod` (`sub_477320`, already matched)'s 8-branch
cascade to read the remaining check functions individually, rather than
leaving them all as an undifferentiated "8-format check-then-load"
block. Four resolve cleanly via their own magic-string checks (none
renamed -- still no JGMOD source tree in this repo to verify exact
function names against -- but each is now documented with its specific
format role):

- **`sub_47D9E0`** (format 1): reads 18 bytes, compares against the
  matched string `"JGMOD 01 module : "` -- JGMOD's own
  native/proprietary module format, not a generic tracker format.
- **`sub_47D4D0`** (format 2): reads 4 bytes, compares against `"IMPM"`
  -- the standard Impulse Tracker (`.it`) magic signature.
- **`sub_47B4E0`** (format 3): seeks to offset `0x2C`(44), reads 4
  bytes, compares against `"SCRM"` -- the standard Scream Tracker 3
  (`.s3m`) magic signature, at its standard file offset.
- **`sub_47AD30`** (format 4): seeks to offset `0x438`(1080), reads 4
  bytes, loops through a table of classic ProTracker-family MOD magic
  tags starting with `"M.K."`, stepping 6 bytes per table entry -- the
  standard MOD magic at its standard offset, checked against several
  known tag variants (a well-documented convention among MOD loaders,
  since ProTracker itself and various trackers/converters all wrote
  slightly different 4-byte tags at that position).

Combined with the two already-documented formats (`sub_47C4C0`=XM via
`"Extended Module: "`, `sub_47C520`=its loader), this cascade is now
known to detect (in order): JGMOD-native, IT, S3M, MOD, XM, plus two
more unidentified pairs (`sub_47B410`/`sub_47B2B0`, not examined this
round) and a `dword_5477F8`-gated final fallback pair. **JGMOD is
confirmed as a genuine multi-format tracker-music library**, not a
single-format loader -- consistent with real JGMOD's documented feature
set (it supports MOD/S3M/XM/IT plus its own native format).

**A shared unresolved sub-thread**: `sub_47C360` (an XM-adjacent check,
doing a byte-by-byte sliding-window scan for `"Extended Module: "`
rather than `sub_47C4C0`'s simple fixed-offset read) and `sub_47D400`
(an IT-adjacent check, same sliding-window technique for `"IMPM"`) both
check an IDENTICAL, non-ASCII 4-byte constant (`0xC1 0x83 0x2A 0x9E`)
FIRST, before falling back to their respective text scans. The exact
purpose of that shared constant wasn't resolved this round, but its
presence in TWO otherwise-unrelated format checks strongly suggests a
shared JGMOD-internal helper pattern (plausibly a generic "packed/
wrapped module" pre-check) rather than coincidence. `sub_47D400` also
returns a position/index rather than a plain boolean (used by the
caller as an argument to its load function), hinting it may be probing
for an embedded/archived module rather than doing a simple top-of-file
signature check -- not investigated further.

## Next up

- **dumb-0.9.2**: CLOSED, do not revisit -- conclusively not linked
  into this binary (released April 2003, seven months after this
  binary's 2002-07-21 link date). `CORE/REGISTER.C`'s lead (previously
  flagged as a "separate lead from the XM cluster") should be treated
  the same way -- it can only ever be a false positive now.
- **JGMOD**: identity established, and 6 of 8 cascade branches now have
  a documented format role via magic-string checks (JGMOD-native, IT,
  S3M, MOD, XM x2 -- see the cascade-characterization section above),
  though none renamed since no JGMOD source tree exists in this repo to
  verify exact function names against. Remaining open items: the last
  2 format pairs (`sub_47B410`/`sub_47B2B0`) and the `dword_5477F8`-
  gated fallback pair haven't been read yet; the shared unresolved
  `0xC1832A9E` constant checked by `sub_47C360`/`sub_47D400` before
  their text scans; `sub_47D400`'s odd "returns an index, not a
  boolean" behavior. If JGMOD source becomes available, revisit the
  whole `sub_477320`/`sub_477CE0` cluster (and their many internal
  helpers, including `sub_47D670`/`sub_47D700`/`sub_47D720`/
  `sub_47B360`, the shared low-level I/O primitives) for real function
  names -- likely a productive round, similar to what adding the full
  Allegro tree unlocked.
- **almp3-2.0.5**: `sub_47E7A0` (called right after `almp3_create_mp3`
  with a 6-argument shape not matching any single known ALMP3 API
  function cleanly) is the one open lead -- see its own writeup above.
  Otherwise essentially done for what's reachable from `PlayMusic`;
  the streaming `my_load_mp3`/`ALMP3_MP3STREAM` path (`acsound.cpp`
  lines ~179-330) hasn't been checked against the disassembly at all
  yet, a plausible next avenue if this round's caller-shape technique
  is revisited.
- **aastr-0.1.1, hq2x**: CLOSED for the string-matching technique --
  genuinely zero string literals in either library's source, so
  `leads.json` will never populate for them via that method. Only a
  structural/callgraph approach could make progress here, and for
  `aastr` specifically there's decent circumstantial evidence (see its
  own section above) that its actual sprite-zoom-antialiasing use case
  may not even be present in this build at all. Low priority.

## Real source added for Allegro/JGMOD/ALMP3 -- the three biggest gaps closed

Per the user's own request, three concrete source gaps flagged
throughout this file were closed by downloading real reference
material (all official releases, from their original hosting):

- **Allegro 4.0.2** (`Engine/libsrc/allegro-4.0.2/`, SourceForge,
  released 2002-07-03 -- 18 days before Rob Blanc 1's own 2002-07-21
  link date, vs. the existing 4.2.2 reference's ~2005 release).
- **JGMOD's full `src/` tree** (`Engine/libsrc/jgmod/`,
  `ArtificialRaccoon/JGMOD` GitHub archive) -- this library had ZERO
  source in the repo before now, identified purely by its own
  distinctive error strings. Its own `history.txt` shows no recorded
  changes between April 2001 and October 2002, so this snapshot should
  be functionally equivalent to whatever Rob Blanc 1 actually linked
  in July 2002.
- **ALMP3 2.0.5's missing top-level `src/almp3.c`/`include/*.h`**
  (SourceForge `almp3` project) -- the repo already had the
  `decoder/` (mpg123-derived internals) subfolder, but never the
  actual public-API implementation these already-matched call sites
  need to verify against.

### A genuine vtable-version nuance found and honestly recorded, not papered over

Before trusting 4.0.2 as "the" Allegro version, cross-checked its own
`GFX_VTABLE` declaration (`include/allegro/gfx.h`) against this
project's own long-standing disassembly-observed vtable-shift finding
(draw_lit_sprite sits at disassembly offset `+0x5C`, one slot earlier
than the 4.2.2 reference's declared `+0x60`, previously explained as
"draw_trans_rgba_sprite doesn't exist yet in this build's older
Allegro"). Diffing 4.0.2's `gfx.h` against 4.2.2's found 4.0.2 is
missing an ENTIRELY DIFFERENT slot instead -- `set_blender_mode`
(between `restore_video_state` and `fetch_mode_list`) and, separately,
`fastline` (between `line` and `rectfill`) -- while `draw_trans_rgba_
sprite` itself is STILL PRESENT in 4.0.2, unlike the original
hypothesis assumed.

Working through the arithmetic precisely (anchored on the already-
disassembly-confirmed `draw_256_sprite`@`+0x48`, which requires
`fastline` to be PRESENT in the real linked build) shows 4.0.2's own
vtable is NOT a perfect match either -- it's missing `fastline`, which
the disassembly proves is present. Checked Allegro 4.1.1 (SourceForge
OldFiles, released 2002-08-25) too: same result, `fastline` still
absent there as well. **Neither available snapshot has a byte-perfect
vtable match** -- the real linked version is most likely an untitled
WIP/CVS snapshot from sometime in the March-August 2002 window that
happens to sit between what's publicly archived. This doesn't
invalidate 4.0.2 as the working reference (it's still by far the
closest available, correct for the vast majority of non-`GFX_VTABLE`
code, and the ORIGINAL vtable-shift hypothesis -- one slot missing
somewhere between `draw_256_sprite` and `draw_lit_sprite` -- still
holds up independently of which exact intermediate slot is responsible)
-- just recorded honestly as an unresolved precision limit rather than
claimed as settled.

**One direct payoff from the same investigation**: Allegro 4.0.2's own
`PACKFILE` struct declaration (`include/allegro/file.h`) matches this
binary's real, disassembly-observed field offsets EXACTLY --
`hndl`@0x00/`flags`@0x04/`buf_pos`@0x08/`buf_size`@0x0C/`todo`@0x10 --
independently resolving the long-standing MEDIUM-confidence caution on
`pack_fopen`/`pack_fread` ("the `todo` field this build reads at
`+0x10` doesn't match the 4.2.2 reference's declared `+0x1C` offset")
in favor of 4.0.2 for this specific struct, even though the vtable
itself isn't a perfect match.

### 10 new matches from the JGMOD/Allegro I/O cluster in one pass

Reading JGMOD's own `file_io.c` (now available) against the cluster of
previously-unnamed low-level I/O helpers this project's own earlier
rounds had already isolated (called from `load_mod`'s format-detection
cascade) closed the whole group in one sitting:

- **`jgmod_fopen`**(`sub_47D670`), **`jgmod_fread`**(`sub_47D700`),
  **`jgmod_fseek`**(`sub_47D6A0`), **`jgmod_skip`**(`sub_47D6E0`) --
  all exact, complete matches to `file_io.c`'s own thin `#ifdef
  JGMOD_PACKFILE` forwarding wrappers around Allegro's `pack_fopen`/
  `pack_fread`/`pack_fclose`/`pack_fseek` (all four of those already
  matched from earlier rounds). Confirms `JGMOD_PACKFILE` is defined
  in this build (AGS's music assets live inside the CLIB archive, so
  JGMOD reads through Allegro's packfile layer, not raw `fopen`).
- **`jgmod_calloc`**(`sub_47B360`) -- exact match to `load_mod.c`'s
  own `void *jgmod_calloc(int size) { return calloc(1, size); }`,
  JGMOD's universal allocator used throughout every format loader.
  Retroactively corrects `install_mod`'s own citation of a
  "`malloc`'d init-sample struct" -- it's `jgmod_calloc`'d.
- **`jgmod_igetw`**(`sub_47D800`)/**`jgmod_igetl`**(`sub_47D840`) --
  exact matches to `file_io.c`'s own little-endian 16/32-bit buffered
  reads, each built from repeated `jgmod_getc` calls. Both call
  `pack_getc` DIRECTLY (see next point) rather than through a separate
  `jgmod_getc` function -- confirming `jgmod_getc`'s own trivial
  `#ifdef JGMOD_PACKFILE return pack_getc(f);` body was small enough
  for the compiler to inline away entirely, leaving no standalone
  function to find.
- **CORRECTION: `pack_getc`(`sub_47D720`)** -- a much earlier round
  had informally described this function in prose only (never a real
  `matches.json` entry) as "read-1-byte", grouping it alongside the
  genuine JGMOD wrappers above as if it were their sibling
  `jgmod_getc`. It's actually Allegro's OWN `AL_INLINE pack_getc`
  (`file.inl:26-32`), compiled here as a real out-of-line function.
  Decisive, field-for-field match: `f->buf_size--; if (f->buf_size>0)
  return *(f->buf_pos++); else return _sort_out_getc(f);` matches the
  disassembly exactly, including the exact `PACKFILE` field offsets
  (see above). This is the direct source of the earlier "PACKFILE
  layout drift" caution's resolution.
- **`pack_fseek`**(`sub_42F43B`) and **`_sort_out_getc`**
  (`sub_42FC67`) -- two more Allegro internals identified via pointer-
  target + call-shape/role (both reached only through already-matched
  JGMOD/Allegro functions' own confirmed calls), MEDIUM confidence,
  own bodies not instruction-traced per the third-party scope rule.

**One promising lead investigated but correctly left open rather than
forced**: `sub_477CE0` shares the exact same format-detection cascade
shape as the already-matched `load_mod` (calling the same `detect_*`-
equivalent functions in the same order), but takes a SECOND argument
that gates the entire cascade behind a non-zero check -- not matching
`load_mod`'s own already-confirmed 1-argument body (which implements
the cascade unconditionally, with no such gate, and does NOT call
`sub_477CE0` at all). Plausibly a variant entry point (the archived
source's own `detect_unreal_*` functions return an offset rather than
a boolean, hinting at Unreal Tournament `.umx`-embedded-module
support, which would need exactly this kind of second argument) --
not conclusively identified this round, left as a genuine open lead
rather than guessed.

Renamed all 10, applied to the live IDB and re-exported (2586
functions, 958 named -- up from 535 at the start of this project).

### `sub_47E7A0` (the long-open ALMP3 lead) re-investigated -- one solid new match, the shape itself still doesn't fit

Went back to the standing `sub_47E7A0` open lead with ALMP3's own
newly-available `almp3.c` in hand. Its own 6-argument callee,
`sub_443050`, is a decisive match: `AUDIOSTREAM *play_audio_stream(int
len, int bits, int stereo, int freq, int vol, int pan)`
(`allegro-4.0.2/src/stream.c:23-27`) -- Allegro's public streamed-
audio API, matching the exact 6-argument shape, with the leading `len`
argument computed by dividing a caller-supplied byte count by a
stream-rate divisor read from the struct itself and adjusting for a
stereo flag, exactly matching `play_audio_stream`'s own documented
role. Renamed `sub_443050` -> `play_audio_stream`.

`sub_47E7A0` itself is now MUCH better understood -- it operates on an
`ALMP3_MP3STREAM`-shaped struct (huge `+0xA6xx` field offsets,
consistent with an embedded `mpg123` decoder state, genuinely tens of
KB), calls the newly-identified `play_audio_stream`, and conditionally
calls the already-matched `adjust_sample` when its own speed-like
argument isn't the literal `1000` (a percentage-based "100%=normal"
convention) -- but its own 6-argument shape still doesn't cleanly
match EITHER of ALMP3 2.0.5's own separately-declared
`almp3_play_mp3stream(mp3,buffer_len,vol,pan)` (4 args) or
`almp3_adjust_mp3stream(mp3,vol,pan,speed)` (4 args, no loop
parameter). Most plausible explanation: this 2002 build's own ALMP3
version fused what the archived (later) source keeps as two separate
functions into one -- the same "one big pre-refactor function, later
split" pattern already found repeatedly for AGS's own code in this
project, now observed in a third-party library too. Left unnamed
rather than force a name onto a shape neither archived function
actually has, but its own `matches.json` entry is now far richer than
the original "doesn't cleanly match" framing.

### The entire JGMOD format-detection cascade closes -- 14 more matches

`load_mod`'s own body (already matched, fully reading its own real
`Engine/acsound.cpp:1116`-equivalent call site) implements JGMOD's
whole multi-format cascade INLINE -- no separate top-level dispatcher
function exists in this build at all, meaning `load_mod` itself IS the
real `JGMOD *load_mod(char *filename)` from `mod.c:159-214`, not a
thin wrapper around one. Its own already-recorded call sequence
(8 `detect_*`/`load_*` pairs plus the leading JGMOD-native check) turns
out to match source's own call ORDER exactly, branch for branch, with
zero reordering -- letting every remaining cascade function be
identified purely by position, several without even reading their own
bodies:

- **`detect_jgm`**(`sub_47D9E0`), **`detect_it`**(`sub_47D4D0`),
  **`detect_xm`**(`sub_47C4C0`), **`detect_s3m`**(`sub_47B4E0`),
  **`detect_m31`**(`sub_47AD30`), **`detect_unreal_it`**
  (`sub_47D400`), **`detect_unreal_xm`**(`sub_47C360`) -- all already
  had thin matches.json entries from earlier string-matching rounds;
  this round confirms each against its own real source body (opens
  the file, reads N bytes, `memcmp`s against the exact already-known
  magic string/offset) and formally renames them.
- **`load_it`**(`sub_47D5D0`), **`load_xm`**(`sub_47C520`),
  **`load_s3m`**(`sub_47B710`), **`load_m`**(`sub_47ADB0`),
  **`load_jgm`**(`sub_47DA80`) -- the five format-specific loaders,
  each a brand-new match (no prior entry existed), identified purely
  via call-order position matching source's own `load_mod()` body
  (`load_m`'s own 2-argument `(filename, no_inst)` signature explains
  why it's called TWICE, once for the 31-instrument MOD variant and
  once for the 15-instrument one).
- **`detect_unreal_s3m`**(`sub_47B410`), **`detect_m15`**
  (`sub_47B2B0`) -- the last two functions in the whole cascade this
  project's own notes had explicitly flagged as "not examined this
  round" several sessions ago ("the last 2 format pairs... haven't
  been read yet"). Closed the same way, by call-order position.

With this, **every single function in JGMOD's own format-detection
cascade is now named** -- a whole library subsystem fully closed out,
not just individually characterized fragments. The shared unresolved
`0xC1832A9E` 4-byte constant `detect_unreal_it`/`detect_unreal_xm`
both check before their own text scans remains genuinely undecoded
(not addressed by having real function names now) -- a small, low-
priority loose end for a future round if it ever matters.

Renamed all 14, applied to the live IDB and re-exported (2586
functions, 973 named).

### `almp3_create_mp3`'s own remaining callees close, plus a bonus: all four `SOUNDCLIP`-family AGS-side constructors

Went back to `almp3_create_mp3`'s (already matched) own body with
`almp3.c`/`decoder/interface.c` in hand, since it has several still-
unnamed callees.

- **`almp3_get_big_endian`**(`sub_47E730`) -- a DECISIVE, complete
  match. `almp3_create_mp3` parses an mp3's Xing VBR header (`memcmp`
  against `"Xing"`, already visible in the disassembly but not
  previously connected to source): one unconditional call for
  `xing_header->flags`, then up to three more gated on bits 1/2/8 of
  that value (`XING_FRAMES_FLAG`/`XING_BYTES_FLAG`/
  `XING_VBR_SCALE_FLAG`), while bit 4 (`XING_TOC_FLAG`) is a raw
  100-byte copy loop with NO function call at all -- matching
  source's own `for(j=0;j<100;j++,p++) mp3->xing_header->toc[j]=*p;`
  exactly (single bytes, not big-endian dwords, so no call needed).
  Four real call sites in the disassembly, matching source's own four
  (1 unconditional + 3 conditional) exactly.
- **`ExitMP3`**(`sub_4818A0`) and **`decodeMP3`**(`sub_4818C0`) -- the
  mpg123 decoder's own two public entry points `almp3_create_mp3`
  calls directly (1-arg cleanup, 6-arg frame-decode-probe
  respectively), identified via call-shape and role at MEDIUM
  confidence -- THIRD-PARTY LIBRARY BOUNDARY per this project's own
  scope rule, their own internal decode logic not chased further.
- **Bonus, found while confirming `ExitMP3`'s caller context: all
  four `SOUNDCLIP`-family AGS-side constructors** (`Engine/
  acsound.cpp`, already partially documented via struct-recovery work
  many sessions ago, but never actually matched as functions).
  `SOUNDCLIP::SOUNDCLIP`(`sub_424B10`) is this build's own base-class
  constructor -- confirmed via TRIPLE cross-confirmation, called as
  the first step of three independent derived constructors, each
  setting its own vtable pointer immediately after it returns:
  `MYWAVE::MYWAVE`(`sub_424A60`, from `my_load_wave`),
  `MYMP3::MYMP3`(`sub_424B30`, from `my_load_mp3`), and
  `MYSTATICMP3::MYSTATICMP3`(`sub_424CC0`, from `my_load_static_mp3`
  -- resolving that function's own long-standing "operator new(0x18)
  -> a constructor call (sub_424CC0)" forward reference from several
  sessions ago). All three derived constructors are genuinely empty
  bodies in source (`ClassName() : SOUNDCLIP() {}`), matching the
  disassembly's own "call base ctor, patch vtable, return" shape
  exactly. Flat-named as C++ member functions per this project's
  established convention.

Applied all 7, re-exported (2586 functions, 980 named -- up from 535
at the start of this project).

### `sub_47F160`/`sub_47F130`: the MP3STREAM-flavored twin of `sub_47E7A0`, plus `almp3_is_playing_mp3stream`

`my_load_mp3`'s own remaining callee, `sub_47F130`, turns out to be a
thin wrapper around `sub_47F160` -- and `sub_47F160` is structurally
IDENTICAL to `sub_47E7A0` (this project's own earlier ALMP3 lead,
characterized but left unnamed a few rounds ago): both compute a
sample count from a caller-supplied byte count, call the already-
matched `play_audio_stream`, then conditionally call the already-
matched `adjust_sample` when a trailing argument isn't the literal
`1000` ("100%=normal speed"). `sub_47E7A0` operates on `ALMP3_MP3`
fields; `sub_47F160` operates on the `ALMP3_MP3STREAM`-shaped
equivalent (the same huge `+0xA6xx` offsets), matching ALMP3's own
static-vs-streaming API split.

One piece closes decisively: `sub_47F160`'s own opening guard,
`sub_47F690`, is `int almp3_is_playing_mp3stream(ALMP3_MP3STREAM
*mp3) { if (mp3->audiostream==NULL) return FALSE; else return TRUE; }`
(`almp3.c:1901-1906`) -- its entire body is the classic `(x!=0)?-1:0`
idiom testing exactly the field `sub_47F160` itself writes with
`play_audio_stream`'s own return value, confirming both the field
identity and the function match at once. Renamed.

`sub_47F160`/`sub_47F130` themselves stay UNNAMED for the same reason
as `sub_47E7A0`: their fused 5-argument play+adjust shape doesn't
cleanly match either of ALMP3 2.0.5's own separately-declared
4-argument `almp3_play_mp3stream`/`almp3_adjust_mp3stream` -- this
2002 build most likely fuses what the archived source keeps as two
functions into one, for BOTH the static and streaming MP3 cases
identically. `sub_47F130` itself is just a convenience wrapper
defaulting the speed argument to `1000`, matching `my_load_mp3`'s own
lack of any reason to request non-default playback speed at load
time. Both are now richly characterized in `matches.json` even though
left unnamed, matching this project's established "record what's
known, don't force a name" convention.
