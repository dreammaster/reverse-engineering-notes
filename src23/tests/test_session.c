/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_session test_session.c ../session.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
 *       ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c \
 *       ../lighting.c ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../spellrecord.c ../spellcast.c ../consumable.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c ../conversation.c ../dialog.c ../dialogservice.c ../shop.c && ./test_session
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd4.h"
#include "lightsource.h"
#include "dialogservice.h"
#include "globalflags.h"
#include "spellcast.h"
#include "session.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static uint8_t *readFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

static void place(GameSession *s, int x, int y, uint16_t facing) {
    s->x = x;
    s->y = y;
    s->facing = facing;
    sessionRebuild(s);
}

static void testGame(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    GameSession *s = sessionNew(game, world, size);
    check(label, s != NULL);
    if (!s) {
        return;
    }
    check("the party is the four heroes and the window is built", sessionPartyRecord(s, 0) && sessionPartyRecord(s, 3) && !sessionPartyRecord(s, 4) &&
                                                                   dungeonGridCellAtWorldPos(&s->grid, s->x, s->y) != NULL);

    uint16_t facing = s->facing;
    check("turning left changes the facing and nothing else", sessionMove(s, MovementTurnLeft) == SessionStepTurned && s->facing != facing);
    sessionMove(s, MovementTurnRight);
    check("... and right turns back", s->facing == facing);

    int startX = s->x, startY = s->y;
    unsigned moved = 0;
    for (int i = 0; i < 20; i++) {
        moved += sessionMove(s, MovementForward) == SessionStepMoved;
    }
    check("walking forward moves the party until a wall stops it", moved >= 1 && (s->x != startX || s->y != startY) && moved < 20);

    /* a healing potion from the inventory (Chapter 2: item 0x14 heals fully; Chapter 3: 0x36) */
    uint8_t *hero = sessionPartyRecord(s, 1);
    uint16_t potion = game == GameYendor2 ? 0x14 : 0x36;
    itemSlotSet(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, 0), PartyGroupMain), 3), potion, 1);
    partySetStat(hero, PartyStatHitPoints, 1);
    SessionUse used = sessionUseItem(s, 0, 3, 1);
    check("a healing potion heals the recipient to full", used == SessionUseDone && partyGetStat(hero, PartyStatHitPoints) == partyGetStatMax(hero, PartyStatHitPoints));
    check("... and is used up when it was a single-use item", sessionUseItem(s, 0, 3, 1) != SessionUseDone);
    check("an empty slot is refused", sessionUseItem(s, 0, 5, 1) == SessionUseNothing);

    RestOutcome rest = sessionRest(s);
    check("resting uses up the time (eight hours or an interruption)", !rest.refused && (rest.interrupted || rest.minutes == 480));

    /* random play on every game: the invariants must hold */
    unsigned seed = 1, bad = 0;
    GameSession *soak = sessionNew(game, world, size);
    for (unsigned i = 0; i < 1500 && soak; i++) {
        seed = seed * 1103515245u + 12345u;
        unsigned pick = (seed >> 16) % 14;
        if (pick < 6) {
            sessionMove(soak, MovementForward);
        } else if (pick < 8) {
            sessionMove(soak, MovementTurnLeft);
        } else if (pick < 9) {
            sessionMove(soak, MovementTurnRight);
        } else if (pick < 10) {
            sessionMove(soak, MovementBackward);
        } else if (pick < 13) {
            sessionAttack(soak);
        } else {
            sessionRest(soak);
        }
        ViewScene scene;
        DungeonGridCell cells[ViewportCellCount];
        sessionScene(soak, &scene, cells);
        bad += !movementInBounds(game, (uint16_t)soak->x, (uint16_t)soak->y);
        bad += soak->combat.active && soak->combat.wiped;
        bad += soak->clock.minutes >= 1440;
    }
    check("1500 random commands keep the party in bounds and the clock in a day", bad == 0);
    sessionFree(soak);
    sessionFree(s);
    free(world);
}

