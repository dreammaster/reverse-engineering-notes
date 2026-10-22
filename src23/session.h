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
    /*
     * g_uiScratchFlags4's 0xE000 bits, taken from the first monster's flags (+0x92) when the fight starts: 0x8000 = it fights in a group at all, 0x4000 = a pair at most, 0x2000 = room for a third.
     * After each join they are recomputed (0x4000 when either of the two monsters has it, else 0x2000). The three slots are the left, middle and right of the view (51C0 / 525C / 52F8); the first
     * monster stands in the middle one, and a monster's layer (+0xA) says where it is drawn (one less on the left, one more on the right).
     */
    unsigned formation;
    /* g_activeCombatMonster: the slot the party's attacks fall on (CombatMonsterSlotCount = none chosen). It stays until the monster dies, then the first live one in the turn order (SelectActiveMonster)
     * takes over; a click on a monster's name in the right-hand panel chooses it. */
    unsigned target;
} SessionCombat;

/*
 * What the last fight or shot earned (ShowLootAndAwardExperience, yendor2.asm:33826): when the staged experience is not zero the rewards are added to the party's counters
 * and members and the front end shows them in the text panel -- GOLD COINS, MAGIC ORE and NUORE when they are not zero, and EXPERIENCE -- until the next command. `pending` is
 * set by every victory (a combat, a shot, a spell) and cleared by the front end.
 */
typedef struct {
    bool pending;
    Bcd4 gold, ore, nuore, experience;
} SessionLoot;

/*
 * The ranged shot, the S key and the first icon of the row (HandleRangedOrCombatAction with g_uiScratchFlags3 bit 0x100, yendor2.asm:23769; Chapter 2 only -- Chapter 3's routine has not been
 * identified): every member who can act (status bits 0x1C40 clear) and carries a weapon that classifies as one (item.h itemClassifyServiceTier) shoots it, which counts as a use of it (the
 * wear counter ticks, partyTickEquippedItemDurability; a weapon that breaks does not shoot). The shots fly together down the party's column of the view -- the four weapon slices of picture
 * 0x51 (rocks, for the sling ids 0x21E-0x223) or 0x50 (arrows) as one sprite at the cells 49, 46, 43, 40, 36, 25 (ShotRows), one frame per animation tick -- and what is in the way is classified
 * at each cell from 46 on (ClassifyObstacleAtViewportRow): clear, a wall (1) or an object (2) -- the shot is deflected (sound 6) -- a feature (3, nothing), or a monster (4), which is
 * fought one member at a time, first member first: combatResolveAttack(its Absorption, the shooter's EquipRating1 and 2), thrownApplyResolvedDamage; a miss sounds 0x1E, a hit 0xD (and the
 * monster flashes); the member's slice leaves the picture; a monster that dies is reaped and the rest of the shots go on to the next cell. The fights and the flight use one step each
 * (sessionShootStep, per animation tick; `wait` ticks between the sounds); the end awards the loot (monsterRewardsAward) and lets the monsters take their turn.
 */
