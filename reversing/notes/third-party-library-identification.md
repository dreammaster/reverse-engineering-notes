# Third-party library identification

Aim: name the statically-linked library code (Allegro, libcda, apeg, dumb,
aastr, almp3, hq2x under `Engine/libsrc/`) in `rob_blanc_1.asm`. Lower
priority than Engine/Common matches for the "reconstruct Rob Blanc 1" goal,
but worth doing for IDB completeness (see `CLAUDE.md`).

**Status: paused by explicit user request (not exhausted).** One
productive session got `libcda-0.4` essentially done, made a solid dent
in Allegro (window/thread creation, mouse/keyboard/sound config reading,
DirectDraw driver init, assert/trace/exit-hook plumbing), and
conclusively ruled `apeg` out entirely (see its own section below) --
roughly 40 new `matches.json` entries total this thread. To resume, start
from the "Next up" section at the bottom: the still-unresolved
`dumb-0.9.2` XM-loader cascade, then `aastr-0.1.1`/`almp3-2.0.5`/`hq2x`
(untouched so far).

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

## Open lead: dumb-0.9.2 XM loader cluster (`sub_47C360`/`sub_47C4C0`) -- ambiguous, not resolved

Both functions reference the matched string `"Extended Module: "`
(`aExtendedModule`/`aExtendedModule_0`), and both are called identically
from the same two callers (`sub_477320`, `sub_477CE0`) at different
offsets. Investigation found the string is checked in exactly one place
in `IT/READXM.C` (`it_xm_load_sigdata`, line 613/627), but there's also a
`dumb_load_xm()` wrapper in the separate file `IT/LOADXM.C` (opens the
file, calls `dumb_read_xm()`, closes it) that doesn't reference the
string directly.

**Follow-up investigation (this round)**: `sub_477320` (`CODE XREF:
PlayMusic+1CE`) is NOT `my_load_mod` (`Engine/acsound.cpp:1290`, the
file-extension-based `if(charAfterDot=='X') dumb_load_xm(...)` dispatcher
originally suspected) -- reading its full body reveals a **5-format
check-then-load cascade**: `check1(filename)` -> if true,
`open("rb")`+`load1(file)`; else `check2(filename)` -> if true,
`load2(0,filename)`; else `check3(filename)` [= `sub_47C4C0`, our XM
signature check] -> if true, `load3(0,filename)` [= `sub_47C520`, NOT
`sub_47C360` as originally guessed]; else `check4(filename)` -> ...;
else `check5(filename)` -> ... This means `sub_47C4C0` is very likely a
lightweight XM *signature-check-only* probe (opens the file, reads 17
bytes, checks against "Extended Module: ", closes it, returns a
boolean) rather than the XM loader itself -- and the actual XM loader in
this cascade is `sub_47C520`, a function not previously investigated at
all. `sub_47C360`'s role is now unclear again (it was originally paired
with `sub_47C4C0` based on shared string reference alone, but doesn't
appear in this specific 5-format cascade at all -- it may belong to a
*different* caller/context entirely).

This 5-check/5-load cascade doesn't match any DUMB public API found in
`CORE/` (`REGISTER.C` is confirmed to be unrelated -- just DUMB's
internal `DUH_SIGTYPE_DESC` linked-list registry for signal *rendering*,
not file-format *detection*; `LOADDUH.C`/`MAKEDUH.C` don't match either).
Each `check(filename)` call takes a bare filename directly rather than an
opened `DUMBFILE*`, which doesn't match DUMB's usual `dumbfile_open`-first
architecture -- this cascade may well be **AGS's own custom
multi-format-probing code** (Engine-side, calling into individual DUMB/
JGMOD/other loaders), not DUMB library internals at all, which would
make it higher-value to identify than plain library code but wasn't
locatable in the available `Engine/*.cpp` source this round.

Logged as a genuine open lead, substantially better characterized than
before but still not resolved -- a future session should look for a
5-branch cascading format-prober in `Engine/` source (not yet found by
string search) before assuming this is DUMB-internal.

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

## Next up

- **dumb-0.9.2, `CORE/REGISTER.C`**: separate lead from the XM cluster
  above, not yet investigated.
- **aastr-0.1.1, almp3-2.0.5, hq2x**: no single-file leads currently in
  `leads.json`; lower priority, revisit if the above run dry.
