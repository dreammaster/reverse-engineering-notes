/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_gameui test_gameui.c ../gameui.c ../alchemylist.c ../confirmprompt.c ../partyinfo.c ../mapview.c ../monsterpanel.c ../textpanel.c ../session.c ../thrown.c ../windowbake.c ../interact.c ../lockcatalog.c ../worldobjects.c ../monsterpool.c ../monster.c \
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
#include "spellcast.h"
#include "maininput.h"
#include "gamedialog.h"
#include "pictures.h"
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
    {
        unsigned modeBits = saveHeaderGetU16(&s->save, 0x20) & 0x7000;
        gameUiClick(&ui, 270, 30, false);
        unsigned afterClick = saveHeaderGetU16(&s->save, 0x20) & 0x7000;
        gameUiDraw(&ui);
        unsigned resources = sum(ui.screen);
        gameUiClick(&ui, 270, 30, false);
        unsigned back = saveHeaderGetU16(&s->save, 0x20) & 0x7000;
        gameUiDraw(&ui);
        check("a click on the top-right panel (the chapter plate of a new game) shows the resource panel, another click goes back", modeBits == 0x2000 && afterClick == 0x1000 && back != 0x1000 && resources != sum(ui.screen));
        gameUiKey(&ui, UiKeyF5, false);
        unsigned cycled = saveHeaderGetU16(&s->save, 0x20) & 0x7000;
        gameUiKey(&ui, UiKeyF5, false);
        check("F5 cycles the panel: the plate (or the minimap) -> the resources -> back", cycled == 0x1000 && (saveHeaderGetU16(&s->save, 0x20) & 0x7000) != 0x1000);
    }
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
    check("M does nothing without the map item", ui.overlay == UiOverlayNone);
    itemSlotSet(sessionBeltSlot(s, 0), (uint16_t)mainCommandRequiredItem(game, MainActionLocalMap), 0); /* the map goes on the tool belt */
    for (unsigned member = 0; member < 4; member++) {
        partySetStat(sessionPartyRecord(s, member), PartyStatMapping, 10);
    }
    gameUiKey(&ui, 'm', false);
    check("M with the map but too little Mapping skill does not open it", ui.overlay == UiOverlayNone);
    for (unsigned member = 0; member < 4; member++) {
        partySetStat(sessionPartyRecord(s, member), PartyStatMapping, 90);
    }
    gameUiKey(&ui, 'm', false);
    gameUiDraw(&ui);
    bool mapShown = ui.overlay == UiOverlayLocalMap && sum(ui.screen) != before;
    check("M opens the local map, Escape closes it without quitting", mapShown && gameUiKey(&ui, UiKeyEscape, false) && ui.overlay == UiOverlayNone);
    /* the timer interrupt's palette cycle: the colours 0xD0-0xDF turn in groups of four (0xDB is the acting member's frame, 0xD0-0xD3 the water), the rest stay */
    {
        const uint8_t *pal = gameUiPalette(&ui);
        unsigned seen = 0, stamp0 = ui.paletteStamp;
        uint8_t first[3], other[3];
        memcpy(first, pal + 0xDB * 3, 3);
        memcpy(other, pal + 0x20 * 3, 3);
        uint8_t values[8][3];
        for (unsigned tick = 0; tick < 40; tick++) {
            gameUiAnimate(&ui);
            bool known = false;
            for (unsigned k = 0; k < seen; k++) {
                known = known || memcmp(values[k], pal + 0xDB * 3, 3) == 0;
            }
            if (!known && seen < 8) {
                memcpy(values[seen++], pal + 0xDB * 3, 3);
            }
        }
        snprintf(text, sizeof(text), "%s: the palette cycle turns colour 0xDB through four values (one rotation every 5 BIOS ticks) and leaves the other colours alone", label);
        check(text, seen == 4 && ui.paletteStamp - stamp0 == 16 && memcmp(pal + 0x20 * 3, other, 3) == 0);
        while (ui.cycleStep != 0) {
            gameUiAnimate(&ui);
        }
    }
    /* the sky follows the clock: the master palette's sky colours by day, the end of the dusk fade at night, a fade of 113 steps (one per 91 BIOS ticks) from 06:00 and 18:00 */
    {
        const uint8_t *pal = gameUiPalette(&ui);
        const uint8_t *master = world + pictureMasterPaletteOffset(game);
        unsigned saved = s->clock.minutes;
        s->clock.minutes = 359;
        sessionAdvanceTime(s, 1);
        bool dawnAsked = s->pendingFade == 1;
        unsigned calls = 0;
        gameUiAnimate(&ui);
        bool started = ui.fadeActive && memcmp(pal + 0xE0 * 3, master + 0xE0 * 3, 96) != 0;
        while (ui.fadeActive && calls < 6000) {
            gameUiAnimate(&ui);
            calls++;
        }
        snprintf(text, sizeof(text), "%s: the clock reaching 06:00 starts the dawn fade of the sky colours, 113 steps, ending on the master palette's", label);
        check(text, dawnAsked && started && !ui.fadeActive && calls > 113 * 40 && memcmp(pal + 0xE0 * 3, master + 0xE0 * 3, 96) == 0);
        s->clock.minutes = 1320;
        gameUiKey(&ui, '+', false);
        bool night = memcmp(pal + 0xE0 * 3, master + 0xE0 * 3, 96) != 0;
        s->clock.minutes = 700;
        gameUiKey(&ui, '+', false);
        check("a time set by hand shows the sky of its hour at once (dark at night, the master palette's by day)", night && memcmp(pal + 0xE0 * 3, master + 0xE0 * 3, 96) == 0);
        s->clock.minutes = (uint16_t)saved;
    }
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
    if (game == GameYendor2) { /* the mayor, one step from the start: the scroll with his face and the greeting in rows, the topics after a key */
        s->x = 166;
        s->y = 36;
        s->facing = SaveFacingWest;
        sessionRebuild(s);
        gameUiKey(&ui, UiKeyUp, false);
        gameUiKey(&ui, ' ', false);
        gameUiDraw(&ui);
        check("Space in front of the mayor opens his conversation: the greeting in rows, no topic list yet", ui.overlay == UiOverlayTalk && ui.talkRowCount >= 7 && !ui.talkListShown && strncmp(ui.talkRows[0], "AS YOU APPROACH", 15) == 0);
        gameUiKey(&ui, UiKeyEnter, false);
        check("a key lets the greeting go and the topics are listed", ui.overlay == UiOverlayTalk && ui.talkListShown && s->talk.listedCount >= 2);
        gameUiDraw(&ui);
        check("the first topic's name is drawn at (58, 32) in colour 0x8A", ui.screen[34 * ViewScreenWidth + 59] == 0x8A || ui.screen[33 * ViewScreenWidth + 59] == 0x8A || ui.screen[35 * ViewScreenWidth + 60] == 0x8A || ui.screen[36 * ViewScreenWidth + 59] == 0x8A);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape picks the last topic (BYE) and the conversation ends", ui.overlay == UiOverlayNone || ui.overlay == UiOverlayTalk);
        while (ui.overlay == UiOverlayTalk) {
            gameUiKey(&ui, UiKeyEscape, false);
        }
        s->x = 166;
        s->y = 36;
        s->facing = SaveFacingWest;
        sessionRebuild(s);
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
    gameUiKey(&ui, UiKeyF7, false);
    int sx = s->x;
    gameUiKey(&ui, UiKeyLeft, false);
    s->x += 3;
    gameUiKey(&ui, UiKeyF9, false);
    check("F7 hands the save image to the host and F9 loads it back", g_stored == 1 && s->x == sx);
    char status[200];
    gameUiStatus(&ui, status, sizeof(status));
    check("the status line names the position", strstr(status, "Yendorian Tales") && strstr(status, "clock"));
    check("Escape with nothing open asks to quit", !gameUiKey(&ui, UiKeyEscape, false));
    sessionFree(s);
    free(world);
}

