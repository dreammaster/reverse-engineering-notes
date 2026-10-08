#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chest.h"
#include "explore.h"
#include "globalflags.h"
#include "lighting.h"
#include "newgame.h"
#include "consumable.h"
#include "dialogservice.h"
#include "party.h"
#include "spelljump.h"
#include "spellcast.h"
#include "storyevent.h"
#include "travel.h"
#include "windowbake.h"

static uint16_t *placeWord(GameSession *s);

GameSession *sessionNew(GameKind game, const uint8_t *worldDat, size_t size) {
    GameSession *s = calloc(1, sizeof(*s));
    if (!s) {
        return NULL;
    }
    s->game = game;
    saveGameInit(&s->save, game);
    if (!worldMapParseWorldDat(&s->map, game, worldDat, size) || !worldObjectTableParseWorldDat(&s->objects, game, worldDat, size) ||
        !lockCatalogParseWorldDat(&s->locks, game, worldDat, size) || !monsterCatalogParseWorldDat(&s->monsters, game, worldDat, size) ||
        !itemCatalogParseWorldDat(&s->items, game, worldDat, size) || !spellCatalogParseWorldDat(&s->spells, game, worldDat, size) || !dialogCatalogParseWorldDat(&s->dialogs, game, worldDat, size) || !saveGameNewGame(&s->save, game, worldDat, size)) {
        free(s);
        return NULL;
    }
    for (unsigned slot = 0; slot < 4; slot++) { /* the ready-made heroes (records 5-8) are the party */
        saveHeaderSetU16(&s->save, SaveHeaderPartySlots + slot * 2, (uint16_t)(6 + slot));
    }
    s->x = saveHeaderGetU16(&s->save, SaveHeaderWorldX);
    s->y = saveHeaderGetU16(&s->save, SaveHeaderWorldY);
    s->facing = saveHeaderGetU16(&s->save, SaveHeaderFacing);
    s->clock.minutes = saveHeaderGetU16(&s->save, SaveHeaderClockMinutes);
    s->clock.day = s->clock.month = s->clock.year = 1;
    *placeWord(s) = saveHeaderGetU16(&s->save, saveHeaderPlaceFlagsOffset(game));
    randomStart(&s->rng, 30, 50);
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
    sessionRebuild(s);
    return s;
}

void sessionFree(GameSession *session) {
    free(session);
}

void sessionRebuild(GameSession *s) {
    dungeonGridBuild(&s->grid, s->game, &s->map, &s->save, s->x, s->y);
    dungeonGridBakeMarkers(&s->grid, s->game, &s->objects, &s->locks, &s->save);
    monsterPoolRefreshWindow(s->pool, &s->grid, &s->save);
}

uint8_t *sessionPartyRecord(GameSession *s, unsigned slot) {
    unsigned id = slot < 4 ? saveHeaderGetU16(&s->save, SaveHeaderPartySlots + 2 * slot) : 0;
    return id ? saveGamePartyRecordById(&s->save, id) : NULL;
}

static void say(GameSession *s, const char *format, unsigned a) {
    snprintf(s->log, sizeof(s->log), format, a);
}

static void combatNewRound(GameSession *s) {
    SessionCombat *c = &s->combat;
    c->count = combatBuildTurnOrder(&s->save, c->slots, &s->rng, c->order, c->targets);
    c->cursor = 0;
}

static void combatCheckWipe(GameSession *s) {
    const uint8_t *records[4];
    for (unsigned i = 0; i < 4; i++) {
        records[i] = sessionPartyRecord(s, i);
    }
    if (partyWipedOut(records)) {
        s->combat.wiped = true;
        s->combat.active = false;
        snprintf(s->log, sizeof(s->log), "the party has been defeated");
    }
}

