#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chest.h"
#include "container.h"
#include "explore.h"
#include "globalflags.h"
#include "lighting.h"
#include "newgame.h"
#include "thrown.h"
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

static unsigned keyRingId(GameKind game);
static bool isRingKey(GameKind game, unsigned id);

/* The six tool belt slots of the save header (DS:0x9519; savegame.h SaveHeaderBeltSlots): what IsItemRangeAvailable, the rest's food search and the story-item checks look at first. */
static uint8_t *beltSlots(GameSession *s) {
    return saveHeaderBcd4(&s->save, SaveHeaderBeltSlots);
}
static void arriveAt(GameSession *s, int x, int y, uint16_t facing);
static unsigned bagLevel(uint8_t *record);
static uint8_t *bagGroup(uint8_t *record, unsigned level);

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

/* A sound for the front end, after those already asked for (TriggerSoundEventAfterDriverWait). */
static void queueSound(GameSession *s, unsigned id) {
    if (id && s->soundCount < sizeof(s->soundQueue) / sizeof(s->soundQueue[0])) {
        s->soundQueue[s->soundCount++] = id;
    }
}

unsigned sessionNextSound(GameSession *s) {
    if (!s->soundCount) {
        return 0;
    }
    unsigned id = s->soundQueue[0];
    memmove(s->soundQueue, s->soundQueue + 1, (s->soundCount - 1) * sizeof(s->soundQueue[0]));
    s->soundCount--;
    return id;
}

/* ApplyEffectAndDrawIconBar plays the sound of the effect it applies (effect.h: +0 of its row), once per call. */
static void effectSound(GameSession *s, unsigned effectId) {
    EffectDef def;
    if (effectGetDef(s->game, effectId, &def)) {
        queueSound(s, def.sound);
    }
}

/* The sound ids a fight is played with (HandleDungeonInput: a miss `_val43`; ShowPartyWipeScreen is the front end's): Chapter 2 / Chapter 3. */
static unsigned missSound(const GameSession *s) {
    return s->game == GameYendor2 ? 0x1E : 0x23;
}

static void combatNewRound(GameSession *s) {
    SessionCombat *c = &s->combat;
    memset(c->defeated, 0, sizeof(c->defeated)); /* BuildCombatTurnOrder makes fresh entries: nobody is struck off yet */
    c->count = combatBuildTurnOrder(&s->save, c->slots, &s->rng, c->order, c->targets);
    c->cursor = 0;
}

/* g_activeCombatMonster: the chosen monster while it lives, else (SelectActiveMonster) the first live one in the turn order. */
static unsigned combatActiveSlot(SessionCombat *c) {
    if (c->target < CombatMonsterSlotCount && monsterGetU16(c->slots + (size_t)c->target * MonsterRecordSize, MonsterFieldType) && !c->defeated[c->target]) {
        return c->target;
    }
    unsigned slot = CombatMonsterSlotCount;
    c->target = combatSelectActiveMonster(c->order, c->count, c->defeated, &slot) ? slot : CombatMonsterSlotCount;
    return c->target;
}

unsigned sessionCombatTarget(GameSession *s) {
    return s->combat.active ? combatActiveSlot(&s->combat) : CombatMonsterSlotCount;
}

bool sessionCombatChoose(GameSession *s, unsigned slot) {
    SessionCombat *c = &s->combat;
    if (!c->active || slot >= CombatMonsterSlotCount || !monsterGetU16(c->slots + (size_t)slot * MonsterRecordSize, MonsterFieldType) || c->defeated[slot]) {
        return false;
    }
    c->target = slot;
    return true;
}

static unsigned slotType(const SessionCombat *c, unsigned slot) {
    return monsterGetU16(c->slots + (size_t)slot * MonsterRecordSize, MonsterFieldType);
}

static void slotLayer(SessionCombat *c, unsigned slot, int delta) {
    uint8_t *record = c->slots + (size_t)slot * MonsterRecordSize;
    monsterSetU16(record, MonsterFieldAnimSet, (uint16_t)(monsterGetU16(record, MonsterFieldAnimSet) + delta));
}

/* Moves a combat slot's record to another (a copy, then the source is cleared); the turn order, the struck-off marks and the targets follow it. */
static void slotMove(SessionCombat *c, unsigned from, unsigned to) {
    uint8_t *source = c->slots + (size_t)from * MonsterRecordSize;
    memcpy(c->slots + (size_t)to * MonsterRecordSize, source, MonsterRecordSize);
    memset(source, 0, MonsterRecordSize);
    for (unsigned i = 0; i < c->count; i++) {
        if (c->order[i].isMonster && c->order[i].index == from) {
            c->order[i].index = to;
        }
    }
    c->defeated[to] = c->defeated[from];
    c->defeated[from] = false;
    if (c->target == from) {
        c->target = to;
    }
    c->targets[to] = c->targets[from];
    c->targets[from] = 0;
}

/*
 * CompactMonsterSlots (yendor2.asm:33995): one step toward a tidy formation, run by ProcessCombatRound whenever a monster is left. A left monster with the right slot free: the middle one (or, with the
 * middle empty, the left one) moves one place right (its layer + 1); with the left empty and the right occupied: the middle one (or the right one) moves one place left (layer - 1). A full or a
 * left-and-right pair is left as it is.
 */
