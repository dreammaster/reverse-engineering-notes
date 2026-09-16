"""
Names sub_25862 and sub_2587E -- thin, extremely widely-used
wrappers around StepPaletteFadeRange for a full (cx=0x100) palette
fade.

sub_25862 (88 refs, e.g. ShowClueBook): clears word_3295A bit 0x1000,
then calls StepPaletteFadeRange(ax=0, cx=0x100) -- mode 0 snapshots
the current palette and begins fading the whole thing down toward
black. -> TriggerFullPaletteFadeOut

sub_2587E (36 refs, e.g. ShowClueBook): calls
StepPaletteFadeRange(ax=1, cx=0x100) -- an untraced mode, paired with
setting word_3295A bit 0x1000 afterward (the same bit
TriggerFullPaletteFadeOut clears), consistent with marking a fade-in
initiated/ready state. -> TriggerFullPaletteFadeIn

Run via:
    .\run_ida_script.ps1 name_trigger_palette_fade_wrappers.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25862
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TriggerFullPaletteFadeOut", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TriggerFullPaletteFadeOut': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "Clears word_3295A bit 0x1000, then StepPaletteFadeRange(ax=0, "
    "cx=0x100) -- snapshots and begins fading the whole palette down "
    "toward black. Called very widely (88 refs).",
    False,
)

ea = 0x2587E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TriggerFullPaletteFadeIn", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TriggerFullPaletteFadeIn': {'ok' if ok else 'FAILED'}")
ida_bytes.set_cmt(
    ea,
    "StepPaletteFadeRange(ax=1, cx=0x100, an untraced mode), then "
    "sets word_3295A bit 0x1000 -- the counterpart to "
    "TriggerFullPaletteFadeOut. Called widely (36 refs).",
    False,
)