static void testCombat(const char *envName, const char *defaultDir) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        return;
    }
    GameSession *s = sessionNew(GameYendor2, world, size);
    place(s, 161, 31, SaveFacingNorth); /* spiders walk the corridors around here */
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    unsigned turns = 0;
    while (!s->combat.active && turns < 300) { /* turn on the spot and let them come */
        sessionScene(s, &scene, cells);
        sessionMove(s, turns & 1 ? MovementTurnRight : MovementTurnLeft);
        turns++;
    }
    check("monsters reach a standing party and start a combat", s->combat.active);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), 50); /* the spell costs nuore (and some need ore) */
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 50);
    unsigned swings = 0, casts = 0, refused = 0;
    while (s->combat.active && swings < 400) {
        SessionCast cast = sessionCast(s, 2); /* the attack spell the second and third heroes know */
        if (cast == SessionCastDone) {
            casts++;
        } else {
            refused += cast == SessionCastCannot;
            sessionAttack(s);
        }
        swings++;
    }
    check("the attack spell is cast by the heroes who know it (and refused when the MP are gone), the others swing", casts > 0 && casts + refused <= swings);
    check("the combat ends one way or the other", !s->combat.active);
    if (!s->combat.wiped) {
        check("a won combat leaves the monster out of the pool and the move command free again", sessionMove(s, MovementTurnLeft) == SessionStepTurned);
    } else {
        check("a wiped party cannot move", sessionMove(s, MovementTurnLeft) == SessionStepBusy);
    }
    sessionFree(s);

    /* loot and unlock: the chest ahead of (163, 43) facing north has contents; the second visit finds it empty */
    s = sessionNew(GameYendor2, world, size);
    place(s, 163, 43, SaveFacingNorth);
    unsigned first = sessionLoot(s), second = sessionLoot(s);
    check("a chest is looted once", first > 0 && second == 0);
    place(s, 163, 43, SaveFacingNorth);
    check("the taken slots stay taken after the window is rebuilt", sessionLoot(s) == 0);
    InteractUnlockOutcome nothing = sessionUnlock(s, 0xFFC0, 0);
    check("unlocking with nothing ahead of interest reports a result without crashing", nothing.result <= InteractUnlockLocked);
    sessionFree(s);
    free(world);
}

static unsigned findSpell(GameSession *s, SpellBranch branch, unsigned skip) {
    for (unsigned id = 1; id <= s->spells.recordCount; id++) {
        const uint8_t *r = spellRecord(&s->spells, id);
        if (r && spellSelectBranch(r) == branch && spellGetU16(r, SpellFieldMpCost) > 0 && skip-- == 0) {
            return id;
        }
    }
    return 0;
}

