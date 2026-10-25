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
#include "partyinfo.h"
#include "consumable.h"
#include "pictures.h"
#include "roster.h"
#include "spellcast.h"
#include "statsheet.h"
#include "statuspanel.h"
#include "textpanel.h"
#include "titlemenu.h"
#include "windowbake.h"
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
/*
 * A row of a document in the reader's skill (DrawIndentedTextColumn, yendor2.asm:44453; DrawWordToken draws a word and the spaces after it): plain text and a reader who knows everything see the normal
 * font; a reader below the first threshold sees every character in the document's foreign font (offset / 2: 1, 3 or 2), and in between the first 1, 2 or 4 words of every five -- counted afresh in each
 * row -- are in the normal font and the others foreign.
 */
static void drawReadRow(GameUi *ui, int x, int y, const char *row, const DocumentReadability *r) {
    const GameSession *s = ui->session;
    unsigned foreign = r->fontOffset / 2;
    unsigned word = 0;
    for (const char *p = row; *p;) {
        unsigned font = r->plain || r->clearWordsOfFive >= 5 ? 0 : r->clearWordsOfFive == 0 ? foreign : (word % 5) < r->clearWordsOfFive ? 0 : foreign;
        for (; *p && *p != ' '; p++) {
            x = fontDrawChar(s->game, font, ui->screen, ViewScreenWidth, x, y, (unsigned char)*p, 0x0D, 0, FontTransparent);
        }
        for (; *p == ' '; p++) {
            x += 6;
        }
        word++;
    }
}

static const char *materialLabel(const GameUi *ui, unsigned index);

static void drawTalk(GameUi *ui) {
    GameSession *s = ui->session;
    const ViewRenderer *r = &ui->renderer;
    viewDrawPicture(r, 1, 3, 15, 23, true, 0);
    if (s->talk.npc && !ui->reading) {
        viewDrawPicture(r, 7, dialogGetU16(s->talk.npc, 0), 21, 31, true, 0);
    }
    for (unsigned i = 0; i < ui->talkRowCount; i++) {
        if (ui->reading) {
            drawReadRow(ui, 22, 67 + 6 * (int)i, ui->talkRows[i], &s->readability);
        } else if (s->talk.npc && conversationNeedsSpeaker(s->talk.npc)) { /* an NPC with a foreign tongue: the translator reads as much as his or her Linguistics allows */
            DocumentReadability readable = sessionTalkReadability(s);
            drawReadRow(ui, 22, 67 + 6 * (int)i, ui->talkRows[i], &readable);
        } else {
            fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 22, 67 + 6 * (int)i, ui->talkRows[i], 0x0D, 0, FontTransparent);
        }
    }
    for (unsigned i = 0; i < ui->talkExtraCount; i++) {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, ui->talkExtra[i].x, ui->talkExtra[i].y, ui->talkExtra[i].text, ui->talkExtra[i].colour, 0, FontTransparent);
    }
    if (ui->talkQuote) { /* DrawConfirmPromptGoldLine: GOLD COINS: and what the party has, at (22, 115) */
        char gold[16];
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 22, 115, materialLabel(ui, 0), 0x0D, 0, FontTransparent);
        bcd4Format(saveHeaderBcd4(&s->save, SaveHeaderGold), gold);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 94, 115, gold, 0x8A, 0, FontTransparent);
    }
    if (sessionTalkHasSelector(s) && !ui->reading) { /* the member the service is for: the frame round the face (DrawPartyStatusIconRow) */
        static const int kX[4] = {8, 0x42, 0x7C, 0xB6};
        if (s->talkSpeaker < 4) {
            viewDrawPicture(r, 7, 0, kX[s->talkSpeaker], 0x94, true, 0);
        }
    }
    for (unsigned i = 0; !ui->reading && ui->talkListShown && i < s->talk.listedCount && i < ConversationListMax; i++) {
        char name[DialogTopicNameSize + 1];
        dialogTopicName(dialogTopic(&s->dialogs, s->talk.listed[i]), name);
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, i < 5 ? 58 : 142, 32 + 6 * (int)(i % 5), name, 0x8A, 0, FontTransparent);
    }
}

/* The stock of the shop a "buy" topic opened: number, name and price; the gold; the item on the cursor. */
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
        if (locationName(game, ui->worldDat, ui->worldSize, id - 1, game == GameYendor2 ? " LEVEL X" : " LEVEL XXX", game == GameYendor2 ? " MAP X" : " MAP XXX", &name)) { /* every id the list names has a name: index 0 is a name too (PORT HOPE, the starting map) */
            snprintf(out, ClueNameSize, "%s%s", name.name, name.suffix);
        }
    } else if (category == 2) {
        uint8_t record[MonsterRecordSize];
        if (monsterRecordFromBlock(record, &ui->session->monsters, id)) { /* the master list's ids are catalog block numbers */
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

/*
 * PlayClueBookOpenAnimation's fades (yendor2.asm:8293), on the palette of block 5 with the screen black at first: the colours 0-0xAE ("DARK UNION") fade in (StepPaletteFadeRange mode 1, 63 rounds, about 0.9 s),
 * 30 BIOS ticks go by, the colours 0xB0-0xBE (the words "The On-Line Clue Book") fade in, 35 ticks, and the whole palette fades out. Counted in fade ticks (27.5 ms each; ClueFadeTicks of them make a fade):
 * the title in, held ClueHoldTitle, the words in, held ClueHoldWords, everything out.
 */
enum {
    ClueFadeTicks = 16,                                  /* a fade is 16 steps of the palette in about 0.44 s: a fade tick is 27.5 ms (gameUiFadeStep) */
    ClueHoldTitle = 60,                                  /* 30 BIOS ticks (1.65 s) with the title alone */
    ClueHoldWords = 70,                                  /* 35 BIOS ticks (1.9 s) with the words too */
    ClueSplashTicks = ClueFadeTicks + ClueHoldTitle + ClueFadeTicks + ClueHoldWords + ClueFadeTicks,
    ClueTitleColours = 0xAF, ClueWordsFirst = 0xB0, ClueWordsLast = 0xBE
};

static void clueSplashPalette(GameUi *ui) {
    uint32_t block = paletteBlockOffset(ui->session->game, 5);
    uint8_t out[PaletteBytes], full[PaletteBytes];
    unsigned elapsed = ClueSplashTicks - ui->clueSplash;
    if (block + PaletteBytes > ui->worldSize) {
        return;
    }
    const uint8_t *target = ui->worldDat + block;
    memset(out, 0, sizeof(out));
    memcpy(full, target, sizeof(full));
    const unsigned wordsAt = ClueFadeTicks + ClueHoldTitle, outAt = wordsAt + ClueFadeTicks + ClueHoldWords;
    if (elapsed < ClueFadeTicks) { /* the title comes in */
        paletteFadeInFrame(target, (elapsed + 1) * FadeRounds / ClueFadeTicks, full);
        memcpy(out, full, ClueTitleColours * 3);
    } else {
        memcpy(out, target, ClueTitleColours * 3);
        if (elapsed >= wordsAt) { /* the words come in */
            unsigned round = elapsed >= wordsAt + ClueFadeTicks ? FadeRounds : (elapsed - wordsAt + 1) * FadeRounds / ClueFadeTicks;
            paletteFadeInFrame(target, round, full);
            memcpy(out + ClueWordsFirst * 3, full + ClueWordsFirst * 3, (ClueWordsLast - ClueWordsFirst + 1) * 3);
        }
        if (elapsed >= outAt) { /* everything fades out, a step of the six-bit values a round */
            unsigned round = (elapsed - outAt + 1) * FadeRounds / ClueFadeTicks;
            for (unsigned i = 0; i < PaletteBytes; i++) {
                out[i] = out[i] > round ? (uint8_t)(out[i] - round) : 0;
            }
        }
    }
    memcpy(ui->palette, out, sizeof(out));
    ui->paletteStamp++;
}

/*
 * The whole-screen fades round the book (TriggerFullPaletteFadeOut / TriggerFullPaletteFadeIn, 63 rounds each, about 0.9 s = ClueFadeTicks animation ticks): ShowClueBook fades the game out, shows the picture (the
 * first time) and fades the help screen in; leaving it fades the book out and the game in. The picture that fades out is the last one drawn (`clueFrozen`); `clueBase` is the game's palette.
 */
typedef enum { ClueStageNone, ClueStageOutGame, ClueStageInHelp, ClueStageOutBook, ClueStageInGame } ClueStage;

static void clueFadeFrame(GameUi *ui, bool in, unsigned tick) {
    unsigned round = (tick + 1) * FadeRounds / ClueFadeTicks;
    if (in) {
        paletteFadeInFrame(ui->clueSavedPalette, round, ui->palette);
    } else {
        for (unsigned i = 0; i < PaletteBytes; i++) {
            ui->palette[i] = ui->clueSavedPalette[i] > round ? (uint8_t)(ui->clueSavedPalette[i] - round) : 0;
        }
    }
    ui->paletteStamp++;
}

static void clueStageStart(GameUi *ui, ClueStage stage) {
    ui->clueStage = stage;
    ui->clueStageTick = 0;
    if (stage == ClueStageInHelp || stage == ClueStageInGame) { /* the screen is black until the first round */
        memset(ui->palette, 0, PaletteBytes);
        ui->paletteStamp++;
    }
}

/* One animation tick of a fade; at its end the next part begins. */
static void clueStageTick(GameUi *ui) {
    ClueStage stage = (ClueStage)ui->clueStage;
    clueFadeFrame(ui, stage == ClueStageInHelp || stage == ClueStageInGame, ui->clueStageTick);
    if (++ui->clueStageTick < ClueFadeTicks) {
        return;
    }
    ui->clueStage = ClueStageNone;
    switch (stage) {
    case ClueStageOutGame:
        if (ui->clueSplash) { /* the picture: the screen is black and its first colours are about to come in */
            clueSplashPalette(ui);
        } else {
            clueStageStart(ui, ClueStageInHelp);
        }
        break;
    case ClueStageOutBook:
        gameUiAmbientMusic(ui);
        clueStageStart(ui, ClueStageInGame);
        break;
    default:
        memcpy(ui->palette, ui->clueSavedPalette, PaletteBytes);
        ui->paletteStamp++;
        break;
    }
}

static void clueSettle(GameUi *ui) { /* the fades are over (a key does not wait for them) */
    while (ui->clueStage) {
        clueStageTick(ui);
    }
}

/* ShowClueBook: the music of track 21, PlayClueBookOpenAnimation (once: category 0 picture 12 on the palette of block 5) and ShowClueBookHelpScreen -- the list of the F keys -- which stays until one is pressed. */
static void clueOpen(GameUi *ui) {
    clueSettle(ui);
    memcpy(ui->clueFrozen, ui->cursorBase, sizeof(ui->clueFrozen));
    memcpy(ui->clueSavedPalette, ui->palette, PaletteBytes);
    ui->overlay = UiOverlayClue;
    ui->clueHelp = true;
    ui->clueNag = false;
    ui->clueScreen = 0;
    ui->cluePage = 1;
    if (ui->host.music) {
        ui->host.music(ui->host.ctx, 21);
    }
    if (!ui->clueSeen && paletteBlockOffset(ui->session->game, 5) + PaletteBytes <= ui->worldSize) {
        ui->clueSplash = ClueSplashTicks;
    }
    ui->clueSeen = true;
    clueStageStart(ui, ClueStageOutGame);
}

static void clueSplashEnd(GameUi *ui) {
    if (ui->clueSplash) {
        ui->clueSplash = 0;
        clueStageStart(ui, ClueStageInHelp); /* the help screen fades in on the black the picture left */
    }
}

static void clueClose(GameUi *ui) {
    if (ui->clueSplash) {
        ui->clueSplash = 0;
        memcpy(ui->palette, ui->clueSavedPalette, PaletteBytes);
        ui->paletteStamp++;
    }
    memcpy(ui->clueFrozen, ui->cursorBase, sizeof(ui->clueFrozen));
    ui->overlay = UiOverlayNone;
    clueStageStart(ui, ClueStageOutBook);
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
        if (category == 2 && monsterRecordFromBlock(ui->clueMonster, &ui->session->monsters, id)) {
            clueMonsterAnimStart(ui->clueMonster, &ui->clueAnim);
        }
    }
}

/* The sounds a session step asked for (pendingSound), played now. */
/* The sounds waiting their turn: one starts when the host says the last is over (TriggerSoundEventAfterDriverWait); a host that cannot tell starts them all. */
static void soundPump(GameUi *ui) {
    while (ui->soundFifoCount) {
        if (ui->host.soundBusy && ui->host.soundBusy(ui->host.ctx)) {
            return;
        }
        effect(ui, ui->soundFifo[0]);
        memmove(ui->soundFifo, ui->soundFifo + 1, (ui->soundFifoCount - 1) * sizeof(ui->soundFifo[0]));
        ui->soundFifoCount--;
        if (ui->host.soundBusy) {
            return;
        }
    }
}

static void playPending(GameUi *ui) {
    GameSession *s = ui->session;
    unsigned id = s->pendingSound;
    s->pendingSound = 0;
    do {
        if (id && ui->soundFifoCount < sizeof(ui->soundFifo) / sizeof(ui->soundFifo[0])) {
            ui->soundFifo[ui->soundFifoCount++] = id;
        }
        id = sessionNextSound(s);
    } while (id);
    soundPump(ui);
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
    unsigned active = sessionCombatTarget(s);
    unsigned tier = monsterRevealTier(s);
    textPanelClear(&ui->renderer, true);
    for (unsigned i = 0; i < MonsterPanelCount && i < CombatMonsterSlotCount; i++) {
        uint8_t *monster = c->slots + (size_t)i * MonsterRecordSize;
        if (monsterGetU16(monster, MonsterFieldType)) {
            monsterPanelDraw(&ui->renderer, i, monster, i == active, tier);
        }
    }
}

/* The three counters' labels (DS:0x7C55 / 0x7F87): Chapter 2 gold coins, magic ore, nuore; Chapter 3 gold coins, FOOD, nuore. */
static const char *materialLabel(const GameUi *ui, unsigned index) {
    static const char *const kYendor2[3] = {"GOLD COINS:", "MAGIC ORE: ", "NUORE: "};
    static const char *const kYendor3[3] = {"GOLD COINS:", "FOOD: ", "NUORE: "};
    return (ui->session->game == GameYendor2 ? kYendor2 : kYendor3)[index % 3];
}

/* ShowLootAndAwardExperience's panel: GOLD COINS always, MAGIC ORE and NUORE when there are any, EXPERIENCE lower down. */
static void lootDraw(GameUi *ui) {
    const ViewRenderer *r = &ui->renderer;
    const SessionLoot *loot = &ui->session->loot;
    static const uint8_t zero[4] = {0, 0, 0, 0};
    char number[12];
    int y = 96;
    textPanelClear(r, false);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, materialLabel(ui, 0), 0x8A, 4, FontOpaque);
    bcd4Format(loot->gold, number);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, number, 0x0F, 4, FontOpaque);
    y += 18;
    if (memcmp(loot->ore, zero, 4) != 0) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, materialLabel(ui, 1), 0x8A, 4, FontOpaque);
        bcd4Format(loot->ore, number);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, number, 0x0F, 4, FontOpaque);
        y += 12;
    }
    if (memcmp(loot->nuore, zero, 4) != 0) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, materialLabel(ui, 2), 0x8A, 4, FontOpaque);
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

