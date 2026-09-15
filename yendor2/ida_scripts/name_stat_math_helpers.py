"""
Traced the two small arithmetic helpers UseTrainingItem's class-growth
formulas lean on, both clean and widely reused.

- sub_25A66 -> ScaleByPercentRounded(ax=value, bx=percent):
  ax = (ax*bx + 50) / 100 -- round-to-nearest percentage scale.

- sub_1AA53 -> AddToStatCapped(ax=delta, bx=field offset): adds ax to
  word_328D4's [bx] field, clamped at a type-appropriate cap -- 9999
  for HP/MP fields (+0x52/+0x92/+0x54/+0x94), 999 for everything else.
  Requires the field to already be nonzero (errorCode=2 if not, i.e.
  can't grow an uninitialized stat); errorCode=1 if the add had to be
  clamped, 0 if it applied cleanly.

Run via:
    .\run_ida_script.ps1 name_stat_math_helpers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x25A66: "ScaleByPercentRounded",
    0x1AA53: "AddToStatCapped",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25A66,
    "ScaleByPercentRounded(ax=value, bx=percent): ax = (ax*bx+50)/100.",
    False,
)
ida_bytes.set_cmt(
    0x1AA53,
    "AddToStatCapped(ax=delta, bx=field offset on word_328D4): "
    "[word_328D4+bx] += ax, clamped at 9999 for HP/MP fields "
    "(+0x52/+0x92/+0x54/+0x94) or 999 otherwise. errorCode: 2 if the "
    "field was 0 (uninitialized, not applied), 1 if clamped, 0 if "
    "applied cleanly.",
    False,
)
