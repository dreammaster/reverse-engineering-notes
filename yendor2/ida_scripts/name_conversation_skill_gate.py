"""
Traced sub_2B9D4, called before every RunConversation topic-display
branch. It's a skill-gated response-quality classifier: word_2E548's
own [+2] bits (8/4/2) select one of 4 threshold ladders (each 4
values, 5 apart: 0x41/0x46/0x4B/0x50, then 0x4B/0x50/0x55/0x5A, then
0x55/0x5A/0x5F/0x69), and the current party member's [+0x6E] field
(a new party-record field, plausibly a charisma/persuasion-like skill,
adjacent to [+0x6C]'s lockpicking/perception role from ShowLockStatus)
is compared against them. If it doesn't clear even the lowest
threshold for the selected ladder, sets word_328C4 bit 0x20 (and
fontOffset=0) -- presumably "minimal/no response". Otherwise sets one
of bits 2/4/8/0x10 depending on which band it lands in -- a 4-tier
"how much does the NPC reveal" gate.

Named on its confirmed mechanism; the exact in-universe stat name for
[+0x6E] isn't confirmed.

-> ClassifyConversationSkillTier

Run via:
    .\run_ida_script.ps1 name_conversation_skill_gate.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B9D4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClassifyConversationSkillTier", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClassifyConversationSkillTier': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Skill-gated response-quality classifier, called before every "
    "RunConversation topic display. word_2E548's own [+2] bits "
    "(8/4/2) select one of 4 threshold ladders; the current party "
    "member's [+0x6E] (plausibly charisma/persuasion, adjacent to "
    "[+0x6C]'s lockpicking/perception role) is compared against them. "
    "Below the lowest threshold: word_328C4 bit 0x20 (minimal "
    "response). Otherwise: bit 2/4/8/0x10 depending on the band -- a "
    "4-tier 'how much the NPC reveals' gate.",
    False,
)
