#ifndef YENDOR23_GAMEUI_H
#define YENDOR23_GAMEUI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cluebook.h"
#include "clueentries.h"
#include "cluemap.h"
#include "cluemonster.h"
#include "cluepaged.h"
#include "creationwizard.h"
#include "cluespell.h"
#include "exedata.h"
#include "session.h"
#include "viewrender.h"

/*
 * The playable game as a platform-independent front end: the screens the original draws around a GameSession (the first-person view with the minimap and the four panels,
 * the local map, the pause dialog, the hero sheets, the paper dolls, the NPC talk and the shop) and the key bindings that drive the session. A platform (src23/tools/explore_sdl.c
 * for SDL2; a ScummVM engine would be the other) owns the window, the keyboard, the clock and the sound, and talks to this module through three things:
 *   gameUiKey      a key press (UiKey; letters and digits are their lower-case ASCII), which returns false when the player quits;
 *   gameUiDraw     renders the 320 x 200 screen of palette indices into `screen` (the palette is the master palette, gameUiPalette);
 *   GameUiHost     what the game asks of the platform: a sound effect, a music track, a cinematic (the ending), reading and writing a save image.
 * Time and the ailment sweep are the platform's to call (sessionAdvanceTime, sessionTickAilments); gameUiKey runs the sweep once per command, like one pass of the loop.
 * Everything that needs a picture file goes through the ViewPictureFn given at init.
 */
typedef enum { UiOverlayNone, UiOverlayLocalMap, UiOverlayDialog, UiOverlayDetail, UiOverlayDolls, UiOverlayTalk, UiOverlayShop, UiOverlayTitle, UiOverlayClue, UiOverlayRoster, UiOverlayCreate } UiOverlay;

typedef enum {
    UiKeyNone = 0,
    UiKeyEscape = 27,
    UiKeyBackspace = 8,
    UiKeyEnter = 13,
    UiKeyUp = 0x100,
    UiKeyDown,
    UiKeyLeft,
    UiKeyRight,
    UiKeyF1 = 0x110, /* F1 .. F9 */
    UiKeyF2,
    UiKeyF3,
    UiKeyF4,
    UiKeyF5,
    UiKeyF6,
    UiKeyF7,
    UiKeyF8,
    UiKeyF9
} UiKey;

typedef struct {
    void *ctx;
    void (*effect)(void *ctx, unsigned id);        /* play sound effect `id` (the WORLD.DAT VOC list) */
    void (*music)(void *ctx, unsigned track);      /* play music track `track` (0 = silence) */
    bool (*cinematic)(void *ctx, const char *kind); /* play "ending" (introstory.h); false when it could not */
    bool (*store)(void *ctx, const uint8_t *image, size_t size); /* F5: write the save image */
    size_t (*fetch)(void *ctx, uint8_t *image, size_t capacity); /* F9: read it back; 0 when there is none */
} GameUiHost;

typedef struct {
    GameSession *session;
    ViewRenderer renderer;
    uint8_t screen[ViewScreenWidth * ViewScreenHeight];
    uint8_t tables[ViewTablesSize];
    const uint8_t *worldDat;
    size_t worldSize;
    GameUiHost host;
    UiOverlay overlay;
    unsigned hero;         /* the hero of the detail sheet */
    char talkText[1200];   /* what the NPC last said */
    unsigned shopLock;     /* the lock record of the shop being shown */
    unsigned lastMusicPage;
    unsigned castNext;     /* the next spell id the C key tries outside a combat */
    const ExeData *exe;    /* the executable's data segment (the clue book's texts); NULL = no clue book */
    CreationWizard wizard; /* the character creation under way (UiOverlayCreate) */
    unsigned wizardSlot;   /* the roster record it will be stored in */
    unsigned rosterCursor; /* the roster screen's slot (0-8) */
    bool clueRegistered;   /* the program counts as registered: the walk through goes past page 5 (else page 6 is refused with the reminder, as in the shareware) */
    bool clueNag;          /* the reminder is showing on the page */
    int clueMarker;        /* the map page's legend marker whose label shows, -1 none */
    unsigned clueCategory; /* the clue category (clueentries.h, 1-17) whose list or entry the screen shows */
    unsigned clueScreen;   /* 0 the walk through (or help), 1 the category's list, 2 an entry's page */
    ClueList clueList;     /* the list's paging */
    ClueEntry clueEntries[ClueEntriesMax]; /* the list's entries */
    unsigned clueParent;   /* the row chosen in the list one level up (a class, an item group), to return to */
    unsigned clueSub;      /* the item page's "+N" variant */
    bool clueHelp;         /* the clue book shows its help screen (Tab), else the walk through */
    unsigned cluePage;     /* the walk through's page, 1-based */
    bool debugMarkers;     /* print the monster markers in view to stderr */
} GameUi;

/* worldDat must stay valid for the life of the UI. False if the view tables are missing from it. */
bool gameUiInit(GameUi *ui, GameSession *session, const uint8_t *worldDat, size_t worldSize, ViewPictureFn picture, void *pictureCtx, const GameUiHost *host);

/*
 * The title menu (titlemenu.h: PICTURES category 0 picture 2 with its five commands baked in) as the first screen: C character creation (UiOverlayCreate, creationwizard.h; the new
 * hero goes into the first empty roster slot; refused with the beep when the roster is full), A the party roster ("Assemble a Party", UiOverlayRoster: the nine heroes, arrows
 * move, Enter or Space puts a hero in or out of the party of four, D or Escape returns), E enters the game (the overlay closes; refused with the beep, effect 3, while no hero is in the
 * party), I plays the opening cinematic through the host ("opening"), R the credits ("credits"), Escape or Ctrl+Q quits.
 */
void gameUiShowTitle(GameUi *ui);

/* Hands the UI the executable (it must stay valid) so that F8, the clue book, can open. It has the walk through (F6, I / Q turn pages), the help screen (Tab) and the category lists F1 maps, F2 monsters, F3 spells, F4 magic users (a class, then its spells), F5 items (a group, then its items): Up / Down / I / Q choose, Enter opens, Escape backs out; the entries are the executable's own lists (clueentries.h). */
void gameUiSetExe(GameUi *ui, const ExeData *exe);

/* One key press; false = quit (Escape with no overlay open). */
bool gameUiKey(GameUi *ui, UiKey key, bool ctrl);

/* Renders the current screen (the view and panels, or the open overlay) into ui->screen. */
void gameUiDraw(GameUi *ui);

/* The master palette: 256 x 3 six-bit DAC values inside the world data; NULL if it is not there. */
const uint8_t *gameUiPalette(const GameUi *ui);

/* Starts the area's music when the party has moved to a page with another track (call after loading or placing the party). */
void gameUiAmbientMusic(GameUi *ui);

/* A one-line status for a window title. */
void gameUiStatus(const GameUi *ui, char *out, size_t capacity);

#endif
