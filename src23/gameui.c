#include "gameui.h"

#include <stdio.h>
#include <string.h>

#include "cluebook.h"
#include "clueentries.h"
#include "clueitem.h"
#include "cluemap.h"
#include "cluemonster.h"
#include "cluespell.h"
#include "cluetransport.h"
#include "creationwizard.h"
#include "explore.h"
#include "font.h"
#include "gamedialog.h"
#include "lighting.h"
#include "localmap.h"
#include "maininput.h"
#include "minimap.h"
#include "monsterpanel.h"
#include "music.h"
#include "paperdoll.h"
#include "party.h"
#include "pictures.h"
#include "roster.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "textpanel.h"
#include "titlemenu.h"
#include "uiregions.h"

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
    ui->driverFlags = DriverMusicAvailable | DriverMusicOn | DriverSoundFxAvailable | DriverSoundFxOn;
    ui->animationSpeed = 5;
    return true;
}

const uint8_t *gameUiPalette(const GameUi *ui) {
    uint32_t offset = pictureMasterPaletteOffset(ui->session->game);
    return offset + PicturePaletteSize <= ui->worldSize ? ui->worldDat + offset : NULL;
}

static void effect(GameUi *ui, unsigned id) {
    if (ui->host.effect && id && (ui->driverFlags & DriverSoundFxOn)) {
        ui->host.effect(ui->host.ctx, id);
    }
}

void gameUiAmbientMusic(GameUi *ui) {
    GameSession *s = ui->session;
    uint16_t track;
    if ((ui->driverFlags & DriverMusicOn) && musicRegionChanged(&ui->lastMusicPage, s->x, s->y, s->game, ui->worldDat, ui->worldSize, &track) && ui->host.music) {
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

/*
 * The clue book's category screens, from the entry lists the executable carries (clueentries.h). A list page (the backdrop of picture 14, 14 names to a page) shows one category's entries,
 * Enter opens the selected one: a page for the maps (1), monsters (2), spells (3, 5-10) and items (12-17), the spell list of a class from the class list (4 -> 5-10), the item list of a
 * group from the group list (11 -> 12-17; group 7 is the transportation page, group 2 -- the attribute enhancers -- the legend of the six consumable kinds). Escape backs out one level. Entries without the
 * shareware flag are dim and refused with the registration reminder unless the program is registered (clueRegistered).
 */
enum { ClueNameSize = ClueEntryTextSize + 16 };

static unsigned clueItemGroupCategory(unsigned group) { /* the item group's list: 1 -> 12 ... 6 -> 16, 8 -> 17 (2 and 7 are screens of their own) */
    static const unsigned kCategory[9] = {0, 12, 0, 13, 14, 15, 16, 0, 17};
    return group < 9 ? kCategory[group] : 0;
}

static unsigned clueTab(unsigned category) { /* the navigation bar tab: maps, monsters, spells, magic users, items */
    return category == 1 ? 0 : category == 2 ? 1 : category == 3 ? 2 : category <= 10 ? 3 : 4;
}

static const ClueEntry *clueSelected(const GameUi *ui) {
    return ui->clueList.count ? &ui->clueEntries[ui->clueList.selected] : NULL;
}

static void clueEntryName(const GameUi *ui, unsigned category, unsigned id, char out[ClueNameSize]) {
    GameKind game = ui->session->game;
    out[0] = 0;
    if (category == 1) {
        LocationName name;
        if (locationBlockNamed(game, ui->worldDat, ui->worldSize, id - 1) &&
            locationName(game, ui->worldDat, ui->worldSize, id - 1, game == GameYendor2 ? " LEVEL X" : " LEVEL XXX", game == GameYendor2 ? " MAP X" : " MAP XXX", &name)) {
            snprintf(out, ClueNameSize, "%s%s", name.name, name.suffix);
        }
    } else if (category == 2) {
        uint8_t record[MonsterRecordSize];
        if (monsterRecordSpawn(record, &ui->session->monsters, id)) {
            monsterGetName(record, out);
        }
    } else if (category == 3 || (category >= 5 && category <= 10)) {
        const uint8_t *record = spellRecord(&ui->session->spells, id);
        if (record) {
            char name[SpellNameFieldSize + 1];
            spellGetName(record, name);
            snprintf(out, ClueNameSize, "%s", name);
        }
    } else if (category == 4 || category == 11) {
        clueEntryPackedString(ui->exe, game, category, id, out);
    } else if (category >= 12 && category <= 17) {
        const uint8_t *record = itemCatalogRecord(&ui->session->items, id);
        if (record) {
            char name[ItemNameBufferSize];
            itemGetName(record, name);
            snprintf(out, ClueNameSize, "%s", name);
        }
    }
}

static void clueListOpen(GameUi *ui, unsigned category) {
    ui->clueCategory = category;
    clueListInit(&ui->clueList, clueEntries(ui->exe, ui->session->game, category, ui->clueEntries));
    ui->clueScreen = 1;
    ui->clueNag = false;
    ui->clueHelp = false;
}

static void clueEntryOpen(GameUi *ui) { /* Enter on the selected entry */
    const ClueEntry *entry = clueSelected(ui);
    if (!entry) {
        return;
    }
    if (!entry->free && !ui->clueRegistered) {
        ui->clueNag = true;
        effect(ui, 3);
        return;
    }
    ui->clueNag = false;
    unsigned category = ui->clueCategory, id = entry->id;
    ui->clueSub = 0;
    ui->clueMarker = -1;
    if (category == 4) {
        ui->clueParent = ui->clueList.selected;
        clueListOpen(ui, 4 + id);
    } else if (category == 11) {
        unsigned next = clueItemGroupCategory(id);
        ui->clueParent = ui->clueList.selected;
        if (next) {
            clueListOpen(ui, next);
        } else if (id == 7 || id == 2) { /* the transportation page, the attribute enhancers legend */
            ui->clueScreen = 2;
        }
    } else {
        ui->clueScreen = 2;
        if (category == 2 && monsterRecordSpawn(ui->clueMonster, &ui->session->monsters, id)) {
            clueMonsterAnimStart(ui->clueMonster, &ui->clueAnim);
        }
    }
}

/* The sounds a session step asked for (pendingSound), played now. */
static void playPending(GameUi *ui) {
    if (ui->session->pendingSound) {
        effect(ui, ui->session->pendingSound);
        ui->session->pendingSound = 0;
    }
}

/* A button of the main screen drawn pressed (the original draws the pressed picture while the command runs: HandleMovementInput's DrawMovementFeedbackIcon, the icon row's own routines). */
static void pressButton(GameUi *ui, unsigned picture, int x, int y, unsigned ticks) {
    ui->pressPicture = picture;
    ui->pressX = x;
    ui->pressY = y;
    ui->pressTicks = ticks;
}

enum { IconRowY = 67, IconShootX = 239, IconAlchemyX = 258, IconRestX = 277, IconDialogX = 296 };

/* g_monsterDetailRevealTier (UpdatePartyAverageStatTiers, yendor2.asm:19028): the average of the record word +0x58 over the members who can act. */
static unsigned monsterRevealTier(GameSession *s) {
    unsigned sum = 0, count = 0;
    for (unsigned slot = 0; slot < 4; slot++) {
        const uint8_t *member = sessionPartyRecord(s, slot);
        if (!member) {
            break;
        }
        if (!(partyGetU16(member, PartyFieldStatusFlags) & 0x1C40)) {
            sum += partyGetU16(member, 0x58);
            count++;
        }
    }
    return count ? sum / count : 0;
}

/* DrawMonsterInfoPanels: the combat's three monsters, by name (and by what the party has revealed), in the right-hand panel; the one the next attack falls on is in 0x8A. */
static void combatPanelsDraw(GameUi *ui) {
    GameSession *s = ui->session;
    SessionCombat *c = &s->combat;
    unsigned active = CombatMonsterSlotCount;
    combatSelectActiveMonster(c->order, c->count, c->defeated, &active);
    unsigned tier = monsterRevealTier(s);
    textPanelClear(&ui->renderer, true);
    for (unsigned i = 0; i < MonsterPanelCount && i < CombatMonsterSlotCount; i++) {
        uint8_t *monster = c->slots + (size_t)i * MonsterRecordSize;
        if (monsterGetU16(monster, MonsterFieldType)) {
            monsterPanelDraw(&ui->renderer, i, monster, i == active, tier);
        }
    }
}

/* ShowLootAndAwardExperience's panel: GOLD COINS always, MAGIC ORE and NUORE when there are any, EXPERIENCE lower down. */
static void lootDraw(GameUi *ui) {
    const ViewRenderer *r = &ui->renderer;
    const SessionLoot *loot = &ui->session->loot;
    static const uint8_t zero[4] = {0, 0, 0, 0};
    char number[12];
    int y = 96;
    textPanelClear(r, false);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, "GOLD COINS:", 0x8A, 4, FontOpaque);
    bcd4Format(loot->gold, number);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, number, 0x0F, 4, FontOpaque);
    y += 18;
    if (memcmp(loot->ore, zero, 4) != 0) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, "MAGIC ORE: ", 0x8A, 4, FontOpaque);
        bcd4Format(loot->ore, number);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, number, 0x0F, 4, FontOpaque);
        y += 12;
    }
    if (memcmp(loot->nuore, zero, 4) != 0) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, "NUORE: ", 0x8A, 4, FontOpaque);
        bcd4Format(loot->nuore, number);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, number, 0x0F, 4, FontOpaque);
    }
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 144, "EXPERIENCE:", 0x7B, 4, FontOpaque);
    bcd4Format(loot->experience, number);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 150, number, 0x0F, 4, FontOpaque);
}

