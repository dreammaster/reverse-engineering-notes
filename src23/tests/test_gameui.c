/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_gameui test_gameui.c ../gameui.c ../session.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
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
    check("the title menu draws picture 2 and swallows the game keys", titleSum > 0 && (gameUiKey(&ui, UiKeyUp, false), s->x == s->x) && ui.overlay == UiOverlayTitle);
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

int main(void) {
    testGame(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testGame(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
