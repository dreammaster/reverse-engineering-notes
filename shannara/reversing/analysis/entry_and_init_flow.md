# Program entry & top-level flow

Traced directly from `Shannara Demo.asm` (not previously covered by the prior
analysis pass, which focused on structures and library identification rather
than the top-level control flow). All of this is HIGH confidence — it's a
straight linear read of `main`, `setup_demo`, `init`, and their direct
callees, not inference.

## Call chain

```
start (LE/DOS4GW loader stub)
  -> __CMain                          [Watcom CRT startup, boilerplate]
       - sets up the C stack (__ASTACKSIZ/__ASTACKPTR)
       - call __CommonInit             [CRT init: heap, FPU, etc.]
       - call main(argc=____Argc, argv=____Argv)
main                                   [0x157785, the real entry point of interest]
  -> _clearscreen(0)
  -> fprintf_ x3                       banner text (title, copyright, DOS/4GW)
  -> [if argv[1] is -?/-h//?/-H/etc.]  print usage line, jump to shutdown banner, exit
  -> init()
  -> setup_demo()
  -> init_files("DEMO")                sets the file-prefix used by the resource loader
  -> init_audio()
  -> graphics_mode(5)                  switch to the demo's SVGA mode
  -> get_ini("LOOPING") != -1?         -> looping = key present (bool)
  -> q_init(edx=0, ebx=&_default_palette, ecx=looping)   [see note below - ecx is NOT actually read]
  -> [if q_init failed] text_mode(); exit(0x10)
  -> set_music_volume(80)
  -> loop:                             kiosk-mode intro reel
       black_palette()
       gxClearDisplay(color=0, 6); gxUpdateDisplay()
       flix_play(&_logosvga)                     [Legend logo splash]
       flix_play(&_market, count=0x10)            [16-entry cutscene script]
       flix_play(&_still_pics?, count=0x14)       [20-entry cutscene script]
       flix_play(&_morph?, count=3)                [3-entry cutscene script]
       if !looping: break                          (play once, exit)
       if key_hit(): break                          (looping: any keypress ends it)
       else: goto loop                              (looping: repeat the reel)
  -> clear_keys(); q_kill(); text_mode()
  -> fprintf_ x3                       shutdown banner ("Shannara CD-ROM Demo v%s", etc.)
  -> exit(0)
```

**This confirms the demo genuinely has no interactive gameplay** — `main`
never reaches any room/UI/game-logic code path at all. It is a fixed,
non-interactive slideshow of `flix_play` cutscene scripts (see
[`FlixEntry`](engine_overview.md) in the existing structure notes), gated by
an INI-driven "loop until keypress" option. The four data blobs
(`_logosvga`, `_market`, `_still_pics?`, `_morph?`) are `FlixEntry[]` arrays;
their names are the disassembler's best-effort guesses and worth confirming
against the actual bytes if reimplementing this demo specifically. Everything
under `_rgn`/`Region`/room-dispatch/etc. that the previous pass found is
therefore **linked into the binary but never called from this entry point in
the demo build** — real evidence that it's shared full-game engine code
statically pulled in, not demo-specific.

## `init()` — engine bootstrap (0x15E1F6-ish region; see `init` proc)

```
init(a1=eax):
    init_memory()          -> MemoryBlock/_master_table handle system, see engine_overview.md
    init_ini_info(a1)
    capture_break()        -> installs Ctrl-Break trap
    trap_errors()          -> installs DOS critical-error / exception trap
    _AIL_startup()         -> Miles/AIL sound system bring-up
    _AIL_delay(2)
    atexit(kill_all)       -> registers the shutdown/cleanup handler
    get_clock()             -> primes the clock subsystem (return value discarded)
```

## `setup_demo()` — text-mode config wizard, run before any graphics init

Entirely text console I/O via `fprintf_`/`_textBuffer`, `get_key_upper`,
`msleep`. Sequence:

1. Prints the two disclaimer paragraphs ("run directly from the CD", "requires
   VESA compatible SuperVGA").
2. If `LOOPING` isn't already in the INI: asks the user Y/N whether the demo
   should loop continuously, `Esc` exits immediately (`_null_exit_rtn_(1)`).
   Writes the `LOOPING` key via `add_ini` if yes.
3. Music/sound driver setup:
   - If both `MDI.INI` and `DIG.INI` exist (Miles driver-list config files
     for MIDI/digital audio), skip straight to a **style** picker: `_style`
     is presented via `select_option` (`"for music"`, `"Please select the
     configuration you are..."`, 3 options), and the chosen entry's value is
     written straight to the INI via `add_ini`.
   - Otherwise: check whether `MUSIC` and `SOUND` INI keys are already both
     set; if so, done. If not, run `select_option` against the `_music`
     table (15 entries, `"for music"`) to pick a music driver, write it via
     `add_ini`, then feed the chosen index through **`sub_1574EC`** (see
     below) to auto-suggest a matching digital-audio driver from `_sound`
     (13 entries) — if there's no good match (`sub_1574EC` returns -1),
     fall back to another `select_option` prompt against `_sound` directly
     ("for digital audio"). Either way the final choice is written via
     `add_ini`.

### `sub_1574EC` (undocumented, now resolved) — music→sound auto-match table

Not really a function so much as a hardcoded correlation table implemented
as a 10-case jump table on the chosen `_music[]` index (`eax`):
- index 0,1,2 -> returns 0 (first `_sound[]` entry)
- index 3..9 -> returns `index - 1`
- index >= 10 -> returns -1 (no auto-suggestion, force manual pick)

This is Legend's period-appropriate assumption about which digital-audio
driver typically accompanies a given MIDI/music driver choice (e.g. Sound
Blaster-family MIDI implies Sound Blaster-family digital audio). The exact
`_music[]`/`_sound[]` string tables would need pulling from the data segment
to name indices precisely — noted as a follow-up.

### `graphics_mode(mode)`

Only 3 values of `mode` are valid; everything else prints
`"Error: graphics_mode( %d ) is invalid\n"` and calls `_null_exit_rtn_(1)`:

| mode | internal gx mode value passed to `gxSetMode` |
|---|---|
| 3 | 8 |
| 4 | 0xB7 (183) |
| 5 | 0x25 (37) — **this is what `main` actually uses** |

Also calls `reset_color_cycle()`, `reset_clip_region()`, and
`set_gr_color(0xF)` (default draw color = white/15) on an actual mode
change. `gxSetMode`'s own internal mode-value semantics weren't traced
further — worth checking if they're VESA mode numbers or a private enum.

### `q_init` — corrected signature (concrete example of the documented Phase-2 auto-signature undercount)

Auto-inferred as `int __usercall q_init@<eax>(int a1@<edx>)` — a
single-parameter function. Reading the body shows this is an **undercount**,
exactly the failure mode flagged in `engine_overview.md` §5 ("known
limitation... a real parameter only consumed after an early guard-clause
branch can be undercounted"):

- `edx` *is* read early (`movsx edx, cx` after `mov ecx, edx`) — correctly
  detected.
- `ebx` (palette pointer) is read via `test ebx, ebx` right after the
  function's **first** branch (`jnz`/`jz` on `_qbuffer`) — undetected, but
  genuinely a parameter: gates whether a new `_qbuffer` is allocated at all,
  and is stored into `_qpalette`.
- The incoming `ecx` (which `main` sets to the `LOOPING` flag before the
  call) is `push`ed and `pop`ped completely unused — a case of a caller
  passing a value in a register the callee happens not to clobber, **not**
  a real parameter. `eax` (0 at the only call site) is similarly unused as
  input; the function only writes `eax` on the way out (`mov eax,
  _video_buffer`, i.e. the return value).

Real signature: `void *q_init(int edx_yOrigin, void *ebx_palette)` — returns
`_video_buffer`, and as a side effect (first successful call only, gated by
`_qbuffer == 0 && ebx != 0`) allocates a `0x13C06`-byte Q-codec buffer via
`new_pointer`, stores it as both `_qbuffer`/`_qbuffer_orig`, computes
`_video_buffer` via `gxVideoAddr`, sets `_q_y1`/`_q_y2` (a 0x11F-tall window
starting at the `edx` row), and `memset`s the palette region referenced by
`ebx` to 0.

## Open follow-ups from this pass

- `_logosvga`, `_market`, `_still_pics?`, `_morph?` — confirm these
  `FlixEntry[]` array contents/names against the actual `.LE` data bytes;
  the `?` suffixes mean the disassembler's guess is unconfirmed.
- `_music`/`_sound`/`_style` option tables (8-byte `{namePtr, value}` stride,
  consumed by `select_option`) haven't had their string contents pulled out
  yet — would directly name `sub_1574EC`'s indices.
- `gxSetMode`'s mode-value semantics (8 / 0xB7 / 0x25) not independently
  confirmed as VESA vs. private enum.
