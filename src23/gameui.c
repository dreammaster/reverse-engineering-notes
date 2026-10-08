#include "gameui.h"

#include <stdio.h>
#include <string.h>

#include "cluebook.h"
#include "cluemonster.h"
#include "font.h"
#include "gamedialog.h"
#include "lighting.h"
#include "localmap.h"
#include "maininput.h"
#include "minimap.h"
#include "music.h"
#include "paperdoll.h"
#include "pictures.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "titlemenu.h"

bool gameUiInit(GameUi *ui, GameSession *session, const uint8_t *worldDat, size_t worldSize, ViewPictureFn picture, void *pictureCtx, const GameUiHost *host) {
    memset(ui, 0, sizeof(*ui));
    if (!session || viewTablesOffset(session->game) + ViewTablesSize > worldSize) {
        return false;
    }
    ui->session = session;
    ui->worldDat = worldDat;
    ui->worldSize = worldSize;
    memcpy(ui->tables, worldDat + viewTablesOffset(session->game), ViewTablesSize);
    ui->renderer.game = session->game;
    ui->renderer.tables = ui->tables;
    ui->renderer.picture = picture;
    ui->renderer.pictureCtx = pictureCtx;
    ui->renderer.screen = ui->screen;
    ui->renderer.rng = NULL;
    if (host) {
        ui->host = *host;
    }
    ui->lastMusicPage = 0xFFFF;
    ui->castNext = 1;
    return true;
}

const uint8_t *gameUiPalette(const GameUi *ui) {
    uint32_t offset = pictureMasterPaletteOffset(ui->session->game);
    return offset + PicturePaletteSize <= ui->worldSize ? ui->worldDat + offset : NULL;
}

static void effect(GameUi *ui, unsigned id) {
    if (ui->host.effect && id) {
        ui->host.effect(ui->host.ctx, id);
    }
}

void gameUiAmbientMusic(GameUi *ui) {
    GameSession *s = ui->session;
    uint16_t track;
    if (musicRegionChanged(&ui->lastMusicPage, s->x, s->y, s->game, ui->worldDat, ui->worldSize, &track) && ui->host.music) {
        ui->host.music(ui->host.ctx, track);
    }
}

/* The NPC's text (the '%' marks a line break; ~ is the font's apostrophe), word-wrapped, and the topics that can be chosen. */
static void drawTalk(GameUi *ui) {
    GameSession *s = ui->session;
    int y = 10;
    char line[64];
    unsigned used = 0;
    for (const char *p = ui->talkText;; p++) {
        bool end = *p == 0, brk = *p == '%', space = *p == ' ';
        if (!end && !brk && !(space && used == 0)) {
            line[used++] = *p == '~' ? '\'' : *p;
        }
        bool wrap = (!end && !brk && used >= 46 && space) || used >= 52;
        if (end || brk || wrap) {
            while (used > 0 && line[used - 1] == ' ') {
                used--;
            }
            line[used] = 0;
            if (used) {
                fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8, y, line, 0x8A, 0, FontTransparent);
                y += 7;
            }
            used = 0;
        }
        if (end) {
            break;
        }
    }
    y = 120;
    for (unsigned i = 0; i < s->talk.listedCount; i++) {
        char name[DialogTopicNameSize + 1], row[32];
        dialogTopicName(dialogTopic(&s->dialogs, s->talk.listed[i]), name);
        snprintf(row, sizeof(row), "%u %s", i + 1, name);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8 + (int)(i / 5) * 150, y + (int)(i % 5) * 8, row, 0x9B, 0, FontTransparent);
    }
}

