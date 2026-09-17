"""
Round 15: resolved the real `ShowIntroPicture` while chasing
`DrawShadowedTextAlt` further (its blocker, `PlayStudioCreditsIntro`,
is still unresolved, but this is a related, adjacent find).

`sub_14F40` (round 9's confirmed-bad `PlayCreditsWipeAnimation` guess)
matches yendor2's `ShowIntroPicture` closely: draws a picture, loads a
master palette via `FileEntry_Read`/`ErrorCheck`/
`TriggerFullPaletteFadeOut` (via its own tail helper, `sub_14FDA`,
just reordered to run after the main loop instead of before), then
loops a data table of text entries (terminated `0xFFFF`) calling
`DrawStringColumn` with `PollForEscapeKeyOnly` checks and palette-fade
transitions between entries -- the exact shape of yendor2's
`ShowIntroPicture` (which achieves its "shadow text" via two manual
offset `DrawStringColumn` calls, not `DrawShadowedTextAlt` -- that
confirms `DrawShadowedTextAlt` was never used by `ShowIntroPicture`
in either game, only by `PlayStudioCreditsIntro`). Clinched by
`WaitFrameTicksOrEscape`'s inherited comment ("Called from
ShowIntroPicture") -- its only yendor3 caller is `sub_14F40`. Renamed.

`sub_14FDA` (the master-palette-reload/fade-out tail helper) left
unnamed -- its role matches ShowIntroPicture's own inlined setup code
but Chapter 3 moved it to run after the text loop instead of before,
which is a real reordering, not just a factored-out duplicate; not
confident enough yet to name it as a stand-alone function.

`PlayStudioCreditsIntro` (and therefore `DrawShadowedTextAlt`) remains
unresolved. Its caller, `sub_1BE7E`, is now known to also call the
real `ShowIntroPicture` -- a useful anchor for a future attempt at
finding it, but not chased further this round.

Run via:
    .\run_ida_script.ps1 apply_round15_findings.py
"""
import idc
import ida_name

ea = 0x14F40
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowIntroPicture", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowIntroPicture': {'ok' if ok else 'FAILED'}")