/* The support and exploration spells: every hero knows everything and has the points for it. */
static void testSpells(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *s = sessionNew(game, world, size);
    if (!s) {
        return;
    }
    for (unsigned m = 0; m < 4; m++) {
        uint8_t *hero = sessionPartyRecord(s, m);
        for (unsigned id = 1; id <= s->spells.recordCount; id++) {
            spellLearn(hero, id);
        }
        partySetStat(hero, PartyStatMagicPoints, 900);
    }
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), 90);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 90);
    char text[160];

    unsigned heal = findSpell(s, SpellBranchSingleTarget, 0), partyHeal = findSpell(s, SpellBranchWholeParty, 0);
    uint8_t *hero = sessionPartyRecord(s, 1);
    partySetStat(hero, PartyStatHitPoints, 1);
    uint16_t before = partyGetStat(hero, PartyStatHitPoints), mp = partyGetStat(hero, PartyStatMagicPoints);
    SessionCast cast = sessionCastBy(s, 1, heal, false);
    snprintf(text, sizeof(text), "%s: a single-target heal raises the caster's hit points and costs magic points", label);
    check(text, cast == SessionCastDone && partyGetStat(hero, PartyStatHitPoints) > before && partyGetStat(hero, PartyStatMagicPoints) < mp);
    for (unsigned m = 0; m < 4; m++) {
        partySetStat(sessionPartyRecord(s, m), PartyStatHitPoints, 1);
    }
    cast = sessionCastBy(s, 0, partyHeal, false);
    bool all = cast == SessionCastDone;
    for (unsigned m = 0; m < 4; m++) {
        all = all && partyGetStat(sessionPartyRecord(s, m), PartyStatHitPoints) > 1;
    }
    snprintf(text, sizeof(text), "%s: the whole-party heal reaches every member", label);
    check(text, all);

    unsigned light = 0;
    for (unsigned id = 1; id <= s->spells.recordCount; id++) {
        const uint8_t *r = spellRecord(&s->spells, id);
        light = light ? light : (spellSelectBranch(r) == SpellBranchLightTimer ? id : 0);
    }
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    int16_t dark = scene.gradient[0];
    s->clock.minutes = 0; /* midnight */
    sessionScene(s, &scene, cells);
    dark = scene.gradient[0];
    cast = sessionCastBy(s, 0, light, false);
    sessionScene(s, &scene, cells);
    snprintf(text, sizeof(text), "%s: a light spell lights the view and the timer runs out with the clock", label);
    check(text, cast == SessionCastDone && s->light.litFlags != 0 && scene.gradient[0] > dark);
    s->light.timers[0] = s->light.timers[1] = s->light.timers[2] = s->light.timers[3] = s->light.timers[4] = s->light.timers[5] = 1;
    lightSourceTickTimers(&s->light, 600);
    check("... (all timers expire)", (s->light.litFlags & 0x1F8) == 0);

    /* the mark: saved, the party walks away, returned */
    place(s, 161, 31, SaveFacingNorth);
    unsigned mark = findSpell(s, SpellBranchLocationBookmark, 0);
    cast = sessionCastBy(s, 0, mark, false);
    s->x += 3;
    s->y += 2;
    sessionRebuild(s);
    SessionCast back = sessionCastBy(s, 0, mark, true);
    snprintf(text, sizeof(text), "%s: MARK saves the position and RETURN brings the party back", label);
    check(text, cast == SessionCastDone && back == SessionCastDone && s->x == 161 && s->y == 31);

    /* the jump: the walk-through spell either moves the party two cells or finds the way blocked */
    unsigned jump = findSpell(s, SpellBranchTeleportEngage, 0);
    unsigned jumped = 0, wrong = 0;
    for (unsigned turn = 0; turn < 4; turn++) {
        int cx = s->x, cy = s->y;
        bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), 90);
        bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 90);
        cast = sessionCastBy(s, 0, jump, false);
        int moved = abs(s->x - cx) + abs(s->y - cy);
        jumped += cast == SessionCastDone;
        wrong += !((cast == SessionCastDone && moved == 2) || (cast == SessionCastNoEffect && moved == 0));
        place(s, cx, cy, s->facing);
        sessionMove(s, MovementTurnRight);
    }
    snprintf(text, sizeof(text), "%s: a jump moves the party two cells or does nothing, and works in some direction", label);
    check(text, jumped >= 1 && wrong == 0);

    /* created items land on the cursor once */
    unsigned held = findSpell(s, SpellBranchHeldItem, 0);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), 90);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 90);
    if (held) {
        SessionCast first = sessionCastBy(s, 0, held, false), second = sessionCastBy(s, 0, held, false);
        snprintf(text, sizeof(text), "%s: a created item lands on the empty cursor and not on a full one", label);
        check(text, first == SessionCastDone && s->heldItem != 0 && second == SessionCastNoEffect);
    }

    /* a spell that needs monsters on the viewport rows is not composed and costs nothing */
    unsigned projectile = findSpell(s, SpellBranchProjectile, 0);
    uint16_t mpBefore = partyGetStat(sessionPartyRecord(s, 0), PartyStatMagicPoints);
    check("a projectile spell is reported unsupported and charges nothing",
          sessionCastBy(s, 0, projectile, false) == SessionCastUnsupported && partyGetStat(sessionPartyRecord(s, 0), PartyStatMagicPoints) == mpBefore);

    /* a heal in a combat takes the turn */
    place(s, 161, 31, SaveFacingNorth);
    unsigned turns = 0;
    while (!s->combat.active && turns < 300) {
        sessionScene(s, &scene, cells);
        sessionMove(s, turns & 1 ? MovementTurnRight : MovementTurnLeft);
        turns++;
    }
    if (s->combat.active && !s->combat.order[s->combat.cursor].isMonster) {
        unsigned actor = s->combat.order[s->combat.cursor].index;
        partySetStat(sessionPartyRecord(s, actor), PartyStatHitPoints, 1);
        uint16_t hp = partyGetStat(sessionPartyRecord(s, actor), PartyStatHitPoints);
        unsigned wrongSlot = (actor + 1) % 4;
        check("in a combat only the member whose turn it is can cast", sessionCastBy(s, wrongSlot, heal, false) == SessionCastCannot);
        check("a heal in a combat works and the turn passes", sessionCastBy(s, actor, heal, false) == SessionCastDone && partyGetStat(sessionPartyRecord(s, actor), PartyStatHitPoints) != hp);
    }
    sessionFree(s);
}