typedef struct {
    bool active;
    bool fighting;          /* a monster has been reached: the shooters take it on one per step */
    bool shown;             /* the picture is on the screen (it goes with the first hit's redraw and comes back when the flight goes on) */
    unsigned row;           /* index into ShotRows of the cell the picture is at */
    unsigned wait;          /* animation ticks the next step waits */
    unsigned attacks;       /* g_attacksRemaining: members whose shot has not been fought yet */
    uint8_t weapon[4][2];   /* per party slot: the weapon item id (u16, low byte first; 0 = not shooting) */
    uint8_t picture[4];     /* per slot: the picture id of its slice (0x50 / 0x51) while it is in flight, else 0 */
    uint8_t pending[4];     /* per slot: 1 = still to shoot at a monster */
    unsigned target;        /* the pool record index of the monster being fought */
    MonsterRewardStaging staging;
} SessionShot;

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
    bool doorOpening;          /* a door is opening (sessionMove returned SessionStepDoorOpening): sessionScene draws the opening frame until sessionDoorFinish */
    SessionLoot loot;          /* the rewards of the last victory, for the text panel */
    SessionShot shot;          /* the ranged shot in flight (sessionShootStart): sessionScene draws it, sessionShootStep moves it */
    bool shareware;            /* the unregistered program: sessionInteract refuses the demo boundary (Chapter 2) */
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
    unsigned castTarget;       /* internal: the recipient slot + 1 of a single-target spell chosen on the alchemy screen (sessionCastOn), 0 = the caster */
    unsigned unlockMessage;    /* sessionUnlock: 1 = NOT LOCKED, 2 = UNLOCKED for the status panel (the front end clears it) */
    bool lockReport;           /* a step into a marked door cell was refused (sessionMove): `lockFlags` / `lockValue` / `lockSkill` are what ShowLockStatus (lockStatusText) needs; the front end writes the message and clears this */
    uint16_t lockFlags, lockValue, lockSkill;
    RestOutcome rest;          /* the last rest (sessionRest) ... */
    bool restReport;           /* ... and whether the front end has still to show it in the message area */
    AilmentClock ailments;  /* the counters of the periodic ailment sweep (ailment.h) */
    unsigned pendingFade;   /* 1 at 06:00, 2 at 18:00 (sessionAdvanceTime): the front end runs the dawn / dusk palette fade (palette.h) and clears it */
    unsigned worldTickCountdown; /* minutes to the next world tick (word_32954) */
    unsigned pendingMusic;  /* a music track a command asked for (the pan flute); the front end plays it and clears it */
    unsigned pendingSound;  /* the sound event the last command asked for (a travel arrival, a story event); the front end plays it and clears it */
    uint16_t heldItem, heldExtra; /* the item on the cursor (CREATE FOOD / FORGE put one there when the cursor is empty); 0 = none */
    SessionCombat combat;
    MonsterRewardStaging staged; /* the rewards of monsters whose timers ran out (GrantMonsterRewards stages them in globals): awarded with the next victory, or at the end of the monster pass when they carry experience */
    char log[96]; /* the last thing that happened, as the original's message area would say it */
} GameSession;

/* A new game from a WORLD.DAT image: the four ready-made heroes are the party, at the template's start position. NULL if the image lacks a table. */
GameSession *sessionNew(GameKind game, const uint8_t *worldDat, size_t size);
void sessionFree(GameSession *session);

/* RefreshDungeonMapWindow: grid + markers + monsters. Called by the commands below; a front end calls it once after loading a save. */
void sessionRebuild(GameSession *session);

typedef enum { SessionStepMoved, SessionStepTurned, SessionStepBlocked, SessionStepBusy, SessionStepDoorOpening } SessionStep;

/*
 * One movement command. SessionStepBusy: a combat is on (or the party is wiped) and nothing moves.
 *
 * Doors (HandleSpecialCellEntry, yendor2.asm:2480; Chapter 3 :6116): a door tile (wall types 6-11, Chapter 3 200-299, the cell not locked or trapped) is not stood on: the party passes it, two
 * cells in one command (the entry step and then the command's own step). Walking forward into one is SessionStepDoorOpening: nothing has moved yet, `doorOpening` is set, `pendingSound` is
 * the door sound (5), and sessionScene shows the frame of the door opening (the door cell drawn with a scratch tile -- the open door picture, the floor and ceiling of the cell beyond it); the
 * front end shows that frame for a moment and then calls sessionDoorFinish, which makes the two steps. Any other way into a door (backing, strafing) plays the sound and makes the two
 * steps at once. The second step is not checked (the original does not).
 *
 * After a step that moved the party (HandleMovementInput, loc_11598 on) a monster standing on the party's new cell is engaged at once (it need not be aware, and it could not have walked there:
 * a doorway's second step ends on the cell of a monster that has come up behind it), otherwise the monsters take their turns (ProcessLevelMonsters), and then the cell's trigger fires. A turn
 * on the spot and a step that is refused give the monsters no turn: the original calls ProcessLevelMonsters only when its "moved" flags (0x1800) are set.
 */
