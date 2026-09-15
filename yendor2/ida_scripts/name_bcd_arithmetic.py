"""
Traced CastSpell's still-open 0x1C branch (flagged in a prior session
as "reads as an offensive/damage spell ... not fully traced") and found
it's actually a materials-transmutation ability, not damage: dumped its
message strings directly from the data segment (ds:7D7E/802B/7D2C/8D8C,
ds = word_2D86) and got "YOUR SKILL IS NOT HIGH ENOUGH!",
"YOU MUST HAVE AT LEAST 10 UNITS.", "[[4sp]]NUORE CREATED" and
"[[4sp]]MAGIC ORE CREATED." -- a two-recipe crafting ability (letter
'5'/'7' confirm-prompt answers select which of two materials converts
into the other), gated on a per-target skill byte and a >=10-unit
resource check.

That resource check (and the add/subtract that follows a successful
craft) led to a genuinely new, well-defined, and widely-reused
subsystem: 4-byte (8-digit) packed-BCD bignum arithmetic, not specific
to crafting -- these are called from combat/inventory code
(sub_17032, sub_147FF, sub_1E61B, HandleMovementInput, etc.) all over
the executable, almost certainly the engine behind gold/currency and
other large counters. Confirmed via the actual x86 opcodes used
(DAA/DAS -- decimal-adjust after add/subtract -- only make sense for
packed BCD):

- sub_19AB3: converts a 16-bit binary value (word_3293E, reused as a
  generic scratch parameter -- not renamed) into 4-byte packed BCD,
  written to ds:0xAFA8 (word_38808/word_3880A -- also a generic
  scratch pair used for unrelated things elsewhere, not renamed).
  -> ConvertWordToBCD4
- sub_19A56: raw 4-byte (8-nibble) packed-BCD comparison of [si] vs
  [di], most-significant-digit first, each nibble pair compared with
  `and ax,0F0F0h`/`0F0Fh` + cmp; exits at the first mismatch, so the
  final CF reflects whether [si] < [di] (standard multi-digit
  comparison order) -- returns CF=1 if [si] is the smaller value.
  -> CompareBCD4
- sub_19A7C: wraps CompareBCD4 -- converts word_3293E to BCD via
  ConvertWordToBCD4, then compares a caller-supplied BCD counter (si)
  against it. Callers use `jnb` on the result, i.e. "counter >=
  threshold". -> IsBCDCounterAtLeast
- sub_19A16: raw 4-byte packed-BCD addition, [si] += [di], via
  DAA-adjusted byte adds from least to most significant byte.
  -> AddBCD4
- sub_19A3C: wraps AddBCD4 -- converts word_3293E to BCD, adds it into
  a caller-supplied BCD counter (si). -> AddToBCDCounter
- sub_19C7B: raw 4-byte packed-BCD subtraction, [si] -= [di], via
  DAS-adjusted byte subtracts. -> SubBCD4
- sub_19DFB: wraps SubBCD4 -- converts word_3293E to BCD, subtracts it
  from a caller-supplied BCD counter (si). -> SubtractFromBCDCounter

Run via:
    .\run_ida_script.ps1 name_bcd_arithmetic.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x19AB3: "ConvertWordToBCD4",
    0x19A56: "CompareBCD4",
    0x19A7C: "IsBCDCounterAtLeast",
    0x19A16: "AddBCD4",
    0x19A3C: "AddToBCDCounter",
    0x19C7B: "SubBCD4",
    0x19DFB: "SubtractFromBCDCounter",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19AB3,
    "Converts a 16-bit binary value (word_3293E) into 4-byte packed "
    "BCD, written to ds:0xAFA8 (word_38808/word_3880A -- a generic "
    "scratch pair reused for unrelated things elsewhere).",
    False,
)
ida_bytes.set_cmt(
    0x19A56,
    "Raw 4-byte packed-BCD comparison, [si] vs [di], most-significant "
    "digit first (matches CompareBCD4/IsBCDCounterAtLeast usage). "
    "Exits at the first mismatching nibble; CF=1 if [si] < [di].",
    False,
)
ida_bytes.set_cmt(
    0x19A7C,
    "IsBCDCounterAtLeast(si=BCD counter, word_3293E=threshold): "
    "converts word_3293E via ConvertWordToBCD4, then CompareBCD4 "
    "against the counter at si. Callers use jnb on the result to mean "
    "'counter >= threshold'.",
    False,
)
ida_bytes.set_cmt(
    0x19A16,
    "Raw 4-byte packed-BCD addition: [si] += [di], DAA-adjusted, "
    "least-significant byte first with carry propagation.",
    False,
)
ida_bytes.set_cmt(
    0x19A3C,
    "AddToBCDCounter(si=BCD counter, word_3293E=amount): converts "
    "word_3293E via ConvertWordToBCD4, then AddBCD4 into the counter "
    "at si.",
    False,
)
ida_bytes.set_cmt(
    0x19C7B,
    "Raw 4-byte packed-BCD subtraction: [si] -= [di], DAS-adjusted, "
    "least-significant byte first with borrow propagation.",
    False,
)
ida_bytes.set_cmt(
    0x19DFB,
    "SubtractFromBCDCounter(si=BCD counter, word_3293E=amount): "
    "converts word_3293E via ConvertWordToBCD4, then SubBCD4 from the "
    "counter at si.",
    False,
)