/*
 * A fight starts with the resource panel in the top-right corner (RunDungeonGameLoop :16324: the panel bits 0x7000 are kept, ShowResourceDepletedOverlay clears 0x6000 and sets 0x1000), so the belt
 * can be worked and its items used while it lasts; the victory puts the bits back (`& 0xEFFF`, then the kept ones).
 */
static void combatPanelSync(GameUi *ui) {
    GameSession *s = ui->session;
    if (s->combat.active && !ui->combatPanel) {
        uint16_t flags = panelFlags(ui);
        ui->combatPanel = true;
        ui->combatPanelSaved = (uint16_t)(flags & 0x7000);
        panelEnterResources(ui, flags);
    } else if (!s->combat.active && ui->combatPanel) {
        ui->combatPanel = false;
        panelSet(ui, (uint16_t)((panelFlags(ui) & 0xEFFF) | ui->combatPanelSaved));
    }
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
    const Bcd4 *amounts[3] = {(const Bcd4 *)saveHeaderBcd4(save, SaveHeaderGold), (const Bcd4 *)saveHeaderBcd4(save, SaveHeaderOreCounter1), (const Bcd4 *)saveHeaderBcd4(save, SaveHeaderOreCounter2)};
    char number[12];
    resourceIconsDraw(ui);
    if (ui->panelRunCount) { /* a message or an item's name has the text area */
        return;
    }
    textPanelClear(r, false);
    for (unsigned i = 0; i < 3; i++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 96 + 18 * (int)i, materialLabel(ui, i), 0x8A, 4, FontOpaque);
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

static void examineStart(GameUi *ui, unsigned itemId);

/* A belt item of the resource panel under (x, y): the id, else 0. */
static unsigned beltItemAt(GameUi *ui, int x, int y) {
    unsigned icon = uiRegionHit(ui->session->game, UiRegionsDungeonMain, x, y) == 2 && (panelFlags(ui) & MapFlagHidden) ? uiRegionHit(ui->session->game, UiRegionsStatusIconBar, x, y) : 0;
    return icon >= 4 && icon <= 9 ? sessionSlotItem(ui->session, 0, SessionBeltSlotBase + icon - 4) : 0;
}

bool gameUiWantsDoubleClick(GameUi *ui, int x, int y) {
    return ui->overlay == UiOverlayNone && !ui->pickActive && !ui->session->combat.active && beltItemAt(ui, x, y) != 0;
}

bool gameUiDoubleClick(GameUi *ui, int x, int y) {
    if (!gameUiWantsDoubleClick(ui, x, y)) {
        return false;
    }
    examineStart(ui, beltItemAt(ui, x, y));
    return true;
}

/* ---- the cursor ---- */

void gameUiMouseMove(GameUi *ui, int x, int y) {
    ui->mouseX = x;
    ui->mouseY = y;
}

bool gameUiCursorShown(const GameUi *ui) {
    if (ui->dropPrompt) { /* the cursor is empty while the question is asked: the system pointer answers it */
        return false;
    }
    return ui->targeting || ui->pickActive || ui->session->heldItem != 0;
}

/* The cursor item or the crosshair, over everything else (DrawMouseCursor: the 16 x 16 sprite blitted at the pointer with 0xFF transparent). */
static void cursorDraw(GameUi *ui) {
    unsigned picture = 0;
    if (ui->pickActive) {
        viewDrawPicture(&ui->renderer, 8, 0x0F, ui->mouseX, ui->mouseY, true, 0);
        return;
    }
    if (ui->dropPrompt) { /* the cursor is empty while the question is asked */
        return;
    }
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

/* ---- the party panels and the cursor item ---- */

/* The list a click on a member's icons writes into the text area (partyinfo.h): the runs replace whatever the area held. */
static void partyInfoShow(GameUi *ui, unsigned member, int kind) {
    GameSession *s = ui->session;
    const uint8_t *record = sessionPartyRecord(s, member);
    PanelRun runs[PartyInfoMaxRuns];
    unsigned count = 0;
    if (!record) {
        return;
    }
    switch (kind) {
    case 0:
        count = partyInfoAfflictions(record, runs, PartyInfoMaxRuns);
        break;
    case 1:
        count = partyInfoTraining(record, runs, PartyInfoMaxRuns);
        break;
    case 2:
        count = partyInfoProtections(record, runs, PartyInfoMaxRuns);
        break;
    case 3:
        count = partyInfoTransports(record, s->clock.minutes, runs, PartyInfoMaxRuns);
        break;
    case 4:
        count = partyInfoStatBars(record, runs, PartyInfoMaxRuns);
        break;
    default:
        break;
    }
    ui->panelRunCount = 0;
    for (unsigned i = 0; i < count && i < UiPanelRunMax; i++) {
        ui->panelRuns[ui->panelRunCount++] = runs[i];
    }
    if (kind == 3 && count > 0) { /* g_uiScratchFlags3 bit 0x1000: the list's lines 2-5 are the member's mounts, a click on one rides it */
        ui->mountList = member + 1;
    }
}

/*
 * RevealMapRegion (yendor2.asm:44801): the mount chosen from the TRANSPORTS list. The view area becomes a box of the explored map around the party (a blank tile, then each explored cell's tile as on
 * the local map, the compass arrow in the middle), its size by the party's Navigation; the message area names the mount and the charges that remain. A click on a cell flies there (the verdict
 * decides: a refusal is the warning, a trap or trigger on the cell "THAT IS AN ILLUSION"), Escape gives up.
 */
static void mountStart(GameUi *ui, unsigned member, unsigned ability) {
    GameSession *s = ui->session;
    SessionMountInfo info;
    if (sessionMountBegin(s, member, ability, &info) != MountCheckOk) {
        return;
    }
    ui->mountMember = member;
    ui->mountSlot = info.slot;
    ui->mountRemaining = info.remaining;
    ui->mountName = info.name;
    ui->mountBox = mountRevealBox(partyAverageStatTiers(&s->save).navigation);
    mountBoxOrigin(&ui->mountBox, s->x, s->y, &ui->mountOriginX, &ui->mountOriginY);
    ui->panelRunCount = 0;
    ui->overlay = UiOverlayMount;
}

static void mountDraw(GameUi *ui) {
    GameSession *s = ui->session;
    const ViewRenderer *r = &ui->renderer;
    for (unsigned row = 0; row < 17; row++) {
        for (unsigned col = 0; col < 28; col++) {
            viewDrawPicture(r, LocalMapCategory, MinimapBlankTile, 8 + 8 * (int)col, 8 + 8 * (int)row, false, 0);
        }
    }
    const MountBox *box = &ui->mountBox;
    unsigned originPage = (unsigned)(s->y / LocalMapRows) * LocalMapBlocksPerRow + (unsigned)(s->x / LocalMapColumns);
    uint8_t originAttr = mountPageAttribute(s->game, ui->worldDat, ui->worldSize, originPage);
    for (unsigned row = 0; row < box->rows; row++) {
        for (unsigned col = 0; col < box->columns; col++) {
            int cx = ui->mountOriginX + (int)col, cy = ui->mountOriginY + (int)row;
            if (cx < 0 || cy < 0 || cx >= WorldMapColumns || cy >= (int)s->map.rowCount || !exploreIsExplored(&s->save, cx, cy)) {
                continue;
            }
            unsigned page = (unsigned)(cy / LocalMapRows) * LocalMapBlocksPerRow + (unsigned)(cx / LocalMapColumns);
            if (!mountCellVisible(page == originPage, originAttr, mountPageAttribute(s->game, ui->worldDat, ui->worldSize, page))) {
                continue;
            }
            int px = box->pixelX + 8 * (int)col, py = box->pixelY + 8 * (int)row;
            uint16_t picture, overlay;
            if (worldMapWallPictureOffset(s->game, worldMapTileA(&s->map, (unsigned)cy, (unsigned)cx), &picture)) {
                viewDrawPicture(r, LocalMapCategory, picture, px, py, false, 0);
            }
            if (worldMapFloorPictureOffset(s->game, worldMapTileB(&s->map, (unsigned)cy, (unsigned)cx), &overlay) && overlay) {
                viewDrawPicture(r, LocalMapCategory, overlay, px, py, true, 0);
            }
        }
    }
    unsigned arrow = (s->facing & SaveFacingNorth) ? 0 : (s->facing & SaveFacingSouth) ? 2 : (s->facing & SaveFacingEast) ? 1 : 3; /* DrawRevealMapDirectionIcon */
    viewDrawPicture(r, LocalMapCategory, arrow, 0x74, 0x48, true, 0);
    textPanelClear(r, true);
    char line[40];
    fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 241, 87, ui->mountName, 0x8A, 0, FontTransparent);
    snprintf(line, sizeof(line), "%u REMAINING", ui->mountRemaining);
    fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 241, 96, line, 0x0F, 0, FontTransparent);
    fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 241, 102, "USES TODAY.", 0x0F, 0, FontTransparent);
    panelRunsDraw(ui);
}

static void mountClick(GameUi *ui, int x, int y) {
    GameSession *s = ui->session;
    int cx, cy;
    if (!mountCellFromClick(&ui->mountBox, ui->mountOriginX, ui->mountOriginY, x, y, &cx, &cy)) {
        flashWarning(ui);
        return;
    }
    MountTravelResult verdict = sessionMountFly(s, ui->mountMember, ui->mountSlot, cx, cy, ui->worldDat, ui->worldSize);
    playPending(ui);
    if (verdict == MountTravelOk) {
        ui->overlay = UiOverlayNone;
        gameUiAmbientMusic(ui);
    } else if (verdict == MountTravelInteractive) { /* THAT IS / AN ILLUSION. in 0x5B at (240, 114) */
        panelRun(ui, 240, 114, 0x5B, true, "THAT IS AN");
        panelRun(ui, 240, 120, 0x5B, true, "ILLUSION.");
    } else {
        flashWarning(ui);
    }
}

/* HandleItemDropOnPartyPortrait: the cursor item dropped on a member's face is worn if it has a slot and the slot is free, else stowed in the first free bag slot, if the carry weight allows (sound 6); else the beep. */
static void dropOnPortrait(GameUi *ui, unsigned member) {
    if (sessionDropOnPortrait(ui->session, member)) {
        effect(ui, 6);
        ui->panelRunCount = 0;
    } else {
        flashWarning(ui);
    }
}