static void combatCompact(SessionCombat *c) {
    bool left = slotType(c, 0) != 0, middle = slotType(c, 1) != 0, right = slotType(c, 2) != 0;
    if (left) {
        if (right) {
            return;
        }
        unsigned from = middle ? 1 : 0;
        slotLayer(c, from, 1);
        slotMove(c, from, from + 1);
    } else if (right) {
        unsigned from = middle ? 1 : 2;
        slotLayer(c, from, -1);
        slotMove(c, from, from - 1);
    }
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

/* The rewards of a victory go to the party; when there is experience they are also kept to be shown (ShowLootAndAwardExperience) and the "treasure" sound (_val16, 7) is asked for. */
static void stagingAdd(MonsterRewardStaging *to, const MonsterRewardStaging *from) {
    bcd4Add(to->gold, from->gold);
    bcd4Add(to->ore, from->ore);
    bcd4Add(to->nuore, from->nuore);
    bcd4Add(to->experience, from->experience);
}

static void awardLoot(GameSession *s, const MonsterRewardStaging *given) {
    MonsterRewardStaging total = *given; /* the rewards of monsters that timed out earlier are still staged: they go with this victory */
    stagingAdd(&total, &s->staged);
    memset(&s->staged, 0, sizeof(s->staged));
    const MonsterRewardStaging *staging = &total;
    monsterRewardsAward(&s->save, s->game, staging);
    static const uint8_t zero[4] = {0, 0, 0, 0};
    if (memcmp(staging->experience, zero, 4) != 0) {
        s->loot.pending = true;
        memcpy(s->loot.gold, staging->gold, 4);
        memcpy(s->loot.ore, staging->ore, 4);
        memcpy(s->loot.nuore, staging->nuore, 4);
        memcpy(s->loot.experience, staging->experience, 4);
        if (!s->pendingSound) {
            s->pendingSound = 7;
        }
    }
}

static bool monstersTakeTurns(GameSession *s);

/* ProcessCombatRound after a combatant has acted: reap the dead, then the next combatant. */
static void combatAfterAction(GameSession *s) {
    SessionCombat *c = &s->combat;
    CombatRoundOutcome outcome = combatProcessRound(c->slots, c->order, c->count, c->defeated, &c->cursor, &c->staging, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
    if (outcome == CombatRoundNoMonstersLeft) {
        awardLoot(s, &c->staging);
        memset(&c->staging, 0, sizeof(c->staging));
        c->active = false;
        snprintf(s->log, sizeof(s->log), "victory: the loot and experience are in the party's totals");
    } else {
        combatCompact(c);
        if (outcome == CombatRoundNewRound) { /* ProcessLevelMonsters and the ambushes between two rounds (RunDungeonGameLoop :16377): the monsters outside walk, and may join */
            monstersTakeTurns(s);
            sessionAmbushShots(s);
            if (c->active) {
                combatNewRound(s);
            }
        }
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
        if (out.tick == MonsterTickIdle) { /* ProcessMonsterAttackTurn: its hit sound (+0x5C) when an attack lands, else its idle one (+0x5E) */
            queueSound(s, monsterGetU16(monster, out.attacked ? MonsterFieldHitSound : MonsterFieldIdleSound));
        }
        say(s, out.attacked ? "monster %u hits the party" : "monster %u misses or waits", monsterGetU16(monster, MonsterFieldType));
        combatCheckWipe(s);
        if (!c->active) {
            return;
        }
        combatAfterAction(s);
    }
}

/*
 * A monster steps onto the party's cell (ProcessLevelMonsters, :22FCC). The first one starts the fight: it stands in the middle slot, and its flags' top three bits are the formation. When a fight
 * is on already, a monster that fights in a group (flags 0x8000, the formation's too) joins: with the left slot free it goes there and the middle one moves to the right; with the left taken and the
 * middle free (a formation with room for a third: 0x2000, and the monster has 0x2000 too) the left one moves to the middle and it takes the left. Anything else stays outside. True when it was taken.
 */
static bool combatEngage(GameSession *s, uint8_t *poolRecord) {
    SessionCombat *c = &s->combat;
    uint16_t flags = (uint16_t)monsterGetU16(poolRecord, MonsterFieldFlags);
    if (!c->active) {
        memset(c, 0, sizeof(*c));
        memcpy(c->slots + MonsterRecordSize, poolRecord, MonsterRecordSize);
        monsterPoolRemove(poolRecord, &s->grid);
        c->active = true;
        c->formation = flags & 0xE000;
        c->target = CombatMonsterSlotCount;
        return true;
    }
    if (!(flags & 0x8000) || !(c->formation & 0x8000)) {
        return false;
    }
    bool left = slotType(c, 0) != 0;
    if (!left || (c->formation & 0x2000)) {
        if (left && (slotType(c, 1) || !(flags & 0x2000))) {
            return false;
        }
        if (!left) { /* the middle one steps to the right, the new one stands on the left */
            slotLayer(c, 1, 1);
            slotMove(c, 1, 2);
        } else { /* the left one steps to the middle */
            slotLayer(c, 0, 1);
            slotMove(c, 0, 1);
        }
        memcpy(c->slots, poolRecord, MonsterRecordSize);
        monsterPoolRemove(poolRecord, &s->grid);
        slotLayer(c, 0, -1);
        if (!left) {
            uint16_t either = (uint16_t)(monsterGetU16(c->slots, MonsterFieldFlags) | monsterGetU16(c->slots + 2 * MonsterRecordSize, MonsterFieldFlags));
            c->formation = (c->formation & 0x9FFF) | ((either & 0x4000) ? 0x4000 : 0x2000);
        }
        return true;
    }
    return false;
}

/* The fight begins (RunDungeonGameLoop :16348 on): the turn order, and the monsters whose turn comes first. */
static void combatBegin(GameSession *s) {
    combatNewRound(s);
    snprintf(s->log, sizeof(s->log), "combat");
    combatRunMonsters(s);
}

static void combatStart(GameSession *s, uint8_t *poolRecord) {
    combatEngage(s, poolRecord);
    combatBegin(s);
}

bool sessionCombatEngage(GameSession *s, uint8_t *poolRecord) {
    bool first = !s->combat.active;
    bool taken = combatEngage(s, poolRecord);
    if (first && taken) {
        combatBegin(s);
    } else if (taken) { /* a join belongs to a round boundary: the turn order is built afresh with it */
        combatNewRound(s);
    }
    return taken;
}

/*
 * ProcessSideTrapsOnMovement (yendor2.asm:32653), run after every monster pass: each pool monster whose approach check armed an ambush (wound bit 0x1000) shoots at a random member.
 * TriggerSideTrapForRandomPartyMember picks among the first three party slots (RandomInRange(3), until one is occupied and able to act), rolls
 * RollTrapAvoidanceMagnitude on the member's EquipRating5 against the monster's ranged accuracy / damage (combatRollTrapAvoidanceMagnitude: Chapter 3 rolls out of 55), keeps the result in the
 * member's slot of a four-entry table (a later shot at the same member overwrites an earlier one, effect and sound included), and clears the direction bits. PresentTriggeredSideTrapEffects then
 * plays the first sound and applies every nonzero magnitude as the monster's ranged effect (+0x70) through the icon-bar pipeline. The animation (the direction bit that matches the party's
 * facing decides whether the shot is seen) is the front end's.
 */
bool sessionAmbushShots(GameSession *s) {
    struct {
        uint16_t magnitude, effect, sound;
        uint8_t *member;
    } table[4];
    memset(table, 0, sizeof(table));
    bool any = false;
    for (unsigned slot = 0; slot < MonsterPoolSize; slot++) {
        uint8_t *record = s->pool + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0 || !(monsterGetU16(record, MonsterFieldWound) & MonsterWoundAmbushPending)) {
            continue;
        }
        any = true;
        unsigned pick = 0;
        bool found = false;
        for (unsigned tries = 0; tries < 64 && !found; tries++) { /* the original retries forever; a party with nobody able in the first three slots would hang it */
            pick = randomInRange(&s->rng, 3);
            const uint8_t *member = sessionPartyRecord(s, pick);
            found = member && !(partyGetU16(member, PartyFieldStatusFlags) & PartyStatusIncapacitated);
        }
        uint16_t wound = monsterGetU16(record, MonsterFieldWound);
        monsterSetU16(record, MonsterFieldWound, wound & 0xE0FF);
        if (!found) {
            continue;
        }
        uint8_t *member = sessionPartyRecord(s, pick);
        CombatSideTrapOutcome shot = combatResolveSideTrap(s->game, s->facing, wound, partyGetStat(member, PartyStatEquipRating5),
                                                           monsterGetU16(record, MonsterFieldRangedAccuracy), monsterGetU16(record, MonsterFieldRangedDamage), &s->rng);
        table[pick].magnitude = shot.magnitude;
        table[pick].member = member;
        table[pick].effect = monsterGetU16(record, MonsterFieldRangedEffect);
        table[pick].sound = monsterGetU16(record, MonsterFieldRangedSound);
    }
    if (!any) {
        return false;
    }
    bool shotSound = false;
    for (unsigned i = 0; i < 4 && !shotSound; i++) { /* the first sound of the table plays; the driver is busy then, so the effect's own is skipped */
        if (table[i].sound) {
            queueSound(s, table[i].sound);
            shotSound = true;
        }
    }
    bool hit = false;
    for (unsigned i = 0; i < 4; i++) {
        if (table[i].magnitude && table[i].member) {
            combatApplyPresetEffect(table[i].member, &s->save, s->game, table[i].effect, table[i].magnitude, 0, &s->rng);
            if (!hit && !shotSound) {
                effectSound(s, table[i].effect);
            }
            hit = true;
        }
    }
    if (hit) {
        snprintf(s->log, sizeof(s->log), "an ambush hits the party");
        combatCheckWipe(s);
    }
    return hit;
}

/*
 * ProcessLevelMonsters (yendor2.asm:33366): every live monster takes its turn, all 80 slots whether or not a fight is on. One that steps onto the party's cell starts the fight, or -- when one is on
 * already -- joins it (combatEngage); the fight itself begins after the pass. The rewards of monsters whose timers ran out are staged: with no fight on, they are awarded at the end of the pass
 * once they hold experience (the shown loot panel), else they wait for the next victory.
 */
static bool monstersTakeTurns(GameSession *s) {
    bool wasActive = s->combat.active;
    MonsterRewardStaging staging;
    memset(&staging, 0, sizeof(staging));
    for (unsigned slot = 0; slot < MonsterPoolSize; slot++) {
        uint8_t *record = s->pool + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) == 0) {
            continue;
        }
        MonsterFullTurn turn = monsterPoolTakeTurn(record, s->game, &s->map, &s->grid, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize, &staging, s->x, s->y, &s->rng);
        if (turn.move == MonsterMoveEngaged) {
            combatEngage(s, record);
        }
    }
    static const uint8_t zero[4] = {0, 0, 0, 0};
    if (s->combat.active) {
        stagingAdd(&s->combat.staging, &staging);
    } else {
        stagingAdd(&s->staged, &staging);
        if (memcmp(s->staged.experience, zero, 4) != 0) {
            MonsterRewardStaging none;
            memset(&none, 0, sizeof(none));
            awardLoot(s, &none);
        }
    }
    if (!wasActive && s->combat.active) {
        combatBegin(s);
    }
    return s->combat.active;
}

/*
 * The party has moved onto a cell: a monster standing on it is engaged at once (HandleMovementInput, loc_115C8 on, and the same scan in the jump spell, loc_2C4F5): the 80 pool slots are
 * searched for the first whose cell is the party's, and it goes to a combat slot -- aware or not, and whether it could have walked there or not. This is what happens when the party's
 * two steps through a doorway end on the cell of a monster that came up behind it. True when combat has begun.
 */
static bool engageMonsterOnPartyCell(GameSession *s) {
    for (unsigned slot = 0; slot < MonsterPoolSize; slot++) {
        uint8_t *record = s->pool + (size_t)slot * MonsterRecordSize;
        if (monsterGetU16(record, MonsterFieldType) != 0 && (int)monsterGetU16(record, MonsterFieldWorldX) == s->x && (int)monsterGetU16(record, MonsterFieldWorldY) == s->y) {
            combatStart(s, record);
            return true;
        }
    }
    return false;
}

/* ApplyMapTriggerEffect (yendor2.asm:17458): run for the cell the party stands on after every step -- a trap that hurts, steals or corrodes the members who can act, or (flag
 * 0x4000) a teleport to the record's own cell. Returns what happened for the log. */
/* The trigger-table entry for the cell the party stands on (IsPositionInTriggerList: matched on the cell's wall and floor types). */
static bool partyCellTrigger(const GameSession *s, MapTriggerRecord *record) {
    const DungeonGridCell *cell = dungeonGridCellAtWorldPos(&s->grid, s->x, s->y);
    return cell && mapTriggerFind(s->game, cell->wallType, cell->floorType, record);
}

