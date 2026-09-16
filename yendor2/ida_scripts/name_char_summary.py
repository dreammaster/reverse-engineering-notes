"""
Names sub_25103, moderate confidence: the last ShowPartyMembers
pipeline step. Reuses the same message pointers ShowCharacterStats
does (0x7A11 header, 0x8572 line) alongside others, drawing several
single/multi-line groups (1+1+1+1+6+1 = 11 lines) -- reads as a
condensed recap/overview combining info from the other per-character
screens rather than fresh content. -> ShowCharacterSummary

Run via:
    .\run_ida_script.ps1 name_char_summary.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25103
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCharacterSummary", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCharacterSummary': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Moderate confidence: last ShowPartyMembers pipeline step. Reuses "
    "message pointers ShowCharacterStats also uses (0x7A11, 0x8572) "
    "alongside others -- reads as a condensed recap/overview screen "
    "rather than fresh content.",
    False,
)