static unsigned wordAt(const uint8_t *p) {
    return (unsigned)(p[0] | (p[1] << 8));
}

/* The first world object whose flags include `flag` (and, if `value` is nonzero, with that value); false if there is none. */
static bool findObject(const GameSession *s, unsigned flag, unsigned value, int *col, int *row) {
    for (unsigned c = 0; c < WorldObjectColumns; c++) {
        for (unsigned at = wordAt(s->objects.data + 2 * c);; at += 6) {
            unsigned y = wordAt(s->objects.data + at);
            if (y == 0xFFFF) {
                break;
            }
            if ((wordAt(s->objects.data + at + 2) & flag) && (!value || wordAt(s->objects.data + at + 4) == value)) {
                *col = (int)(c + WorldObjectColumnMin);
                *row = (int)y;
                return true;
            }
        }
    }
    return false;
}

static void stand(GameSession *s, int col, int row) { /* one cell south of the object, looking north */
    place(s, col, row + 1, SaveFacingNorth);
}

/* The use command: destinations, talkers and story events of both games. */
static void testInteract(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *s = sessionNew(game, world, size);
    if (!s) {
        return;
    }
    char text[160];
    int col, row;
    snprintf(text, sizeof(text), "%s: nothing ahead means nothing happens", label);
    place(s, 5, 5, SaveFacingNorth);
    check(text, sessionInteract(s).kind == SessionInteractNothing);

    /* a destination without a gate */
    bool travelled = false;
    for (unsigned id = 1; id <= travelDestinationCount(game) && !travelled; id++) {
        TravelDestination d;
        travelDestinationLookup(game, id, &d);
        unsigned gate = game == GameYendor2 ? 0x2000u : 0xC000u;
        int c, r;
        if ((d.flags & gate) || !findObject(s, WorldObjectFlagUnknown2000, id, &c, &r)) {
            continue;
        }
        stand(s, c, r);
        SessionInteract use = sessionInteract(s);
        TravelArrival a = travelArrival(game, &d, 0, 600);
        travelled = use.kind == SessionInteractTravelled && s->x == a.x && s->y == a.y && s->facing == a.facing;
    }
    snprintf(text, sizeof(text), "%s: using an ungated destination object moves the party to its destination", label);
    check(text, travelled);

    snprintf(text, sizeof(text), "%s: a talker object reports its NPC", label);
    check(text, findObject(s, WorldObjectFlagTalker, 0, &col, &row) && (stand(s, col, row), sessionInteract(s).kind == SessionInteractTalker));

    /* a gated destination: refused or asking for a password until its flag is set */
    bool gated = false;
    for (unsigned id = 1; id <= travelDestinationCount(game) && !gated; id++) {
        TravelDestination d;
        travelDestinationLookup(game, id, &d);
        unsigned gate = game == GameYendor2 ? 0x2000u : 0xC000u;
        int c, r;
        if (!(d.flags & gate) || !findObject(s, WorldObjectFlagUnknown2000, id, &c, &r)) {
            continue;
        }
        TravelUnlockCheck unlock = travelCheckUnlock(game, id, d.flags, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
        if (unlock.outcome != TravelUnlockNeedsPassword && unlock.outcome != TravelUnlockDeniedWithMessage) {
            continue;
        }
        stand(s, c, r);
        int x = s->x, y = s->y;
        SessionInteract use = sessionInteract(s);
        gated = (unlock.outcome == TravelUnlockNeedsPassword ? use.kind == SessionInteractTravelPassword : use.kind == SessionInteractTravelRefused) && s->x == x && s->y == y;
    }
    snprintf(text, sizeof(text), "%s: a locked destination leaves the party where it is", label);
    check(text, gated);

    uint8_t *flags = saveGlobalFlags(&s->save);
    if (game == GameYendor2) {
        check("Chapter 2: the Quartz Chamber object exists", findObject(s, WorldObjectFlagStoryEvent, 1, &col, &row));
        stand(s, col, row);
        SessionInteract before = sessionInteract(s);
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, StoryEndingFlagA);
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, StoryEndingFlagB);
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, StoryEndingFlagC);
        SessionInteract after = sessionInteract(s);
        check("... it shows a message until the three quest flags are set, then the ending", before.kind == SessionInteractEndingNotReady && before.textOffset == 0x86AD && after.kind == SessionInteractEnding);
    } else {
        check("Chapter 3: event 1 exists", findObject(s, WorldObjectFlagStoryEvent, 1, &col, &row));
        stand(s, col, row);
        SessionInteract first = sessionInteract(s);
        bool arrived = first.kind == SessionInteractStoryEvent && s->x == 397 && s->y == 94 && s->facing == SaveFacingWest && first.textLines == 3 && s->pendingSound == 0x21 &&
                       globalFlagTest(flags, SaveHeaderGlobalFlagsSize, 5) && saveHeaderGetU16(&s->save, SaveHeaderTravelMode) == 2;
        stand(s, col, row);
        SessionInteract second = sessionInteract(s);
        check("... it teleports the party to Thaine and only once", arrived && second.kind == SessionInteractNothing && s->x == col);

        check("Chapter 3: event 3 exists", findObject(s, WorldObjectFlagStoryEvent, 3, &col, &row));
        stand(s, col, row);
        SessionInteract locked = sessionInteract(s);
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, 0x2E);
        stand(s, col, row);
        SessionInteract open = sessionInteract(s);
        check("... event 3 is locked until flag 0x2E, then moves the party and sets the travel flags",
              locked.kind == SessionInteractStoryRefused && locked.textOffset == 0x7F00 && open.kind == SessionInteractStoryEvent && s->x == 638 && s->y == 36 && (s->travelFlags & 0x2000) &&
                  saveHeaderGetU16(&s->save, saveHeaderPlaceFlagsOffset(GameYendor3)) == s->travelFlags);

        /* events 5 and 6: the artifact realm, and the weapons that vanish on the way back */
        findObject(s, WorldObjectFlagStoryEvent, 5, &col, &row);
        globalFlagSet(flags, SaveHeaderGlobalFlagsSize, 0x91);
        stand(s, col, row);
        SessionInteract realm = sessionInteract(s);
        check("Chapter 3: event 5 enters the artifact realm (flag 0x9D, travel flag 0x4000)",
              realm.kind == SessionInteractStoryEvent && globalFlagTest(flags, SaveHeaderGlobalFlagsSize, 0x9D) && (s->travelFlags & 0x4000));
        itemSlotSet(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, 0), PartyGroupMain), 2), 0x17F, 1);
        itemSlotSet(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, 2), PartyGroupMain), 4), 0x181, 1);
        findObject(s, WorldObjectFlagStoryEvent, 6, &col, &row);
        stand(s, col, row);
        SessionInteract back = sessionInteract(s);
        unsigned left = itemSlotId(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, 0), PartyGroupMain), 2)) +
                        itemSlotId(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, 2), PartyGroupMain), 4));
        check("... event 6 takes the light weapons away, clears the realm flag and says so",
              back.kind == SessionInteractStoryEvent && left == 0 && !globalFlagTest(flags, SaveHeaderGlobalFlagsSize, 0x9D) && back.textOffset == 0x9581 && !(s->travelFlags & 0x4000) &&
                  s->x == 332 && s->y == 70);
    }
    sessionFree(s);
}