/* ProcessCombatRound after a combatant has acted: reap the dead, then the next combatant. */
static void combatAfterAction(GameSession *s) {
    SessionCombat *c = &s->combat;
    CombatRoundOutcome outcome = combatProcessRound(c->slots, c->order, c->count, c->defeated, &c->cursor, &c->staging, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
    if (outcome == CombatRoundNoMonstersLeft) {
        monsterRewardsAward(&s->save, s->game, &c->staging);
        memset(&c->staging, 0, sizeof(c->staging));
        c->active = false;
        snprintf(s->log, sizeof(s->log), "victory: the loot and experience are in the party's totals");
    } else if (outcome == CombatRoundNewRound) {
        combatNewRound(s);
    }
}

/* Monster turns until it is a party member's turn or the combat ends. */
static void combatRunMonsters(GameSession *s) {
    SessionCombat *c = &s->combat;
    while (c->active && c->cursor < c->count && c->order[c->cursor].isMonster) {
        unsigned slot = c->order[c->cursor].index;
        uint8_t *monster = c->slots + (size_t)slot * MonsterRecordSize;
        uint16_t targetId = c->targets[slot];
        uint8_t *target = targetId ? saveGamePartyRecordById(&s->save, targetId) : NULL;
        CombatMonsterTurnOutcome out = combatProcessMonsterTurn(monster, target, &s->save, &s->items, s->game, &s->rng);
        say(s, out.attacked ? "monster %u hits the party" : "monster %u misses or waits", monsterGetU16(monster, MonsterFieldType));
        combatCheckWipe(s);
        if (!c->active) {
            return;
        }
        combatAfterAction(s);
    }
}

static void combatStart(GameSession *s, uint8_t *poolRecord) {
    SessionCombat *c = &s->combat;
    memset(c, 0, sizeof(*c));
    memcpy(c->slots, poolRecord, MonsterRecordSize);
    monsterPoolRemove(poolRecord, &s->grid);
    c->active = true;
    combatNewRound(s);
    snprintf(s->log, sizeof(s->log), "combat");
    combatRunMonsters(s);
}

/* ProcessLevelMonsters: every live monster takes its turn; one that reaches the party engages it and stops the pass. */
static bool monstersTakeTurns(GameSession *s) {
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    for (unsigned slot = 0; slot < MonsterPoolSize && !s->combat.active; slot++) {
        uint8_t *record = s->pool + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0) {
            continue;
        }
        MonsterFullTurn turn = monsterPoolTakeTurn(record, s->game, &s->map, &s->grid, NULL, 0, &staging, s->x, s->y, &s->rng);
        if (turn.move == MonsterMoveEngaged) {
            combatStart(s, record);
        }
    }
    return s->combat.active;
}

SessionStep sessionMove(GameSession *s, MovementAction action) {
    if (s->combat.active || s->combat.wiped) {
        return SessionStepBusy;
    }
    SessionStep result = SessionStepTurned;
    MovementResult move = movementApply(action, s->facing);
    if (move.deltaCol || move.deltaRow) {
        result = SessionStepBlocked;
        int nx = s->x + move.deltaCol, ny = s->y + move.deltaRow;
        if (movementInBounds(s->game, (uint16_t)nx, (uint16_t)ny)) {
            const DungeonGridCell *cell = dungeonGridCellAtWorldPos(&s->grid, nx, ny);
            bool isDoor = cell && (cell->flags & 0x6000);
            MovementCellOutcome outcome =
                movementClassifyCell(s->game, worldMapTileA(&s->map, (unsigned)ny, (unsigned)nx), worldMapTileB(&s->map, (unsigned)ny, (unsigned)nx), isDoor, false);
            if (outcome == MovementCellClear || outcome == MovementCellSpecial) {
                s->x = nx;
                s->y = ny;
                result = SessionStepMoved;
            }
        }
    } else {
        s->facing = move.facing;
    }
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
    sessionRebuild(s);
    monstersTakeTurns(s);
    return result;
}

void sessionScene(GameSession *s, ViewScene *scene, DungeonGridCell cells[ViewportCellCount]) {
    viewportBuild(&s->grid, s->facing, s->x, s->y, cells);
    viewportComputeVisibility(s->game, cells);
    LightingInput light = {s->game == GameYendor2 ? s->light.litFlags : s->travelFlags, s->light.litFlags, s->clock.minutes, s->facing};
    memset(scene, 0, sizeof(*scene));
    bool reset;
    lightingComputeGradient(s->game, &light, 0, scene->gradient, &reset);
    scene->cells = cells;
    scene->facing = s->facing;
    for (unsigned i = 0; s->combat.active && i < CombatMonsterSlotCount; i++) {
        uint8_t *slot = s->combat.slots + (size_t)i * MonsterRecordSize;
        scene->combatMonsters[i] = monsterGetU16(slot, MonsterFieldType) ? slot : NULL;
    }
    monsterPoolEncounterScan(s->pool, &s->monsters, &s->save, s->game, s->facing, (uint16_t)s->x, (uint16_t)s->y, (uint16_t)s->grid.originRow, (uint16_t)s->grid.originCol,
                             cells, scene->cellMonsters, &s->rng);
}