/* DrawPartyStatusIcon: while a member is to act in a combat (HandleDungeonInput) the 32 x 32 selection frame (category 7 picture 0) is drawn over his or her face. */
static void actingFrameDraw(GameUi *ui) {
    SessionCombat *c = &ui->session->combat;
    if (c->cursor < c->count && !c->order[c->cursor].isMonster) {
        static const int kX[4] = {8, 0x42, 0x7C, 0xB6};
        unsigned member = c->order[c->cursor].index;
        if (member < 4) {
            viewDrawPicture(&ui->renderer, 7, 0, kX[member], 0x94, true, 0);
        }
    }
}

void gameUiSettle(GameUi *ui) {
    GameSession *s = ui->session;
    if (s->doorOpening) {
        sessionDoorFinish(s);
        ui->doorTicks = 0;
        gameUiAmbientMusic(ui);
    }
    while (s->shot.active) {
        sessionShootStep(s);
        playPending(ui);
    }
    ui->pressTicks = 0;
}

bool gameUiAnimate(GameUi *ui) {
    bool changed = false;
    if (ui->pressTicks > 0) { /* a pressed button comes up */
        ui->pressTicks--;
        changed = true;
    }
    if (ui->session->shot.active) { /* the shot: one frame (or one fight) per tick */
        sessionShootStep(ui->session);
        playPending(ui);
        return true;
    }
    if (ui->session->doorOpening) { /* the frame of the door opening has been shown for a moment: pass it */
        if (ui->doorTicks > 0) {
            ui->doorTicks--;
        }
        if (ui->doorTicks == 0) {
            gameUiSettle(ui);
        }
        return true;
    }
    if (ui->overlay != UiOverlayClue || ui->clueScreen != 2 || ui->clueCategory != 2) {
        return changed;
    }
    unsigned sound = clueMonsterAnimStep(ui->clueMonster, &ui->clueAnim);
    if (sound) {
        effect(ui, sound);
    }
    return true;
}

static void clueBack(GameUi *ui) { /* Escape: one level up */
    unsigned category = ui->clueCategory;
    ui->clueNag = false;
    if (ui->clueScreen == 2) {
        ui->clueScreen = 1;
        if (category == 11) { /* the transportation page came from the group list */
            ui->clueList.selected = ui->clueParent;
        }
        return;
    }
    unsigned parent = (category >= 5 && category <= 10) ? 4 : (category >= 12 && category <= 17) ? 11 : 0;
    if (parent) {
        unsigned chosen = ui->clueParent;
        clueListOpen(ui, parent);
        if (chosen < ui->clueList.count) { /* back on the row that was chosen, on the page that holds it */
            ui->clueList.selected = chosen;
            ui->clueList.first = chosen - chosen % ClueListRows;
            ui->clueList.last = ui->clueList.first + ClueListRows - 1 < ui->clueList.count - 1 ? ui->clueList.first + ClueListRows - 1 : ui->clueList.count - 1;
        }
        return;
    }
    ui->clueScreen = 0;
    ui->cluePage = 1;
}

static void drawClueList(GameUi *ui, const char *heading) {
    GameKind game = ui->session->game;
    memset(ui->screen, 0, sizeof(ui->screen));
    viewDrawPicture(&ui->renderer, 0, game == GameYendor2 ? 14 : 7, 1, 1, false, 0);
    fontDrawString(game, 0, ui->screen, ViewScreenWidth, clueHeadingX(game, ui->clueCategory <= 3 ? ui->clueCategory : ui->clueCategory <= 10 ? 4 : 11, (unsigned)strlen(heading)), 4, heading, 0x0D, 0, FontTransparent);
    char names[ClueListRows][ClueNameSize];
    ClueListRow rows[ClueListRows];
    unsigned shown = 0;
    for (unsigned i = ui->clueList.first; i < ui->clueList.count && shown < ClueListRows; i++, shown++) {
        clueEntryName(ui, ui->clueCategory, ui->clueEntries[i].id, names[shown]);
        rows[shown].name = names[shown];
        rows[shown].known = ui->clueEntries[i].free;
    }
    clueEntryListDraw(&ui->renderer, rows, shown, (int)(ui->clueList.selected - ui->clueList.first), ui->clueRegistered);
    uint16_t flags = (uint16_t)(ClueTabFirstBit >> clueTab(ui->clueCategory));
    flags |= clueListCanPageUp(&ui->clueList) ? 0x100 : 0;
    flags |= clueListCanPageDown(&ui->clueList) ? 0x80 : 0;
    clueNavBarDraw(&ui->renderer, flags);
}

static void drawClueSpellPage(GameUi *ui, unsigned id, uint16_t tab, bool users) {
    GameKind game = ui->session->game;
    ClueSpellText text;
    if (!clueSpellTextLoad(&text, ui->exe, game)) {
        return;
    }
    ClueSpellDescription description;
    memset(&description, 0, sizeof(description));
    unsigned first, count;
    if (spellDescriptionRange(game, ui->worldDat, ui->worldSize, id, &first, &count) && count < 16) {
        for (unsigned i = 0; i < count; i++) {
            description.lines[i] = spellDescriptionLine(game, ui->worldDat, ui->worldSize, first + i);
        }
        description.lineCount = count;
    }
    const uint8_t *record = spellRecord(&ui->session->spells, id);
    if (record) {
        clueSpellPageDraw(&ui->renderer, &text, record, id, users ? text.usersHeading : text.heading, tab, &description);
    }
}

