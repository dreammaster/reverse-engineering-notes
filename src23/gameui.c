#include "gameui.h"

#include <stdio.h>
#include <string.h>

#include "cluebook.h"
#include "clueentries.h"
#include "clueitem.h"
#include "cluemap.h"
#include "cluemonster.h"
#include "alchemylist.h"
#include "cluespell.h"
#include "cluetransport.h"
#include "creationwizard.h"
#include "explore.h"
#include "font.h"
#include "gamedialog.h"
#include "gameclock.h"
#include "lighting.h"
#include "localmap.h"
#include "mapview.h"
#include "maininput.h"
#include "minimap.h"
#include "monsterpanel.h"
#include "music.h"
#include "paperdoll.h"
#include "party.h"
#include "pictures.h"
#include "roster.h"
#include "spellcast.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "textpanel.h"
#include "titlemenu.h"
#include "uiregions.h"

static void paletteSyncToClock(GameUi *ui);

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
    ui->driverFlags = DriverMusicAvailable | DriverMusicOn | DriverSoundFxAvailable | DriverSoundFxOn;
    ui->animationSpeed = 5;
    uint32_t offset = pictureMasterPaletteOffset(session->game);
    if (offset + PicturePaletteSize <= worldSize) {
        memcpy(ui->palette, worldDat + offset, sizeof(ui->palette));
    }
    paletteSyncToClock(ui);
    return true;
}

const uint8_t *gameUiPalette(const GameUi *ui) {
    uint32_t offset = pictureMasterPaletteOffset(ui->session->game);
    return offset + PicturePaletteSize <= ui->worldSize ? ui->palette : NULL;
}

/* The world data's palette block 2 (the sunrise ramp and the cycled colours), or NULL. */
static const uint8_t *paletteBlock2(const GameUi *ui) {
    uint32_t offset = paletteBlockOffset(ui->session->game, 2);
    return offset + PaletteBytes <= ui->worldSize ? ui->worldDat + offset : NULL;
}

/* AnimatePaletteCycle: the colours 0xD0-0xDF are block 2's colours 144-159, in four groups of four, each group turned by `cycleStep` colours. */
static void paletteCycle(GameUi *ui) {
    const uint8_t *block2 = paletteBlock2(ui);
    if (!block2) {
        return;
    }
    paletteCycleFrame(block2, ui->cycleStep, ui->palette + CycleDacStart * 3);
    ui->cycleStep = (ui->cycleStep + 1) % 4;
    ui->paletteStamp++;
}

/* The sky colours for the game's hour at once: the master palette's (the end of the dawn fade) from 06:00 to 18:00, else the end of the dusk fade. Cancels a fade under way. */
static void paletteSyncToClock(GameUi *ui) {
    const uint8_t *block2 = paletteBlock2(ui);
    uint32_t master = pictureMasterPaletteOffset(ui->session->game);
    if (!block2 || master + PicturePaletteSize > ui->worldSize) {
        return;
    }
    unsigned minutes = ui->session->clock.minutes;
    if (minutes >= 360 && minutes < 1080) {
        memcpy(ui->palette + DayNightDacStart * 3, ui->worldDat + master + DayNightDacStart * 3, DayNightWindowBytes);
    } else {
        DayNightFade fade;
        uint8_t window[DayNightWindowBytes];
        dayNightFadeBegin(&fade, 1080);
        while (dayNightFadeStep(&fade, block2, window)) {
        }
        memcpy(ui->palette + DayNightDacStart * 3, window, sizeof(window));
    }
    ui->fadeActive = false;
    ui->session->pendingFade = 0;
    ui->paletteStamp++;
}