/* TryDropHeldItem: an item that may not be dropped beeps; any other asks first. */
static void dropStart(GameUi *ui) {
    if (!ui->session->heldItem) {
        return;
    }
    if (!sessionHeldDroppable(ui->session)) {
        flashWarning(ui);
        return;
    }
    ui->dropPrompt = true;
}

static void dropAnswer(GameUi *ui, bool yes) {
    ui->dropPrompt = false;
    if (yes) {
        sessionDropHeld(ui->session);
    }
}

static void dropPromptDraw(GameUi *ui) {
    ConfirmPrompt prompt;
    const ViewRenderer *r = &ui->renderer;
    textPanelClear(r, false);
    if (confirmPromptLoad(ui->exe, ui->session->game, 4, &prompt)) {
        confirmPromptDraw(r, &prompt);
    } else {
        static const char *const kAsk[3] = {"DO YOU WANT", "TO DROP THAT", "RIGHT NOW?"};
        for (unsigned i = 0; i < 3; i++) {
            fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 240, 96 + 6 * (int)i, kAsk[i], 0x8A, 4, FontTransparent);
        }
        fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 240, 120, "    YES", 0xAA, 4, FontTransparent);
        fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 240, 132, "    NO", 0x5B, 4, FontTransparent);
    }
}

/* ---- a chest's contents (RunShopScreen) ---- */

/*
 * The screen of a chest or a merchant's stock: the walk pad's place at the lower right (the 72 x 35 pixels from (241, 160)) is blanked and the eight slots -- two rows of four, UiRegionsCatalogSlots --
 * show the icons of what is left (DrawShopItemSlotGrid, :12161), "EMPTY" at (259, 179) when nothing is. The rest of the main screen stays: the view, the panels, the party.
 */
/* The item ids the eight slots hold now (a chest's untaken contents, a merchant's whole stock). */
static void catalogIds(GameUi *ui, unsigned ids[LockContentSlots]) {
    GameSession *s = ui->session;
    memset(ids, 0, sizeof(unsigned) * LockContentSlots);
    if (ui->catalogShop) {
        SessionShopItem stock[LockContentSlots];
        sessionShopStock(s, ui->catalogLock, stock);
        for (unsigned i = 0; i < LockContentSlots; i++) {
            ids[i] = stock[i].itemId;
        }
    } else {
        ChestSlot slots[LockContentSlots];
        sessionChestSlots(s, ui->catalogLock, slots);
        for (unsigned i = 0; i < LockContentSlots; i++) {
            ids[i] = slots[i].itemId;
        }
    }
}

static void catalogDraw(GameUi *ui) {
    GameSession *s = ui->session;
    unsigned count;
    const uint16_t(*cells)[5] = uiRegionEntries(s->game, UiRegionsCatalogSlots, &count);
    unsigned ids[LockContentSlots];
    catalogIds(ui, ids);
    for (int row = 0; row < 35; row++) {
        memset(&ui->screen[(160 + row) * ViewScreenWidth + 241], 4, 72);
    }
    if (ui->catalogShop) { /* ShowMaterialCounterHud: "$" and the gold in the header */
        textPanelGold(&ui->renderer, saveHeaderBcd4(&s->save, SaveHeaderGold));
    }
    unsigned shown = 0;
    for (unsigned i = 0; cells && i < count && i < LockContentSlots; i++) {
        const uint8_t *item = ids[i] ? itemCatalogRecord(&s->items, ids[i]) : NULL;
        if (item) {
            viewDrawPicture(&ui->renderer, 8, item[ItemFieldIcon] | (item[ItemFieldIcon + 1] << 8), cells[i][0], cells[i][2], true, 0);
            shown++;
        }
    }
    if (!shown) {
        char empty[16];
        if (!ui->exe || !exeDataString(ui->exe, s->game == GameYendor2 ? 0x7D26 : 0x8053, empty, sizeof(empty))) {
            snprintf(empty, sizeof(empty), "EMPTY");
        }
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 259, 179, empty, 0x0F, 4, FontOpaque);
    }
}

static void catalogOpen(GameUi *ui, unsigned lockId, bool shop) {
    ui->catalogActive = true;
    ui->catalogLock = lockId;
    ui->catalogShop = shop;
    ui->panelRunCount = 0;
}

static void talkChoose(GameUi *ui, unsigned number);

static void catalogLeave(GameUi *ui) { /* TriggerShopExitSoundAndPersist: sound 6 (a chest that is not inexhaustible), the screen gives the pad back */
    ui->catalogActive = false;
    ui->panelRunCount = 0;
    if (!ui->catalogShop) {
        effect(ui, 6);
    }
    if (ui->shopFromTalk) { /* UseKeyItem's / CheckKeyItem's tail: the panel mode comes back, and the first topic listed runs (DONE, which ends it) */
        ui->shopFromTalk = false;
        saveHeaderSetU16(&ui->session->save, MapFlagsOffset, (uint16_t)((saveHeaderGetU16(&ui->session->save, MapFlagsOffset) & 0xEFFF) | ui->shopPanelWas));
        ui->overlay = UiOverlayTalk;
        if (ui->session->talking && ui->session->talk.listedCount) {
            talkChoose(ui, 1);
        }
    }
}

/* UseKeyItem: the resource panel is shown (the mode it found is kept), then the stock. */
static void shopFromTalkOpen(GameUi *ui, bool shop) {
    ui->shopFromTalk = true;
    ui->shopPanelWas = (uint16_t)(panelFlags(ui) & PanelModeBits);
    panelEnterResources(ui, panelFlags(ui));
    catalogOpen(ui, ui->shopLock, shop);
}

static void inventoryOpen(GameUi *ui, unsigned mask);
static unsigned inventoryAllMask(const GameUi *ui);

/* RunSellItemScreen: the resource panel is shown (its mode kept), the inventory of every member opens with the sell text, and the cursor sells. */
static void sellOpen(GameUi *ui) {
    ui->sellMode = true;
    ui->sellPanelWas = (uint16_t)(panelFlags(ui) & PanelModeBits);
    panelEnterResources(ui, panelFlags(ui));
    ui->session->sellUndoValid = false;
    inventoryOpen(ui, inventoryAllMask(ui));
}

/* The sell text at (241, 173): SPACEBAR TO / SELL ITEM OR / ESC TO UNDO (Chapter 2 DS:0x7FD0, Chapter 3 0x82FD), in 0x7B on the panel's colour 4. */
static void sellTextDraw(GameUi *ui) {
    GameSession *s = ui->session;
    char line[16];
    unsigned offset = s->game == GameYendor2 ? 0x7FD0 : 0x82FD;
    for (int row = 0; row < 35; row++) { /* ClearPortraitPanelAreas: the walk pad's place is blank (the text sits on colour 4) */
        memset(&ui->screen[(160 + row) * ViewScreenWidth + 241], 4, 72);
    }
    for (unsigned i = 0; i < 3 && ui->exe && exeDataString(ui->exe, offset, line, sizeof(line)); i++) {
        fontDrawString(s->game, 0, ui->screen, ViewScreenWidth, 241, 173 + 6 * (int)i, line, 0x7B, 4, FontOpaque);
        offset += (unsigned)strlen(line) + 1;
    }
    textPanelGold(&ui->renderer, saveHeaderBcd4(&s->save, SaveHeaderGold));
}

/* TrySellItemForGold's refusal: "I HAVE NO / NEED FOR / THAT TYPE OF / ITEM." (DS:0x7FF7, Chapter 3 0x8324) in white at (240, 96). */
static void sellRefused(GameUi *ui) {
    ui->panelRunCount = 0;
    panelMessageExe(ui, ui->session->game == GameYendor2 ? 0x7FF7 : 0x8324, 4);
    for (unsigned i = 0; i < ui->panelRunCount; i++) {
        ui->panelRuns[i].colour = 0x0F;
        ui->panelRuns[i].opaque = true;
    }
}

/* UseItem -> ConfirmAndValidatePartyTarget -> UseKeyItem: WHO WILL BARTER? when nobody does yet (giving it up leaves the dialog as it is), then the merchant's screen. */
static void barterStart(GameUi *ui) {
    unsigned barterer;
    ui->panelRunCount = 0;
    if (sessionBarterer(ui->session, &barterer)) {
        shopFromTalkOpen(ui, true);
    } else {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 7;
        ui->pickPrompt = 10;
    }
}

/* HandleShopCatalogSlotClick: a pile shows what it added ("GOLD COINS:" over the amount), an item shows its name on the cursor. */
/*
 * What ShowItemPurchaseConfirmPrompt (yendor2.asm:31180) writes under a merchant's item name, whatever the party's skill (outside a shop the examiner's Repair skill gates the same lines at 65 / 80 / 95: examineShow): WEIGHT:
 * and the weight in tenths at y = 120 / 126, then DAMAGE: for a weapon (item flags 0xC000) or ABSORPTION: for armour and rings (0xE00) with the entry's first word at y = 132 / 138; the VALUE follows at 144.
 */
static void itemLines(GameUi *ui, unsigned itemId, bool weight, bool effect) {
    const ItemCatalog *items = &ui->session->items;
    const uint8_t *item = itemCatalogRecord(items, itemId);
    if (!item) {
        return;
    }
    char text[12];
    if (weight) {
        panelRun(ui, 240, 120, 0x8A, true, "WEIGHT:");
        partyInfoFormatTenths(itemGetU16(item, ItemFieldWeight), text);
        panelRun(ui, 240, 126, 0x0F, true, text);
    }
    unsigned flags = itemGetU16(item, ItemFieldFlags);
    const uint8_t *entry = itemTargetEntry(items, item);
    if (effect && entry && (flags & 0xCE00)) {
        panelRun(ui, 240, 132, 0x8A, true, (flags & 0xC000) ? "DAMAGE:" : "ABSORPTION:");
        snprintf(text, sizeof(text), "%u", itemTargetWord(entry, 0));
        panelRun(ui, 240, 138, 0x0F, true, text);
    }
}

static void merchantLines(GameUi *ui, unsigned itemId) {
    itemLines(ui, itemId, true, true);
}

static void catalogSlotClick(GameUi *ui, unsigned slot) {
    GameSession *s = ui->session;
    if (ui->catalogShop) { /* a merchant: an item is looked at and, if the gold allows, bought; with one held, sold back */
        SessionShopItem stock[LockContentSlots];
        sessionShopStock(s, ui->catalogLock, stock);
        ui->panelRunCount = 0;
        if (s->heldItem) {
            if (sessionShopSell(s, ui->catalogLock) != SessionShopSold) {
                flashWarning(ui);
            } else {
                effect(ui, 7);
            }
            return;
        }
        if (slot >= LockContentSlots || !stock[slot].itemId) {
            return;
        }
        panelItemName(ui, stock[slot].itemId);
        merchantLines(ui, stock[slot].itemId);
        char price[12];
        bcd4Format(stock[slot].price, price);
        panelRun(ui, 240, 144, 0x8A, true, "VALUE:");
        panelRun(ui, 240, 150, 0x0F, true, price);
        if (sessionShopBuy(s, ui->catalogLock, slot) == SessionShopBought) {
            effect(ui, 7);
        }
        return;
    }
    ChestSlot taken;
    memset(&taken, 0, sizeof(taken));
    SessionChestSlot result = sessionChestClick(s, ui->catalogLock, slot, &taken);
    playPending(ui);
    ui->panelRunCount = 0;
    switch (result) {
    case SessionChestSlotItem:
        panelItemName(ui, s->heldItem);
        break;
    case SessionChestSlotPile: {
        char amount[8];
        partyInfoFormatNumber(taken.value, amount);
        panelRun(ui, 240, 96, 0x8A, true, materialLabel(ui, taken.kind == ChestSlotGold ? 0 : taken.kind == ChestSlotOre ? 1 : 2));
        panelRun(ui, 240, 102, 0x0F, true, amount);
        break;
    }
    case SessionChestSlotRefused:
        flashWarning(ui);
        break;
    default:
        break;
    }
}

/* Space on a lock: who opens it (a prompt), whether it is locked, its trap, then the contents. */
static void chestOpen(GameUi *ui) {
    GameSession *s = ui->session;
    SessionChest chest = sessionChestOpen(s);
    playPending(ui);
    switch (chest.result) {
    case SessionChestAskWho:
        ui->pickActive = true;
        ui->pickChest = true;
        ui->pickPrompt = chest.askSearch ? 6 : 5;
        break;
    case SessionChestOpen:
        if (chest.trapDefeated) {
            panelMessageExe(ui, s->game == GameYendor2 ? 0x7CA8 : 0x7FD5, 2);
        }
        if (!chest.inexhaustible) {
            effect(ui, 6);
        }
        catalogOpen(ui, chest.lockId, false);
        break;
    default:
        break;
    }
}

