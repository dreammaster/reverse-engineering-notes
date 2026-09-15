"""
Traced the actual melee-attack resolution code, reached from a mouse-
click handler in sub_16407 (RunDungeonGameLoop's input handler):
clicking a monster panel's icon sets word_32A1E to that monster
(matching SelectActiveMonster's role from the turn-order system);
clicking an attack hotspot then calls into the attack roll.

sub_25A73(ax=target defense, bx=attacker accuracy stat, cx=weapon
damage power) -> ResolveAttack: if cx==0, or (bx-ax) is negative, or a
RandomInRange(0x37=55) roll beats (bx-ax), the attack misses --
word_2E49C is set to 0. Otherwise it hits: word_2E49C =
(cx*(bx-ax)+50)/100, minimum 1. Callers check word_2E49C for 0 (miss,
plays a miss sound) vs. nonzero (hit, calls the damage-application
step). Formula: hit if (accuracy-defense) >= random(0-54); damage =
weaponPower * (accuracy-defense) / 100.

sub_286B2(di=target monster record) -> UpdateMonsterWoundTier: compares
word_2E49C (the damage just dealt) against 10% and 30% of [di+0x50]
(plausibly the monster's max HP/toughness), setting an escalating
wound-severity flag in [di+0xE] (0x8000 light, 0x4000 moderate, 0x2000
severe), plus an unconditional display/animation flag in [di+0xC]
(|= 0xA). Notably this does NOT subtract from any HP counter directly
-- it reads as a purely visual wound-tier indicator; actual monster
death/HP tracking (if numeric at all, vs. a tiered state machine) is
handled elsewhere, not traced this round.

Run via:
    .\run_ida_script.ps1 name_combat_attack.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x25A73: "ResolveAttack",
    0x286B2: "UpdateMonsterWoundTier",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25A73,
    "ResolveAttack(ax=target defense, bx=attacker accuracy, "
    "cx=weapon damage power): miss (word_2E49C=0) if cx==0, if "
    "bx<ax, or if RandomInRange(55) beats (bx-ax). Otherwise hit: "
    "word_2E49C = (cx*(bx-ax)+50)/100, minimum 1.",
    False,
)
ida_bytes.set_cmt(
    0x286B2,
    "UpdateMonsterWoundTier(di=target monster record): compares "
    "word_2E49C (damage just dealt by ResolveAttack) against 10% and "
    "30% of [di+0x50] (plausibly max HP/toughness), setting an "
    "escalating wound-severity flag in [di+0xE] (0x8000 light, "
    "0x4000 moderate, 0x2000 severe) plus a display flag in [di+0xC] "
    "(|=0xA). Doesn't subtract HP directly -- purely a visual "
    "wound-tier indicator as far as traced; actual death/HP tracking "
    "not found yet.",
    False,
)