/* AdvanceDayNightPaletteFade: one window of the sunrise ramp into the sky colours. */
static void paletteFadeStep(GameUi *ui) {
    const uint8_t *block2 = paletteBlock2(ui);
    uint8_t window[DayNightWindowBytes];
    if (!block2) {
        ui->fadeActive = false;
        return;
    }
    ui->fadeActive = dayNightFadeStep(&ui->fade, block2, window);
    memcpy(ui->palette + DayNightDacStart * 3, window, sizeof(window));
    ui->paletteStamp++;
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
/*
 * The conversation (UseItem, yendor2.asm:13168): over the view, DrawItemUseConfirmDialog's frame -- category 1 picture 3, the brown scroll, at (15, 23) -- with the speaker's face (category 7,
 * the NPC header's first word) at (21, 31); BuildItemUseMessage writes the topic's rows in 0x0D from (22, 67), 6 pixels apart; and, once the greeting has been read (a key or a click),
 * DrawEligibleItemList the topic names to the right of the face in 0x8A: five to a column at x = 58 and x = 142 from y = 32.
 */
static void drawTalk(GameUi *ui) {
    GameSession *s = ui->session;
    const ViewRenderer *r = &ui->renderer;
    viewDrawPicture(r, 1, 3, 15, 23, true, 0);
    if (s->talk.npc) {
        viewDrawPicture(r, 7, dialogGetU16(s->talk.npc, 0), 21, 31, true, 0);
    }
    for (unsigned i = 0; i < ui->talkRowCount; i++) {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 22, 67 + 6 * (int)i, ui->talkRows[i], 0x0D, 0, FontTransparent);
    }
    for (unsigned i = 0; ui->talkListShown && i < s->talk.listedCount && i < ConversationListMax; i++) {
        char name[DialogTopicNameSize + 1];
        dialogTopicName(dialogTopic(&s->dialogs, s->talk.listed[i]), name);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, i < 5 ? 58 : 142, 32 + 6 * (int)(i % 5), name, 0x8A, 0, FontTransparent);
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

/*
 * The panel at the top right (mapview.h's map-flags word, saved in the header at +0x20 as word_36C7F): the chapter plate (0x2000: a new game, and what a party that cannot map
 * is left with), the minimap (0x4000, only for a party whose Mapping average reaches 50) or the resource panel (0x1000: the slot grid, category 6 picture 0x14, and the three
 * counters in the text panel). F5 and a click on it cycle them (start's loc_103BA and the F5 handler, :505).
 */
enum { MapFlagsOffset = 0x20, PanelModeBits = 0x7000 };

static uint16_t panelFlags(GameUi *ui) {
    GameSession *s = ui->session;
    PartyStatAverages averages = partyAverageStatTiers(&s->save);
    bool redraw;
    uint16_t flags = mapviewApplyTiers(saveHeaderGetU16(&s->save, MapFlagsOffset), averages.mapping, &redraw);
    saveHeaderSetU16(&s->save, MapFlagsOffset, flags);
    return flags;
}

static void panelSet(GameUi *ui, uint16_t flags) {
    saveHeaderSetU16(&ui->session->save, MapFlagsOffset, flags);
}

static void panelEnterResources(GameUi *ui, uint16_t flags) { /* ShowResourceDepletedOverlay */
    panelSet(ui, (uint16_t)((flags & 0x8FFF) | MapFlagHidden));
}

/* The F5 key: a party that can map goes minimap -> resources -> minimap, one that cannot plate -> resources -> plate. */
static void panelCycle(GameUi *ui) {
    uint16_t flags = panelFlags(ui);
    if (flags & MapFlagLargeCapable) {
        if (flags & MapFlagLarge) {
            panelEnterResources(ui, flags);
        } else {
            panelSet(ui, (uint16_t)((flags & 0x8FFF) | MapFlagLarge));
        }
    } else if (flags & MapFlagSmall) {
        panelEnterResources(ui, flags);
    } else {
        panelSet(ui, (uint16_t)((flags & 0x8FFF) | MapFlagSmall));
    }
    effect(ui, 2);
}

static void panelLeaveResources(GameUi *ui) { /* from the resource panel back to the minimap (a party that can map) or the plate */
    uint16_t flags = (uint16_t)(panelFlags(ui) & 0xEFFF);
    panelSet(ui, (uint16_t)(flags | ((flags & MapFlagLargeCapable) ? MapFlagLarge : MapFlagSmall)));
}

/*
 * The resource panel (ShowResourceDepletedOverlay, yendor2.asm:41015): the plate of category 6 picture 0x14 with nine 16 x 16 item icons in a 3 x 3 grid (the regions of UiRegionsStatusIconBar):
 * the party's gold coins, magic ore and nuore (items 1-3) while it has any, then the six belt slots (session.h) with the icon of the item each holds.
 */
static void resourceIconsDraw(GameUi *ui) {
    const ViewRenderer *r = &ui->renderer;
    GameSession *s = ui->session;
    static const unsigned kMaterial[3] = {SaveHeaderGold, SaveHeaderOreCounter1, SaveHeaderOreCounter2};
    unsigned count;
    const uint16_t(*cells)[5] = uiRegionEntries(s->game, UiRegionsStatusIconBar, &count);
    viewDrawPicture(r, 6, 0x14, 0xF8, 9, true, 0);
    for (unsigned i = 0; cells && i < count && i < 9; i++) {
        unsigned id = 0;
        if (i < 3) {
            const uint8_t *amount = saveHeaderBcd4(&s->save, kMaterial[i]);
            id = amount[0] | amount[1] | amount[2] | amount[3] ? i + 1 : 0;
        } else {
            id = itemSlotId(sessionBeltSlot(s, i - 3));
        }
        const uint8_t *item = id ? itemCatalogRecord(&s->items, id) : NULL;
        if (item) {
            viewDrawPicture(r, 8, item[ItemFieldIcon] | (item[ItemFieldIcon + 1] << 8), cells[i][0], cells[i][2], true, 0);
        }
    }
}

/* DrawResourceCounterPanel (:41134): GOLD COINS, MAGIC ORE and NUORE, each label over its amount, 18 pixels apart from y = 96. */
static void resourcesDraw(GameUi *ui) {
    const ViewRenderer *r = &ui->renderer;
    SaveGame *save = &ui->session->save;
    static const char *const kLabels[3] = {"GOLD COINS:", "MAGIC ORE: ", "NUORE: "};
    const Bcd4 *amounts[3] = {(const Bcd4 *)saveHeaderBcd4(save, SaveHeaderGold), (const Bcd4 *)saveHeaderBcd4(save, SaveHeaderOreCounter1), (const Bcd4 *)saveHeaderBcd4(save, SaveHeaderOreCounter2)};
    char number[12];
    resourceIconsDraw(ui);
    if (ui->panelRunCount) { /* a message or an item's name has the text area */
        return;
    }
    textPanelClear(r, false);
    for (unsigned i = 0; i < 3; i++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 96 + 18 * (int)i, kLabels[i], 0x8A, 4, FontOpaque);
        bcd4Format(*amounts[i], number);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 102 + 18 * (int)i, number, 0x0F, 4, FontOpaque);
    }
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

/*
 * The message area of the right-hand panel: what a command wrote there stays until the next one begins (ClearStatusPanelIfDirty, yendor2.asm:11485, which every command calls first); the compass
 * direction in the header band above it is written again each time (ShowCompassDirection, :30607: "   NORTH" ... in white at (241, 87), when the party's Mapping average has reached 45, outside a combat).
 */
static void panelRun(GameUi *ui, int x, int y, uint8_t colour, bool opaque, const char *text) {
    if (ui->panelRunCount < UiPanelRunMax) {
        PanelRun *run = &ui->panelRuns[ui->panelRunCount++];
        run->x = x;
        run->y = y;
        run->colour = colour;
        run->opaque = opaque;
        snprintf(run->text, sizeof(run->text), "%s", text);
    }
}

static void panelRunsDraw(GameUi *ui) {
    for (unsigned i = 0; i < ui->panelRunCount; i++) {
        const PanelRun *run = &ui->panelRuns[i];
        fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, run->x, run->y, run->text, run->colour, 4, run->opaque ? FontOpaque : FontTransparent);
    }
}

static void compassDraw(GameUi *ui) {
    static const char *const kNames[4] = {"   NORTH", "   SOUTH", "    EAST", "    WEST"};
    uint16_t facing = ui->session->facing;
    fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 241, 87, kNames[facing & 0x8000 ? 0 : facing & 0x4000 ? 1 : facing & 0x1000 ? 2 : 3], 0x0F, 4, FontTransparent);
}

/*
 * The result of a rest (RestPartyAndAdvanceClock, :25686), written the way the original does it: "X HOURS PASS" and "X FOOD EATEN" in 0x8A over the cleared area from (240, 96), 6 pixels apart,
 * the two X's then overwritten with the real numbers in white; an interrupted rest writes the hours reached and, from y = 108, "YOUR REST IS INTERRUPTED" (and waits for a key before the combat
 * shows); where the party cannot rest, "YOU CAN NOT REST HERE" (transparent) and the refusal sound.
 */
