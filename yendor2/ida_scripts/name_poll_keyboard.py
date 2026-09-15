"""
Names the core non-blocking keyboard-input poll, found by tracing
sub_1305E's wait loop: INT 21h/AH=6/DL=0xFF (DOS direct console I/O,
"any key?") twice in sequence -- first call gets a regular character
(uppercased if a-z, stored in byte_2E400, sets the "event type" global
to 1) or, if that returns no key, a second call for an extended/
function-key scan code (stored in byte_2E400, event type 2, with a
special case calling sub_10C40 for scan code 'B' unless a suppression
flag is set). No key at all leaves the event type at 0 (set by this
same function on entry). -> PollKeyboardInput

IMPORTANT: the "event type" global here is the same one named
`errorCode` earlier this session from its role in ErrorCheck/ErrorExit
(0x32930) -- it's reused for a completely different purpose by this
input subsystem (0=no input, 1=char, 2=extended key), not an actual
error code in this context. Noted via comment rather than renamed,
since the ErrorCheck/ErrorExit usage is the more common/primary one and
was independently verified from the reapplied prior annotations.

Also names sub_1305E, which loops calling PollKeyboardInput (with a
Fade? each iteration) until it returns a non-zero event, then handles
event types 1-3 via sub_14D26. -> WaitForKeypress

Run via:
    .\run_ida_script.ps1 name_poll_keyboard.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1D038: "PollKeyboardInput",
    0x1305E: "WaitForKeypress",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1D038,
    "Non-blocking keyboard poll via INT 21h/AH=6/DL=0xFF. Sets "
    "errorCode (reused here as an input-event-type flag, NOT an actual "
    "error code: 0=no input, 1=regular char in byte_2E400 (uppercased "
    "a-z), 2=extended/function-key scan code in byte_2E400). Scan code "
    "'B' triggers sub_10C40 unless word_328CA bit3 is set.",
    False,
)
ida_bytes.set_cmt(
    0x1305E,
    "Loops calling PollKeyboardInput (with a Fade? each iteration) "
    "until a key event is seen (errorCode != 0 as an input-event flag, "
    "see PollKeyboardInput), then for event types 1-3 calls sub_14D26.",
    False,
)
