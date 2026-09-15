"""
Names sub_19264, reached from sub_1869D's main keyboard-input loop
(the top-level dispatcher on byte_2E400, the pressed key) when Space
is pressed while word_31946 (a "currently carrying an item" flag) is
nonzero and word_328C6 bit 0x10 is set -- one of 3 sibling bits
(0x10/8/4) on word_328C6 gating 3 different Space-bar actions,
plausibly per-tile/location action flags for wherever the party is
standing.

Mechanism (fully traced):
- Checks whether the held item's type bitmask (es:[bx+0x10], es:bx =
  word_2E54C:word_2E54E, the held item) overlaps the standing
  location's accepted-type bitmask ([bx2+0x10], bx2 = word_2E546) --
  `test [bx2+10h], ax`.
- On no overlap: shows a 4-line rejection message via sub_23B76(bx=
  0x7FF7, cx=4) -- dumped raw bytes confirm the text is
  "I HAVE NO NEED FOR THAT TYPE OF ITEM." (a shopkeeper/NPC-voiced
  line -- the same message table also holds the alchemy-conversion
  strings "YOU MUST HAVE AT LEAST 10 UNITS" / "IT WILL COST _ GOLD" /
  "IS THAT PRICE AGREEABLE?", confirming this is the same
  trade/conversion message bank documented for the ore-conversion
  alchemy ability).
- On overlap (accepted): sets word_328CA bit 0x20, snapshots the held
  item's fields (word_31948/3194C -> word_32968/3296A) and the
  location's fields (word_32920/32922 -> word_32964/32966) presumably
  for the HUD popup, clears the held-item slot (word_31948/3194A/3194C
  = 0), then commits: `AddBCD4([si=0x94B3], [di=0x50C0=word_32920])`
  -- adds the location's BCD4 amount (word_32920) into the global
  material counter at DS:0x94B3 (already documented as a "third,
  sibling counter... not otherwise identified yet" in
  docs/file-formats.md) -- then calls ShowMaterialCounterHud.

Reads as: hand over a held item at a material-conversion station; if
its type doesn't match what's accepted here, get rebuffed with "I have
no need for that type of item"; if it does, the item is consumed and
its value is added to material counter 0x94B3. Exact narrative (what
kind of station, what 0x94B3 represents) still not identified -- only
the mechanism is confirmed. -> TryConvertItemToMaterial

Run via:
    .\run_ida_script.ps1 name_convert_item_to_material.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19264
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryConvertItemToMaterial", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryConvertItemToMaterial': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Space-bar action (sub_1869D, word_328C6 bit 0x10) while carrying "
    "an item (word_31946): if the held item's type mask (es:[bx+0x10]) "
    "doesn't overlap the standing location's accepted-type mask "
    "([word_2E546+0x10]), shows 'I HAVE NO NEED FOR THAT TYPE OF "
    "ITEM.' (msg 0x7FF7). Otherwise consumes the held item and does "
    "AddBCD4([0x94B3], [word_32920]) -- adds the location's amount "
    "into the global material counter 0x94B3 -- then "
    "ShowMaterialCounterHud. What kind of station/material this is "
    "not identified.",
    False,
)
