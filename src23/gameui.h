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
#include "confirmprompt.h"
#include "gamedialog.h"
#include "palette.h"
#include "partyinfo.h"
#include "textfield.h"
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
typedef enum { UiOverlayNone, UiOverlayLocalMap, UiOverlayDialog, UiOverlayDetail, UiOverlayDolls, UiOverlayTalk, UiOverlayTitle, UiOverlayClue, UiOverlayRoster, UiOverlayCreate, UiOverlayAlchemy, UiOverlayWorldMap } UiOverlay;

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
    UiKeyF9,
    UiKeyPageUp = 0x120,
    UiKeyPageDown
} UiKey;

typedef struct {
    void *ctx;
    void (*effect)(void *ctx, unsigned id);        /* play sound effect `id` (the WORLD.DAT VOC list) */
    void (*music)(void *ctx, unsigned track);      /* play music track `track` (0 = silence) */
    bool (*cinematic)(void *ctx, const char *kind); /* play "ending" (introstory.h); false when it could not */
    bool (*store)(void *ctx, const uint8_t *image, size_t size); /* F5: write the save image */
    size_t (*fetch)(void *ctx, uint8_t *image, size_t capacity); /* F9: read it back; 0 when there is none */
    /* The six save slots of the pause dialog (SAVGAME1 .. SAVGAME6 beside the game's files, savegame.h; each is a whole save image): */
    bool (*slotName)(void *ctx, unsigned slot, char *name, size_t capacity); /* false when the slot file does not exist, else the name saved in it (the header's first string) */
    bool (*slotWrite)(void *ctx, unsigned slot, const uint8_t *image, size_t size);
    size_t (*slotRead)(void *ctx, unsigned slot, uint8_t *image, size_t capacity); /* 0 when it cannot be read */
    bool (*soundBusy)(void *ctx); /* optional: a sound effect is still playing (the original's WaitForSoundDriverIdle); without it queued sounds are all started at once */
} GameUiHost;

/* Where the start-up (gameUiBoot) has got to: the dialog with LOAD / NEW GAME / DOS that opens when there are saves, the title menu when there are none, then the options dialog before the game. */
typedef enum { UiBootNone, UiBootSaves, UiBootTitle, UiBootOptions } UiBoot;

/* What the pause dialog is doing: showing its options, a slot list of SAVE or LOAD, a name being typed for a save, or asking a question. */
typedef enum { UiDialogMenu, UiDialogSlots, UiDialogName, UiDialogPrompt } UiDialogMode;

/* What the alchemy screen is doing: showing its list, waiting for the portrait a single-target spell is for, or asking whether to mark or return. */
typedef enum { UiAlchemyList, UiAlchemyTarget, UiAlchemyMark } UiAlchemyMode;

enum { UiAlchemyMax = 128, UiPanelRunMax = PartyInfoMaxRuns };