/* Conversations through the session: every NPC can be started and left with Escape, and the services that need no screen work. */
static void testTalk(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *s = sessionNew(game, world, size);
    if (!s) {
        return;
    }
    char text[160];
    unsigned started = 0, left = 0, total = 0;
    for (unsigned id = 1; id < s->dialogs.npcCount; id++) {
        if (!dialogNpc(&s->dialogs, id) || !dialogGetU16(dialogNpc(&s->dialogs, id), DialogNpcTopicCount)) {
            continue;
        }
        total++;
        ConversationStep step;
        if (!sessionTalkStart(s, id, 0, &step)) {
            continue;
        }
        started++;
        for (unsigned n = 0; n < 60 && s->talking; n++) {
            step = sessionTalkChoose(s, 0);
            if (step.needsResolve) {
                sessionTalkResolve(s, true);
            }
        }
        left += !s->talking;
    }
    snprintf(text, sizeof(text), "%s: every NPC (%u) can be talked to and left with Escape through the session", label, total);
    check(text, total > 50 && started == total && left == total);
    check("an NPC that does not exist cannot be started", !sessionTalkStart(s, 9999, 0, NULL));

    /* the typed services, on a conversation that is open */
    ConversationStep dummy;
    sessionTalkStart(s, 1, 0, &dummy);
    ConversationStep riddle;
    memset(&riddle, 0, sizeof(riddle));
    riddle.topic = 1;
    riddle.flags = DialogTopicBuy;
    riddle.arg = 1;
    const char *answer = dialogRiddleAnswer(game, 1);
    check("a riddle topic asks for the answer", sessionTalkService(s, &riddle, NULL) == SessionServiceNeedText);
    check("... a wrong answer fails, the right one is done", sessionTalkService(s, &riddle, "NOT THE ANSWER") == SessionServiceFailed && answer && sessionTalkService(s, &riddle, answer) == SessionServiceDone);
    ConversationStep buy = riddle;
    buy.buy = true;
    buy.arg = 3; /* NUORE */
    memset(saveHeaderBcd4(&s->save, SaveHeaderGold), 0, 4);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 500);
    memset(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 0, 4);
    check("buying ore: 30 units cost 300 gold", sessionTalkService(s, &buy, "30") == SessionServiceDone && bcd4AtLeastU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 30) &&
                                                  !bcd4AtLeastU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 31) && bcd4AtLeastU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 200) &&
                                                  !bcd4AtLeastU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 201));
    check("... and more than the gold affords is refused", sessionTalkService(s, &buy, "99") == SessionServiceFailed);
    ConversationStep shop = riddle;
    shop.flags = DialogTopicUseKey;
    check("a shop topic is for the front end's screen", sessionTalkService(s, &shop, NULL) == SessionServiceNeedScreen);
    sessionFree(s);
}

