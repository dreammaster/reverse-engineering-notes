"""
Names sub_16F84, called directly from `start` at program entry --
the classic DOS command-line switch parser.

Gets the PSP segment (INT 21h AH=0x51), reads the command-tail length
byte at PSP+0x80 and scans the tail text at PSP+0x81 for '/'-prefixed
switches (case-insensitive):
- "/P" (single letter) -> sets word_328C4 bit 0x8000.
- "/NOM" -> sets word_328C8 bit 2 (plausibly "no music", matching the
  BLASTER/sound-driver detection traced earlier this session).
- "/NOS" -> sets word_328C8 bit 1 (plausibly "no sound").
Any other switch letter is silently skipped (falls through to
looking for the next '/'). -> ParseCommandLineSwitches

Run via:
    .\run_ida_script.ps1 name_parse_command_line_switches.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16F84
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ParseCommandLineSwitches", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ParseCommandLineSwitches': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Scans the PSP command-tail (INT 21h AH=0x51, PSP+0x80/0x81) for "
    "'/'-prefixed switches: '/P' sets word_328C4 bit 0x8000; '/NOM' "
    "sets word_328C8 bit 2 (plausibly no-music); '/NOS' sets "
    "word_328C8 bit 1 (plausibly no-sound). Called from start.",
    False,
)
