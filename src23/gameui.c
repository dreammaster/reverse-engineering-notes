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
#include "font.h"
#include "gamedialog.h"
#include "lighting.h"
#include "localmap.h"
#include "maininput.h"
#include "minimap.h"
#include "music.h"
#include "paperdoll.h"
#include "party.h"
#include "pictures.h"
#include "roster.h"
#include "statsheet.h"
#include "statuspanel.h"
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

/*
 * The clue book's category screens, from the entry lists the executable carries (clueentries.h). A list page (the backdrop of picture 14, 14 names to a page) shows one category's entries,
 * Enter opens the selected one: a page for the maps (1), monsters (2), spells (3, 5-10) and items (12-17), the spell list of a class from the class list (4 -> 5-10), the item list of a
 * group from the group list (11 -> 12-17; group 7 is the transportation page, group 2 -- the attribute enhancers -- has nothing). Escape backs out one level. Entries without the
 * shareware flag are dim and refused with the registration reminder unless the program is registered (clueRegistered).
 */
enum { ClueNameSize = ClueEntryTextSize + 16 };

static unsigned clueItemGroupCategory(unsigned group) { /* the item group's list: 1 -> 12 ... 6 -> 16, 8 -> 17 (2 and 7 have none) */
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
        } else if (id == 7) {
            ui->clueScreen = 2;
        }
    } else {
        ui->clueScreen = 2;
    }
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
    viewDrawPicture(&ui->renderer, 0, game == GameYendor2 ? 14 : 6, 1, 1, false, 0);
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
    if (entry && kind == ItemTargetWearable) {
        clueArmorRowDraw(&ui->renderer, &text, entry, itemEffectEntry(catalog, record));
    } else if (entry && kind == ItemTargetWeapon) {
        clueWeaponRowDraw(&ui->renderer, &text, entry);
    } else if (entry && kind == ItemTargetConsumable) {
        clueHealingRowDraw(&ui->renderer, &text, game, entry);
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
    if (category == 11) { /* group 7: the transportation page */
        static ClueTransportData transport;
        if (clueTransportLoad(&transport, ui->exe, game)) {
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
        uint8_t record[MonsterRecordSize];
        if (clueMonsterTextLoad(&text, ui->exe, game) && monsterRecordSpawn(record, &ui->session->monsters, entry->id)) {
            char name[MonsterNameBufferSize];
            monsterGetName(record, name);
            clueMonsterPageDraw(&ui->renderer, &text, record, name, tab);
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
            if (!titleCanEnter(rosterSlotSum(ui))) {
                effect(ui, 3);
                return true;
            }
            ui->overlay = UiOverlayNone;
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
        } else if (key == UiKeyEscape || (key == 'q' && ctrl)) {
            return false;
        }
        return true;
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
