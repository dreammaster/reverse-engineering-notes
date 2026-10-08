#ifndef YENDOR23_SESSION_H
#define YENDOR23_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "chest.h"
#include "ailment.h"
#include "combat.h"
#include "conversation.h"
#include "document.h"
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
#include "storyevent.h"
#include "travel.h"
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
    DialogCatalog dialogs;  /* the NPCs, topics and lines of the game (dialog.h) */
    Conversation talk;      /* the talk in progress (talking) or the last one */
    bool talking;
    unsigned talkSpeaker;      /* the party slot (0-3) of the member who talks: the one the healer, trainer and challenge services act on */
    uint16_t serviceType;      /* the healer's last quoted type (word_3298E) / the challenge's outcome */
    Bcd4 serviceCost;          /* the last quote of a healer, trainer or challenge, packed BCD (512Ah) */
    uint16_t travelFlags;   /* Chapter 3's ds:0CEF9 (saved in the header at +0x1C); Chapter 2 keeps its place bits in light.litFlags */
    unsigned pendingTravel; /* a destination waiting for its password (SessionInteractTravelPassword) */
    DocumentCatalog documents; /* the readable texts (document.h) */
    unsigned readCategory, readId; /* the document the last SessionUseRead opened */
    DocumentReadability readability; /* how much of it the reader could read */
    bool scrollCast;           /* internal: a spell being cast from a scroll (no knowledge, no cost, attacks "already resolved") */
    unsigned scrollTarget;     /* its single-target recipient slot */
    AilmentClock ailments;  /* the counters of the periodic ailment sweep (ailment.h) */
    unsigned pendingFade;   /* 1 at 06:00, 2 at 18:00 (sessionAdvanceTime): the front end runs the dawn / dusk palette fade (palette.h) and clears it */
    unsigned worldTickCountdown; /* minutes to the next world tick (word_32954) */
    unsigned pendingMusic;  /* a music track a command asked for (the pan flute); the front end plays it and clears it */
    unsigned pendingSound;  /* the sound event the last command asked for (a travel arrival, a story event); the front end plays it and clears it */
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
 * SessionCastNoEffect: the costs were paid and nothing happened (a blocked jump, nothing ahead to open, nothing marked, a full cursor, a shot at nothing).
 * Outside a combat the projectile family, the screen-wide attacks and the beam / tremor / rain / turbulence spells attack the monsters in front: a projectile flies down the
 * centre column of the view (cells 49, 46, 43, 40, 36, 25) and a splash one hits three runs of three cells around the impact, the screen-wide family hits every monster in the
 * 51-cell buffer and the beam the first one of the column (session.c castProjectile / castColumnAttack; GetMonsterAtViewportRow's marker lookup). SessionCastUnsupported is
 * kept for branches without a composition (none now).
 */
typedef enum { SessionCastDone, SessionCastNoCombat, SessionCastNotKnown, SessionCastCannot, SessionCastUnsupported, SessionCastNoEffect } SessionCast;
SessionCast sessionCast(GameSession *session, unsigned spellId);
SessionCast sessionCastBy(GameSession *session, unsigned casterSlot, unsigned spellId, bool returnToMark);

/*
 * Using a restorative from a member's main inventory (slot 1-8) on a recipient member (0-3): the healing / magic / cure items and, in Chapter 2, the percentage
 * healers (consumable.h). The item's charge is spent only when it did something; in a combat the use takes the member's turn. SessionUseNothing: no item
 * there; SessionUseNoEffect: it would do nothing for that recipient (the original flashes a warning and spends nothing); SessionUseUnsupported: neither a restorative nor a
 * key. A key to a destination (ExamineTarget: the item's target entry has flag 4 and no container bits) takes the party there when its gate flag is set (Travelled), else
 * shows its description (Closed) or, in Chapter 3 inside the artifact realm holding a light weapon, refuses (Blocked); nothing is spent. A readable item answers Read: `readCategory` / `readId` name the text, `readability` says how many words of five the reader's Linguistics can read (sessionDocumentLine gives the lines). The light sources (items 8 / 0xE / 0xB, lit 9 / 0xF / 0xC) light and go out, and in Chapter 2 the quest relics (relics.h) work.
 */
typedef enum { SessionUseDone, SessionUseNothing, SessionUseNoEffect, SessionUseUnsupported, SessionUseTravelled, SessionUseClosed, SessionUseBlocked, SessionUseRead, SessionUseScroll } SessionUse;
SessionUse sessionUseItem(GameSession *session, unsigned userSlot, unsigned itemSlot, unsigned recipientSlot);

/*
 * The use command on the object ahead (the click on the view in `start`, yendor2.asm:1058E): the party's own cell first, else the next one; the object's flag picks
 * what happens, in the original's order -- door 0x8000, destination 0x2000, talker 0x1000, story event 0x400.
 *   Door          a lock or chest: sessionUnlock / sessionLoot
 *   Travelled     a destination (travel.h): the party is there (sessionScene shows it; `pendingSound` holds the arrival sound; the place flags, the day / night tracks
 *                 and, in Chapter 3, the travel mode are in the save header)
 *   TravelRefused a locked destination; `messageId` is the canned rejection (travelCheckUnlock)
 *   TravelPassword the destination asks for a password (`promptId`): answer with sessionTravelPassword
 *   Talker        an NPC (`value` = the id, dialog.h)
 *   StoryEvent    Chapter 3's scripted teleports (storyevent.h), carried out; `textOffset` / `textLines` name a message in the executable's data segment to show
 *   StoryRefused  the event's condition is not met (`textOffset`: the one line it shows)
 *   Ending        Chapter 2's Quartz Chamber with the three quest flags: play introEndingScript (the original also sets the clock to 08:00)
 *   EndingNotReady the same without them: the two-line message at textOffset
 */
typedef enum {
    SessionInteractNothing,
    SessionInteractDoor,
    SessionInteractTravelled,
    SessionInteractTravelRefused,
    SessionInteractTravelPassword,
    SessionInteractTalker,
    SessionInteractStoryEvent,
    SessionInteractStoryRefused,
    SessionInteractEnding,
    SessionInteractEndingNotReady
} SessionInteractKind;

typedef struct {
    SessionInteractKind kind;
    unsigned value;
    unsigned messageId, promptId;
    unsigned textOffset, textLines;
} SessionInteract;

SessionInteract sessionInteract(GameSession *session);

/*
 * Talking (conversation.h; the loop of UseItem). sessionInteract reports SessionInteractTalker with the NPC id; sessionTalkStart then loads the NPC, runs its opening
 * topic and lists the topics (`talk.listed`, up to ten; `sessionTalkChoose(n)` runs the n-th, 0 = Escape's choice, the last). Each step says what the topic asks of
 * the caller (ConversationStep); sessionTalkService carries out the services that need no screen:
 *   attribute / experience tomes (dialogservice.h), the ore purchase (`text` = the quantity typed) and the riddle answer (`text` = the answer): Done / Failed, and a
 *   conditional topic is resolved with the outcome;
 *   SessionServiceNeedText: the topic needs the typed text; SessionServiceNeedScreen: the healer, trainer, challenge, mount, shop, sell, enhance and repair services (their decision
 *   modules are dialogservice.h, shop.h, chest.h; the choice of party member and item is the front end's); None: a plain topic.
 * `speaker` is the party slot (0-3) that talks (the original asks only when the NPC's speaker mode is set; a member who cannot act is refused then). The healer, trainer,
 * challenge and mount services (the type word the NPC's greeting loads, dialog.h) are carried out for that member by sessionTalkService as well: a quote topic fills
 * `serviceCost`, the accept topic (argument bit 2) pays it and does the work (dialogservice.h; party.h's partyApplyTraining), the refresh topic (bit 4) re-marks what
 * can be offered, bit 1 leaves the service. The sell, enhance and repair topics work on the cursor item (`heldItem`; NeedItem when it is empty): sell pays shopSellPrice for an item of the topic's class, enhance swaps in the next catalog id for the NPC's price, repair restores the id parked in the extra word. The shop topics (DialogTopicUseKey) answer NeedScreen: sessionShop*.
 */
typedef enum { SessionServiceNone, SessionServiceDone, SessionServiceFailed, SessionServiceNeedText, SessionServiceNeedScreen, SessionServiceNeedItem } SessionService;

bool sessionTalkStart(GameSession *session, unsigned npcId, unsigned speaker, ConversationStep *first);
ConversationStep sessionTalkChoose(GameSession *session, unsigned number);
void sessionTalkResolve(GameSession *session, bool success);
SessionService sessionTalkService(GameSession *session, const ConversationStep *step, const char *text);

/* The typed password for the destination sessionInteract asked about; true (and the party travels) when it is right. */
bool sessionTravelPassword(GameSession *session, const char *typed);

/*
 * Game time passing, minute by minute as AdvanceGameClock does (the original is driven by a timer; the front end decides how fast): the clock and calendar (a day is 1440
 * minutes, 30-day months, the original's 1441 bound reproduced), the daily ability charges reset at a new day, `pendingFade` set at 06:00 and 18:00, and, every fifth
 * minute while a light burns, TickWorldAilments: the lit candles and torches and the spell timers burn down. (sessionRest advances the clock by itself.)
 */
void sessionAdvanceTime(GameSession *session, unsigned minutes);

/*
 * One pass of the main loop's ailment sweep (TickPartyAilmentIconBar, ailment.h): every fortieth call the diseased / poisoned / sick lose hit points and the cursed / hexed /
 * jinxed lose magic points, in a Chapter 2 "slow" place (the place flag bit 2) the survival skill is tested every call, in Chapter 3 a cold place (bit 1) chills the party.
 * Call it once per pass of the front end's loop (not while it waits for a key).
 */
void sessionTickAilments(GameSession *session);

/*
 * A spell scroll (sessionUseItem answers SessionUseScroll; InteractWithContainer, yendor2.asm:53446): cast it -- the scroll's spell (target entry word 2) runs with no cost, even
 * for a member who does not know it, its attacks "already resolved", a single-target one on `memberSlot`, the context rules of the spell as for a cast, and the scroll spends a
 * charge -- or, when the scroll allows it (entry flag 0x200), `study` it: `memberSlot` learns the spell if spellCanLearn says so (not known yet, class and level fit).
 */
SessionCast sessionUseScroll(GameSession *session, unsigned userSlot, unsigned itemSlot, bool study, unsigned memberSlot);

/* Line `line` of the document sessionUseItem last opened (spacing kept); false past its end. */
bool sessionDocumentLine(GameSession *session, unsigned line, char out[DocumentLineBufferSize]);

/* Takes every slot still in the chest ahead; returns how many. */
unsigned sessionLoot(GameSession *session);

/*
 * The shop screen of a "buy" topic (DialogTopicUseKey: UseKeyItem sets the buy mode and runs RunShopScreen on the lock record named by the topic's argument; the
 * same record opened as a chest is free, sessionLoot). Stock is inexhaustible: `sessionShopStock` lists the eight slots with the price of each --
 * shopBuyPrice of the item's base value for the haggling member (the first role assignment, else the first member: Bartering) -- and `sessionShopBuy` pays it and puts
 * the item on the cursor (`heldItem`; the hand must be empty). `sessionShopSell` takes the cursor item back for shopSellPrice if the shop stocks that item
 * (HandleShopCatalogSlotClick's buy-mode branch); `sessionStowHeld` moves the cursor item into a member's first free inventory slot.
 */
typedef struct {
    uint16_t itemId, extra; /* 0 = empty slot */
    uint8_t price[4];       /* packed BCD */
} SessionShopItem;

typedef enum { SessionShopBought, SessionShopSold, SessionShopHandFull, SessionShopEmpty, SessionShopNoGold, SessionShopNotStocked } SessionShop;

unsigned sessionShopStock(GameSession *session, unsigned lockId, SessionShopItem out[LockContentSlots]);
SessionShop sessionShopBuy(GameSession *session, unsigned lockId, unsigned slot);
SessionShop sessionShopSell(GameSession *session, unsigned lockId);
bool sessionStowHeld(GameSession *session, unsigned member);

/*
 * The cursor item and the inventory (the inventory screen without its screen): sessionDropOnPortrait is HandleItemDropOnPartyPortrait -- the held item is equipped in its
 * slot when it has one and it is free (the stat effect applied, the wear counter cleared), else stowed in the first free inventory slot, the carry weight checked and the
 * weight added (inventoryPlanAutoEquip); false = refused. sessionPickUp takes an item to the cursor from a main-inventory slot (1-8) or, with `equipmentCode` (0xA-0x14),
 * from that equipment slot (its stat effect taken off, the weight subtracted).
 */
bool sessionDropOnPortrait(GameSession *session, unsigned member);
bool sessionPickUp(GameSession *session, unsigned member, unsigned slotNumber, unsigned equipmentCode);

/*
 * The save image (savegame.h: the CURGAME / SAVGAMEn file of the original, byte for byte): sessionSave first copies the session's own state into the header (position,
 * facing, clock, the place flags, Chapter 2's lit lights and timers) and the monster pool into its section, then stores the whole image; sessionLoad replaces the state
 * with an image of the same game (false otherwise) and rebuilds the window. A combat in progress is not saved; the cursor item neither.
 */
size_t sessionSave(GameSession *session, uint8_t *out, size_t capacity);
bool sessionLoad(GameSession *session, const uint8_t *data, size_t size);

/* Replaces the four active members with characters made elsewhere (chargen.h: partyChooseClass, partyRollAttributes...; charcreate.c's screens); the party keeps its start position. */
void sessionSetParty(GameSession *session, const uint8_t records[4][SavePartyRecordSize]);

/* One of the four active party members' records (0-3), or NULL. */
uint8_t *sessionPartyRecord(GameSession *session, unsigned slot);

#endif
