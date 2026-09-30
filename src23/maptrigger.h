#ifndef YENDOR23_MAPTRIGGER_H
#define YENDOR23_MAPTRIGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd4.h"
#include "game.h"
#include "item.h"
#include "savegame.h"

/*
 * IsPositionInTriggerList's own table (both games) plus
 * ApplyMapTriggerEffect's dispatch (yendor2.asm:17458,
 * yendor3.asm:19629, confirmed instruction-identical except one real,
 * currently-dormant Chapter 3 addition noted below) -- called once
 * per movement step for the current cell. See file-formats.md's
 * "ApplyMapTriggerEffect" section for the full decode this
 * reimplements; tables extracted via
 * ida_scripts/dump_trigger_list_table.py, one script per game.
 *
 * Both real tables are tiny (9 entries Chapter 2, 19 Chapter 3) and,
 * cross-checked entry by entry, only ever exercise 3 of the 6 possible
 * dispatch branches with real data: the fully data-driven default, the
 * corrosion branch (one Chapter 2 entry), and teleport (not modeled
 * here -- pure UI orchestration, see below). The gold/ore-theft and
 * ailment-tick branches, and Chapter 3's own extra item-exclusion
 * check on the default branch, are real code paths but aren't
 * exercised by either game's actual data today.
 */

unsigned mapTriggerCount(GameKind game);

typedef struct {
    int coord;      /* the x-or-y value to match */
    bool matchIsX;   /* true: compare against worldX; false: compare against worldY (the record's own +2 bit 0x8000) */
    uint16_t flags;  /* raw +2 -- selects which branch applies, see mapTriggerDecide */
    int16_t rawA;    /* +4 -- meaning depends on flags: teleport destX (not modeled) / gold-ore theft amount's
                         high BCD digit pair / corrosion's own party-record slot offset / the default branch's
                         own data-driven effect id */
    int16_t rawB;    /* +6 -- teleport destY (not modeled) / gold-ore theft amount's low BCD digit pair /
                         unused by corrosion / unused by the default branch (the original never populates it
                         there, matching IsRestingAllowedHere-style "confirmed by omission" findings elsewhere
                         in this project) */
} MapTriggerRecord;

/* First matching entry (linear scan, matching the original's own first-match-wins order), false if none. */
bool mapTriggerFind(GameKind game, int worldX, int worldY, MapTriggerRecord *out);

typedef enum {
    MapTriggerNone,          /* flags bit 0x4000 (teleport) or 0x2000 (ailment tick) -- deliberately not
                                 decided further here: teleport is pure UI-driving orchestration (matching
                                 TravelToDestination's own established scope boundary), and the ailment-tick
                                 system itself isn't reimplemented anywhere in this project yet */
    MapTriggerApplyEffect,   /* apply effectId to every eligible (non-incapacitated) party member --
                                 see mapTriggerApplyEffect */
    MapTriggerApplyCorrosion /* apply equipment corrosion at slotOffset to every eligible party member that
                                 has an item equipped there -- see mapTriggerApplyCorrosion */
} MapTriggerOutcome;

typedef struct {
    MapTriggerOutcome outcome;
    unsigned effectId;            /* valid when outcome == MapTriggerApplyEffect */
    unsigned slotOffset;          /* valid when outcome == MapTriggerApplyCorrosion -- a raw party-record byte offset */
    uint16_t corrosionModeFlags;  /* valid when outcome == MapTriggerApplyCorrosion -- effectGetDef's own
                                      modeFlags for the resolved corrosion effect id (destroy, id 1, if the
                                      matched record's own flags bit 0x200 is clear; replace, id 0x2B, if
                                      set), for mapTriggerApplyCorrosion to pass straight to
                                      partyHandleIconBarItemExpiry */
    bool requiresItemExclusion;   /* valid when outcome == MapTriggerApplyEffect -- Chapter 3 only (the
                                      matched record's own flags bit 0x1): skip a party member who has item
                                      0x275 equipped at equipment code 0x13's slot (party-record offset
                                      0x158). A real, confirmed Chapter 3-only addition the default branch
                                      alone can carry -- Chapter 2 never sets this bit. Genuinely exercised
                                      by real Chapter 3 data (4 of the game's 19 real trigger entries),
                                      not a dormant/unreachable case. */
} MapTriggerDecision;

/*
 * The fixed effect ids (0xF/0x10/0x11, selected by flags bits
 * 0x1000/0x800/0x400) are gold/ore-theft traps, not HP/MP damage --
 * confirmed via their own EffectCostGold/Ore1/Ore2 costFlags in the
 * already-embedded effect table (`effect.c`). This is why they alone
 * (among the ApplyEffect-outcome branches) carry a meaningful rawB:
 * their "amount" is a 4-byte packed BCD value split across rawA (high
 * digit pair)/rawB (low digit pair), matching
 * ResolveAttackerActionOutcome's own already-established gold-theft
 * convention -- not modeled as a separate outcome since
 * mapTriggerApplyEffect already handles the split via effectId's own
 * costFlags. The fully data-driven default branch (no flags bits set
 * at all) only ever supplies rawA as a plain magnitude; real data
 * confirms rawB is always 0 there and never read by
 * ApplyMapTriggerEffect's own default-branch code.
 */
MapTriggerDecision mapTriggerDecide(GameKind game, const MapTriggerRecord *record);

/*
 * Applies a MapTriggerApplyEffect decision to one eligible (non-
 * incapacitated -- caller's own job to check first, matching this
 * project's other per-member apply functions, e.g.
 * combatApplyEncodedItemEffectParty) party member. A no-op if
 * requiresItemExclusion is set and this member has item 0x275
 * equipped at party-record offset 0x158 (see MapTriggerDecision's own
 * doc comment).
 */
void mapTriggerApplyEffect(uint8_t *partyRecord, SaveGame *save, GameKind game, unsigned effectId, int16_t rawA,
                            int16_t rawB, bool requiresItemExclusion);

/*
 * Applies a MapTriggerApplyCorrosion decision to one eligible party
 * member -- a no-op if slotOffset's own equipment slot is empty, or if
 * the equipped item doesn't classify for corrosion at all (both
 * matching ResolveAttackerActionOutcome's own equipment-corrosion
 * eligibility chain exactly, see combat.c's
 * combatResolveAttackerAction).
 */
void mapTriggerApplyCorrosion(uint8_t *partyRecord, const ItemCatalog *catalog, GameKind game,
                               const MapTriggerDecision *decision);

#endif