static void restReport(GameUi *ui) {
    GameSession *s = ui->session;
    if (!s->restReport) {
        return;
    }
    s->restReport = false;
    const RestOutcome *rest = &s->rest;
    char digit[2] = {0, 0};
    ui->panelRunCount = 0;
    if (rest->noRest) {
        panelRun(ui, 240, 96, 0x8A, false, "YOU CAN NOT");
        panelRun(ui, 240, 102, 0x8A, false, "REST HERE");
        effect(ui, 3);
    } else if (rest->interrupted) {
        digit[0] = (char)('0' + rest->hour % 10);
        panelRun(ui, 240, 96, 0x8A, true, "X HOURS PASS");
        panelRun(ui, 240, 96, 0x0F, true, digit);
        panelRun(ui, 240, 108, 0x8A, true, "YOUR REST IS");
        panelRun(ui, 240, 114, 0x8A, true, "INTERRUPTED");
        ui->restPause = true;
    } else {
        panelRun(ui, 240, 96, 0x8A, true, "X HOURS PASS");
        panelRun(ui, 240, 102, 0x8A, true, "X FOOD EATEN");
        digit[0] = (char)('0' + rest->hour % 10);
        panelRun(ui, 240, 96, 0x0F, true, digit);
        digit[0] = (char)('0' + rest->fed % 10);
        panelRun(ui, 240, 102, 0x0F, true, digit);
    }
}

/* ShowLockStatus: what a refused step into a marked door cell tells (lockcatalog.h lockStatusText), written over the cleared message area, 6 pixels apart from (240, 96) in 0x8A. */
static void lockReport(GameUi *ui) {
    GameSession *s = ui->session;
    if (!s->lockReport) {
        return;
    }
    s->lockReport = false;
    const char *lines[2];
    unsigned count = lockStatusText(s->lockFlags, s->lockValue, s->lockSkill, lines);
    ui->panelRunCount = 0;
    for (unsigned i = 0; i < count; i++) {
        panelRun(ui, 240, 96 + 6 * (int)i, 0x8A, false, lines[i]);
    }
}

/*
 * ShowStatusPanelMessage (yendor2.asm:35105): the lines of a message in 0x8A, transparent, from (240, 96), 6 pixels apart, over the cleared message area; they stay until the next command.
 */
static void panelMessage(GameUi *ui, const char *const *lines, unsigned count) {
    ui->panelRunCount = 0;
    for (unsigned i = 0; i < count; i++) {
        panelRun(ui, 240, 96 + 6 * (int)i, 0x8A, false, lines[i]);
    }
}

/* A message the game keeps in the executable's data segment: `lines` consecutive strings at `offset` (the original's fixed-width, space-padded rows). */
static void panelMessageExe(GameUi *ui, unsigned offset, unsigned lines) {
    char text[UiPanelRunMax][16];
    const char *shown[UiPanelRunMax];
    unsigned count = 0;
    for (; ui->exe && count < lines && count < UiPanelRunMax; count++) {
        if (!exeDataString(ui->exe, offset, text[count], sizeof(text[count]))) {
            break;
        }
        offset += (unsigned)strlen(text[count]) + 1;
        shown[count] = text[count];
    }
    panelMessage(ui, shown, count);
}

/* The message of a refused step into a locked destination: Chapter 2 DS:0x7BCE, Chapter 3 DS:0x7F00 ("LOCKED"); Chapter 3's fixed three lines (DS:0x9606). Nothing without the executable. */
static void panelMessageDenied(GameUi *ui, bool fixed) {
    if (fixed) {
        panelMessageExe(ui, 0x9606, 3);
    } else {
        panelMessageExe(ui, ui->session->game == GameYendor2 ? 0x7BCE : 0x7F00, 1);
    }
}

/* A destination that asks for a password (IsDestinationUnlocked: ShowConfirmPrompt on the gate row's message, EditTextField at (240, 114)). */
static void passwordOpen(GameUi *ui, unsigned promptId) {
    ui->passwordActive = true;
    ui->passwordPrompt = promptId;
    textFieldInit(&ui->password, 13);
}

static void passwordDraw(GameUi *ui) {
    ConfirmPrompt prompt;
    GameSession *s = ui->session;
    for (int row = 0; row < 60; row++) { /* ClearMessageBoxArea */
        memset(&ui->screen[(96 + row) * ViewScreenWidth + 240], 4, 72);
    }
    if (confirmPromptLoad(ui->exe, s->game, ui->passwordPrompt, &prompt)) {
        confirmPromptDraw(&ui->renderer, &prompt);
    } else {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 240, 96, "PASSWORD?", 0x8A, 4, FontTransparent);
    }
    char typed[TextFieldMaxSize + 2];
    snprintf(typed, sizeof(typed), "%s-", ui->password.text);
    fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 240, 114, typed, 0x0F, 4, FontOpaque);
}

static bool passwordKey(GameUi *ui, UiKey key) {
    GameSession *s = ui->session;
    unsigned ascii = key >= 'a' && key <= 'z' ? (unsigned)key - 32 : key < 0x80 ? (unsigned)key : 0; /* the keyboard poll upper-cases the letters */
    switch (ascii ? textFieldKey(&ui->password, (uint8_t)ascii) : TextFieldEditing) {
    case TextFieldConfirmed:
        ui->passwordActive = false;
        if (sessionTravelPassword(s, ui->password.text)) {
            playPending(ui);
            gameUiAmbientMusic(ui);
        }
        break;
    case TextFieldCancelled:
        ui->passwordActive = false;
        break;
    case TextFieldBeep:
        effect(ui, 3);
        break;
    default:
        break;
    }
    return true;
}

/* The name of an item in the text area, white on the panel, three 12-character rows (RedrawItemDescriptionAndMaterials: the record's +0x13 text). */
static void panelItemName(GameUi *ui, unsigned itemId) {
    const uint8_t *item = itemId ? itemCatalogRecord(&ui->session->items, itemId) : NULL;
    ui->panelRunCount = 0;
    for (unsigned i = 0; item && i < 3; i++) {
        char line[ItemNameLineSize];
        itemGetNameLine(item, i, line);
        panelRun(ui, 240, 96 + 6 * (int)i, 0x0F, true, line);
    }
}

/* The refusal beep (FlashStatusWarning plays sound 3 and flashes the cursor). */
static void flashWarning(GameUi *ui) {
    effect(ui, 3);
}

/*
 * HandleStatusPanelItemSlotClick's tail (yendor2.asm:41175): a click on a belt slot of the resource panel. Picking an item up or swapping shows the name of the item now on the cursor, putting it down
 * leaves the counters; an empty slot with an empty hand, or a refusal (a container, a key that is no key, the beep), is what ends the resource panel (errorCode 1).
 */
