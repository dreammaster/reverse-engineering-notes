"""
Names sub_20C1E and sub_20C46, RenderDungeonViewport's two direct
callers (both called from `start`, several sites each) -- the
"redraw the dungeon screen" pair.

sub_20C1E (-> RedrawDungeonScreen): the fuller variant -- calls
sub_21306, sub_213FC, sub_2784A, sub_20D2F, sub_20C8E, then
RenderDungeonViewport, then conditionally ShowResourceDepletedOverlay
(word_36C7F bit 0x1000).

sub_20C46 (-> RefreshDungeonScreen): a lighter variant -- skips
sub_21306/sub_213FC, otherwise the same (sub_2784A, sub_20D2F,
sub_20C8E, RenderDungeonViewport, conditional
ShowResourceDepletedOverlay), plus an extra conditional DrawMinimap
(word_36C7F bit 0x4000).

Exact distinction between the two (e.g. "on movement" vs "on partial
state change") not traced -- named by their relative call-list scope.
Shared callees (sub_21306, sub_213FC, sub_2784A, sub_20D2F, sub_20C8E)
left unnamed.

Run via:
    .\run_ida_script.ps1 name_dungeon_screen_redraw.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20C1E: "RedrawDungeonScreen",
    0x20C46: "RefreshDungeonScreen",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20C1E,
    "Fuller dungeon-screen redraw: sub_21306/sub_213FC/sub_2784A/"
    "sub_20D2F/sub_20C8E setup, then RenderDungeonViewport, then "
    "conditional ShowResourceDepletedOverlay. Called from `start`. "
    "Sibling of the lighter RefreshDungeonScreen.",
    False,
)
ida_bytes.set_cmt(
    0x20C46,
    "Lighter dungeon-screen redraw (skips sub_21306/sub_213FC vs. "
    "RedrawDungeonScreen): sub_2784A/sub_20D2F/sub_20C8E, "
    "RenderDungeonViewport, conditional ShowResourceDepletedOverlay, "
    "plus a conditional DrawMinimap. Called from `start`.",
    False,
)
