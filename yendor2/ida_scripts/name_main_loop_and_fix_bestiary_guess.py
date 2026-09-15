"""
Traced sub_234D3's caller chain up to find out what it actually shows.
sub_232A8 calls it exactly 3 times with 3 FIXED addresses (0x51C0,
0x525C, 0x52F8 -- stride 0x9C/156 bytes apart) at 3 fixed screen
y-positions (0x57/0x7B/0x9F, 36px apart, x=0xF1 -- right edge of
screen) -- not a dynamic per-monster/per-item list, so the earlier
"plausibly a bestiary/identify mechanic" guess for sub_234D3 doesn't
fit well (a bestiary would be a dynamic list, not exactly 3 fixed
slots). Correcting that hedge to something more accurate: 3 fixed
status/info widgets, content still unclear.

More importantly, sub_232A8 turned out to be called from inside
sub_162F6 -- which is called directly from `start` right after setup
and loops on itself (jmp back to its own body) calling movement input,
full redraws, and resource-depletion checks every iteration. This is
the main dungeon game loop. -> RunDungeonGameLoop

Also noted in passing: right after the loop's per-iteration work,
sub_162F6 checks IsBCDCounterAtLeast(si=0x51B6, threshold=0) -- a BCD
counter sitting immediately before sub_232A8's first fixed record
(0x51C0 = 0x51B6+10), conditionally calling sub_23151 if it's nonzero.
Not traced further this round.

Run via:
    .\run_ida_script.ps1 name_main_loop_and_fix_bestiary_guess.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x162F6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunDungeonGameLoop", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunDungeonGameLoop': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Main dungeon game loop, called once from `start`. Each iteration: "
    "checks movement/menu input (sub_16407/sub_16881/sub_25AAC), "
    "redraws (sub_20C1E, BuildMinimapTileData, DrawMinimap), shows a "
    "resource-depleted overlay if needed, draws 3 fixed status/info "
    "widgets via sub_232A8, and checks a BCD counter at 0x51B6 to "
    "conditionally call sub_23151. Loops via jmp back to its own body "
    "until byte_2E400 signals exit.",
    False,
)
