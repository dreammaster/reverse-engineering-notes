"""
Names the last 4 entries of the pre-existing ErrorTable jump table
(indices 16-19, DATA XREF from seg102:0361/0363/0365/0367) --
sub_28A19/sub_28A1F/sub_28A25/sub_28A2B.

Each does the same thing as their earlier, real-message siblings
(mov ax, X; jmp ErrorExit) but with a critical difference: X here is
a small raw value (0x281, 0x285, 0x289, 0x289) rather than a real
string offset. Every other ErrorTable entry uses `offset aXxx`
pointing into the message-string block starting near aMemoryAllocati
(actual address ~0x28715+) -- these tiny values are two orders of
magnitude too small to be valid pointers into that same block, and
the last two entries share the exact same value (breaking the +4
per-entry progression the first three suggest). This strongly looks
like incomplete/vestigial table entries -- reserved error-code slots
that never got real message text, quite possibly related to content
stripped from this shareware build. Named modestly, without inventing
message content that isn't there.

-> ErrorExitCode281 / ErrorExitCode285 / ErrorExitCode289 / ErrorExitCode289Alt

Run via:
    .\run_ida_script.ps1 name_error_table_stub_entries.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x28A19: "ErrorExitCode281",
    0x28A1F: "ErrorExitCode285",
    0x28A25: "ErrorExitCode289",
    0x28A2B: "ErrorExitCode289Alt",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

comment = (
    "ErrorTable slot with ax set to a raw small value (not a real "
    "'offset aXxx' string pointer like its siblings -- too small to "
    "address the message-string block near aMemoryAllocati), before "
    "jmp ErrorExit. Plausibly a vestigial/incomplete error-code slot, "
    "not confirmed to ever be triggered."
)
for ea in names:
    ida_bytes.set_cmt(ea, comment, False)
