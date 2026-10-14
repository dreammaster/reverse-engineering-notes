/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_gameui test_gameui.c ../gameui.c ../session.c ../thrown.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
 *       ../globalflags.c ../chest.c ../rest.c ../gameclock.c ../combat.c ../explore.c ../newgame.c ../party.c ../item.c ../bcd4.c ../effect.c ../random.c ../viewport.c ../lighting.c \
 *       ../dungeongrid.c ../movement.c ../worldmap.c ../savegame.c ../spellrecord.c ../spellcast.c ../consumable.c ../lightsource.c ../spelljump.c ../travel.c ../storyevent.c \
 *       ../conversation.c ../dialog.c ../dialogservice.c ../shop.c ../relics.c ../inventory.c ../maptrigger.c ../ailment.c ../document.c ../viewrender.c ../minimap.c ../statuspanel.c \
 *       ../font.c ../uiregions.c ../localmap.c ../location.c ../gamedialog.c ../maininput.c ../music.c ../paperdoll.c ../statsheet.c ../roster.c ../pictures.c ../palette.c ../cluepaged.c ../cluebook.c ../titlemenu.c ../creationwizard.c ../creation.c ../charcreate.c ../textfield.c ../clueentries.c ../cluetransport.c ../cluemonster.c ../cluespell.c ../cluemap.c ../worldmap_stdio.c ../chargen.c ../clueitem.c ../exedata.c && ./test_gameui
 *
 * Needs WORLD.DAT of yendor2/game and yendor3/game (each game skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override). The pictures are synthetic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gameui.h"
#include "gamedialog.h"
#include "uiregions.h"

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

static const uint8_t *picture(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[318 * 198];
    (void)ctx;
    memset(pixels, (int)(0x20 + category * 7 + id % 5), sizeof(pixels));
    return pixels;
}

static unsigned g_effects, g_tracks, g_stored;
static uint8_t g_image[SaveFileSizeMax];
static size_t g_imageSize;

static void onEffect(void *ctx, unsigned id) {
    (void)ctx;
    (void)id;
    g_effects++;
}

static void onMusic(void *ctx, unsigned track) {
    (void)ctx;
    (void)track;
    g_tracks++;
}

static bool onStore(void *ctx, const uint8_t *image, size_t size) {
    (void)ctx;
    memcpy(g_image, image, size);
    g_imageSize = size;
    g_stored++;
    return true;
}

static size_t onFetch(void *ctx, uint8_t *image, size_t capacity) {
    (void)ctx;
    if (!g_imageSize || g_imageSize > capacity) {
        return 0;
    }
    memcpy(image, g_image, g_imageSize);
    return g_imageSize;
}

static unsigned rosterSlotSumCheck(GameSession *s) {
    unsigned total = 0;
    for (unsigned slot = 0; slot < 4; slot++) {
        total += saveHeaderGetU16(&s->save, SaveHeaderPartySlots + 2 * slot);
    }
    return total;
}