static bool beltClick(GameUi *ui, unsigned slot) {
    GameSession *s = ui->session;
    SessionBelt result = sessionBeltClick(s, slot);
    playPending(ui);
    ui->panelRunCount = 0;
    switch (result) {
    case SessionBeltTook:
    case SessionBeltSwapped:
        panelItemName(ui, s->heldItem);
        return true;
    case SessionBeltPlaced:
    case SessionBeltKeyRing:
        return true;
    case SessionBeltRefused:
        flashWarning(ui);
        return false;
    default:
        return false;
    }
}

/* A left click on the top-right panel (start's loc_103BA): outside the resource panel it opens it; inside, the three materials show their counters, a belt slot is worked, and a click anywhere
 * else -- or on a slot that cannot be used -- leaves it. */
static void panelClick(GameUi *ui, int x, int y) {
    uint16_t flags = panelFlags(ui);
    if (!(flags & MapFlagHidden)) {
        panelEnterResources(ui, flags);
        ui->panelRunCount = 0;
        effect(ui, 2);
        return;
    }
    unsigned region = uiRegionHit(ui->session->game, UiRegionsStatusIconBar, x, y);
    if (region >= 1 && region <= 3) {
        ui->panelRunCount = 0;
        return;
    }
    if (region >= 4 && region <= 9 && beltClick(ui, region - 4)) {
        return;
    }
    panelLeaveResources(ui);
    effect(ui, 2);
}

/* ---- the cursor ---- */

void gameUiMouseMove(GameUi *ui, int x, int y) {
    ui->mouseX = x;
    ui->mouseY = y;
}

bool gameUiCursorShown(const GameUi *ui) {
    return ui->targeting || ui->session->heldItem != 0;
}

/* The cursor item or the crosshair, over everything else (DrawMouseCursor: the 16 x 16 sprite blitted at the pointer with 0xFF transparent). */
static void cursorDraw(GameUi *ui) {
    unsigned picture = 0;
    if (ui->session->heldItem) {
        const uint8_t *item = itemCatalogRecord(&ui->session->items, ui->session->heldItem);
        picture = item ? (unsigned)(item[ItemFieldIcon] | (item[ItemFieldIcon + 1] << 8)) : 0;
    } else if (ui->targeting) {
        picture = 0x0F;
    }
    if (picture) {
        viewDrawPicture(&ui->renderer, 8, picture, ui->mouseX, ui->mouseY, true, 0);
    }
}

/* ---- the commands that need an item (K, M, T, W) ---- */

/* IsItemRangeAvailable on the command's item: the belt, then every member's inventory. An unavailable command does nothing, silently. */
static bool commandItem(GameUi *ui, MainAction action) {
    unsigned id = mainCommandRequiredItem(ui->session->game, action);
    return id == 0 || sessionHasItem(ui->session, id, id);
}

/* "YOUR SKILL / IS NOT HIGH / ENOUGH!" (ShowMapSkillTooLowMessage: DS:0x7D7E, Chapter 3 0x80AB). */
static void mapSkillTooLow(GameUi *ui) {
    panelMessageExe(ui, ui->session->game == GameYendor2 ? 0x7D7E : 0x80AB, 3);
}

/*
 * ShowGameClockCommand (yendor2.asm:43387): the time in the 12-hour form "HH:MM AM" and the date "MM/DD/YYYY", digits zero-padded, white, over the cleared text area at (240, 96) and (240, 102).
 * Midnight to 00:59 reads 12:xx AM, noon to 12:59 12:xx PM, from 13:00 the hour less 12.
 */
static void clockShow(GameUi *ui) {
    GameClock *c = &ui->session->clock;
    unsigned minutes = c->minutes, hour = minutes / 60, minute = minutes % 60;
    bool pm = minutes > 0x2CF;
    char time[16], date[16];
    hour = minutes <= 59 ? 12 : minutes <= 0x30B ? hour : hour - 12;
    snprintf(time, sizeof(time), "%02u:%02u %cM", hour % 100, minute, pm ? 'P' : 'A');
    snprintf(date, sizeof(date), "%02u/%02u/%04u", (unsigned)c->month % 100, (unsigned)c->day % 100, (unsigned)c->year % 10000);
    ui->panelRunCount = 0;
    panelRun(ui, 240, 96, 0x0F, false, time);
    panelRun(ui, 240, 102, 0x0F, false, date);
}

/* The status panel's word for an unlock (UnlockDoorCommand: NOT LOCKED DS:0x7BB7, UNLOCKED 0x7BAE; Chapter 3 0x7EE9 / 0x7EE0). */
static void unlockReport(GameUi *ui) {
    GameSession *s = ui->session;
    if (s->unlockMessage) {
        bool y2 = s->game == GameYendor2;
        panelMessageExe(ui, s->unlockMessage == 1 ? (y2 ? 0x7BB7 : 0x7EE9) : (y2 ? 0x7BAE : 0x7EE0), 1);
        s->unlockMessage = 0;
    }
}

/* K with the key ring: the crosshair waits for a click on the view (Escape gives up), then the faced lock is tried. */
static void unlockStart(GameUi *ui) {
    if (ui->session->combat.active) {
        flashWarning(ui);
        return;
    }
    ui->targeting = true;
}

static void unlockTry(GameUi *ui) {
    GameSession *s = ui->session;
    ui->targeting = false;
    ui->panelRunCount = 0;
    sessionUnlockWithRing(s);
    playPending(ui);
    unlockReport(ui);
}

/* ---- the alchemy screen ---- */

static bool alchemyMember(const GameUi *ui, unsigned slot) { /* a member with magic points to spend: the record word +0x94 (maximum MP) is not zero */
    const uint8_t *member = sessionPartyRecord(ui->session, slot);
    return member && partyGetStatMax(member, PartyStatMagicPoints) != 0;
}

