"""
Names sub_179AE, called once from RunShopScreen (near the start) --
builds the shop's category tab list into an EMS-mapped scratch buffer.

Clears a 16-word buffer at 0x558A (segment word_2E4AA), then walks a
fixed 8-entry category-id table (0x5572), gating each by a bit in
byte_32DCC (shifted left per iteration -- a per-shop-type bitmask of
which of 8 categories it offers). For each enabled, nonzero category,
stores [category_id, value] pairs into the buffer (4 bytes/entry):
category ids 1/2/3 get a fixed global value (word_32DE2/word_32DE4/
word_32DE6 respectively -- plausibly special currency-like tabs, e.g.
gold/ore/nuore exchange), while other categories load the category's
own catalog record and, if catalog flag [+0xC] bit 0x100 and the
record's own [+2] bit 1 are both set, pull the value from the
record's [+4] field.
-> BuildShopCategoryTabList

Run via:
    .\run_ida_script.ps1 name_build_shop_category_list.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x179AE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "BuildShopCategoryTabList", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'BuildShopCategoryTabList': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears the 0x558A/word_2E4AA scratch buffer, then walks the "
    "8-entry category table (0x5572) gated by byte_32DCC's bits, "
    "storing [category_id, value] pairs -- ids 1/2/3 get fixed "
    "globals (word_32DE2/32DE4/32DE6), others pull from their own "
    "catalog record's [+4] field when eligible. Called from "
    "RunShopScreen.",
    False,
)
