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
