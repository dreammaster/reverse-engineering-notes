"""
Names sub_16DAA, called from sub_16881: selects which of two possible
trap effect ids (a record's [+0x6C] primary, [+0x6E] alternate) to
apply. If flag bit 0x400 ([+0xC]) is set, or there's no alternate
([+0x6E]==0), always uses the primary. Otherwise rolls
RandomInRange(100): a 25% chance (roll < 0x19) uses the alternate
effect instead (and sets word_328CA bit 0x200), else the primary.
Either way, resolves the chosen id via PrepareTrapEffectSlots. Reads
as "usually trigger the primary trap effect, but a 1-in-4 chance of a
different one" -- part of the trap/effect-definition system.
-> SelectTrapEffectVariant

Run via:
    .\run_ida_script.ps1 name_select_trap_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16DAA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectTrapEffectVariant", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectTrapEffectVariant': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Picks between a record's primary ([+0x6C]) and alternate "
    "([+0x6E]) trap effect id: always primary if flag [+0xC] bit "
    "0x400 is set or no alternate exists, else a 25% chance "
    "(RandomInRange(100)<0x19) of the alternate. Resolves the chosen "
    "id via PrepareTrapEffectSlots. Called from sub_16881.",
    False,
)