/* PanelRun (partyinfo.h): a piece of text written into the message area of the right-hand panel: where, in which colour, over the panel (opaque) or not. */

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
    char talkRows[16][DialogLineChars + 1]; /* the same in the rows the screen draws it in */
    unsigned talkRowCount;
    bool talkListShown;    /* the topic list is up: it comes after the greeting has been read (any key or click) */
    bool shopFromTalk;     /* the merchant's screen was opened from a conversation, which goes on when it is left */
    unsigned lastMusicPage;
    PanelRun panelRuns[UiPanelRunMax]; /* the message area's text: what the last command wrote (a rest's result); cleared by the next key or click */
    unsigned panelRunCount;
    unsigned soundFifo[16]; /* sounds waiting for the one playing to end (sessionNextSound's queue, drained as the host reports it idle) */
    unsigned soundFifoCount;
    bool wipe;             /* the party is dead: the empty frame and the LOAD / NEW GAME / DOS dialog (CheckPartyWipeAndReinitLevel) */
    bool combatPanel;      /* a fight is on: the top-right panel shows the resource icons and belt (RunDungeonGameLoop's ShowResourceDepletedOverlay) ... */
    uint16_t combatPanelSaved; /* ... and the panel bits (0x7000) it replaced, put back when the fight ends */
    bool restPause;        /* an interrupted rest's message stays until a key (the combat's panels wait behind it) */
    int mouseX, mouseY;    /* the pointer, from gameUiMouseMove: where the cursor item is drawn */
    bool targeting;        /* K: the crosshair cursor waits for a click on the view (WaitForTargetClick) */
    unsigned dollMask;     /* the inventory screen (UiOverlayDolls): bit n = member n's expanded panel is shown */
    bool catalogActive;    /* a chest's contents screen: the eight slots under the walk pad's place (RunShopScreen) */
    unsigned catalogLock;  /* its lock record */
    bool catalogShop;      /* ... a merchant's stock rather than a chest */
    unsigned pickPrompt;   /* the message shown while a member is picked (0 = none: the blank one): 5 / 6 WHO WILL OPEN? / SEARCH? */
    bool pickChest;        /* the member is the one who opens the chest ahead, not the recipient of an item */
    bool pickActive;       /* an item waits for the member it is used on (ConfirmAndSelectPartyTarget): a click on a face or F1-F4 */
    unsigned pickUser, pickSlot; /* ... the member whose bag slot (1-8) it is in */
    bool reading;          /* the scroll (UiOverlayTalk) shows a readable item's text, not a conversation */
    bool scrollAsk;        /* a spell scroll that can be studied asks USE ITEM? / LEARN SPELL? (message 0x20) */
    unsigned scrollUser, scrollSlot; /* ... the member and item slot (1-8 bag, SessionBeltSlotBase + n belt) of the scroll in use */
    unsigned pickScroll;   /* the member picked is a scroll's: 1 = the cast's recipient (message 0x12), 2 = who learns it (0x21) */
    bool dropPrompt;       /* the cursor item is about to be dropped: DO YOU WANT TO DROP THAT RIGHT NOW? (TryDropHeldItem) */
    bool passwordActive;   /* a destination's password is being typed in the message area (ShowConfirmPrompt, mode 0x2000) */
    unsigned passwordPrompt; /* its message id */
    TextField password;
    UiAlchemyMode alchemyMode;
    unsigned alchemyCaster; /* the party slot (0-3) whose spells the screen lists */
    unsigned alchemyCount;  /* the list: the spells the caster knows, in id order */
    unsigned alchemyIds[UiAlchemyMax];
    bool alchemyOk[UiAlchemyMax]; /* castable now (shown white, else grey) */
    unsigned alchemySelected;
    unsigned alchemySpell;  /* the spell waiting for its target / mark answer */
    const ExeData *exe;    /* the executable's data segment (the clue book's texts); NULL = no clue book */
    CreationWizard wizard; /* the character creation under way (UiOverlayCreate) */
    unsigned wizardSlot;   /* the roster record it will be stored in */
    unsigned doorTicks;    /* animation ticks until an opening door (session.h) is passed */
    uint8_t palette[768];  /* the palette on screen: the master palette, with entries 0xD0-0xDF cycled (gameUiAnimate) */
    unsigned paletteStamp; /* counts the changes of `palette`: a platform that has converted it reconverts when this moves */
    unsigned cycleStep;    /* which of the four rotations of the cycled colours is next (0-3) */
    unsigned cycleTicks;   /* BIOS ticks (18.2 a second) since the last rotation; one comes every 5 */
    DayNightFade fade;     /* the dawn / dusk fade of the sky colours 0xE0-0xFF under way (palette.h) */
    bool fadeActive;
    unsigned fadeTicks;    /* BIOS ticks since its last step; one step comes every 91 */
    unsigned pressTicks;   /* animation ticks the pressed button below stays down */
    unsigned pressPicture; /* category 8 picture of the pressed button (the pad arrows 18..28, the icon row 2, 4, 6, 8, 10) */
    int pressX, pressY;    /* where it is drawn: the button's corner */
    unsigned driverFlags;  /* the pause dialog's switches (gamedialog.h: music available / on, sound effects available / on) */
    UiBoot boot;
    unsigned dialogFlags;  /* the options the open dialog enables (the GameDialogFlag bits, the original's g_uiScratchFlags1) */
    UiDialogMode dialogMode;
    GameDialogOption dialogOption; /* SAVE or LOAD (the slot list, the name, the question), DOS or NEW GAME (the question) */
    GameDialogPrompt dialogPrompt;
    int dialogSlot;        /* the chosen slot row, -1 none */
    TextField dialogName;  /* the name of the save being made */
    bool dialogNameKept;   /* the slot's own name is showing with the cursor: Enter keeps it, any other key starts a new one */
    SaveSlotEntry slots[SaveSlotCount];
    unsigned animationSpeed; /* 1 fast, 5 medium, 9 slow: what the platform should use for its animation tick */
    unsigned rosterCursor; /* the roster screen's slot (0-8) */
    bool clueRegistered;   /* the program counts as registered: the walk through goes past page 5 (else page 6 is refused with the reminder, as in the shareware) */
    bool clueNag;          /* the reminder is showing on the page */
    int clueMarker;        /* the map page's legend marker whose label shows, -1 none */
    unsigned clueCategory; /* the clue category (clueentries.h, 1-17) whose list or entry the screen shows */
    unsigned clueScreen;   /* 0 the walk through (or help), 1 the category's list, 2 an entry's page */
    ClueList clueList;     /* the list's paging */
    ClueEntry clueEntries[ClueEntriesMax]; /* the list's entries */
    unsigned clueParent;   /* the row chosen in the list one level up (a class, an item group), to return to */
    uint8_t clueMonster[MonsterRecordSize]; /* the monster page's record, animated */
    unsigned clueAnim;     /* its animation counter (cluemonster.h) */
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

/*
 * The start-up the original runs after its boot title and the introduction (InitGame, yendor2.asm:3266): when any of the six save files exists, the pause dialog opens with only LOAD, NEW GAME
 * and DOS (nothing is drawn behind it but the empty frame): LOAD lists the slots, NEW GAME (after the question) starts a new game on the title menu, DOS leaves. When there are none, the new game
 * goes straight to the title menu (UiBootTitle) and its E (enter the game) opens the dialog with SAVE, DOS, ANIMATION and RETURN, which RETURN leaves for the game. A new game has an empty party:
 * E is refused until one is assembled with A.
 */