/* The stock of the shop a "buy" topic opened: number, name and price; the gold; the item on the cursor. */
static void drawShop(GameUi *ui) {
    GameSession *s = ui->session;
    SessionShopItem stock[LockContentSlots];
    sessionShopStock(s, ui->shopLock, stock);
    char line[160], gold[12], name[ItemNameBufferSize], price[12];
    bcd4Format(saveHeaderBcd4(&s->save, SaveHeaderGold), gold);
    snprintf(line, sizeof(line), "SHOP    GOLD %s", gold);
    fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8, 10, line, 0x8A, 0, FontTransparent);
    for (unsigned i = 0; i < LockContentSlots; i++) {
        if (!stock[i].itemId) {
            continue;
        }
        itemGetName(itemCatalogRecord(&s->items, stock[i].itemId), name);
        bcd4Format(stock[i].price, price);
        snprintf(line, sizeof(line), "%u %-28s %s", i + 1, name, price);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8, 30 + (int)i * 8, line, 0x9B, 0, FontTransparent);
    }
    if (s->heldItem) {
        itemGetName(itemCatalogRecord(&s->items, s->heldItem), name);
        snprintf(line, sizeof(line), "ON THE CURSOR: %s  (S SELLS IT BACK, 1-4 STOWS IT)", name);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8, 110, line, 0x8A, 0, FontTransparent);
    } else {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 8, 110, "1-8 BUY, ESCAPE LEAVES", 0x8A, 0, FontTransparent);
    }
}

void gameUiSetExe(GameUi *ui, const ExeData *exe) {
    ui->exe = exe;
}

static void drawClueMonsters(GameUi *ui);

static void drawClue(GameUi *ui) {
    GameKind game = ui->session->game;
    if (ui->clueScreen) {
        drawClueMonsters(ui);
        return;
    }
    if (ui->clueHelp) {
        ClueHelpText help;
        if (clueHelpTextLoad(&help, ui->exe, game)) {
            clueHelpDraw(&ui->renderer, &help, ClueTabFirstBit);
        }
        return;
    }
    CluePagedText text;
    if (cluePagedTextLoad(&text, ui->exe, game)) {
        cluePagedDraw(&ui->renderer, &text, cluePagedPage(game, ui->worldDat, ui->worldSize, ui->cluePage), ui->cluePage, (uint16_t)((ClueTabFirstBit >> 5) | cluePagedNavFlags(game, ui->cluePage)));
        ClueHelpText help;
        if (ui->clueNag && clueHelpTextLoad(&help, ui->exe, game)) {
            clueNagDraw(&ui->renderer, &help);
        }
    }
}

static void clueMonsterName(const GameUi *ui, unsigned typeId, char out[MonsterNameBufferSize]) {
    uint8_t record[MonsterRecordSize];
    out[0] = 0;
    if (monsterRecordSpawn(record, &ui->session->monsters, typeId)) {
        monsterGetName(record, out);
    }
}

/* The monster list: every type the catalog can spawn (clue entries are the party's to discover in the original; no record of that is decoded, so all show as known). */
static void clueMonstersOpen(GameUi *ui) {
    unsigned n = 0;
    bool seen[MonsterBlockCountMax + 1];
    memset(seen, 0, sizeof(seen));
    for (unsigned type = 1; type < ui->session->monsters.lookupCount && n < sizeof(ui->clueTypes) / sizeof(ui->clueTypes[0]); type++) {
        unsigned block = monsterCatalogBlockIndex(&ui->session->monsters, type);
        if (block && block <= MonsterBlockCountMax && !seen[block]) { /* the types that share a block are one entry */
            seen[block] = true;
            ui->clueTypes[n++] = (uint16_t)type;
        }
    }
    clueListInit(&ui->clueList, n);
    ui->clueScreen = 1;
    ui->clueNag = false;
}

