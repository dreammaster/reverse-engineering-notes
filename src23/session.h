#ifndef YENDOR23_SESSION_H
#define YENDOR23_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "combat.h"
#include "dungeongrid.h"
#include "game.h"
#include "gameclock.h"
#include "interact.h"
#include "lightsource.h"
#include "item.h"
#include "lockcatalog.h"
#include "monster.h"
#include "monsterpool.h"
#include "movement.h"
#include "random.h"
#include "spellrecord.h"
#include "rest.h"
#include "savegame.h"
#include "viewport.h"
#include "viewrender.h"
#include "worldmap.h"
#include "worldobjects.h"

/*
 * The playable core of the exploration game: everything the main loop of the original does between a key press and the next picture, composed from the
 * modules and with no display, sound or timing in it (RunDungeonGameLoop's order: the input is applied, the window is rebuilt, the monsters take their turns, a
 * monster that reaches the party starts a combat which then runs turn by turn). An engine owns a GameSession, feeds it commands and draws what it reports;
 * src23/tools/explore_sdl.c is such a front end.
 *
 *   window      sessionRebuild: the 78 x 78 grid around the party, the interaction markers baked in (windowbake.h) and the live monsters linked in
 *   movement    sessionMove: passability (movement.h, doors and locked cells block), the fog-of-war reveal, a new window, then every live monster's turn
 *   combat      started by a monster stepping onto the party's cell: the turn order (dexterity), the party's A (sessionAttack) and the monsters' turns until a
 *               party member is to act; victory awards the loot and experience, a wipe ends the game (`wiped`)
 *   rest        sessionRest (rest.h), unlock / loot the object ahead (interact.h, chest.h)
 *
 * What is not in it: spells and abilities, items on the cursor, inventory, shops and dialogs (their decision modules exist; their screens are the front end's),
 * and time passing other than by resting (the clock only changes there).
 */
typedef struct {
    bool active, wiped;
    uint8_t slots[CombatMonsterSlotCount * MonsterRecordSize];
    bool defeated[CombatMonsterSlotCount];
    CombatTurnOrderEntry order[CombatTurnOrderCapacity];
    unsigned count, cursor;
    uint16_t targets[CombatMonsterSlotCount];
    MonsterRewardStaging staging;
} SessionCombat;

typedef struct GameSession {
    GameKind game;
    WorldMap map;
    WorldObjectTable objects;
    LockCatalog locks;
    MonsterCatalog monsters;
    ItemCatalog items;
    SpellCatalog spells;
    SaveGame save;
    DungeonGrid grid;
    uint8_t pool[MonsterPoolSize * MonsterRecordSize];
    RandomState rng;
    int x, y;
    uint16_t facing; /* SaveFacing */
    GameClock clock;
    LightSourceState light; /* the carried and spell light timers (lightsource.h); the view's light tiers come from litFlags */
    uint16_t heldItem, heldExtra; /* the item on the cursor (CREATE FOOD / FORGE put one there when the cursor is empty); 0 = none */
    SessionCombat combat;
    char log[96]; /* the last thing that happened, as the original's message area would say it */
} GameSession;

/* A new game from a WORLD.DAT image: the four ready-made heroes are the party, at the template's start position. NULL if the image lacks a table. */
GameSession *sessionNew(GameKind game, const uint8_t *worldDat, size_t size);
void sessionFree(GameSession *session);

/* RefreshDungeonMapWindow: grid + markers + monsters. Called by the commands below; a front end calls it once after loading a save. */
void sessionRebuild(GameSession *session);

typedef enum { SessionStepMoved, SessionStepTurned, SessionStepBlocked, SessionStepBusy } SessionStep;

/* One movement command. SessionStepBusy: a combat is on (or the party is wiped) and nothing moves. */
SessionStep sessionMove(GameSession *session, MovementAction action);

/* The view cells, their occlusion and the monsters to draw (spawning the ones that come into view, monsterPoolEncounterScan); fills `scene` for viewRender. */
void sessionScene(GameSession *session, ViewScene *scene, DungeonGridCell cells[ViewportCellCount]);