void gameUiBoot(GameUi *ui);

/* Hands the UI the executable (it must stay valid) so that F8, the clue book, can open. It has the walk through (F6, I / Q turn pages), the help screen (Tab) and the category lists F1 maps, F2 monsters, F3 spells, F4 magic users (a class, then its spells), F5 items (a group, then its items): Up / Down / I / Q choose, Enter opens, Escape backs out; the entries are the executable's own lists (clueentries.h). */
void gameUiSetExe(GameUi *ui, const ExeData *exe);

/*
 * The alchemy screen (RunAlchemyScreen, yendor2.asm:24614; the second icon, the C key; UiOverlayAlchemy): the dialog frame over the view lists, 13 to a page, the spells the member knows -- white when
 * they can be cast now (enough MP, ore and nuore; not a combat-only spell outside a combat nor the reverse), grey when not -- with their MP, NUORE and MAGIC ORE costs, and the status panel shows
 * the member's MP and the party's two ore counters. Up / Down move the selection, I / PageUp and Q / PageDown turn the page, F1-F4 (or a click on a portrait; not in a combat, where the caster is the
 * member whose turn it is) change the member, Space or Enter casts the selected spell, a click selects a row and a second click on it casts (the original casts on a held button), Escape, C or the
 * right button leave. A spell for one member (SpellFlagsASingleTarget) first asks which: a portrait click or F1-F4; the location spell asks MARK or RETURN. Casting pays the costs and closes the screen.
 */

/* One key press; false = quit (Escape with no overlay open, DOS in the dialog, Ctrl+Q on the title menu). */
bool gameUiKey(GameUi *ui, UiKey key, bool ctrl);

/* One animation step of what is on screen: a shot in flight (session.h SessionShot: the sound it asks for is played here), a pressed button coming up, a door opening, or the monster page of the clue book (cluemonster.h clueMonsterAnimStep; the hit sound goes to the host). The platform calls it once per
 * animation tick (the speed of the pause dialog; the original steps an attacking monster every 2 ticks) and redraws; true when the screen changed. */
bool gameUiAnimate(GameUi *ui);

/* Finishes what animation ticks would (a door that is opening is passed now, a shot in flight is over): for a platform that is about to take a screenshot or save. */
void gameUiSettle(GameUi *ui);

/* A mouse click at (x, y) in the 320 x 200 screen (`right` for the right button): the same commands as the keys, through the original's click regions (uiregions.h) -- the title menu, the roster
 * (a face selects, a check box puts the hero in or out of the party, DONE returns), the creation wizard's lists, the clue book (tabs, entries, page arrows, legend markers), the pause dialog,
 * conversations and shops, and on the main screen the icon row, the walk pad, the map panel and the party panels. The right button backs out of an overlay. False = quit. */
bool gameUiClick(GameUi *ui, int x, int y, bool right);

/* Renders the current screen (the view and panels, or the open overlay) into ui->screen, with the cursor item or the crosshair over it. */
void gameUiDraw(GameUi *ui);

/*
 * The pointer moved to (x, y) in the 320 x 200 screen. While the party holds an item (session.h heldItem) the item's icon is the cursor (UpdateCursorForHeldItem: the 16 x 16 picture of category 8
 * the catalog record's +8 names) and while K waits for its target a crosshair (picture 0xF); gameUiCursorShown says when the platform should hide its own pointer because the screen draws one.
 */
void gameUiMouseMove(GameUi *ui, int x, int y);
bool gameUiCursorShown(const GameUi *ui);

/*
 * The palette to show: 256 x 3 six-bit DAC values, the master palette in the world data, whose entries 0xD0-0xDF are cycled the way the original's timer interrupt does
 * (AnimatePaletteCycle, yendor2.asm:28023, every 5 ticks while a fade has finished): each of the four groups of four colours (0xD0-0xD3, 0xD4-0xD7, 0xD8-0xDB, 0xDC-0xDF) is rotated
 * by one colour more at every step, 0, 1, 2, 3 and round again. Those colours are the ones the picture shading leaves alone (water, lava, the acting member's frame -- colour 0xDB --
 * and the like), so they shimmer. gameUiAnimate steps it and changes `paletteStamp`; NULL if the world data has no palette.
 *
 * The sky colours 0xE0-0xFF follow the game clock (AdvanceDayNightPaletteFade, yendor2.asm:28157): when the session's clock reaches 06:00 or 18:00 (GameSession.pendingFade) a fade of
 * 113 steps starts, one step -- a sliding window of the palette's sunrise ramp -- every 91 BIOS ticks (five seconds), so dawn and dusk each take about nine minutes of real time; a game
 * loaded, started or moved in time shows the palette of its hour at once.
 */
const uint8_t *gameUiPalette(const GameUi *ui);

/* Starts the area's music when the party has moved to a page with another track (call after loading or placing the party). */
void gameUiAmbientMusic(GameUi *ui);

/* A one-line status for a window title. */
void gameUiStatus(const GameUi *ui, char *out, size_t capacity);

#endif