static void drawClueMonsters(GameUi *ui) {
    GameKind game = ui->session->game;
    ClueMonsterText text;
    if (!clueMonsterTextLoad(&text, ui->exe, game)) {
        return;
    }
    const ViewRenderer *r = &ui->renderer;
    uint16_t tab = (uint16_t)(ClueTabFirstBit >> 1);
    if (ui->clueScreen == 2) {
        uint8_t record[MonsterRecordSize];
        char name[MonsterNameBufferSize];
        if (monsterRecordSpawn(record, &ui->session->monsters, ui->clueTypes[ui->clueList.selected])) {
            monsterGetName(record, name);
            clueMonsterPageDraw(r, &text, record, name, tab);
        }
        return;
    }
    memset(ui->screen, 0, sizeof(ui->screen));
    viewDrawPicture(r, 0, game == GameYendor2 ? 14 : 6, 1, 1, false, 0);
    fontDrawString(game, 0, ui->screen, ViewScreenWidth, clueHeadingX(game, 2, (unsigned)strlen(text.heading)), 4, text.heading, 0x0D, 0, FontTransparent);
    char names[ClueListRows][MonsterNameBufferSize];
    ClueListRow rows[ClueListRows];
    unsigned shown = 0;
    for (unsigned i = ui->clueList.first; i < ui->clueList.count && shown < ClueListRows; i++, shown++) {
        clueMonsterName(ui, ui->clueTypes[i], names[shown]);
        rows[shown].name = names[shown];
        rows[shown].known = true;
    }
    clueEntryListDraw(r, rows, shown, (int)(ui->clueList.selected - ui->clueList.first), true);
    uint16_t flags = tab;
    flags |= clueListCanPageUp(&ui->clueList) ? 0x100 : 0;
    flags |= clueListCanPageDown(&ui->clueList) ? 0x80 : 0;
    clueNavBarDraw(r, flags);
}

static bool clueKey(GameUi *ui, UiKey key) { /* Tab help, F6 the walk through, I / Q turn its pages, Escape closes */
    if (key == UiKeyF2) {
        clueMonstersOpen(ui);
        ui->clueHelp = false;
    } else if (ui->clueScreen == 2) { /* a monster's page: Escape returns to the list */
        if (key == UiKeyEscape) {
            ui->clueScreen = 1;
        }
    } else if (ui->clueScreen == 1) {
        if (key == UiKeyEscape) {
            ui->clueScreen = 0;
            ui->cluePage = 1;
        } else if (key == 'i' || key == 'q') {
            clueListPageKey(&ui->clueList, key == 'q');
        } else if (key == UiKeyUp || key == UiKeyDown) {
            clueListRowKey(&ui->clueList, key == UiKeyDown);
        } else if (key == UiKeyEnter && ui->clueList.count) {
            ui->clueScreen = 2;
        } else if (key == UiKeyF6) {
            ui->clueScreen = 0;
        }
    } else if (key == UiKeyEscape) {
        ui->overlay = UiOverlayNone;
    } else if (key == 9 /* Tab */) {
        ui->clueHelp = !ui->clueHelp;
    } else if (key == UiKeyF6) {
        ui->clueHelp = false;
    } else if ((key == 'i' || key == 'q') && !ui->clueHelp) {
        CluePagedResult result = cluePagedNavigate(ui->session->game, &ui->cluePage, (uint8_t)(key == 'i' ? 'I' : 'Q'), 0, ui->clueRegistered);
        ui->clueNag = result == CluePagedNag;
        if (result == CluePagedNag) {
            effect(ui, 3);
        }
    }
    return true;
}

void gameUiShowTitle(GameUi *ui) {
    ui->overlay = UiOverlayTitle;
}

