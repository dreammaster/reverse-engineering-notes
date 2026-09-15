"""
Traced sub_23151, called when the global BCD counter 0x51B6 crosses a
threshold (checked in both RunDungeonGameLoop and ProcessLevelMonsters
via IsBCDCounterAtLeast) -- it's the "you have found treasure" /
experience-award screen, and reveals the correct direction of the BCD
transfers GrantMonsterRewards does (last round had this backwards --
see the correction below).

sub_23151(no params) -> ShowLootAndAwardExperience:
  - Plays a sound, sets up a text panel.
  - Prints a header, formats+draws the 0x51BA staging counter (via the
    new FormatAndDrawBCD4), then folds it into the permanent material
    counter: AddBCD4(si=0x94B3, di=0x51BA) => 0x94B3 += 0x51BA.
  - If the 0x539A staging counter is nonzero: shows a second message,
    formats+draws it, folds it in: 0x94B7 += 0x539A.
  - If the 0x5396 staging counter is nonzero: shows a third message,
    formats+draws it, folds it in: 0x94BB += 0x5396.
  - Shows a final message, formats+draws the 0x51B6 counter itself
    (the "experience/score" staging value, separate from the 3
    materials), then for every valid (non-status-1C40) party member,
    AddBCD4(si=partymember+0x18, di=0x51B6) => that member's own
    [+0x18] field += 0x51B6. New party-record field: +0x18 is
    plausibly an experience-points counter.
  - For the currently-selected member (word_328D4), if its [+0x1E]
    field is nonzero, calls sub_22445 with each party slot -- this
    turned out to be a portrait/status-icon redraw routine (draws a
    slot's party-member icon at a table-driven position, with a status
    overlay if dead/flagged), not a level-up handler as first guessed
    from the call context alone -- left unnamed since its own role
    isn't confirmed, only noted here.

sub_19B80 -> FormatAndDrawBCD4: formats a 4-byte packed-BCD value (si)
into a comma-grouped ASCII decimal string (leading zero suppressed
unless a flag forces it) and draws it via writeString -- the BCD
counterpart to FormatNumber's plain-word formatting.

CORRECTION to GrantMonsterRewards (named last round): the AddBCD4
direction was documented backwards. It's [FIXED_GLOBAL] += [monster
field], not the other way around: monster[+0x7E] -> +=0x51BA,
monster[+0x82] -> +=0x5396, monster[+0x86] -> +=0x539A,
monster[+0x8A] -> +=0x51B6. I.e. GrantMonsterRewards *stages* a dead/
expired monster's own loot fields into these 4 global counters, and
ShowLootAndAwardExperience later drains those staging counters into
the permanent material counters (0x94B3/0x94B7/0x94BB) and into each
party member's [+0x18] (XP) field.

Run via:
    .\run_ida_script.ps1 name_loot_and_xp_screen.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x23151: "ShowLootAndAwardExperience",
    0x19B80: "FormatAndDrawBCD4",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x23151,
    "Fires when the global staging counter 0x51B6 crosses a threshold "
    "(see RunDungeonGameLoop/ProcessLevelMonsters). Shows a 'treasure "
    "found' panel: drains 3 staging BCD counters (0x51BA always, "
    "0x539A and 0x5396 if nonzero) into the permanent material "
    "counters (0x94B3/0x94B7/0x94BB respectively), displaying each "
    "via FormatAndDrawBCD4. Then displays 0x51B6 itself (an "
    "experience/score staging value) and adds it into every valid "
    "party member's own [+0x18] field (plausibly XP). Also "
    "conditionally redraws party portrait icons via sub_22445 when "
    "the active member's [+0x1E] is set (role of that field/call not "
    "confirmed).",
    False,
)
ida_bytes.set_cmt(
    0x19B80,
    "FormatAndDrawBCD4(si=4-byte packed-BCD value): formats it into a "
    "comma-grouped ASCII decimal string (leading zero suppressed "
    "unless dl forces it) and draws it via writeString. The BCD "
    "counterpart to FormatNumber.",
    False,
)
ida_bytes.set_cmt(
    0x22B96,
    "Stages this monster's own loot fields into 4 global counters "
    "(CORRECTED direction from last round's comment): "
    "0x51BA += [+0x7E], 0x5396 += [+0x82], 0x539A += [+0x86], "
    "0x51B6 += [+0x8A]. ShowLootAndAwardExperience later drains "
    "these staging counters into the permanent material counters and "
    "party XP. Also adjusts two signed stat deltas ([+0x14]/[+0x16] "
    "-> sub_27A46/sub_27A2A, not traced).",
    False,
)
