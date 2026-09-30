#include "maptrigger.h"

#include <string.h>

#include "combat.h"
#include "effect.h"
#include "item.h"
#include "party.h"

/* Extracted via ida_scripts/dump_trigger_list_table.py (both games). Only coord/flags/rawA/rawB are kept --
   the teleport branch's own extra Chapter 3 fields aren't needed since teleport isn't decided further here. */
typedef struct {
    int coord;
    bool matchIsX;
    uint16_t flags;
    int16_t rawA;
    int16_t rawB;
} RawTriggerEntry;

static const RawTriggerEntry g_triggersYendor2[] = {
    {17, false, 0x0000, 2, 0},
    {18, false, 0x0000, 21, 0},
    {19, false, 0x0000, 12, 0},
    {20, false, 0x0000, 11, 0},
    {63, false, 0x0000, 23, 999},
    {64, false, 0x0000, 31, 0},
    {65, false, 0x4000, 334, 99},
    {66, false, 0x4000, 346, 99},
    {67, false, 0x0200, 322, 0},
};

static const RawTriggerEntry g_triggersYendor3[] = {
    {144, false, 0x0000, 2, 0},
    {332, true, 0x8001, 12, 0},
    {333, true, 0x8001, 12, 0},
    {336, true, 0x8001, 12, 0},
    {337, true, 0x8001, 12, 0},
    {145, false, 0x4000, 493, 59},
    {146, false, 0x4000, 502, 59},
    {147, false, 0x4000, 496, 59},
    {148, false, 0x4000, 506, 59},
    {155, false, 0x4000, 587, 126},
    {156, false, 0x4000, 592, 128},
    {158, false, 0x4000, 591, 138},
    {159, false, 0x4000, 579, 130},
    {157, false, 0x0000, 46, 0},
    {162, false, 0x4000, 685, 76},
    {163, false, 0x4000, 714, 75},
    {164, false, 0x4000, 714, 92},
    {165, false, 0x4000, 685, 91},
    {166, false, 0x4000, 702, 82},
};

unsigned mapTriggerCount(GameKind game) {
    return game == GameYendor2 ? sizeof(g_triggersYendor2) / sizeof(g_triggersYendor2[0])
                                : sizeof(g_triggersYendor3) / sizeof(g_triggersYendor3[0]);
}

bool mapTriggerFind(GameKind game, int worldX, int worldY, MapTriggerRecord *out) {
    const RawTriggerEntry *table = (game == GameYendor2) ? g_triggersYendor2 : g_triggersYendor3;
    unsigned count = mapTriggerCount(game);
    for (unsigned i = 0; i < count; i++) {
        const RawTriggerEntry *e = &table[i];
        int value = e->matchIsX ? worldX : worldY;
        if (value == e->coord) {
            out->coord = e->coord;
            out->matchIsX = e->matchIsX;
            out->flags = e->flags;
            out->rawA = e->rawA;
            out->rawB = e->rawB;
            return true;
        }
    }
    return false;
}

MapTriggerDecision mapTriggerDecide(GameKind game, const MapTriggerRecord *record) {
    MapTriggerDecision d;
    memset(&d, 0, sizeof(d));

    if (record->flags & 0x4000) {
        d.outcome = MapTriggerNone; /* teleport -- not decided further */
        return d;
    }
    if (record->flags & 0x2000) {
        d.outcome = MapTriggerNone; /* ailment tick -- not decided further */
        return d;
    }
    if (record->flags & 0x1000) {
        d.outcome = MapTriggerApplyEffect;
        d.effectId = 0xF; /* gold theft */
        return d;
    }
    if (record->flags & 0x0800) {
        d.outcome = MapTriggerApplyEffect;
        d.effectId = 0x10; /* ore1 theft */
        return d;
    }
    if (record->flags & 0x0400) {
        d.outcome = MapTriggerApplyEffect;
        d.effectId = 0x11; /* ore2 theft */
        return d;
    }
    if (record->flags & 0x0300) {
        d.outcome = MapTriggerApplyCorrosion;
        d.slotOffset = (unsigned)record->rawA;
        unsigned corrosionEffectId = (record->flags & 0x0200) ? 0x2B : 1;
        EffectDef def;
        d.corrosionModeFlags = effectGetDef(game, corrosionEffectId, &def) ? def.modeFlags : 0;
        return d;
    }

    /* Fully data-driven default. */
    d.outcome = MapTriggerApplyEffect;
    d.effectId = (unsigned)record->rawA;
    d.requiresItemExclusion = (game == GameYendor3) && (record->flags & 0x1) != 0;
    return d;
}

void mapTriggerApplyEffect(uint8_t *partyRecord, SaveGame *save, GameKind game, unsigned effectId, int16_t rawA,
                            int16_t rawB, bool requiresItemExclusion) {
    enum { ExclusionItemId = 0x275, ExclusionSlotOffset = 0x158 };
    if (requiresItemExclusion && partyGetU16(partyRecord, ExclusionSlotOffset) == ExclusionItemId) {
        return;
    }

    EffectDef def;
    if (!effectGetDef(game, effectId, &def)) {
        return;
    }
    EffectSpend spend = effectSpend(&def);
    Bcd4 amount = {0, 0, 0, 0};
    uint16_t magnitude = (uint16_t)rawA;
    if (spend == EffectSpendGold || spend == EffectSpendOre1 || spend == EffectSpendOre2) {
        /* rawA/rawB together are a 4-byte packed BCD amount, high digit pair then low -- matching
           ResolveAttackerActionOutcome's own gold-theft convention (file-formats.md's "staged combat
           event" section). Each raw word's own two bytes are the BCD digit pairs directly, most-
           significant byte first (matching Bcd4's own "most-significant digit pair first" convention). */
        amount[0] = (uint8_t)(((uint16_t)rawA) >> 8);
        amount[1] = (uint8_t)((uint16_t)rawA);
        amount[2] = (uint8_t)(((uint16_t)rawB) >> 8);
        amount[3] = (uint8_t)((uint16_t)rawB);
        magnitude = 0;
    }
    combatApplyEffect(partyRecord, save, spend, magnitude, amount, 0);
}

void mapTriggerApplyCorrosion(uint8_t *partyRecord, const ItemCatalog *catalog, GameKind game,
                               const MapTriggerDecision *decision) {
    (void)game;
    uint16_t equippedItemId = partyGetU16(partyRecord, decision->slotOffset);
    if (equippedItemId == 0) {
        return;
    }
    const uint8_t *itemRecord = itemCatalogRecord(catalog, equippedItemId);
    if (!itemRecord) {
        return;
    }
    uint16_t replacementId = itemCorrosionReplacement(catalog, itemRecord);
    if (replacementId == 0) {
        return;
    }
    partyHandleIconBarItemExpiry(partyRecord, catalog, decision->corrosionModeFlags, equippedItemId, replacementId,
                                  decision->slotOffset);
}
