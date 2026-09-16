"""
Names sub_1559A, called once from RunCharacterCreation as its "step
3" -- previously flagged in file-formats.md as "not yet traced,"
matched only by elimination to the manual's "CHARACTER CREATION" /
"PICK A CLASS" / "MALE" / "FEMALE" / "PICK A PORTRAIT" string
cluster. A partial trace (large function, 2218 bytes, not fully
disentangled) confirms its opening structure: continues the intro
animation from PlayCharacterCreationIntroAnimation
(DrawCharacterCreationAnimationFrame in several more staged loops,
each ESC-cancelable via PollForEscapeKeyOnlyAlt), fades the palette
via StepPaletteFadeRange between stages, then draws a shadowed
5-item text list (DrawShadowedText) -- consistent with presenting the
class-selection menu. The full step likely also covers gender and
portrait selection per the manual-string cluster it's matched to,
but those sub-sections weren't individually traced.
-> RunCharacterCreationSelectionStep

Run via:
    .\run_ida_script.ps1 name_run_character_creation_selection_step.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1559A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunCharacterCreationSelectionStep", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunCharacterCreationSelectionStep': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "RunCharacterCreation's step 3: continues the intro animation, "
    "then presents the class/gender/portrait selection UI (matched "
    "to the manual's PICK A CLASS/MALE/FEMALE/PICK A PORTRAIT string "
    "cluster) -- opens with a shadowed 5-item text list, consistent "
    "with the class menu. Not fully disentangled internally. Called "
    "once from RunCharacterCreation.",
    False,
)
