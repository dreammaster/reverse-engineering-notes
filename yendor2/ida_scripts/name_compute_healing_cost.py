"""
Names sub_1BA35, called twice from UseHealingItem -- sums a per-
affliction cost contribution over the confirmed party-record +0x1C
affliction bitfield.

For the current party member (bx=word_328D4), adds to ax for each
active affliction bit: SICK (0x8000) +5, POISONED (0x4000) +10,
DISEASED (0x2000) +20, PARALYZED (0x1000) +40, FROZEN (0x800) +50,
STONED (0x400) +60, JINXED (0x200) +20, HEXED (0x100) +30, CURSED
(0x80) +40. Returns the total in ax. In short: computes the combined
healing/curing cost (or difficulty) for this character's currently
active afflictions, one term per affliction -- ties directly into the
already-named ShowHealingCostPrompt.
-> ComputeAfflictionHealingCost

Run via:
    .\run_ida_script.ps1 name_compute_healing_cost.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1BA35
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeAfflictionHealingCost", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeAfflictionHealingCost': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Sums a per-affliction cost over [bx+0x1C] (SICK+5/POISONED+10/"
    "DISEASED+20/PARALYZED+40/FROZEN+50/STONED+60/JINXED+20/HEXED+30/"
    "CURSED+40), bx=word_328D4. Returns total in ax -- the combined "
    "healing/curing cost for this character's active afflictions. "
    "Called from UseHealingItem.",
    False,
)