SessionStep sessionMove(GameSession *session, MovementAction action);

/* Completes an opening door: the two steps, the reveal, then what follows any step (above): the engagement, the monsters' turn and the map triggers. A no-op (SessionStepBlocked) when no door is opening. */
SessionStep sessionDoorFinish(GameSession *session);

/* The view cells, their occlusion and the monsters to draw (spawning the ones that come into view, monsterPoolEncounterScan); fills `scene` for viewRender. */
void sessionScene(GameSession *session, ViewScene *scene, DungeonGridCell cells[ViewportCellCount]);

/* The current party member's melee attack on the first live monster of the combat; the monsters then act until it is a party member's turn again. */
bool sessionAttack(GameSession *session);

/* The slot (0-2) the party's attacks fall on now, or CombatMonsterSlotCount when no monster is left; a click on a monster's name line (HandleDungeonInput :1667A) chooses another (false: no monster there). */
unsigned sessionCombatTarget(GameSession *session);
bool sessionCombatChoose(GameSession *session, unsigned slot);

/* The S key: false when nobody shoots (no weapon, a combat or another shot is on); else the shot is under way (`shot.active`, the launch sound -- Chapter 2 0x1D, Chapter 3 6 -- in `pendingSound`). */
bool sessionShootStart(GameSession *session);

/* One animation tick of the shot (see SessionShot); false once it is over. Sounds go to `pendingSound` one at a time. */
bool sessionShootStep(GameSession *session);

/*
 * The R command (rest.h). Sets `rest` and `restReport` (the front end shows "X HOURS PASS / X FOOD EATEN", "YOUR REST IS INTERRUPTED" or "YOU CAN NOT REST HERE" and clears the flag) unless nothing happened at
 * all (an item on the cursor: the original returns at once). A spot of the map-trigger table (maptrigger.h: IsPositionInTriggerList, also feeding ApplyMapTriggerEffect) refuses the rest.
 */
RestOutcome sessionRest(GameSession *session);

/*
 * ProcessSideTrapsOnMovement, run after every monster pass (`sessionMove`, the rest): each pool monster armed by its approach check (wound bit 0x1000) shoots at a random member of the first three
 * slots -- the member's EquipRating5 against the monster's ranged accuracy / damage decides whether and how hard (Chapter 3 rolls out of 55) -- and the hits apply the monster's ranged effect as
 * a preset amount. True when somebody was hit. The sound of the shot goes to `pendingSound`.
 */
bool sessionAmbushShots(GameSession *session);

/* A pool monster steps onto the party's cell (what the monster pass does for one that reaches it): it starts the fight, or joins the fight that is on when its flags and the formation allow it
 * (SessionCombat::formation). True when it was taken into a combat slot (the pool record is cleared then). */
bool sessionCombatEngage(GameSession *session, uint8_t *poolRecord);

/* UnlockDoorCommand with a key (see interactUnlockFacing); on success the faced cell's door marker is cleared. The sound, `unlockMessage` and (a lock that holds) `lockReport` are set for the front end. */
InteractUnlockOutcome sessionUnlock(GameSession *session, uint16_t keyWord0, uint16_t heldKeyFlags);

/* The K command: the key ring's own flags (item 0x2F, a master key) against the faced lock. */
InteractUnlockOutcome sessionUnlockWithRing(GameSession *session);
uint16_t sessionKeyRingFlags(const GameSession *session);

/*
 * The tool belt (savegame.h SaveBeltSlotCount slots of item id + extra, under the resource panel's materials): the map, clock, key ring and the like the K / M / T / W commands need
 * (sessionHasItem: IsItemRangeAvailable -- the belt first, then every member's inventory). sessionBeltClick is HandleStatusPanelItemSlotClick for the cursor item.
 */