void gameUiDraw(GameUi *ui) {
    GameSession *s = ui->session;
    if (ui->overlay == UiOverlayTitle) {
        memset(ui->screen, 0, sizeof(ui->screen));
        const uint8_t *menu = ui->renderer.picture(ui->renderer.pictureCtx, 0, TitleMenuPicture);
        for (unsigned row = 0; menu && row < 198; row++) {
            memcpy(&ui->screen[(row + 1) * ViewScreenWidth + 1], &menu[row * 318], 318);
        }
        return;
    }
    ViewScene scene;
    DungeonGridCell cells[ViewportCellCount];
    sessionScene(s, &scene, cells);
    if (ui->debugMarkers) {
        for (unsigned i = 0; i < ViewportCellCount; i++) {
            if (cells[i].flags & DungeonGridCellFlagOverlay) {
                fprintf(stderr, "view cell %u: marker type %u flags %04X monster %s\n", i, (unsigned)cells[i].reserved4, (unsigned)cells[i].flags, scene.cellMonsters[i] ? "yes" : "no");
            }
        }
    }
    memset(ui->screen, 0, sizeof(ui->screen));
    const uint8_t *frame = ui->renderer.picture(ui->renderer.pictureCtx, 0, 1);
    for (unsigned row = 0; frame && row < 198; row++) {
        memcpy(&ui->screen[(row + 1) * ViewScreenWidth + 1], &frame[row * 318], 318);
    }
    viewRender(&ui->renderer, &scene);
    MinimapTile tiles[MinimapCells];
    int16_t shades[LightingViewportCells];
    lightingViewportTable(scene.gradient, shades);
    minimapBuild(s->game, &s->grid, s->x, s->y, tiles);
    minimapDraw(&ui->renderer, tiles, shades, s->facing);
    for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
        statusPanelDraw(&ui->renderer, panel, sessionPartyRecord(s, panel));
    }
    switch (ui->overlay) {
    case UiOverlayLocalMap: {
        static LocalMapCell localCells[LocalMapColumns * LocalMapRows];
        int c0, r0;
        localMapBlockOrigin(s->x, s->y, &c0, &r0);
        localMapFill(localCells, s->game, &s->map, &s->save, c0, r0);
        memset(ui->screen, 0, sizeof(ui->screen));
        localMapDraw(&ui->renderer, localCells, s->x, s->y, s->facing);
        break;
    }
    case UiOverlayDetail: {
        uint16_t roles[5];
        for (unsigned i = 0; i < 5; i++) {
            roles[i] = saveHeaderGetU16(&s->save, SaveHeaderRoleAssignments + 2 * i);
        }
        detailSheetDraw(&ui->renderer, sessionPartyRecord(s, ui->hero), 6 + ui->hero, roles);
        break;
    }
    case UiOverlayShop:
        memset(ui->screen, 0, sizeof(ui->screen));
        drawShop(ui);
        break;
    case UiOverlayTalk:
        memset(ui->screen, 0, sizeof(ui->screen));
        drawTalk(ui);
        break;
    case UiOverlayClue:
        drawClue(ui);
        break;
    case UiOverlayDolls:
        for (unsigned i = 0; i < 4; i++) {
            paperDollDraw(&ui->renderer, &s->items, sessionPartyRecord(s, i), 8 + 56 * (int)i, 8);
        }
        break;
    case UiOverlayDialog:
        gameDialogDraw(&ui->renderer, GameDialogFlagReturn | GameDialogFlagAnimation | GameDialogFlagSave | GameDialogFlagLoad | GameDialogFlagNewGame,
                       DriverMusicAvailable | DriverSoundFxAvailable | DriverMusicOn, 5);
        break;
    default:
        break;
    }
}

void gameUiStatus(const GameUi *ui, char *out, size_t capacity) {
    const GameSession *s = ui->session;
    snprintf(out, capacity, "Yendorian Tales - (%d, %d) clock %02u:%02u%s%s%s", s->x, s->y, s->clock.minutes / 60, s->clock.minutes % 60, s->combat.active ? " - COMBAT: A attacks" : "",
             s->log[0] ? " - " : "", s->log);
}

static void talkStep(GameUi *ui, const ConversationStep *step) {
    GameSession *s = ui->session;
    if (!step->topic) {
        return;
    }
    conversationText(&s->talk, step->topic, ui->talkText, sizeof(ui->talkText));
    SessionService service = sessionTalkService(s, step, NULL);
    if (service == SessionServiceNone && step->needsResolve) {
        sessionTalkResolve(s, true);
    }
    if (service == SessionServiceNeedScreen && (step->flags & DialogTopicUseKey)) { /* a shop: the lock record named by the topic's argument */
        ui->shopLock = step->arg;
        ui->overlay = UiOverlayShop;
    }
}

