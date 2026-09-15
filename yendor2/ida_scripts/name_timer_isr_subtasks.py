"""
Traced AdvanceGameClock's actual trigger -- a real INT 1Ch timer
interrupt service routine (18.2 Hz hardware tick, ending in `iret`,
paired with the already-named RestoreInt1cVector). This confirms the
game's clock/lighting/ailment systems are wall-clock-driven, not tied
to player movement or turns.

The ISR multiplexes 5 independent periodic sub-tasks, each gated by
its own word_3295A bit and its own countdown reload value:
  word_3294C -> sub_1FD17 (bit 0x8000)
  word_3294E -> AdvanceGameClock (bit 0x4000, already named)
  word_32950 -> AdvanceDayNightPaletteFade (bit 0x2000, already named
                -- confirms the 113-step fade is driven by repeated
                ISR calls, not a single burst)
  word_32952 -> sub_1FECF (bit 0x1000)
  word_32958 -> (bit 0x200) inline: clears the bit and sets
                word_328CA bit 0x10 when it lapses (not a call)

Named the two remaining, cleanly-bounded sub-tasks this round:

sub_1FD17 -> TickRedrawTimer: reloads its own countdown from
word_36CE7 and flags a redraw (word_328C4 |= 0x400) -- a periodic
"mark the screen dirty" tick at a configurable rate.

sub_1FECF -> AnimatePaletteCycle: dispatches on word_328CE (0-3) and
shuffles small color-table chunks between 0x4C0C and 0x472A --
classic palette-cycling animation (torch flicker / water shimmer
style), not fully decoded but confidently an animation-cycle effect.

The raw ISR entry itself is left unnamed -- it's embedded in bytes
IDA hasn't cleanly separated from a preceding data declaration
(byte_1F986), so forcing a name/function boundary onto it risks
corrupting already-good disassembly; the dispatch structure above is
documented instead.

Run via:
    .\run_ida_script.ps1 name_timer_isr_subtasks.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1FD17: "TickRedrawTimer",
    0x1FECF: "AnimatePaletteCycle",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1FD17,
    "Timer-ISR sub-task (word_3294C, word_3295A bit 0x8000): reloads "
    "its own countdown from word_36CE7 and flags a redraw "
    "(word_328C4 |= 0x400).",
    False,
)
ida_bytes.set_cmt(
    0x1FECF,
    "Timer-ISR sub-task (word_32952, word_3295A bit 0x1000): "
    "dispatches on word_328CE (0-3) and shuffles small color-table "
    "chunks between 0x4C0C and 0x472A -- a palette-cycling animation "
    "effect (torch flicker / water shimmer style), not fully decoded.",
    False,
)