/* BuildAlchemySpellList: the spells the caster knows, each marked castable by CheckSpellCastability (none are when the caster cannot act); the spell chosen last stays selected. */
static void alchemyBuild(GameUi *ui) {
    GameSession *s = ui->session;
    uint8_t *caster = sessionPartyRecord(s, ui->alchemyCaster);
    unsigned known[256];
    unsigned found = caster ? partyKnownAbilityIds(caster, s->game, known, 256) : 0;
    bool able = caster && !(partyGetU16(caster, PartyFieldStatusFlags) & PartyStatusIncapacitated);
    ui->alchemyCount = 0;
    for (unsigned i = 0; i < found && i < 256 && ui->alchemyCount < UiAlchemyMax; i++) {
        const uint8_t *spell = spellRecord(&s->spells, known[i]);
        if (!spell) {
            continue;
        }
        ui->alchemyIds[ui->alchemyCount] = known[i];
        ui->alchemyOk[ui->alchemyCount] = able && spellCanCast(spell, caster, &s->save, s->combat.active);
        ui->alchemyCount++;
    }
    ui->alchemySelected = 0;
    unsigned last = caster ? partyGetU16(caster, PartyFieldLastAlchemySpell) : 0;
    for (unsigned i = 0; last && i < ui->alchemyCount; i++) {
        if (ui->alchemyIds[i] == last) {
            ui->alchemySelected = i;
        }
    }
}

/* The start of RunAlchemyScreen: outside a combat the caster is the one used last if he or she still has magic points, else the first member who has; in a combat it is the member whose turn it is. */
static void alchemyOpen(GameUi *ui) {
    GameSession *s = ui->session;
    SessionCombat *c = &s->combat;
    unsigned slot = 0;
    if (c->active) {
        if (c->cursor >= c->count || c->order[c->cursor].isMonster) {
            return;
        }
        slot = c->order[c->cursor].index;
        if (!alchemyMember(ui, slot)) {
            effect(ui, 3);
            return;
        }
    } else {
        unsigned cached = saveHeaderGetU16(&s->save, SaveHeaderLastCaster);
        bool found = false;
        for (unsigned i = 0; i < 4 && !found; i++) {
            if (cached && saveGetPartySlot(&s->save, i) == cached && alchemyMember(ui, i)) {
                slot = i;
                found = true;
            }
        }
        for (unsigned i = 0; i < 4 && !found; i++) {
            if (alchemyMember(ui, i)) {
                slot = i;
                found = true;
            }
        }
        if (!found) {
            effect(ui, 3);
            return;
        }
    }
    ui->alchemyCaster = slot;
    alchemyBuild(ui);
    if (!ui->alchemyCount) {
        effect(ui, 3);
        return;
    }
    ui->alchemyMode = UiAlchemyList;
    ui->overlay = UiOverlayAlchemy;
    effect(ui, 1);
}

static void alchemyClose(GameUi *ui) {
    ui->overlay = UiOverlayNone;
    ui->alchemyMode = UiAlchemyList;
    effect(ui, 1);
}

/* The costs are paid and the spell takes effect (DeductAlchemySpellCosts, then ApplyEncodedItemEffect); the screen is left. */
static void alchemyFinish(GameUi *ui, unsigned spellId, bool returnToMark, int targetSlot) {
    GameSession *s = ui->session;
    saveHeaderSetU16(&s->save, SaveHeaderLastCaster, saveGetPartySlot(&s->save, ui->alchemyCaster));
    ui->overlay = UiOverlayNone;
    ui->alchemyMode = UiAlchemyList;
    if (targetSlot >= 0) {
        sessionCastOn(s, ui->alchemyCaster, spellId, returnToMark, (unsigned)targetSlot);
    } else {
        sessionCastBy(s, ui->alchemyCaster, spellId, returnToMark);
    }
    playPending(ui);
    paletteSyncToClock(ui); /* SAFE HAVEN rests */
    gameUiAmbientMusic(ui);
}

/* Space, Enter or a held click on the selected spell (loc_1DFAD): a grey one does nothing; the member's last choice is remembered; some spells ask first. */
static void alchemyCast(GameUi *ui) {
    GameSession *s = ui->session;
    if (!ui->alchemyCount || !ui->alchemyOk[ui->alchemySelected]) {
        return;
    }
    unsigned id = ui->alchemyIds[ui->alchemySelected];
    const uint8_t *spell = spellRecord(&s->spells, id);
    uint8_t *caster = sessionPartyRecord(s, ui->alchemyCaster);
    if (!spell || !caster) {
        return;
    }
    partySetU16(caster, PartyFieldLastAlchemySpell, (uint16_t)id);
    ui->alchemySpell = id;
    if (spellGetU16(spell, SpellFieldFlagsA) & SpellFlagsASingleTarget) {
        ui->alchemyMode = UiAlchemyTarget;
    } else if (spellGetU16(spell, SpellFieldFlagsB) & SpellFlagsBLocationBookmark) {
        ui->alchemyMode = UiAlchemyMark;
    } else {
        alchemyFinish(ui, id, false, -1);
    }
}

static void alchemyChooseMember(GameUi *ui, unsigned slot) { /* F1-F4 or a portrait: another member's list, if he or she has magic points (else the refusal beep) */
    if (ui->session->combat.active || !sessionPartyRecord(ui->session, slot)) {
        return;
    }
    if (!alchemyMember(ui, slot)) {
        effect(ui, 3);
        return;
    }
    ui->alchemyCaster = slot;
    alchemyBuild(ui);
}

static void alchemyMoveTo(GameUi *ui, unsigned index) {
    if (ui->alchemyCount) {
        ui->alchemySelected = index < ui->alchemyCount ? index : ui->alchemyCount - 1;
    }
}

static void alchemyPage(GameUi *ui, bool forward) { /* I / PageUp and Q / PageDown (loc_1E163 ... loc_1E18F) */
    unsigned page = ui->alchemySelected / AlchemyListRows, pages = (ui->alchemyCount + AlchemyListRows - 1) / AlchemyListRows;
    if (!forward) {
        alchemyMoveTo(ui, page == 0 ? 0 : ui->alchemySelected - AlchemyListRows);
    } else {
        alchemyMoveTo(ui, page + 1 >= pages ? ui->alchemyCount - 1 : ui->alchemySelected + AlchemyListRows);
    }
}

static void alchemyMark(GameUi *ui, bool returnToMark) {
    effect(ui, returnToMark ? 0x0B : 0x34);
    alchemyFinish(ui, ui->alchemySpell, returnToMark, -1);
}