/* The use command on the object ahead (sessionInteract): chests are looted, destinations travelled to, the Chapter 2 ending is played; the rest shows up in the status line. */
static void useAhead(GameUi *ui) {
    GameSession *s = ui->session;
    SessionInteract use = sessionInteract(s);
    switch (use.kind) {
    case SessionInteractDoor:
        sessionLoot(s);
        break;
    case SessionInteractTravelled:
    case SessionInteractStoryEvent:
        gameUiAmbientMusic(ui);
        break;
    case SessionInteractTravelRefused:
        snprintf(s->log, sizeof(s->log), "the way is closed (message %u)", use.messageId);
        break;
    case SessionInteractTravelPassword:
        snprintf(s->log, sizeof(s->log), "this way asks for a password (prompt %u)", use.promptId);
        break;
    case SessionInteractTalker: {
        ConversationStep first;
        if (sessionTalkStart(s, use.value, 0, &first)) {
            ui->overlay = UiOverlayTalk;
            talkStep(ui, &first);
        }
        break;
    }
    case SessionInteractStoryRefused:
    case SessionInteractEndingNotReady:
        snprintf(s->log, sizeof(s->log), "nothing happens (yet)");
        break;
    case SessionInteractEnding:
        if (ui->host.cinematic) {
            ui->host.cinematic(ui->host.ctx, "ending");
        }
        s->clock.minutes = 480;
        break;
    default:
        break;
    }
    if (s->pendingSound) {
        effect(ui, s->pendingSound);
        s->pendingSound = 0;
    }
    if (s->pendingMusic) {
        if (ui->host.music) {
            ui->host.music(ui->host.ctx, s->pendingMusic);
        }
        s->pendingMusic = 0;
    }
}

static void saveLoad(GameUi *ui, bool save) {
    GameSession *s = ui->session;
    static uint8_t image[SaveFileSizeMax];
    if (save && ui->host.store) {
        size_t bytes = sessionSave(s, image, sizeof(image));
        snprintf(s->log, sizeof(s->log), ui->host.store(ui->host.ctx, image, bytes) ? "saved %u bytes" : "could not save", (unsigned)bytes);
    } else if (!save && ui->host.fetch) {
        size_t bytes = ui->host.fetch(ui->host.ctx, image, sizeof(image));
        if (bytes) {
            snprintf(s->log, sizeof(s->log), sessionLoad(s, image, bytes) ? "loaded" : "that is not a save of this game");
            gameUiAmbientMusic(ui);
        }
    }
}

static bool shopKey(GameUi *ui, UiKey key) { /* 1-8 buy a slot, S sells the cursor item back, 1-4 stow it when the hand is full, Escape leaves */
    GameSession *s = ui->session;
    if (key == UiKeyEscape) {
        if (s->heldItem) {
            sessionStowHeld(s, 0);
        }
        ui->overlay = UiOverlayTalk;
    } else if (key == 's') {
        sessionShopSell(s, ui->shopLock);
    } else if (key >= '1' && key <= '8') {
        unsigned n = (unsigned)(key - '1');
        if (s->heldItem && n < 4) {
            sessionStowHeld(s, n);
        } else {
            sessionShopBuy(s, ui->shopLock, n);
        }
    }
    return true;
}

static bool talkKey(GameUi *ui, UiKey key) { /* 1-9 choose a topic, Escape the last one listed (the way out) */
    GameSession *s = ui->session;
    unsigned number = key == UiKeyEscape ? 0 : key >= '1' && key <= '9' ? (unsigned)(key - '1' + 1) : 99;
    if (number != 99) {
        ConversationStep step = sessionTalkChoose(s, number);
        talkStep(ui, &step);
        if (!s->talking && ui->overlay == UiOverlayTalk) {
            ui->overlay = UiOverlayNone;
        }
    }
    return true;
}

