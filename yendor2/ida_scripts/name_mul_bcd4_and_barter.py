"""
Names two functions found investigating sub_1CCBC (a ranked naming
candidate, called from PayGoldAndAcquireItem and SellClickedCatalogItem
-- the shop/vendor cluster):

sub_19CA1 (0x19CA1): decomposes a packed-BCD4 value at [si] into its 8
decimal digits, and for each digit (weighted by place value via
repeated BCD addition through AddToBCDCounter/AddBCD4, the already-named
BCD4 arithmetic library) multiplies by a 16-bit word (word_32940) and
accumulates, writing the resulting BCD4 product back to the original
[si]. I.e. "packed-BCD4 value * word -> packed-BCD4 result", a sibling
of ConvertWordToBCD4/CompareBCD4/AddBCD4/SubBCD4. The per-digit "+50"
constant's exact purpose (rounding for a later /100?) isn't confirmed.
-> MulBCD4ByWord

sub_1CCBC (0x1CCBC): computes a tiered discount/markup percentage from
a party record field, [bx+0x68] (not otherwise identified yet --
plausibly the BARTERING skill/stat, given the shop context and the
already-found attribute/skill list that includes BARTERING), via 7
descending thresholds (0x36/0x40/0x4F/0x64/0x7C/0x95/0x3E7 -> 0x37/
0x2D/0x23/0x19/0xF/0x8/0x2). Uses that percentage with MulBCD4ByWord to
preview a scaled sell/buy price, then -- gated on
IsItemEligibleForEnhance / IsItemEligibleForRepair -- loads item catalog
records and computes further scaled preview prices for the enhance/
repair costs. -> ComputeBarterPricingPreview

Run via:
    .\run_ida_script.ps1 name_mul_bcd4_and_barter.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x19CA1, "MulBCD4ByWord"),
    (0x1CCBC, "ComputeBarterPricingPreview"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19CA1,
    "Multiplies a packed-BCD4 value at [si] by a 16-bit word "
    "(word_32940), digit-by-digit via repeated BCD addition "
    "(AddToBCDCounter/AddBCD4), writing the BCD4 product back to [si]. "
    "A MulBCD4-style sibling of ConvertWordToBCD4/CompareBCD4/AddBCD4/ "
    "SubBCD4. Called (twice each) from ComputeBarterPricingPreview.",
    False,
)
ida_bytes.set_cmt(
    0x1CCBC,
    "Computes a tiered discount/markup percentage from party record "
    "field +0x68 (plausibly the BARTERING skill/stat) via 7 descending "
    "thresholds, applies it via MulBCD4ByWord to preview a scaled sell/ "
    "buy price, then -- gated on IsItemEligibleForEnhance / "
    "IsItemEligibleForRepair -- computes further scaled preview prices "
    "for enhance/repair costs. Called from PayGoldAndAcquireItem and "
    "SellClickedCatalogItem.",
    False,
)