static unsigned sum(const uint8_t *screen) {
    unsigned total = 0;
    for (unsigned i = 0; i < ViewScreenWidth * ViewScreenHeight; i++) {
        total += screen[i] * (1 + i % 7);
    }
    return total;
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
    static GameUi ui;
    g_stored = 0;
    g_imageSize = 0;
    GameUiHost host = {NULL, onEffect, onMusic, NULL, onStore, onFetch};
    char text[160];
    snprintf(text, sizeof(text), "%s: the UI starts on a session with the world's view tables and palette", label);
    check(text, s && gameUiInit(&ui, s, world, size, picture, NULL, &host) && gameUiPalette(&ui));
    gameUiShowTitle(&ui);
    gameUiDraw(&ui);
    unsigned titleSum = sum(ui.screen);
    check("the title menu draws picture 2 and swallows the game keys", titleSum > 0 && (gameUiKey(&ui, UiKeyUp, false), s->x == 166 || s->x == 460) && ui.overlay == UiOverlayTitle);
    gameUiKey(&ui, 'a', false);
    gameUiDraw(&ui);
    unsigned rosterSum = sum(ui.screen);
    check("A opens the party roster", ui.overlay == UiOverlayRoster && rosterSum > 0 && rosterSum != titleSum);
    gameUiKey(&ui, UiKeyDown, false);
    gameUiKey(&ui, UiKeyRight, false);
    gameUiKey(&ui, UiKeyRight, false);
    gameUiKey(&ui, UiKeyLeft, false); /* (1, 1): slot 5, the middle */
    check("the arrows move the cursor over the 3 x 3 grid", ui.rosterCursor == 4);
    unsigned effectsBefore = g_effects;
    for (unsigned i = 5; i < 9; i++) {
        ui.rosterCursor = i;
        gameUiKey(&ui, UiKeyEnter, false);
    }
    check("Enter takes the four heroes out of the party", rosterSlotSumCheck(s) == 0);
    gameUiKey(&ui, UiKeyEscape, false);
    gameUiKey(&ui, 'e', false);
    check("E is refused with the beep while the party is empty", ui.overlay == UiOverlayTitle && g_effects > effectsBefore);
    gameUiKey(&ui, 'a', false);
    ui.rosterCursor = 4;
    gameUiKey(&ui, UiKeyEnter, false);
    check("an empty slot cannot join", rosterSlotSumCheck(s) == 0);
    for (unsigned i = 5; i < 9; i++) {
        ui.rosterCursor = i;
        gameUiKey(&ui, UiKeyEnter, false);
    }
    check("the four heroes are back in the party, in order", saveHeaderGetU16(&s->save, SaveHeaderPartySlots) == 6 && saveHeaderGetU16(&s->save, SaveHeaderPartySlots + 6) == 9);
    gameUiKey(&ui, UiKeyEscape, false);
    gameUiKey(&ui, 'c', false);
    gameUiDraw(&ui);
    unsigned createSum = sum(ui.screen);
    check("C opens character creation on the class step", ui.overlay == UiOverlayCreate && ui.wizard.step == CreationStepClass && ui.wizardSlot == 0 && createSum != rosterSum);
    const char *script = "g3i1nann#k"; /* # = Enter */
    for (const char *k = script; *k; k++) {
        gameUiKey(&ui, *k == '#' ? UiKeyEnter : (UiKey)*k, false);
        gameUiDraw(&ui);
    }
    const uint8_t *made = saveGamePartyRecord(&s->save, 0);
    check("a hero made through the keys lands in the first empty roster slot and the title returns", ui.overlay == UiOverlayTitle && made[PartyFieldClass] == 7 && memcmp(made + PartyFieldName, "ANN", 4) == 0);
    gameUiKey(&ui, 'c', false);
    gameUiKey(&ui, 'q', false);
    check("Q abandons a creation: nothing is stored", ui.overlay == UiOverlayTitle && saveGamePartyRecord(&s->save, 1)[PartyFieldClass] == 0);

    /* the mouse: the same flows through clicks */
    gameUiClick(&ui, 100, 45, false);
    check("a click on the title's first line opens character creation", ui.overlay == UiOverlayCreate && ui.wizard.step == CreationStepClass);
    gameUiClick(&ui, 20, 190, false);
    check("a click on QUIT \"CREATE\" abandons it", ui.overlay == UiOverlayTitle);
    gameUiClick(&ui, 100, 45, false);
    gameUiClick(&ui, 20, 54, false);
    check("a click on a class picks it", ui.wizard.step == CreationStepPortrait && partyGetU16(ui.wizard.record, PartyFieldClass) == 1);
    gameUiClick(&ui, 20, 50, false);
    check("a click on a face picks the portrait", ui.wizard.step == CreationStepRoll);
    gameUiClick(&ui, 20, 72, false);
    check("PICK ITEMS goes on to the items", ui.wizard.step == CreationStepItems);
    gameUiClick(&ui, 20, 50, false);
    gameUiClick(&ui, 20, 179, false);
    check("a click takes an item and NAME CHARACTER goes on", ui.wizard.taken == 1 && ui.wizard.step == CreationStepName);
    for (const char *k = "zed#"; *k; k++) {
        gameUiKey(&ui, *k == '#' ? UiKeyEnter : (UiKey)*k, false);
    }
    gameUiClick(&ui, 20, 54, false);
    made = saveGamePartyRecord(&s->save, 1);
    check("KEEP CHARACTER keeps the hero in the next free slot", ui.overlay == UiOverlayTitle && made[PartyFieldClass] == 1 && memcmp(made + PartyFieldName, "ZED", 4) == 0);
    gameUiClick(&ui, 100, 70, false);
    check("the second title line assembles the party", ui.overlay == UiOverlayRoster);
    unsigned slotsBefore = rosterSlotSumCheck(s);
    gameUiClick(&ui, 259, 92, false);
    check("a click on a check box takes a hero out of the party", rosterSlotSumCheck(s) != slotsBefore && ui.rosterCursor == 5);
    gameUiClick(&ui, 259, 92, false);
    check("and another puts it back", rosterSlotSumCheck(s) == slotsBefore);
    gameUiClick(&ui, 240, 192, false);
    check("DONE returns to the title", ui.overlay == UiOverlayTitle);
    gameUiClick(&ui, 100, 100, false);
    check("the third line enters the game", ui.overlay == UiOverlayNone);
    uint16_t facing0 = s->facing;
    gameUiClick(&ui, 254, 169, false);
    check("a click on the walk pad's turn-left arrow turns the party", s->facing != facing0);
    gameUiClick(&ui, 299, 169, false);
    check("and turn-right turns it back", s->facing == facing0);
    gameUiClick(&ui, 303, 75, false);
    check("the icon row's last button opens the pause dialog", ui.overlay == UiOverlayDialog);
    gameUiClick(&ui, 100, 106, false);
    check("a click on the MUSIC option switches the music off", !(ui.driverFlags & DriverMusicOn));
    gameUiClick(&ui, 140, 106, false);
    check("and SOUND FX", !(ui.driverFlags & DriverSoundFxOn));
    gameUiClick(&ui, 100, 106, false);
    gameUiClick(&ui, 140, 106, false);
    check("a second click switches them back on", (ui.driverFlags & (DriverMusicOn | DriverSoundFxOn)) == (DriverMusicOn | DriverSoundFxOn));
    gameUiClick(&ui, 170, 120, false);
    check("RETURN closes the dialog", ui.overlay == UiOverlayNone);
    gameUiKey(&ui, 'd', false);
    check("D opens the pause dialog again", ui.overlay == UiOverlayDialog);
    gameUiClick(&ui, 10, 10, true);
    check("the right button backs out of it", ui.overlay == UiOverlayNone);
    gameUiClick(&ui, 270, 30, false);
    check("a click on the map panel opens the local map, any click closes it", ui.overlay == UiOverlayLocalMap && (gameUiClick(&ui, 5, 5, false), ui.overlay == UiOverlayNone));
    if (game == GameYendor2) { /* the inn's door: the opening frame, then the party is two cells on after two animation ticks */
        s->x = 170;
        s->y = 36;
        s->facing = SaveFacingEast;
        sessionRebuild(s);
        gameUiKey(&ui, UiKeyUp, false);
        bool opening = s->doorOpening && s->x == 170;
        gameUiAnimate(&ui);
        bool stillOpening = s->doorOpening;
        gameUiAnimate(&ui);
        check("a walk into a door shows it opening for two animation ticks, then passes it", opening && stillOpening && !s->doorOpening && s->x == 172);
        s->x = 166;
        s->y = 36;
        s->facing = SaveFacingWest;
        sessionRebuild(s);
    }
    gameUiClick(&ui, 20, 160, false);
    check("a click on a party portrait opens that hero's sheet", ui.overlay == UiOverlayDetail);
    gameUiClick(&ui, 180, 116, false);
    check("RETURN on the sheet closes it", ui.overlay == UiOverlayNone);
    gameUiShowTitle(&ui);
    check("E enters the game", gameUiKey(&ui, 'e', false) && ui.overlay == UiOverlayNone);
    gameUiDraw(&ui);
    unsigned first = sum(ui.screen);
    check("the game frame differs from the title", first != titleSum);
    check("the first frame draws something", first > 0);
    int x = s->x, y = s->y;
    unsigned moved = 0;
    for (unsigned i = 0; i < 20; i++) {
        gameUiKey(&ui, UiKeyUp, false);
        moved += s->x != x || s->y != y;
    }
    gameUiDraw(&ui);
    check("the Up key walks the party and the next frame differs", moved > 0 && sum(ui.screen) != first);
    unsigned before = sum(ui.screen);
    gameUiKey(&ui, 'm', false);
    gameUiDraw(&ui);
    bool mapShown = ui.overlay == UiOverlayLocalMap && sum(ui.screen) != before;
    check("M opens the local map, Escape closes it without quitting", mapShown && gameUiKey(&ui, UiKeyEscape, false) && ui.overlay == UiOverlayNone);
    /* the buttons show pressed: the walk pad's arrow while the move runs, the shoot icon while a shot flies */
    {
        unsigned padCount;
        const uint16_t(*pad)[5] = uiRegionEntries(game, UiRegionsDirectionPad, &padCount);
        gameUiAnimate(&ui);
        gameUiKey(&ui, UiKeyLeft, false);
        gameUiDraw(&ui);
        bool down = pad && padCount >= 6 && ui.pressTicks == 1 && ui.pressPicture == 18 && ui.screen[pad[0][2] * ViewScreenWidth + pad[0][0]] == 0x20 + 8 * 7 + 18 % 5;
        gameUiAnimate(&ui);
        gameUiDraw(&ui);
        check("the turn-left arrow is drawn pressed (category 8 picture 18) for a tick after the key, then comes up", down && ui.pressTicks == 0 && ui.screen[pad[0][2] * ViewScreenWidth + pad[0][0]] != 0x20 + 8 * 7 + 18 % 5);
        gameUiKey(&ui, UiKeyRight, false);
        gameUiAnimate(&ui);
        s->x = game == GameYendor2 ? 172 : s->x;
        if (game == GameYendor2) {
            s->y = 36;
            s->facing = SaveFacingEast;
            sessionRebuild(s);
        }
        gameUiKey(&ui, 's', false);
        gameUiDraw(&ui);
        bool flying = s->shot.active && ui.screen[67 * ViewScreenWidth + 239] == 0x20 + 8 * 7 + 2 % 5;
        unsigned ticks = 0;
        while (s->shot.active && ticks < 80) {
            gameUiAnimate(&ui);
            ticks++;
        }
        gameUiDraw(&ui);
        snprintf(text, sizeof(text), "%s: S shoots (Chapter 2 only): the shoot icon is pressed while the shot flies, and the shot ends", label);
        check(text, game == GameYendor3 ? !s->shot.active : (flying && !s->shot.active && ticks > 0));
        gameUiKey(&ui, ' ', false);
        check("Space uses what is ahead (the inn's wall: nothing happens; in Chapter 3 it may open a talk)", ui.overlay == UiOverlayNone || game == GameYendor3);
        for (unsigned close = 0; close < 4 && ui.overlay != UiOverlayNone; close++) {
            gameUiKey(&ui, UiKeyEscape, false);
        }
        check("so does the right button on the view", gameUiClick(&ui, 100, 100, true) && (ui.overlay == UiOverlayNone || game == GameYendor3));
        for (unsigned close = 0; close < 4 && ui.overlay != UiOverlayNone; close++) {
            gameUiKey(&ui, UiKeyEscape, false);
        }
        if (game == GameYendor2) {
            s->x = 166;
            s->y = 36;
            s->facing = SaveFacingWest;
            sessionRebuild(s);
        }
    }
    gameUiKey(&ui, UiKeyF2, false);
    gameUiDraw(&ui);
    check("F2 opens the second hero's sheet", ui.overlay == UiOverlayDetail && ui.hero == 1);
    gameUiKey(&ui, UiKeyEscape, false);
    gameUiKey(&ui, 'p', false);
    gameUiDraw(&ui);
    check("P shows the paper dolls", ui.overlay == UiOverlayDolls);
    gameUiKey(&ui, UiKeyEscape, false);
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, game == GameYendor3 ? "REGISTER.EXE" : "SW.EXE");
    size_t exeSize;
    uint8_t *exeBytes = readFile(path, &exeSize);
    static ExeData exe;
    if (exeBytes && exeDataOpen(&exe, game, exeBytes, exeSize)) {
        gameUiKey(&ui, UiKeyF8, false);
        check("F8 does nothing without the executable", ui.overlay == UiOverlayNone);
        gameUiSetExe(&ui, &exe);
        gameUiKey(&ui, UiKeyF8, false);
        gameUiDraw(&ui);
        unsigned page1 = sum(ui.screen);
        check("F8 opens the clue book on the walk through's first page", ui.overlay == UiOverlayClue && ui.cluePage == 1 && page1 > 0);
        gameUiKey(&ui, 'q', false);
        gameUiDraw(&ui);
        check("Q turns to page 2, which draws differently", ui.cluePage == 2 && sum(ui.screen) != page1);
        gameUiKey(&ui, 'i', false);
        gameUiKey(&ui, 'i', false);
        check("I does not go before page 1", ui.cluePage == 1);
        for (unsigned i = 0; i < 6; i++) {
            gameUiKey(&ui, 'q', false);
        }
        check("the unregistered walk through stops at page 6 with the reminder", ui.cluePage == CluePagedFreePages + 1 && ui.clueNag);
        gameUiDraw(&ui);
        unsigned nag = sum(ui.screen);
        ui.clueRegistered = true;
        gameUiKey(&ui, 'q', false);
        gameUiDraw(&ui);
        check("registered, it goes on and the reminder goes away", ui.cluePage == CluePagedFreePages + 2 && !ui.clueNag && sum(ui.screen) != nag);
        ui.clueRegistered = false;
        while (ui.cluePage > 1) {
            gameUiKey(&ui, 'i', false);
        }
        gameUiKey(&ui, 9, false);
        gameUiDraw(&ui);
        unsigned help = sum(ui.screen);
        check("Tab shows the help screen", ui.clueHelp && help != page1);
        gameUiKey(&ui, UiKeyF6, false);
        check("F6 goes back to the walk through", !ui.clueHelp);
        gameUiKey(&ui, UiKeyF2, false);
        gameUiDraw(&ui);
        unsigned list = sum(ui.screen);
        while (ui.clueList.selected + 1 < ui.clueList.count && ui.clueEntries[ui.clueList.selected].free) {
            gameUiKey(&ui, UiKeyDown, false);
        }
        bool locked = !ui.clueEntries[ui.clueList.selected].free;
        gameUiKey(&ui, UiKeyEnter, false);
        check("a shareware-locked entry is refused with the reminder (Chapter 3 is the registered program, nothing is locked)", !locked || (ui.clueScreen == 1 && ui.clueNag));
        ui.clueRegistered = true;
        gameUiKey(&ui, UiKeyF2, false);
        check("F2 lists the monsters", ui.clueScreen == 1 && ui.clueList.count > 14 && list != page1);
        unsigned types = ui.clueList.count;
        gameUiKey(&ui, UiKeyDown, false);
        gameUiKey(&ui, UiKeyDown, false);
        check("Down moves the selection", ui.clueList.selected == 2);
        gameUiKey(&ui, 'q', false);
        gameUiDraw(&ui);
        check("Q pages down", ui.clueList.first > 0 && ui.clueList.selected >= ui.clueList.first && sum(ui.screen) != list);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned monster = sum(ui.screen);
        check("Enter shows the monster's statistics page", ui.clueScreen == 2 && monster != list);
        unsigned frame0 = monsterGetU16(ui.clueMonster, MonsterFieldAnim);
        bool animated = false;
        for (unsigned i = 0; i < 12 && !animated; i++) {
            animated = gameUiAnimate(&ui) && monsterGetU16(ui.clueMonster, MonsterFieldAnim) != frame0;
        }
        check("the monster page animates (the platform's tick)", animated || (monsterGetU16(ui.clueMonster, MonsterFieldFlags) & (MonsterFlagAnimCycle | MonsterFlagAnimPingPong)) == 0);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape goes back to the list", ui.clueScreen == 1 && ui.clueList.count == types && ui.overlay == UiOverlayClue);
        gameUiKey(&ui, UiKeyEscape, false);
        check("and once more to the walk through", ui.clueScreen == 0 && ui.overlay == UiOverlayClue);
        gameUiKey(&ui, UiKeyF1, false);
        gameUiDraw(&ui);
        unsigned maps = sum(ui.screen);
        check("F1 lists the maps", ui.clueScreen == 1 && ui.clueCategory == 1 && ui.clueList.count > 14 && maps != list);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned mapPage = sum(ui.screen);
        check("Enter shows the map page", ui.clueScreen == 2 && ui.clueMarker == -1 && mapPage != maps);
        gameUiKey(&ui, UiKeyRight, false);
        gameUiDraw(&ui);
        check("Right picks a legend marker when the map has one", ui.clueMarker == 0 ? sum(ui.screen) != mapPage : sum(ui.screen) == mapPage);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyF3, false);
        gameUiDraw(&ui);
        unsigned spells = sum(ui.screen);
        check("F3 lists the spells", ui.clueScreen == 1 && ui.clueCategory == 3 && ui.clueList.count > 14 && spells != list);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        check("Enter shows the spell's page", ui.clueScreen == 2 && sum(ui.screen) != spells);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape twice is back at the walk through", ui.clueScreen == 0);
        gameUiKey(&ui, UiKeyF4, false);
        gameUiDraw(&ui);
        unsigned classes = sum(ui.screen);
        check("F4 offers the six magic user classes", ui.clueScreen == 1 && ui.clueCategory == 4 && ui.clueList.count == 6 && classes != spells);
        gameUiKey(&ui, UiKeyDown, false);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned count1 = ui.clueList.count;
        check("Enter lists the chosen class's spells", ui.clueScreen == 1 && ui.clueCategory == 6 && count1 > 3 && count1 < 126 && sum(ui.screen) != classes);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        check("Enter shows the spell page", ui.clueScreen == 2);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape returns to the class list on the same class", ui.clueCategory == 4 && ui.clueScreen == 1 && ui.clueList.selected == 1);
        gameUiKey(&ui, UiKeyEscape, false);
        check("and then to the walk through", ui.clueScreen == 0);
        gameUiKey(&ui, UiKeyF5, false);
        gameUiDraw(&ui);
        unsigned groups = sum(ui.screen);
        check("F5 lists the eight item groups", ui.clueScreen == 1 && ui.clueCategory == 11 && ui.clueList.count == 8 && groups != classes);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned armor = sum(ui.screen);
        check("Enter lists the group's items", ui.clueScreen == 1 && ui.clueCategory == 12 && ui.clueList.count > 14 && armor != groups);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned itemPage = sum(ui.screen);
        check("Enter shows the item page", ui.clueScreen == 2 && itemPage != armor);
        gameUiKey(&ui, UiKeyRight, false);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape returns to the group list", ui.clueCategory == 11 && ui.clueScreen == 1 && ui.clueList.selected == 0);
        for (unsigned i = 0; i < 6; i++) {
            gameUiKey(&ui, UiKeyDown, false);
        }
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        check("group 7 is the transportation page", ui.clueScreen == 2 && ui.clueCategory == 11 && sum(ui.screen) != groups);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape returns from it to the group list", ui.clueScreen == 1 && ui.clueList.selected == 6);
        gameUiKey(&ui, UiKeyF5, false);
        gameUiKey(&ui, UiKeyDown, false);
        gameUiKey(&ui, UiKeyEnter, false);
        gameUiDraw(&ui);
        unsigned legend = sum(ui.screen);
        check("group 2 shows the legend of the attribute enhancers", ui.clueScreen == 2 && ui.clueCategory == 11 && legend != groups);
        gameUiKey(&ui, UiKeyEscape, false);
        gameUiKey(&ui, UiKeyEscape, false);
        check("and then to the walk through", ui.clueScreen == 0);
        gameUiKey(&ui, 'e', false);
        check("the game keys are swallowed inside the clue book", ui.overlay == UiOverlayClue);
        check("Escape closes the clue book without quitting", gameUiKey(&ui, UiKeyEscape, false) && ui.overlay == UiOverlayNone);
    } else {
        printf("SKIP clue book (%s not found)\n", path);
        g_skipCount++;
    }
    gameUiKey(&ui, UiKeyF5, false);
    int sx = s->x;
    gameUiKey(&ui, UiKeyLeft, false);
    s->x += 3;
    gameUiKey(&ui, UiKeyF9, false);
    check("F5 hands the save image to the host and F9 loads it back", g_stored == 1 && s->x == sx);
    char status[200];
    gameUiStatus(&ui, status, sizeof(status));
    check("the status line names the position", strstr(status, "Yendorian Tales") && strstr(status, "clock"));
    check("Escape with nothing open asks to quit", !gameUiKey(&ui, UiKeyEscape, false));
    sessionFree(s);
    free(world);
}

