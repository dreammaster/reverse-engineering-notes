"""
Names the clue-book "ability info" overlay cluster used by
RunClueBookItemDetailWithAbilityInfo.

sub_14876 (-> DrawLabeledNumberIfNonzero): writeString(bx=label) then,
if [si] (a plain integer, not packed-BCD) is nonzero, formats and
draws it via FormatNumber. The plain-integer sibling of
DrawLabeledBCDIfNonzero. Also called from sub_13780.

sub_1381C (-> ShowItemEffectDuration): draws "DURATION-" (msg 0x8B45)
plus a scaled value (word_2E548's [+4] field x10) via
DrawLabeledNumberIfNonzero, then "MINUTES" (msg 0x8B4F, confirmed by
message dump) -- shows how long a scroll/potion's timed effect lasts.

sub_13957 (-> ShowItemAbilityEffectInfo): shows either a
percent-chance line (word_2E54A, choosing between two labels
depending on a value at [si]) or, when word_2E548's [+0] bit 0 is
set, an effect-amount line whose value is selected by word_32974
(0x19/0x1A/0x1B/_val46) -- confirmed to use the *exact same* damage
constants (0x3C/0x23/0x32/0x28) as ResolveAbilityEffect's own
dispatch, i.e. this literally shows the same numbers the ability
actually uses in combat. 163 lines total; only the confirmed high-
level role and the constant cross-reference are established, not
every branch.

Run via:
    .\run_ida_script.ps1 name_ability_overlay_cluster.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x14876: "DrawLabeledNumberIfNonzero",
    0x1381C: "ShowItemEffectDuration",
    0x13957: "ShowItemAbilityEffectInfo",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x14876,
    "Draws a label then a plain integer (not packed-BCD) via "
    "FormatNumber, only if [si] is nonzero. Plain-integer sibling of "
    "DrawLabeledBCDIfNonzero. Called from ShowClueBookItemDetail, "
    "ShowItemEffectDuration, ShowItemAbilityEffectInfo, and sub_13780.",
    False,
)
ida_bytes.set_cmt(
    0x1381C,
    "Shows 'DURATION- <n> MINUTES' for a scroll/potion's timed effect "
    "(value from word_2E548's [+4] field x10). Called from "
    "RunClueBookItemDetailWithAbilityInfo.",
    False,
)
ida_bytes.set_cmt(
    0x13957,
    "Shows the clue-book description of an item's granted ability "
    "effect: either a percent-chance line, or an effect-amount line "
    "whose value (selected by word_32974) uses the exact same damage "
    "constants as ResolveAbilityEffect's own dispatch -- i.e. shows "
    "the real numbers the ability uses in combat. Called from "
    "RunClueBookItemDetailWithAbilityInfo.",
    False,
)
