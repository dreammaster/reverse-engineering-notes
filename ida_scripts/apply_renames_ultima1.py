"""
IDA Pro script: master list of symbol renames (functions + globals) for
ULTIMA.EXE (ultima1.idb).

Single accumulating script per the convention established in
apply_renames_out.py (see that file's docstring for the full
rationale). Whenever a finding is confirmed, add an entry to RENAMES/
GLOBAL_RENAMES/LOCATION_RENAMES below and re-run.

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1 -ScriptName apply_renames_ultima1.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- first pass: the CRT layer. ULTIMA.EXE turns out to link the
    # exact same Microsoft C runtime object files as OUT.EXE -- every
    # one of these matched OUT.EXE's already-decoded functions
    # byte-for-byte in shape (identical AH= DOS subfunctions, identical
    # magic constants like the 0x8102/0x180 open-mode flags, several
    # even matching OUT.EXE's exact byte size). Confirmed by direct
    # reads, not just size/shape inference, before renaming. See
    # docs/overview.md#ultimaexe-crt-layer-transferred-from-outexe. --

    (0x12437, "_dos_open", "AH=3Dh OPEN DISK FILE WITH HANDLE."),
    (0x12448, "_dos_close", "AH=3Eh CLOSE A FILE WITH HANDLE."),
    (0x12456, "_dos_read", "AH=3Fh READ FROM FILE WITH HANDLE."),
    (0x1246A, "_dos_lseek", "AH=42h MOVE FILE READ/WRITE POINTER."),
    (0x1248E, "_dos_write",
     "AH=40h WRITE TO FILE WITH HANDLE; sets errno=0x1C (ENOSPC) on a "
     "short write, exactly matching OUT.EXE's _dos_write."),
    (0x124B7, "_dos_ioctl_get", "AX=4400h IOCTL GET DEVICE INFORMATION."),
    (0x124CB, "_exit",
     "calls the single registered exit hook (word_18530) if set, then "
     "INT 21h/4Ch QUIT WITH EXIT CODE."),
    (0x12BE6, "_dos_ioctl_set", "AX=4401h IOCTL SET DEVICE INFORMATION."),
    (0x12BF8, "_dos_creat", "AH=3Ch CREATE A FILE WITH HANDLE (CREAT)."),
    (0x12C09, "_dos_creatnew", "AH=5Bh CREATE NEW FILE."),
    (0x12C16, "_dos_creattemp", "AH=5Ah CREATE UNIQUE FILE."),

    (0x114B7, "_nheapinit",
     "one-time near-heap setup called from start2: extends the "
     "program's memory block via SETBLOCK and zeroes the free-list "
     "globals (word_18468/6A/6C/6E/74). Same IDB-hygiene situation as "
     "OUT.EXE's _nheapinit -- its 'FUNCTION CHUNK AT 23CC' is actually "
     "the shared translateDosErrorToErrno tail nested inside this "
     "proc's range, not real heap-init code."),
    (0x1141E, "setCriticalErrorHandler",
     "toggles a custom DOS INT 24h handler on/off, saving/restoring "
     "the original vector in dword_18446 (byte_18445 = installed "
     "flag). Called from _main."),
    (0x11D03, "_open",
     "composite open: interprets O_CREAT/O_TRUNC/O_EXCL flag bits and "
     "dispatches to _dos_open/_dos_creat/_dos_creatnew/_dos_creattemp. "
     "Called only from _openfile."),
    (0x11053, "_openfile",
     "the real body of fopen (already named, thin wrapper): parses "
     "the mode string ('r'/'w'/'a', 'b', '+') and calls _open."),
    (0x111B4, "_fread",
     "byte-at-a-time buffered read with automatic refill, called "
     "(buffer, size, count, stream) from loadLogo to read the raw "
     "castle.4/castle.16 CGA/EGA framebuffer images."),
    (0x10F96, "_fclose",
     "flushes via _flsbuf if dirty, frees the stream buffer via "
     "_nfree, releases the OS handle via releaseFileHandle."),
    (0x120F4, "_flsbuf",
     "flush-buffer-and-write-one-char, matching OUT.EXE's _flsbuf "
     "exactly in size (728 bytes) and call signature ((char, stream) "
     "-- confirmed by _fclose's call site pushing (0xFFFF, stream))."),
    (0x11B71, "_nfree",
     "near-heap free: coalesces the freed block into the free-list "
     "(sentinel word_1846C/cursor word_18470), matching the call site "
     "in _fclose exactly."),
    (0x12CA4, "_nmalloc",
     "near-heap allocator: walks the free-list, splits a block if big "
     "enough, else calls _nheapgrow (0x133B2) to extend the heap and "
     "_nfree to link the new space in before retrying."),
    (0x133B2, "_nheapgrow",
     "grows the near heap via DOS SETBLOCK. Only called from "
     "_nmalloc."),
    (0x11C1A, "releaseFileHandle",
     "closes the OS handle and clears the internal handle-table slot; "
     "called only from _fclose."),

    # -- second pass: confirms and names the exec-chaining cluster,
    # transferred from OUT.EXE the same way as the CRT cluster above,
    # but with an extra payoff: traced all the way to the literal
    # "gen.exe" string. showTrademarks unconditionally chains to
    # GEN.EXE after displaying the trademark screen -- ULTIMA.EXE's
    # entire job is the title/attract-mode sequence, then hand off.
    # No other executable name (out.exe, space.exe, mondain.exe)
    # appears anywhere in this binary's strings. See
    # docs/overview.md#ultimaexe-chains-unconditionally-to-genexe. --

    (0x10E4D, "chainToExecutable",
     "pushes (filename, &argv, envp) and calls execProgramEntry. "
     "Called from showTrademarks with filename='gen.exe' (literal "
     "string aGen_exe) -- the only executable name referenced "
     "anywhere in this binary. If it returns (chain attempt failed), "
     "showTrademarks shows 'Insert ULTIMA I disk and press RETURN' "
     "and loops back to retry, matching OUT.EXE's insertDisk pattern "
     "exactly."),
    (0x11C93, "execProgramEntry",
     "findExecutableFile then buildAndChainExecutable -- same role as "
     "OUT.EXE's execProgramEntry, and also nested inside _nheapinit's "
     "proc range here (not renamed as a location since it already had "
     "its own clean proc/endp boundary in this IDB, unlike OUT.EXE's "
     "copy)."),
    (0x12595, "findExecutableFile",
     "tries the bare filename first (hasFileExtension check via "
     "_dos_getfileattr), else tries two candidate extensions (offsets "
     "0x4764/0x4768, presumably \"EXE\"/\"COM\" strings) via "
     "ensureFileExtension."),
    (0x12E3B, "hasFileExtension",
     "scans a filename backward for '.' before a path separator, "
     "matching OUT.EXE's hasFileExtension."),
    (0x1331F, "_dos_getfileattr",
     "AH=43h/AL=00h GET FILE ATTRIBUTES, used as a file-exists check "
     "by findExecutableFile."),
    (0x12EBC, "ensureFileExtension",
     "appends a default extension if the filename doesn't already "
     "have one, matching OUT.EXE's ensureFileExtension."),
    (0x12A1A, "buildAndChainExecutable",
     "measures and concatenates the argv array into a DOS command-"
     "tail buffer via _nmalloc'd scratch space (strlen via sub_12C8D "
     "below), then calls execProgram to load and jump to the target."),
    (0x12C8D, "strlen", "plain length-count helper used while building the command tail."),
    (0x130F4, "execProgram",
     "the actual overlay loader -- 555 bytes, exactly matching "
     "OUT.EXE's execProgram in size. Reads the target file directly "
     "(open/read/close/lseek), resizes this process's own memory "
     "block, and (per OUT.EXE's identical implementation) far-JMPs "
     "into the loaded image without ever calling DOS INT 21h/4Bh EXEC "
     "or returning control here."),

    # -- third pass: writeString2_mb (pre-existing name from earlier
    # work) turned out to be a printf-family formatter, not anything
    # multi-byte-encoding-related -- renamed along with its 3 helpers
    # once the evidence was clear (walks a %-format string, dispatches
    # per-specifier conversion, then fputs/putc's the result to a
    # fixed FILE* at 0xC73A, presumably stdout). Called only from
    # checkMem and sub_104D0, both early _main startup diagnostics --
    # not a general in-game text routine (that's writeString/
    # writeCharacter_0 elsewhere, already named). --

    (0x11AE7, "vsprintf",
     "walks a %-format string char by char (handling %% as a literal "
     "percent), dispatching each real specifier to formatArg. Matches "
     "the classic C runtime vsprintf loop shape exactly."),
    (0x1153D, "formatArg",
     "per-specifier conversion (width/precision/flags handling for "
     "%d/%s/%x/etc., matching MSC's internal _output-family size and "
     "shape) -- 1245 bytes, the largest function in this executable, "
     "confirming this is real format-conversion logic and not "
     "something simpler."),
    (0x11CD1, "fputs",
     "writes a null-terminated string to a stream one character at a "
     "time via putc, stopping at the null terminator."),
    (0x1253B, "putc", "writes one character to a stream via _fwrite-family internals."),
    (0x137FE, "toupper",
     "classic 'a'-'z' -> subtract 0x20 uppercase check. Called from "
     "_main on argv[1][0] right before comparing it to 'C' -- resolves "
     "that mystery: it's a case-insensitive command-line flag."),

    (0x10E82, "exit",
     "the real C-standard exit(): flushes/closes every entry in the "
     "FILE table (same 20-slot, 14-byte-stride table _flushall walks "
     "in OUT.EXE) AND closes raw OS handles from an internal handle "
     "table, then calls the already-named _exit (which just runs the "
     "exit hook and INT 21h/4Ch -- no cleanup). This distinction "
     "matches real ANSI C semantics: exit() cleans up, _exit() "
     "doesn't. Called by checkMem/checkVideoCard on fatal startup "
     "errors."),

    (0x104D0, "checkVideoCard",
     "calls detectVideoMode; if no supported adapter is found, prints "
     "'Ultima I requires a Color Graphics Adapter...' and exit(1)s. "
     "Otherwise calls probeMemoryForVideoMode with a flag for whether "
     "EGA mode was detected (and no -C override), or 0 otherwise."),

    (0x13BD5, "probeMemoryForVideoMode",
     "queries total available memory via the classic DOS trick (INT "
     "21h/48h ALLOCATE MEMORY with BX=0xFFFFh deliberately over-"
     "requesting; the failure still returns the true max in BX) into "
     "word_18866 -- the value checkMem later compares against 0x1900. "
     "In EGA mode, additionally reserves a 16KB (0x400-paragraph) "
     "block and pokes its segment into a fixed low-memory location "
     "(physical 0x810, via es=0x50/[bx=0x10]) before freeing a "
     "temporary larger reservation -- purpose of the fixed-address "
     "poke not fully traced (possibly a video buffer other code reads "
     "by convention); flagged as a loose end rather than asserted."),

    (0x11F5A, "_filbuf",
     "refill-buffer-and-return-one-char, matching OUT.EXE's _filbuf "
     "exactly in size (410 bytes) and call site (_fread+3C)."),
    (0x1260E, "_read",
     "text-mode-aware read underneath _filbuf, matching OUT.EXE's "
     "_read exactly in size (199 bytes)."),
    (0x12D66, "_lseek",
     "CRT-level seek used by _read/_write, matching OUT.EXE's _lseek "
     "exactly in size (213 bytes)."),
    (0x128F8, "_write",
     "text-mode-aware write underneath _flsbuf, matching OUT.EXE's "
     "_write exactly in size (290 bytes)."),
    (0x12B8B, "allocFileBuffer",
     "lazily allocates a stream's I/O buffer, matching OUT.EXE's "
     "allocFileBuffer exactly in size (91 bytes)."),
    (0x124F2, "findFileHandleSlot",
     "internal handle-table lookup, matching OUT.EXE's "
     "findFileHandleSlot exactly in size (73 bytes)."),
    (0x13333, "strncpy",
     "bounded copy helper, matching OUT.EXE's strncpy in role -- "
     "called from ensureFileExtension and findExecutableFile, same as "
     "OUT.EXE's equivalent cluster."),

    (0x13915, "videoDrawPoint",
     "plots a single pixel at (si=x, di=y) in color dx, directly into "
     "video memory (segment 0xB800), with separate CGA/Tandy/EGA "
     "bit-packing branches selected by videoMode. The underlying "
     "primitive beneath the already-named fillRect/drawLine."),
    (0x138F5, "videoDrawPointAlt",
     "alternate entry into videoDrawPoint's shared body: sets "
     "word_186C6=1 (videoDrawPoint's own entry sets it to 0) before "
     "falling through to the same pixel-plot code, changing how the "
     "combine step further down treats the target byte -- used by "
     "the flag-animation code (animateFlag) and the logo fade/slide "
     "sequence, consistent with drawing in an alternate (XOR/erase-"
     "style) mode for animating a sprite over existing background. "
     "Exact meaning of the mode bit not traced further."),
    (0x13A78, "buildScanlineOffsetTable",
     "precomputes the row-to-framebuffer-offset table, matching "
     "OUT.EXE's buildScanlineOffsetTable in shape exactly (same CGA "
     "interleaved-scanline vs. linear EGA/Tandy address math). Called "
     "once from init_video."),
    (0x1383B, "drawLineInternal",
     "Bresenham line rasterizer (dx/dy deltas, sign determination, "
     "error-term accumulation) -- the core drawLine calls after "
     "argument setup."),
    (0x10744, "drawLogoPixelRow",
     "scans a row of glyph/logo bitmap data for '*' marker bytes and "
     "plots each one via videoDrawPointAlt -- the ASCII-art-style "
     "renderer for fadeInLogo's title graphic."),
    (0x13EA8, "flushKeyboardBuffer",
     "checks the BIOS ROM ID byte at F000:FFFEh (0xFF = classic PC/XT); "
     "on any other (newer) BIOS, copies the BIOS keyboard buffer tail "
     "pointer (0040:001C) over the head pointer (0040:001A), which "
     "empties the type-ahead buffer -- a workaround for a BIOS "
     "quirk/bug on non-PC/XT hardware. Called from checkKeypresses."),

    (0x1134D, "divmod32",
     "full signed 32-bit division: identical shift-subtract long-"
     "division shape to OUT.EXE's divmod32 (same sign-normalization "
     "via not/neg/sbb, same 32-iteration shl/rcl comparison loop). "
     "Used by fadeInLogo's animation timing math."),
    (0x126D5, "atoi",
     "parses a decimal string (handles leading '-'/'+', accumulates "
     "digits via isdigit), returns the value and chars consumed. "
     "Called twice from formatArg -- parsing width/precision fields "
     "in a format specifier like the '5' in '%5d'."),
    (0x12D47, "isdigit", "'0'-'9' range check, used by atoi."),
    (0x12755, "formatHex",
     "hex-digit extraction via repeated shift + nibble lookup (table "
     "at 0x476C, presumably \"0123456789ABCDEF\"), then strncpy's the "
     "result out -- formatArg's %x conversion, reached via a jump "
     "table (matching playSound-style dispatch elsewhere in this "
     "codebase)."),
    (0x127E0, "formatOctal",
     "octal-digit extraction (3 bits at a time, '0'-'7'), the %o "
     "counterpart to formatHex."),
    (0x13367, "strncpy2",
     "byte-for-byte identical to the already-named strncpy (0x13333) "
     "at a different address -- a second copy from a different linked "
     "object file, not a distinct implementation. Called from "
     "hasFileExtension. Suffixed '2' since IDA requires unique global "
     "names -- 'strncpy' was already taken by 0x13333."),
    (0x12F3C, "toupper2",
     "byte-for-byte identical role to the already-named toupper "
     "(0x137FE) via a different code shape (islower-check + "
     "conditional subtract instead of a range check) -- another "
     "duplicate copy from a different object file. Suffixed '2' for "
     "the same name-uniqueness reason as strncpy2."),
    (0x13348, "islower", "'a'-'z' range check, used by the second toupper copy."),

    (0x13E49, "drawAnimatedCursor",
     "early-returns if word_188E2 (color) is 0; otherwise blits an "
     "8x8 bitmap glyph (byte_188B2, one of 4 frames selected by "
     "word_188DE) bit-by-bit via videoDrawPoint at (word_188DA, "
     "word_188DC), then advances word_188DE through a 0-3 cycle for "
     "next time. Called from checkKeypresses during the title-screen "
     "input poll -- almost certainly the blinking 'press any key' "
     "cursor/prompt indicator, though the exact glyph content wasn't "
     "decoded. The last unnamed function in ULTIMA.EXE -- "
     "100/100 (100%) once applied."),

    (0x1126F, "printStartupMessage",
     "renamed from the pre-existing 'writeString2_mb' -- confirmed "
     "this is a printf-style formatter (vsprintf + fputs to a fixed "
     "stream at 0xC73A, presumably stdout), nothing to do with multi-"
     "byte character encoding as the old name implied. Called only "
     "from checkMem and sub_104D0 for early startup diagnostic "
     "messages (e.g. low-memory warnings), not general in-game text."),
]

# (ea, new_name, note) -- labeled locations, not proc-boundary
# functions, nested inside _nheapinit's proc range for the same
# IDB-hygiene reason documented for OUT.EXE.
LOCATION_RENAMES = [
    (0x11476, "criticalErrorHandler",
     "the INT 24h ISR installed by setCriticalErrorHandler. Not "
     "recognized as its own proc by IDA (no 'sub_'/'proc' boundary), "
     "unlike OUT.EXE's equivalent which at least got a proc far "
     "boundary -- named directly as a location instead."),
    (0x123CC, "translateDosErrorToErrno",
     "the shared failure tail nearly every _dos_* primitive jumps to, "
     "translating _doserrno to a POSIX-style errno. Lives inside "
     "_nheapinit's proc range purely because of contiguous layout, "
     "exactly matching OUT.EXE's translateDosErrorToErrno situation."),
]

# (ea, new_name, note)
GLOBAL_RENAMES = [
    (0x13F41, "_doserrno",
     "set by every _dos_* primitive after a failed DOS call; matches "
     "OUT.EXE's _doserrno exactly in role."),
    (0x18460, "errno",
     "translated POSIX-style error code -- confirmed by _dos_write "
     "storing 0x1C (ENOSPC) into it on a short write, matching "
     "OUT.EXE's errno exactly."),
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
    for ea, new_name, note in LOCATION_RENAMES:
        apply_rename(ea, new_name, note)
    for ea, new_name, note in GLOBAL_RENAMES:
        apply_rename(ea, new_name, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