bool sessionAttack(GameSession *s) {
    SessionCombat *c = &s->combat;
    if (!c->active || c->cursor >= c->count || c->order[c->cursor].isMonster) {
        return false;
    }
    uint8_t *member = sessionPartyRecord(s, c->order[c->cursor].index);
    unsigned monsterSlot = 0;
    if (!member || !combatSelectActiveMonster(c->order, c->count, c->defeated, &monsterSlot)) {
        return false;
    }
    CombatPlayerMeleeOutcome out = combatPlayerMeleeAttack(member, c->slots + (size_t)monsterSlot * MonsterRecordSize, &s->items, s->game, &s->rng);
    snprintf(s->log, sizeof(s->log), "party member %u %s", c->order[c->cursor].index + 1, out.weaponBroke ? "breaks the weapon" : out.hit ? "hits" : "misses");
    combatAfterAction(s);
    combatRunMonsters(s);
    return true;
}

typedef struct {
    GameSession *session;
} JumpCtx;

static bool jumpCell(void *ctx, int col, int row, uint16_t *wallType, uint16_t *floorType) {
    GameSession *s = ((JumpCtx *)ctx)->session;
    if (!movementInBounds(s->game, (uint16_t)col, (uint16_t)row)) {
        return false;
    }
    *wallType = worldMapTileA(&s->map, (unsigned)row, (unsigned)col);
    *floorType = worldMapTileB(&s->map, (unsigned)row, (unsigned)col);
    return true;
}

/* The object ahead (the party cell, else the next one) as the interact.h branches want it: the record, its lock or curgame flags and the "already" bit. */
static bool castOnFacing(GameSession *s, SpellBranch branch) {
    WorldObjectProbeResult probe = worldObjectProbeFacingTile(&s->objects, s->game, s->x, s->y, s->facing);
    if (probe.outcome == WorldObjectProbeNone) {
        return false;
    }
    bool isLock = (probe.object.flags & WorldObjectFlagDoor) != 0;
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));
    uint16_t curgameFlags = 0, ignored = 0;
    if (isLock) {
        lockCatalogRecord(&s->locks, probe.object.value, &lock);
    } else if (probe.object.flags & WorldObjectFlagCurgameRecord) {
        lockCatalogCurgameRecord(&s->locks, probe.object.value, &curgameFlags, &ignored);
    }
    bool already = interactBitmapTest(&s->save, interactWorldObjectBitIndex(s->game, &probe.object));
    bool lockDone = isLock && already, curgameDone = !isLock && already;
    if (branch == SpellBranchKnock) {
        return interactKnock(&s->save, s->game, &probe.object, &lock, lockDone, curgameFlags, curgameDone, false);
    }
    if (branch == SpellBranchTriggerCurgameEvent) {
        return interactTriggerFacingCurgameEvent(&s->save, s->game, &probe.object, &lock, lockDone, curgameFlags, curgameDone, false);
    }
    static const InteractOutcome qualifying[] = {InteractOutcomeLockFlag40, InteractOutcomeLockPriced, InteractOutcomeCurgameFlag40};
    return interactResolveIfOutcome(&s->save, s->game, &probe.object, &lock, lockDone, curgameFlags, curgameDone, false, qualifying, 3);
}

/* The healing / curing branches: effect 18 is a capped stat delta read from the record (see session.h). */
static void castStatDelta(GameSession *s, uint8_t *record, const EffectDef *def, const uint8_t *spell) {
    uint16_t clearMask = (spellGetU16(spell, SpellFieldResistFlags) & 0xC0) ? 0xFFFF : spellGetU16(spell, SpellFieldPositionResetFlags);
    partyApplyIconBarStatDelta(record, s->game, def->modeFlags, spellGetU16(spell, SpellFieldInflictedStatus), spellGetU16(spell, SpellFieldInflictedMagnitude),
                               spellGetU16(spell, SpellFieldDrainAmount), clearMask);
}

static bool castSupported(SpellBranch branch) {
    switch (branch) {
    case SpellBranchSingleTarget: case SpellBranchWholeParty: case SpellBranchLightTimer: case SpellBranchHeldItem: case SpellBranchTeleportEngage:
    case SpellBranchLocationBookmark: case SpellBranchTriggerCurgameEvent: case SpellBranchRest: case SpellBranchKnock: case SpellBranchFacingLockOrEvent:
    case SpellBranchAttackActiveMonster: case SpellBranchLifeForce: case SpellBranchAttackAllSlots:
        return true;
    default:
        return false;
    }
}

