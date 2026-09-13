"""
IDA Pro script: master list of symbol renames (functions + globals) for
Ultima III (DOS).

Single accumulating script instead of one standalone name_*.py file per
finding. Whenever a function's or global's role becomes clear enough to
name confidently, add an entry to RENAMES below and re-run. Safe to
re-run repeatedly -- each entry is checked against the address's
*current* name and skipped if already applied, so old entries are
harmless to leave in place. Keep them rather than deleting: this file
doubles as a dated changelog of "what have we named and roughly why"
that's easy to diff in git.

Naming convention (Paul's call, 2026-09-13): **camelCase** for function
names, matching ultima1 rather than ultima2 (which uses snake_case) --
see docs/overview.md. Globals/statics keep the shared cross-project
convention regardless: leading underscore + camelCase (`_savegame`,
`_playerX`). Structs: PascalCase. Struct fields: underscore-prefixed
camelCase. Constant/lookup tables: ALL_CAPS. CRT-internal runtime
functions get a leading underscore to mark them as non-game-logic
regardless of the function-casing convention (`_fopen`, `_toupper`).

Convention: DRY_RUN starts True until a first batch of renames has been
verified end-to-end (export, re-open, spot check) -- flip to False only
as a deliberate, logged decision (see roadmap.md), same as ultima1/
ultima2's precedent.

Scope: plain renames only (idc.set_name on an address that IDA can
already address -- a function start, or an existing named/auto-named
data item). If a finding requires *creating* new data where there was
previously nothing (splitting an array, building a table, adding xrefs)
that's structural surgery, not a rename -- write a dedicated one-off
script for it instead. Struct member renames/additions go in
apply_structs.py, not here, since they use a different IDA API
(add_struc_member / set_member_name) and address a struct definition
rather than a single linear address.

For fuller justification of each rename, see the matching section of
docs/overview.md or docs/file-formats.md -- the `note` field here is
just enough to read the list top-to-bottom without cross-referencing.
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- ULTIMA.COM's true role, discovered while tracing start_0's tail
    # (asm ~213-246): it is NOT the main game, it's a title-screen/boot-
    # loader stub that loads and chains into BOOTUP.BIN. The chain
    # mechanism is a distinct trick from Ultima I's chainToExecutable:
    # DTA is set to offset 100h (this program's OWN code start), BOOTUP.BIN
    # is opened via FCB, the FCB's record-size field (ds:6Ah) is set to
    # the file's own size (copied from ds:6Ch), then a 2-byte "INT 21h"
    # opcode (CD 21) is hand-written into the PSP at offset FEh, SP is set
    # to FEh, and AH=14h (FCB sequential read) before jumping to FEh --
    # executing that freshly-written INT 21h reads the ENTIRE file (one
    # "sequential read" of record-size = whole file) directly over this
    # program's own code at offset 100h, and the INT 21h's own return
    # address (pushed by the CPU, popped on IRET since SP=FEh) lands
    # execution at offset 100h -- which now holds BOOTUP.BIN, not
    # ULTIMA.COM. If BOOTUP.BIN isn't found, falls through to `jmp near
    # ptr 0` (the PSP's built-in INT 20h stub at offset 0 -- clean
    # terminate). This explains why this IDB only has 57 functions: the
    # actual game (character creation, main loop, combat, etc.) lives
    # entirely in BOOTUP.BIN, an undisassembled second executable. See
    # docs/overview.md and docs/roadmap.md.
    (0x15E5A, "waitFrames",
     "busy-wait helper: if pollKeypressAndAnimate reports a key "
     "pending, returns immediately; otherwise spins (cx inner-loop, "
     "decrementing ax outer-loop) for a caller-specified duration. "
     "Used throughout titleScreenAndChainToBootup to pace the boot "
     "sequence's image reveals while staying keypress-responsive."),

    (0x15E68, "drawAnimatedPixelPath",
     "reads (x,y) coordinate pairs from a data stream at es:bx "
     "(es=_cgaSegment, bx=byte_14117 -- the buffer loaded from "
     "NAME.DAT) until a 0 length-byte terminator, plotting 2 adjacent "
     "pixels per pair (at column cl and cl+1, row 0xC0-nextByte) via "
     "plotPixel2bpp, with a wait between each via a delay loop counted "
     "by si. CORRECTS the file-formats.md guess that NAME.DAT is a "
     "'name generator table' -- direct disassembly evidence here shows "
     "it's actually consumed as a coordinate-pair pixel-path animation "
     "script (the classic hand-drawn logo reveal effect), not text "
     "data. See docs/file-formats.md, needs updating."),

    (0x15E96, "drawTitleBox1",
     "wrapper over drawSparkleBox: flashes (ah alternates 0FFh/0, one "
     "full pass each) an 8x8 box at position (0x4D,0x21). Exact visual "
     "role not confirmed without running the game -- see "
     "docs/roadmap.md."),

    (0x15EC2, "drawTitleBox2",
     "wrapper over drawSparkleBox: single pass (ah=0), 0x1D x 0x1D "
     "box at position (0x10,0x0B)."),

    (0x15ED8, "drawTitleBox3",
     "wrapper over drawSparkleBox: single pass (ah=0), 0x1D x 0x1D "
     "box at position (0x10,0x0C) -- same size as drawTitleBox2, "
     "different row."),

    (0x15EEE, "drawTitleBox4",
     "wrapper over drawSparkleBox: flashes (ah alternates 0FFh/0) an "
     "8x8 box at position (0x2B,0x21), same shape as drawTitleBox1 "
     "with different fixed position/pattern constants (0x55/0x55 vs "
     "0x08/0x08)."),

    (0x15F1A, "drawTitleBox5",
     "wrapper over drawSparkleBox: single pass (ah=0), 0x11 x 0x11 "
     "box at position (9,4)."),

    (0x15F30, "drawTitleBox6",
     "wrapper over drawSparkleBox: single pass (ah=0), 5x5 box at "
     "position (5,0x20) -- smallest of the 6 title-box wrappers."),

    (0x15F41, "plotPixel2bpp",
     "sets one pixel (ORs the bit in, doesn't clear) in a CGA 2bpp "
     "framebuffer: cl=column (offset +0x14 before use), ch=row, using "
     "_scanlineAddressTable[row] for the base address and a computed "
     "2-bit mask/shift for the column within its byte. Called by "
     "drawAnimatedPixelPath (direct pixel-path plotting) and indirectly "
     "underlies drawSparkleBox's per-pixel writes."),

    (0x15F6C, "drawSparkleBox",
     "draws a WxH box (bp+2/bp+0Eh give position, dl=height, [bp+1] a "
     "sparkle-mode flag) by copying from a source table "
     "(_scanlineAddressTable-indexed) to the destination framebuffer "
     "(_cgaSegment) row by row; when the sparkle flag is set, each "
     "pixel is conditionally replaced with 0 based on _prngState "
     "cycling past a threshold and a PC-speaker click is toggled in "
     "sync -- a shimmering/static-reveal box effect with an audible "
     "tick, the reusable primitive behind all 6 drawTitleBoxN wrappers "
     "plus 2 direct call sites in titleScreenAndChainToBootup."),

    (0x15FF3, "runBootFlagAnimation",
     "drives the boot logo/flag color-cycle animation: loops si=2 "
     "downto 0 selecting a row via 3-entry inline tables (byte_15FF2/"
     "byte_15FE9/byte_15FEC/byte_15FEF, addressed as [si+5FF0h] etc.), "
     "calling drawAnimationFrameRow and waiting (cx=0x9000 spin) each "
     "step, advancing only when the pixel/palette values at bx-indexed "
     "positions change -- an animated color-cycle/reveal effect, exact "
     "visual content not confirmed."),

    (0x16024, "drawAnimationFrameRow",
     "blits 16 rows of image data from byte_1432D (the ANIMATE.DAT-"
     "loaded buffer) to the screen using _scanlineAddressTable for "
     "destination addressing, at a position computed from bx (0x150 "
     "downto 2, stepping by 2) and dx/ax offsets into the source "
     "buffer. Only caller is runBootFlagAnimation."),

    (0x1607E, "initGraphicsMode",
     "sets CGA 320x200 4-color mode (INT 10h AH=0,AL=4) with a fixed "
     "palette (AH=0Bh calls), resets _textCursorPos to 0,0. Before "
     "that, probes for EGA/non-CGA memory-mapped I/O quirks via a "
     "read/write/restore test against _cgaSegment vs. the BIOS "
     "equipment word at 0000:0410h, adjusting _cgaSegment between "
     "0B800h/0BA00h accordingly -- if the probe fails outright, prints "
     "'Unable to access graphics display.' via raw INT 10h teletype "
     "output (not writeCharacter -- the text-mode font/cursor system "
     "isn't set up yet at this point) and hangs."),

    (0x15CE0, "titleScreenAndChainToBootup",
     "start_0, renamed for clarity now its full role is understood: "
     "seeds the PRNG/clock globals from DOS time/date, loads "
     "BLANK.IBM/EXOD.IBM/NAME.DAT/ANIMATE.DAT, runs the boot logo/flag "
     "animation and wind-direction display while polling for a "
     "keypress, then loads and chains into BOOTUP.BIN via the "
     "self-modifying FCB-read trick described above."),

    # -- low-level text output primitives, matching the exact functional
    # role (and, where applicable, the exact name) of ultima1/ultima2's
    # equivalent routines --

    (0x186CB, "writeString",
     "walks a null-terminated string at ds:si, calling writeCharacter "
     "per byte. Direct equivalent of ultima1/ultima2's write_string."),

    (0x186E1, "writeStringPreserveCx",
     "push cx; call writeString; pop cx -- thin wrapper so a caller's "
     "own cx-based counter survives the call. Used by printErrorPrefix "
     "and the file-error handlers, whose cx holds an in-progress "
     "loop/record count at the time of the error."),

    (0x186E7, "swapCursorPos",
     "xchg dx,word_16220 (the packed text-cursor x/y). Used to jump the "
     "cursor to a fixed message position and back by swapping with a "
     "caller-supplied dx, e.g. printErrorPrefix and the numeric-input "
     "prompt's blink toggle."),

    (0x186EC, "writeCharacter",
     "the single-character output primitive: special-cases 8 "
     "(backspace, decrements the cursor column with wraparound to the "
     "previous row), otherwise draws the glyph via drawCharGlyph and "
     "advances the cursor, wrapping every 40 (28h) columns. Matches "
     "ultima1/ultima2's write_character/print_char role exactly."),

    (0x18720, "drawCharGlyph",
     "blits one 8x8 character glyph to the CGA framebuffer "
     "(word_186C9 segment) at the current text cursor position, "
     "reading 16 bytes of glyph data from ds:[charCode*16 + 76C9h] "
     "(absolute 0x176C9-0x17EB9 for charCode 0-127). OPEN QUESTION: "
     "that address range is entirely zero bytes in the current IDB "
     "(verified directly, not just by address arithmetic) -- so "
     "despite falling within byte_172C9's declared 'db 1400h dup(0)' "
     "span, the real CHARSET.ULT font data is NOT compiled into "
     "ULTIMA.COM's file image the way first suspected. Either it's "
     "loaded at runtime by code not yet found in this file, or "
     "BOOTUP.BIN populates this shared buffer after chaining in, "
     "before any of ULTIMA.COM's own text output (including "
     "updateWindDisplay's wind-direction strings) actually needs "
     "readable glyphs. Flagged in docs/roadmap.md."),

    (0x18790, "checkDebugModeFlag",
     "LOW CONFIDENCE: trivial stub, always `mov al,0; retn`. Its "
     "result is stored in byte_1432C, which titleScreenAndChainToBootup "
     "later compares against 0FFh in a wait loop -- since this always "
     "returns 0, that loop never actually waits. Plausibly a compiled-"
     "out debug/test-mode gate; not independently confirmed."),

    (0x18893, "readLine",
     "interactive line editor: reads chars via pollKeypressAndAnimate/"
     "getKeypressAndWaitRaw, echoes printable chars (0x20-0x7F) via "
     "writeCharacter into es:di up to a max count in cx, handles "
     "backspace (Ctrl-Backspace/Del/F3 all treated as backspace) by "
     "erasing the last char, ends on Enter. Beeps (playErrorBeep, "
     "al=0FEh) on backspace-past-start or a non-printable/full-buffer "
     "key. Returns chars actually consumed in bx (orig cx - final cx)."),

    # -- boot-time PRNG / animation-timing primitives --

    (0x188E1, "stepTimeSeededPrng",
     "15-byte rolling ADC accumulator seeded from the boot-time DOS "
     "date/time snapshot (word_16225.._16231, captured once in "
     "titleScreenAndChainToBootup via INT 21h AH=2Ch/2Ah), then "
     "self-increments its own state each call (loop at loc_18903). "
     "Used both as a busy-wait delay AND an entropy source for the "
     "logo animation's timing/pitch variance -- same dual-purpose "
     "pattern as ultima2's rand_byte, but here explicitly tied to the "
     "captured clock bytes rather than a generic running seed."),

    (0x1891A, "swapAnimTableRows",
     "swaps 4 words (8 bytes) between two rows of a table, at "
     "[bx+dx]/[bx+dx+20h] and [bx+dx+2]/[bx+dx+22h], looping cx=8 "
     "-- wait, single swap per call (cx counts the word-pairs: 8 "
     "iterations of a 4-byte double-swap = 32 bytes moved per call). "
     "Generic table-row-swap helper, only caller is updateLogoAnimationA."),

    (0x18940, "updateLogoAnimationA",
     "4 independent countdown timers (byte_16233/16234/16235/16236, "
     "periods 3/2/3/2 ticks) each gating a swapAnimTableRows call at a "
     "different fixed table offset (0/0x840/0x800/0x880) into "
     "byte_162C9. One of several periodic decorative animation updaters "
     "driven by runIdleAnimationTick during the boot/title screen wait-"
     "for-keypress loop -- exact visual effect (logo shimmer? flag "
     "wave?) not confirmed without running the game."),

    (0x1898F, "updateLogoAnimationB",
     "same shape as updateLogoAnimationA (countdown-gated periodic "
     "update) but swaps 2-word pairs directly instead of via "
     "swapAnimTableRows, at 3 different fixed offsets into byte_162C9 "
     "(0x1C0/0x180/0x2C0), timers byte_16237/16238/16239 (periods "
     "4/3/2)."),

    (0x18A07, "updateLogoAnimationC",
     "countdown timer byte_1623C (period 0Ah). Swaps the first 0x400 "
     "bytes of byte_172C9 with byte_166C9 (960 bytes) then byte_165C9 "
     "(64 bytes) -- a double-buffer flip for animation frame data. "
     "byte_172C9+0x400 onward (up to +0x1400) is where drawCharGlyph's "
     "glyph-data offset (76C9h relative) lands, but that region reads "
     "as all zeros in the current IDB -- see drawCharGlyph's note and "
     "docs/roadmap.md, not yet resolved."),

    (0x18A48, "updateLogoAnimationD",
     "countdown timer byte_1623B, branches on byte_10105's mode "
     "(gameMode==0x80 uses one table row via ds:0-indexed lookup + "
     "computeAnimTableByte, any other mode uses a different fixed "
     "period/row). Mechanism traced; exact visual purpose not "
     "confirmed."),

    (0x18AA7, "runIdleAnimationTick",
     "the top-level per-poll animation dispatcher: conditionally calls "
     "updateWindDisplay (skipped if gameMode is 0x80 or 4), then always "
     "updateLogoAnimationA/B/D/C in that order, then drawLogoTileGrid. "
     "Called from pollKeypressAndAnimate every time the keyboard buffer "
     "is polled and empty."),

    (0x18ACA, "pollKeypressAndAnimate",
     "INT 16h AH=1 (peek keyboard buffer, non-destructive), and unless "
     "gameMode==1, also drives runIdleAnimationTick -- this is how the "
     "boot-screen animation advances while idly waiting for input."),

    (0x18ADD, "getKeypressAndWaitRaw",
     "blocking key read with a blinking text-cursor (glyph 0x1Eh) drawn "
     "while polling via pollKeypressAndAnimate, erased once a key "
     "arrives, then INT 16h AH=0 (read char, wait). Named to match "
     "ultima1's function of the exact same role/mechanism."),

    (0x18B43, "printHexByte",
     "prints AL as 2 hex digit characters via printHexNibble (high "
     "nibble first)."),

    (0x18B5C, "printHexNibble",
     "prints the low nibble of AL as one hex digit (0-9/A-F) via "
     "writeCharacter."),

    (0x18B6E, "accumulateInputDigit",
     "per-character accumulator for a numeric-input prompt: maintains "
     "a hex value in bx (bx = bx*16 | nibble, for any 0-9/A-F char) and, "
     "in parallel, a decimal value in dx (dx = dx*10 + digit) that gets "
     "invalidated (set to 0FFFFh) the moment a non-decimal hex digit "
     "(A-F) appears. Belongs to an orphaned (no IDA-recognized function "
     "boundary) numeric-entry prompt routine at ~0x18C79 -- see "
     "docs/roadmap.md, needs ida_funcs.add_func before it can be named "
     "and its purpose (looks like a 2-hex-digit code-entry prompt, "
     "possibly copy-protection related, no confirmed caller found in "
     "this file) pinned down."),

    # -- file I/O, matching ultima1/ultima2's access_file/load_map role --

    (0x18D10, "loadFile",
     "FCB-based file loader: parses the given DS:DX filename into an "
     "FCB (INT 21h AH=29h), opens it via openFileWithRetry (which "
     "prompts 'Wrong Diskette!' and retries on open failure -- this is "
     "the boot sequence's floppy-disk-swap handling), then one "
     "sequential read (AH=14h) into the DTA set by the caller, closes "
     "the file. On read failure, prints 'Unable to read file <name>' "
     "and hangs (`jmp $`). Same architectural role as ultima2's "
     "access_file, but no inline-data-after-CALL trick here -- the "
     "filename is passed normally via DX, not embedded in the code "
     "stream."),

    (0x18D63, "openFileWithRetry",
     "INT 21h AH=0Fh (open). On failure, keeps prompting 'Wrong "
     "Diskette!' (with an error beep) and retrying until the file "
     "opens or the game is otherwise interrupted -- classic 1980s "
     "floppy-swap UX, not a hard error path."),

    (0x18D9E, "printErrorPrefix",
     "swapCursorPos to a caller-given fixed position, then "
     "writeStringPreserveCx the caller's message -- used to print the "
     "fixed portion of 'Unable to read/write file ' before the actual "
     "filename (printed separately by the caller right after)."),

    (0x18DA7, "adjustAnimSpeed",
     "LOW CONFIDENCE: scales/signs a delta in AL based on gameMode "
     "(byte_10105==0 halves it on the way in and out via shr, and "
     "forces a nonzero sign in between). Only caller is "
     "updateLogoAnimationD; exact purpose not confirmed."),

    (0x18DCA, "drawLogoTileGrid",
     "calls drawTileGrid with an 11x11 grid (al=ah=0Bh), source data "
     "byte_162C9, screen block offset 0x142 -- draws a decorative "
     "11x11 tile image on the boot/title screen using the same 64-"
     "byte-per-tile primitive the real game's SHAPES.ULT tiles use "
     "(see file-formats.md). Called every runIdleAnimationTick, after "
     "the frame data has potentially been shuffled by "
     "updateLogoAnimationA/B/C."),

    (0x18808, "drawTileGrid",
     "generic N x M grid blitter for 64-byte CGA-2bpp tiles (al=rows, "
     "ah=cols, bp=source tile-index array, di=screen block offset). "
     "High confidence this is the same primitive the real game (in "
     "BOOTUP.BIN) uses for map/combat-arena tile rendering -- the "
     "11x11 grid size drawLogoTileGrid uses matches the CNFLCT_* "
     "combat-arena dimensions documented in file-formats.md exactly."),

    (0x18DE5, "computeAnimTableByte",
     "LOW CONFIDENCE: bit-index computation (bh scaled/added into bh, "
     "then combined with bl to form a table offset) returning a single "
     "byte via `mov al,[bx]`. Only caller is updateLogoAnimationD; "
     "exact table/purpose not confirmed."),

    (0x18E0A, "updateWindDisplay",
     "HIGH CONFIDENCE, resolves a real Ultima III game mechanic: every "
     "25 (0x19) ticks (byte_1623A countdown), picks a new wind-"
     "direction index 0-4 via stepTimeSeededPrng (rejecting a repeat "
     "of the current one), stores it, then looks up and prints one of "
     "5 fixed strings via a pointer table at 0x18E00 (WIND_DIRECTION_"
     "TABLE) at fixed screen position 0x1706: 'Calm Wind', 'North "
     "Wind', 'East Wind', 'South Wind', 'West Wind' (see the "
     "individual aXxxWind renames below). This is Ultima III's classic "
     "wind-direction indicator, shown decoratively on the boot/title "
     "screen ahead of the real game."),

    (0x18E79, "playSoundEffect",
     "sound effect dispatcher: for al in 0F4h..0FFh (checked via `cmp "
     "al,0F4h; jb skip`), and only if _soundEnabled is nonzero, "
     "computes index=(0FFh-al)*2 and calls SOUND_EFFECT_TABLE[index]. "
     "12 effects total (0F4h-0FFh); only 0FEh has a confirmed call "
     "site within this file (see playErrorBeep) -- the other 11 are "
     "presumably called from BOOTUP.BIN, which shares this low-level "
     "sound/graphics runtime (same architecture as the tile-grid "
     "primitive above)."),

    (0x18E99, "playToneFF",
     "sound effect 0FFh (table index 0). Square-wave beep, ~16 "
     "cycles. No confirmed call site in this file -- named generically "
     "by effect ID pending a BOOTUP.BIN cross-reference."),

    (0x18EBB, "playErrorBeep",
     "sound effect 0FEh (table index 1). Confirmed: called directly "
     "(al=0FEh; call playSoundEffect) from readLine (invalid/full-"
     "buffer key), the orphaned numeric-input prompt, and "
     "openFileWithRetry's 'Wrong Diskette!' prompt -- the universal "
     "'invalid input' beep."),

    (0x18EDD, "playToneFD",
     "sound effect 0FDh (table index 2). Also called directly (not "
     "via the dispatch table) by playToneF9 with fixed parameters -- "
     "a shared tone-sweep primitive, not effect-0FDh-exclusive."),

    (0x18F36, "playToneFC", "sound effect 0FCh (table index 3), no confirmed call site."),
    (0x18F5F, "playToneFB", "sound effect 0FBh (table index 4), no confirmed call site."),
    (0x18F7A, "playToneFA", "sound effect 0FAh (table index 5), no confirmed call site."),

    (0x18F98, "playToneF9",
     "sound effect 0F9h (table index 6): thin wrapper, calls "
     "playToneFD (sub_18EDD) with fixed bl=0E0h,bh=4."),

    (0x18FA2, "playToneF8", "sound effect 0F8h (table index 7), no confirmed call site."),

    (0x18FC8, "playToneF7",
     "sound effect 0F7h (table index 8): thin wrapper over the shared "
     "helper at 0x18FDC (bh=0,bl=0FFh)."),

    (0x18FD2, "playToneF6",
     "sound effect 0F6h (table index 9): thin wrapper over the shared "
     "helper at 0x18FDC (bh=0,bl=8)."),

    (0x18FDC, "playToneHelper",
     "shared tone-sweep primitive for playToneF7/playToneF6 -- not "
     "itself in SOUND_EFFECT_TABLE, just a factored-out helper."),

    (0x18FFE, "playToneF5", "sound effect 0F5h (table index 10), no confirmed call site."),
    (0x1902E, "playToneF4", "sound effect 0F4h (table index 11), no confirmed call site."),

    # -- globals --

    (0x14115, "_cgaSegment",
     "either 0B800h or 0BA00h depending on the EGA-vs-CGA memory probe "
     "in initGraphicsMode -- the segment used for the boot logo's "
     "double-buffered load-in effect (distinct from the fixed "
     "_cgaFramebufferSegment used for actual text/tile drawing)."),

    (0x186C9, "_cgaFramebufferSegment",
     "fixed 0B800h -- the CGA framebuffer segment used by drawCharGlyph "
     "and drawTileGrid for on-screen drawing."),

    (0x16220, "_textCursorPos",
     "packed text cursor position: low byte = column (0-39), high byte "
     "= row. Read/written throughout the text-output primitives "
     "(writeCharacter, swapCursorPos, drawCharGlyph)."),

    (0x15A2D, "_prngState",
     "small 2-register additive PRNG seed (init 9DE3h), stepped by "
     "drawSparkleBox's inner loop -- distinct from stepTimeSeededPrng's "
     "clock-derived state, used specifically for the sparkle-reveal "
     "box-drawing effect's randomized bit pattern."),

    (0x18E60, "_soundEnabled",
     "mute flag gating playSoundEffect (init 0FFh = enabled). Toggled "
     "elsewhere via `xor ...,0FFh` per the sound primitives' shared "
     "comment block (not yet traced to a specific toggle site)."),

    (0x15A62, "_scanlineAddressTable",
     "100-entry (0x64) table of CGA scanline addresses, built once in "
     "titleScreenAndChainToBootup (stepping alternately by 0x2000/"
     "0xE050 -- the classic CGA interleaved-bank scanline stride) and "
     "read by plotPixel2bpp/drawSparkleBox instead of recomputing the "
     "address formula per pixel."),

    (0x18E61, "SOUND_EFFECT_TABLE",
     "12-entry jump table for playSoundEffect, indexed by "
     "(0FFh-effectId)*2, effect IDs 0F4h-0FFh -- see the playTone* "
     "renames above for the full id-to-function mapping. Verified via "
     "get_word() against every entry, not just address arithmetic -- "
     "IDA's own auto-generated label for this table was already "
     "'funcs_18E91', which does NOT match its real address (0x18E61); "
     "don't trust an auto-name's numeric suffix as its address."),

    (0x18E00, "WIND_DIRECTION_TABLE",
     "5-entry pointer table read by updateWindDisplay, indexing the 5 "
     "wind-direction strings (aCalmWind/aNorthWind/aEastWind/"
     "aSouthWind/aWestWind) in selection order Calm/North/East/South/"
     "West -- note this differs from their in-memory layout order "
     "(Calm/North/South/East/West), i.e. the table deliberately "
     "reorders them rather than just being a plain sequential index."),

    (0x1623D, "aCalmWind", "'\\x10Calm Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x1624A, "aNorthWind", "'\\x10North Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x16257, "aSouthWind", "'\\x10South Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x16264, "aEastWind", "'\\x10East Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),
    (0x16271, "aWestWind", "'\\x10West Wind\\x11\\x00' -- wind-direction display string, see updateWindDisplay."),

]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    if not RENAMES:
        print("[no-op] RENAMES is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