static void drawClueMapPage(GameUi *ui, unsigned id, uint16_t tab) {
    GameKind game = ui->session->game;
    static LocalMapCell cells[LocalMapColumns * LocalMapRows];
    ClueMapMarker markers[ClueMapMarkersMax];
    LocationName name;
    (void)tab;
    clueMapFill(cells, game, &ui->session->map, ui->worldDat, ui->worldSize, id);
    unsigned count = clueMapMarkers(game, ui->worldDat, ui->worldSize, id, markers, ClueMapMarkersMax);
    if (locationName(game, ui->worldDat, ui->worldSize, id - 1, game == GameYendor2 ? " LEVEL X" : " LEVEL XXX", game == GameYendor2 ? " MAP X" : " MAP XXX", &name)) {
        clueMapPageDraw(&ui->renderer, cells, &name, "SELECT LEGEND OR ESC", markers, count);
        char label[ClueMapLabelSize];
        if (ui->clueMarker >= 0 && (unsigned)ui->clueMarker < count && clueMapLabel(game, ui->worldDat, ui->worldSize, markers[ui->clueMarker].label, label)) {
            clueMapLabelDraw(&ui->renderer, label);
        }
    }
}

/* The item page: the common part, the sub-icon row of the "+N" variants (Left / Right step through them) and the row for the kind of item. */
static void drawClueItemPage(GameUi *ui, unsigned id, uint16_t tab) {
    GameKind game = ui->session->game;
    static ClueItemText text;
    if (!clueItemTextLoad(&text, ui->exe, game)) {
        return;
    }
    const ItemCatalog *catalog = &ui->session->items;
    uint16_t mask;
    unsigned count = clueSubIconCount(catalog, id, &mask);
    unsigned shown = id + (count && ui->clueSub < count ? ui->clueSub : 0);
    const uint8_t *record = itemCatalogRecord(catalog, shown);
    if (!record) {
        return;
    }
    clueItemPageDraw(&ui->renderer, &text, record, tab, ui->clueCategory);
    if (count) {
        clueSubIconRowDraw(&ui->renderer, count, (uint16_t)((mask & 0x7FFF) | (0x8000 >> (ui->clueSub < count ? ui->clueSub : 0))));
    }
    const uint8_t *entry = itemTargetEntry(catalog, record);
    ItemTargetKind kind = itemTargetKind(record);
    if (ui->clueCategory >= 13 && ui->clueCategory <= 16) { /* groups 3-6: duration, healing or the ability info */
        clueInfoRowDraw(&ui->renderer, &text, game, shown, record, entry, itemEffectEntry(catalog, record));
    } else if (entry && kind == ItemTargetWearable) {
        clueArmorRowDraw(&ui->renderer, &text, entry, itemEffectEntry(catalog, record));
    } else if (entry && kind == ItemTargetWeapon) {
        clueWeaponRowDraw(&ui->renderer, &text, entry);
    }
}

static void drawClueCategory(GameUi *ui) {
    GameKind game = ui->session->game;
    unsigned category = ui->clueCategory;
    uint16_t tab = (uint16_t)(ClueTabFirstBit >> clueTab(category));
    const ClueEntry *entry = clueSelected(ui);
    if (ui->clueScreen == 1) {
        char heading[32] = "";
        if (category == 1) {
            clueMapHeading(ui->exe, game, heading);
        } else if (category == 2) {
            ClueMonsterText text;
            if (clueMonsterTextLoad(&text, ui->exe, game)) {
                snprintf(heading, sizeof(heading), "%s", text.heading);
            }
        } else if (category <= 10) {
            ClueSpellText text;
            if (clueSpellTextLoad(&text, ui->exe, game)) {
                snprintf(heading, sizeof(heading), "%s", category == 3 ? text.heading : text.usersHeading);
            }
        } else {
            static ClueItemText items;
            if (clueItemTextLoad(&items, ui->exe, game)) {
                snprintf(heading, sizeof(heading), "%s", items.headings[category][0] ? items.headings[category] : items.headings[11]);
            }
        }
        drawClueList(ui, heading);
        if (ui->clueNag) {
            ClueHelpText help;
            if (clueHelpTextLoad(&help, ui->exe, game)) {
                clueNagDraw(&ui->renderer, &help);
            }
        }
        return;
    }
    if (category == 11) { /* group 7: the transportation page; group 2: the legend of the attribute enhancers */
        static ClueTransportData transport;
        static ClueLegendText legend;
        if (entry && entry->id == 2) {
            if (clueLegendLoad(&legend, ui->exe, game)) {
                clueLegendDraw(&ui->renderer, &legend, tab);
            }
        } else if (clueTransportLoad(&transport, ui->exe, game)) {
            clueTransportPageDraw(&ui->renderer, &transport, tab);
        }
        return;
    }
    if (!entry) {
        return;
    }
    if (category == 1) {
        drawClueMapPage(ui, entry->id, tab);
    } else if (category == 2) {
        ClueMonsterText text;
        if (clueMonsterTextLoad(&text, ui->exe, game)) {
            char name[MonsterNameBufferSize];
            monsterGetName(ui->clueMonster, name);
            clueMonsterPageDraw(&ui->renderer, &text, ui->clueMonster, name, tab);
        }
    } else if (category <= 10) {
        drawClueSpellPage(ui, entry->id, tab, category != 3);
    } else {
        drawClueItemPage(ui, entry->id, tab);
    }
}