static SessionCast castSpell(GameSession *s, unsigned casterSlot, unsigned spellId, bool returnToMark) {
    SessionCombat *c = &s->combat;
    uint8_t *caster = sessionPartyRecord(s, casterSlot);
    const uint8_t *spell = spellRecord(&s->spells, spellId);
    if (!caster || !spell) {
        return SessionCastNotKnown;
    }
    unsigned known[256];
    unsigned knownCount = partyKnownAbilityIds(caster, s->game, known, 256);
    bool knows = false;
    for (unsigned i = 0; i < knownCount && i < 256; i++) {
        knows = knows || known[i] == spellId;
    }
    if (!knows) {
        return SessionCastNotKnown;
    }
    SpellBranch branch = spellSelectBranch(spell);
    if (!castSupported(branch)) {
        return SessionCastUnsupported;
    }
    if (!spellCanCast(spell, caster, &s->save, c->active)) {
        return SessionCastCannot;
    }
    spellDeductCosts(spell, caster, &s->save);
    bool done = true;
    switch (branch) {
    case SpellBranchAttackAllSlots:
        combatApplySpellAttackToActiveSlots(c->slots, caster, spell, false, &s->rng);
        break;
    case SpellBranchAttackActiveMonster:
    case SpellBranchLifeForce: {
        unsigned slot = 0;
        if (!combatSelectActiveMonster(c->order, c->count, c->defeated, &slot)) {
            break;
        }
        uint8_t *target = c->slots + (size_t)slot * MonsterRecordSize;
        if (branch == SpellBranchLifeForce) {
            combatApplyLifeForceSpell(target, caster, spell, &s->save, s->game, monsterGetU16(target, MonsterFieldSaveDifficulty), &s->rng);
        } else {
            CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, false, &s->rng);
            combatApplySpellAttack(target, spell, result);
            if (result.hasEffect) {
                combatMarkSpellAttackHit(target, spell);
            }
        }
        break;
    }
    case SpellBranchSingleTarget:
    case SpellBranchWholeParty: {
        EffectDef def;
        if (!effectGetDef(s->game, spellGetU16(spell, SpellFieldEffectId), &def)) {
            done = false;
        } else if (branch == SpellBranchSingleTarget) {
            if (spellGetU16(spell, SpellFieldFlagsA) & (SpellFlagsAPositionReset | SpellFlagsAIconBarPresetAmount)) {
                castStatDelta(s, caster, &def, spell);
            } else {
                done = false;
            }
        } else {
            for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
                uint16_t id = saveGetPartySlot(&s->save, slot);
                if (id == 0) {
                    break;
                }
                uint8_t *record = saveGamePartyRecordById(&s->save, id);
                if (record && !(s->game == GameYendor3 && (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusCursed))) {
                    castStatDelta(s, record, &def, spell);
                }
            }
        }
        break;
    }
    case SpellBranchLightTimer:
        done = lightSourceArmSpellTimer(&s->light, spellGetU16(spell, SpellFieldTimerSlot), spellGetU16(spell, SpellFieldTimerDuration));
        break;
    case SpellBranchHeldItem:
        if (s->heldItem == 0) {
            uint16_t bound = spellCreatedItemRollBound(spell);
            SpellCreatedItem created = spellCreatedItem(spell, bound ? (uint16_t)randomInRange(&s->rng, bound) : 0);
            s->heldItem = created.itemId;
            s->heldExtra = created.extra;
        } else {
            done = false;
        }
        break;
    case SpellBranchTeleportEngage: {
        JumpCtx ctx = {s};
        SpellJumpResult jump = spellResolveJump(s->game, spell, s->facing, s->x, s->y, jumpCell, &ctx);
        if (jump.ok) {
            s->x = jump.col;
            s->y = jump.row;
            ExploreReveal revealed;
            exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
            sessionRebuild(s);
            monstersTakeTurns(s);
        }
        done = jump.ok;
        break;
    }
    case SpellBranchLocationBookmark:
        if (!returnToMark) {
            CombatLocationBookmark mark = {(int16_t)s->x, (int16_t)s->y, s->facing, 0, 0, 0, 0};
            combatSaveLocationBookmark(caster, spellGetU16(spell, SpellFieldBookmarkOffset), mark);
        } else {
            CombatLocationBookmark mark;
            done = combatRestoreLocationBookmark(caster, spellGetU16(spell, SpellFieldBookmarkOffset), &mark);
            if (done) {
                s->x = mark.worldX;
                s->y = mark.worldY;
                s->facing = mark.facing;
                ExploreReveal revealed;
                exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
                sessionRebuild(s);
            }
        }
        break;
    case SpellBranchKnock:
    case SpellBranchTriggerCurgameEvent:
    case SpellBranchFacingLockOrEvent:
        done = castOnFacing(s, branch);
        if (done) {
            sessionRebuild(s);
        }
        break;
    case SpellBranchRest:
        sessionRest(s);
        break;
    default:
        done = false;
        break;
    }
    snprintf(s->log, sizeof(s->log), "party member %u casts spell %u%s", casterSlot + 1, spellId, done ? "" : " (nothing happens)");
    return done ? SessionCastDone : SessionCastNoEffect;
}