/* Saving and loading: the state of the session goes into a save image and comes back in a fresh session. */
static void testSaveLoad(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *a = sessionNew(game, world, size), *b = sessionNew(game, world, size);
    if (!a || !b) {
        return;
    }
    char text[160];
    for (int i = 0; i < 12; i++) {
        sessionMove(a, i % 5 == 4 ? MovementTurnRight : MovementForward);
    }
    sessionRest(a);
    globalFlagSet(saveGlobalFlags(&a->save), SaveHeaderGlobalFlagsSize, 77);
    partySetStat(sessionPartyRecord(a, 2), PartyStatHitPoints, 3);
    a->light.timers[1] = 40;
    a->light.litFlags |= 0x300;
    a->travelFlags = 0x2000;
    a->clock.day = 9;
    static uint8_t image[SaveFileSizeMax];
    size_t bytes = sessionSave(a, image, sizeof(image));
    snprintf(text, sizeof(text), "%s: a save image has the game's file size", label);
    check(text, bytes == (game == GameYendor2 ? SaveFileSizeYendor2 : SaveFileSizeYendor3));
    check("... and loads into another session", sessionLoad(b, image, bytes));
    bool same = a->x == b->x && a->y == b->y && a->facing == b->facing && a->clock.minutes == b->clock.minutes && b->clock.day == 9 &&
                globalFlagTest(saveGlobalFlags(&b->save), SaveHeaderGlobalFlagsSize, 77) && partyGetStat(sessionPartyRecord(b, 2), PartyStatHitPoints) == 3 &&
                memcmp(a->pool, b->pool, sizeof(a->pool)) == 0;
    check("... with the position, clock, flags, party and monsters as they were", same);
    if (game == GameYendor2) {
        check("... and Chapter 2's light timers and lit flags", b->light.timers[1] == 40 && (b->light.litFlags & 0x300) == 0x300);
    } else {
        check("... and Chapter 3's travel flags", b->travelFlags == 0x2000);
    }
    check("a save of the other game is refused", !sessionLoad(b, (const uint8_t *)"short", 5));
    sessionFree(a);
    sessionFree(b);
}