/* The current party member's melee attack on the first live monster of the combat; the monsters then act until it is a party member's turn again. */
bool sessionAttack(GameSession *session);

RestOutcome sessionRest(GameSession *session);

/* UnlockDoorCommand with a key (see interactUnlockFacing); on success the faced cell's door marker is cleared. */
InteractUnlockOutcome sessionUnlock(GameSession *session, uint16_t keyWord0, uint16_t heldKeyFlags);

/*
 * A spell cast by a party member (RunAlchemyScreen: the costs are paid first -- DeductAlchemySpellCosts -- then ApplyEncodedItemEffect runs the branch
 * spellSelectBranch picks). The member must know it (the ability bank), the context, the nuore / ore and the MP must allow it (spellCanCast). In a combat the
 * caster is the member whose turn it is (sessionCast) and the cast takes the turn like an attack; outside one the caster is named (sessionCastBy).
 *
 * Composed branches:
 *   attack      SpellFlagsBAttackPath (the first live monster), SpellFlagsBAttackAllSlots (every live monster), the LIFE FORCE spells (the caster or the whole
 *               party pays); combat only
 *   heal / cure FlagsB single target (the caster; HEAL, CURE POISON, RESURRECT, FEET OF FEATHERS...) and whole party (PARTY HEAL...): effect 18, a capped
 *               stat delta -- the record's 0x22 is the delta, 0x24 / 0x26 the byte offsets of the stat and its maximum in the party record (82 / 146 = hit
 *               points), 0x28 the mask ANDed into the status flags (party.h, partyApplyIconBarStatDelta); Chapter 3 skips a cursed member for the whole party
 *   light       MINER'S LIGHT I / II, INFINITE ILLUMINATION arm a light timer (lightsource.h)
 *   jump        JUMP OVER / JUMP THROUGH move the party if the path allows it (spelljump.h); a monster on the landing cell is left to the next monster pass
 *   mark        MARK OR RETURN: `returnToMark` false saves the position in the caster's record, true restores it (the original asks with a confirm prompt)
 *   knock       UNLOCK MAGIC (interactKnock), DISPEL ILLUSION (the curgame event), SAFE UNLOCK (lock flag 0x40 / priced / curgame flag 0x40) on the object ahead
 *   rest        SAFE HAVEN (sessionRest)
 *   held item   CREATE FOOD / FORGE put the item on the cursor if it is empty
 * SessionCastNoEffect: the costs were paid and nothing happened (a blocked jump, nothing ahead to open, nothing marked, a full cursor). Not composed
 * (SessionCastUnsupported, nothing paid): the projectile family, the screen-wide attacks, the beam / tremor / rain / turbulence spells -- they need monsters
 * on the viewport rows, which the session keeps in the pool.
 */
typedef enum { SessionCastDone, SessionCastNoCombat, SessionCastNotKnown, SessionCastCannot, SessionCastUnsupported, SessionCastNoEffect } SessionCast;
SessionCast sessionCast(GameSession *session, unsigned spellId);
SessionCast sessionCastBy(GameSession *session, unsigned casterSlot, unsigned spellId, bool returnToMark);

/*
 * Using a restorative from a member's main inventory (slot 1-8) on a recipient member (0-3): the healing / magic / cure items and, in Chapter 2, the percentage
 * healers (consumable.h). The item's charge is spent only when it did something; in a combat the use takes the member's turn. SessionUseNothing: no item
 * there; SessionUseNoEffect: it would do nothing for that recipient (the original flashes a warning and spends nothing); SessionUseUnsupported: not a restorative.
 */
typedef enum { SessionUseDone, SessionUseNothing, SessionUseNoEffect, SessionUseUnsupported } SessionUse;
SessionUse sessionUseItem(GameSession *session, unsigned userSlot, unsigned itemSlot, unsigned recipientSlot);

/* Takes every slot still in the chest ahead; returns how many. */
unsigned sessionLoot(GameSession *session);

/* One of the four active party members' records (0-3), or NULL. */
uint8_t *sessionPartyRecord(GameSession *session, unsigned slot);

#endif