SessionCast sessionCast(GameSession *s, unsigned spellId) {
    SessionCombat *c = &s->combat;
    if (!c->active || c->cursor >= c->count || c->order[c->cursor].isMonster) {
        return SessionCastNoCombat;
    }
    SessionCast result = castSpell(s, c->order[c->cursor].index, spellId, false);
    if (result == SessionCastDone || result == SessionCastNoEffect) {
        combatAfterAction(s);
        combatRunMonsters(s);
    }
    return result;
}

SessionCast sessionCastBy(GameSession *s, unsigned casterSlot, unsigned spellId, bool returnToMark) {
    SessionCombat *c = &s->combat;
    if (c->wiped) {
        return SessionCastNoCombat;
    }
    if (c->active) {
        if (c->cursor >= c->count || c->order[c->cursor].isMonster || c->order[c->cursor].index != casterSlot) {
            return SessionCastCannot;
        }
        return sessionCast(s, spellId);
    }
    return castSpell(s, casterSlot, spellId, returnToMark);
}

SessionUse sessionUseItem(GameSession *s, unsigned userSlot, unsigned itemSlot, unsigned recipientSlot) {
    uint8_t *user = sessionPartyRecord(s, userSlot), *recipient = sessionPartyRecord(s, recipientSlot);
    if (!user || !recipient) {
        return SessionUseNothing;
    }
    uint8_t *slot = inventoryGroupSlot(partyInventoryGroup(user, PartyGroupMain), itemSlot);
    unsigned id = slot ? itemSlotId(slot) : 0;
    if (!id) {
        return SessionUseNothing;
    }
    bool done;
    RestorativeKind kind = partyRestorativeForItem(s->game, id);
    if (kind != RestorativeNone) {
        done = partyUseRestorative(kind, recipient);
    } else if (partyIsPercentRestorativeItem(s->game, id)) {
        const uint8_t *record = itemCatalogRecord(&s->items, id);
        const uint8_t *entry = record ? itemTargetEntry(&s->items, record) : NULL;
        if (!entry) {
            return SessionUseUnsupported;
        }
        partyUsePercentRestorative(s->game, recipient, (itemTargetWord(entry, ItemTargetSlotFlags) & 0x8000) != 0, itemTargetWord(entry, 2));
        done = true;
    } else {
        return SessionUseUnsupported;
    }
    if (!done) {
        return SessionUseNoEffect;
    }
    partyConsumeItemCharge(user, &s->items, slot);
    snprintf(s->log, sizeof(s->log), "party member %u uses item %u on member %u", userSlot + 1, id, recipientSlot + 1);
    if (s->combat.active && s->combat.cursor < s->combat.count && !s->combat.order[s->combat.cursor].isMonster) {
        combatAfterAction(s);
        combatRunMonsters(s);
    }
    return SessionUseDone;
}

/* The place-flag word of the game (Chapter 2: the one that also carries the lit light sources; Chapter 3: the travel flags), kept in step with the save header. */
static uint16_t *placeWord(GameSession *s) {
    return s->game == GameYendor2 ? &s->light.litFlags : &s->travelFlags;
}

static void storePlace(GameSession *s) {
    saveHeaderSetU16(&s->save, saveHeaderPlaceFlagsOffset(s->game), *placeWord(s));
}

static bool flagSet(GameSession *s, int index) {
    return globalFlagTest(saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize, (unsigned)index);
}

/* Puts the party at a new place: position and facing, a rebuilt window, the cells around it revealed. */
static void arriveAt(GameSession *s, int x, int y, uint16_t facing) {
    s->x = x;
    s->y = y;
    s->facing = facing;
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
    sessionRebuild(s);
}