static int plain(const char *formatted) { /* "19,563" -> 19563 */
    int value = 0;
    for (; *formatted; formatted++) {
        if (*formatted >= '0' && *formatted <= '9') {
            value = value * 10 + (*formatted - '0');
        }
    }
    return value;
}

/* Shops: the stock of a buy topic's lock record, prices by bartering, buying, selling back, stowing. */
static void testShop(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *s = sessionNew(game, world, size);
    if (!s) {
        return;
    }
    unsigned lockId = 0;
    SessionShopItem stock[LockContentSlots];
    for (unsigned t = 0; t < s->dialogs.topicCount && !lockId; t++) {
        const uint8_t *topic = dialogTopic(&s->dialogs, t);
        if (topic && (dialogGetU16(topic, DialogTopicFlags) & DialogTopicUseKey)) {
            unsigned id = dialogTopicU16(&s->dialogs, topic, DialogTopicArg);
            if (sessionShopStock(s, id, stock) >= 2) {
                lockId = id;
            }
        }
    }
    char text[160];
    snprintf(text, sizeof(text), "%s: a shop topic names a lock record with stock", label);
    check(text, lockId != 0);
    if (!lockId) {
        sessionFree(s);
        return;
    }
    unsigned slot = 0;
    while (slot < LockContentSlots && (!stock[slot].itemId || (stock[slot].price[0] | stock[slot].price[1] | stock[slot].price[2] | stock[slot].price[3]) == 0)) {
        slot++;
    }
    if (slot == LockContentSlots) {
        check("a stocked item has a price", false);
        sessionFree(s);
        return;
    }
    memset(saveHeaderBcd4(&s->save, SaveHeaderGold), 0, 4);
    check("with no gold nothing can be bought", sessionShopBuy(s, lockId, slot) == SessionShopNoGold && !s->heldItem);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
    char before[12], after[12], price[12];
    bcd4Format(saveHeaderBcd4(&s->save, SaveHeaderGold), before);
    check("with gold the item goes to the cursor and the gold goes down by its price", sessionShopBuy(s, lockId, slot) == SessionShopBought && s->heldItem == stock[slot].itemId);
    bcd4Format(saveHeaderBcd4(&s->save, SaveHeaderGold), after);
    bcd4Format(stock[slot].price, price);
    check("... the price is positive and the stock is still there", plain(before) - plain(after) == plain(price) && plain(price) > 0 && sessionShopStock(s, lockId, stock) >= 2);
    check("a full hand buys nothing more", sessionShopBuy(s, lockId, slot) == SessionShopHandFull);
    SessionShop sold = sessionShopSell(s, lockId);
    char back[12];
    bcd4Format(saveHeaderBcd4(&s->save, SaveHeaderGold), back);
    check("selling it back pays less than it cost", sold == SessionShopSold && !s->heldItem && plain(back) > plain(after) && plain(back) < plain(before));
    sessionShopBuy(s, lockId, slot);
    check("an item can be stowed in a member's inventory", sessionStowHeld(s, 1) && !s->heldItem);
    s->heldItem = 0xFFFF;
    check("an item the shop does not stock is not bought", sessionShopSell(s, lockId) == SessionShopNotStocked);
    sessionFree(s);
}