/* ---- the inventory screen (RunPartyInventoryScreen, yendor2.asm:14679) ---- */

/* The members' expanded panels (RefreshPartyPortraits: all four, the P key; HandlePortraitClick: the one whose face was right-clicked). */
static void inventoryOpen(GameUi *ui, unsigned mask) {
    ui->dollMask = mask;
    ui->overlay = UiOverlayDolls;
    ui->panelRunCount = 0;
    effect(ui, 1);
}

/* Leaving the inventory screen: every open bag is written back (CloseAllAlternateBags, called as the portraits are redrawn). */
static void inventoryClose(GameUi *ui) {
    ui->overlay = UiOverlayNone;
    for (unsigned member = 0; member < 4; member++) {
        sessionBagsClose(ui->session, member);
    }
    if (ui->sellMode) { /* RunSellItemScreen's tail: the panel mode comes back and the conversation's list is waiting (UseItem loc_17D7D) */
        ui->sellMode = false;
        ui->session->sellUndoValid = false;
        saveHeaderSetU16(&ui->session->save, MapFlagsOffset, (uint16_t)((saveHeaderGetU16(&ui->session->save, MapFlagsOffset) & 0xEFFF) | ui->sellPanelWas));
        ui->overlay = UiOverlayTalk;
    }
}

static unsigned inventoryAllMask(const GameUi *ui) {
    unsigned mask = 0;
    for (unsigned i = 0; i < 4; i++) {
        mask |= sessionPartyRecord(ui->session, i) ? 1u << i : 0;
    }
    return mask;
}

/* A click on a doll: the member and the slot region (UiRegionsRosterPortraits finds the doll, UiRegionsInventoryGrid the slot in it, relative to the doll's corner). */
static void inventorySlotClick(GameUi *ui, int x, int y) {
    GameSession *s = ui->session;
    unsigned member = uiRegionHit(s->game, UiRegionsRosterPortraits, x, y);
    if (!member || member > 4 || !(ui->dollMask & (1u << (member - 1)))) {
        if (s->heldItem) { /* beside the dolls: SelectClickedRosterPortrait missed, TryDropHeldItem */
            dropStart(ui);
        }
        return;
    }
    member--;
    unsigned region = uiRegionHit(s->game, UiRegionsInventoryGrid, x - (8 + 56 * (int)member), y - 8);
    SessionInv result = sessionInventoryClick(s, member, region);
    playPending(ui);
    ui->panelRunCount = 0;
    switch (result) {
    case SessionInvTook:
    case SessionInvSwapped:
        panelItemName(ui, s->heldItem);
        if (ui->sellMode && s->heldItem) { /* ShowItemPurchaseConfirmPrompt in the sell mode: what it weighs and what the merchant pays */
            Bcd4 price;
            char text[12];
            merchantLines(ui, s->heldItem);
            if (sessionSellPrice(s, price)) {
                bcd4Format(price, text);
                panelRun(ui, 240, 144, 0x8A, true, "VALUE:");
                panelRun(ui, 240, 150, 0x0F, true, text);
            }
        }
        break;
    case SessionInvRefused:
        flashWarning(ui);
        break;
    default:
        break;
    }
}

/* A readable item (HandleGameCommand -> RunConversation): its text in the scroll of the conversation screen, no face, the rows in 0x0D, until a key. */
static void readOpen(GameUi *ui) {
    GameSession *s = ui->session;
    char line[DocumentLineBufferSize];
    ui->talkRowCount = 0;
    for (unsigned i = 0; ui->talkRowCount < 16 && sessionDocumentLine(s, i, line); i++) {
        snprintf(ui->talkRows[ui->talkRowCount], sizeof(ui->talkRows[0]), "%s", line);
        ui->talkRowCount++;
    }
    ui->reading = true;
    ui->talkListShown = false;
    ui->overlay = UiOverlayTalk;
}

/*
 * Using an item from a bag slot (the right button on it: HandleInventoryGridClick -> HandleGameCommand): the restoratives and stat items first ask whom (ConfirmAndSelectPartyTarget: the
 * crosshair, a face or F1-F4, Escape gives up); an item that would do nothing for that member beeps and asks again. Readable items open the scroll, a key to a destination takes the party there.
 */
static void scrollStart(GameUi *ui, unsigned user, unsigned slot);
static void searchStart(GameUi *ui, unsigned user, unsigned slot);
static void repairStart(GameUi *ui, unsigned user, unsigned slot);
static void repairRun(GameUi *ui);
static void unlockReport(GameUi *ui);
static void transmuteStart(GameUi *ui, unsigned user, unsigned slot);
static void talkStep(GameUi *ui, const ConversationStep *step);
static void clockShow(GameUi *ui);
static void mapSkillTooLow(GameUi *ui);

static void useItemOn(GameUi *ui, unsigned user, unsigned slot, unsigned recipient) {
    GameSession *s = ui->session;
    SessionUse result = sessionUseItem(s, user, slot, recipient);
    playPending(ui);
    ui->panelRunCount = 0;
    unlockReport(ui);
    switch (result) {
    case SessionUseDone:
        break;
    case SessionUseRead:
        inventoryClose(ui);
        readOpen(ui);
        break;
    case SessionUseTravelled:
        inventoryClose(ui);
        gameUiAmbientMusic(ui);
        break;
    case SessionUseNoEffect:
        flashWarning(ui);
        if (ui->pickActive) { /* the target prompt comes again */
            return;
        }
        break;
    case SessionUseScroll:
        scrollStart(ui, user, slot);
        return;
    case SessionUseRepair:
        repairStart(ui, user, slot);
        return;
    case SessionUseTransmute:
        transmuteStart(ui, user, slot);
        return;
    case SessionUseDiscovery:
    case SessionUseQuest: {
        unsigned itemId = sessionSlotItem(s, user, slot);
        inventoryClose(ui);
        ui->targeting = true;
        if (result == SessionUseDiscovery) {
            ui->targetDiscover = itemId;
        } else {
            ui->targetQuest = itemId;
        }
        return;
    }
    case SessionUseClock:
        inventoryClose(ui);
        clockShow(ui);
        return;
    case SessionUseLocalMap:
    case SessionUseWorldMap: /* the item itself is the map: the same screens as M and W, with the same Mapping skill gates */
        inventoryClose(ui);
        if ((result == SessionUseLocalMap ? (panelFlags(ui) & MapFlagLocalMap) : (panelFlags(ui) & MapFlagFullOverview)) || ui->freeMaps) {
            ui->overlay = result == SessionUseLocalMap ? UiOverlayLocalMap : UiOverlayWorldMap;
        } else {
            mapSkillTooLow(ui);
        }
        return;
    case SessionUseVision:
        inventoryClose(ui);
        sessionVisionBegin(s);
        ui->vision = true;
        return;
    case SessionUseSearch:
        inventoryClose(ui);
        searchStart(ui, user, slot);
        return;
    case SessionUseKey: { /* UnlockDoorCommand: a fight refuses it, else the crosshair waits for a click with that key */
        unsigned keyId = sessionSlotItem(s, user, slot);
        inventoryClose(ui);
        if (s->combat.active) {
            flashWarning(ui);
        } else {
            ui->targeting = true;
            ui->targetKey = keyId;
        }
        return;
    }
    default:
        break;
    }
    ui->pickActive = false;
}

/* The member named for a pick prompt: for a chest, the one who opens it (a refusal asks again), else the recipient of the item. */
static void pickCancel(GameUi *ui) {
    if (ui->pickScroll == 7 && !ui->catalogActive) { /* nobody bartered: the conversation goes on without the shop */
        ui->shopFromTalk = false;
    }
    ui->pickActive = ui->pickChest = false;
    ui->pickScroll = 0;
}

/*
 * A spell scroll (InteractWithContainer, yendor2.asm:53446): one that can be studied first asks USE ITEM? / LEARN SPELL? (a click on a row, the right button or Escape gives up). Using it casts its spell
 * for nothing, asking for the recipient (message 0x12) when the spell is for a single member; learning asks who (message 0x21) and a member who cannot learn it is refused with the beep and asked again.
 * A cast the place or the combat does not allow gives the refusal sound (3).
 */
static void unlockReport(GameUi *ui);

static void scrollFinish(GameUi *ui, bool study, unsigned member) {
    GameSession *s = ui->session;
    SessionCast result = sessionUseScroll(s, ui->scrollUser, ui->scrollSlot, study, member);
    playPending(ui);
    ui->panelRunCount = 0;
    unlockReport(ui);
    if (study && result == SessionCastCannot) { /* the member cannot learn it: asked again */
        flashWarning(ui);
        return;
    }
    if (result != SessionCastDone && result != SessionCastNoEffect) {
        effect(ui, 3);
    }
    pickCancel(ui);
}

static void scrollPick(GameUi *ui, unsigned mode, unsigned prompt) {
    ui->pickActive = true;
    ui->pickChest = false;
    ui->pickScroll = mode;
    ui->pickPrompt = prompt;
}

static void scrollCast(GameUi *ui, const SessionScrollInfo *info) {
    if (info->singleTarget) {
        scrollPick(ui, 1, 0x12);
    } else {
        scrollFinish(ui, false, ui->scrollUser);
    }
}

static void scrollStart(GameUi *ui, unsigned user, unsigned slot) {
    SessionScrollInfo info;
    ui->scrollUser = user;
    ui->scrollSlot = slot;
    if (!sessionScrollInfo(ui->session, user, slot, &info)) {
        flashWarning(ui);
        pickCancel(ui);
        return;
    }
    ui->pickActive = false;
    ui->askPrompt = 0x20;
    if (info.studyable) {
        ui->scrollAsk = true;
    } else {
        scrollCast(ui, &info);
    }
}

static void transmuteAnswer(GameUi *ui, int row);

static void scrollAnswer(GameUi *ui, int row) { /* 1 = USE ITEM?, 0 = LEARN SPELL? (for the ore question 1 = NUORE, 0 = MAGIC ORE) */
    SessionScrollInfo info;
    if (ui->askPrompt == 0x23) {
        transmuteAnswer(ui, row);
        return;
    }
    ui->scrollAsk = false;
    if (!sessionScrollInfo(ui->session, ui->scrollUser, ui->scrollSlot, &info)) {
        return;
    }
    if (row == 1) {
        scrollCast(ui, &info);
    } else {
        sessionMarkIneligible(ui->session, ui->scrollUser, ui->scrollSlot);
        scrollPick(ui, 2, 0x21);
    }
}

static void scrollPromptDraw(GameUi *ui) {
    ConfirmPrompt prompt;
    textPanelClear(&ui->renderer, false);
    if (confirmPromptLoad(ui->exe, ui->session->game, ui->askPrompt ? ui->askPrompt : 0x20, &prompt)) {
        confirmPromptDraw(&ui->renderer, &prompt);
    } else {
        fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 240, 120, "USE ITEM?", 0x8A, 4, FontTransparent);
        fontDrawString(ui->session->game, 0, ui->screen, ViewScreenWidth, 240, 132, "LEARN SPELL?", 0x8A, 4, FontTransparent);
    }
}

/*
 * ShowItemPurchaseConfirmPrompt outside a shop (a double click on a belt item): the item's name, then the lines the examiner's Repair skill reads (sessionExamine). The key ring shows its weight at
 * y = 102 / 108 and, from 114, a row for each key tier on it: the metal's name with C (a chest key) and D (a door key) in the brackets.
 */
static void examineShow(GameUi *ui, unsigned itemId) {
    GameSession *s = ui->session;
    SessionExamine info;
    panelItemName(ui, itemId);
    if (!sessionExamine(s, itemId, &info)) {
        return;
    }
    if (sessionIsKeyRing(s, itemId)) {
        const uint8_t *item = itemCatalogRecord(&s->items, itemId);
        char text[12];
        panelRun(ui, 240, 102, 0x8A, true, "WEIGHT:");
        partyInfoFormatTenths(item ? itemGetU16(item, ItemFieldWeight) : 0, text);
        panelRun(ui, 240, 108, 0x0F, true, text);
        unsigned flags = sessionKeyRingFlags(s), row = 0;
        for (unsigned i = 0; i < 8; i++) {
            unsigned shifted = (flags << i) & 0xFFFF;
            char name[16];
            if (!(shifted & 0x8080) || !ui->exe || !exeDataString(ui->exe, (s->game == GameYendor2 ? 0x7CB7 : 0x7FE4) + 13 * i, name, sizeof(name)) || strlen(name) < 11) {
                continue;
            }
            name[9] = (shifted & 0x8000) ? 'C' : name[9];
            name[10] = (shifted & 0x80) ? 'D' : name[10];
            panelRun(ui, 240, 114 + 6 * (int)row++, 0x0F, true, name);
        }
        return;
    }
    if (info.weight) {
        itemLines(ui, itemId, true, info.effect);
    }
    if (info.value) {
        char price[12];
        bcd4Format(info.price, price);
        panelRun(ui, 240, 144, 0x8A, true, "VALUE:");
        panelRun(ui, 240, 150, 0x0F, true, price);
    }
}