static bool alchemyKey(GameUi *ui, UiKey key) {
    if (ui->alchemyMode == UiAlchemyTarget) { /* ConfirmAlchemyInteraction: the portrait the spell is for (message 0x12: nothing but the click) */
        if (key >= UiKeyF1 && key <= UiKeyF4 && sessionPartyRecord(ui->session, (unsigned)(key - UiKeyF1))) {
            alchemyFinish(ui, ui->alchemySpell, false, (int)(key - UiKeyF1));
        } else if (key == UiKeyEscape) {
            ui->alchemyMode = UiAlchemyList;
        }
        return true;
    }
    if (ui->alchemyMode == UiAlchemyMark) { /* message 0x22: MARK or RETURN (the keys M and R are a convenience; the original answers to clicks only) */
        if (key == 'm') {
            alchemyMark(ui, false);
        } else if (key == 'r') {
            alchemyMark(ui, true);
        } else if (key == UiKeyEscape) {
            ui->alchemyMode = UiAlchemyList;
        }
        return true;
    }
    if (key == UiKeyEscape || key == 'c') {
        alchemyClose(ui);
    } else if (key == ' ' || key == UiKeyEnter) {
        alchemyCast(ui);
    } else if (key == UiKeyUp) {
        alchemyMoveTo(ui, ui->alchemySelected ? ui->alchemySelected - 1 : 0);
    } else if (key == UiKeyDown) {
        alchemyMoveTo(ui, ui->alchemySelected + 1);
    } else if (key == 'i' || key == UiKeyPageUp) {
        alchemyPage(ui, false);
    } else if (key == 'q' || key == UiKeyPageDown) {
        alchemyPage(ui, true);
    } else if (key >= UiKeyF1 && key <= UiKeyF4) {
        alchemyChooseMember(ui, (unsigned)(key - UiKeyF1));
    }
    return true;
}

/* The status panel's texts come from the executable (exedata.h) when it is there. */
static void alchemyLabel(const GameUi *ui, unsigned offset2, unsigned offset3, const char *fallback, char *out, size_t capacity) {
    unsigned offset = ui->session->game == GameYendor2 ? offset2 : offset3;
    if (!ui->exe || !offset || !exeDataString(ui->exe, offset, out, capacity)) {
        snprintf(out, capacity, "%s", fallback);
    }
}

static void alchemyDraw(GameUi *ui) {
    GameSession *s = ui->session;
    const ViewRenderer *r = &ui->renderer;
    static char names[AlchemyListRows][SpellNameFieldSize + 1];
    AlchemyRow rows[AlchemyListRows];
    unsigned page = ui->alchemySelected / AlchemyListRows, pages = (ui->alchemyCount + AlchemyListRows - 1) / AlchemyListRows;
    unsigned first = page * AlchemyListRows, shown = ui->alchemyCount > first ? ui->alchemyCount - first : 0;
    shown = shown > AlchemyListRows ? AlchemyListRows : shown;
    for (unsigned i = 0; i < shown; i++) {
        const uint8_t *spell = spellRecord(&s->spells, ui->alchemyIds[first + i]);
        spellGetName(spell, names[i]);
        rows[i].name = names[i];
        rows[i].colour = ui->alchemyOk[first + i] ? 0x0F : 6;
        rows[i].mp = spellGetU16(spell, SpellFieldMpCost);
        rows[i].nuore = spellGetU16(spell, SpellFieldNuoreCost);
        rows[i].ore = spellGetU16(spell, SpellFieldOreCost);
    }
    alchemyListDraw(r, rows, shown, (int)(ui->alchemySelected - first));
    if (page > 0) { /* the scroll arrows: glyphs 'a' and 'b' at y = 119 */
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 0xA5, 0x77, "a", 0x0F, 0, FontTransparent);
    }
    if (page + 1 < pages) {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 0xB0, 0x77, "b", 0x0F, 0, FontTransparent);
    }
    const uint8_t *caster = sessionPartyRecord(s, ui->alchemyCaster);
    char blank[16], magic[16], label1[16], label2[16];
    alchemyLabel(ui, 0x7955, 0x7C87, "            ", blank, sizeof(blank));
    alchemyLabel(ui, 0x7B15, 0x7E47, "MAGIC:", magic, sizeof(magic));
    alchemyLabel(ui, 0x7C61, 0, "MAGIC ORE: ", label1, sizeof(label1));
    alchemyLabel(ui, 0x7C6D, 0x7F9A, "NUORE: ", label2, sizeof(label2));
    AlchemyPanelText text = {blank, magic, s->game == GameYendor2 ? label1 : NULL, label2};
    if (caster) {
        alchemyStatusPanelDraw(r, &text, (const char *)caster + PartyFieldName, partyGetStat(caster, PartyStatMagicPoints), partyGetStatMax(caster, PartyStatMagicPoints),
                               saveHeaderBcd4(&s->save, SaveHeaderOreCounter1), saveHeaderBcd4(&s->save, SaveHeaderOreCounter2));
    }
    if (ui->alchemyMode == UiAlchemyMark) { /* message 0x22 */
        textPanelClear(r, false);
        ConfirmPrompt prompt;
        if (confirmPromptLoad(ui->exe, s->game, 0x22, &prompt)) {
            confirmPromptDraw(r, &prompt);
        } else {
            static const char *const kAsk[3] = {"DO YOU WANT", "TO MARK OR", "RETURN?"};
            for (unsigned i = 0; i < 3; i++) {
                fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 240, 96 + 6 * (int)i, kAsk[i], 0x0F, 4, FontTransparent);
            }
            fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 240, 120, "MARK", 0x8A, 4, FontTransparent);
            fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 240, 132, "RETURN", 0x8A, 4, FontTransparent);
        }
    }
    if (ui->alchemyMode == UiAlchemyList) { /* the caster's portrait carries the selection frame (DrawPartyStatusIconRow) */
        static const int kX[4] = {8, 0x42, 0x7C, 0xB6};
        viewDrawPicture(r, 7, 0, kX[ui->alchemyCaster & 3], 0x94, true, 0);
    }
    viewDrawPicture(r, 8, 6, IconAlchemyX, IconRowY, false, 0); /* ShowAlchemyIconActive: the icon stays down */
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
    ui->cycleTicks += 2; /* one animation tick is two BIOS ticks; the timer interrupt rotates the colours every five */
    while (ui->cycleTicks >= 5) {
        ui->cycleTicks -= 5;
        paletteCycle(ui);
        changed = true;
    }
    if (ui->session->pendingFade) { /* the clock reached 06:00 or 18:00: the sky starts to change */
        dayNightFadeBegin(&ui->fade, ui->session->pendingFade == 2 ? 1080 : 360);
        ui->session->pendingFade = 0;
        ui->fadeActive = true;
        ui->fadeTicks = 91;
    }
    if (ui->fadeActive) {
        ui->fadeTicks += 2;
        if (ui->fadeTicks >= 91) {
            ui->fadeTicks = 0;
            paletteFadeStep(ui);
            changed = true;
        }
    }
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
    paletteSyncToClock(ui);
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