/* A healer NPC: the hurt speaker asks for a quote, accepts it, and ends at full health with less gold. */
static void testHealer(GameKind game, const uint8_t *world, size_t size, const char *label) {
    GameSession *s = sessionNew(game, world, size);
    if (!s) {
        return;
    }
    unsigned npcId = 0;
    for (unsigned id = 1; id < s->dialogs.npcCount && !npcId; id++) {
        const uint8_t *npc = dialogNpc(&s->dialogs, id);
        if (npc && dialogGetU16(npc, DialogNpcTopicCount) && dialogServiceForType(dialogGreetingType(&s->dialogs, npc)) == DialogServiceHealer) {
            npcId = id;
        }
    }
    char text[160];
    snprintf(text, sizeof(text), "%s: the data has a healer NPC", label);
    check(text, npcId != 0);
    if (!npcId) {
        sessionFree(s);
        return;
    }
    uint8_t *member = sessionPartyRecord(s, 1);
    partySetStat(member, PartyStatHitPoints, 1);
    memset(saveHeaderBcd4(&s->save, SaveHeaderGold), 0, 4);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
    ConversationStep step;
    check("the healer talk starts for the hurt member", sessionTalkStart(s, npcId, 1, &step));
    bool quoted = false, accepted = false;
    for (unsigned n = 0; n < 12 && s->talking && !accepted; n++) {
        unsigned pick = 0;
        for (unsigned i = 0; i < s->talk.listedCount && !pick; i++) { /* the quote first, then the accept */
            unsigned arg = dialogTopicU16(&s->dialogs, dialogTopic(&s->dialogs, s->talk.listed[i]), DialogTopicArg);
            if (!quoted && (arg & 0xF000)) {
                pick = i + 1;
            } else if (quoted && (arg & 2)) {
                pick = i + 1;
            }
        }
        if (!pick) {
            break;
        }
        step = sessionTalkChoose(s, pick);
        SessionService result = sessionTalkService(s, &step, NULL);
        if (!quoted) {
            quoted = result == SessionServiceDone;
        } else {
            accepted = result == SessionServiceDone;
        }
    }
    check("a quote fills the cost and the accept heals the member and charges the gold",
          quoted && accepted && partyGetStat(member, PartyStatHitPoints) == partyGetStatMax(member, PartyStatHitPoints) && !bcd4AtLeastU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 19998));
    sessionFree(s);
}

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: a session starts");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: a session starts");
    testCombat("YENDOR2_GAME_DIR", "../../yendor2/game");
    for (int g = 2; g <= 3; g++) {
        const char *dirI = getenv(g == 2 ? "YENDOR2_GAME_DIR" : "YENDOR3_GAME_DIR");
        char pathI[512];
        snprintf(pathI, sizeof(pathI), "%s/WORLD.DAT", dirI ? dirI : (g == 2 ? "../../yendor2/game" : "../../yendor3/game"));
        size_t sizeI;
        uint8_t *worldI = readFile(pathI, &sizeI);
        if (worldI) {
            testInteract(g == 2 ? GameYendor2 : GameYendor3, worldI, sizeI, g == 2 ? "Chapter 2" : "Chapter 3");
            testTalk(g == 2 ? GameYendor2 : GameYendor3, worldI, sizeI, g == 2 ? "Chapter 2" : "Chapter 3");
            testShop(g == 2 ? GameYendor2 : GameYendor3, worldI, sizeI, g == 2 ? "Chapter 2" : "Chapter 3");
            testHealer(g == 2 ? GameYendor2 : GameYendor3, worldI, sizeI, g == 2 ? "Chapter 2" : "Chapter 3");
            testSaveLoad(g == 2 ? GameYendor2 : GameYendor3, worldI, sizeI, g == 2 ? "Chapter 2" : "Chapter 3");
            free(worldI);
        }
    }
    for (int g = 2; g <= 3; g++) {
        const char *dir = getenv(g == 2 ? "YENDOR2_GAME_DIR" : "YENDOR3_GAME_DIR");
        char path[512];
        snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : (g == 2 ? "../../yendor2/game" : "../../yendor3/game"));
        size_t size;
        uint8_t *world = readFile(path, &size);
        if (world) {
            testSpells(g == 2 ? GameYendor2 : GameYendor3, world, size, g == 2 ? "Chapter 2" : "Chapter 3");
            free(world);
        }
    }
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
