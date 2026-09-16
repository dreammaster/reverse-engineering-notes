"""
Round 8 of global-variable renaming: focused on likely structures and
the biggest multi-purpose bitfields, per Paul's request to push into
those specifically rather than single-function scratch values.

**The clue-book pagination struct, completed.** Rounds 4/5 named
g_clueBookCategory/g_clueEntryScrollOffset/g_clueEntrySelectedIndex;
fresh reads of RunClueEntryMenu's scroll/page handlers confirm the
remaining 4 fields of the same state cluster:
- word_2E3EA -> g_clueEntryLowerBound: compared against 0 as an
  "active list loaded?" sentinel, and used as the lower clamp bound
  (word_2E3EA+2) when paging up.
- word_2E3EC -> g_clueEntryVisibleRowCount: how many entries fit on
  screen (defaults to 0xE, or a smaller count on a partial last page).
- word_2E3F2 -> g_clueEntryPageUpperBound: g_clueEntryScrollOffset+0x34,
  clamped to the total count -- the current page's upper bound.
- word_2E3F4 -> g_clueEntryTotalCount: the current category's total
  entry count, read once on category entry.

**The lock-state pair**, read directly from LoadLockState and its many
callers: word_32DC8 -> g_lockUnlockedMask (a bit-packed "previously
unlocked?" mask freshly loaded per lock/object id from CURGAME) and
byte_32DCD -> g_lockUnlockedAccumulator (consistently OR'd/tested with
the same low byte right after loading g_lockUnlockedMask, then written
back to CURGAME -- a running accumulator of which locks in this id
group have been unlocked).

**word_32DC4 -> g_currentToolbarIconPtr**: selects between two fixed
small records (0x6ED0/0x6EE8) for the SHOOT/CAST action-toolbar icons;
`[bx+2]` is consistently incremented/decremented as a charge/uses
counter across both the main dispatcher's icon-click handling and
UseAbilityCommand.

**The big multi-purpose flag words (word_328C4/C6/C8/CA/CC).**
Enumerated every `and`/`or`/`test` bit-mask constant used against each
across the whole binary. Four of the five are confirmed, genuine
grab-bags -- the SAME bit value is reused for unrelated purposes
depending on which mutually-exclusive screen is active (e.g.
word_328C6 bit 0 gates DrawPicture's masked-blit prep, nothing to do
with its shop-mode/portrait-dirty bits elsewhere in the same word) --
so they get honest, generic names rather than a falsely-specific one,
with every confirmed bit meaning catalogued in a comment on each:
- word_328C4 -> g_uiScratchFlags1
- word_328C6 -> g_uiScratchFlags2
- word_328C8 -> g_uiScratchFlags3
- word_328CA -> g_uiScratchFlags4
The fifth, word_328CC, is different: essentially every confirmed bit
is specifically about the clue book's category nav bar (per-tab
highlight bits, LIST/MAP hint toggles), so it gets a real name:
- word_328CC -> g_clueBookNavFlags

Run via:
    .\run_ida_script.ps1 rename_globals_round8.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = [
    (0x2E3EA, "g_clueEntryLowerBound"),
    (0x2E3EC, "g_clueEntryVisibleRowCount"),
    (0x2E3F2, "g_clueEntryPageUpperBound"),
    (0x2E3F4, "g_clueEntryTotalCount"),
    (0x32DC8, "g_lockUnlockedMask"),
    (0x32DCD, "g_lockUnlockedAccumulator"),
    (0x32DC4, "g_currentToolbarIconPtr"),
    (0x328C4, "g_uiScratchFlags1"),
    (0x328C6, "g_uiScratchFlags2"),
    (0x328C8, "g_uiScratchFlags3"),
    (0x328CA, "g_uiScratchFlags4"),
    (0x328CC, "g_clueBookNavFlags"),
]

COMMENTS = {
    0x328C4: (
        "General-purpose UI/game-state scratch flags word, reused by many "
        "unrelated, mutually-exclusive-screen systems -- not one coherent "
        "mode register. Confirmed bits: 0x1=full local-area-map render "
        "toggle (HandleGameCommand 0x1E / RunMapEditorScreen); "
        "0x2/0x4/0x8/0x10/0x20=DrawIndentedTextColumn wrap-mode selector, "
        "the SAME bits also gate RunGameDialog's pause-menu SAVE/LOAD/"
        "NEWGAME/DOS/ANIMATION label suppression and (separately) a "
        "4-tier NPC conversation response-detail gate; 0x100=widespread "
        "'redraw needed' dirty flag; 0x200=roster-cleanup 'something "
        "changed' flag (ShowWorldMap exit); 0x400='tick ready', a "
        "periodic timer/vsync flag consumed by many independent "
        "busy-wait call sites (ShowIntroPicture, character-creation "
        "intro, WaitForTickFlagAndClear, debug overlays); 0x800/0x1000 "
        "(tested together as 0x1800)=ailment icon-bar periodic-timer "
        "gate; 0x2000=debug HUD toggle; 0x8000=command-line /P switch. "
        "Other bit combinations are OR'd/AND'd at additional call sites "
        "not individually traced."
    ),
    0x328C6: (
        "General-purpose UI scratch flags word, mostly party-inventory/"
        "shop-screen action and portrait-dirty bits but NOT exclusively "
        "-- e.g. bit 0 gates DrawPicture's masked-blit-mask-expand prep, "
        "unrelated to its shop bits. Confirmed bits: 0x1=masked-blit "
        "toggle (DrawPicture and friends); 0x2=an AnimateProjectileStep "
        "variant selector in ApplyEncodedItemEffect; 0x4=RunRepairItemScreen "
        "action mode (gates TryRepairItemForGold in the inventory main "
        "loop); 0x8=RunEnhanceItemScreen action mode (gates "
        "TryEnhanceItemForGold); 0x10=RunSellItemScreen action mode "
        "(gates TrySellItemForGold); 0x20=UseItem's key-item dispatch "
        "(item type value doubles as a lock/door id) and (tested with "
        "0x80 as 0x1Ch) an item-use context hint; 0x40=BuildItemUseMessage "
        "'could afford this special cost' success flag; 0x200=shop-has-"
        "nothing-to-show flag; 0x800/0x1000/0x2000/0x4000 (tested "
        "together as 0x7800)=portrait-dirty bits gating a cached-EMS "
        "background restore. Other bit combinations are OR'd/AND'd at "
        "additional call sites not individually traced."
    ),
    0x328C8: (
        "General-purpose scratch flags word spanning combat, item-use, "
        "palette-fade, and audio-setting systems -- not one coherent "
        "mode register. Confirmed bits: 0x1=command-line /NOS (no sound) "
        "switch; 0x2=command-line /NOM (no music) switch; 0x10=dungeon-"
        "movement-input fast-exit gate; 0x20=ApplyEncodedItemEffect's "
        "bitmask-switch item/spell effect dispatcher entry; 0x100=combat-"
        "action mode selector (set = ranged/thrown-weapon attack request, "
        "clear = spell/ability cast); 0x800=palette-fade-in-progress flag, "
        "reused across ShowIntroPicture/RunCharacterCreationSelectionStep/"
        "the credits intro; 0x2000/0x4000/0x8000=ConsumeItemChargeResource's "
        "caller-configured consumption-mode bits (recharge+reset-wear / "
        "full discard / swap-effect-then-discard). Other bit combinations "
        "are OR'd/AND'd at additional call sites not individually traced."
    ),
    0x328CA: (
        "General-purpose scratch flags word. Confirmed bits: 0x2="
        "'registered version' flag (unconditionally set at boot; the "
        "shareware-demo boundary check it guards is dead code in this "
        "binary); 0x4=local-area-map cell draw mode (TryInteractAtPosition "
        "special-object check vs. plain stored tile); 0x400/0x800=clue-book/"
        "alchemy-screen pagination scroll-arrow indicators; 0x1000=the "
        "widely-read 'in formal (row-based) combat' flag, gating the "
        "combat-action dispatcher's melee-vs-ranged/ability branch, the "
        "compass HUD, the unlock-door handler, and a debug HUD overlay. "
        "Other bit combinations are OR'd/AND'd at additional call sites "
        "not individually traced."
    ),
}

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

for ea, text in COMMENTS.items():
    ida_bytes.set_cmt(ea, text, False)
    print(f"{ea:#x}  comment set ({len(text)} chars)")