static void drawClue(GameUi *ui) {
    GameKind game = ui->session->game;
    if (ui->clueScreen) {
        drawClueCategory(ui);
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

/* F1-F5 the category lists, F6 the walk through (I / Q turn its pages), Tab the help; in a list Up / Down / I / Q choose and Enter opens; on a map page Left / Right pick a legend
 * marker, on an item page the "+N" variant; Escape backs out and from the first level closes the book. */
static bool clueKey(GameUi *ui, UiKey key) {
    static const unsigned kFKeyCategory[5] = {1, 2, 3, 4, 11};
    if (key >= UiKeyF1 && key <= UiKeyF5) {
        clueListOpen(ui, kFKeyCategory[key - UiKeyF1]);
    } else if (ui->clueScreen == 2) {
        unsigned category = ui->clueCategory;
        if (key == UiKeyEscape) {
            clueBack(ui);
        } else if (category == 1 && (key == UiKeyLeft || key == UiKeyRight)) {
            ClueMapMarker markers[ClueMapMarkersMax];
            const ClueEntry *entry = clueSelected(ui);
            int count = entry ? (int)clueMapMarkers(ui->session->game, ui->worldDat, ui->worldSize, entry->id, markers, ClueMapMarkersMax) : 0;
            if (count) {
                ui->clueMarker = key == UiKeyRight ? (ui->clueMarker + 1) % count : (ui->clueMarker + count - 1 + (ui->clueMarker < 0 ? 1 : 0)) % count;
            }
        } else if (category >= 12 && (key == UiKeyLeft || key == UiKeyRight)) {
            const ClueEntry *entry = clueSelected(ui);
            uint16_t mask;
            unsigned count = entry ? clueSubIconCount(&ui->session->items, entry->id, &mask) : 0;
            if (count) {
                ui->clueSub = key == UiKeyRight ? (ui->clueSub + 1) % count : (ui->clueSub + count - 1) % count;
            }
        }
    } else if (ui->clueScreen == 1) {
        if (key == UiKeyEscape) {
            clueBack(ui);
        } else if (key == UiKeyF6) {
            ui->clueScreen = 0;
            ui->cluePage = 1;
        } else if (key == 'i' || key == 'q') {
            clueListPageKey(&ui->clueList, key == 'q');
        } else if (key == UiKeyUp || key == UiKeyDown) {
            clueListRowKey(&ui->clueList, key == UiKeyDown);
        } else if (key == UiKeyEnter) {
            clueEntryOpen(ui);
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

/* The roster's check boxes are the party flag (0x800 of +0x15C) of the records the four party slots name. */
static void rosterSyncFlags(GameUi *ui) {
    SaveGame *save = &ui->session->save;
    for (unsigned i = 0; i < SavePartyRecordCount; i++) {
        uint8_t *record = saveGamePartyRecord(save, i);
        bool active = false;
        for (unsigned slot = 0; slot < 4; slot++) {
            active = active || saveHeaderGetU16(save, SaveHeaderPartySlots + 2 * slot) == i + 1;
        }
        unsigned flags = record[0x15C] | (record[0x15D] << 8);
        flags = active ? flags | 0x800 : flags & ~0x800u;
        record[0x15C] = (uint8_t)flags;
        record[0x15D] = (uint8_t)(flags >> 8);
    }
}

/* Puts the hero in slot `index` into the party (the first free of the four slots) or takes it out (the others close up). False when the party is full or the slot is empty. */
static bool rosterToggle(GameUi *ui, unsigned index) {
    SaveGame *save = &ui->session->save;
    uint8_t *record = saveGamePartyRecord(save, index);
    uint16_t slots[4];
    unsigned count = 0;
    bool present = false;
    if (!record || (record[PartyFieldLevel] | (record[PartyFieldLevel + 1] << 8)) == 0) {
        return false;
    }
    for (unsigned slot = 0; slot < 4; slot++) {
        uint16_t id = saveHeaderGetU16(save, SaveHeaderPartySlots + 2 * slot);
        if (id == index + 1) {
            present = true;
        } else if (id) {
            slots[count++] = id;
        }
    }
    if (!present) {
        if (count == 4) {
            return false;
        }
        slots[count++] = (uint16_t)(index + 1);
    }
    for (unsigned slot = 0; slot < 4; slot++) {
        saveHeaderSetU16(save, SaveHeaderPartySlots + 2 * slot, slot < count ? slots[slot] : 0);
    }
    rosterSyncFlags(ui);
    sessionRebuild(ui->session);
    return true;
}

static unsigned rosterSlotSum(const GameUi *ui) { /* what RunTitleScreen tests: the sum of the four slot assignments */
    unsigned sum = 0;
    for (unsigned slot = 0; slot < 4; slot++) {
        sum += saveHeaderGetU16(&ui->session->save, SaveHeaderPartySlots + 2 * slot);
    }
    return sum;
}

/* ShowCreateCharacterPrompt's slot: the first roster record without a class (+0x0E); false when all nine are taken. */
static bool wizardStart(GameUi *ui) {
    for (unsigned i = 0; i < SavePartyRecordCount; i++) {
        const uint8_t *record = saveGamePartyRecord(&ui->session->save, i);
        if (record && (record[PartyFieldClass] | (record[PartyFieldClass + 1] << 8)) == 0) {
            ui->wizardSlot = i;
            creationWizardStart(&ui->wizard, ui->session->game, &ui->session->items, &ui->session->rng);
            return true;
        }
    }
    return false;
}

static bool wizardKey(GameUi *ui, UiKey key) {
    int ascii = key == UiKeyEscape ? 27 : (key >= 8 && key < 128 ? (int)key : 0);
    if (!ascii) {
        return true;
    }
    if (!creationWizardKey(&ui->wizard, ascii)) {
        if (ui->wizard.step == CreationStepDone) {
            memcpy(saveGamePartyRecord(&ui->session->save, ui->wizardSlot), ui->wizard.record, SavePartyRecordSize);
        }
        ui->overlay = UiOverlayTitle;
    }
    return true;
}

static void drawRoster(GameUi *ui) {
    const uint8_t *records[RosterSlots];
    for (unsigned i = 0; i < RosterSlots; i++) {
        records[i] = saveGamePartyRecord(&ui->session->save, i);
    }
    memset(ui->screen, 0, sizeof(ui->screen));
    rosterDraw(&ui->renderer, records);
    unsigned count;
    const uint16_t(*grid)[5] = uiRegionEntries(ui->session->game, UiRegionsPartyRoster, &count);
    if (grid && count >= (ui->rosterCursor + 1) * 4) { /* a frame round the chosen slot's face */
        const uint16_t(*e)[5] = grid + ui->rosterCursor * 4;
        int x0 = e[0][0] - 2, x1 = e[0][1] + 1, y0 = e[0][2] - 2, y1 = e[0][3] + 1;
        for (int x = x0; x <= x1; x++) {
            ui->screen[y0 * ViewScreenWidth + x] = ui->screen[y1 * ViewScreenWidth + x] = 0xF;
        }
        for (int y = y0; y <= y1; y++) {
            ui->screen[y * ViewScreenWidth + x0] = ui->screen[y * ViewScreenWidth + x1] = 0xF;
        }
    }
}

static bool rosterKey(GameUi *ui, UiKey key) {
    unsigned col = ui->rosterCursor % 3, row = ui->rosterCursor / 3;
    if (key == UiKeyEscape || key == 'd') { /* "D DONE" */
        ui->overlay = UiOverlayTitle;
    } else if (key == UiKeyLeft && col > 0) {
        ui->rosterCursor--;
    } else if (key == UiKeyRight && col < 2) {
        ui->rosterCursor++;
    } else if (key == UiKeyUp && row > 0) {
        ui->rosterCursor -= 3;
    } else if (key == UiKeyDown && row < 2) {
        ui->rosterCursor += 3;
    } else if (key == UiKeyEnter || key == ' ') {
        if (!rosterToggle(ui, ui->rosterCursor)) {
            effect(ui, 3);
        }
    }
    return true;
}

void gameUiShowTitle(GameUi *ui) {
    ui->overlay = UiOverlayTitle;
}

static void slotsRefresh(GameUi *ui);
static void dialogOpen(GameUi *ui, unsigned flags);

void gameUiBoot(GameUi *ui) {
    slotsRefresh(ui);
    for (unsigned i = 0; i < SaveSlotCount; i++) {
        if (ui->slots[i].used) {
            ui->boot = UiBootSaves;
            dialogOpen(ui, GameDialogFlagLoad | GameDialogFlagNewGame | GameDialogFlagDos);
            return;
        }
    }
    sessionNewGame(ui->session, ui->worldDat, ui->worldSize);
    ui->boot = UiBootTitle;
    ui->overlay = UiOverlayTitle;
}

/* The pause dialog as it is drawn in each of its modes. */
static void dialogDraw(GameUi *ui) {
    const ViewRenderer *r = &ui->renderer;
    viewDrawPicture(r, 8, 10, IconDialogX, IconRowY, false, 0); /* RunGameDialog: the dialog's icon is down while it is open */
    bool slotList = ui->dialogMode == UiDialogSlots || ui->dialogMode == UiDialogName || (ui->dialogMode == UiDialogPrompt && (ui->dialogPrompt == GameDialogPromptSave || ui->dialogPrompt == GameDialogPromptLoad));
    if (ui->dialogMode == UiDialogMenu) {
        gameDialogDraw(r, ui->dialogFlags, ui->driverFlags, ui->animationSpeed);
        return;
    }
    gameDialogStyleDraw(r, ui->dialogOption, ui->driverFlags);
    if (!slotList) {
        gameDialogPromptDraw(r, ui->dialogPrompt);
        return;
    }
    gameDialogSlotsDraw(r, ui->slots);
    if (ui->dialogSlot >= 0) {
        gameDialogSlotMark(r, (unsigned)ui->dialogSlot, true);
        if (ui->dialogOption == GameDialogSave) { /* the name being made: with the cursor while it is typed */
            char text[TextFieldMaxSize + 2];
            snprintf(text, sizeof(text), "%s%s", ui->dialogName.text, ui->dialogMode == UiDialogName ? "-" : " ");
            gameDialogSlotText(r, (unsigned)ui->dialogSlot, "                        ", false);
            gameDialogSlotText(r, (unsigned)ui->dialogSlot, text, false);
        }
    }
    if (ui->dialogMode == UiDialogPrompt) {
        gameDialogPromptDraw(r, ui->dialogPrompt);
    }
}

/* What the original has up behind the dialog while it starts: the frame of the main screen, with the party's panels when a new game has been started and not before. */
static void bootBackdrop(GameUi *ui) {
    memset(ui->screen, 0, sizeof(ui->screen));
    const uint8_t *frame = ui->renderer.picture(ui->renderer.pictureCtx, 0, 1);
    for (unsigned row = 0; frame && row < 198; row++) {
        memcpy(&ui->screen[(row + 1) * ViewScreenWidth + 1], &frame[row * 318], 318);
    }
    viewDrawPicture(&ui->renderer, 6, 0x15, 0xF8, 9, true, 0); /* RestoreAndRedrawFixedStatusIcon: the chapter plate (the minimap covers it in the game) */
    if (ui->boot == UiBootOptions) {
        for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
            statusPanelDraw(&ui->renderer, panel, sessionPartyRecord(ui->session, panel));
        }
    }
}

void gameUiDraw(GameUi *ui) {
    GameSession *s = ui->session;
    if (ui->overlay == UiOverlayRoster) {
        drawRoster(ui);
        return;
    }
    if (ui->overlay == UiOverlayCreate) {
        creationWizardDraw(&ui->wizard, &ui->renderer);
        return;
    }
    if (ui->overlay == UiOverlayDialog && (ui->boot == UiBootSaves || ui->boot == UiBootOptions)) {
        bootBackdrop(ui);
        dialogDraw(ui);
        return;
    }
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
    if (ui->overlay == UiOverlayNone) {
        if (s->combat.active) { /* DrawAndCacheStatusIcon: the swords take the shoot icon's place in a combat */
            viewDrawPicture(&ui->renderer, 8, 3, IconShootX, IconRowY, false, 0);
            combatPanelsDraw(ui);
            actingFrameDraw(ui);
        } else if (s->loot.pending) {
            lootDraw(ui);
        }
        if (s->shot.active) { /* HandleRangedOrCombatAction: the shoot icon is down while the shot flies */
            viewDrawPicture(&ui->renderer, 8, 2, IconShootX, IconRowY, false, 0);
        } else if (ui->pressTicks > 0) {
            viewDrawPicture(&ui->renderer, 8, ui->pressPicture, ui->pressX, ui->pressY, false, 0);
        }
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
        if (sessionPartyRecord(s, ui->hero)) { /* an empty slot has no sheet */
            detailSheetDraw(&ui->renderer, sessionPartyRecord(s, ui->hero), 6 + ui->hero, roles);
        }
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
            if (sessionPartyRecord(s, i)) {
                paperDollDraw(&ui->renderer, &s->items, sessionPartyRecord(s, i), 8 + 56 * (int)i, 8);
            }
        }
        break;
    case UiOverlayDialog:
        dialogDraw(ui);
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
/* A message the session names in the executable's data segment: its lines, joined, into the log. */
static void showExeMessage(GameUi *ui, unsigned offset, unsigned lines) {
    GameSession *s = ui->session;
    s->log[0] = 0;
    for (unsigned i = 0; ui->exe && i < lines; i++) {
        char line[80];
        if (!exeDataString(ui->exe, offset, line, sizeof(line))) {
            break;
        }
        snprintf(s->log + strlen(s->log), sizeof(s->log) - strlen(s->log), "%s%s", i ? " " : "", line);
        offset += (unsigned)strlen(line) + 1;
    }
}

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
    case SessionInteractDemoLimit:
        if (ui->exe && use.textOffset) {
            showExeMessage(ui, use.textOffset, use.textLines ? use.textLines : 1);
        } else {
            snprintf(s->log, sizeof(s->log), "nothing happens (yet)");
        }
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

/* ---- the pause dialog (gamedialog.h; RunGameDialog, yendor2.asm:26081) ---- */

enum { InGameDialogFlags = GameDialogFlagReturn | GameDialogFlagAnimation | GameDialogFlagDos | GameDialogFlagNewGame | GameDialogFlagLoad | GameDialogFlagSave };

/* findSavegame: which of SAVGAME1-6 exist and the name each holds. */
static void slotsRefresh(GameUi *ui) {
    for (unsigned i = 0; i < SaveSlotCount; i++) {
        SaveSlotEntry *slot = &ui->slots[i];
        slot->used = ui->host.slotName && ui->host.slotName(ui->host.ctx, i, slot->name, sizeof(slot->name));
        if (!slot->used) {
            slot->name[0] = 0;
            slot->highlighted = false;
        }
    }
}

static void dialogOpen(GameUi *ui, unsigned flags) {
    slotsRefresh(ui);
    ui->overlay = UiOverlayDialog;
    ui->dialogFlags = flags;
    ui->dialogMode = UiDialogMenu;
    ui->dialogSlot = -1;
}

/* The dialog is left for the game (RETURN, or a save or a load that has been made). */
static void dialogLeave(GameUi *ui) {
    ui->overlay = UiOverlayNone;
    ui->dialogMode = UiDialogMenu;
    if (ui->boot != UiBootNone) {
        ui->boot = UiBootNone;
        ui->lastMusicPage = 0xFFFF;
        gameUiAmbientMusic(ui);
    }
}

static void slotHighlight(GameUi *ui, unsigned slot) { /* HighlightGameDialogMenuEntry */
    for (unsigned i = 0; i < SaveSlotCount; i++) {
        ui->slots[i].highlighted = i == slot;
    }
}

/* The question's yes: SaveCurrentGameToSlot with the name chosen (copied as a string, so a shorter name leaves the tail of the old one), or the load of the chosen slot. */
static void dialogSave(GameUi *ui) {
    GameSession *s = ui->session;
    static uint8_t image[SaveFileSizeMax];
    unsigned slot = (unsigned)ui->dialogSlot;
    memcpy(s->save.bytes + SaveHeaderName, ui->dialogName.text, ui->dialogName.length + 1);
    size_t bytes = sessionSave(s, image, sizeof(image));
    if (ui->host.slotWrite && ui->host.slotWrite(ui->host.ctx, slot, image, bytes)) {
        ui->slots[slot].used = true;
        snprintf(ui->slots[slot].name, sizeof(ui->slots[slot].name), "%.24s", ui->dialogName.text);
        slotHighlight(ui, slot);
        snprintf(s->log, sizeof(s->log), "saved in slot %u", slot + 1);
        dialogLeave(ui);
    } else {
        snprintf(s->log, sizeof(s->log), "could not save");
        ui->dialogMode = UiDialogSlots;
        ui->dialogSlot = -1;
    }
}

static void dialogLoad(GameUi *ui) {
    GameSession *s = ui->session;
    static uint8_t image[SaveFileSizeMax];
    size_t bytes = ui->host.slotRead ? ui->host.slotRead(ui->host.ctx, (unsigned)ui->dialogSlot, image, sizeof(image)) : 0;
    if (bytes && sessionLoad(s, image, bytes)) {
        ExploreReveal revealed; /* InitializeDungeonLevel: RevealCellsAroundPlayer */
        exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
        sessionRebuild(s);
        slotHighlight(ui, (unsigned)ui->dialogSlot);
        s->log[0] = 0;
        dialogLeave(ui);
    } else {
        snprintf(s->log, sizeof(s->log), "that is not a save of this game");
        ui->dialogMode = UiDialogSlots;
        ui->dialogSlot = -1;
    }
}

/* ConfirmNewGame's yes: RunTitleScreen starts a new game (InitializeNewGameWorldState) and the title menu is up; its E goes on into the game. */
static void startNewGame(GameUi *ui) {
    if (sessionNewGame(ui->session, ui->worldDat, ui->worldSize)) {
        ui->dialogMode = UiDialogMenu;
        ui->overlay = UiOverlayTitle;
    } else {
        ui->dialogMode = UiDialogMenu;
    }
}

static bool dialogPromptAnswer(GameUi *ui, bool yes) {
    switch (ui->dialogPrompt) {
    case GameDialogPromptDos:
        if (yes) {
            return false;
        }
        break;
    case GameDialogPromptNewGame:
        if (yes) {
            startNewGame(ui);
            return true;
        }
        break;
    case GameDialogPromptSave:
        if (yes) {
            dialogSave(ui);
        } else { /* back to choosing a slot */
            ui->dialogMode = UiDialogSlots;
            ui->dialogSlot = -1;
        }
        return true;
    case GameDialogPromptLoad:
        if (yes) {
            dialogLoad(ui);
        } else {
            ui->dialogMode = UiDialogSlots;
            ui->dialogSlot = -1;
        }
        return true;
    }
    ui->dialogMode = UiDialogMenu;
    return true;
}

static void dialogAsk(GameUi *ui, GameDialogPrompt prompt) {
    ui->dialogMode = UiDialogPrompt;
    ui->dialogPrompt = prompt;
}

/* SelectGameDialogOption's pick of a row of the slot list. */
static void dialogChooseSlot(GameUi *ui, unsigned slot) {
    if (slot >= SaveSlotCount) {
        return;
    }
    if (ui->dialogOption == GameDialogLoad) {
        if (!ui->slots[slot].used) {
            effect(ui, 3); /* nothing to load */
            return;
        }
        effect(ui, 4);
        ui->dialogSlot = (int)slot;
        dialogAsk(ui, GameDialogPromptLoad);
        return;
    }
    effect(ui, 4);
    ui->dialogSlot = (int)slot;
    ui->dialogMode = UiDialogName;
    textFieldInit(&ui->dialogName, 24);
    ui->dialogNameKept = ui->slots[slot].used;
    if (ui->dialogNameKept) { /* the slot's name with the cursor after it */
        snprintf(ui->dialogName.text, sizeof(ui->dialogName.text), "%s", ui->slots[slot].name);
        ui->dialogName.length = (unsigned)strlen(ui->dialogName.text);
    }
}

static void dialogNameKey(GameUi *ui, UiKey key) {
    if (key == UiKeyEscape) {
        ui->dialogMode = UiDialogSlots;
        ui->dialogSlot = -1;
        return;
    }
    if (ui->dialogNameKept) {
        if (key == UiKeyEnter) {
            dialogAsk(ui, GameDialogPromptSave);
            return;
        }
        textFieldInit(&ui->dialogName, 24); /* the first key replaces the old name */
        ui->dialogNameKept = false;
    }
    int ascii = key >= 8 && key < 128 ? (int)key : 0;
    if (ascii >= 'a' && ascii <= 'z') {
        ascii -= 32; /* the keyboard poll gives upper case */
    }
    switch (ascii ? textFieldKey(&ui->dialogName, (uint8_t)ascii) : TextFieldEditing) {
    case TextFieldConfirmed:
        dialogAsk(ui, GameDialogPromptSave);
        break;
    case TextFieldCancelled:
        ui->dialogMode = UiDialogSlots;
        ui->dialogSlot = -1;
        break;
    case TextFieldBeep:
        effect(ui, 3);
        break;
    default:
        break;
    }
}

/* The pause dialog (gamedialog.h): S / L list the six save slots (1-6 or a click choose one; a save asks for a name, then both ask before they go ahead), N starts a new game on the title menu
 * after the question, M / F the music and sound effect switches, A the animation speed, D asks whether to leave for DOS, R or Escape returns. The caller's flag word says which options exist. */
static bool dialogAct(GameUi *ui, GameDialogOption option) {
    bool changed;
    if (!gameDialogOptionEnabled(option, ui->dialogFlags, ui->driverFlags)) {
        return true;
    }
    switch (option) {
    case GameDialogSave:
    case GameDialogLoad:
        ui->dialogMode = UiDialogSlots;
        ui->dialogOption = option;
        ui->dialogSlot = -1;
        break;
    case GameDialogNewGame:
        ui->dialogOption = option;
        dialogAsk(ui, GameDialogPromptNewGame);
        break;
    case GameDialogMusic:
        ui->driverFlags = gameDialogToggleMusic(ui->driverFlags, &changed);
        if (changed && !(ui->driverFlags & DriverMusicOn) && ui->host.music) {
            ui->host.music(ui->host.ctx, 0);
        } else if (changed) {
            ui->lastMusicPage = 0xFFFF; /* start the area's music again */
            gameUiAmbientMusic(ui);
        }
        break;
    case GameDialogSoundFx:
        ui->driverFlags = gameDialogToggleSoundFx(ui->driverFlags, &changed);
        break;
    case GameDialogAnimation:
        ui->animationSpeed = gameDialogNextAnimationSpeed(ui->animationSpeed);
        break;
    case GameDialogDos:
        ui->dialogOption = option;
        dialogAsk(ui, GameDialogPromptDos);
        break;
    case GameDialogReturn:
        dialogLeave(ui);
        break;
    default:
        break;
    }
    return true;
}

static bool dialogKey(GameUi *ui, UiKey key) {
    switch (ui->dialogMode) {
    case UiDialogSlots:
        if (key >= '1' && key <= '6') {
            dialogChooseSlot(ui, (unsigned)(key - '1'));
        } else if (key == UiKeyEscape || key == (ui->dialogOption == GameDialogSave ? 's' : 'l')) {
            ui->dialogMode = UiDialogMenu;
        }
        return true;
    case UiDialogName:
        dialogNameKey(ui, key);
        return true;
    case UiDialogPrompt: {
        int answer = gameDialogPromptKey(key >= 'a' && key <= 'z' ? (uint8_t)(key - 32) : (uint8_t)key);
        return answer < 0 ? true : dialogPromptAnswer(ui, answer == 1);
    }
    default:
        break;
    }
    GameDialogOption option = key == UiKeyEscape ? GameDialogReturn : key >= 'a' && key <= 'z' ? gameDialogKeyOption((uint8_t)(key - 32)) : GameDialogNone;
    return dialogAct(ui, option);
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
            pressButton(ui, 4, IconShootX, IconRowY, 2);
            sessionAttack(s);
        } else if (key == 'c') { /* the first attack spell the member can cast */
            for (unsigned spell = 1; spell < 126; spell++) {
                if (sessionCast(s, spell) == SessionCastDone) {
                    break;
                }
            }
        }
        playPending(ui);
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
    } else if (key == ' ') {
        extended = false;
        code = ' ';
    } else if (key >= 'a' && key <= 'z') {
        extended = false;
        code = (uint8_t)(key - 'a' + 'A');
    }
    MainCommand command = mainCommandForKey(s->game, extended, code);
    switch (command.action) {
    case MainActionMove:
        {
            unsigned pad = command.movement == MovementTurnLeft ? 0 : command.movement == MovementForward ? 1 : command.movement == MovementTurnRight ? 2 : command.movement == MovementStrafeLeft ? 3
                           : command.movement == MovementBackward ? 4 : 5;
            unsigned regionCount;
            const uint16_t(*pads)[5] = uiRegionEntries(s->game, UiRegionsDirectionPad, &regionCount);
            if (pads && pad < regionCount) { /* the pressed arrow: category 8 pictures 18, 20, 22, 24, 26, 28 at the button's corner */
                pressButton(ui, 18 + 2 * pad, pads[pad][0], pads[pad][2], 1);
            }
            SessionStep step = sessionMove(s, command.movement);
            if (step == SessionStepBlocked && s->game == GameYendor2) {
                effect(ui, 6); /* the bump sound, _val33 */
            } else if (step == SessionStepDoorOpening) {
                ui->doorTicks = 2; /* the opening frame stays for two animation ticks */
            }
            if (s->pendingSound) {
                effect(ui, s->pendingSound);
                s->pendingSound = 0;
            }
        }
        gameUiAmbientMusic(ui);
        break;
    case MainActionRest:
        pressButton(ui, 8, IconRestX, IconRowY, 2);
        sessionRest(s);
        break;
    case MainActionAlchemy:
        pressButton(ui, 6, IconAlchemyX, IconRowY, 2); /* the alchemy screen itself is not in the standalone */
        break;
    case MainActionUnlockDoor: /* a demo skeleton key: every tier, as chest and door key at once */
        sessionUnlock(s, 0xFFC0, 0);
        break;
    case MainActionAct: /* S: the party shoots its weapons down the corridor */
        if (sessionShootStart(s)) {
            playPending(ui);
        }
        break;
    case MainActionExamine: /* Space, or the right button on the view: use what is ahead */
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
        dialogOpen(ui, InGameDialogFlags);
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
    if (s->doorOpening || s->shot.active) { /* any key finishes a door that is still opening, or a shot, first */
        gameUiSettle(ui);
    }
    s->loot.pending = false; /* the loot panel stays until the next command */
    if (ui->overlay == UiOverlayTitle) {
        if (key == 'e') {
            if (!titleCanEnter(rosterSlotSum(ui))) {
                effect(ui, 3);
                return true;
            }
            if (ui->boot == UiBootTitle) { /* a first start without saves: the options dialog comes before the game */
                ui->boot = UiBootOptions;
                dialogOpen(ui, GameDialogFlagSave | GameDialogFlagDos | GameDialogFlagAnimation | GameDialogFlagReturn);
                return true;
            }
            ui->overlay = UiOverlayNone;
            if (ui->boot != UiBootNone) {
                ui->boot = UiBootNone;
                ui->lastMusicPage = 0xFFFF;
            }
            gameUiAmbientMusic(ui);
        } else if (key == 'a') {
            rosterSyncFlags(ui);
            ui->overlay = UiOverlayRoster;
        } else if (key == 'c') {
            if (wizardStart(ui)) {
                ui->overlay = UiOverlayCreate;
            } else {
                effect(ui, 3);
            }
        } else if ((key == 'i' || key == 'r') && ui->host.cinematic) {
            ui->host.cinematic(ui->host.ctx, key == 'i' ? "opening" : "credits");
        } else if (key == 'q' && ctrl) { /* Ctrl+Q, the only way out: Escape does nothing on the title menu */
            return false;
        } else if (key == UiKeyEnter || (key == 'm' && ctrl)) { /* the music switch (RunTitleScreen: Enter, which is Ctrl+M, as the picture says), and Ctrl+S the sound effects */
            bool changed;
            ui->driverFlags = gameDialogToggleMusic(ui->driverFlags, &changed);
            if (changed && !(ui->driverFlags & DriverMusicOn) && ui->host.music) {
                ui->host.music(ui->host.ctx, 0);
            }
        } else if (key == 's' && ctrl) {
            bool changed;
            ui->driverFlags = gameDialogToggleSoundFx(ui->driverFlags, &changed);
        }
        return true;
    }
    if (ui->overlay == UiOverlayDialog) {
        return dialogKey(ui, key);
    }
    if (ui->overlay == UiOverlayCreate) {
        return wizardKey(ui, key);
    }
    if (ui->overlay == UiOverlayRoster) {
        return rosterKey(ui, key);
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

/* ---- the mouse ---- */

static bool inRect(int x, int y, int x0, int y0, int x1, int y1) {
    return x >= x0 && x <= x1 && y >= y0 && y <= y1;
}

static bool press(GameUi *ui, int key) {
    return gameUiKey(ui, (UiKey)key, false);
}

static bool titleClick(GameUi *ui, int x, int y) {
    static const char kKeys[5] = {'c', 'a', 'e', 'r', 'i'}; /* the regions in order: creation, assemble, enter, credits, introduction */
    unsigned region = uiRegionHit(ui->session->game, UiRegionsTitleMenu, x, y);
    return region >= 1 && region <= 5 ? press(ui, kKeys[region - 1]) : true;
}

static bool rosterClick(GameUi *ui, int x, int y) { /* 1-9 a hero's face, name or class selects it, 10-18 its check box (de)selects it, 19 DONE */
    unsigned region = uiRegionHit(ui->session->game, UiRegionsPartyRoster, x, y);
    if (region >= 1 && region <= 9) {
        ui->rosterCursor = region - 1;
    } else if (region >= 10 && region <= 18) {
        ui->rosterCursor = region - 10;
        press(ui, UiKeyEnter);
    } else if (region == 19) {
        press(ui, 'd');
    }
    return true;
}

static bool createClick(GameUi *ui, int x, int y) {
    CreationWizard *w = &ui->wizard;
    int key = 0;
    if (inRect(x, y, 8, 185, 127, 191) && w->step != CreationStepName) { /* QUIT "CREATE" under every step */
        key = 'q';
    }
    switch (w->step) {
    case CreationStepClass:
        for (unsigned id = 1; id <= 9 && !key; id++) {
            static const char kHotkeys[] = "fmroapgdk";
            int top = ((id - 1) / 3 == 0 ? 51 : (id - 1) / 3 == 1 ? 96 : 141) + 9 * (int)((id - 1) % 3);
            if (inRect(x, y, 8, top, 127, top + 8)) {
                key = kHotkeys[id - 1];
            }
        }
        break;
    case CreationStepPortrait:
        for (int cell = 0; cell < 9 && !key; cell++) {
            if (inRect(x, y, 8 + 33 * (cell % 3), 42 + 33 * (cell / 3), 39 + 33 * (cell % 3), 73 + 33 * (cell / 3))) {
                key = '1' + cell;
            }
        }
        if (!key && inRect(x, y, 8, 150, 55, 156)) {
            key = w->gender == 2 ? 'm' : 'f';
        }
        break;
    case CreationStepRoll:
        if (inRect(x, y, 8, 51, 127, 59)) {
            key = 'r';
        } else if (inRect(x, y, 8, 69, 127, 77)) {
            key = 'i';
        }
        break;
    case CreationStepItems: {
        unsigned region = uiRegionHit(ui->session->game, UiRegionsCharacterInventory, x, y);
        if (region >= 1 && region <= 8) {
            key = '0' + (int)region;
        } else if (inRect(x, y, 8, 176, 127, 182)) {
            key = 'n';
        }
        break;
    }
    case CreationStepSummary:
        for (unsigned i = 0; i < 6 && !key; i++) {
            static const char kOptions[] = "kcprin";
            static const int kY[6] = {51, 69, 78, 87, 96, 105};
            if (inRect(x, y, 8, kY[i], 127, kY[i] + 8)) {
                key = kOptions[i];
            }
        }
        break;
    default:
        break;
    }
    return key ? press(ui, key) : true;
}

static bool clueClick(GameUi *ui, int x, int y) {
    GameKind game = ui->session->game;
    unsigned tab = uiRegionHit(game, UiRegionsClueCategories, x, y);
    if (tab >= 2 && tab <= 7) { /* the tabs: maps, monsters, spells, magic users, items, the walk through */
        return press(ui, UiKeyF1 + (int)(tab - 2));
    }
    if (tab == 8) { /* EXIT */
        ui->overlay = UiOverlayNone;
        return true;
    }
    unsigned scroll = uiRegionHit(game, UiRegionsClueScroll, x, y);
    if (scroll && (ui->clueScreen == 1 || ui->clueScreen == 0)) {
        return press(ui, scroll == 1 ? 'i' : 'q');
    }
    if (ui->clueScreen == 1) {
        unsigned row = uiRegionHit(game, UiRegionsClueEntries, x, y);
        if (row && ui->clueList.first + row - 1 < ui->clueList.count) {
            ui->clueList.selected = ui->clueList.first + row - 1;
            press(ui, UiKeyEnter);
        }
    } else if (ui->clueScreen == 2 && ui->clueCategory == 1) { /* a legend marker of a map */
        ClueMapMarker markers[ClueMapMarkersMax];
        const ClueEntry *entry = clueSelected(ui);
        unsigned count = entry ? clueMapMarkers(game, ui->worldDat, ui->worldSize, entry->id, markers, ClueMapMarkersMax) : 0;
        int hit = clueMapMarkerAt(markers, count, x, y);
        if (hit >= 0) {
            ui->clueMarker = hit;
        }
    } else if (ui->clueScreen == 2 && ui->clueCategory >= 12) { /* the "+N" icons under an item */
        const ClueEntry *entry = clueSelected(ui);
        uint16_t mask;
        unsigned count = entry ? clueSubIconCount(&ui->session->items, entry->id, &mask) : 0;
        for (unsigned i = 0; i < count; i++) {
            uint16_t region[5];
            clueSubIconRegion(i, region);
            if (inRect(x, y, region[0], region[2], region[1], region[3])) {
                ui->clueSub = i;
            }
        }
    }
    return true;
}

static bool dialogClick(GameUi *ui, int x, int y) {
    unsigned region = uiRegionHit(ui->session->game, UiRegionsGameDialog, x, y);
    switch (ui->dialogMode) {
    case UiDialogSlots:
        if (region >= 1 && region <= SaveSlotCount) {
            dialogChooseSlot(ui, region - 1);
        } else if (region == (ui->dialogOption == GameDialogSave ? 7u : 8u)) { /* the option's own button gives it up */
            ui->dialogMode = UiDialogMenu;
        }
        return true;
    case UiDialogName:
        return true;
    case UiDialogPrompt: {
        int answer = gameDialogPromptClick(x, y);
        return answer < 0 ? true : dialogPromptAnswer(ui, answer == 1);
    }
    default:
        return dialogAct(ui, gameDialogRegionOption(region));
    }
}

static bool talkClick(GameUi *ui, int x, int y) { /* the topic rows drawTalk lists: five to a column from y = 120 */
    GameSession *s = ui->session;
    for (unsigned i = 0; i < s->talk.listedCount && i < 9; i++) {
        int x0 = 8 + (int)(i / 5) * 150, y0 = 120 + (int)(i % 5) * 8;
        if (inRect(x, y, x0, y0, x0 + 140, y0 + 7)) {
            return press(ui, '1' + (int)i);
        }
    }
    return true;
}

static bool shopClick(GameUi *ui, int x, int y) { /* the stock rows of drawShop, and the cursor line that sells the item back */
    for (int i = 0; i < 8; i++) {
        if (inRect(x, y, 8, 30 + 8 * i, 300, 37 + 8 * i)) {
            return press(ui, '1' + i);
        }
    }
    if (ui->session->heldItem && inRect(x, y, 8, 110, 300, 117)) {
        return press(ui, 's');
    }
    return true;
}

static bool mainClick(GameUi *ui, int x, int y, bool right) {
    GameSession *s = ui->session;
    GameKind game = s->game;
    if (right) { /* the right button on the view uses what is ahead (the same as Space) */
        return uiRegionHit(game, UiRegionsDungeonMain, x, y) == 1 ? press(ui, ' ') : true;
    }
    unsigned pad = uiRegionHit(game, UiRegionsDirectionPad, x, y);
    if (pad) { /* the walk pad: turn left, forward, turn right, strafe left, back, strafe right */
        static const UiKey kKeys[6] = {UiKeyLeft, UiKeyUp, UiKeyRight, UiKeyLeft, UiKeyDown, UiKeyRight};
        return gameUiKey(ui, kKeys[pad - 1], pad == 4 || pad == 6);
    }
    unsigned region = uiRegionHit(game, UiRegionsDungeonMain, x, y);
    if (region == 2) {
        return press(ui, 'm');
    }
    if (region == 3) { /* the icon row: act (attack in a combat), alchemy, rest, the pause dialog */
        unsigned icon = uiRegionHit(game, UiRegionsDungeonIconRow, x, y);
        return icon == 1 ? press(ui, s->combat.active ? 'a' : 's') : icon == 3 ? press(ui, 'r') : icon == 4 ? press(ui, 'd') : true;
    }
    if (region == 6) { /* a party panel: its portrait or name bar opens that hero's sheet */
        unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y);
        unsigned member = panel / 10, part = panel % 10;
        if (panel && (part == 1 || part == 8) && member < 4) {
            return press(ui, UiKeyF1 + (int)member);
        }
    }
    return true;
}

bool gameUiClick(GameUi *ui, int x, int y, bool right) {
    ui->session->loot.pending = false; /* the loot panel stays until the next command */
    if (right && ui->overlay != UiOverlayNone && ui->overlay != UiOverlayTitle) { /* the right button backs out */
        return press(ui, UiKeyEscape);
    }
    switch (ui->overlay) {
    case UiOverlayTitle:
        return titleClick(ui, x, y);
    case UiOverlayRoster:
        return rosterClick(ui, x, y);
    case UiOverlayCreate:
        return createClick(ui, x, y);
    case UiOverlayClue:
        return clueClick(ui, x, y);
    case UiOverlayDialog:
        return dialogClick(ui, x, y);
    case UiOverlayTalk:
        return talkClick(ui, x, y);
    case UiOverlayShop:
        return shopClick(ui, x, y);
    case UiOverlayLocalMap:
    case UiOverlayDolls:
        ui->overlay = UiOverlayNone; /* any click closes them */
        return true;
    case UiOverlayDetail:
        if (uiRegionHit(ui->session->game, UiRegionsMemberDetail, x, y) == 1) { /* RETURN */
            ui->overlay = UiOverlayNone;
        }
        return true;
    default:
        return mainClick(ui, x, y, right);
    }
}
