#ifndef YENDOR23_GAMEUI_H
#define YENDOR23_GAMEUI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
typedef enum { UiOverlayNone, UiOverlayLocalMap, UiOverlayDialog, UiOverlayDetail, UiOverlayDolls, UiOverlayTalk, UiOverlayShop, UiOverlayTitle } UiOverlay;

typedef enum {
    UiKeyNone = 0,
    UiKeyEscape = 27,
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
    bool debugMarkers;     /* print the monster markers in view to stderr */
} GameUi;

/* worldDat must stay valid for the life of the UI. False if the view tables are missing from it. */
bool gameUiInit(GameUi *ui, GameSession *session, const uint8_t *worldDat, size_t worldSize, ViewPictureFn picture, void *pictureCtx, const GameUiHost *host);

/*
 * The title menu (titlemenu.h: PICTURES category 0 picture 2 with its five commands baked in) as the first screen: E enters the game (the overlay closes), I plays the opening
 * cinematic through the host ("opening"), R the credits ("credits"; Chapter 2), Escape or Ctrl+Q quits. The roster, the world map and character creation screens are not driven from here (creation.h
 * builds heroes; sessionSetParty installs them).
 */
void gameUiShowTitle(GameUi *ui);

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
