"""
Traced sub_2704C -- the "depleted" hook called both from
SpendMaterialCounterClamped (when a BCD material counter can't cover a
cost) and directly from DrawMinimap (when word_36C7F bit 0x1000 is
set, blanking the dungeon view). Its body does three things:

1. Sets word_36C7F bit 0x1000 (confirms/re-asserts the "blanked view"
   state regardless of which caller triggered it) and blits a
   58-row x 74-byte region (row stride 0x140=320, matching a normal
   VGA scanline) from an EMS-mapped page frame onto the video buffer
   -- an overlay image drawn over the minimap's screen area.

2. Checks all 3 material BCD counters (si starts at 0x94B3, stride 4
   -- this confirms 0x94B3/0x94B7/0x94BB, already known individually
   from the alchemy/cost-dispatch system, are exactly consecutive
   4-byte BCD counters) via IsBCDCounterAtLeast(threshold=0), and
   builds a small indicator array at 0x950D: 1/2/3 (a per-material id)
   if that counter is nonzero, 0 if it's exactly empty.

3. Draws a fixed banner icon (g_pictureDir entry 6) plus, via the new
   DrawResourceStatusIcons (was sub_27441), a row of per-resource
   status icons at a small position table (0x636C) for whichever ids
   in that array are nonzero.

So this is a unified "you're out of something" overlay: whichever
resource triggered it (light/torch fuel, going by DrawMinimap's use,
or a material counter, going by SpendMaterialCounterClamped's use), it
redraws a blackout-style overlay over the dungeon view AND refreshes
the status icons for all 3 material counters. -> ShowResourceDepletedOverlay

sub_27441 is a small generic helper: for cx entries, if the id at [si]
is nonzero, looks it up (sub_12554, not traced) and draws its icon at
the matching (x,y) from a position table. -> DrawResourceStatusIcons
(the 0x950D indicator array and 0x636C position table are left
unnamed -- sub_2704C's caller only populates 3 of the 9 entries
DrawResourceStatusIcons reads, so the other 6 are populated somewhere
not yet traced).

Run via:
    .\run_ida_script.ps1 name_resource_depleted_overlay.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2704C: "ShowResourceDepletedOverlay",
    0x27441: "DrawResourceStatusIcons",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2704C,
    "Unified 'resource depleted' overlay, called both when a material "
    "BCD counter can't cover a cost (SpendMaterialCounterClamped) and "
    "when the dungeon view itself is blanked (DrawMinimap, "
    "word_36C7F bit 0x1000). Sets that bit, blits a fixed overlay "
    "image (EMS page frame -> video buffer) over the minimap's screen "
    "area, checks all 3 material BCD counters (0x94B3/0x94B7/0x94BB, "
    "confirmed consecutive, stride 4) and builds a small per-material "
    "'nonzero' indicator array, then draws a banner icon and the "
    "material status icons via DrawResourceStatusIcons.",
    False,
)
ida_bytes.set_cmt(
    0x27441,
    "For cx entries: if the id at [si] is nonzero, looks it up "
    "(sub_12554) and draws its icon at the matching (x,y) from a "
    "position table at di (stride 0xA: x at +0, y at +4).",
    False,
)
