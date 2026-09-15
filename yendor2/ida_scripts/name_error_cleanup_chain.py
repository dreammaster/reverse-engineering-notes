"""
Names the three cleanup helpers ErrorExit unconditionally calls before
tearing everything down, plus two globals in that chain. Read directly
(see dump_range.py transcripts from this session) rather than guessed;
re-confirmed after fix_ds_segreg.py made the ds:-relative operands in
this function resolve to their real global names instead of raw hex
offsets:

- sub_1FC21: if a custom INT 1Ch (timer tick) vector was installed
  (dword_1F97C non-null), restores the original one via
  INT 21h/AH=25h, and clears a bit in word_3295A. -> RestoreInt1cVector
- sub_29B06: ES = _videoBufferSeg (already-named global); INT 21h/AH=49h
  (free memory) on it. -> FreeVideoBuffer
- sub_2838F: tests driver-active bits in a flags word (off_36CE5); for
  each active driver, sends shutdown-style commands through its
  function-pointer dispatch table then frees its memory block via
  INT 21h/AH=49h. First driver's dispatch pointer is a genuine far
  pointer at 0x3CC78 (offset word) / 0x3CC7A (segment word, reused
  directly as the ES to free) -- typed unk_/word_ because the two
  halves aren't merged into one item. -> ShutdownAudioDrivers
- errorCode (already named, at 0x32930 -- confirms the reapplied prior
  annotations included it, just unresolvable/invisible in disassembly
  until the DS-segreg fix): the pending error code, set by callers
  before jumping into ErrorTable via ErrorCheck, read back by ErrorExit
  as the DOS exit code. No rename needed, already good.
- off_36CE5: driver-active/capability flags word tested throughout this
  function and sub_283EA. -> g_driverStateFlags
- unk_3CC78: far pointer (dword) to the first active driver's command
  entry point; called with bx = command number (8, 9 here).
  -> g_soundDriverFarPtr

Run via:
    .\run_ida_script.ps1 name_error_cleanup_chain.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1FC21: "RestoreInt1cVector",
    0x29B06: "FreeVideoBuffer",
    0x2838F: "ShutdownAudioDrivers",
    0x36CE5: "g_driverStateFlags",
    0x3CC78: "g_soundDriverFarPtr",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1FC21,
    "Restores the original INT 1Ch (timer tick) vector if this session "
    "installed a custom one (dword_1F97C != 0), via INT 21h/AH=25h. "
    "Called unconditionally from ErrorExit before exiting.",
    False,
)
ida_bytes.set_cmt(
    0x29B06,
    "Frees the video buffer segment (_videoBufferSeg) via INT 21h/AH=49h. "
    "Called unconditionally from ErrorExit before exiting.",
    False,
)
ida_bytes.set_cmt(
    0x2838F,
    "Shuts down whichever audio driver(s) are currently active (per "
    "g_driverStateFlags), sending shutdown-style commands through each "
    "driver's function-pointer dispatch table before freeing its memory "
    "block via INT 21h/AH=49h. Called unconditionally from ErrorExit "
    "before exiting.",
    False,
)
ida_bytes.set_cmt(
    0x3CC7A,
    "Segment half of the far pointer g_soundDriverFarPtr (0x3CC78); "
    "reused directly as the ES segment to free when shutting the driver "
    "down.",
    False,
)
ida_bytes.set_cmt(
    0x2899C,
    "Generic error/exit path: runs the RestoreInt1cVector / "
    "FreeVideoBuffer / ShutdownAudioDrivers cleanup trio, then (if the "
    "mouse/video subsystem flag ds:40FCh bit0 is set) resets the mouse "
    "driver and video mode 3, prints the DOS '$'-terminated string at "
    "DS:AX (set by the ErrorTable handler that jumped here), and exits "
    "via INT 21h/AH=4Ch with errorCode as the exit code.",
    False,
)
