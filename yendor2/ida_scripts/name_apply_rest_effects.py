"""
Names sub_1E943, called from RestPartyAndAdvanceClock (the "R rest"
command's per-tick processing): the per-character rest-tick handler.

Skips entirely if the character is incapacitated (+0x1C bits 0x1C40).
Otherwise copies a block of fields (+0x72..+0xB2 into +0x32..+0x72,
46 bytes) whose exact purpose isn't fully mapped, then branches:

- If any of +0x1C bits 0xE380 (SICK/JINXED/DISEASED/CURSED and others)
  are set: clears the SICK (0x8000) and JINXED (0x200) flags outright;
  if DISEASED (0x2000), drains HP (+0x52) by 0x24 per rest tick, and if
  it reaches 0 or below, sets the DEAD flag (0x40) and zeroes HP; else
  if CURSED (0x80), drains MP (+0x54) by 0x30 per tick, floored at 0.
- Otherwise (no active affliction from that set): applies the normal
  percentage-based HP and MP regen (matching RestCharacter's own
  `(max*rate+50)/100 + current`, capped at max, formula) to both stats.

Reads as "apply this rest tick's effects to one character: either
affliction-driven degradation, or normal regeneration."
-> ApplyRestEffectsToCharacter

Run via:
    .\run_ida_script.ps1 name_apply_rest_effects.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E943
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyRestEffectsToCharacter", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyRestEffectsToCharacter': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Per-character rest-tick handler: skips if incapacitated (+0x1C "
    "0x1C40). If DISEASED/CURSED (among other +0x1C bits, 0xE380), "
    "drains HP or MP instead of regenerating (DISEASED HP loss can set "
    "the DEAD flag at 0); otherwise applies normal percentage-based "
    "HP/MP regen. Called from RestPartyAndAdvanceClock.",
    False,
)