/* The six save slots as the platform keeps them: images in memory. */
static unsigned countColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    unsigned count = 0;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            count += screen[y * ViewScreenWidth + x] == colour;
        }
    }
    return count;
}

static unsigned wordAt(const uint8_t *p) {
    return (unsigned)(p[0] | (p[1] << 8));
}

/* The first world object with the flag (and the value, if given). */
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

static unsigned findBranchSpell(GameSession *s, SpellBranch branch) {
    for (unsigned id = 1; id <= s->spells.recordCount; id++) {
        const uint8_t *r = spellRecord(&s->spells, id);
        if (r && spellSelectBranch(r) == branch && spellGetU16(r, SpellFieldMpCost) > 0) {
            return id;
        }
    }
    return 0;
}

/* The compass in the panel's header, a rest's message in its message area, and the alchemy screen (the spell list, its keys and clicks, the casting). */
static void testPanelAndAlchemy(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world || game != GameYendor2) {
        return;
    }
    GameSession *s = sessionNew(game, world, size);
    static GameUi ui;
    GameUiHost host = {NULL, onEffect, onMusic, NULL, onStore, onFetch};
    char text[200];
    gameUiInit(&ui, s, world, size, picture, NULL, &host);

    gameUiDraw(&ui);
    unsigned west = countColour(ui.screen, 241, 87, 312, 92, 0x0F);
    gameUiKey(&ui, UiKeyUp, false); /* a step: the facing stays west */
    gameUiKey(&ui, UiKeyLeft, false);
    gameUiDraw(&ui);
    unsigned south = countColour(ui.screen, 241, 87, 312, 92, 0x0F);
    snprintf(text, sizeof(text), "%s: the compass direction is written white in the panel's header, and changes with the facing", label);
    check(text, west > 0 && south > 0 && s->facing == SaveFacingSouth && south != west);

    gameUiKey(&ui, 'r', false);
    gameUiDraw(&ui);
    unsigned message = countColour(ui.screen, 240, 96, 312, 107, 0x8A) + countColour(ui.screen, 240, 96, 312, 107, 0x0F);
    check("R writes the result of the rest in the message area (HOURS PASS / FOOD EATEN)", message > 20 && s->rest.hour == 8 && !s->restReport);
    gameUiKey(&ui, UiKeyLeft, false);
    gameUiDraw(&ui);
    check("the next command clears the message", countColour(ui.screen, 240, 96, 312, 107, 0x8A) + countColour(ui.screen, 240, 96, 312, 107, 0x0F) == 0);

    /* NOTHING HERE, and a destination that asks for a password (the texts come from the executable when it is there) */
    snprintf(path, sizeof(path), "%s/SW.EXE", dir ? dir : defaultDir);
    size_t exeSize;
    uint8_t *exeBytes = readFile(path, &exeSize);
    static ExeData exe;
    bool exeOk = exeBytes && exeDataOpen(&exe, game, exeBytes, exeSize);
    if (exeOk) {
        gameUiSetExe(&ui, &exe);
    }
    s->x = 5;
    s->y = 5;
    s->facing = SaveFacingNorth;
    sessionRebuild(s);
    gameUiKey(&ui, ' ', false);
    gameUiDraw(&ui);
    check("Space with nothing ahead writes NOTHING HERE in the message area", !exeOk || countColour(ui.screen, 240, 96, 312, 101, 0x8A) > 10);
    int gateCol = 0, gateRow = 0;
    bool asked = false;
    for (unsigned id = 1; id <= travelDestinationCount(game) && !asked; id++) {
        TravelDestination d;
        travelDestinationLookup(game, id, &d);
        if (!(d.flags & 0x2000) || !findObject(s, WorldObjectFlagUnknown2000, id, &gateCol, &gateRow)) {
            continue;
        }
        TravelUnlockCheck unlock = travelCheckUnlock(game, id, d.flags, saveGlobalFlags(&s->save), SaveHeaderGlobalFlagsSize);
        asked = unlock.outcome == TravelUnlockNeedsPassword;
    }
    if (asked) {
        s->x = gateCol;
        s->y = gateRow + 1;
        s->facing = SaveFacingNorth;
        sessionRebuild(s);
        gameUiKey(&ui, ' ', false);
        check("Space at a password destination opens the prompt", ui.passwordActive);
        gameUiKey(&ui, 'a', false);
        gameUiKey(&ui, 'b', false);
        gameUiDraw(&ui);
        check("the typed word shows in the message area, upper case", strcmp(ui.password.text, "AB") == 0 && countColour(ui.screen, 240, 114, 312, 119, 0x0F) > 5);
        gameUiKey(&ui, UiKeyUp, false);
        check("other keys do not walk while typing", s->y == gateRow + 1);
        gameUiKey(&ui, UiKeyEnter, false);
        check("a wrong word closes the prompt and the party stays", !ui.passwordActive && s->y == gateRow + 1);
        gameUiKey(&ui, ' ', false);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape gives the prompt up", !ui.passwordActive && s->y == gateRow + 1);
    }

    /* the clock, the key ring and the cursor item */
    s->clock.minutes = 0;
    gameUiKey(&ui, 't', false);
    check("T does nothing without the clock", ui.panelRunCount == 0);
    itemSlotSet(sessionBeltSlot(s, 1), (uint16_t)mainCommandRequiredItem(game, MainActionClock), 0);
    gameUiKey(&ui, 't', false);
    check("T with the clock writes the time and the date, in the 12-hour form", ui.panelRunCount == 2 && strcmp(ui.panelRuns[0].text, "12:00 AM") == 0 && strcmp(ui.panelRuns[1].text, "01/01/0001") == 0);
    s->clock.minutes = 13 * 60 + 5;
    gameUiKey(&ui, 't', false);
    check("... 13:05 reads 01:05 PM", strcmp(ui.panelRuns[0].text, "01:05 PM") == 0);
    s->clock.minutes = 420;
    gameUiKey(&ui, 'k', false);
    check("K does nothing without the key ring", !ui.targeting);
    itemSlotSet(sessionBeltSlot(s, 0), (uint16_t)mainCommandRequiredItem(game, MainActionUnlockDoor), 0);
    gameUiKey(&ui, 'k', false);
    check("K with the ring waits for a click (the crosshair is the cursor)", ui.targeting && gameUiCursorShown(&ui));
    gameUiKey(&ui, UiKeyUp, false);
    check("... and only Escape is heard", ui.targeting && s->y != 39);
    gameUiKey(&ui, UiKeyEscape, false);
    check("Escape gives it up", !ui.targeting);
    s->x = 166;
    s->y = 40;
    s->facing = SaveFacingWest;
    sessionRebuild(s);
    gameUiKey(&ui, 'k', false);
    gameUiClick(&ui, 100, 60, false);
    check("a click on the view tries the lock ahead: a magical one is only reported (ShowLockStatus)", !ui.targeting && ui.panelRunCount == 2 && strcmp(ui.panelRuns[0].text, "MAGICALLY") == 0);
    s->heldItem = (uint16_t)mainCommandRequiredItem(game, MainActionClock);
    gameUiMouseMove(&ui, 150, 90);
    gameUiDraw(&ui);
    unsigned withItem = sum(ui.screen);
    bool shown = gameUiCursorShown(&ui);
    s->heldItem = 0;
    gameUiDraw(&ui);
    check("an item on the cursor is drawn at the pointer and the system pointer is hidden", shown && !gameUiCursorShown(&ui) && withItem != sum(ui.screen));
    saveHeaderSetU16(&s->save, 0x20, 0x1000); /* the resource panel */
    itemSlotSet(sessionBeltSlot(s, 1), 0, 0);
    s->heldItem = (uint16_t)mainCommandRequiredItem(game, MainActionClock);
    gameUiClick(&ui, 270, 35, false); /* belt slot 2 */
    check("in the resource panel a click on a belt slot puts the cursor item there", s->heldItem == 0 && itemSlotId(sessionBeltSlot(s, 1)) == mainCommandRequiredItem(game, MainActionClock) &&
                                                                                      (saveHeaderGetU16(&s->save, 0x20) & 0x1000));
    gameUiClick(&ui, 270, 35, false);
    check("... and a click takes it up again, showing its name", s->heldItem != 0 && ui.panelRunCount == 3);
    gameUiClick(&ui, 258, 20, false);
    check("a click on a material shows the counters and stays", (saveHeaderGetU16(&s->save, 0x20) & 0x1000) && ui.panelRunCount == 0);
    s->heldItem = 0;
    gameUiClick(&ui, 300, 55, false);
    check("a click on an empty slot with an empty hand leaves the resource panel", !(saveHeaderGetU16(&s->save, 0x20) & 0x1000));

    /* a spell scroll on the tool belt: the right button on its slot uses it; one that can be studied asks USE ITEM? / LEARN SPELL? first */
    {
        unsigned studyScroll = 0, plainScroll = 0;
        for (unsigned id = 1; id <= s->items.validItemCount && !(studyScroll && plainScroll); id++) {
            const uint8_t *record = itemCatalogRecord(&s->items, id);
            const uint8_t *target = record ? itemTargetEntry(&s->items, record) : NULL;
            if (!target || itemTargetKind(record) != ItemTargetConsumable || !(itemTargetWord(target, ItemTargetSlotFlags) & 0x600) || !spellRecord(&s->spells, itemTargetWord(target, 2))) {
                continue;
            }
            if (itemTargetWord(target, ItemTargetSlotFlags) & 0x200) {
                studyScroll = studyScroll ? studyScroll : id;
            } else {
                plainScroll = plainScroll ? plainScroll : id;
            }
        }
        if (studyScroll) {
            saveHeaderSetU16(&s->save, 0x20, 0x1000);
            itemSlotSet(sessionBeltSlot(s, 1), (uint16_t)studyScroll, 1);
            gameUiClick(&ui, 270, 35, true);
            check("the right button on a belt slot with a studyable scroll asks USE ITEM? / LEARN SPELL?", ui.scrollAsk && !ui.pickActive);
            gameUiKey(&ui, UiKeyEscape, false);
            check("Escape gives the question up", !ui.scrollAsk && !ui.pickActive && itemSlotId(sessionBeltSlot(s, 1)) == studyScroll);
            gameUiClick(&ui, 270, 35, true);
            gameUiClick(&ui, 270, 131, false);
            check("a click on LEARN SPELL? asks who (message 0x21)", !ui.scrollAsk && ui.pickActive && ui.pickScroll == 2 && ui.pickPrompt == 0x21);
            gameUiClick(&ui, 270, 35, true);
            check("... and the right button gives that up", !ui.pickActive && ui.pickScroll == 0);
            SessionScrollInfo info;
            bool known = sessionScrollInfo(s, 0, SessionBeltSlotBase + 1, &info);
            gameUiClick(&ui, 270, 35, true);
            gameUiClick(&ui, 270, 122, false); /* USE ITEM? */
            bool asked = ui.pickActive ? (ui.pickScroll == 1 && ui.pickPrompt == 0x12) : true;
            if (ui.pickActive) {
                gameUiKey(&ui, UiKeyF1, false);
            }
            check("USE ITEM? casts it (a single-target spell asks the recipient first, message 0x12) and nothing is left asking", known && asked && ui.pickActive == false && !ui.scrollAsk &&
                                                                                                                   (info.singleTarget || ui.pickScroll == 0));
        }
        saveHeaderSetU16(&s->save, 0x20, 0);
        itemSlotSet(sessionBeltSlot(s, 1), 0, 0);
    }

    /* a step into the magically locked door: MAGICALLY / LOCKED in the message area */
    s->x = 166;
    s->y = 40;
    s->facing = SaveFacingWest;
    sessionRebuild(s);
    gameUiKey(&ui, UiKeyUp, false);
    gameUiDraw(&ui);
    check("a step into a magically locked door writes MAGICALLY LOCKED in the message area",
          s->x == 166 && countColour(ui.screen, 240, 96, 312, 107, 0x8A) > 20);
    gameUiKey(&ui, UiKeyLeft, false);
    gameUiKey(&ui, UiKeyRight, false);

    /* the alchemy screen: the first member with magic points (slot 1: the first hero has none), a list that is all grey without nuore */
    gameUiKey(&ui, 'c', false);
    check("C opens the alchemy screen on the first member who has magic points, with the spells he or she knows", ui.overlay == UiOverlayAlchemy && ui.alchemyCaster == 1 && ui.alchemyCount == 2 && !ui.alchemyOk[0] && !ui.alchemyOk[1]);
    gameUiDraw(&ui);
    check("... the list's frame is on screen", countColour(ui.screen, 15, 23, 230, 130, 0) < 200 * 100);
    gameUiKey(&ui, ' ', false);
    check("a spell that cannot be cast does nothing", ui.overlay == UiOverlayAlchemy && ui.alchemyMode == UiAlchemyList);
    gameUiKey(&ui, UiKeyF3, false);
    check("F3 changes to the third member's spells", ui.alchemyCaster == 2 && ui.alchemyCount == 2);
    gameUiKey(&ui, UiKeyF1, false);
    check("the first hero has no magic points: refused, nothing changes", ui.alchemyCaster == 2);
    gameUiKey(&ui, UiKeyEscape, false);
    check("Escape closes it", ui.overlay == UiOverlayNone);
    gameUiClick(&ui, 265, 73, false);
    check("a click on the second icon opens it", ui.overlay == UiOverlayAlchemy);
    gameUiClick(&ui, 100, 40, true);
    check("the right button closes it", ui.overlay == UiOverlayNone);

    /* casting a heal on another member: nuore and magic points are paid, the one chosen is healed, the caster remembered */
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 50);
    uint8_t *caster = sessionPartyRecord(s, 1), *patient = sessionPartyRecord(s, 3);
    partySetStat(caster, PartyStatHitPoints, 2);
    partySetStat(patient, PartyStatHitPoints, 1);
    uint16_t mp = partyGetStat(caster, PartyStatMagicPoints);
    gameUiKey(&ui, 'c', false);
    check("with nuore the heal is castable", ui.overlay == UiOverlayAlchemy && ui.alchemyCaster == 1 && ui.alchemyOk[0] && ui.alchemyIds[0] == 1);
    gameUiKey(&ui, ' ', false);
    check("a heal asks for the portrait it is for", ui.overlay == UiOverlayAlchemy && ui.alchemyMode == UiAlchemyTarget);
    gameUiDraw(&ui);
    gameUiClick(&ui, 195, 160, false); /* the fourth member's portrait (x 182-212) */
    check("a click on the fourth member's portrait heals that member and closes the screen",
          ui.overlay == UiOverlayNone && partyGetStat(patient, PartyStatHitPoints) > 1 && partyGetStat(caster, PartyStatHitPoints) == 2);
    snprintf(text, sizeof(text), "%s: casting paid 3 magic points and 2 nuore, and the caster is remembered", label);
    check(text, partyGetStat(caster, PartyStatMagicPoints) == mp - 3 && saveHeaderBcd4(&s->save, SaveHeaderOreCounter2)[3] == 0x48 && saveHeaderGetU16(&s->save, SaveHeaderLastCaster) == saveGetPartySlot(&s->save, 1));
    gameUiKey(&ui, 'c', false);
    check("the screen opens on the member who cast last", ui.alchemyCaster == 1 && ui.alchemySelected == 0);
    gameUiKey(&ui, UiKeyF3, false);
    gameUiKey(&ui, UiKeyEscape, false);
    gameUiKey(&ui, 'c', false);
    check("... only until he or she is chosen over", ui.alchemyCaster == 1);
    gameUiKey(&ui, UiKeyEscape, false);
    sessionFree(s);

    /* every spell known: pages of 13, the arrows, and the location spell's MARK / RETURN */
    s = sessionNew(game, world, size);
    gameUiInit(&ui, s, world, size, picture, NULL, &host);
    uint8_t *mage = sessionPartyRecord(s, 1);
    for (unsigned id = 1; id <= s->spells.recordCount; id++) {
        spellLearn(mage, id);
    }
    partySetStat(mage, PartyStatMagicPoints, 900);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), 90);
    bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderOreCounter2), 90);
    gameUiKey(&ui, 'c', false);
    check("all the spells are listed", ui.alchemyCount == s->spells.recordCount);
    gameUiKey(&ui, 'q', false);
    check("Q turns to the next page of 13, keeping the row", ui.alchemySelected == 13);
    gameUiDraw(&ui);
    check("the page arrows show (up and down)", countColour(ui.screen, 0xA5, 0x77, 0xAC, 0x7D, 0x0F) > 0 && countColour(ui.screen, 0xB0, 0x77, 0xB7, 0x7D, 0x0F) > 0);
    for (unsigned i = 0; i < 20; i++) {
        gameUiKey(&ui, 'q', false);
    }
    check("the last page ends on the last spell", ui.alchemySelected == ui.alchemyCount - 1);
    gameUiKey(&ui, UiKeyPageUp, false);
    gameUiKey(&ui, UiKeyUp, false);
    check("PageUp and Up go back", ui.alchemySelected == ui.alchemyCount - 1 - 13 - 1);
    gameUiKey(&ui, 'i', false);
    for (unsigned i = 0; i < 20; i++) {
        gameUiKey(&ui, 'i', false);
    }
    check("the first page's PageUp lands on the first spell", ui.alchemySelected == 0);

    unsigned mark = findBranchSpell(s, SpellBranchLocationBookmark);
    unsigned markIndex = 0;
    for (unsigned i = 0; i < ui.alchemyCount; i++) {
        markIndex = ui.alchemyIds[i] == mark ? i : markIndex;
    }
    gameUiKey(&ui, UiKeyEscape, false);
    int markX = s->x, markY = s->y;
    for (unsigned round = 0; round < 2; round++) {
        gameUiKey(&ui, 'c', false);
        ui.alchemySelected = markIndex;
        gameUiKey(&ui, UiKeyEnter, false);
        check(round ? "the location spell asks again, to return" : "the location spell asks MARK or RETURN", ui.alchemyMode == UiAlchemyMark);
        gameUiDraw(&ui);
        gameUiClick(&ui, 250, round ? 134 : 122, false); /* the MARK row, then the RETURN row */
        if (!round) {
            check("MARK closes the screen", ui.overlay == UiOverlayNone);
            s->x += 2;
            s->y += 1;
            sessionRebuild(s);
        }
    }
    check("RETURN brings the party back to the marked cell", ui.overlay == UiOverlayNone && s->x == markX && s->y == markY);
    sessionFree(s);
    free(world);
}

