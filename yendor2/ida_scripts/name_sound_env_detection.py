"""
Names a 3-function cluster behind sound hardware auto-detection,
called from InitSoundSystem.

sub_2D578 (called twice from sub_28619): the classic DOS environment-
variable lookup. Gets the PSP segment (INT 21h AH=0x51), reads the
environment segment pointer at PSP+0x2C, then scans the
null-terminated, double-null-terminated environment block comparing
each entry against the caller-supplied search string (ds:bp) via
`repe cmpsb`; on a match, copies the value portion (after the match)
into the caller's buffer (es:dx) and returns ax=1, else ax=0.
-> FindEnvironmentVariable

sub_28619 (called once from InitSoundSystem): uses
FindEnvironmentVariable twice against two different env-var name
strings (constants at 0xCC1 and 0xCB8 -- their exact text wasn't
independently re-verified against the nearby BLASTER=/SOUND=/FMDRV/
EMMXXXX0 string cluster this investigation surfaced, but the parsing
logic below is unambiguous), setting g_driverStateFlags bits
(0x8000 if the first lookup fails, 0xC000 if the second fails or its
value doesn't parse) as fallback/default states. On a successful
second lookup, scans the returned value for 'A'/'a' and parses the
following 3 characters as a base-address-like number into
word_32916, then scans for 'I'/'i' and parses 1 digit into
word_32914, setting word_36CE3/word_36CE1 to 1 -- exactly the
"BLASTER=A220 I5 D1 T3" Sound Blaster environment-variable format
(base address + IRQ). -> ParseSoundBlasterEnvironmentVariable

sub_2827E (called from several unnamed sites plus sub_2D498): checks
g_driverStateFlags bit 0x8; if clear, returns immediately (ax=1). If
set, busy-waits until word_2E494 becomes 0, then returns ax=0 -- a
"wait for the sound driver to finish its current operation" gate.
-> WaitForSoundDriverIdle

Run via:
    .\run_ida_script.ps1 name_sound_env_detection.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2D578: "FindEnvironmentVariable",
    0x28619: "ParseSoundBlasterEnvironmentVariable",
    0x2827E: "WaitForSoundDriverIdle",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2D578,
    "DOS environment-variable lookup: PSP via INT 21h AH=0x51, env "
    "segment from PSP+0x2C, scans the env block for a match against "
    "ds:bp (caller's search string), copies the value to es:dx. "
    "ax=1 found / ax=0 not found. Called from "
    "ParseSoundBlasterEnvironmentVariable.",
    False,
)
ida_bytes.set_cmt(
    0x28619,
    "Looks up two env vars via FindEnvironmentVariable (name "
    "constants at 0xCC1/0xCB8, near the BLASTER=/SOUND=/EMMXXXX0/"
    "FMDRV string cluster; exact text not independently pinned down). "
    "On success, parses 'A<3 digits>' into word_32916 and "
    "'I<1 digit>' into word_32914 -- the classic BLASTER=A220 I5 D1 "
    "T3 format -- setting word_36CE3/word_36CE1=1. Sets "
    "g_driverStateFlags fallback bits (0x8000/0xC000) on lookup/parse "
    "failure. Called from InitSoundSystem.",
    False,
)
ida_bytes.set_cmt(
    0x2827E,
    "Returns immediately (ax=1) if g_driverStateFlags bit 0x8 is "
    "clear; otherwise busy-waits for word_2E494 to become 0 then "
    "returns ax=0 -- waits for the sound driver's current operation "
    "to finish. Called from several sites including sub_2D498.",
    False,
)
