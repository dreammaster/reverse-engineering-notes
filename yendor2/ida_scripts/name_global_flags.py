"""
Traced sub_27A98, sub_27A2A, sub_27A46 -- the two callees
GrantMonsterRewards uses for its signed [+0x14]/[+0x16] stat deltas
(left untraced two rounds ago). They're a global boolean flag-bit
accessor family, backed by a bitfield array at 0x94D1:

- sub_27A98 -> GetGlobalFlagBitAndWord(ax=flag index): classic
  "linear bit index -> (word, mask)" split (word = index/16, bit =
  index%16, MSB-first within the word). Returns si = the word's
  address (0x94D1 + word*2), ax = that bit's mask. -> used by all
  three of the functions below.
- sub_27A2A -> ClearGlobalFlag(ax=flag index): [si] &= ~mask.
- sub_27A46 -> SetGlobalFlag(ax=flag index): [si] |= mask.
- sub_27A5E -> TestGlobalFlag(ax=flag index): ZF = ([si] & mask) == 0.
  Called directly from `start` (multiple sites) -- confirms this is a
  fundamental, widely-used quest/world-state flag system, not
  something local to monster kills.

So GrantMonsterRewards' [+0x14]/[+0x16] signed deltas are actually
flag indices: a negative value clears that global flag, a positive
value sets it, on monster death -- i.e. killing certain monsters can
set or clear arbitrary quest/world-state flags (e.g. "boss defeated").

A closely related but DIFFERENT family (sub_27A6E/sub_27A3E and
sub_27AC1, offsetting from the caller's own si by +0x10C or +0xCA
respectively rather than a fixed global base) manipulates *per-object*
flag banks instead of this global one -- not named this round, a
good next lead (multiple offset constants suggest more than one
record type has its own flag bank).

Run via:
    .\run_ida_script.ps1 name_global_flags.py
"""
import idc
import ida_name
import ida_bytes

FUNC_RENAMES = {
    0x27A98: "GetGlobalFlagBitAndWord",
    0x27A2A: "ClearGlobalFlag",
    0x27A46: "SetGlobalFlag",
    0x27A5E: "TestGlobalFlag",
}

for ea, name in FUNC_RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ds_base = 0x2D860
data_ea = ds_base + 0x94D1
old = idc.get_name(data_ea)
ok = ida_name.set_name(data_ea, "g_globalFlags", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{data_ea:#x}  {old!r} -> 'g_globalFlags': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x27A98,
    "GetGlobalFlagBitAndWord(ax=flag index): si = g_globalFlags + "
    "(index/16)*2, ax = bit mask for index%16 (MSB-first). Shared by "
    "SetGlobalFlag/ClearGlobalFlag/TestGlobalFlag.",
    False,
)
ida_bytes.set_cmt(0x27A2A, "ClearGlobalFlag(ax=flag index): [si] &= ~mask.", False)
ida_bytes.set_cmt(0x27A46, "SetGlobalFlag(ax=flag index): [si] |= mask.", False)
ida_bytes.set_cmt(
    0x27A5E,
    "TestGlobalFlag(ax=flag index): ZF = ([si] & mask) == 0. Called "
    "directly from `start` at several points -- a fundamental quest/"
    "world-state flag system.",
    False,
)
ida_bytes.set_cmt(
    data_ea,
    "Global boolean flag bitfield (quest/world-state flags), accessed "
    "via SetGlobalFlag/ClearGlobalFlag/TestGlobalFlag/"
    "GetGlobalFlagBitAndWord. GrantMonsterRewards sets/clears "
    "specific flags on monster death via its [+0x14]/[+0x16] signed "
    "flag-index fields.",
    False,
)

ida_bytes.set_cmt(
    0x22B96,
    "Stages this monster's own loot fields into 4 global counters: "
    "0x51BA += [+0x7E], 0x5396 += [+0x82], 0x539A += [+0x86], "
    "0x51B6 += [+0x8A] (drained later by ShowLootAndAwardExperience). "
    "Also applies two signed global-flag-index deltas, [+0x14]/[+0x16] "
    "(negative clears, positive sets, via ClearGlobalFlag/"
    "SetGlobalFlag) -- e.g. this monster's death can set/clear an "
    "arbitrary quest/world-state flag.",
    False,
)
