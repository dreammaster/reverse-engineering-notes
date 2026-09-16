"""
Names sub_25AAC and sub_2ADE8, a "total party incapacitation" / reset
handler found while investigating sub_2ADE8 (a ranked naming candidate
that uses several functions named this session: StopMusicAndResetTimer,
DrawFullScreenPictureAndCacheToEMS, RestoreAndRedrawFixedStatusIcon).

sub_25AAC (0x25AAC, called from RunDungeonGameLoop and
ApplyEffectAndDrawIconBar): scans all 4 g_partySlotAssignment members;
if it finds ANY member whose +0x1C status word has NONE of bits
6/10/11/12 (0x1C40) set, it returns immediately (party still has at
least one member not in one of those conditions -- bits 10/11 are the
already-confirmed TickStatusEffects/ApplyStatusEffect timed-ailment
active flags). Only if EVERY slot is either empty or flagged with one
of those bits does it fall through to sub_2ADE8, then reset
word_31946, set up word_328C4/word_3295A flags, call RunGameDialog, and
(unless byte_2E400==0xFF) call InitializeDungeonLevel -- i.e. reload/
reset the level. Reads as "the whole party is down" -> show a screen,
then dialog, then reset. -> CheckPartyWipeAndReinitLevel

sub_2ADE8 (0x2ADE8, called only from CheckPartyWipeAndReinitLevel):
stops music, and (gated on sub_2827E) plays sound effect 0x13 via the
still-unnamed sound dispatch (sub_28412), then draws a full-screen
picture (id 1) and caches it to EMS, redraws the fixed status icon, and
shows the mouse cursor -- the actual "show this screen" step of the
party-wipe sequence. -> ShowPartyWipeScreen

Not naming sub_2827E or sub_2AE1A (their exact roles in this sequence
aren't traced) or resolving which specific afflictions bits 6/10/11/12
represent -- flagged as open in file-formats.md rather than guessed.

Run via:
    .\run_ida_script.ps1 name_party_wipe.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x25AAC, "CheckPartyWipeAndReinitLevel"),
    (0x2ADE8, "ShowPartyWipeScreen"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25AAC,
    "If every one of the 4 party slots is either empty or has one of "
    "+0x1C bits 6/10/11/12 set (bits 10/11 = the confirmed "
    "TickStatusEffects/ApplyStatusEffect timed-ailment flags) -- i.e. "
    "no member is currently unafflicted -- shows ShowPartyWipeScreen, "
    "then RunGameDialog, then InitializeDungeonLevel (unless "
    "byte_2E400==0xFF). Reads as a 'whole party incapacitated' handler. "
    "Called from RunDungeonGameLoop and ApplyEffectAndDrawIconBar.",
    False,
)
ida_bytes.set_cmt(
    0x2ADE8,
    "Stops music, plays sound effect 0x13 via the sound dispatch "
    "(sub_28412) when sub_2827E allows it, draws full-screen picture 1 "
    "and caches it to EMS, redraws the fixed status icon, and shows the "
    "mouse cursor. The 'show this screen' step of "
    "CheckPartyWipeAndReinitLevel's party-wipe sequence.",
    False,
)