/* The six save slots as the platform keeps them: images in memory. */
static uint8_t g_slotImage[SaveSlotCount][SaveFileSizeMax];
static size_t g_slotSize[SaveSlotCount];

static bool onSlotName(void *ctx, unsigned slot, char *name, size_t capacity) {
    (void)ctx;
    if (!g_slotSize[slot]) {
        return false;
    }
    snprintf(name, capacity, "%.24s", (const char *)g_slotImage[slot]);
    return true;
}

static bool onSlotWrite(void *ctx, unsigned slot, const uint8_t *image, size_t size) {
    (void)ctx;
    memcpy(g_slotImage[slot], image, size);
    g_slotSize[slot] = size;
    return true;
}

static size_t onSlotRead(void *ctx, unsigned slot, uint8_t *image, size_t capacity) {
    (void)ctx;
    if (!g_slotSize[slot] || g_slotSize[slot] > capacity) {
        return 0;
    }
    memcpy(image, g_slotImage[slot], g_slotSize[slot]);
    return g_slotSize[slot];
}

static void typeText(GameUi *ui, const char *text) {
    for (; *text; text++) {
        gameUiKey(ui, *text == '#' ? UiKeyEnter : *text == '<' ? UiKeyBackspace : (UiKey)*text, false);
    }
}