static bool commandKey(GameUi *ui, UiKey key, bool ctrl) {
    GameSession *s = ui->session;
    if (s->combat.active) {
        if (key == 'a') {
            sessionAttack(s);
        } else if (key == 'c') { /* the first attack spell the member can cast */
            for (unsigned spell = 1; spell < 126; spell++) {
                if (sessionCast(s, spell) == SessionCastDone) {
                    break;
                }
            }
        }
        return true;
    }
    if (key == 'c') { /* outside a combat: the next spell, in id order, that any member knows and can cast (support and exploration spells) */
        for (unsigned tried = 0; tried < 126; tried++, ui->castNext = ui->castNext % 125 + 1) {
            bool cast = false;
            for (unsigned member = 0; member < 4 && !cast; member++) {
                SessionCast result = sessionCastBy(s, member, ui->castNext, false);
                cast = result == SessionCastDone || result == SessionCastNoEffect;
            }
            if (cast) {
                ui->castNext = ui->castNext % 125 + 1;
                gameUiAmbientMusic(ui);
                return true;
            }
        }
        return true;
    }
    bool extended = true;
    uint8_t code = 0;
    if (key == UiKeyUp) {
        code = 0x48;
    } else if (key == UiKeyDown) {
        code = 0x50;
    } else if (key == UiKeyLeft) {
        code = ctrl ? 0x73 : 0x4B;
    } else if (key == UiKeyRight) {
        code = ctrl ? 0x74 : 0x4D;
    } else if (key >= UiKeyF1 && key <= UiKeyF8) {
        code = (uint8_t)(0x3B + (key - UiKeyF1));
    } else if (key >= 'a' && key <= 'z') {
        extended = false;
        code = (uint8_t)(key - 'a' + 'A');
    }
    MainCommand command = mainCommandForKey(s->game, extended, code);
    switch (command.action) {
    case MainActionMove:
        if (sessionMove(s, command.movement) == SessionStepBlocked && s->game == GameYendor2) {
            effect(ui, 6); /* the bump sound, _val33 */
        }
        gameUiAmbientMusic(ui);
        break;
    case MainActionRest:
        sessionRest(s);
        break;
    case MainActionUnlockDoor: /* a demo skeleton key: every tier, as chest and door key at once */
        sessionUnlock(s, 0xFFC0, 0);
        break;
    case MainActionAct:
        useAhead(ui);
        break;
    case MainActionLocalMap:
        ui->overlay = UiOverlayLocalMap;
        break;
    case MainActionClueBook:
        if (ui->exe) {
            ui->overlay = UiOverlayClue;
            ui->clueHelp = false;
            ui->clueNag = false;
            ui->clueScreen = 0;
            ui->cluePage = 1;
        }
        break;
    case MainActionGameDialog:
        ui->overlay = UiOverlayDialog;
        break;
    case MainActionMemberDetail:
        ui->hero = command.index & 3;
        ui->overlay = UiOverlayDetail;
        break;
    case MainActionPartyInventory:
        ui->overlay = UiOverlayDolls;
        break;
    default:
        break;
    }
    return true;
}

bool gameUiKey(GameUi *ui, UiKey key, bool ctrl) {
    GameSession *s = ui->session;
    bool running = true;
    if (ui->overlay == UiOverlayTitle) {
        if (key == 'e') {
            ui->overlay = UiOverlayNone;
            gameUiAmbientMusic(ui);
        } else if ((key == 'i' || key == 'r') && ui->host.cinematic) {
            ui->host.cinematic(ui->host.ctx, key == 'i' ? "opening" : "credits");
        } else if (key == UiKeyEscape || (key == 'q' && ctrl)) {
            return false;
        }
        return true;
    }
    if (ui->overlay == UiOverlayShop) {
        return shopKey(ui, key);
    }
    if (ui->overlay == UiOverlayTalk) {
        return talkKey(ui, key);
    }
    if (ui->overlay == UiOverlayClue) {
        return clueKey(ui, key);
    }
    if (key == UiKeyEscape) {
        if (ui->overlay != UiOverlayNone) {
            ui->overlay = UiOverlayNone;
            return true;
        }
        return false;
    }
    if (key == UiKeyF5 || key == UiKeyF9) {
        saveLoad(ui, key == UiKeyF5);
        return true;
    }
    if (key == '+' || key == '=') {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 30) % 1440);
        return true;
    }
    if (key == '-') {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 1440 - 30) % 1440);
        return true;
    }
    if (ui->overlay != UiOverlayNone) {
        return true;
    }
    running = commandKey(ui, key, ctrl);
    if (ui->overlay == UiOverlayNone && !s->combat.active) {
        sessionTickAilments(s); /* once per command; the original sweeps once per pass of its loop */
    }
    return running;
}