static void examineStart(GameUi *ui, unsigned itemId) {
    unsigned examiner;
    ui->examineItem = itemId;
    if (sessionExaminer(ui->session, &examiner)) {
        examineShow(ui, itemId);
    } else {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 5;
        ui->pickPrompt = 7;
    }
}

/* Chapter 2's item 0x1C (CastSpell loc_2AB8A): the fifth role's member (message 8 asks), the skill, then WHICH ORE DO YOU WANT TO CREATE? */
static void transmuteSkill(GameUi *ui, unsigned alchemist) {
    pickCancel(ui);
    if (!sessionTransmuteSkillOk(ui->session, alchemist)) {
        ui->panelRunCount = 0;
        unlockReport(ui);
        return;
    }
    ui->askPrompt = 0x23;
    ui->scrollAsk = true;
}

static void transmuteStart(GameUi *ui, unsigned user, unsigned slot) {
    unsigned alchemist;
    ui->scrollUser = user;
    ui->scrollSlot = slot;
    if (sessionAlchemist(ui->session, &alchemist)) {
        transmuteSkill(ui, alchemist);
    } else {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 6;
        ui->pickPrompt = 8;
    }
}

static void transmuteAnswer(GameUi *ui, int row) {
    ui->scrollAsk = false;
    ui->askPrompt = 0;
    ui->panelRunCount = 0;
    sessionTransmute(ui->session, ui->scrollUser, ui->scrollSlot, row == 1, &ui->transmuteMade);
    playPending(ui);
    unlockReport(ui);
}

static void talkOpen(GameUi *ui, unsigned npcId);