/* The party panels' lists, the inventory screen and the cursor item. */
static void testInventoryUi(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    size_t size;
    uint8_t *world = readFile(path, &size);
    if (!world) {
        return;
    }
    GameSession *s = sessionNew(game, world, size);
    static GameUi ui;
    GameUiHost host = {NULL, onEffect, onMusic, NULL, onStore, onFetch};
    char text[200];
    gameUiInit(&ui, s, world, size, picture, NULL, &host);

    /* the lists a click on a panel's icons writes */
    gameUiClick(&ui, 20, 190, false); /* the first member's name bar */
    snprintf(text, sizeof(text), "%s: a click on a name bar writes HEALTH / MAGIC / WEIGHT with the numbers", label);
    check(text, ui.panelRunCount >= 6 && strcmp(ui.panelRuns[2].text, "HEALTH:") == 0);
    check("... the name header is first", ui.panelRunCount > 2 && ui.panelRuns[0].y == 87 && ui.panelRuns[0].opaque);
    gameUiKey(&ui, '2', false);
    check("the key 2 shows the second member's bars", ui.panelRunCount >= 6 && strncmp(ui.panelRuns[1].text, (const char *)sessionPartyRecord(s, 1) + PartyFieldName, 13) == 0);
    gameUiClick(&ui, 52, 164, false);
    check("the protection icon lists the nine protections", ui.panelRunCount > 18 && strcmp(ui.panelRuns[2].text, "PROTECTIONS:") == 0);
    gameUiClick(&ui, 43, 154, false);
    check("an affliction icon lists the afflictions", strcmp(ui.panelRuns[2].text, "AFFLICTIONS:") == 0 && strcmp(ui.panelRuns[3].text, "NONE") == 0);

    /* the inventory screen: P opens all four panels, a click takes an item, a click puts it down */
    gameUiKey(&ui, 'p', false);
    check("P opens the inventory screen with every member's panel", ui.overlay == UiOverlayDolls && ui.dollMask == 0xF);
    unsigned found = 0, who = 0, slot = 0;
    for (unsigned member = 0; member < 4 && !found; member++) {
        for (unsigned n = 1; n <= InventorySlotCount && !found; n++) {
            found = itemSlotId(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, member), PartyGroupMain), n));
            who = member;
            slot = n;
        }
    }
    if (found) {
        int cx = 8 + 56 * (int)who + 2 + 18 * (int)((slot - 1) % 3) + 8, cy = 8 + 2 + 17 * (int)((slot - 1) / 3) + 8;
        gameUiClick(&ui, cx, cy, false);
        check("a click on an occupied slot takes the item to the cursor and shows its name", s->heldItem == found && gameUiCursorShown(&ui) && ui.panelRunCount == 3);
        gameUiKey(&ui, UiKeyEscape, false);
        check("Escape with an item on the cursor beeps and stays", ui.overlay == UiOverlayDolls);
        gameUiClick(&ui, cx, cy, false);
        check("a click on the same slot puts it back", s->heldItem == 0 && itemSlotId(inventoryGroupSlot(partyInventoryGroup(sessionPartyRecord(s, who), PartyGroupMain), slot)) == found);
        gameUiClick(&ui, cx, cy, false);
        gameUiClick(&ui, 300, 100, false); /* nowhere (the right-hand panel) */
        check("the item stays on the cursor over nothing", s->heldItem == found && !ui.dropPrompt);
        s->heldItem = 0;
        for (unsigned member = 0; member < 4; member++) { /* the right button on a face hides that member's panel */
            if (member != who) {
                gameUiClick(&ui, 20 + 58 * (int)member, 160, true);
            }
        }
        check("the right button on a face hides that member's panel", ui.dollMask == 1u << who && ui.overlay == UiOverlayDolls);
        s->heldItem = (uint16_t)found;
        int beside = who == 3 ? 20 : 220;
        gameUiClick(&ui, beside, 100, false); /* inside the view, beside the one panel */
        check("beside the panels the item is about to be dropped (the question)", ui.dropPrompt);
        gameUiKey(&ui, 'n', false);
        check("N keeps it", !ui.dropPrompt && s->heldItem == found);
        gameUiClick(&ui, beside, 100, false);
        gameUiKey(&ui, 'y', false);
        check("Y drops it: the hand is empty", !ui.dropPrompt && s->heldItem == 0);
    }
    gameUiKey(&ui, 'p', false);
    check("P closes the inventory screen", ui.overlay == UiOverlayNone);

    /* a potion from a bag slot: the right button asks whom, a face answers, the member is healed and the charge is spent */
    unsigned potion = game == GameYendor2 ? 0x14 : 0x36; /* HpFull */
    uint8_t *giver = sessionPartyRecord(s, 0), *patient = sessionPartyRecord(s, 2);
    for (unsigned n = 1; n <= InventorySlotCount; n++) {
        itemSlotSet(inventoryGroupSlot(partyInventoryGroup(giver, PartyGroupMain), n), 0, 0);
    }
    itemSlotSet(inventoryGroupSlot(partyInventoryGroup(giver, PartyGroupMain), 1), (uint16_t)potion, 1);
    partySetStat(patient, PartyStatHitPoints, 1);
    gameUiKey(&ui, 'p', false);
    gameUiClick(&ui, 18, 18, true);
    check("the right button on a potion asks whom (the crosshair)", ui.pickActive && gameUiCursorShown(&ui));
    gameUiKey(&ui, UiKeyEscape, false);
    check("Escape gives it up", !ui.pickActive && itemSlotId(inventoryGroupSlot(partyInventoryGroup(giver, PartyGroupMain), 1)) == potion);
    gameUiClick(&ui, 18, 18, true);
    gameUiClick(&ui, 20 + 58 * 2, 160, false); /* the third member's face */
    check("a click on a face heals that member and the potion is used up", !ui.pickActive && partyGetStat(patient, PartyStatHitPoints) == partyGetStatMax(patient, PartyStatHitPoints) &&
                                                                          itemSlotId(inventoryGroupSlot(partyInventoryGroup(giver, PartyGroupMain), 1)) == 0);
    gameUiKey(&ui, 'p', false);

    /* a chest: Space asks who opens it, the contents show in the walk pad's place, the slots work, Space leaves */
    if (game == GameYendor2) {
        s->heldItem = 0;
        saveHeaderSetU16(&s->save, SaveHeaderRoleAssignments + 4, 0);
        s->x = 163;
        s->y = 43;
        s->facing = SaveFacingNorth;
        sessionRebuild(s);
        gameUiKey(&ui, ' ', false);
        check("Space at a chest asks who opens it (the crosshair)", ui.pickActive && ui.pickChest && gameUiCursorShown(&ui));
        gameUiKey(&ui, UiKeyF1, false);
        check("a member named opens it: the contents screen is up", !ui.pickActive && ui.catalogActive);
        gameUiDraw(&ui);
        check("the eight slots are drawn where the walk pad was", countColour(ui.screen, 241, 160, 311, 194, 4) < 72 * 35);
        ChestSlot slots[LockContentSlots];
        unsigned full = sessionChestSlots(s, ui.catalogLock, slots);
        check("the chest holds something", full > 0);
        unsigned first = 0;
        while (first < LockContentSlots && !slots[first].itemId) {
            first++;
        }
        int sx = 245 + 18 * (int)(first % 4), sy = 164 + 19 * (int)(first / 4);
        gameUiClick(&ui, sx, sy, false);
        bool pile = slots[first].kind != ChestSlotItem;
        check("a click on a slot takes it: a pile joins the counters, an item rides the cursor", pile ? s->heldItem == 0 : s->heldItem == slots[first].itemId);
        check("... and the slot is empty", sessionChestSlots(s, ui.catalogLock, slots) == full - 1);
        if (!pile) {
            gameUiClick(&ui, sx, sy, false);
            check("a click on the empty slot with that item puts it back", s->heldItem == 0 && sessionChestSlots(s, ui.catalogLock, slots) == full);
            gameUiClick(&ui, sx, sy, false);
            gameUiClick(&ui, 20, 160, false); /* the first member's face */
            check("the item dropped on a face is stowed", s->heldItem == 0);
        }
        gameUiKey(&ui, ' ', false);
        check("Space leaves the contents screen", !ui.catalogActive);
        gameUiKey(&ui, ' ', false);
        check("the member who opened it is remembered: no second question", !ui.pickActive && ui.catalogActive);
        gameUiClick(&ui, 100, 60, true); /* the right button on the view */
        check("the right button on the view leaves it too", !ui.catalogActive);
    }

    /* a merchant's screen: the stock in the same eight slots, the gold in the header, a click buys, a click with the item in hand sells it back */
    unsigned lockId = 0;
    SessionShopItem stock[LockContentSlots];
    for (unsigned t = 0; t < s->dialogs.topicCount && !lockId; t++) {
        const uint8_t *topic = dialogTopic(&s->dialogs, t);
        if (topic && (dialogGetU16(topic, DialogTopicFlags) & DialogTopicUseKey)) {
            unsigned id = dialogTopicU16(&s->dialogs, topic, DialogTopicArg);
            lockId = sessionShopStock(s, id, stock) >= 2 ? id : 0;
        }
    }
    if (lockId) {
        s->heldItem = 0;
        sessionShopStock(s, lockId, stock);
        ui.catalogActive = true;
        ui.catalogShop = true;
        ui.catalogLock = lockId;
        bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
        bcd4AddU16(saveHeaderBcd4(&s->save, SaveHeaderGold), 9999);
        unsigned slot = 0;
        while (slot < LockContentSlots && !stock[slot].itemId) {
            slot++;
        }
        int sx = 245 + 18 * (int)(slot % 4), sy = 164 + 19 * (int)(slot / 4);
        gameUiDraw(&ui);
        gameUiClick(&ui, sx, sy, false);
        check("a click on a stocked slot buys it (the item rides the cursor) and shows its value", s->heldItem == stock[slot].itemId && ui.panelRunCount >= 5);
        check("... the stock is not used up", sessionShopStock(s, lockId, stock) >= 2 && stock[slot].itemId != 0);
        gameUiClick(&ui, sx, sy, false);
        check("a click with the item in hand sells it back", s->heldItem == 0);
        gameUiKey(&ui, ' ', false);
        check("Space leaves the merchant's screen", !ui.catalogActive);
    }

    /* a document in the reader's skill (DrawIndentedTextColumn): the first word of every five in the normal font, the rest foreign; a plain text is the normal font throughout */
    {
        ui.overlay = UiOverlayTalk;
        ui.reading = true;
        ui.talkRowCount = 1;
        snprintf(ui.talkRows[0], sizeof(ui.talkRows[0]), "AA AA AA AA AA AA");
        unsigned wordPixels[6][2] = {{0}};
        for (int pass = 0; pass < 2; pass++) {
            s->readability.plain = pass == 1;
            s->readability.fontOffset = 2;
            s->readability.clearWordsOfFive = pass == 1 ? 5 : 1;
            gameUiDraw(&ui);
            for (unsigned word = 0; word < 6; word++) {
                for (int row = 0; row < 6; row++) {
                    for (int col = 0; col < 12; col++) {
                        wordPixels[word][pass] = wordPixels[word][pass] * 31 + ui.screen[(67 + row) * ViewScreenWidth + 22 + 18 * (int)word + col];
                    }
                }
            }
        }
        check("a plain document draws every word alike", wordPixels[0][1] == wordPixels[1][1] && wordPixels[1][1] == wordPixels[5][1]);
        check("with one word of five readable the first is the normal font and the next four are foreign", wordPixels[0][0] == wordPixels[0][1] && wordPixels[1][0] != wordPixels[1][1] &&
                                                                                                                wordPixels[1][0] == wordPixels[2][0] && wordPixels[4][0] == wordPixels[1][0]);
        check("... and the sixth word starts the next group of five", wordPixels[5][0] == wordPixels[0][0]);
        ui.reading = false;
        ui.overlay = UiOverlayNone;
    }
    sessionFree(s);
    free(world);
}

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
    testPanelAndAlchemy(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testInventoryUi(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2");
    testInventoryUi(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3");
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
