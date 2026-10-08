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
#include "ailment.h"
#include "dialogservice.h"
#include "document.h"
#include "maptrigger.h"
#include "inventory.h"
#include "relics.h"
#include "shop.h"
#include "party.h"
#include "spelljump.h"
#include "spellcast.h"
#include "storyevent.h"
#include "travel.h"
#include "windowbake.h"

static uint16_t *placeWord(GameSession *s);
static void arriveAt(GameSession *s, int x, int y, uint16_t facing);

GameSession *sessionNew(GameKind game, const uint8_t *worldDat, size_t size) {
    GameSession *s = calloc(1, sizeof(*s));
    if (!s) {
        return NULL;
    }
    s->game = game;
    saveGameInit(&s->save, game);
    if (!worldMapParseWorldDat(&s->map, game, worldDat, size) || !worldObjectTableParseWorldDat(&s->objects, game, worldDat, size) ||
        !lockCatalogParseWorldDat(&s->locks, game, worldDat, size) || !monsterCatalogParseWorldDat(&s->monsters, game, worldDat, size) ||
        !itemCatalogParseWorldDat(&s->items, game, worldDat, size) || !spellCatalogParseWorldDat(&s->spells, game, worldDat, size) || !dialogCatalogParseWorldDat(&s->dialogs, game, worldDat, size) || !documentCatalogParseWorldDat(&s->documents, game, worldDat, size) || !saveGameNewGame(&s->save, game, worldDat, size)) {
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

void sessionSetParty(GameSession *s, const uint8_t records[4][SavePartyRecordSize]) {
    for (unsigned slot = 0; slot < 4; slot++) {
        uint8_t *dest = sessionPartyRecord(s, slot);
        if (dest) {
            memcpy(dest, records[slot], SavePartyRecordSize);
        }
    }
    sessionRebuild(s);
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

/* ApplyMapTriggerEffect (yendor2.asm:17458): run for the cell the party stands on after every step -- a trap that hurts, steals or corrodes the members who can act, or (flag
 * 0x4000) a teleport to the record's own cell. Returns what happened for the log. */
static bool applyMapTrigger(GameSession *s) {
    MapTriggerRecord record;
    if (!mapTriggerFind(s->game, s->x, s->y, &record)) {
        return false;
    }
    if (record.flags & 0x4000) {
        arriveAt(s, record.rawA, record.rawB, s->facing);
        say(s, "teleported by a trigger to column %u", (unsigned)record.rawA);
        return true;
    }
    MapTriggerDecision decision = mapTriggerDecide(s->game, &record);
    if (decision.outcome == MapTriggerNone) {
        return false;
    }
    for (unsigned slot = 0; slot < 4; slot++) {
        uint8_t *member = sessionPartyRecord(s, slot);
        if (!member) {
            break;
        }
        if (partyGetU16(member, PartyFieldStatusFlags) & PartyStatusIncapacitated) {
            continue;
        }
        if (decision.outcome == MapTriggerApplyEffect) {
            mapTriggerApplyEffect(member, &s->save, s->game, decision.effectId, record.rawA, record.rawB, decision.requiresItemExclusion);
        } else {
            mapTriggerApplyCorrosion(member, &s->items, s->game, &decision);
        }
    }
    snprintf(s->log, sizeof(s->log), "a trap goes off");
    combatCheckWipe(s);
    return true;
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
    if (result == SessionStepMoved) {
        applyMapTrigger(s);
    }
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

/*
 * GetMonsterAtViewportRow (yendor2.asm:23393): the viewport "depth" the spells use is the index into the 51-cell buffer (viewport.h); a cell with the monster marker
 * (+6 bit 0x400, its +4 the monster's type) names the pool monster of that type. Hidden cells are not looked at.
 */
static uint8_t *monsterAtDepth(GameSession *s, const DungeonGridCell cells[ViewportCellCount], unsigned depth) {
    if (depth >= ViewportCellCount || !(cells[depth].flags & 0x400)) {
        return NULL;
    }
    int slot = monsterPoolFindType(s->pool, cells[depth].reserved4);
    return slot >= 0 ? s->pool + (size_t)slot * MonsterRecordSize : NULL;
}

/*
 * The projectile family (ApplyEncodedItemEffect's loc_2C92A): the shot flies down the party's column of the view through the depths 0x31, 0x2E, 0x2B, 0x28, 0x24, 0x19
 * (cells 49, 46, 43, 40, 36, 25), stops at a wall (a cell the view hides) and hits the first monster; a piercing one goes on. A splash spell (FlagsB 0x400) instead
 * runs ApplyAttackAlongCorridorLine over three runs of three cells around the impact -- the runs start at 24 / 35 / 39 for depth 0x24, 35 / 39 / 42 for 0x28, 39 / 42 / 45
 * for 0x2B and 42 / 45 / 48 for the rest -- every monster there takes the attack and then the compounded second application, and the dead are reaped.
 * The flight and its frames are the front end's.
 */
static bool castProjectile(GameSession *s, uint8_t *caster, const uint8_t *spell) {
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    uint8_t *flags = saveGlobalFlags(&s->save);
    bool splash = (spellGetU16(spell, SpellFieldFlagsB) & SpellFlagsBSplash) != 0, hit = false;
    static const unsigned column[] = {49, 46, 43, 40, 36, 25};
    for (unsigned i = 0; i < sizeof(column) / sizeof(column[0]); i++) {
        unsigned depth = column[i];
        if (depth != 49 && (cells[depth].flags & ViewportCellHidden)) {
            break;
        }
        uint8_t *monster = monsterAtDepth(s, cells, depth);
        if (!monster) {
            continue;
        }
        hit = true;
        if (splash) {
            unsigned starts[3] = {42, 45, 48};
            if (depth == 36) {
                starts[0] = 24, starts[1] = 35, starts[2] = 39;
            } else if (depth == 40) {
                starts[0] = 35, starts[1] = 39, starts[2] = 42;
            } else if (depth == 43) {
                starts[0] = 39, starts[1] = 42, starts[2] = 45;
            }
            for (unsigned run = 0; run < 3; run++) {
                for (unsigned k = 0; k < 3; k++) {
                    uint8_t *target = monsterAtDepth(s, cells, starts[run] + k);
                    if (target) {
                        combatApplySplashHit(target, caster, spell, s->scrollCast, &s->rng);
                    }
                }
            }
            combatReapDeadMapMonsters(s->pool, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid);
            break;
        }
        CombatProjectileHitOutcome outcome = combatApplyProjectileHit(monster, caster, spell, s->scrollCast, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid, &s->rng);
        if (!outcome.continues) {
            break;
        }
    }
    monsterRewardsAward(&s->save, s->game, &staging);
    sessionRebuild(s);
    return hit;
}

/* The attacks on what is in front of the party outside a combat: the screen-wide family (ScreenWide, Tremor, Rain, Turbulence: the monsters of every cell in the original's
 * order -- depths 0x32, 0x30, then 0x2F down to 0 -- combatApplyScreenWideAttack) and the beam (the first monster down the centre column, combatApplyDamageToMapMonster). The
 * animations are the front end's. */
static bool castColumnAttack(GameSession *s, uint8_t *caster, const uint8_t *spell, bool beam) {
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    uint8_t *rowMonsters[CombatScreenAttackRows];
    memset(rowMonsters, 0, sizeof(rowMonsters));
    unsigned found = 0;
    if (beam) {
        static const unsigned column[] = {46, 43, 40, 36, 25};
        for (unsigned i = 0; i < sizeof(column) / sizeof(column[0]) && !found; i++) {
            if (cells[column[i]].flags & ViewportCellHidden) {
                break;
            }
            rowMonsters[0] = monsterAtDepth(s, cells, column[i]);
            found = rowMonsters[0] != NULL;
        }
    } else {
        for (unsigned row = 0; row < CombatScreenAttackRows; row++) { /* [0] depth 0x32, [1] 0x30, [2..49] 0x2F .. 0 */
            unsigned depth = row == 0 ? 0x32 : row == 1 ? 0x30 : 0x2F - (row - 2);
            rowMonsters[row] = monsterAtDepth(s, cells, depth);
            found += rowMonsters[row] != NULL;
        }
    }
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    uint8_t *flags = saveGlobalFlags(&s->save);
    if (beam) {
        if (found) {
            combatApplyDamageToMapMonster(rowMonsters[0], caster, spell, s->scrollCast, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid, &s->rng);
        }
    } else {
        combatApplyScreenWideAttack(NULL, rowMonsters, caster, spell, s->scrollCast, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid, &s->rng);
    }
    monsterRewardsAward(&s->save, s->game, &staging);
    sessionRebuild(s);
    return found > 0;
}

static bool castSupported(SpellBranch branch, const uint8_t *spell) {
    (void)spell;
    switch (branch) {
    case SpellBranchSingleTarget: case SpellBranchWholeParty: case SpellBranchLightTimer: case SpellBranchHeldItem: case SpellBranchTeleportEngage:
    case SpellBranchLocationBookmark: case SpellBranchTriggerCurgameEvent: case SpellBranchRest: case SpellBranchKnock: case SpellBranchFacingLockOrEvent:
    case SpellBranchAttackActiveMonster: case SpellBranchLifeForce: case SpellBranchAttackAllSlots:
        return true;
    case SpellBranchScreenWide: case SpellBranchBeam: case SpellBranchTremor: case SpellBranchRain: case SpellBranchTurbulence:
        return true;
    case SpellBranchProjectile:
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
    if (!knows && !s->scrollCast) {
        return SessionCastNotKnown;
    }
    SpellBranch branch = spellSelectBranch(spell);
    if (!castSupported(branch, spell)) {
        return SessionCastUnsupported;
    }
    if (s->scrollCast ? !spellUsableInContext(spell, c->active) : !spellCanCast(spell, caster, &s->save, c->active)) {
        return SessionCastCannot;
    }
    if (!s->scrollCast) {
        spellDeductCosts(spell, caster, &s->save);
    }
    bool done = true;
    switch (branch) {
    case SpellBranchAttackAllSlots:
        combatApplySpellAttackToActiveSlots(c->slots, caster, spell, s->scrollCast, &s->rng);
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
            CombatSpellAttackResult result = combatResolveSpellAttack(target, caster, spell, s->scrollCast, &s->rng);
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
                castStatDelta(s, s->scrollCast && sessionPartyRecord(s, s->scrollTarget) ? sessionPartyRecord(s, s->scrollTarget) : caster, &def, spell);
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
    case SpellBranchProjectile:
        done = castProjectile(s, caster, spell);
        break;
    case SpellBranchScreenWide: case SpellBranchTremor: case SpellBranchRain: case SpellBranchTurbulence:
    case SpellBranchBeam:
        done = castColumnAttack(s, caster, spell, branch == SpellBranchBeam);
        break;
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

/* TickWorldAilments (yendor2.asm:27798): the light sources burn down one unit -- the global slots, every member's eight main slots, the spell timers. */
static void worldTick(GameSession *s) {
    for (unsigned member = 0; member < 4; member++) {
        uint8_t *record = sessionPartyRecord(s, member);
        for (unsigned slot = 1; record && slot <= InventorySlotCount; slot++) {
            lightSourceTickItemSlot(&s->light, inventoryGroupSlot(partyInventoryGroup(record, PartyGroupMain), slot), 1);
        }
    }
    lightSourceTickTimers(&s->light, 1);
}

static bool lightsActive(const GameSession *s) {
    unsigned total = 0;
    for (unsigned i = 0; i < LightSourceCount; i++) {
        total += s->light.litCount[i];
    }
    for (unsigned i = 0; i < LightSpellTimerCount; i++) {
        total += s->light.timers[i];
    }
    return total != 0;
}

bool sessionDocumentLine(GameSession *s, unsigned line, char out[DocumentLineBufferSize]) {
    return documentGetLine(&s->documents, (DocumentCategory)s->readCategory, s->readId, line, out);
}

SessionCast sessionUseScroll(GameSession *s, unsigned userSlot, unsigned itemSlot, bool study, unsigned memberSlot) {
    uint8_t *user = sessionPartyRecord(s, userSlot);
    uint8_t *slot = user ? inventoryGroupSlot(partyInventoryGroup(user, PartyGroupMain), itemSlot) : NULL;
    const uint8_t *item = slot && itemSlotId(slot) ? itemCatalogRecord(&s->items, itemSlotId(slot)) : NULL;
    const uint8_t *target = item ? itemTargetEntry(&s->items, item) : NULL;
    if (!target || itemTargetKind(item) != ItemTargetConsumable || !(itemTargetWord(target, ItemTargetSlotFlags) & 0x600)) {
        return SessionCastNotKnown;
    }
    unsigned spellId = itemTargetWord(target, 2);
    const uint8_t *spell = spellRecord(&s->spells, spellId);
    if (!spell) {
        return SessionCastNotKnown;
    }
    if (study) { /* MarkIneligiblePartyMembers: only a member who does not know it, of a fitting class and level */
        uint8_t *member = sessionPartyRecord(s, memberSlot);
        if (!(itemTargetWord(target, ItemTargetSlotFlags) & 0x200) || !member || !spellCanLearn(spell, spellId, member)) {
            return SessionCastCannot;
        }
        spellLearn(member, spellId);
        partyConsumeItemCharge(user, &s->items, slot);
        return SessionCastDone;
    }
    s->scrollCast = true;
    s->scrollTarget = memberSlot;
    SessionCast result = castSpell(s, userSlot, spellId, false);
    s->scrollCast = false;
    if (result == SessionCastDone || result == SessionCastNoEffect) {
        partyConsumeItemCharge(user, &s->items, slot);
        if (s->combat.active && s->combat.cursor < s->combat.count && !s->combat.order[s->combat.cursor].isMonster) {
            combatAfterAction(s);
            combatRunMonsters(s);
        }
    }
    return result;
}

void sessionTickAilments(GameSession *s) {
    if (s->combat.wiped) {
        return;
    }
    AilmentPass passes[4];
    unsigned n = ailmentTickIconBar(&s->ailments, &s->save, s->game, AilmentUiGateMask & 0x0800, *placeWord(s), &s->rng, passes);
    for (unsigned p = 0; p < n; p++) {
        for (unsigned i = 0; i < passes[p].count; i++) {
            const AilmentStage *stage = &passes[p].stages[i];
            uint8_t *member = sessionPartyRecord(s, stage->slot);
            uint16_t amount = passes[p].kind == AilmentPassCold ? stage->magnitude : stage->severity;
            if (member) {
                combatApplyPresetEffect(member, &s->save, s->game, stage->effectId, amount, 0, &s->rng);
            }
        }
    }
    if (n) {
        combatCheckWipe(s);
    }
}

void sessionAdvanceTime(GameSession *s, unsigned minutes) {
    for (unsigned n = 0; n < minutes; n++) {
        GameClock *c = &s->clock;
        c->minutes++;
        if (c->minutes == 0x5A1) { /* the per-minute AdvanceGameClock: a new day (the original's bounds, 1441, are reproduced) */
            for (unsigned i = 0; i < SavePartyRecordCount; i++) {
                uint8_t *record = saveGamePartyRecord(&s->save, i);
                if (record) {
                    partyResetDailyAbilityCharges(record);
                }
            }
            c->minutes = 1;
            if (++c->day == 31) {
                c->day = 1;
                if (++c->month == 13) {
                    c->year++;
                    c->month = 1;
                }
            }
        } else if (c->minutes == 0x168 || c->minutes == 0x438) { /* 06:00 and 18:00: the day / night palette fade */
            s->pendingFade = c->minutes == 0x168 ? 1 : 2;
        }
        if (lightsActive(s)) { /* every fifth minute while anything burns */
            if (s->worldTickCountdown != 0 && --s->worldTickCountdown == 0) {
                worldTick(s);
            }
            if (s->worldTickCountdown == 0) {
                s->worldTickCountdown = 5;
            }
        }
    }
}

static void travelTo(GameSession *s, unsigned destinationId);

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
    const uint8_t *itemRec = itemCatalogRecord(&s->items, id);
    const uint8_t *target = itemRec ? itemTargetEntry(&s->items, itemRec) : NULL;
    if (target && itemTargetKind(itemRec) == ItemTargetConsumable && (itemTargetWord(target, 0) & 4) && !(itemTargetWord(target, ItemTargetSlotFlags) & 0x7E00)) {
        /* ExamineTarget: a key to a destination (KEY OF PORT HOPE..., ANKH OF PORTALS, ATHANEUM KEY): the road opens when its gate flag is set */
        uint8_t globalSlots[24];
        memset(globalSlots, 0, sizeof(globalSlots));
        bool storyItem = itemRangeAvailable(globalSlots, &s->save, TravelStoryItemLow, TravelStoryItemHigh).found;
        unsigned destination = itemTargetWord(target, 2);
        TravelExamineOutcome outcome = travelExamineKey(s->game, destination, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize, storyItem);
        if (outcome != TravelExamineTravels) {
            return outcome == TravelExamineGateClosed ? SessionUseClosed : SessionUseBlocked;
        }
        travelTo(s, destination);
        return SessionUseTravelled;
    }
    /* a spell scroll (InteractWithContainer): the entry's flags 0x200 / 0x400, word 2 the spell; cast it (sessionUseScroll) or, with 0x200, study it */
    if (target && itemTargetKind(itemRec) == ItemTargetConsumable && (itemTargetWord(target, ItemTargetSlotFlags) & 0x600)) {
        return spellRecord(&s->spells, itemTargetWord(target, 2)) ? SessionUseScroll : SessionUseNoEffect;
    }
    /* a readable item (HandleGameCommand -> RunConversation): the target entry's flags 0x4000 / 0x2000 / 0x1000 / 0x800 pick the pool, its word 2 the entry, the reader's
     * Linguistics (against the difficulty bits 0x8 / 0x4 / 0x2 of word 1) how much of it is readable */
    if (target && itemTargetKind(itemRec) == ItemTargetConsumable && (itemTargetWord(target, ItemTargetSlotFlags) & 0x7800) && !(itemTargetWord(target, ItemTargetSlotFlags) & 0x600)) {
        unsigned word1 = itemTargetWord(target, ItemTargetSlotFlags);
        s->readCategory = word1 & 0x4000 ? DocumentCategoryJournal : word1 & 0x2000 ? DocumentCategoryDocument : word1 & 0x1000 ? DocumentCategoryNote : DocumentCategoryUnused;
        s->readId = itemTargetWord(target, 2);
        s->readability = documentClassifyReadability(s->game, (uint16_t)word1, (int16_t)partyGetStat(user, PartyStatLinguistics));
        return documentLineCount(s->game, (DocumentCategory)s->readCategory, s->readId) ? SessionUseRead : SessionUseNoEffect;
    }
    /* the light sources (HandleGameCommand: 8 / 0xE / 0xB light, 9 / 0xF / 0xC put out; ApplyStatusEffect, TickStatusEffects): the lit-item family steps to the next id */
    if (id == 8 || id == 0xE || id == 0xB || id == 9 || id == 0xF || id == 0xC) {
        bool lighting = id == 8 || id == 0xE || id == 0xB;
        if (lighting && s->game == GameYendor2 && (s->light.litFlags & 2)) {
            return SessionUseNoEffect; /* the place bit 2: the original flashes a warning */
        }
        bool ok = lighting ? lightSourceApply(&s->light, id) : lightSourceTick(&s->light, id);
        if (!ok) {
            return SessionUseNoEffect;
        }
        itemSlotSet(slot, (uint16_t)(lighting ? id + 1 : id - 1), itemSlotExtra(slot));
        snprintf(s->log, sizeof(s->log), lighting ? "light source %u lit" : "light source %u put out", id);
        return SessionUseDone;
    }
    /* Chapter 2's quest relics (DispatchItemAbilityCommand) */
    if (s->game == GameYendor2 && !(target && itemTargetKind(itemRec) == ItemTargetConsumable && (itemTargetWord(target, 0) & 4)) && relicClassify(id) != RelicActionNone) {
        RelicAction action = relicClassify(id);
        uint8_t *flags = saveGlobalFlags(&s->save);
        if (action == RelicActionCharged) {
            if (!relicReady(flags, SaveHeaderGlobalFlagsSize)) {
                return SessionUseNoEffect; /* "PATIENCE IS A VIRTUE." */
            }
            if (id == RelicNuoreCache) {
                relicAddCache(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2));
            } else if (id == RelicMagicOreCache) {
                relicAddCache(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1));
            } else if (id == RelicMassHeal) {
                relicMassHealAndOverheal(&s->save);
            } else {
                unsigned monster = 0;
                if (!s->combat.active || !combatSelectActiveMonster(s->combat.order, s->combat.count, s->combat.defeated, &monster)) {
                    return SessionUseNoEffect;
                }
                relicInstantKill(s->combat.slots + (size_t)monster * MonsterRecordSize);
            }
            partyConsumeItemCharge(user, &s->items, slot);
            return SessionUseDone;
        }
        if (action == RelicActionLocationPotion) {
            if (!relicUseLocationPotion(s->x, s->y, flags, SaveHeaderGlobalFlagsSize)) {
                return SessionUseNoEffect;
            }
            partyConsumeItemCharge(user, &s->items, slot);
            return SessionUseDone;
        }
        if (action == RelicActionAssemblePotion) {
            uint8_t globalSlots[24];
            memset(globalSlots, 0, sizeof(globalSlots));
            if (!relicAssemblePotion(globalSlots, &s->save, &s->items)) {
                return SessionUseNoEffect;
            }
            s->heldItem = RelicPotionOfAppreciation;
            s->heldExtra = 0;
            return SessionUseDone;
        }
        if (action == RelicActionPlayFlute) {
            s->pendingMusic = RelicFluteTrack;
            return SessionUseDone;
        }
    }
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
        if (s->game == GameYendor2 && s->shareware && s->x == 0x200 && s->y == 0x2B && s->facing == SaveFacingSouth) { /* EnforceDemoBoundary comes before the destination's own checks */
            out.kind = SessionInteractDemoLimit;
            out.textOffset = 0x8FD2;
            out.textLines = 2;
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

/*
 * What ShowItemUsagePreview does when a topic loads a service's type word (yendor2.asm:22337): it marks what the speaker can be offered by rewriting the topic masks --
 * the healer classifies the member's condition, the trainer checks for a level-up (CheckAndAnnounceLevelUp: mask A keeps its low 13 bits, gets 0x1000 and, when a
 * level is pending, 0x8000), the challenge NPCs mark whether the member already won, the mount sellers collect the topics the member's abilities allow
 * (AccumulateLearnedAbilityFlags: mask A / B become the OR of the own masks of every topic whose argument has bit 1, or whose flag 0x8 matches an ability the member has).
 */
static void refreshService(GameSession *s) {
    uint8_t *member = sessionPartyRecord(s, s->talkSpeaker);
    const uint8_t *npc = s->talk.npc;
    if (!member || !npc) {
        return;
    }
    switch (dialogServiceForType(s->talk.typeWord)) {
    case DialogServiceHealer:
        dialogClassifyCondition(&s->talk.masks, member);
        break;
    case DialogServiceTrainer:
        s->talk.masks.availA = (uint16_t)((s->talk.masks.availA & 0x1FFF) | 0x1000);
        if (partyCheckForLevelUp(member, s->game)) {
            s->talk.masks.availA |= 0x8000;
        }
        break;
    case DialogServiceChallenge:
        dialogMarkServiceAvailability(&s->talk.masks, npc, member);
        break;
    case DialogServiceMounts: {
        uint16_t abilities = partyGetU16(member, PartyFieldAbilities);
        s->talk.masks.availA = s->talk.masks.availB = 0;
        unsigned first = dialogGetU16(npc, DialogNpcFirstTopic), count = dialogGetU16(npc, DialogNpcTopicCount);
        for (unsigned n = 0; n < count; n++) {
            const uint8_t *topic = dialogTopic(&s->dialogs, first + n);
            if (!topic) {
                continue;
            }
            if ((dialogTopicU16(&s->dialogs, topic, DialogTopicArg) & 1) ||
                ((dialogGetU16(topic, DialogTopicFlags) & 8) && (dialogTopicU16(&s->dialogs, topic, DialogTopicTextOffset) & abilities))) {
                s->talk.masks.availA |= dialogTopicU16(&s->dialogs, topic, DialogTopicOwnMaskA);
                s->talk.masks.availB |= dialogTopicU16(&s->dialogs, topic, DialogTopicOwnMaskB);
            }
        }
        break;
    }
    default:
        return;
    }
    conversationList(&s->talk, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
}

/*
 * The services a type word hands every topic to (UseHealingItem, UseTrainingItem, UseItemType_800, UseAbilityScroll; yendor2.asm:21328, :21510, :20987, :21916). They
 * all read the topic's argument as a small command: 1 = leave the service (the topic only visits), 4 = refresh what the member can be offered (the healer classifies
 * the member's condition, the trainer checks for a level-up, the challenge marks whether the member has won, the mount seller nothing), 2 = accept (pay the quote
 * and do it), and a higher bit = a quote (the healer's 0x1000 / 0x2000 / 0x4000 / 0x8000 types, the trainer's 0x800, the challenge's 0x400), which stores the cost
 * in `serviceCost`.
 */
static SessionService serviceByType(GameSession *s, const ConversationStep *step) {
    uint8_t *member = sessionPartyRecord(s, s->talkSpeaker);
    const uint8_t *npc = s->talk.npc;
    if (!member) {
        return SessionServiceFailed;
    }
    unsigned arg = step->arg;
    Bcd4 *cost = &s->serviceCost;
    uint8_t *gold = saveHeaderBcd4(&s->save, SaveHeaderGold);
    if (arg & 1) {
        return SessionServiceDone;
    }
    switch (step->typeService) {
    case DialogServiceHealer:
        if (arg & 4) {
            dialogClassifyCondition(&s->talk.masks, member);
        } else if (arg & 2) {
            if (!dialogApplyHealing(s->serviceType, *cost, gold, member, s->game)) {
                return SessionServiceFailed;
            }
            dialogClassifyCondition(&s->talk.masks, member);
        } else if (arg & 0xF000) {
            s->serviceType = (uint16_t)(arg & 0xF000);
            dialogHealingCost(npc, s->serviceType, &s->talk.masks, member, *cost);
        }
        break;
    case DialogServiceTrainer:
        if (arg & 4) {
            partyCheckForLevelUp(member, s->game);
        } else if (arg & 2) {
            if (partyApplyTraining(member, s->game, &s->save, *cost) != PartyTrainOutcomeApplied) {
                return SessionServiceFailed;
            }
            partyRecomputeEquipmentStatBonuses(member, &s->items, s->game);
        } else if (arg & 0x800) {
            if (!dialogTrainingQuote(npc, member, *cost)) {
                return SessionServiceFailed;
            }
        }
        break;
    case DialogServiceChallenge:
        if (arg & 4) {
            dialogMarkServiceAvailability(&s->talk.masks, npc, member);
        } else if (arg & 2) {
            Bcd4 reward;
            memset(reward, 0, sizeof(reward));
            DialogChallengeOutcome outcome = dialogAttemptChallenge(npc, member, gold, reward);
            s->serviceType = (uint16_t)outcome;
            if (outcome == DialogChallengeNoGold) {
                return SessionServiceFailed;
            }
        } else if (arg & 0x400) {
            dialogNpcFee(npc, *cost);
        }
        break;
    case DialogServiceMounts: {
        const uint8_t *topic = dialogTopic(&s->dialogs, step->topic);
        uint16_t mask = topic ? dialogTopicU16(&s->dialogs, topic, DialogTopicTextOffset) : 0;
        if (arg & 2) {
            if (s->talk.typeWord & 0x2000) {
                dialogSellTransport(mask, member, gold);
            } else if (dialogLearnTransport(mask, member, gold) != DialogTransportLearned) {
                return SessionServiceFailed;
            }
        }
        break;
    }
    default:
        return SessionServiceNone;
    }
    conversationList(&s->talk, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
    return SessionServiceDone;
}

static const uint8_t *haggler(GameSession *s);

/* The sell, enhance and repair topics work on the cursor item (the original's screens ask the player to put it down; here it is `heldItem`). */
static SessionService itemService(GameSession *s, const ConversationStep *step) {
    if (!s->heldItem) {
        return SessionServiceNeedItem;
    }
    const uint8_t *npc = s->talk.npc;
    const uint8_t *record = itemCatalogRecord(&s->items, s->heldItem);
    uint8_t *gold = saveHeaderBcd4(&s->save, SaveHeaderGold);
    Bcd4 amount;
    if (!record) {
        return SessionServiceFailed;
    }
    if (step->flags & DialogTopicSell) {
        if (!dialogSellAccepts(step->arg, record)) {
            return SessionServiceFailed;
        }
        memcpy(amount, record + ItemFieldBaseValue, 4);
        Bcd4 price;
        shopSellPrice(amount, (uint16_t)partyGetStat(haggler(s), PartyStatBartering), price);
        bcd4Add(gold, price);
        s->heldItem = s->heldExtra = 0;
        return SessionServiceDone;
    }
    if (step->flags & DialogTopicEnhance) {
        if (!dialogEnhanceEligible(npc, &s->items, record) || !dialogEnhanceCost(npc, &s->items, s->heldItem, amount) || bcd4Compare(gold, amount) < 0) {
            return SessionServiceFailed;
        }
        bcd4Sub(gold, amount);
        s->heldItem = (uint16_t)(s->heldItem + 1);
        return SessionServiceDone;
    }
    /* repair: the damaged item holds its original id in its extra word */
    const uint8_t *original = s->heldExtra ? itemCatalogRecord(&s->items, s->heldExtra) : NULL;
    if (!original || !dialogRepairEligible(&s->items, original) || !dialogRepairCost(npc, &s->items, s->heldExtra, amount) || bcd4Compare(gold, amount) < 0) {
        return SessionServiceFailed;
    }
    bcd4Sub(gold, amount);
    s->heldItem = s->heldExtra;
    s->heldExtra = 0;
    return SessionServiceDone;
}

bool sessionTalkStart(GameSession *s, unsigned npcId, unsigned speaker, ConversationStep *first) {
    const uint8_t *npc = dialogNpc(&s->dialogs, npcId);
    const uint8_t *voice = sessionPartyRecord(s, speaker);
    if (!npc || s->combat.active || s->combat.wiped || !voice) {
        return false;
    }
    if (conversationNeedsSpeaker(npc) && (partyGetU16(voice, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
        return false; /* LoadItemData asks again for a member who can act */
    }
    s->talkSpeaker = speaker;
    memset(s->serviceCost, 0, sizeof(s->serviceCost));
    s->serviceType = 0;
    uint8_t globalSlots[24];
    memset(globalSlots, 0, sizeof(globalSlots));
    unsigned item = dialogGetU16(npc, DialogNpcRequiredItemRange);
    bool held = item && itemRangeAvailable(globalSlots, &s->save, (uint16_t)item, (uint16_t)item).found;
    s->talking = conversationStart(&s->talk, &s->dialogs, npcId, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize, held, first);
    if (s->talking && s->talk.ended) {
        s->talking = false;
    }
    refreshService(s);
    return s->talking || (first && first->ended);
}

ConversationStep sessionTalkChoose(GameSession *s, unsigned number) {
    DialogService before = dialogServiceForType(s->talk.typeWord);
    ConversationStep step = conversationChoose(&s->talk, number, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
    if (before == DialogServiceNone) {
        refreshService(s);
    }
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
    if (step->typeService != DialogServiceNone && !(step->flags & (DialogTopicAttributeBoost | DialogTopicExperienceBoost))) {
        return serviceByType(s, step);
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
    if (step->flags & (DialogTopicRepair | DialogTopicEnhance | DialogTopicSell)) {
        return itemService(s, step);
    }
    if (step->flags & (DialogTopicUseKey | DialogTopicCheckKey)) {
        return SessionServiceNeedScreen;
    }
    return SessionServiceNone;
}

/* Chapter 2's light-source counters and spell timers in the save header (word_36C85 / 36C89 / 36C8B, 36C93-36C9D: header + 0x26, 0x2A, 0x2C and 0x34-0x3E). */
static const unsigned kLightCountOffset[LightSourceCount] = {0x26, 0x2A, 0x2C};
enum { LightTimerOffset = 0x34 };

static void syncHeader(GameSession *s) {
    saveHeaderSetU16(&s->save, SaveHeaderWorldX, (uint16_t)s->x);
    saveHeaderSetU16(&s->save, SaveHeaderWorldY, (uint16_t)s->y);
    saveHeaderSetU16(&s->save, SaveHeaderFacing, s->facing);
    saveHeaderSetU16(&s->save, SaveHeaderClockMinutes, s->clock.minutes);
    saveHeaderSetU16(&s->save, SaveHeaderGameDay, s->clock.day);
    saveHeaderSetU16(&s->save, SaveHeaderGameMonth, s->clock.month);
    saveHeaderSetU16(&s->save, SaveHeaderGameYear, s->clock.year);
    storePlace(s);
    if (s->game == GameYendor2) {
        for (unsigned i = 0; i < LightSourceCount; i++) {
            saveHeaderSetU16(&s->save, kLightCountOffset[i], s->light.litCount[i]);
        }
        for (unsigned i = 0; i < LightSpellTimerCount; i++) {
            saveHeaderSetU16(&s->save, LightTimerOffset + 2 * i, s->light.timers[i]);
        }
    }
}

size_t sessionSave(GameSession *s, uint8_t *out, size_t capacity) {
    syncHeader(s);
    memcpy(saveGameSection(&s->save, SaveSectionMonsters), s->pool, sizeof(s->pool));
    return saveGameStore(&s->save, out, capacity);
}

bool sessionLoad(GameSession *s, const uint8_t *data, size_t size) {
    SaveGame loaded;
    if (!saveGameLoad(&loaded, data, size) || loaded.kind != s->game) {
        return false;
    }
    s->save = loaded;
    s->x = saveHeaderGetU16(&s->save, SaveHeaderWorldX);
    s->y = saveHeaderGetU16(&s->save, SaveHeaderWorldY);
    s->facing = saveHeaderGetU16(&s->save, SaveHeaderFacing);
    s->clock.minutes = saveHeaderGetU16(&s->save, SaveHeaderClockMinutes);
    s->clock.day = saveHeaderGetU16(&s->save, SaveHeaderGameDay);
    s->clock.month = saveHeaderGetU16(&s->save, SaveHeaderGameMonth);
    s->clock.year = saveHeaderGetU16(&s->save, SaveHeaderGameYear);
    *placeWord(s) = saveHeaderGetU16(&s->save, saveHeaderPlaceFlagsOffset(s->game));
    if (s->game == GameYendor2) {
        for (unsigned i = 0; i < LightSourceCount; i++) {
            s->light.litCount[i] = saveHeaderGetU16(&s->save, kLightCountOffset[i]);
        }
        for (unsigned i = 0; i < LightSpellTimerCount; i++) {
            s->light.timers[i] = saveHeaderGetU16(&s->save, LightTimerOffset + 2 * i);
        }
    }
    memcpy(s->pool, saveGameSection(&s->save, SaveSectionMonsters), sizeof(s->pool));
    memset(&s->combat, 0, sizeof(s->combat));
    s->talking = false;
    s->pendingTravel = 0;
    s->pendingSound = 0;
    s->heldItem = s->heldExtra = 0;
    sessionRebuild(s);
    return true;
}

/* The party member who haggles (g_partyRoleAssignment1, the speaker): the record id in the first role slot, else the first member. */
static const uint8_t *haggler(GameSession *s) {
    unsigned id = saveHeaderGetU16(&s->save, SaveHeaderRoleAssignments);
    const uint8_t *record = id ? saveGamePartyRecordById(&s->save, id) : NULL;
    return record ? record : sessionPartyRecord(s, 0);
}

static bool shopRecord(GameSession *s, unsigned lockId, LockRecord *lock) {
    return lockCatalogRecord(&s->locks, lockId, lock);
}

unsigned sessionShopStock(GameSession *s, unsigned lockId, SessionShopItem out[LockContentSlots]) {
    LockRecord lock;
    memset(out, 0, sizeof(SessionShopItem) * LockContentSlots);
    if (!shopRecord(s, lockId, &lock)) {
        return 0;
    }
    ChestSlot slots[LockContentSlots];
    chestSlots(&lock, 0, &s->items, slots);
    uint16_t bartering = (uint16_t)partyGetStat(haggler(s), PartyStatBartering);
    unsigned count = 0;
    for (unsigned i = 0; i < LockContentSlots; i++) {
        out[i].itemId = slots[i].itemId;
        out[i].extra = slots[i].value;
        const uint8_t *record = slots[i].itemId ? itemCatalogRecord(&s->items, slots[i].itemId) : NULL;
        if (record) {
            Bcd4 base;
            memcpy(base, record + ItemFieldBaseValue, 4);
            shopBuyPrice(base, bartering, out[i].price);
            count++;
        }
    }
    return count;
}

SessionShop sessionShopBuy(GameSession *s, unsigned lockId, unsigned slot) {
    SessionShopItem stock[LockContentSlots];
    if (slot >= LockContentSlots || !sessionShopStock(s, lockId, stock) || !stock[slot].itemId) {
        return SessionShopEmpty;
    }
    if (s->heldItem) {
        return SessionShopHandFull;
    }
    bool drained;
    if (!shopPay(saveHeaderBcd4(&s->save, SaveHeaderGold), stock[slot].price, &drained)) {
        return SessionShopNoGold;
    }
    s->heldItem = stock[slot].itemId;
    s->heldExtra = stock[slot].extra;
    say(s, "bought item %u", stock[slot].itemId);
    return SessionShopBought;
}

SessionShop sessionShopSell(GameSession *s, unsigned lockId) {
    SessionShopItem stock[LockContentSlots];
    if (!s->heldItem) {
        return SessionShopEmpty;
    }
    if (!sessionShopStock(s, lockId, stock)) {
        return SessionShopNotStocked;
    }
    bool stocked = false;
    for (unsigned i = 0; i < LockContentSlots; i++) {
        stocked = stocked || stock[i].itemId == s->heldItem;
    }
    const uint8_t *record = itemCatalogRecord(&s->items, s->heldItem);
    if (!stocked || !record) {
        return SessionShopNotStocked;
    }
    Bcd4 base, price;
    memcpy(base, record + ItemFieldBaseValue, 4);
    shopSellPrice(base, (uint16_t)partyGetStat(haggler(s), PartyStatBartering), price);
    bcd4Add(saveHeaderBcd4(&s->save, SaveHeaderGold), price);
    say(s, "sold item %u", s->heldItem);
    s->heldItem = s->heldExtra = 0;
    return SessionShopSold;
}

static uint16_t itemWeightOf(GameSession *s, unsigned id) {
    const uint8_t *item = itemCatalogRecord(&s->items, id);
    return item ? (uint16_t)(item[ItemFieldWeight] | (item[ItemFieldWeight + 1] << 8)) : 0;
}

bool sessionDropOnPortrait(GameSession *s, unsigned member) {
    uint8_t *record = sessionPartyRecord(s, member);
    const uint8_t *item = s->heldItem ? itemCatalogRecord(&s->items, s->heldItem) : NULL;
    if (!record || !item) {
        return false;
    }
    const uint8_t *target = itemTargetEntry(&s->items, item);
    uint16_t weight = itemWeightOf(s, s->heldItem);
    AutoEquipPlan plan = inventoryPlanAutoEquip(s->game, record, s->heldItem, weight, itemGetU16(item, ItemFieldFlags), target ? itemTargetWord(target, ItemTargetSlotFlags) : 0);
    if (plan.kind == AutoEquipRefused) {
        return false;
    }
    uint8_t *slot = record + plan.slotOffset;
    if (plan.storesExtra) {
        itemSlotSet(slot, s->heldItem, s->heldExtra);
    } else {
        partySetU16(record, plan.slotOffset, s->heldItem);
    }
    if (plan.wearOffset) {
        partySetU16(record, plan.wearOffset, 0);
    }
    if (plan.appliesStatEffect) {
        partyApplyMultiStatEffect(record, itemEffectEntry(&s->items, item));
    }
    if (plan.addsWeight) {
        uint8_t *group = partyInventoryGroup(record, PartyGroupMain);
        inventoryGroupSetWeight(group, (uint16_t)(inventoryGroupWeight(group) + weight));
    }
    partyRecomputeEquipmentStatBonuses(record, &s->items, s->game);
    s->heldItem = s->heldExtra = 0;
    return true;
}

bool sessionPickUp(GameSession *s, unsigned member, unsigned slotNumber, unsigned equipmentCode) {
    uint8_t *record = sessionPartyRecord(s, member);
    if (!record || s->heldItem) {
        return false;
    }
    uint8_t *slot = equipmentCode ? partyEquipmentSlot(record, equipmentCode, s->game) : inventoryGroupSlot(partyInventoryGroup(record, PartyGroupMain), slotNumber);
    bool shortSlot = equipmentCode >= PartyEquipmentFirstShort;
    unsigned id = !slot ? 0 : shortSlot ? (unsigned)(slot[0] | (slot[1] << 8)) : itemSlotId(slot);
    if (!id) {
        return false;
    }
    s->heldItem = (uint16_t)id;
    s->heldExtra = shortSlot ? 0 : itemSlotExtra(slot);
    if (shortSlot) {
        slot[0] = slot[1] = 0;
    } else {
        itemSlotSet(slot, 0, 0);
    }
    const uint8_t *item = itemCatalogRecord(&s->items, id);
    if (equipmentCode && equipmentCode != 0x0B && item) { /* equipped gear carries its stat effect */
        partyRemoveMultiStatEffect(record, itemEffectEntry(&s->items, item));
    }
    uint8_t *group = partyInventoryGroup(record, PartyGroupMain);
    uint16_t weight = itemWeightOf(s, id);
    inventoryGroupSetWeight(group, (uint16_t)(inventoryGroupWeight(group) - weight));
    partyRecomputeEquipmentStatBonuses(record, &s->items, s->game);
    return true;
}

bool sessionStowHeld(GameSession *s, unsigned member) {
    uint8_t *record = sessionPartyRecord(s, member);
    if (!record || !s->heldItem) {
        return false;
    }
    uint8_t *group = partyInventoryGroup(record, PartyGroupMain);
    for (unsigned slot = 1; slot <= InventorySlotCount; slot++) {
        uint8_t *cell = inventoryGroupSlot(group, slot);
        if (cell && itemSlotId(cell) == 0) {
            const uint8_t *item = itemCatalogRecord(&s->items, s->heldItem);
            itemSlotSet(cell, s->heldItem, s->heldExtra);
            inventoryGroupSetWeight(group, (uint16_t)(inventoryGroupWeight(group) + (item ? item[ItemFieldWeight] | (item[ItemFieldWeight + 1] << 8) : 0)));
            s->heldItem = s->heldExtra = 0;
            return true;
        }
    }
    return false;
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