static void pickAnswer(GameUi *ui, unsigned member) {
    if (ui->pickScroll == 9) { /* who barters, for the sell screen */
        if (!sessionChooseBarterer(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        sellOpen(ui);
        return;
    }
    if (ui->pickScroll == 8) { /* who translates */
        if (!sessionChooseTranslator(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        talkOpen(ui, ui->talkNpc);
        return;
    }
    if (ui->pickScroll == 7) { /* who barters */
        if (!sessionChooseBarterer(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        shopFromTalkOpen(ui, true);
        return;
    }
    if (ui->pickScroll == 6) { /* who works the alchemist's item */
        if (!sessionChooseAlchemist(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        transmuteSkill(ui, member);
        return;
    }
    if (ui->pickScroll == 5) { /* who looks the item over */
        if (!sessionChooseRepairer(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        examineShow(ui, ui->examineItem);
        return;
    }
    if (ui->pickScroll == 4) { /* who repairs */
        if (!sessionChooseRepairer(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        repairRun(ui);
        return;
    }
    if (ui->pickScroll == 3) { /* who searches: a member who cannot act is refused and asked again */
        if (!sessionChestChooseOpener(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        pickCancel(ui);
        ui->targetSearch = ui->targeting = true;
        return;
    }
    if (ui->pickScroll) {
        scrollFinish(ui, ui->pickScroll == 2, member);
        return;
    }
    if (ui->pickChest) {
        if (!sessionChestChooseOpener(ui->session, member)) {
            flashWarning(ui);
            return;
        }
        ui->pickActive = ui->pickChest = false;
        chestOpen(ui);
        return;
    }
    useItemOn(ui, ui->pickUser, ui->pickSlot, member);
}

static void inventoryUse(GameUi *ui, unsigned member, unsigned slot) {
    GameSession *s = ui->session;
    unsigned id = sessionSlotItem(s, member, slot);
    if (!id || s->heldItem) {
        return;
    }
    const uint8_t *itemRecord = itemCatalogRecord(&s->items, id);
    const uint8_t *statEffect = itemRecord && (itemGetU16(itemRecord, ItemFieldFlags) & ItemFlagConsumable) ? itemEffectEntry(&s->items, itemRecord) : NULL;
    if (partyRestorativeForItem(s->game, id) != RestorativeNone || partyIsPercentRestorativeItem(s->game, id) || (statEffect && itemEffectPairs(statEffect))) {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 0;
        ui->pickPrompt = 0;
        ui->pickUser = member;
        ui->pickSlot = slot;
        return;
    }
    useItemOn(ui, member, slot, member);
}

/* ---- the commands that need an item (K, M, T, W) ---- */

/* IsItemRangeAvailable on the command's item: the belt, then every member's inventory. An unavailable command does nothing, silently. */
static bool commandItem(GameUi *ui, MainAction action) {
    unsigned id = mainCommandRequiredItem(ui->session->game, action);
    return id == 0 || ui->freeMaps || sessionHasItem(ui->session, id, id);
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
        if (s->unlockMessage >= SessionMessageLocked) { /* LOCKED (DS:0x7BCE, Chapter 3 0x7F00) and Chapter 3's throne words */
            static const unsigned kThrone[3][2] = {{0x95E5, 3}, {0x95A8, 2}, {0x95C0, 3}};
            if (s->unlockMessage == SessionMessageLocked) {
                panelMessageExe(ui, y2 ? 0x7BCE : 0x7F00, 1);
            } else {
                panelMessageExe(ui, kThrone[s->unlockMessage - SessionMessageThrone][0], kThrone[s->unlockMessage - SessionMessageThrone][1]);
            }
        } else if (s->unlockMessage >= SessionMessageSkill) { /* the ore transmutation's words (DS:0x7D7E, 0x802B, 0x7D2C, 0x8D8C), and under the two that say what was made, how much */
            static const unsigned kMade[4][2] = {{0x7D7E, 3}, {0x802B, 4}, {0x7D2C, 2}, {0x8D8C, 2}};
            unsigned which = s->unlockMessage - SessionMessageSkill;
            panelMessageExe(ui, kMade[which][0], kMade[which][1]);
            if (which >= 2) {
                char amount[12];
                partyInfoFormatNumber(ui->transmuteMade, amount);
                panelRun(ui, 240, 108, 0x0F, true, amount);
            }
        } else if (s->unlockMessage >= SessionMessageNotHere) { /* Chapter 2's relics, in white: the table of (offset, lines) in the order of the enum */
            static const unsigned kRelic[7][2] = {{0x8DA3, 3}, {0x8DBE, 3}, {0x8DDD, 3}, {0x8E1A, 2}, {0x8E30, 2}, {0x8E3D, 2}, {0x8E4E, 2}};
            unsigned which = s->unlockMessage - SessionMessageNotHere;
            panelMessageExe(ui, kRelic[which < 7 ? which : 0][0], kRelic[which < 7 ? which : 0][1]);
            for (unsigned i = 0; i < ui->panelRunCount; i++) {
                ui->panelRuns[i].colour = 0x0F;
            }
        } else if (s->unlockMessage >= 8) { /* a spell that did not work, in red: YOUR SPELL / HAS FAILED. (DS:0x867F, Chapter 3 0x89A6) or Chapter 3's YOU CAN NOT / JUMP THROUGH / HERE (0x9562) */
            panelMessageExe(ui, s->unlockMessage == 8 ? (y2 ? 0x867F : 0x89A6) : 0x9562, s->unlockMessage == 8 ? 2 : 3);
            for (unsigned i = 0; i < ui->panelRunCount; i++) {
                ui->panelRuns[i].colour = 0x59;
            }
        } else if (s->unlockMessage >= 5) { /* the repair's: YOUR ATTEMPT / TO REPAIR ... (4 and 6 lines: DS:0x82D8 / 0x8304, Chapter 3 0x85FF / 0x862B), THE ITEM IS / REPAIRED. (0x8349, 0x8670) */
            unsigned offset = s->unlockMessage == 5 ? (y2 ? 0x82D8 : 0x85FF) : s->unlockMessage == 6 ? (y2 ? 0x8304 : 0x862B) : (y2 ? 0x8349 : 0x8670);
            panelMessageExe(ui, offset, s->unlockMessage == 5 ? 4 : s->unlockMessage == 6 ? 6 : 2);
        } else if (s->unlockMessage >= 3) { /* the lockpick's: THE LOCKPICK / BROKE (DS:0x7C89, Chapter 3 0x7FB6), TRAP / DEFEATED (0x7CA8, 0x7FD5) */
            panelMessageExe(ui, s->unlockMessage == 3 ? (y2 ? 0x7C89 : 0x7FB6) : (y2 ? 0x7CA8 : 0x7FD5), 2);
        } else {
            panelMessageExe(ui, s->unlockMessage == 1 ? (y2 ? 0x7BB7 : 0x7EE9) : (y2 ? 0x7BAE : 0x7EE0), 1);
        }
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
    if (ui->targetSearch) { /* the lockpick: sessionSearch on what is ahead, the pick of the slot it came from breaking if the search fails */
        ui->targetSearch = false;
        sessionSearch(s, ui->searchUser, ui->searchSlot);
    } else if (ui->targetDiscover) {
        sessionUseDiscoveryKey(s, ui->targetDiscover);
        ui->targetDiscover = 0;
    } else if (ui->targetQuest) {
        unsigned npc = sessionUseQuestItem(s, ui->targetQuest);
        ui->targetQuest = 0;
        ConversationStep first;
        if (npc && sessionTalkStart(s, npc, 0, &first)) { /* UseItem(0x8C), then the arrival */
            ui->overlay = UiOverlayTalk;
            ui->talkListShown = false;
            talkStep(ui, &first);
        } else if (npc) {
            sessionQuestArrive(s);
        }
    } else if (ui->targetKey) {
        sessionUnlockWithKey(s, ui->targetKey);
        ui->targetKey = 0;
    } else {
        sessionUnlockWithRing(s);
    }
    playPending(ui);
    unlockReport(ui);
}

/* RepairItemCommand: the second role's member repairs (message 9 asks who when nobody is chosen), then the roll and its words. */
static void repairRun(GameUi *ui) {
    ui->panelRunCount = 0;
    sessionRepair(ui->session, ui->searchUser, ui->searchSlot);
    playPending(ui);
    unlockReport(ui);
}

static void repairStart(GameUi *ui, unsigned user, unsigned slot) {
    unsigned repairer;
    ui->searchUser = user;
    ui->searchSlot = slot;
    if (sessionRepairer(ui->session, &repairer)) {
        repairRun(ui);
    } else {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 4;
        ui->pickPrompt = 9;
    }
}

/* The lockpick (HandleSearchCommand): who searches (the third role, asked with message 8 "WHO WILL USE?" when nobody is), then the crosshair waits for a click on the view. */
static void searchStart(GameUi *ui, unsigned user, unsigned slot) {
    unsigned searcher;
    ui->searchUser = user;
    ui->searchSlot = slot;
    if (ui->session->combat.active) {
        flashWarning(ui);
        return;
    }
    if (sessionSearcher(ui->session, &searcher)) {
        ui->targetSearch = ui->targeting = true;
    } else {
        ui->pickActive = true;
        ui->pickChest = false;
        ui->pickScroll = 3;
        ui->pickPrompt = 8;
    }
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
    unlockReport(ui);
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

bool gameUiFading(const GameUi *ui) {
    return ui->clueStage != 0 || ui->clueSplash != 0;
}

bool gameUiFadeStep(GameUi *ui) {
    if (ui->clueStage) {
        clueStageTick(ui);
        return true;
    }
    if (ui->clueSplash) {
        if (--ui->clueSplash == 0) {
            ui->clueSplash = 1;
            clueSplashEnd(ui);
        } else {
            clueSplashPalette(ui);
        }
        return true;
    }
    return false;
}

void gameUiSettle(GameUi *ui) {
    GameSession *s = ui->session;
    clueSettle(ui);
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

static void wipeCheck(GameUi *ui);

bool gameUiAnimate(GameUi *ui) {
    bool changed = false;
    wipeCheck(ui);
    playPending(ui); /* the sounds the session has queued, and the next of those waiting */
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
    clueClose(ui); /* from the first list Escape is the EXIT tab (command 8): the book closes */
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
    if (ui->clueList.count > ClueListRows) { /* "a  MORE  b" at (129, 167): the letters are there only where there is more to turn to (DrawClueEntryList) */
        char more[16] = "a  MORE  b";
        if (!clueListCanPageUp(&ui->clueList)) {
            more[0] = ' ';
        }
        if (!clueListCanPageDown(&ui->clueList)) {
            more[9] = ' ';
        }
        fontDrawString(game, 0, ui->screen, ViewScreenWidth, 129, 167, more, 0x0D, 0, FontTransparent);
    }
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
    if (ui->clueSplash) {
        memset(ui->screen, 0, sizeof(ui->screen));
        viewDrawPicture(&ui->renderer, 0, 12, 1, 1, false, 0);
        return;
    }
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
    if (ui->clueSplash) { /* the first-time picture goes at a key */
        clueSplashEnd(ui);
        return true;
    }
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
        clueClose(ui);
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
/*
 * CheckPartyWipeAndReinitLevel (yendor2.asm:38529): when nobody is left standing ShowPartyWipeScreen stops the music, plays the sound (Chapter 2 0x13, Chapter 3 0x32) and clears the screen to the bare frame
 * with the chapter plate; RunGameDialog then offers LOAD, NEW GAME and DOS (the flags 0x70: no RETURN, no SAVE) until one is chosen.
 */
static void wipeCheck(GameUi *ui) {
    GameSession *s = ui->session;
    if (!s->combat.wiped || ui->wipe) {
        return;
    }
    ui->wipe = true;
    ui->combatPanel = false;
    ui->scrollAsk = ui->pickActive = ui->dropPrompt = ui->targeting = false;
    ui->panelRunCount = 0;
    s->heldItem = s->heldExtra = 0;
    if (ui->host.music) {
        ui->host.music(ui->host.ctx, 0);
    }
    effect(ui, s->game == GameYendor2 ? 0x13 : 0x32);
    dialogOpen(ui, GameDialogFlagLoad | GameDialogFlagNewGame | GameDialogFlagDos);
}

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
    if (ui->overlay == UiOverlayDialog && (ui->boot == UiBootSaves || ui->boot == UiBootOptions || ui->wipe)) {
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
    combatPanelSync(ui);
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
    if (ui->overlay == UiOverlayNone || (ui->overlay == UiOverlayTalk && ui->shopFromTalk)) { /* (a conversation's merchant keeps the right-hand panels) */
        if (s->combat.active) { /* DrawAndCacheStatusIcon: the swords take the shoot icon's place in a combat */
            viewDrawPicture(&ui->renderer, 8, 3, IconShootX, IconRowY, false, 0);
            resourceIconsDraw(ui);
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
        if (ui->scrollAsk) {
            scrollPromptDraw(ui);
        }
        if (ui->pickActive && ui->pickPrompt) {
            ConfirmPrompt prompt;
            textPanelClear(&ui->renderer, false);
            if (confirmPromptLoad(ui->exe, s->game, ui->pickPrompt, &prompt)) {
                confirmPromptDraw(&ui->renderer, &prompt);
            }
        }
        if (s->shot.active) { /* HandleRangedOrCombatAction: the shoot icon is down while the shot flies */
            viewDrawPicture(&ui->renderer, 8, 2, IconShootX, IconRowY, false, 0);
        } else if (ui->pressTicks > 0) {
            viewDrawPicture(&ui->renderer, 8, ui->pressPicture, ui->pressX, ui->pressY, false, 0);
        }
    }
    switch (ui->overlay) {
    case UiOverlayMount:
        mountDraw(ui);
        break;
    case UiOverlayLocalMap: {
        static LocalMapCell localCells[LocalMapColumns * LocalMapRows];
        int c0, r0;
        localMapBlockOrigin(s->x, s->y, &c0, &r0);
        localMapFill(localCells, s->game, &s->map, &s->save, c0, r0);
        for (int i = 0; i < LocalMapColumns * LocalMapRows; i++) { /* DrawLocalMapCell: a trigger that changed the wall or the floor shows its new tile */
            if (localCells[i].explored) {
                uint16_t scripted;
                if (windowBakeLocalMapFloor(s->game, &s->objects, &s->save, c0 + i % LocalMapColumns, r0 + i / LocalMapColumns, &scripted)) { /* Chapter 3's story-event cells: a marker instead of the floor */
                    localCells[i].floorType = scripted;
                    continue;
                }
                WindowBake bake = windowBakeAtCell(s->game, &s->objects, &s->locks, &s->save, c0 + i % LocalMapColumns, r0 + i / LocalMapColumns);
                if (bake.setWall) {
                    localCells[i].wallType = bake.newType;
                }
                if (bake.setFloor) {
                    localCells[i].floorType = bake.newType;
                }
            }
        }
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
        if (ui->sellMode) {
            sellTextDraw(ui);
        }
        for (unsigned i = 0; i < 4; i++) {
            if (sessionPartyRecord(s, i) && (ui->dollMask & (1u << i))) {
                paperDollDraw(&ui->renderer, &s->items, sessionPartyRecord(s, i), 8 + 56 * (int)i, 8);
            }
        }
        panelRunsDraw(ui);
        break;
    case UiOverlayDialog:
        dialogDraw(ui);
        break;
    default:
        break;
    }
}

void gameUiDraw(GameUi *ui) {
    if (sessionQuestArrivalPending(ui->session) && ui->overlay != UiOverlayTalk) { /* the throne's talk is over: the party is carried off */
        sessionQuestArrive(ui->session);
        playPending(ui);
        gameUiAmbientMusic(ui);
    }
    if (!(ui->pickActive && ui->pickScroll == 2)) { /* the dimmed faces of those who cannot learn the scroll last as long as the question (ClearIneligibleFlagForAllMembers) */
        sessionClearIneligible(ui->session);
    }
    wipeCheck(ui);
    if (ui->clueStage == ClueStageOutGame || ui->clueStage == ClueStageOutBook) { /* the picture that is fading out */
        memcpy(ui->screen, ui->clueFrozen, sizeof(ui->screen));
        memcpy(ui->cursorBase, ui->screen, sizeof(ui->cursorBase));
        ui->cursorBaseValid = true;
        return;
    }
    drawScreen(ui);
    if (ui->catalogActive && ((ui->overlay != UiOverlayTalk && ui->overlay != UiOverlayDetail) || (ui->overlay == UiOverlayTalk && ui->shopFromTalk))) {
        catalogDraw(ui);
    }
    if (ui->dropPrompt) {
        dropPromptDraw(ui);
    }
    memcpy(ui->cursorBase, ui->screen, sizeof(ui->cursorBase));
    ui->cursorBaseValid = true;
    cursorDraw(ui);
}

void gameUiRedrawCursor(GameUi *ui) {
    if (!ui->cursorBaseValid) {
        gameUiDraw(ui);
        return;
    }
    memcpy(ui->screen, ui->cursorBase, sizeof(ui->screen));
    cursorDraw(ui);
}

void gameUiStatus(const GameUi *ui, char *out, size_t capacity) {
    const GameSession *s = ui->session;
    snprintf(out, capacity, "Yendorian Tales - (%d, %d) clock %02u:%02u%s%s%s", s->x, s->y, s->clock.minutes / 60, s->clock.minutes % 60, s->combat.active ? " - COMBAT: A attacks" : "",
             s->log[0] ? " - " : "", s->log);
}

/*
 * A healer's or trainer's quote (ShowHealingCostPrompt): where the topic's words were, IT WILL COST <price> GOLD / what it buys / a blank row / IS THAT PRICE AGREEABLE?, in the dialog's rows, and under
 * them the gold line. The strings are the executable's: the offsets of Chapter 2 and of Chapter 3 (the data segment moved 0x32D bytes, a little less for the trainer's refusal).
 */
static const char *quoteText(GameUi *ui, unsigned ch2, unsigned ch3, char *buffer, size_t size) {
    buffer[0] = 0;
    if (ui->exe) {
        exeDataString(ui->exe, ui->session->game == GameYendor2 ? ch2 : ch3, buffer, size);
    }
    return buffer;
}

static void talkQuoteShow(GameUi *ui) {
    GameSession *s = ui->session;
    char cost[16], line[64], what[48], agree[48], goldWord[16], prefix[32];
    static const unsigned kWhat[5][2] = {{0x7F26, 0x8253}, {0x7F47, 0x8274}, {0x7F62, 0x828F}, {0x7F79, 0x82A6}, {0x7F94, 0x82C1}};
    unsigned kind = s->quoteKind;
    ui->talkExtraCount = 0;
    if (kind == SessionQuoteNoGold) { /* the rows stay as they were; only the refusal is written, in red, lower down */
        ui->talkExtra[0].x = 22;
        ui->talkExtra[0].y = 103;
        ui->talkExtra[0].colour = 0x7B;
        quoteText(ui, 0x808C, 0x83B9, ui->talkExtra[0].text, sizeof(ui->talkExtra[0].text));
        ui->talkExtraCount = 1;
        return;
    }
    if (kind == SessionQuoteTrainTooHigh) { /* UNFORTUNATELY, I CAN ONLY TRAIN / YOU THROUGH LEVEL <n> */
        char second[48];
        quoteText(ui, 0x86C4, 0x89EB, line, sizeof(line));
        snprintf(second, sizeof(second), "YOU THROUGH LEVEL");
        if (ui->exe && line[0]) {
            unsigned next = (s->game == GameYendor2 ? 0x86C4u : 0x89EBu) + (unsigned)strlen(line) + 1;
            exeDataString(ui->exe, next, second, sizeof(second));
        }
        ui->talkRowCount = 2;
        snprintf(ui->talkRows[0], sizeof(ui->talkRows[0]), "%.33s", line);
        snprintf(ui->talkRows[1], sizeof(ui->talkRows[1]), "%.33s", second);
        ui->talkExtra[0].x = 22 + 6 * ((int)strlen(second) + 1);
        ui->talkExtra[0].y = 73;
        ui->talkExtra[0].colour = 0x8A;
        snprintf(ui->talkExtra[0].text, sizeof(ui->talkExtra[0].text), "%u", s->quoteNumber);
        ui->talkExtraCount = 1;
        ui->talkQuote = false;
        return;
    }
    bcd4Format(s->serviceCost, cost);
    quoteText(ui, 0x805F, 0x838C, prefix, sizeof(prefix));
    quoteText(ui, 0x806D, 0x839A, goldWord, sizeof(goldWord));
    quoteText(ui, 0x8073, 0x83A0, agree, sizeof(agree));
    quoteText(ui, kWhat[kind == SessionQuoteTrain ? 4 : kind - 1][0], kWhat[kind == SessionQuoteTrain ? 4 : kind - 1][1], what, sizeof(what));
    if (kind == SessionQuoteTrain) { /* TO TRAIN TO LEVEL <n>. */
        size_t at = strlen(what);
        snprintf(what + at, sizeof(what) - at, "%u.", s->quoteNumber);
    }
    snprintf(line, sizeof(line), "%s%s%s", prefix, cost, goldWord);
    snprintf(ui->talkRows[0], sizeof(ui->talkRows[0]), "%.33s", line);
    snprintf(ui->talkRows[1], sizeof(ui->talkRows[1]), "%.33s", what);
    ui->talkRows[2][0] = 0;
    snprintf(ui->talkRows[3], sizeof(ui->talkRows[3]), "%.33s", agree);
    ui->talkRowCount = 4;
    ui->talkQuote = true;
}

static void talkStep(GameUi *ui, const ConversationStep *step) {
    GameSession *s = ui->session;
    if (!step->topic) {
        return;
    }
    char keptRows[16][DialogLineChars + 1];
    unsigned keptCount = ui->talkRowCount;
    memcpy(keptRows, ui->talkRows, sizeof(keptRows));
    bool keptQuote = ui->talkQuote;
    conversationText(&s->talk, step->topic, ui->talkText, sizeof(ui->talkText));
    ui->talkRowCount = conversationRows(&s->talk, step->topic, ui->talkRows, sizeof(ui->talkRows) / sizeof(ui->talkRows[0]));
    if (step->flags & DialogTopicPreview) {
        ui->talkPreviewTopic = step->topic;
    }
    ui->talkQuote = false;
    ui->talkExtraCount = 0;
    if (step->flags & DialogTopicSell) { /* RunSellItemScreen after ConfirmAndValidatePartyTarget: the inventory is the screen (the dialog's menu is waiting behind it) */
        unsigned barterer;
        ui->sellMask = step->arg;
        if (sessionBarterer(s, &barterer)) {
            sellOpen(ui);
        } else {
            ui->pickActive = true;
            ui->pickChest = false;
            ui->pickScroll = 9;
            ui->pickPrompt = 10;
        }
        return;
    }
    SessionService service = sessionTalkService(s, step, NULL);
    if (s->quoteKind == SessionQuoteNoGold) { /* the quote stays on the screen, with the refusal under it */
        memcpy(ui->talkRows, keptRows, sizeof(keptRows));
        ui->talkRowCount = keptCount;
        ui->talkQuote = keptQuote;
        talkQuoteShow(ui);
    } else if (s->quoteKind) {
        talkQuoteShow(ui);
    }
    if (service == SessionServiceNone && step->needsResolve) {
        sessionTalkResolve(s, true);
    }
    if (service == SessionServiceNeedScreen && (step->flags & DialogTopicUseKey)) { /* a shop: the lock record named by the topic's argument */
        ui->shopFromTalk = true; /* the dialog stays up behind the merchant's screen (RunShopScreen is called from it) */
        ui->shopLock = step->arg;
        barterStart(ui);
    } else if (service == SessionServiceNeedScreen && (step->flags & DialogTopicCheckKey)) { /* CheckKeyItem: the same screen with no question and no price -- what the topic gives (the armory's answer: gold nuggets) */
        ui->shopLock = step->arg;
        shopFromTalkOpen(ui, false);
    }
}

/* LoadItemData: an NPC with a foreign tongue first asks WHO WILL TRANSLATE? (message 0xB) unless the fourth role has someone; giving the question up, the conversation does not begin. */
static void talkOpen(GameUi *ui, unsigned npcId) {
    GameSession *s = ui->session;
    ConversationStep first;
    if (sessionNpcNeedsTranslator(s, npcId)) {
        unsigned translator;
        if (!sessionTranslator(s, &translator)) {
            ui->talkNpc = npcId;
            ui->pickActive = true;
            ui->pickChest = false;
            ui->pickScroll = 8;
            ui->pickPrompt = 0x0B;
            return;
        }
    }
    if (sessionTalkStart(s, npcId, 0, &first)) {
        ui->overlay = UiOverlayTalk;
        ui->talkListShown = false; /* the greeting first */
        talkStep(ui, &first);
    }
}

/* The use command on the object ahead (sessionInteract): chests are looted, destinations travelled to, the Chapter 2 ending is played; the rest shows up in the status line. */
/* A message the session names in the executable's data segment: its lines, joined, into the log. */

static void useAhead(GameUi *ui) {
    GameSession *s = ui->session;
    SessionInteract use = sessionInteract(s);
    switch (use.kind) {
    case SessionInteractDoor:
        chestOpen(ui);
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
        talkOpen(ui, use.value);
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
    if (ui->wipe) { /* a load that has been made: back in the game */
        ui->wipe = false;
        ui->lastMusicPage = 0xFFFF;
        gameUiAmbientMusic(ui);
    }
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

bool gameUiLoadSlot(GameUi *ui, unsigned slot) {
    ui->dialogSlot = (int)slot;
    GameSession *s = ui->session;
    static uint8_t image[SaveFileSizeMax];
    size_t bytes = ui->host.slotRead ? ui->host.slotRead(ui->host.ctx, slot, image, sizeof(image)) : 0;
    if (!bytes || !sessionLoad(s, image, bytes)) {
        return false;
    }
    ExploreReveal revealed;
    exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
    sessionRebuild(s);
    s->log[0] = 0;
    paletteSyncToClock(ui);
    return true;
}

/* ConfirmNewGame's yes: RunTitleScreen starts a new game (InitializeNewGameWorldState) and the title menu is up; its E goes on into the game. */
static void startNewGame(GameUi *ui) {
    if (sessionNewGame(ui->session, ui->worldDat, ui->worldSize)) {
        ui->wipe = false;
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

static void talkChoose(GameUi *ui, unsigned number) { /* topic `number` of the list (0 = the last one: the way out) */
    GameSession *s = ui->session;
    ConversationStep step = sessionTalkChoose(s, number);
    talkStep(ui, &step);
    if (!s->talking && ui->overlay == UiOverlayTalk) {
        if (step.topic) { /* the ending topic's own words wait for a key (UseItem: WaitForKeypressTickingMusic) */
            ui->talkEnding = true;
            ui->talkListShown = false;
        } else {
            ui->overlay = UiOverlayNone;
        }
    }
}

/*
 * A key or a click puts the ending conversation away. UseItem then hands its exit code (the ending topic's argument when it is 1 or 2, word_2E52A) to `start`, which answers 1 and 2 with
 * RunTitleScreen and, when that is left with E, InitializeDungeonLevel -- the "GO TO THE MAIN MENU? YES" of the inns.
 */
static void talkFinish(GameUi *ui) {
    GameSession *s = ui->session;
    ui->talkEnding = false;
    ui->overlay = UiOverlayNone;
    if (s->talk.exitCode == 1 || s->talk.exitCode == 2) {
        ui->overlay = UiOverlayTitle;
        ui->boot = UiBootNone;
        gameUiAmbientMusic(ui);
    }
}

/* UseItem loc_17E3B: another member is selected (a click on a face or F1-F4): the frame moves, the service asks again what that member can be offered, and the words of the topic that loaded it return. */
static bool talkSelect(GameUi *ui, unsigned member) {
    GameSession *s = ui->session;
    if (!sessionTalkSelect(s, member)) {
        return false;
    }
    if (ui->talkPreviewTopic) {
        conversationText(&s->talk, ui->talkPreviewTopic, ui->talkText, sizeof(ui->talkText));
        ui->talkRowCount = conversationRows(&s->talk, ui->talkPreviewTopic, ui->talkRows, sizeof(ui->talkRows) / sizeof(ui->talkRows[0]));
    }
    ui->talkQuote = false;
    ui->talkExtraCount = 0;
    return true;
}

static bool talkKey(GameUi *ui, UiKey key) { /* 1-9 choose a topic, Escape the last one listed (the way out); the first key only lets the greeting go and shows the list */
    if (ui->reading) { /* a readable item's text: any key puts it away */
        ui->reading = false;
        ui->overlay = UiOverlayNone;
        return true;
    }
    if (ui->talkEnding) {
        talkFinish(ui);
        return true;
    }
    if (!ui->talkListShown) {
        ui->talkListShown = true;
        return true;
    }
    if (key >= UiKeyF1 && key <= UiKeyF4 && sessionTalkHasSelector(ui->session)) {
        talkSelect(ui, (unsigned)(key - UiKeyF1));
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
    } else if (key >= '1' && key <= '4') { /* the party panels' keys */
        extended = false;
        code = (uint8_t)key;
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
        playPending(ui);
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
            if ((panelFlags(ui) & MapFlagFullOverview) || ui->freeMaps) {
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
            if ((panelFlags(ui) & MapFlagLocalMap) || ui->freeMaps) {
                ui->overlay = UiOverlayLocalMap;
            } else {
                mapSkillTooLow(ui);
            }
        }
        break;
    case MainActionClueBook:
        if (ui->exe) {
            clueOpen(ui);
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
        inventoryOpen(ui, inventoryAllMask(ui));
        break;
    case MainActionPartyPanel: /* 1-4: the numbers under a member's bars (DrawThreeStatBars) */
        partyInfoShow(ui, command.index & 3, 4);
        break;
    case MainActionStatusMessage: /* Chapter 3's V: the two lines at DS:0x9538 (REGISTERED / VER 2.00) in the status panel */
        panelMessageExe(ui, 0x9538, 2);
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
    if (ui->scrollAsk) { /* ShowConfirmPrompt 0x20 answers to clicks on its two rows only; Escape gives up */
        if (key == UiKeyEscape) {
            ui->scrollAsk = false;
        }
        return true;
    }
    if (ui->pickActive) { /* the member the item is used on (or who opens the chest): F1-F4 name one, Escape gives up */
        if (key >= UiKeyF1 && key <= UiKeyF4 && sessionPartyRecord(s, (unsigned)(key - UiKeyF1))) {
            pickAnswer(ui, (unsigned)(key - UiKeyF1));
        } else if (key == UiKeyEscape) {
            pickCancel(ui);
        }
        return true;
    }
    if (ui->catalogActive) { /* RunShopScreen's keys: Space leaves, P the inventory, 1-4 the member bars, F1-F4 the sheets; the rest is ignored */
        if (key == ' ') {
            catalogLeave(ui);
        } else if (key == 'p') {
            inventoryOpen(ui, inventoryAllMask(ui));
        } else if (key >= '1' && key <= '4') {
            partyInfoShow(ui, (unsigned)(key - '1'), 4);
        } else if (key >= UiKeyF1 && key <= UiKeyF4) {
            ui->hero = (unsigned)(key - UiKeyF1);
            ui->overlay = UiOverlayDetail;
        }
        return true;
    }
    if (ui->dropPrompt) { /* ShowConfirmPrompt 4: Y or Enter drop it, N or Escape keep it */
        int answer = gameDialogPromptKey(key >= 'a' && key <= 'z' ? (uint8_t)(key - 32) : (uint8_t)key);
        if (answer >= 0) {
            dropAnswer(ui, answer == 1);
        }
        return true;
    }
    if (ui->targeting) { /* WaitForTargetClick: only Escape is heard */
        if (key == UiKeyEscape) {
            ui->targeting = ui->targetSearch = false;
            ui->targetKey = ui->targetDiscover = ui->targetQuest = 0;
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
            { /* InitializeDungeonLevel: the level is built again for the party the title menu left */
                ExploreReveal revealed;
                exploreRevealAroundPlayer(&s->save, s->x, s->y, s->facing, &revealed);
                sessionRebuild(s);
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
    if (ui->overlay == UiOverlayTalk) {
        return talkKey(ui, key);
    }
    if (ui->overlay == UiOverlayClue) {
        return clueKey(ui, key);
    }
    if (ui->overlay == UiOverlayAlchemy) {
        return alchemyKey(ui, key);
    }
    if (ui->overlay == UiOverlayDolls && ui->sellMode) { /* the sell screen: Space sells the cursor item, Escape undoes the last sale (nothing held) or warns (something held), P leaves */
        if (key == ' ' && s->heldItem) {
            SessionSell result = sessionSellHeld(s, ui->sellMask);
            ui->panelRunCount = 0;
            if (result == SessionSellRefused) {
                sellRefused(ui);
            }
        } else if (key == UiKeyEscape) {
            if (s->heldItem) {
                flashWarning(ui);
            } else if (sessionSellUndo(s)) {
                ui->panelRunCount = 0;
                panelItemName(ui, s->heldItem);
            }
        } else if (key == 'p') {
            inventoryClose(ui);
            effect(ui, 1);
        }
        return true;
    }
    if (ui->overlay == UiOverlayDolls) {
        if (key == ' ' && ui->catalogActive && ui->catalogShop && s->heldItem) { /* TrySellItemForGold: the item on the cursor is sold to the merchant */
            catalogSlotClick(ui, 0);
        } else if (key == 'p' || (key == UiKeyEscape && !s->heldItem)) {
            inventoryClose(ui);
            ui->panelRunCount = 0;
            effect(ui, 1);
        } else if (key == UiKeyEscape) { /* an item on the cursor: FlashStatusWarning */
            flashWarning(ui);
        } else if (key >= '1' && key <= '4') {
            partyInfoShow(ui, (unsigned)(key - '1'), 4);
        }
        return true;
    }
    if (ui->overlay == UiOverlayMount) { /* the flight waits for a click on the box; only Escape is heard */
        if (key == UiKeyEscape) {
            ui->overlay = UiOverlayNone;
        }
        return true;
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

static bool gameUiKeyInner(GameUi *ui, UiKey key, bool ctrl);

bool gameUiKey(GameUi *ui, UiKey key, bool ctrl) {
    bool result = gameUiKeyInner(ui, key, ctrl);
    playPending(ui); /* whatever the command made the session ask for: a fight's sounds, a trap's */
    return result;
}

static bool gameUiKeyInner(GameUi *ui, UiKey key, bool ctrl) {
    wipeCheck(ui);
    clueSettle(ui);
    ui->mountList = 0;
    if (ui->vision) { /* WaitForKeypressTickingMusic: any key brings the party's own view back */
        ui->vision = false;
        sessionVisionEnd(ui->session);
        return true;
    }
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
    if (ui->clueSplash) { /* the first-time picture goes at a click */
        clueSplashEnd(ui);
        return true;
    }
    if (tab == 8) { /* EXIT */
        clueClose(ui);
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

static bool talkClick(GameUi *ui, int x, int y) {
    if (ui->reading) {
        ui->reading = false;
        ui->overlay = UiOverlayNone;
        return true;
    } /* the click table at DS:0x6458: the topic rows drawTalk lists, five to a column (x 58-135 and 142-220, 6 pixels each from y = 32) */
    GameSession *s = ui->session;
    if (ui->talkEnding) {
        talkFinish(ui);
        return true;
    }
    if (!ui->talkListShown) {
        ui->talkListShown = true;
        return true;
    }
    if (sessionTalkHasSelector(s)) { /* a face (the party panel table's regions 1, 11, 21, 31) selects its member */
        unsigned panel = uiRegionHit(s->game, UiRegionsPartyPanels, x, y);
        if (panel % 10 == 1 && panel / 10 < 4) {
            talkSelect(ui, panel / 10);
            return true;
        }
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

/* The contents screen's clicks: a slot works it, the view drops the cursor item (the right button there leaves), the belt, the faces and the party panels work as on the main screen. */
static bool catalogClick(GameUi *ui, int x, int y, bool right) {
    GameSession *s = ui->session;
    GameKind game = s->game;
    unsigned region = uiRegionHit(game, UiRegionsDungeonMain, x, y);
    if (right) {
        if (region == 1) {
            catalogLeave(ui);
        } else if (region == 6) {
            unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y), member = panel / 10;
            if (panel % 10 == 1 && member < 4 && sessionPartyRecord(s, member)) {
                inventoryOpen(ui, 1u << member);
            }
        }
        return true;
    }
    if (region == 5) {
        unsigned slot = uiRegionHit(game, UiRegionsCatalogSlots, x, y);
        if (slot) {
            catalogSlotClick(ui, slot - 1);
        }
    } else if (region == 1) {
        dropStart(ui);
    } else if (region == 2) {
        panelClick(ui, x, y);
    } else if (region == 6) {
        unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y), member = panel / 10, part = panel % 10;
        if (panel && member < 4 && sessionPartyRecord(s, member)) {
            if (s->heldItem) {
                part == 1 ? dropOnPortrait(ui, member) : flashWarning(ui);
            } else if (part == 1) {
                ui->hero = member;
                ui->overlay = UiOverlayDetail;
            } else if (partyInfoKindForRegion(part) >= 0) {
                partyInfoShow(ui, member, partyInfoKindForRegion(part));
            }
        }
    }
    return true;
}

/*
 * A click on a monster's info panel in a fight (HandleDungeonInput loc_1667A, the table UiRegionsMonsterPanels: five regions a panel): the name lines choose the monster the attacks fall on, the health
 * bar asks for the HEALTH line (state bit 0x40) and the three status icons for the poison / disease (0x200), paralysis / frozen (0x100) and hex / curse (0x80) lines -- shown by the next draw of the panel.
 */
static void monsterPanelClick(GameUi *ui, int x, int y) {
    SessionCombat *c = &ui->session->combat;
    unsigned code = uiRegionHit(ui->session->game, UiRegionsMonsterPanels, x, y);
    if (!code) {
        return;
    }
    unsigned slot = (code - 1) / 5, part = (code - 1) % 5 + 1;
    uint8_t *monster = c->slots + (size_t)slot * MonsterRecordSize;
    if (slot >= CombatMonsterSlotCount || !monsterGetU16(monster, MonsterFieldType)) {
        return;
    }
    static const unsigned kBits[5] = {0, 0x40, 0x200, 0x100, 0x80};
    if (part == 1) {
        sessionCombatChoose(ui->session, slot);
    } else {
        monsterSetU16(monster, MonsterFieldState, (uint16_t)(monsterGetU16(monster, MonsterFieldState) | kBits[part - 1]));
    }
}

static bool mainClick(GameUi *ui, int x, int y, bool right) {
    GameSession *s = ui->session;
    GameKind game = s->game;
    if (ui->catalogActive) { /* RunShopScreen's own clicks (below), not the walk pad's */
        return catalogClick(ui, x, y, right);
    }
    if (right) { /* the right button on the view uses what is ahead (the same as Space); on a belt slot of the resource panel it uses that item (TryCureAilmentFromIconClick) */
        unsigned where = uiRegionHit(game, UiRegionsDungeonMain, x, y);
        if (where == 2 && (panelFlags(ui) & MapFlagHidden)) {
            unsigned icon = uiRegionHit(game, UiRegionsStatusIconBar, x, y);
            bool myTurn = !s->combat.active || (s->combat.cursor < s->combat.count && !s->combat.order[s->combat.cursor].isMonster);
            if (icon >= 4 && icon <= 9 && myTurn) {
                unsigned user = 0; /* the member whose turn it is in a fight, else the first who can act */
                if (s->combat.active) {
                    user = s->combat.order[s->combat.cursor].index;
                } else {
                    while (user < 3 && sessionPartyRecord(s, user + 1) && (!sessionPartyRecord(s, user) || (partyGetU16(sessionPartyRecord(s, user), PartyFieldStatusFlags) & PartyStatusIncapacitated))) {
                        user++;
                    }
                }
                inventoryUse(ui, user, SessionBeltSlotBase + icon - 4);
            }
            return true;
        }
        return where == 1 ? press(ui, ' ') : true;
    }
    unsigned pad = uiRegionHit(game, UiRegionsDirectionPad, x, y);
    if (pad) { /* the walk pad: turn left, forward, turn right, strafe left, back, strafe right */
        static const UiKey kKeys[6] = {UiKeyLeft, UiKeyUp, UiKeyRight, UiKeyLeft, UiKeyDown, UiKeyRight};
        return gameUiKey(ui, kKeys[pad - 1], pad == 4 || pad == 6);
    }
    unsigned region = uiRegionHit(game, UiRegionsDungeonMain, x, y);
    if (region == 4 && s->combat.active) { /* the monsters' panels */
        monsterPanelClick(ui, x, y);
        return true;
    }
    if (region == 2) { /* the top-right panel: the plate or the minimap, then the resource panel */
        if (!s->combat.active) {
            panelClick(ui, x, y);
        } else {
            unsigned icon = uiRegionHit(game, UiRegionsStatusIconBar, x, y);
            if (icon >= 4 && icon <= 9) { /* in a fight the panel is the resource panel: its belt slots work, the counters are not shown */
                beltClick(ui, icon - 4);
            }
        }
        return true;
    }
    if (region == 3) { /* the icon row: act (attack in a combat), alchemy, rest, the pause dialog */
        unsigned icon = uiRegionHit(game, UiRegionsDungeonIconRow, x, y);
        return icon == 1 ? press(ui, s->combat.active ? 'a' : 's') : icon == 2 ? press(ui, 'c') : icon == 3 ? press(ui, 'r') : icon == 4 ? press(ui, 'd') : true;
    }
    if (region == 6) { /* a party panel: the cursor item dropped on a face, else its icons' lists and (a click standing for the held button) the face's sheet */
        unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y);
        unsigned member = panel / 10, part = panel % 10;
        if (panel && member < 4 && sessionPartyRecord(s, member)) {
            if (s->heldItem) {
                if (part == 1) {
                    dropOnPortrait(ui, member);
                } else {
                    flashWarning(ui);
                }
            } else if (part == 1) {
                return press(ui, UiKeyF1 + (int)member);
            } else if (partyInfoKindForRegion(part) >= 0) {
                partyInfoShow(ui, member, partyInfoKindForRegion(part));
            }
        }
        return true;
    }
    if (region == 1 && s->heldItem) { /* TryDropHeldItem */
        dropStart(ui);
    }
    return true;
}

/* The inventory screen's clicks (RunPartyInventoryScreen's event machine): the left button works a slot, the belt or the portraits; the right button on a face shows or hides that member's panel (the
 * last one gone ends the screen). Right clicks that would use an item are not handled here. */
static bool dollsClick(GameUi *ui, int x, int y, bool right) {
    GameSession *s = ui->session;
    GameKind game = s->game;
    unsigned region = uiRegionHit(game, UiRegionsDungeonMain, x, y);
    if (right) {
        if (region == 6) {
            unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y), member = panel / 10;
            if (panel % 10 == 1 && member < 4 && sessionPartyRecord(s, member)) {
                ui->dollMask ^= 1u << member;
                sessionBagsClose(s, member);
                effect(ui, 1);
                if (!ui->dollMask) {
                    inventoryClose(ui);
                }
            }
        } else if (region == 1) { /* the right button on a bag slot uses the item */
            unsigned doll = uiRegionHit(game, UiRegionsRosterPortraits, x, y);
            if (doll >= 1 && doll <= 4 && (ui->dollMask & (1u << (doll - 1)))) {
                unsigned slot = uiRegionHit(game, UiRegionsInventoryGrid, x - (8 + 56 * (int)(doll - 1)), y - 8);
                SessionBag bag = slot ? sessionBagToggle(s, doll - 1, slot) : SessionBagIgnored; /* HandleInventoryGridClick: a container opens or closes, the open bag's icon closes the newest */
                if (bag == SessionBagRefused) {
                    flashWarning(ui);
                } else if (bag == SessionBagIgnored && slot >= 1 && slot <= 8) {
                    inventoryUse(ui, doll - 1, slot);
                }
            }
        }
        return true;
    }
    if (region == 5 && ui->catalogActive) { /* the chest's slots under the panels */
        unsigned slot = uiRegionHit(game, UiRegionsCatalogSlots, x, y);
        if (slot) {
            catalogSlotClick(ui, slot - 1);
        }
    } else if (region == 1) {
        inventorySlotClick(ui, x, y);
    } else if (region == 2) { /* the belt works here too */
        panelClick(ui, x, y);
    } else if (region == 6) {
        unsigned panel = uiRegionHit(game, UiRegionsPartyPanels, x, y), member = panel / 10, part = panel % 10;
        if (panel && member < 4 && sessionPartyRecord(s, member)) {
            if (s->heldItem) {
                if (part == 1) {
                    dropOnPortrait(ui, member);
                } else {
                    flashWarning(ui);
                }
            } else if (partyInfoKindForRegion(part) >= 0) {
                partyInfoShow(ui, member, partyInfoKindForRegion(part));
            }
        }
    }
    return true;
}

static bool clickDispatch(GameUi *ui, int x, int y, bool right) {
    ui->session->loot.pending = false; /* the loot panel stays until the next command */
    if (ui->passwordActive) { /* EditTextField takes no clicks */
        return true;
    }
    if (ui->scrollAsk) {
        int row = gameDialogPromptClick(x, y);
        if (right) {
            ui->scrollAsk = false;
        } else if (row >= 0) {
            scrollAnswer(ui, row);
        }
        return true;
    }
    if (ui->pickActive) { /* a face is the answer; the right button gives up */
        unsigned panel = uiRegionHit(ui->session->game, UiRegionsPartyPanels, x, y), member = panel / 10;
        if (right) {
            pickCancel(ui);
        } else if (panel % 10 == 1 && member < 4 && sessionPartyRecord(ui->session, member)) {
            pickAnswer(ui, member);
        }
        return true;
    }
    if (ui->catalogActive && ui->shopFromTalk && ui->overlay == UiOverlayTalk) { /* RunShopScreen with the dialog up: the first row of the topic list (DONE) leaves; the rest is the merchant's */
        GameKind game = ui->session->game;
        if (!right && uiRegionHit(game, UiRegionsDungeonMain, x, y) == 1) {
            if (!ui->session->heldItem && x >= 58 && x <= 135 && y >= 32 && y <= 37 && ui->session->talk.listedCount) {
                catalogLeave(ui);
            }
            return true;
        }
        return catalogClick(ui, x, y, right);
    }
    if (ui->dropPrompt) {
        int answer = gameDialogPromptClick(x, y);
        if (answer >= 0) {
            dropAnswer(ui, answer == 1);
        }
        return true;
    }
    if (ui->overlay == UiOverlayDolls) {
        return dollsClick(ui, x, y, right);
    }
    if (ui->targeting) { /* a left click on the view is the target; the right button and every other place are ignored */
        if (!right && uiRegionHit(ui->session->game, UiRegionsDungeonMain, x, y) == 1) {
            unlockTry(ui);
        }
        return true;
    }
    if (ui->overlay == UiOverlayMount) { /* a left click names the cell to fly to; the right button is not heard */
        if (!right) {
            mountClick(ui, x, y);
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
    case UiOverlayAlchemy:
        return alchemyClick(ui, x, y);
    case UiOverlayLocalMap:
    case UiOverlayWorldMap:
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

static bool gameUiClickInner(GameUi *ui, int x, int y, bool right);

bool gameUiClick(GameUi *ui, int x, int y, bool right) {
    bool result = gameUiClickInner(ui, x, y, right);
    playPending(ui);
    return result;
}

static bool gameUiClickInner(GameUi *ui, int x, int y, bool right) {
    wipeCheck(ui);
    clueSettle(ui);
    if (ui->vision) {
        ui->vision = false;
        sessionVisionEnd(ui->session);
        return true;
    }
    if (ui->restPause) { /* the interrupted rest's message is read: any click lets the combat show */
        ui->restPause = false;
        ui->panelRunCount = 0;
        return true;
    }
    unsigned list = ui->mountList;
    ui->mountList = 0;
    ui->panelRunCount = 0;
    if (list && !right && ui->overlay == UiOverlayNone && x >= 240 && x <= 311 && y >= 96 && y <= 191 && (y - 96) / 6 + 1 >= 2 && (y - 96) / 6 + 1 <= 5) {
        mountStart(ui, list - 1, (unsigned)((y - 96) / 6 + 1)); /* the region table 0x5D5E: the lines of the TRANSPORTS list are the abilities 2-5 */
        return true;
    }
    bool running = clickDispatch(ui, x, y, right);
    restReport(ui);
    lockReport(ui);
    return running;
}