typedef enum { SessionBeltNothing, SessionBeltTook, SessionBeltPlaced, SessionBeltSwapped, SessionBeltKeyRing, SessionBeltRefused } SessionBelt;
bool sessionHasItem(GameSession *session, unsigned lowId, unsigned highId);
uint8_t *sessionBeltSlot(GameSession *session, unsigned slot);
SessionBelt sessionBeltClick(GameSession *session, unsigned slot);

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

/* The same with the recipient of a single-target spell (SpellFlagsASingleTarget: the alchemy screen asks for a portrait first) named: a party slot 0-3. */
SessionCast sessionCastOn(GameSession *session, unsigned casterSlot, unsigned spellId, bool returnToMark, unsigned targetSlot);

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
 *   NothingHere   nothing is ahead or underfoot (ProbeFacingTile found no object): the status panel says NOTHING HERE (DS:0x7D71; Chapter 3 DS:0x809E)
 *   TravelFixed   Chapter 3's locked destination without a gate message: the three-line YOU CANNOT / ENTER HERE / RIGHT NOW (DS:0x9606)
 *   DemoLimit     the shareware program's boundary (EnforceDemoBoundary, yendor2.asm:867): the destination at (0x200, 0x2B) facing south is refused with the two-line
 *                 message at textOffset (REGISTER TODAY!) while `shareware` is set (Chapter 2 only; a registered program, the default, takes it)
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
    SessionInteractEndingNotReady,
    SessionInteractDemoLimit,
    SessionInteractNothingHere,
    SessionInteractTravelFixed
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

/*
 * The item slots the use commands take: 1-8 are the member's bag, `SessionBeltSlotBase` + n (n = 0-5) the tool belt (the right button on a belt slot of the resource panel,
 * TryCureAilmentFromIconClick -> HandleGameCommand). `sessionSlotItem` is the id in such a slot (0 = empty). What a scroll asks: `studyable` (the entry's flag 0x200: the prompt "USE ITEM? /
 * LEARN SPELL?" comes first) and `singleTarget` (the spell's own flag: the member the cast is for is chosen).
 */
enum { SessionBeltSlotBase = 100 };
unsigned sessionSlotItem(GameSession *session, unsigned memberSlot, unsigned itemSlot);

typedef struct {
    bool studyable, singleTarget;
} SessionScrollInfo;
bool sessionScrollInfo(GameSession *session, unsigned userSlot, unsigned itemSlot, SessionScrollInfo *out);

/* Line `line` of the document sessionUseItem last opened (spacing kept); false past its end. */
bool sessionDocumentLine(GameSession *session, unsigned line, char out[DocumentLineBufferSize]);

/* Takes every slot still in the chest ahead; returns how many. */
unsigned sessionLoot(GameSession *session);

/*
 * Opening what is ahead (the Space command on a lock object: UseAbilityCommand, yendor2.asm:12820): the member who opens is the third role's (header +0xA8; a prompt "WHO WILL OPEN?" -- "SEARCH?"
 * for a lock with flag 0x2 -- asks when none is chosen, and one who cannot act is refused and asked again). A lock that is not trapped must be unlocked already (sessionUnlock, a key or a spell)
 * or the way it is locked is reported (`lockReport`). A trapped lock (flag 0x80) opens at the first try and sets its trap off: the opener's Thievery and level can avoid it, the trap word
 * (the lock's +2) decides the effect on the opener or the party, and a defeated trap says so (TRAP DEFEATED, `trapDefeated`). Then the contents screen opens.
 */
typedef enum { SessionChestNothing, SessionChestAskWho, SessionChestLocked, SessionChestOpen } SessionChestResult;

typedef struct {
    SessionChestResult result;
    bool askSearch;     /* AskWho: the prompt reads SEARCH? (message 6), else OPEN? (5) */
    unsigned lockId;    /* Open: the lock record the contents screen shows */
    bool inexhaustible; /* the lock's flag 0x2 */
    bool trapDefeated;
} SessionChest;

SessionChest sessionChestOpen(GameSession *session);
bool sessionChestChooseOpener(GameSession *session, unsigned memberSlot); /* false when that member cannot act */

