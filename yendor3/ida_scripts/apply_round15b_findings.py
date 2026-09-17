"""
Round 15 continued: confirmed `RunTitleScreen` at last.

This was BinDiff's own suggested address all along (0x1BE7E, 0.88
similarity) -- round 4 deliberately left it unrenamed as "too
large/noisy to trust without a dedicated read." Doing that dedicated
read now (prompted by tracing `ShowIntroPicture`'s caller while
chasing `DrawShadowedTextAlt`) confirms it: a direct diff shows the
same ~20-call core sequence in the same order in both games
(`InitializeNewGameWorldState`, `DrawPicture`, `ShowIntroPicture`
(now correctly resolved, round 15), `RunCharacterCreation`,
`UpdateAmbientMusic`, `ShowPartyMembers`, `ShowWorldMap`,
`UpdatePartyAverageStatTiers`, and both `ShowIntroPicture`/
`RunCharacterCreation` called a second time later -- matching exactly).
Confirmed via `ConfirmNewGame`'s call to it too (already an
established, correctly-named function). Renamed.

Bonus: the diff shows yendor3's version calling `~PlayTitleScreenSequence`
(the still-unrenamed 0x20DC4, 0.39 similarity BinDiff guess) as a new
call -- consistent with that guess being correct after all, though not
independently verified this round.

Run via:
    .\run_ida_script.ps1 apply_round15b_findings.py
"""
import idc
import ida_name

ea = 0x1BE7E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunTitleScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunTitleScreen': {'ok' if ok else 'FAILED'}")