static bool applyMapTrigger(GameSession *s) {
    MapTriggerRecord record;
    if (!partyCellTrigger(s, &record)) {
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
    effectSound(s, decision.outcome == MapTriggerApplyEffect ? decision.effectId : (record.flags & 0x0200) ? 0x2B : 1);
    snprintf(s->log, sizeof(s->log), "a trap goes off");
    combatCheckWipe(s);
    return true;
}

/*
 * The sounds of a step (HandleMovementInput, Chapter 2 :2338 / Chapter 3 :5973): the door's (id 5 in both chapters) and the bump of a refused step -- Chapter 2's `_val33` (InitGlobals sets it to 6),
 * Chapter 3's literal 0xD (the ids index each game's own sound list). Only a door cell and a blocked one bump: a refused step into the map's void border band (ClassifyFloorType's code 2), or off the
 * dungeon grid, is silent.
 */
enum { DoorSound = 5 /* _val30, both chapters */, BumpSoundYendor2 = 6, BumpSoundYendor3 = 0x0D };

/* One step in the direction of `move`; the second step of a door is not checked (loc_11598 adds the delta unconditionally) beyond keeping the party on the map. */
static void stepUnchecked(GameSession *s, MovementResult move) {
    int nx = s->x + move.deltaCol, ny = s->y + move.deltaRow;
    if (movementInBounds(s->game, (uint16_t)nx, (uint16_t)ny)) {
        s->x = nx;
        s->y = ny;
    }
}

/* What follows a step of the party (HandleMovementInput, loc_11598 on): a monster on the new cell is engaged, else the monsters take their turns (ProcessLevelMonsters), and then the cell's trigger. */
static void movedOnto(GameSession *s) {
    if (!engageMonsterOnPartyCell(s)) {
        monstersTakeTurns(s);
        sessionAmbushShots(s); /* ProcessSideTrapsOnMovement follows the monster pass after a step (and after a shot, below), not the rests' */
    }
    applyMapTrigger(s);
}

SessionStep sessionDoorFinish(GameSession *s) {
    if (!s->doorOpening) {
        return SessionStepBlocked;
    }
    s->doorOpening = false;
    MovementResult move = movementApply(MovementForward, s->facing);
    stepUnchecked(s, move);
    stepUnchecked(s, move);
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
    sessionRebuild(s);
    movedOnto(s);
    return SessionStepMoved;
}

/*
 * HandleMovementInput's door branch (:2301): TryInteractAtPosition loads the lock record of the object at the cell the party walked into (a door's lock, or a curgame record: the same two words),
 * the member the original selects -- the third role's, else the first in the party -- is the current record, and ShowLockStatus reads his or her skill word +0x6C. Nothing is shown for a cell with
 * no such object (the original would show whatever was loaded last).
 */
static void lockStatusLoad(GameSession *s, int col, int row) {
    WorldObjectRecord object;
    LockRecord lock;
    uint16_t flags = 0, value = 0;
    if (!worldObjectFind(&s->objects, s->game, col, row, &object)) {
        return;
    }
    if (object.flags & WorldObjectFlagDoor) {
        if (!lockCatalogRecord(&s->locks, object.value, &lock)) {
            return;
        }
        flags = lock.flags;
        value = lock.price;
    } else if (!(object.flags & WorldObjectFlagCurgameRecord) || !lockCatalogCurgameRecord(&s->locks, object.value, &flags, &value)) {
        return;
    }
    unsigned id = saveHeaderGetU16(&s->save, SaveHeaderRoleAssignments + 4);
    if (id == 0) {
        id = saveGetPartySlot(&s->save, 0);
    }
    const uint8_t *member = id ? saveGamePartyRecordById(&s->save, id) : NULL;
    s->lockFlags = flags;
    s->lockValue = value;
    s->lockSkill = member ? partyGetU16(member, 0x6C) : 0;
    s->lockReport = true;
}

SessionStep sessionMove(GameSession *s, MovementAction action) {
    if (s->combat.active || s->combat.wiped) {
        return SessionStepBusy;
    }
    if (s->doorOpening) { /* a door that was left opening is passed first */
        sessionDoorFinish(s);
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
            if (outcome == MovementCellDoor) {
                lockStatusLoad(s, nx, ny);
            }
            if (outcome == MovementCellDoor || outcome == MovementCellBlocked) {
                s->pendingSound = s->game == GameYendor2 ? BumpSoundYendor2 : BumpSoundYendor3;
            }
            if (outcome == MovementCellSpecial && action == MovementForward) { /* the door opens first */
                s->doorOpening = true;
                s->pendingSound = DoorSound;
                return SessionStepDoorOpening;
            }
            if (outcome == MovementCellSpecial) { /* entered any other way: the sound and the two steps at once */
                s->pendingSound = DoorSound;
                s->x = nx;
                s->y = ny;
                stepUnchecked(s, move);
                result = SessionStepMoved;
            } else if (outcome == MovementCellClear) {
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
    if (result == SessionStepMoved) { /* a turn or a step that was refused does not give the monsters a turn: ProcessLevelMonsters is only called when flags 0x1800 say the party moved */
        movedOnto(s);
    }
    return result;
}

/*
 * The ids differ between the chapters (each indexes its own sound list): Chapter 2 launches with _val19 = 0x1D, is deflected with _val33 = 6, hits with word_329E6 = 0x0D and misses with _val43 = 0x1E;
 * Chapter 3 (HandleRangedOrCombatAction, yendor3.asm:22208) literals 6, 0x0D, 0x22 and 0x23, and waits three BIOS ticks before each member's shot.
 */
typedef struct {
    unsigned launch, deflect, hit, miss, fightWait;
} ShotSounds;
static const ShotSounds kShotSounds2 = {0x1D, 6, 0x0D, 0x1E, 0}, kShotSounds3 = {6, 0x0D, 0x22, 0x23, 2};

enum {
    ShotRowCount = 6,
    ShotMessageTicks = 3,  /* ShowCombatMessageOrWait: six BIOS ticks, three of ours */
    ShotWeaponSlot = 0x13A,
    ShotSlingFirst = 0x21E,
    ShotSlingLast = 0x223,
    ShotCannotAct = 0x1C40 /* dead, paralysed, frozen, stoned */
};
static const unsigned kShotRows[ShotRowCount] = {49, 46, 43, 40, 36, 25};

void sessionScene(GameSession *s, ViewScene *scene, DungeonGridCell cells[ViewportCellCount]) {
    viewportBuild(&s->grid, s->facing, s->x, s->y, cells);
    viewportComputeVisibility(s->game, cells);
    LightingInput light = {s->game == GameYendor2 ? s->light.litFlags : s->travelFlags, s->light.litFlags, s->clock.minutes, s->facing};
    memset(scene, 0, sizeof(*scene));
    bool reset;
    lightingComputeGradient(s->game, &light, 0, scene->gradient, &reset);
    scene->cells = cells;
    scene->facing = s->facing;
    if (s->doorOpening) { /* HandleSpecialCellEntry's frame: the door cell ahead (cell 46) becomes the scratch tile type -- the door's legend with its wall picture + 1 (open) and the floor and ceiling of
                             the cell beyond it (cell 43) */
        MovementResult step = movementApply(MovementForward, s->facing);
        const DungeonGridCell *door = dungeonGridCellAtWorldPos(&s->grid, s->x + step.deltaCol, s->y + step.deltaRow);
        const uint16_t *doorLegend, *beyondLegend;
        if (door && worldMapWallLegend(s->game, door->wallType, &doorLegend) && worldMapWallLegend(s->game, cells[43].wallType, &beyondLegend)) {
            scene->scratchType = s->game == GameYendor2 ? 60 : 400;
            scene->scratchLegend[0] = beyondLegend[0];
            scene->scratchLegend[1] = beyondLegend[1];
            scene->scratchLegend[2] = (uint16_t)(doorLegend[2] + 1);
            for (unsigned i = 3; i < 6; i++) {
                scene->scratchLegend[i] = doorLegend[i];
            }
            cells[46].wallType = scene->scratchType;
        }
    }
    for (unsigned i = 0; s->combat.active && i < CombatMonsterSlotCount; i++) {
        uint8_t *slot = s->combat.slots + (size_t)i * MonsterRecordSize;
        scene->combatMonsters[i] = monsterGetU16(slot, MonsterFieldType) ? slot : NULL;
    }
    if (s->shot.active && s->shot.shown) { /* the picture of the shot, at the cell it has reached */
        scene->projectileDepth = kShotRows[s->shot.row];
        memcpy(scene->projectilePictures, s->shot.picture, sizeof(scene->projectilePictures));
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
    unsigned monsterSlot = combatActiveSlot(c);
    if (!member || monsterSlot >= CombatMonsterSlotCount) {
        return false;
    }
    CombatPlayerMeleeOutcome out = combatPlayerMeleeAttack(member, c->slots + (size_t)monsterSlot * MonsterRecordSize, &s->items, s->game, &s->rng);
    if (out.hit) { /* the weapon's own sound: word 5 of its target entry */
        const uint8_t *weapon = itemCatalogRecord(&s->items, partyGetU16(member, 0x142));
        const uint8_t *entry = weapon ? itemTargetEntry(&s->items, weapon) : NULL;
        queueSound(s, entry ? itemTargetWord(entry, 5) : 0);
    } else if (!out.weaponBroke) {
        queueSound(s, missSound(s));
    }
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

/* ---- the ranged shot ---- */

static unsigned shotWeapon(const SessionShot *shot, unsigned slot) {
    return (unsigned)shot->weapon[slot][0] | ((unsigned)shot->weapon[slot][1] << 8);
}

static const ShotSounds *shotSounds(const GameSession *s) {
    return s->game == GameYendor2 ? &kShotSounds2 : &kShotSounds3;
}

/* DrawWeaponSelectIcon (yendor2.asm:50265, yendor3.asm:50095): the picture of category 1 a shooter's slice is taken from. Chapter 2: four rocks for a sling (0x21E-0x223), else four arrows;
 * Chapter 3 picks by the weapon id: 0x1E4 -> 0x66, 0x10D -> 0x67, 0x20D-0x217 -> 0x9B, 8-0xA -> 9, any other -> 0xA. */
static uint8_t shotPicture(GameKind game, unsigned weapon) {
    if (game == GameYendor2) {
        return weapon >= ShotSlingFirst && weapon <= ShotSlingLast ? 0x51 : 0x50;
    }
    if (weapon == 0x1E4) {
        return 0x66;
    }
    if (weapon == 0x10D) {
        return 0x67;
    }
    if (weapon >= 0x20D && weapon <= 0x217) {
        return 0x9B;
    }
    return weapon >= 8 && weapon <= 0xA ? 9 : 0xA;
}

/* ClassifyObstacleAtViewportRow (yendor2.asm:1936, yendor3.asm:5578): 0 clear, 1 wall, 2 an object in the way, 3 a feature, 4 a monster (`*monster` then points at it). */
static unsigned shotClassify(GameSession *s, const DungeonGridCell cells[ViewportCellCount], unsigned depth, uint8_t **monster) {
    const DungeonGridCell *cell = &cells[depth];
    *monster = monsterAtDepth(s, cells, depth);
    if (*monster) {
        return 4;
    }
    if (cell->flags & 0x800) {
        return 3;
    }
    if (s->game == GameYendor3) { /* walls are the types 2-0x63 and 0xC8-0x12B, and any floor object stops the shot */
        if ((cell->wallType >= 2 && cell->wallType <= 0x63) || (cell->wallType >= 0xC8 && cell->wallType <= 0x12B)) {
            return 1;
        }
        return cell->floorType ? 2 : 0;
    }
    if (cell->wallType >= 2 && cell->wallType <= 15) {
        return 1;
    }
    unsigned object = cell->floorType;
    if (object == 0) {
        return 0;
    }
    if (object < 0x11 || (object > 0x14 && object < 0x3F) || object > 0x43) {
        return 2;
    }
    return 0;
}

bool sessionShootStart(GameSession *s) {
    SessionShot *shot = &s->shot;
    if (s->combat.active || s->combat.wiped || shot->active || s->doorOpening) {
        return false;
    }
    memset(shot, 0, sizeof(*shot));
    for (unsigned slot = 0; slot < 4; slot++) {
        uint8_t *member = sessionPartyRecord(s, slot);
        if (!member) {
            break; /* the scan stops at the first empty slot */
        }
        if (partyGetU16(member, PartyFieldStatusFlags) & ShotCannotAct) {
            continue;
        }
        unsigned weapon = partyGetU16(member, ShotWeaponSlot);
        const uint8_t *record = weapon ? itemCatalogRecord(&s->items, weapon) : NULL;
        ItemServiceTier tier;
        if (!record || !itemClassifyServiceTier(record, &tier)) {
            continue;
        }
        if (partyTickEquippedItemDurability(member, &s->items, s->game, ShotWeaponSlot, &s->rng) != PartyItemDurabilityUnchanged) {
            continue; /* it broke */
        }
        shot->weapon[slot][0] = (uint8_t)weapon;
        shot->weapon[slot][1] = (uint8_t)(weapon >> 8);
        shot->picture[slot] = shotPicture(s->game, weapon);
        shot->pending[slot] = 1;
        shot->attacks++;
    }
    if (!shot->attacks) {
        return false;
    }
    shot->active = shot->shown = true;
    shot->row = 0;
    s->pendingSound = shotSounds(s)->launch;
    return true;
}

static void shotEnd(GameSession *s) { /* the common epilogue (loc_1D747): the loot, the monsters' turn */
    SessionShot *shot = &s->shot;
    awardLoot(s, &shot->staging);
    memset(&shot->staging, 0, sizeof(shot->staging));
    shot->active = shot->fighting = shot->shown = false;
    memset(shot->picture, 0, sizeof(shot->picture));
    sessionRebuild(s);
    monstersTakeTurns(s);
    sessionAmbushShots(s);
}

/* ResolveAttackOrAbilityAction's ranged branch for the first member still to shoot, against the monster at pool index `target`. True when it hit. */
static bool shotFight(GameSession *s, unsigned slot, uint8_t *monster) {
    uint8_t *member = sessionPartyRecord(s, slot);
    const uint8_t *record = itemCatalogRecord(&s->items, shotWeapon(&s->shot, slot));
    const uint8_t *entry = record ? itemTargetEntry(&s->items, record) : NULL;
    ThrownEffect effect;
    memset(&effect, 0, sizeof(effect));
    effect.typeFlags = thrownWeaponTypeFlags(entry ? itemTargetWord(entry, 1) : 0, entry ? itemTargetWord(entry, 4) : 0);
    effect.damage = member ? combatResolveAttack(monsterGetU16(monster, MonsterFieldAbsorption), (uint16_t)partyGetStat(member, PartyStatEquipRating1),
                                                  (uint16_t)partyGetStat(member, PartyStatEquipRating2), &s->rng)
                           : 0;
    return thrownApplyResolvedDamage(monster, &effect);
}

bool sessionShootStep(GameSession *s) {
    SessionShot *shot = &s->shot;
    if (!shot->active) {
        return false;
    }
    if (shot->wait) {
        shot->wait--;
        return true;
    }
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    uint8_t *monster = NULL;
    if (!shot->fighting) { /* the flight: the next cell, and what is in it */
        if (shot->row + 1 >= ShotRowCount) {
            shotEnd(s);
            return false;
        }
        shot->row++;
        shot->shown = true;
        unsigned found = shotClassify(s, cells, kShotRows[shot->row], &monster);
        if (found == 0) {
            if (shot->row + 1 >= ShotRowCount) { /* the last cell was clear too: the shot is over after its frame */
                shot->wait = 0;
            }
            return true;
        }
        if (found == 1 || found == 2) {
            s->pendingSound = shotSounds(s)->deflect;
            shotEnd(s);
            return false;
        }
        if (found == 3) {
            shotEnd(s);
            return false;
        }
        shot->fighting = true;
        shot->target = (unsigned)(monster - s->pool) / MonsterRecordSize;
        shot->wait = shotSounds(s)->fightWait;
        return true; /* the picture is at the monster; the first shot is fought on the next tick */
    }
    monster = s->pool + (size_t)shot->target * MonsterRecordSize;
    unsigned slot = 0;
    while (slot < 4 && !shot->pending[slot]) {
        slot++;
    }
    if (slot >= 4 || monsterGetU16(monster, MonsterFieldType) == 0) {
        shotEnd(s);
        return false;
    }
    shot->attacks--; /* ResetWeaponSlotDisplayCache: the slice of the member that shoots leaves the picture, except for the last one */
    if (shot->attacks != 0) {
        shot->picture[slot] = 0;
    }
    bool hit = shotFight(s, slot, monster);
    shot->pending[slot] = 0;
    shot->wait = ShotMessageTicks + shotSounds(s)->fightWait;
    s->pendingSound = hit ? shotSounds(s)->hit : shotSounds(s)->miss;
    snprintf(s->log, sizeof(s->log), "party member %u %s", slot + 1, hit ? "hits" : "misses");
    if (hit) {
        shot->shown = false; /* the redraw that shows the hit (the monster's flash) takes the picture off the screen */
    }
    if (hit && monsterGetU16(monster, MonsterFieldHealth) == 0) { /* dead: reaped, and the rest of the shots fly on */
        uint8_t *flags = saveGlobalFlags(&s->save);
        combatReapDeadMapMonsters(s->pool, &shot->staging, flags, SaveHeaderGlobalFlagsSize, &s->grid);
        sessionRebuild(s);
        shot->fighting = false;
        shot->shown = true;
        if (shot->attacks == 0 || shot->row + 1 >= ShotRowCount) {
            shotEnd(s);
            return false;
        }
        return true;
    }
    if (shot->attacks == 0) {
        shotEnd(s);
        return false;
    }
    return true;
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
    queueSound(s, spellGetU16(spell, SpellFieldEffectId)); /* the cast: word_332DA, the record's +0x20 is a sound id in the attack branches */
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
        queueSound(s, spellGetU16(spell, SpellFieldInflictedStatus)); /* the strike: word_332DC */
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
    awardLoot(s, &staging);
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
    queueSound(s, spellGetU16(spell, SpellFieldEffectId)); /* the cast (loc_2CEE7), then a strike sound for each monster hit */
    for (unsigned n = 0; n < found && n < 4; n++) {
        queueSound(s, spellGetU16(spell, SpellFieldInflictedStatus));
    }
    if (beam) {
        if (found) {
            combatApplyDamageToMapMonster(rowMonsters[0], caster, spell, s->scrollCast, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid, &s->rng);
        }
    } else {
        combatApplyScreenWideAttack(NULL, rowMonsters, caster, spell, s->scrollCast, &staging, flags, SaveHeaderGlobalFlagsSize, &s->grid, &s->rng);
    }
    awardLoot(s, &staging);
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
    case SpellBranchAttackAllSlots: {
        CombatSpellAreaAttackOutcome area = combatApplySpellAttackToActiveSlots(c->slots, caster, spell, s->scrollCast, &s->rng);
        for (unsigned i = 0; i < MonsterActiveSlots; i++) {
            if (area.hit[i]) {
                queueSound(s, spellGetU16(spell, SpellFieldInflictedStatus)); /* one strike sound for each monster hit */
            }
        }
        break;
    }
    case SpellBranchAttackActiveMonster:
    case SpellBranchLifeForce: {
        unsigned slot = combatActiveSlot(c);
        if (slot >= CombatMonsterSlotCount) {
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
                queueSound(s, spellGetU16(spell, SpellFieldInflictedStatus)); /* the strike's sound (word_332DC) */
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
            queueSound(s, def.sound);
            if (spellGetU16(spell, SpellFieldFlagsA) & (SpellFlagsAPositionReset | SpellFlagsAIconBarPresetAmount)) {
                uint8_t *recipient = s->scrollCast ? sessionPartyRecord(s, s->scrollTarget) : s->castTarget ? sessionPartyRecord(s, s->castTarget - 1) : NULL;
                castStatDelta(s, recipient ? recipient : caster, &def, spell);
            } else {
                done = false;
            }
        } else {
            queueSound(s, def.sound);
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
            if (!engageMonsterOnPartyCell(s)) { /* a monster on the landing cell is fought at once (loc_2C4F5) */
                monstersTakeTurns(s);
            }
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

SessionCast sessionCastOn(GameSession *s, unsigned casterSlot, unsigned spellId, bool returnToMark, unsigned targetSlot) {
    s->castTarget = targetSlot + 1;
    SessionCast result = sessionCastBy(s, casterSlot, spellId, returnToMark);
    s->castTarget = 0;
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

/* The record of a use command's item slot: a bag slot of the member or, from SessionBeltSlotBase, a belt slot. */
static uint8_t *useSlot(GameSession *s, uint8_t *user, unsigned itemSlot) {
    if (itemSlot >= SessionBeltSlotBase) {
        return itemSlot - SessionBeltSlotBase < SaveBeltSlotCount ? beltSlots(s) + 4 * (itemSlot - SessionBeltSlotBase) : NULL;
    }
    return user ? inventoryGroupSlot(bagGroup(user, bagLevel(user)), itemSlot) : NULL;
}

unsigned sessionSlotItem(GameSession *s, unsigned memberSlot, unsigned itemSlot) {
    uint8_t *slot = useSlot(s, sessionPartyRecord(s, memberSlot), itemSlot);
    return slot ? itemSlotId(slot) : 0;
}

bool sessionScrollInfo(GameSession *s, unsigned userSlot, unsigned itemSlot, SessionScrollInfo *out) {
    uint8_t *slot = useSlot(s, sessionPartyRecord(s, userSlot), itemSlot);
    const uint8_t *item = slot && itemSlotId(slot) ? itemCatalogRecord(&s->items, itemSlotId(slot)) : NULL;
    const uint8_t *target = item ? itemTargetEntry(&s->items, item) : NULL;
    if (!target || itemTargetKind(item) != ItemTargetConsumable || !(itemTargetWord(target, ItemTargetSlotFlags) & 0x600)) {
        return false;
    }
    const uint8_t *spell = spellRecord(&s->spells, itemTargetWord(target, 2));
    out->studyable = (itemTargetWord(target, ItemTargetSlotFlags) & 0x200) != 0;
    out->singleTarget = spell && (spellGetU16(spell, SpellFieldFlagsA) & SpellFlagsASingleTarget);
    return true;
}

SessionCast sessionUseScroll(GameSession *s, unsigned userSlot, unsigned itemSlot, bool study, unsigned memberSlot) {
    uint8_t *user = sessionPartyRecord(s, userSlot);
    uint8_t *slot = useSlot(s, user, itemSlot);
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
                if (i == 0) {
                    effectSound(s, stage->effectId); /* one ApplyEffectAndDrawIconBar a pass: its first effect's sound */
                }
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
    uint8_t *slot = useSlot(s, user, itemSlot);
    unsigned id = slot ? itemSlotId(slot) : 0;
    if (!id) {
        return SessionUseNothing;
    }
    bool done;
    const uint8_t *itemRec = itemCatalogRecord(&s->items, id);
    const uint8_t *target = itemRec ? itemTargetEntry(&s->items, itemRec) : NULL;
    /* HandleGameCommand's first test: an item with stat bonuses is a stat potion (ApplyMultiStatEffect): the recipient's fields rise, the charge is spent */
    const uint8_t *statEffect = itemRec && (itemGetU16(itemRec, ItemFieldFlags) & ItemFlagConsumable) ? itemEffectEntry(&s->items, itemRec) : NULL;
    if (statEffect && itemEffectPairs(statEffect)) {
        if (partyGetU16(recipient, PartyFieldStatusFlags) & PartyStatusIncapacitated) {
            return SessionUseNoEffect;
        }
        partyUseStatItem(recipient, statEffect);
        partyConsumeItemCharge(user, &s->items, slot);
        return SessionUseDone;
    }
    if (target && itemTargetKind(itemRec) == ItemTargetConsumable && (itemTargetWord(target, 0) & 4) && !(itemTargetWord(target, ItemTargetSlotFlags) & 0x7E00)) {
        /* ExamineTarget: a key to a destination (KEY OF PORT HOPE..., ANKH OF PORTALS, ATHANEUM KEY): the road opens when its gate flag is set */
        uint8_t *globalSlots = beltSlots(s);
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
                unsigned monster = s->combat.active ? combatActiveSlot(&s->combat) : CombatMonsterSlotCount;
                if (monster >= CombatMonsterSlotCount) {
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
            uint8_t *globalSlots = beltSlots(s);
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
    uint8_t *globalSlots = beltSlots(s);
    for (;;) {
        ItemRangeAvailability r = itemRangeAvailableDeep(globalSlots, &s->save, &s->items, from, from);
        uint8_t *slot = itemRangeSlot(&r, globalSlots, &s->save);
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
    uint8_t *globalSlots = beltSlots(s);
    for (unsigned guard = 0; guard < 64; guard++) {
        ItemRangeAvailability r = itemRangeAvailableDeep(globalSlots, &s->save, &s->items, low, high);
        if (!r.found) {
            break;
        }
        if (!r.inGlobalTable) {
            uint8_t *record = saveGamePartyRecordById(&s->save, r.partyRecordId);
            if (!record) {
                break;
            }
            partyRemoveMultiStatEffect(record, itemEffectEntry(&s->items, itemCatalogRecord(&s->items, r.itemId)));
        }
        itemRangeConsume(&r, globalSlots, &s->save, &s->items);
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
        uint8_t *globalSlots = beltSlots(s);
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
        out.kind = SessionInteractNothingHere;
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
                out.kind = check.outcome == TravelUnlockDeniedSilent ? SessionInteractNothing : check.outcome == TravelUnlockDeniedFixedMessage ? SessionInteractTravelFixed : SessionInteractTravelRefused;
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
    uint8_t *globalSlots = beltSlots(s);
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
    s->doorOpening = false;
    s->pendingTravel = 0;
    s->pendingSound = 0;
    s->heldItem = s->heldExtra = 0;
    sessionRebuild(s);
    return true;
}

bool sessionNewGame(GameSession *s, const uint8_t *worldDat, size_t size) {
    SaveGame fresh;
    saveGameInit(&fresh, s->game);
    if (!saveGameNewGame(&fresh, s->game, worldDat, size)) {
        return false;
    }
    static uint8_t image[SaveFileSizeMax];
    size_t bytes = saveGameStore(&fresh, image, sizeof(image));
    if (!bytes || !sessionLoad(s, image, bytes)) {
        return false;
    }
    s->clock.day = s->clock.month = s->clock.year = 1;
    s->log[0] = 0;
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
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

/* ---- bags: containers open on the inventory screen ---- */

/*
 * GetInventorySlotPtr (yendor2.asm:40203): the slot codes 1-8 address the contents of the first open bag in the priority bag 1, 2, 3 (the markers +0x17C / +0x1A2 / +0x1C8), else the main inventory.
 * A level is 0 for the main group and n for the bag whose marker is n - 1; the bags are opened in the order 3, 2, 1, so a higher level is nested inside the lower ones.
 */
static unsigned bagLevel(uint8_t *record) {
    for (unsigned bag = 0; bag < 3; bag++) {
        if (partyGetU16(partyBagMarker(record, bag), 0) != 0) {
            return bag + 1;
        }
    }
    return 0;
}

static uint8_t *bagGroup(uint8_t *record, unsigned level) {
    return level ? partyBagMarker(record, level - 1) + 4 : partyInventoryGroup(record, PartyGroupMain);
}

/* The weight an item carries: its own and, for a container, everything inside it (PickUpItemFromSlot: the record's first word is added). An item with no record of its own (extra 0) holds nothing. */
static unsigned carriedWeight(GameSession *s, unsigned id, unsigned extra) {
    const uint8_t *item = itemCatalogRecord(&s->items, id);
    unsigned weight = itemWeightOf(s, id);
    if (item && (itemGetU16(item, ItemFieldFlags) & ItemFlagEquipCode0B)) {
        weight += containerWeight(&s->save, extra);
    }
    return (uint16_t)weight;
}

/*
 * The groups an item's weight counts in (PlaceItemInSlot :39886 / PickUpItemFromSlot :40519, the bag-slot codes and code 0xB): in the main group the load (+0x118) -- a MAGIC CONTAINER (0x11) weighs
 * nothing there; in an open bag the weight of the group itself, then of every bag it is nested in, then the load -- a bag that is a MAGIC CONTAINER does not take it, and one at the outermost
 * level stops it from reaching the load at all.
 */
static void bagWeight(uint8_t *record, unsigned level, unsigned id, unsigned weight, bool add) {
    uint8_t *main = partyInventoryGroup(record, PartyGroupMain);
    int delta = add ? (int)weight : -(int)weight;
    if (level == 0) {
        if (id != 0x11) {
            inventoryGroupSetWeight(main, (uint16_t)(inventoryGroupWeight(main) + delta));
        }
        return;
    }
    for (unsigned bag = level - 1; bag < 3; bag++) {
        uint8_t *marker = partyBagMarker(record, bag);
        if (partyGetU16(marker, 0) == 0x11) {
            if (bag == 2) {
                return;
            }
            continue;
        }
        inventoryGroupSetWeight(marker + 4, (uint16_t)(inventoryGroupWeight(marker + 4) + delta));
    }
    inventoryGroupSetWeight(main, (uint16_t)(inventoryGroupWeight(main) + delta));
}

/* IsContainerTypeCompatible (:40619): with a bag open (the open flags 0x8000 / 0x4000 / 0x2000 / the equipped one 0x1000, in that order) an item for a slot code below 0xA must fit that bag's kind. */
static bool bagAccepts(GameSession *s, uint8_t *record, unsigned code, unsigned heldId) {
    unsigned ui = partyGetU16(record, PartyFieldUiFlags), container;
    if (code >= 0x0A) {
        return true;
    }
    if (ui & PartyUiBag1Open) {
        container = partyGetU16(partyBagMarker(record, 0), 0);
    } else if (ui & PartyUiBag2Open) {
        container = partyGetU16(partyBagMarker(record, 1), 0);
    } else if (ui & (PartyUiBag3Open | PartyUiEquippedBagOpen)) {
        container = partyGetU16(partyBagMarker(record, 2), 0);
    } else {
        return true;
    }
    const uint8_t *bag = itemCatalogRecord(&s->items, container), *held = itemCatalogRecord(&s->items, heldId);
    return bag && held && inventoryContainerAccepts((uint16_t)itemGetU16(bag, ItemFieldFlags), (uint16_t)itemGetU16(held, ItemFieldFitFlags));
}

/* SaveAndCloseContainer (:40148): an open bag's contents go back to the record the marker names, and the marker is cleared. */
static void bagClose(GameSession *s, uint8_t *record, unsigned bag) {
    uint8_t *marker = partyBagMarker(record, bag);
    if (partyGetU16(marker, 0) != 0) {
        uint8_t *contents = saveGameRecord(&s->save, SaveSectionItemInstances, partyGetU16(marker, 2));
        if (contents) {
            memcpy(contents, marker + 4, InventoryGroupSize);
        }
    }
    partySetU16(marker, 0, 0);
    partySetU16(marker, 2, 0);
}

/* LoadContainerContents (:40069): the record of the container's contents is read into the marker's group. */
static void bagOpen(GameSession *s, uint8_t *record, unsigned bag, unsigned id, unsigned extra) {
    uint8_t *marker = partyBagMarker(record, bag);
    const uint8_t *contents = saveGameRecord(&s->save, SaveSectionItemInstances, extra);
    partySetU16(marker, 0, (uint16_t)id);
    partySetU16(marker, 2, (uint16_t)extra);
    if (contents) {
        memcpy(marker + 4, contents, InventoryGroupSize);
    } else {
        memset(marker + 4, 0, InventoryGroupSize);
    }
}

SessionBag sessionBagToggle(GameSession *s, unsigned member, unsigned region) {
    uint8_t *record = sessionPartyRecord(s, member);
    if (!record) {
        return SessionBagIgnored;
    }
    unsigned ui = partyGetU16(record, PartyFieldUiFlags);
    if (region == 9) { /* the open bag's icon is the button that closes the top one (HandleInventoryGridClick loc_264B3) */
        if (!(ui & (PartyUiBag1Open | PartyUiBag2Open | PartyUiBag3Open))) {
            return SessionBagIgnored;
        }
        if (partyGetU16(partyBagMarker(record, 0), 0)) {
            bagClose(s, record, 0);
            ui &= ~(unsigned)PartyUiBag1Open;
        } else if (partyGetU16(partyBagMarker(record, 1), 0)) {
            bagClose(s, record, 1);
            ui &= ~(unsigned)PartyUiBag2Open;
        } else {
            bagClose(s, record, 2);
            ui &= ~(unsigned)PartyUiBag3Open;
        }
        partySetU16(record, PartyFieldUiFlags, (uint16_t)ui);
        return SessionBagClosed;
    }
    uint8_t *cell = NULL;
    unsigned level = 0;
    if (region >= 1 && region <= 8) {
        level = bagLevel(record);
        cell = inventoryGroupSlot(bagGroup(record, level), region);
    } else if (region == 0x0B) {
        cell = partyEquipmentSlot(record, 0x0B, s->game);
    }
    const uint8_t *item = cell && itemSlotId(cell) ? itemCatalogRecord(&s->items, itemSlotId(cell)) : NULL;
    if (!item || !(itemGetU16(item, ItemFieldFlags) & ItemFlagEquipCode0B)) {
        return SessionBagIgnored;
    }
    if (itemSlotExtra(cell) == 0) { /* a container always has a record: one that has none yet gets it now */
        itemSlotSet(cell, itemSlotId(cell), (uint16_t)containerAllocate(&s->save));
    }
    if (region == 0x0B) { /* the equipped container: it replaces every other open bag and takes the third marker (loc_26490) */
        bool wasOpen = (ui & PartyUiEquippedBagOpen) != 0;
        for (unsigned bag = 0; bag < 3; bag++) {
            bagClose(s, record, bag);
        }
        if (wasOpen) {
            ui &= 0x0FFF;
        } else {
            ui &= 0x1FFF;
            bagOpen(s, record, 2, itemSlotId(cell), itemSlotExtra(cell));
            ui |= PartyUiEquippedBagOpen;
        }
        partySetU16(record, PartyFieldUiFlags, (uint16_t)ui);
        return wasOpen ? SessionBagClosed : SessionBagOpened;
    }
    if (partyGetU16(partyBagMarker(record, 0), 0)) { /* three are open already (loc_26533: a warning) */
        return SessionBagRefused;
    }
    if (partyGetU16(partyBagMarker(record, 1), 0)) {
        bagOpen(s, record, 0, itemSlotId(cell), itemSlotExtra(cell));
        ui |= PartyUiBag1Open;
    } else if (partyGetU16(partyBagMarker(record, 2), 0)) {
        bagOpen(s, record, 1, itemSlotId(cell), itemSlotExtra(cell));
        ui |= PartyUiBag2Open;
    } else {
        bagOpen(s, record, 2, itemSlotId(cell), itemSlotExtra(cell));
        ui |= PartyUiBag3Open;
    }
    partySetU16(record, PartyFieldUiFlags, (uint16_t)ui);
    return SessionBagOpened;
}

void sessionBagsClose(GameSession *s, unsigned member) {
    uint8_t *record = sessionPartyRecord(s, member);
    if (!record) {
        return;
    }
    partySetU16(record, PartyFieldUiFlags, (uint16_t)(partyGetU16(record, PartyFieldUiFlags) & 0x0FFF)); /* CloseAllAlternateBags (:39856) */
    for (unsigned bag = 0; bag < 3; bag++) {
        bagClose(s, record, bag);
    }
}

bool sessionDropOnPortrait(GameSession *s, unsigned member) {
    uint8_t *record = sessionPartyRecord(s, member);
    const uint8_t *item = s->heldItem ? itemCatalogRecord(&s->items, s->heldItem) : NULL;
    if (!record || !item) {
        return false;
    }
    const uint8_t *target = itemTargetEntry(&s->items, item);
    uint16_t weight = (uint16_t)carriedWeight(s, s->heldItem, s->heldExtra);
    AutoEquipPlan plan = inventoryPlanAutoEquip(s->game, record, s->heldItem, weight, itemGetU16(item, ItemFieldFlags), target ? itemTargetWord(target, ItemTargetSlotFlags) : 0);
    if (plan.kind == AutoEquipRefused) {
        return false;
    }
    if (plan.linksContainer && (itemGetU16(item, ItemFieldFlags) & ItemFlagEquipCode0B) && s->heldExtra == 0) { /* TryLoadNextContainerLink */
        s->heldExtra = (uint16_t)containerAllocate(&s->save);
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
    unsigned level = equipmentCode ? 0 : bagLevel(record);
    uint8_t *slot = equipmentCode ? partyEquipmentSlot(record, equipmentCode, s->game) : inventoryGroupSlot(bagGroup(record, level), slotNumber);
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
    unsigned weight = carriedWeight(s, id, s->heldExtra);
    bool worn = equipmentCode && equipmentCode != 0x0B;
    if (worn) { /* equipped gear carries its stat effect and counts in the load */
        if (item) {
            partyRemoveMultiStatEffect(record, itemEffectEntry(&s->items, item));
        }
        uint8_t *main = partyInventoryGroup(record, PartyGroupMain);
        inventoryGroupSetWeight(main, (uint16_t)(inventoryGroupWeight(main) - weight));
    } else {
        bagWeight(record, level, id, weight, false);
    }
    partyRecomputeEquipmentStatBonuses(record, &s->items, s->game);
    return true;
}

/* Puts item `id` into the slot a click named (PlaceItemInSlot): its weight joins the groups it is in, worn gear takes effect, the wear counter of a weapon slot is cleared. */
static void placeInSlot(GameSession *s, uint8_t *record, unsigned code, unsigned level, uint8_t *cell, unsigned id, unsigned extra) {
    const uint8_t *item = itemCatalogRecord(&s->items, id);
    if (code >= PartyEquipmentFirstShort) {
        cell[0] = (uint8_t)id;
        cell[1] = (uint8_t)(id >> 8);
    } else {
        itemSlotSet(cell, (uint16_t)id, (uint16_t)extra);
    }
    bool worn = code >= 0x0A && code != 0x0B;
    unsigned weight = carriedWeight(s, id, extra);
    if (worn) {
        uint8_t *main = partyInventoryGroup(record, PartyGroupMain);
        inventoryGroupSetWeight(main, (uint16_t)(inventoryGroupWeight(main) + weight));
        if (item) {
            partyApplyMultiStatEffect(record, itemEffectEntry(&s->items, item));
        }
    } else {
        bagWeight(record, code == 0x0B ? 0 : level, id, weight, true);
    }
    if (code == 0x0A) {
        partySetU16(record, PartyFieldWearMain, 0);
    } else if (code == 0x0C) {
        partySetU16(record, PartyFieldWearSecond, 0);
    } else if (code == 0x0D) {
        partySetU16(record, PartyFieldWearThird, 0);
    }
    partyRecomputeEquipmentStatBonuses(record, &s->items, s->game);
}

/* The weight check of a click that puts the held item down: the load, plus what is carried in, less what is taken out, against the capacity (+0x56). */
static bool carryFits(uint8_t *record, unsigned load, unsigned weight, unsigned taken) {
    return (uint16_t)(weight + load - taken) <= partyGetU16(record, 0x56);
}

SessionInv sessionInventoryClick(GameSession *s, unsigned member, unsigned region) {
    uint8_t *record = sessionPartyRecord(s, member);
    if (!record || region == 0 || region == 9 || region > 21) {
        return SessionInvIgnored;
    }
    unsigned code = region == 21 ? 20 : region; /* the two last regions are the one slot */
    unsigned equipment = code >= 10 ? code : 0;
    unsigned level = equipment ? 0 : bagLevel(record);
    uint8_t *cell = equipment ? partyEquipmentSlot(record, equipment, s->game) : inventoryGroupSlot(bagGroup(record, level), code);
    if (!cell) {
        return SessionInvIgnored;
    }
    uint8_t *main = partyInventoryGroup(record, PartyGroupMain);
    bool shortSlot = code >= PartyEquipmentFirstShort;
    unsigned slotId = shortSlot ? (unsigned)(cell[0] | (cell[1] << 8)) : itemSlotId(cell);
    unsigned ui = partyGetU16(record, PartyFieldUiFlags);
    if (!s->heldItem) {
        if (code == 0x0B && (ui & PartyUiEquippedBagOpen)) { /* an open equipped container cannot be taken off (loc_2627A) */
            return slotId ? SessionInvRefused : SessionInvIgnored;
        }
        return slotId && sessionPickUp(s, member, equipment ? 0 : code, equipment) ? SessionInvTook : SessionInvIgnored;
    }
    const uint8_t *held = itemCatalogRecord(&s->items, s->heldItem);
    if (!held) {
        return SessionInvRefused;
    }
    unsigned flags = itemGetU16(held, ItemFieldFlags);
    if (slotId == keyRingId(s->game)) { /* a key over a ring in a slot */
        if (!isRingKey(s->game, s->heldItem)) {
            return SessionInvRefused;
        }
        const uint8_t *entry = itemTargetEntry(&s->items, held);
        saveHeaderSetU16(&s->save, SaveHeaderKeyFlags, (uint16_t)(sessionKeyRingFlags(s) | (entry ? interactKeyRingContribution(itemTargetWord(entry, 0)) : 0)));
        s->heldItem = s->heldExtra = 0;
        s->pendingSound = 4;
        return SessionInvKeyRing;
    }
    const uint8_t *target = itemTargetEntry(&s->items, held);
    bool eligible = inventoryEligibleForSlot(code, (uint16_t)flags, target ? (uint16_t)itemTargetWord(target, ItemTargetSlotFlags) : 0, partyGetU16(record, 0x13E) != 0,
                                              partyGetU16(record, 0x146) != 0, (partyGetU16(record, PartyFieldUiFlags) & 0x20) != 0);
    if (!eligible) {
        return SessionInvRefused;
    }
    if ((flags & ItemFlagEquipCode0B) && s->heldExtra == 0) { /* LoadNextContainerInChain: a container put down with no record of its own is given one */
        s->heldExtra = (uint16_t)containerAllocate(&s->save);
    }
    unsigned heldWeight = carriedWeight(s, s->heldItem, s->heldExtra), load = inventoryGroupWeight(main), capacity = partyGetU16(record, 0x56);
    const uint8_t *there = slotId ? itemCatalogRecord(&s->items, slotId) : NULL;
    if (slotId && there && (itemGetU16(there, ItemFieldFlags) & ItemFlagEquipCode0B)) {
        /* the slot holds a container: the held item goes inside it (loc_2632B) -- the load must take it, an equipped container must be closed, its kind must take the item, and a slot must be free */
        if ((int16_t)(uint16_t)(load + heldWeight) > (int16_t)capacity) {
            return SessionInvRefused;
        }
        if (code == 0x0B && (ui & PartyUiEquippedBagOpen)) {
            return SessionInvIgnored;
        }
        if (!inventoryContainerAccepts((uint16_t)itemGetU16(there, ItemFieldFlags), (uint16_t)itemGetU16(held, ItemFieldFitFlags))) {
            return SessionInvRefused;
        }
        uint8_t *contents = saveGameRecord(&s->save, SaveSectionItemInstances, itemSlotExtra(cell));
        uint8_t *free = NULL;
        for (unsigned n = 1; contents && n <= InventorySlotCount && !free; n++) {
            uint8_t *entry = inventoryGroupSlot(contents, n);
            free = itemSlotId(entry) == 0 ? entry : NULL;
        }
        if (!free) {
            return SessionInvRefused;
        }
        if (slotId == 0x11) { /* a MAGIC CONTAINER's contents weigh nothing: what goes in is taken off both the load and its own weight first */
            inventoryGroupSetWeight(main, (uint16_t)(inventoryGroupWeight(main) - heldWeight));
            inventoryGroupSetWeight(contents, (uint16_t)(inventoryGroupWeight(contents) - heldWeight));
        }
        itemSlotSet(free, s->heldItem, s->heldExtra);
        inventoryGroupSetWeight(contents, (uint16_t)(inventoryGroupWeight(contents) + heldWeight));
        bagWeight(record, code == 0x0B ? 0 : level, s->heldItem, heldWeight, true);
        s->heldItem = s->heldExtra = 0;
        s->pendingSound = 6;
        return SessionInvPlaced;
    }
    if (slotId) {
        if (!there) {
            return SessionInvRefused;
        }
        unsigned thereWeight = itemWeightOf(s, slotId);
        if (heldWeight != thereWeight && !carryFits(record, load, heldWeight, thereWeight)) {
            return SessionInvRefused;
        }
        if (!bagAccepts(s, record, code, s->heldItem)) {
            return SessionInvRefused;
        }
        uint16_t id = s->heldItem, extra = s->heldExtra;
        s->heldItem = 0;
        sessionPickUp(s, member, equipment ? 0 : code, equipment); /* the slot's item to the cursor */
        cell = equipment ? partyEquipmentSlot(record, equipment, s->game) : inventoryGroupSlot(bagGroup(record, level), code);
        placeInSlot(s, record, code, level, cell, id, extra);
        s->pendingSound = 6;
        return SessionInvSwapped;
    }
    if (!carryFits(record, load, heldWeight, 0) || !bagAccepts(s, record, code, s->heldItem)) {
        return SessionInvRefused;
    }
    placeInSlot(s, record, code, level, cell, s->heldItem, s->heldExtra);
    s->heldItem = s->heldExtra = 0;
    s->pendingSound = 6;
    return SessionInvPlaced;
}

bool sessionHeldDroppable(GameSession *s) {
    return s->heldItem && !inventoryDropBlocked(s->game, &s->items, &s->save, s->heldItem, s->heldExtra);
}

void sessionDropHeld(GameSession *s) {
    if (s->heldItem) {
        inventoryDiscardDropped(&s->items, &s->save, s->heldItem, s->heldExtra);
        s->heldItem = s->heldExtra = 0;
    }
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
    uint8_t *globalSlots = beltSlots(s);
    RestCtx ctx = {s};
    MapTriggerRecord trigger;
    RestOutcome out = restParty(&s->save, &s->clock, &s->items, globalSlots, s->heldItem != 0, false, partyCellTrigger(s, &trigger), restMonsters, &ctx);
    s->rest = out;
    s->restReport = !out.refused || out.noRest;
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

/*
 * UnlockDoorCommand's outcome (yendor2.asm:45960): nothing lockable ahead -> sound 3; already unlocked / triggered -> the status panel says NOT LOCKED (`unlockMessage` 1); the key opens it -> the unlock
 * sound (Chapter 2 word_329EE = 0x2A, Chapter 3 0xB), the bit is set, the grid cell loses its 0x4000 marker and the panel says UNLOCKED (2); otherwise ShowLockStatus tells what the lock needs
 * (`lockReport`, lockStatusText). The front end shows `unlockMessage` (and clears it).
 */
enum { UnlockSoundYendor2 = 0x2A, UnlockSoundYendor3 = 0x0B, RefusalSound = 3 };

/* The key ring and the door / chest keys that can be put on it: items 0x2F and 0x21-0x2E in Chapter 2, 0x32 and 0x24-0x31 in Chapter 3 (HandleStatusPanelItemSlotClick's comparisons). */
static unsigned keyRingId(GameKind game) {
    return game == GameYendor2 ? 0x2F : 0x32;
}

static bool isRingKey(GameKind game, unsigned id) {
    return game == GameYendor2 ? id >= 0x21 && id <= 0x2E : id >= 0x24 && id <= 0x31;
}

InteractUnlockOutcome sessionUnlock(GameSession *s, uint16_t keyWord0, uint16_t heldKeyFlags) {
    InteractUnlockOutcome out = interactUnlockFacing(&s->save, s->game, &s->objects, &s->locks, s->x, s->y, s->facing, keyWord0, heldKeyFlags);
    static const char *const names[] = {"nothing to unlock here", "already unlocked", "unlocked", "locked (needs another key)"};
    snprintf(s->log, sizeof(s->log), "%s", names[out.result]);
    s->unlockMessage = 0;
    switch (out.result) {
    case InteractUnlockNothing:
        s->pendingSound = RefusalSound;
        break;
    case InteractUnlockAlready:
        s->unlockMessage = 1;
        break;
    case InteractUnlockOpened: {
        DungeonGridCell *cell = dungeonGridCellMutable(&s->grid, out.worldRow - s->grid.originRow, out.worldCol - s->grid.originCol);
        if (cell) {
            cell->flags &= (uint16_t)~out.clearCellBits;
        }
        s->pendingSound = s->game == GameYendor2 ? UnlockSoundYendor2 : UnlockSoundYendor3;
        s->unlockMessage = 2;
        break;
    }
    case InteractUnlockLocked:
        lockStatusLoad(s, out.worldCol, out.worldRow);
        break;
    }
    return out;
}

uint16_t sessionKeyRingFlags(const GameSession *s) {
    return saveHeaderGetU16(&s->save, SaveHeaderKeyFlags);
}

/* The K command with the key ring (item 0x2F, a master key: the entry's flags word 0x0030): the tiers of the keys put on the ring decide. */
InteractUnlockOutcome sessionUnlockWithRing(GameSession *s) {
    const uint8_t *ring = itemCatalogRecord(&s->items, keyRingId(s->game));
    const uint8_t *entry = ring ? itemTargetEntry(&s->items, ring) : NULL;
    return sessionUnlock(s, entry ? itemTargetWord(entry, 0) : 0x0030, sessionKeyRingFlags(s));
}

bool sessionHasItem(GameSession *s, unsigned lowId, unsigned highId) {
    return itemRangeAvailableDeep(beltSlots(s), &s->save, &s->items, (uint16_t)lowId, (uint16_t)highId).found; /* IsItemRangeAvailable: the belt, then each member's slots and the containers in them */
}

uint8_t *sessionBeltSlot(GameSession *s, unsigned slot) {
    return slot < SaveBeltSlotCount ? beltSlots(s) + 4 * slot : NULL;
}

/*
 * HandleStatusPanelItemSlotClick (yendor2.asm:41175): a click on one of the six belt slots. With an empty hand the slot's item goes to the cursor; holding a key over the key ring puts it on the
 * ring (its tier is added to the ring's flags, FinishPlacingHeldItem: sound 4, the key is used up); a container is refused; otherwise the held item is put in an empty slot or swapped with the
 * slot's (sound 6). Nothing here is weighed.
 */
SessionBelt sessionBeltClick(GameSession *s, unsigned slot) {
    uint8_t *cell = sessionBeltSlot(s, slot);
    if (!cell) {
        return SessionBeltNothing;
    }
    unsigned id = itemSlotId(cell);
    if (!s->heldItem) {
        if (!id) {
            return SessionBeltNothing;
        }
        s->heldItem = (uint16_t)id;
        s->heldExtra = itemSlotExtra(cell);
        itemSlotSet(cell, 0, 0);
        return SessionBeltTook;
    }
    if (id == keyRingId(s->game)) {
        if (!isRingKey(s->game, s->heldItem)) {
            return SessionBeltRefused;
        }
        const uint8_t *key = itemCatalogRecord(&s->items, s->heldItem);
        const uint8_t *entry = key ? itemTargetEntry(&s->items, key) : NULL;
        saveHeaderSetU16(&s->save, SaveHeaderKeyFlags, (uint16_t)(sessionKeyRingFlags(s) | (entry ? interactKeyRingContribution(itemTargetWord(entry, 0)) : 0)));
        s->heldItem = s->heldExtra = 0;
        s->pendingSound = 4;
        return SessionBeltKeyRing;
    }
    const uint8_t *held = itemCatalogRecord(&s->items, s->heldItem);
    if (!held || (held[ItemFieldFlags] | (held[ItemFieldFlags + 1] << 8)) & ItemFlagEquipCode0B) {
        return SessionBeltRefused;
    }
    s->pendingSound = 6;
    uint16_t heldId = s->heldItem, heldExtra = s->heldExtra;
    if (id) {
        s->heldItem = (uint16_t)id;
        s->heldExtra = itemSlotExtra(cell);
    } else {
        s->heldItem = s->heldExtra = 0;
    }
    itemSlotSet(cell, heldId, heldExtra);
    return id ? SessionBeltSwapped : SessionBeltPlaced;
}

SessionChest sessionChestOpen(GameSession *s) {
    SessionChest out;
    memset(&out, 0, sizeof(out));
    WorldObjectProbeResult probe = worldObjectProbeFacingTile(&s->objects, s->game, s->x, s->y, s->facing);
    LockRecord lock;
    if (probe.outcome == WorldObjectProbeNone || !(probe.object.flags & WorldObjectFlagDoor) || !lockCatalogRecord(&s->locks, probe.object.value, &lock)) {
        return out;
    }
    out.lockId = probe.object.value;
    out.inexhaustible = (lock.flags & 2) != 0;
    unsigned id = saveHeaderGetU16(&s->save, SaveHeaderRoleAssignments + 4);
    uint8_t *opener = id ? saveGamePartyRecordById(&s->save, id) : NULL;
    if (opener && (partyGetU16(opener, PartyFieldStatusFlags) & PartyStatusIncapacitated)) { /* he or she cannot: the beep, and the role is free again */
        s->pendingSound = RefusalSound;
        saveHeaderSetU16(&s->save, SaveHeaderRoleAssignments + 4, 0);
        opener = NULL;
    }
    if (!opener) {
        out.result = SessionChestAskWho;
        out.askSearch = out.inexhaustible;
        return out;
    }
    unsigned bit = interactWorldObjectBitIndex(s->game, &probe.object);
    bool already = interactBitmapTest(&s->save, bit);
    if (lock.flags & LockFlagTrapped) {
        if (!already) {
            interactBitmapSet(&s->save, bit);
            CombatSavingThrowTrapOutcome trap = combatApplySavingThrowTrap(lock.price, opener, &s->save, s->game, &s->rng);
            out.trapDefeated = lock.price != 0 && trap == CombatSavingThrowTrapNone;
            combatCheckWipe(s);
        }
    } else if (!already) {
        lockStatusLoad(s, probe.worldCol, probe.worldRow);
        out.result = SessionChestLocked;
        return out;
    }
    out.result = SessionChestOpen;
    return out;
}

bool sessionChestChooseOpener(GameSession *s, unsigned memberSlot) {
    const uint8_t *member = sessionPartyRecord(s, memberSlot);
    if (!member || (partyGetU16(member, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
        return false;
    }
    saveHeaderSetU16(&s->save, SaveHeaderRoleAssignments + 4, saveGetPartySlot(&s->save, memberSlot));
    return true;
}

unsigned sessionChestSlots(GameSession *s, unsigned lockId, ChestSlot out[LockContentSlots]) {
    LockRecord lock;
    memset(out, 0, sizeof(ChestSlot) * LockContentSlots);
    if (!lockCatalogRecord(&s->locks, lockId, &lock)) {
        return 0;
    }
    chestSlots(&lock, chestTakenMask(&s->save, lockId), &s->items, out);
    unsigned count = 0;
    for (unsigned i = 0; i < LockContentSlots; i++) {
        count += out[i].itemId != 0;
    }
    return count;
}

SessionChestSlot sessionChestClick(GameSession *s, unsigned lockId, unsigned slot, ChestSlot *taken) {
    LockRecord lock;
    ChestSlot slots[LockContentSlots];
    if (slot >= LockContentSlots || !lockCatalogRecord(&s->locks, lockId, &lock)) {
        return SessionChestSlotNothing;
    }
    chestSlots(&lock, chestTakenMask(&s->save, lockId), &s->items, slots);
    if (!s->heldItem) {
        ChestSlot got;
        if (!slots[slot].itemId || !chestTake(&s->save, lockId, &lock, &s->items, slot, &got)) {
            return SessionChestSlotNothing;
        }
        if (taken) {
            *taken = got;
        }
        if (got.kind == ChestSlotItem) {
            s->heldItem = got.itemId;
            s->heldExtra = got.value;
            return SessionChestSlotItem;
        }
        s->pendingSound = 7;
        return SessionChestSlotPile;
    }
    /* putting an item back: the first taken slot whose original item is the same */
    ChestSlot original[LockContentSlots];
    chestSlots(&lock, 0, &s->items, original);
    uint8_t mask = chestTakenMask(&s->save, lockId);
    for (unsigned i = 0; i < LockContentSlots; i++) {
        if ((mask & (0x80u >> i)) && original[i].itemId == s->heldItem) {
            chestRestore(&s->save, lockId, i);
            s->heldItem = s->heldExtra = 0;
            return SessionChestSlotPutBack;
        }
    }
    return SessionChestSlotRefused;
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