static void travelTo(GameSession *s, unsigned destinationId) {
    TravelDestination d;
    if (!travelDestinationLookup(s->game, destinationId, &d)) {
        return;
    }
    TravelArrival a = travelArrival(s->game, &d, *placeWord(s), s->clock.minutes);
    *placeWord(s) = a.placeFlags;
    storePlace(s);
    saveHeaderSetU16(&s->save, SaveHeaderDayTrack, a.dayTrack);
    saveHeaderSetU16(&s->save, SaveHeaderNightTrack, a.nightTrack);
    if (s->game == GameYendor3) {
        saveHeaderSetU16(&s->save, SaveHeaderTravelMode, a.travelMode);
    }
    s->pendingSound = a.sound;
    arriveAt(s, a.x, a.y, a.facing);
    say(s, "travelled to destination %u", destinationId);
}

/* Moves one item at a time from `from` to `to` (SwapItemInstanceEverywhere's loop; only the main inventories and the global slots are searched). */
static unsigned swapItemEverywhere(GameSession *s, uint16_t from, uint16_t to) {
    unsigned swapped = 0;
    uint8_t globalSlots[24];
    memset(globalSlots, 0, sizeof(globalSlots));
    for (;;) {
        ItemRangeAvailability r = itemRangeAvailable(globalSlots, &s->save, from, from);
        if (!r.found) {
            return swapped;
        }
        uint8_t *record = r.inGlobalTable ? NULL : saveGamePartyRecordById(&s->save, r.partyRecordId);
        uint8_t *slot = r.inGlobalTable ? globalSlots + r.slotOffset : record ? record + r.slotOffset : NULL;
        if (!slot) {
            return swapped;
        }
        itemSlotSet(slot, to, itemSlotExtra(slot));
        swapped++;
    }
}

/* Removes every item of the id range, spending a charge at a time (the SWORD / HAMMER / TRIDENT OF LIGHT). */
static unsigned removeItemRange(GameSession *s, uint16_t low, uint16_t high) {
    unsigned removed = 0;
    uint8_t globalSlots[24];
    memset(globalSlots, 0, sizeof(globalSlots));
    for (unsigned guard = 0; guard < 64; guard++) {
        ItemRangeAvailability r = itemRangeAvailable(globalSlots, &s->save, low, high);
        if (!r.found) {
            break;
        }
        if (r.inGlobalTable) {
            itemSlotConsumeGlobalCharge(&s->items, globalSlots + r.slotOffset);
        } else {
            uint8_t *record = saveGamePartyRecordById(&s->save, r.partyRecordId);
            if (!record) {
                break;
            }
            partyRemoveMultiStatEffect(record, itemEffectEntry(&s->items, itemCatalogRecord(&s->items, r.itemId)));
            partyConsumeItemCharge(record, &s->items, record + r.slotOffset);
        }
        removed++;
    }
    return removed;
}

/* HandleScriptedStoryEventTrigger (Chapter 3), see storyevent.h. */
static SessionInteract storyEventRun(GameSession *s, unsigned value) {
    SessionInteract out;
    memset(&out, 0, sizeof(out));
    out.kind = SessionInteractNothing;
    out.value = value;
    const StoryEvent *e = storyEvent(s->game, value);
    if (!e) {
        return out;
    }
    if (e->doneFlag && flagSet(s, e->doneFlag)) {
        return out;
    }
    if (e->requireFlag && !flagSet(s, e->requireFlag)) {
        if (e->failTextOffset) {
            out.kind = SessionInteractStoryRefused;
            out.textOffset = e->failTextOffset;
            out.textLines = 1;
        }
        return out;
    }
    uint8_t *flags = saveGlobalFlags(&s->save);
    if (e->doneFlag) {
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, (unsigned)e->doneFlag);
    }
    if (e->setFlag) {
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, (unsigned)e->setFlag);
    }
    if (e->clearFlag) {
        globalFlagClear(flags, SaveHeaderGlobalFlagsSize, (unsigned)e->clearFlag);
    }
    *placeWord(s) = (uint16_t)((*placeWord(s) & e->travelFlagsAnd) | e->travelFlagsOr);
    storePlace(s);
    saveHeaderSetU16(&s->save, SaveHeaderTravelMode, (uint16_t)e->travelMode);
    if (e->clearTracks) {
        saveHeaderSetU16(&s->save, SaveHeaderDayTrack, 0);
        saveHeaderSetU16(&s->save, SaveHeaderNightTrack, 0);
    }
    unsigned changed = 0;
    if (e->swapFrom) {
        changed += swapItemEverywhere(s, e->swapFrom, e->swapTo);
    }
    if (e->checkItem) {
        uint8_t globalSlots[24];
        memset(globalSlots, 0, sizeof(globalSlots));
        if (itemRangeAvailable(globalSlots, &s->save, e->checkItem, e->checkItem).found) {
            globalFlagSet(flags, SaveHeaderGlobalFlagsSize, (unsigned)e->checkItemFlag);
        }
    }
    if (e->removeLow) {
        changed += removeItemRange(s, e->removeLow, e->removeHigh);
    }
    s->pendingSound = e->sound;
    arriveAt(s, e->x, e->y, e->facing);
    out.kind = SessionInteractStoryEvent;
    if (e->doneTextOffset && (e->doneTextAlways || changed)) {
        out.textOffset = e->doneTextOffset;
        out.textLines = 3;
    }
    say(s, "story event %u", value);
    return out;
}