/*
 * The contents screen (RunShopScreen with the chest's flag: HandleShopCatalogSlotClick, yendor2.asm:11895): `slot` 0-7 is a click on one of the eight icons. With an empty hand a pile (GOLD COINS, MAGIC
 * ORE, NUORE) joins the party's counter at once (sound 7, `taken` says which and how much) and any other item goes to the cursor (its extra its charge); either way the slot is taken for good.
 * With an item held, a taken slot whose original item is the same takes it back (the slot reads full again). SessionChestRefused: a beep.
 */
typedef enum { SessionChestSlotNothing, SessionChestSlotItem, SessionChestSlotPile, SessionChestSlotPutBack, SessionChestSlotRefused } SessionChestSlot;
unsigned sessionChestSlots(GameSession *session, unsigned lockId, ChestSlot out[LockContentSlots]);
SessionChestSlot sessionChestClick(GameSession *session, unsigned lockId, unsigned slot, ChestSlot *taken);

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

/*
 * A left click on one of the slots of a member's expanded doll on the inventory screen (HandleInventorySlotClick, yendor2.asm:39344; `region` is the UiRegionsInventoryGrid result: 1-8 the bag
 * slots, 9 nothing, 10-15 the weapon / B / C / D slots and the two rings, 16-21 the five worn-clothing slots). With an empty hand the slot's item goes to the cursor; with an item held it must
 * suit the slot (inventory.h inventoryEligibleForSlot) and the carry weight must allow it (the member's +0x118 total against his or her capacity +0x56, the exchange counting only the
 * difference), then it is put in an empty slot or swapped (sound 6; a worn item brings its stat effect, the wear counter is cleared); a key over a key ring in a slot goes on the ring (sound 4).
 * Containers (bags, boxes: their contents live in item-instance records the standalone does not open) are refused.
 */
typedef enum { SessionInvIgnored, SessionInvTook, SessionInvPlaced, SessionInvSwapped, SessionInvKeyRing, SessionInvRefused } SessionInv;
SessionInv sessionInventoryClick(GameSession *session, unsigned member, unsigned region);

/* TryDropHeldItem (yendor2.asm:18109): whether the cursor item may be dropped (not a keepsake or a container holding one), and the drop itself (PlaceItemOnGround: the item ceases to exist). */
bool sessionHeldDroppable(GameSession *session);
void sessionDropHeld(GameSession *session);
bool sessionPickUp(GameSession *session, unsigned member, unsigned slotNumber, unsigned equipmentCode);

/*
 * The save image (savegame.h: the CURGAME / SAVGAMEn file of the original, byte for byte): sessionSave first copies the session's own state into the header (position,
 * facing, clock, the place flags, Chapter 2's lit lights and timers) and the monster pool into its section, then stores the whole image; sessionLoad replaces the state
 * with an image of the same game (false otherwise) and rebuilds the window. A combat in progress is not saved; the cursor item neither.
 */
size_t sessionSave(GameSession *session, uint8_t *out, size_t capacity);
bool sessionLoad(GameSession *session, const uint8_t *data, size_t size);

/*
 * The title screen's New Game (InitializeNewGameWorldState, newgame.h): the game starts over from the template in the world data -- the roster of ready-made heroes, the start position and
 * clock, nothing explored but the cells round the start -- and, unlike sessionNew, with no party: the four slots are empty until the player assembles one (the title's E is refused until
 * then). False when the world data lacks the template.
 */
bool sessionNewGame(GameSession *session, const uint8_t *worldDat, size_t size);

/* Replaces the four active members with characters made elsewhere (chargen.h: partyChooseClass, partyRollAttributes...; charcreate.c's screens); the party keeps its start position. */
void sessionSetParty(GameSession *session, const uint8_t records[4][SavePartyRecordSize]);

/* One of the four active party members' records (0-3), or NULL. */
uint8_t *sessionPartyRecord(GameSession *session, unsigned slot);

#endif