static void drawScreen(GameUi *ui) {
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
    uint16_t panel = panelFlags(ui);
    if ((panel & MapFlagHidden) == 0 && (panel & MapFlagLarge)) {
        MinimapTile tiles[MinimapCells];
        int16_t shades[LightingViewportCells];
        lightingViewportTable(scene.gradient, shades);
        minimapBuild(s->game, &s->grid, s->x, s->y, tiles);
        minimapDraw(&ui->renderer, tiles, shades, s->facing);
    } else if ((panel & MapFlagHidden) == 0) {
        viewDrawPicture(&ui->renderer, 6, 0x15, 0xF8, 9, true, 0); /* the chapter plate */
    }
    for (unsigned panel = 0; panel < StatusPanelCount; panel++) {
        statusPanelDraw(&ui->renderer, panel, sessionPartyRecord(s, panel));
    }
    if (ui->overlay == UiOverlayNone) {
        if (s->combat.active) { /* DrawAndCacheStatusIcon: the swords take the shoot icon's place in a combat */
            viewDrawPicture(&ui->renderer, 8, 3, IconShootX, IconRowY, false, 0);
            if (!ui->restPause) {
                combatPanelsDraw(ui);
            }
            actingFrameDraw(ui);
        } else if (s->loot.pending) {
            lootDraw(ui);
        } else if (panel & MapFlagHidden) {
            resourcesDraw(ui);
        } else if (panel & MapFlagCompass) {
            compassDraw(ui);
        }
        panelRunsDraw(ui);
        if (ui->passwordActive) {
            passwordDraw(ui);
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
        drawTalk(ui);
        break;
    case UiOverlayAlchemy:
        alchemyDraw(ui);
        break;
    case UiOverlayWorldMap: { /* ToggleMapViewMode: category 0 picture 6 over the whole screen, a "you are here" marker (category 8 picture 0x11) on the party's cell */
        viewDrawPicture(&ui->renderer, 0, 6, 0, 0, false, 0);
        if (s->x >= 0xA0 && s->y >= 0x30 && s->x <= 0x27F && s->y <= 0xEF) {
            viewDrawPicture(&ui->renderer, 8, 0x11, (s->x - 0xA0) / 0x28 * 0x18 + 0x1C, (s->y - 0x30) / 0x18 * 0x14 + 0x17, true, 0);
        }
        break;
    }
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

void gameUiDraw(GameUi *ui) {
    drawScreen(ui);
    cursorDraw(ui);
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
    ui->talkRowCount = conversationRows(&s->talk, step->topic, ui->talkRows, sizeof(ui->talkRows) / sizeof(ui->talkRows[0]));
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
    case SessionInteractNothingHere:
        panelMessageExe(ui, s->game == GameYendor2 ? 0x7D71 : 0x809E, 1);
        break;
    case SessionInteractTravelRefused:
    case SessionInteractTravelFixed:
        panelMessageDenied(ui, use.kind == SessionInteractTravelFixed);
        break;
    case SessionInteractTravelPassword:
        passwordOpen(ui, use.promptId);
        break;
    case SessionInteractTalker: {
        ConversationStep first;
        if (sessionTalkStart(s, use.value, 0, &first)) {
            ui->overlay = UiOverlayTalk;
            ui->talkListShown = false; /* the greeting first */
            talkStep(ui, &first);
        }
        break;
    }
    case SessionInteractStoryRefused:
    case SessionInteractEndingNotReady:
    case SessionInteractDemoLimit:
        if (use.textOffset) {
            panelMessageExe(ui, use.textOffset, use.textLines ? use.textLines : 1);
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
            paletteSyncToClock(ui);
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
        paletteSyncToClock(ui);
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
        paletteSyncToClock(ui);
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

static void talkChoose(GameUi *ui, unsigned number) { /* topic `number` of the list (0 = the last one: the way out) */
    GameSession *s = ui->session;
    ConversationStep step = sessionTalkChoose(s, number);
    talkStep(ui, &step);
    if (!s->talking && ui->overlay == UiOverlayTalk) {
        ui->overlay = UiOverlayNone;
    }
}

static bool talkKey(GameUi *ui, UiKey key) { /* 1-9 choose a topic, Escape the last one listed (the way out); the first key only lets the greeting go and shows the list */
    if (!ui->talkListShown) {
        ui->talkListShown = true;
        return true;
    }
    unsigned number = key == UiKeyEscape ? 0 : key >= '1' && key <= '9' ? (unsigned)(key - '1' + 1) : 99;
    if (number != 99) {
        talkChoose(ui, number);
    }
    return true;
}

static bool commandKey(GameUi *ui, UiKey key, bool ctrl) {
    GameSession *s = ui->session;
    if (s->combat.active) {
        if (key == 'a') {
            pressButton(ui, 4, IconShootX, IconRowY, 2);
            sessionAttack(s);
        } else if (key == 'c') { /* the alchemy screen, for the member whose turn it is */
            alchemyOpen(ui);
        }
        playPending(ui);
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
            if (step == SessionStepDoorOpening) {
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
        paletteSyncToClock(ui); /* eight hours have gone by */
        break;
    case MainActionAlchemy:
        alchemyOpen(ui);
        break;
    case MainActionUnlockDoor: /* K: needs the key ring */
        if (commandItem(ui, MainActionUnlockDoor)) {
            unlockStart(ui);
        }
        break;
    case MainActionClock: /* T: needs the clock */
        if (commandItem(ui, MainActionClock)) {
            clockShow(ui);
        }
        break;
    case MainActionToggleMapView: /* W: needs the world map item and a Mapping average of 80 */
        if (commandItem(ui, MainActionToggleMapView)) {
            if (panelFlags(ui) & MapFlagFullOverview) {
                ui->overlay = UiOverlayWorldMap;
            } else {
                mapSkillTooLow(ui);
            }
        }
        break;
    case MainActionAct: /* S: the party shoots its weapons down the corridor */
        if (sessionShootStart(s)) {
            playPending(ui);
        }
        break;
    case MainActionExamine: /* Space, or the right button on the view: use what is ahead */
        useAhead(ui);
        break;
    case MainActionLocalMap: /* M: needs the map item and a Mapping average of 60 */
        if (commandItem(ui, MainActionLocalMap)) {
            if (panelFlags(ui) & MapFlagLocalMap) {
                ui->overlay = UiOverlayLocalMap;
            } else {
                mapSkillTooLow(ui);
            }
        }
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

static bool keyDispatch(GameUi *ui, UiKey key, bool ctrl) {
    GameSession *s = ui->session;
    bool running = true;
    if (ui->passwordActive) {
        return passwordKey(ui, key);
    }
    if (ui->targeting) { /* WaitForTargetClick: only Escape is heard */
        if (key == UiKeyEscape) {
            ui->targeting = false;
        }
        return true;
    }
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
    if (ui->overlay == UiOverlayAlchemy) {
        return alchemyKey(ui, key);
    }
    if (ui->overlay == UiOverlayWorldMap) { /* WaitForKeypressTickingMusic: any key */
        ui->overlay = UiOverlayNone;
        return true;
    }
    if (key == UiKeyEscape) {
        if (ui->overlay != UiOverlayNone) {
            ui->overlay = UiOverlayNone;
            return true;
        }
        return false;
    }
    if (key == UiKeyF7 || key == UiKeyF9) { /* a quick save and load of the one slot the host keeps (not the original's: F5 is the panel) */
        saveLoad(ui, key == UiKeyF7);
        return true;
    }
    if (key == UiKeyF5 && ui->overlay == UiOverlayNone && !s->combat.active) {
        panelCycle(ui);
        return true;
    }
    if (key == '+' || key == '=') {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 30) % 1440);
        paletteSyncToClock(ui);
        return true;
    }
    if (key == '-') {
        s->clock.minutes = (uint16_t)((s->clock.minutes + 1440 - 30) % 1440);
        paletteSyncToClock(ui);
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

bool gameUiKey(GameUi *ui, UiKey key, bool ctrl) {
    if (ui->passwordActive) { /* typing: the message area is the prompt's */
        return keyDispatch(ui, key, ctrl);
    }
    if (ui->restPause) { /* the interrupted rest's message is read: any key lets the combat show */
        ui->restPause = false;
        ui->panelRunCount = 0;
        return true;
    }
    ui->panelRunCount = 0; /* the next command clears the message area */
    bool running = keyDispatch(ui, key, ctrl);
    restReport(ui);
    lockReport(ui);
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

static bool talkClick(GameUi *ui, int x, int y) { /* the click table at DS:0x6458: the topic rows drawTalk lists, five to a column (x 58-135 and 142-220, 6 pixels each from y = 32) */
    GameSession *s = ui->session;
    if (!ui->talkListShown) {
        ui->talkListShown = true;
        return true;
    }
    for (unsigned i = 0; i < s->talk.listedCount && i < ConversationListMax; i++) {
        int x0 = i < 5 ? 58 : 142, y0 = 32 + (int)(i % 5) * 6;
        if (inRect(x, y, x0, y0, x0 + 77, y0 + 5)) {
            talkChoose(ui, i + 1);
            return true;
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

/* The alchemy screen's click regions (the table at DS:0x67AC): the 13 rows, 14 the close button, 15 / 16 the arrows; a portrait (the party panel table's regions 1, 11, 21, 31) names a member. */
static bool alchemyClick(GameUi *ui, int x, int y) {
    GameKind game = ui->session->game;
    unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y);
    int member = panel && panel % 10 == 1 && panel / 10 < 4 ? (int)(panel / 10) : -1;
    if (ui->alchemyMode == UiAlchemyTarget) {
        if (member >= 0 && sessionPartyRecord(ui->session, (unsigned)member)) {
            alchemyFinish(ui, ui->alchemySpell, false, member);
        }
        return true;
    }
    if (ui->alchemyMode == UiAlchemyMark) {
        unsigned row = uiRegionHit(game, UiRegionsConfirmList, x, y);
        ConfirmPrompt prompt;
        unsigned mark = 5, back = 7; /* the answers are the rows of the prompt's first and last region (Chapter 3 asks at 4 and 6) */
        if (confirmPromptLoad(ui->exe, game, 0x22, &prompt)) {
            mark = prompt.firstRegion;
            back = prompt.lastRegion;
        }
        if (row && (row == mark || row == back)) {
            alchemyMark(ui, row == back);
        }
        return true;
    }
    unsigned region = uiRegionHit(game, UiRegionsAlchemy, x, y);
    if (region == 14) {
        alchemyClose(ui);
    } else if (region == 15 || region == 16) {
        alchemyPage(ui, region == 16);
    } else if (region >= 1 && region <= AlchemyListRows) {
        unsigned index = ui->alchemySelected / AlchemyListRows * AlchemyListRows + region - 1;
        if (index < ui->alchemyCount) {
            if (index == ui->alchemySelected) {
                alchemyCast(ui);
            } else {
                ui->alchemySelected = index;
            }
        }
    } else if (member >= 0) {
        alchemyChooseMember(ui, (unsigned)member);
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
    if (region == 2) { /* the top-right panel: the plate or the minimap, then the resource panel */
        if (!s->combat.active) {
            panelClick(ui, x, y);
        }
        return true;
    }
    if (region == 3) { /* the icon row: act (attack in a combat), alchemy, rest, the pause dialog */
        unsigned icon = uiRegionHit(game, UiRegionsDungeonIconRow, x, y);
        return icon == 1 ? press(ui, s->combat.active ? 'a' : 's') : icon == 2 ? press(ui, 'c') : icon == 3 ? press(ui, 'r') : icon == 4 ? press(ui, 'd') : true;
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

static bool clickDispatch(GameUi *ui, int x, int y, bool right) {
    ui->session->loot.pending = false; /* the loot panel stays until the next command */
    if (ui->passwordActive) { /* EditTextField takes no clicks */
        return true;
    }
    if (ui->targeting) { /* a left click on the view is the target; the right button and every other place are ignored */
        if (!right && uiRegionHit(ui->session->game, UiRegionsDungeonMain, x, y) == 1) {
            unlockTry(ui);
        }
        return true;
    }
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
    case UiOverlayAlchemy:
        return alchemyClick(ui, x, y);
    case UiOverlayLocalMap:
    case UiOverlayWorldMap:
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

bool gameUiClick(GameUi *ui, int x, int y, bool right) {
    if (ui->restPause) { /* the interrupted rest's message is read: any click lets the combat show */
        ui->restPause = false;
        ui->panelRunCount = 0;
        return true;
    }
    ui->panelRunCount = 0;
    bool running = clickDispatch(ui, x, y, right);
    restReport(ui);
    lockReport(ui);
    return running;
}
