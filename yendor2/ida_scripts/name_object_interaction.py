"""
Resolves both loose ends flagged last round -- sub_294A3
(RunGameDialog's unresolved 0x242-0x245 handler) and sub_2D60A
(HandleGameCommand's other fallback) both scan the same table at
DS:0xDFBB, now decoded:

  0xDFBB table: 0x16 (22)-byte entries, terminated by 0xFFFF at +0:
    +0x0  word   object type (matched against the target's [+4] field)
    +0x2  word   pointer to a per-object-type "known/unlocked" flags byte
    +0x4  word   bitmask to test/set in that flags byte
    +0x8  word   the specific command code required to unlock it

- sub_294A3: calls ProbeFacingTile to find what the player is facing;
  if it's flagged interactive ([+2] bit 0x2000), looks up its type in
  the 0xDFBB table. If the matching capability bit is already set,
  shows a success message. If not, but the *current* command
  (word_32974) matches the table's required command for it, sets the
  bit (permanently "learns"/"unlocks" it for that object type) and
  shows the success message; otherwise shows a different (fail/hint)
  message. A discovery mechanic: try commands on objects until you
  find (and permanently learn) the right one. -> UseAbilityOnTarget
- sub_2D60A: same table lookup on the *currently targeted* object
  (word_2E548, HandleGameCommand's fallback path), but instead of
  trying to unlock anything, just branches on whether the capability is
  already known: known -> one message (sub_29461), not known/not in the
  table at all -> a generic description (sub_1A3F0). Looks like the
  "examine/look at" counterpart to UseAbilityOnTarget's "try it".
  -> ExamineTarget

Run via:
    .\run_ida_script.ps1 name_object_interaction.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x294A3: "UseAbilityOnTarget",
    0x2D60A: "ExamineTarget",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

DS_BASE = 0x2D860
ida_bytes.set_cmt(
    DS_BASE + 0xDFBB,
    "Object-type capability table, 0x16-byte entries (0xFFFF at +0 "
    "terminates): +0 object type, +2 ptr to a per-type 'known/unlocked' "
    "flags byte, +4 bitmask, +8 the command code required to unlock it. "
    "Read by UseAbilityOnTarget (tries to unlock via the current "
    "command) and ExamineTarget (just checks whether it's already "
    "known).",
    False,
)
ida_bytes.set_cmt(
    0x294A3,
    "Discovery mechanic: ProbeFacingTile finds what the player faces; "
    "if interactive, looks it up in the 0xDFBB capability table. "
    "Already-known capability -> success message. Not known but the "
    "current command matches what's required -> sets the bit "
    "(permanently unlocks it for that object type) and shows success. "
    "Otherwise shows a fail/hint message. Try commands on objects until "
    "you find the right one.",
    False,
)
ida_bytes.set_cmt(
    0x2D60A,
    "Looks up the currently-targeted object (word_2E548) in the 0xDFBB "
    "capability table; if its capability is already known, shows one "
    "message (sub_29461), otherwise (or if not in the table at all) "
    "shows a generic description (sub_1A3F0). The 'examine' counterpart "
    "to UseAbilityOnTarget's 'try it'.",
    False,
)
