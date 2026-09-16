"""
Names the top-level driver and resolution/presentation function for
the "side trap" system this session uncovered
(TriggerSideTrapForRandomPartyMember/RollTrapAvoidanceMagnitude).

sub_2278C, called directly from `start` at several points (likely
after each movement step): fast-exits (just redraws the mouse cursor)
unless word_328C8 bit 0x10 is set. Otherwise clears a 4-entry, 0x18-
byte-stride scratch table at 0xBC28 (segment word_2E4AA) plus some
related globals, then walks an 80-entry wall/cell table (si=0xF26,
stride 0x9C) calling TriggerSideTrapForRandomPartyMember for every
entry whose [+0xE] bit 0x1000 ("has a side trap") is set, before
calling sub_2281F to resolve and present whatever triggered.
-> ProcessSideTrapsOnMovement

sub_2281F, called once at the end of the above: a rich resolution/
presentation sequence for up to 4 triggered trap results staged in
the 0xBC28 table. Plays the trap's sound cue once the driver is idle
(via still-unnamed sub_28412); if word_328C8 bit 0x8 is set, draws a
weapon-style icon per triggered entry (DrawWeaponSelectIcon) then does
a full dungeon-screen refresh (redraw + minimap + EMS re-caches); picks
the entry with the highest "severity" value ([di+0xA]) to select a
starting frame for a scaled projectile-style animation
(AnimateProjectileStep, called once for the picked severity then twice
more unconditionally); finally, if word_328C8 bit 0x10 is still set,
transfers each triggered entry into the confirmed icon-bar slot table
(0xC50) -- computing an effect-definition pointer into a 12-byte-
stride table at 0x905B -- and calls ApplyEffectAndDrawIconBar to apply
and render them. -> PresentTriggeredSideTrapEffects

Run via:
    .\run_ida_script.ps1 name_side_trap_pipeline.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2278C: "ProcessSideTrapsOnMovement",
    0x2281F: "PresentTriggeredSideTrapEffects",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2278C,
    "Fast-exits unless word_328C8 bit 0x10 is set. Otherwise clears "
    "the 0xBC28 trap-result scratch table (4 entries, stride 0x18) "
    "and walks 80 wall/cell records (0xF26, stride 0x9C) calling "
    "TriggerSideTrapForRandomPartyMember for every one flagged with a "
    "side trap ([+0xE] bit 0x1000), then calls "
    "PresentTriggeredSideTrapEffects. Called from start.",
    False,
)
ida_bytes.set_cmt(
    0x2281F,
    "Resolves/presents up to 4 triggered side-trap results from the "
    "0xBC28 table: sound cue once the driver is idle, weapon-style "
    "icons + full dungeon-screen refresh (word_328C8 bit 0x8), a "
    "severity-scaled projectile animation, and (word_328C8 bit 0x10) "
    "a transfer into the confirmed icon-bar slot table (0xC50) via "
    "ApplyEffectAndDrawIconBar. Called from "
    "ProcessSideTrapsOnMovement.",
    False,
)