SessionInteract sessionInteract(GameSession *s) {
    SessionInteract out;
    memset(&out, 0, sizeof(out));
    if (s->combat.active || s->combat.wiped) {
        return out;
    }
    WorldObjectProbeResult probe = worldObjectProbeFacingTile(&s->objects, s->game, s->x, s->y, s->facing);
    if (probe.outcome == WorldObjectProbeNone) {
        return out;
    }
    unsigned flags = probe.object.flags, value = probe.object.value;
    out.value = value;
    if (flags & WorldObjectFlagDoor) {
        out.kind = SessionInteractDoor;
        return out;
    }
    if (flags & WorldObjectFlagUnknown2000) { /* a destination: UseAbilityCommand's neighbour in start, then TravelToDestination */
        TravelDestination d;
        if (!travelDestinationLookup(s->game, value, &d)) {
            return out;
        }
        unsigned gate = s->game == GameYendor2 ? 0x2000u : 0xC000u;
        if (d.flags & gate) {
            TravelUnlockCheck check = travelCheckUnlock(s->game, value, d.flags, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
            if (check.outcome == TravelUnlockNeedsPassword) {
                out.kind = SessionInteractTravelPassword;
                out.promptId = check.promptId;
                s->pendingTravel = value;
                return out;
            }
            if (check.outcome != TravelUnlockAlreadyUnlocked) {
                out.kind = check.outcome == TravelUnlockDeniedSilent ? SessionInteractNothing : SessionInteractTravelRefused;
                out.messageId = check.messageId;
                return out;
            }
        }
        travelTo(s, value);
        out.kind = SessionInteractTravelled;
        return out;
    }
    if (flags & WorldObjectFlagTalker) {
        out.kind = SessionInteractTalker;
        return out;
    }
    if (flags & WorldObjectFlagStoryEvent) {
        if (s->game == GameYendor3) {
            return storyEventRun(s, value);
        }
        if (value == 1) {
            if (storyEndingReady(saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize)) {
                out.kind = SessionInteractEnding;
            } else {
                out.kind = SessionInteractEndingNotReady;
                out.textOffset = 0x86AD;
                out.textLines = 2;
            }
        }
    }
    return out;
}

bool sessionTalkStart(GameSession *s, unsigned npcId, ConversationStep *first) {
    const uint8_t *npc = dialogNpc(&s->dialogs, npcId);
    if (!npc || s->combat.active || s->combat.wiped) {
        return false;
    }
    uint8_t globalSlots[24];
    memset(globalSlots, 0, sizeof(globalSlots));
    unsigned item = dialogGetU16(npc, DialogNpcRequiredItemRange);
    bool held = item && itemRangeAvailable(globalSlots, &s->save, (uint16_t)item, (uint16_t)item).found;
    s->talking = conversationStart(&s->talk, &s->dialogs, npcId, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize, held, first);
    if (s->talking && s->talk.ended) {
        s->talking = false;
    }
    return s->talking || (first && first->ended);
}

ConversationStep sessionTalkChoose(GameSession *s, unsigned number) {
    ConversationStep step = conversationChoose(&s->talk, number, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
    if (s->talk.ended) {
        s->talking = false;
    }
    return step;
}

void sessionTalkResolve(GameSession *s, bool success) {
    conversationResolve(&s->talk, success, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
}

static void parseQuantity(const char *text, Bcd4 out) {
    unsigned value = 0;
    for (; text && *text >= '0' && *text <= '9' && value < 100000; text++) {
        value = value * 10 + (unsigned)(*text - '0');
    }
    memset(out, 0, 4);
    for (unsigned chunk = value; chunk > 0;) { /* up to 99,999: add in steps a u16 holds */
        unsigned part = chunk > 9999 ? 9999 : chunk;
        bcd4AddU16(out, (uint16_t)part);
        chunk -= part;
    }
}

SessionService sessionTalkService(GameSession *s, const ConversationStep *step, const char *text) {
    uint8_t *flags = saveGlobalFlags(&s->save);
    const uint8_t *npc = s->talk.npc;
    if (!npc || !step || !step->topic) {
        return SessionServiceNone;
    }
    if (step->flags & DialogTopicAttributeBoost) {
        return dialogApplyAttributeTome(npc, &s->save, flags, SaveHeaderGlobalFlagsSize).applied ? SessionServiceDone : SessionServiceFailed;
    }
    if (step->flags & DialogTopicExperienceBoost) {
        return dialogApplyExperienceTome(npc, &s->save, s->game, flags, SaveHeaderGlobalFlagsSize).applied ? SessionServiceDone : SessionServiceFailed;
    }
    if (step->flags & DialogTopicBuy) {
        if (!text) {
            return SessionServiceNeedText;
        }
        bool ok;
        if (step->buy) {
            Bcd4 quantity;
            parseQuantity(text, quantity);
            ok = dialogBuyOre(step->arg, saveHeaderBcd4(&s->save, SaveHeaderGold), saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), quantity);
        } else {
            ok = dialogCheckRiddleAnswer(s->game, step->arg, text);
        }
        if (step->needsResolve) {
            sessionTalkResolve(s, ok);
        }
        return ok ? SessionServiceDone : SessionServiceFailed;
    }
    if (step->flags & (DialogTopicRepair | DialogTopicEnhance | DialogTopicSell | DialogTopicUseKey | DialogTopicCheckKey) || step->typeService != DialogServiceNone) {
        return SessionServiceNeedScreen;
    }
    return SessionServiceNone;
}

bool sessionTravelPassword(GameSession *s, const char *typed) {
    if (!s->pendingTravel || !travelResolvePassword(s->game, s->pendingTravel, typed, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize)) {
        return false;
    }
    unsigned id = s->pendingTravel;
    s->pendingTravel = 0;
    travelTo(s, id);
    return true;
}

typedef struct {
    GameSession *session;
} RestCtx;

static bool restMonsters(void *ctx) {
    return monstersTakeTurns(((RestCtx *)ctx)->session);
}

RestOutcome sessionRest(GameSession *s) {
    RestOutcome none;
    memset(&none, 0, sizeof(none));
    if (s->combat.active || s->combat.wiped) {
        none.refused = true;
        return none;
    }
    uint8_t globalSlots[24];
    memset(globalSlots, 0, sizeof(globalSlots));
    RestCtx ctx = {s};
    RestOutcome out = restParty(&s->save, &s->clock, &s->items, globalSlots, false, false, false, restMonsters, &ctx);
    s->clock.minutes %= 1440;
    if (!out.refused) {
        lightSourceTickTimers(&s->light, out.minutes);
    }
    if (out.refused) {
        snprintf(s->log, sizeof(s->log), "you cannot rest here");
    } else if (out.interrupted) {
        say(s, "the rest was interrupted in hour %u", out.hour);
    } else {
        say(s, "rested 8 hours, %u fed", out.fed);
    }
    sessionRebuild(s);
    return out;
}

InteractUnlockOutcome sessionUnlock(GameSession *s, uint16_t keyWord0, uint16_t heldKeyFlags) {
    InteractUnlockOutcome out = interactUnlockFacing(&s->save, s->game, &s->objects, &s->locks, s->x, s->y, s->facing, keyWord0, heldKeyFlags);
    static const char *const names[] = {"nothing to unlock here", "already unlocked", "unlocked", "locked (needs another key)"};
    snprintf(s->log, sizeof(s->log), "%s", names[out.result]);
    if (out.result == InteractUnlockOpened) {
        DungeonGridCell *cell = dungeonGridCellMutable(&s->grid, out.worldRow - s->grid.originRow, out.worldCol - s->grid.originCol);
        if (cell) {
            cell->flags &= (uint16_t)~out.clearCellBits;
        }
    }
    return out;
}

unsigned sessionLoot(GameSession *s) {
    WorldObjectProbeResult probe = worldObjectProbeFacingTile(&s->objects, s->game, s->x, s->y, s->facing);
    LockRecord lock;
    unsigned looted = 0;
    if (probe.outcome != WorldObjectProbeNone && (probe.object.flags & WorldObjectFlagDoor) && lockCatalogRecord(&s->locks, probe.object.value, &lock)) {
        for (unsigned slot = 0; slot < LockContentSlots; slot++) {
            looted += chestTake(&s->save, probe.object.value, &lock, &s->items, slot, NULL);
        }
    }
    say(s, "looted %u slots", looted);
    return looted;
}