/* The party of the four ready-made heroes, through the roster screen. */
static void assembleParty(GameUi *ui) {
    gameUiKey(ui, 'a', false);
    for (unsigned i = 5; i < 9; i++) {
        ui->rosterCursor = i;
        gameUiKey(ui, UiKeyEnter, false);
    }
    gameUiKey(ui, UiKeyEscape, false);
}

/* The start-up (gameUiBoot): the title menu of a new game when there are no saves and the options dialog before the game; the dialog with LOAD / NEW GAME / DOS when there are; the six slots. */
static void testBoot(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        printf("SKIP %s boot (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    char text[200];
    memset(g_slotSize, 0, sizeof(g_slotSize));
    GameUiHost host = {NULL, onEffect, onMusic, NULL, onStore, onFetch, onSlotName, onSlotWrite, onSlotRead};
    static GameUi ui;
    GameSession *s = sessionNew(game, world, size);
    gameUiInit(&ui, s, world, size, picture, NULL, &host);
    int startX = s->x, startY = s->y;

    gameUiBoot(&ui); /* no saves */
    snprintf(text, sizeof(text), "%s boot: with no saves the new game is on the title menu, with an empty party", label);
    check(text, ui.overlay == UiOverlayTitle && ui.boot == UiBootTitle && rosterSlotSumCheck(s) == 0);
    unsigned beeps = g_effects;
    gameUiKey(&ui, 'e', false);
    check("E is refused until a party is assembled", ui.overlay == UiOverlayTitle && g_effects > beeps);
    check("Escape does nothing on the title menu (Ctrl+Q leaves)", gameUiKey(&ui, UiKeyEscape, false) && ui.overlay == UiOverlayTitle && !gameUiKey(&ui, 'q', true));
    bool musicBefore = (ui.driverFlags & DriverMusicOn) != 0;
    gameUiKey(&ui, UiKeyEnter, false);
    gameUiKey(&ui, 's', true);
    check("Enter on the title menu switches the music, Ctrl+S the sound effects", musicBefore && !(ui.driverFlags & DriverMusicOn) && !(ui.driverFlags & DriverSoundFxOn));
    ui.driverFlags |= DriverMusicOn | DriverSoundFxOn;
    assembleParty(&ui);
    gameUiKey(&ui, 'e', false);
    snprintf(text, sizeof(text), "%s boot: E on a first start opens the dialog with SAVE, DOS, ANIMATION and RETURN", label);
    check(text, ui.overlay == UiOverlayDialog && ui.boot == UiBootOptions && ui.dialogFlags == (GameDialogFlagSave | GameDialogFlagDos | GameDialogFlagAnimation | GameDialogFlagReturn));
    gameUiDraw(&ui);
    unsigned options = sum(ui.screen);
    gameUiKey(&ui, 'l', false);
    check("LOAD is not offered", ui.dialogMode == UiDialogMenu);
    gameUiKey(&ui, 's', false);
    gameUiDraw(&ui);
    check("S lists the six slots", ui.dialogMode == UiDialogSlots && ui.dialogOption == GameDialogSave && sum(ui.screen) != options);
    gameUiKey(&ui, '3', false);
    check("choosing a slot asks for a name", ui.dialogMode == UiDialogName && ui.dialogSlot == 2 && !ui.dialogNameKept);
    typeText(&ui, "hi#");
    check("Enter on the name asks to save", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptSave && strcmp(ui.dialogName.text, "HI") == 0);
    gameUiDraw(&ui);
    gameUiKey(&ui, 'n', false);
    check("N goes back to choosing a slot", ui.dialogMode == UiDialogSlots && !g_slotSize[2]);
    gameUiKey(&ui, '3', false);
    typeText(&ui, "yo<");
    typeText(&ui, "#");
    check("Backspace and a name again", ui.dialogMode == UiDialogPrompt && strcmp(ui.dialogName.text, "Y") == 0);
    gameUiKey(&ui, 'y', false);
    snprintf(text, sizeof(text), "%s boot: Y saves the game in the slot under that name and the game starts", label);
    check(text, g_slotSize[2] == (size_t)(game == GameYendor3 ? SaveFileSizeYendor3 : SaveFileSizeYendor2) && strcmp((const char *)g_slotImage[2], "Y") == 0 && ui.overlay == UiOverlayNone && ui.boot == UiBootNone &&
                    ui.slots[2].used && ui.slots[2].highlighted);

    /* in the game: the dialog has every option */
    s->x += 2;
    gameUiKey(&ui, 'd', false);
    check("the pause dialog offers SAVE, LOAD, NEW GAME, DOS, ANIMATION and RETURN", ui.overlay == UiOverlayDialog && ui.dialogFlags == (GameDialogFlagSave | GameDialogFlagLoad | GameDialogFlagNewGame | GameDialogFlagDos | GameDialogFlagAnimation | GameDialogFlagReturn));
    gameUiKey(&ui, 's', false);
    gameUiKey(&ui, '3', false);
    check("an occupied slot shows its name with the cursor: Enter keeps it", ui.dialogMode == UiDialogName && ui.dialogNameKept && strcmp(ui.dialogName.text, "Y") == 0);
    gameUiKey(&ui, UiKeyEnter, false);
    check("... and asks to save", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptSave);
    gameUiKey(&ui, UiKeyEscape, false);
    check("Escape answers no", ui.dialogMode == UiDialogSlots);
    gameUiKey(&ui, '3', false);
    gameUiKey(&ui, 'q', false);
    check("any other key starts a new name", !ui.dialogNameKept && strcmp(ui.dialogName.text, "Q") == 0);
    gameUiKey(&ui, UiKeyEscape, false);
    check("Escape gives the name up", ui.dialogMode == UiDialogSlots && ui.dialogSlot == -1);
    gameUiKey(&ui, UiKeyEscape, false);
    check("and Escape the slot list", ui.dialogMode == UiDialogMenu && ui.overlay == UiOverlayDialog);
    gameUiKey(&ui, 'l', false);
    unsigned beepsBefore = g_effects;
    gameUiKey(&ui, '4', false);
    check("LOAD refuses an empty slot with the beep", ui.dialogMode == UiDialogSlots && g_effects > beepsBefore);
    gameUiKey(&ui, '3', false);
    check("an occupied one asks to load", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptLoad);
    gameUiKey(&ui, 'y', false);
    snprintf(text, sizeof(text), "%s boot: Y loads the slot -- the party is back where it was saved", label);
    check(text, ui.overlay == UiOverlayNone && s->x == startX && s->y == startY);
    gameUiKey(&ui, 'd', false);
    gameUiKey(&ui, 'd', false);
    check("DOS asks first", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptDos);
    gameUiKey(&ui, 'n', false);
    check("N stays", ui.dialogMode == UiDialogMenu && ui.overlay == UiOverlayDialog);
    gameUiKey(&ui, 'd', false);
    check("Y leaves", !gameUiKey(&ui, 'y', false));
    sessionFree(s);

    /* a second start: there is a save now */
    s = sessionNew(game, world, size);
    gameUiInit(&ui, s, world, size, picture, NULL, &host);
    gameUiBoot(&ui);
    snprintf(text, sizeof(text), "%s boot: with a save the dialog opens with only LOAD, NEW GAME and DOS", label);
    check(text, ui.overlay == UiOverlayDialog && ui.boot == UiBootSaves && ui.dialogFlags == (GameDialogFlagLoad | GameDialogFlagNewGame | GameDialogFlagDos));
    gameUiDraw(&ui);
    unsigned saves = sum(ui.screen);
    gameUiKey(&ui, 'r', false);
    gameUiKey(&ui, UiKeyEscape, false);
    gameUiKey(&ui, 's', false);
    gameUiDraw(&ui);
    check("RETURN, Escape and SAVE do nothing", ui.overlay == UiOverlayDialog && ui.dialogMode == UiDialogMenu && sum(ui.screen) == saves);
    gameUiKey(&ui, 'n', false);
    check("N asks to start a new game", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptNewGame);
    gameUiDraw(&ui);
    check("... the question is in the message area", sum(ui.screen) != saves);
    gameUiKey(&ui, 'n', false);
    gameUiKey(&ui, 'l', false);
    gameUiKey(&ui, '3', false);
    gameUiKey(&ui, 'y', false);
    check("LOAD, slot 3, Y goes into the game", ui.overlay == UiOverlayNone && ui.boot == UiBootNone && s->x == startX && rosterSlotSumCheck(s) != 0);
    gameUiKey(&ui, 'd', false);
    gameUiKey(&ui, 'n', false);
    gameUiKey(&ui, 'y', false);
    snprintf(text, sizeof(text), "%s boot: NEW GAME in the game starts over on the title menu with an empty party at the start", label);
    check(text, ui.overlay == UiOverlayTitle && rosterSlotSumCheck(s) == 0 && s->x == startX && s->y == startY && ui.boot == UiBootNone);
    assembleParty(&ui);
    gameUiKey(&ui, 'e', false);
    check("E goes straight into the game", ui.overlay == UiOverlayNone);
    gameUiKey(&ui, 'd', false);
    gameUiKey(&ui, 'l', false);
    gameUiClick(&ui, 100, 53, false);
    check("a click on a slot row chooses it (the third holds a save, so it asks)", ui.dialogMode == UiDialogPrompt && ui.dialogPrompt == GameDialogPromptLoad && ui.dialogSlot == 2);
    gameUiClick(&ui, 270, 122, false);
    check("a click on YES loads it", ui.overlay == UiOverlayNone);
    gameUiKey(&ui, 'd', false);
    gameUiKey(&ui, 'l', false);
    gameUiClick(&ui, 100, 53, false);
    gameUiClick(&ui, 270, 134, false);
    check("a click on NO goes back to the slots", ui.dialogMode == UiDialogSlots);
    gameUiClick(&ui, 85, 97, false); /* the LOAD button itself gives the list up */
    check("a click on the LOAD button leaves the slot list", ui.dialogMode == UiDialogMenu);
    sessionFree(s);
    free(world);
}

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    testBoot(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testBoot(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
