#ifndef YENDOR23_MAININPUT_H
#define YENDOR23_MAININPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "movement.h"

/*
 * The main loop's keyboard commands (the two jump tables read by `start`: `cs:[0B07h + 2 * (key - 0x20)]` for ASCII keys and
 * `cs:[0BC7h + 2 * (scan - 0x3B)]` for extended keys in Chapter 2, 0x777 / 0x837 in Chapter 3; yendor{2,3}/ida_scripts/dump_main_key_table.py).
 * A key whose table entry is zero is ignored. Mouse clicks reach the same commands through the click regions (uiregions.h).
 *
 *   key        command                           notes
 *   Space      examine the facing tile           ProbeFacingTile, then a status panel message if nothing is there
 *   1-4        party member panel                HandlePartyStatusPanelInput (index 0-3)
 *   F1-F4      member detail screen              RunPartyMemberDetailScreen (index 0-3)
 *   F5         toggle the monster / status panel mode (bits 0x8000 / 0x4000 / 0x1000 of the panel mode word)
 *   F8         the clue book
 *   Up/Down    walk forward / back               HandleMovementInput (extended scan 0x48 / 0x50)
 *   Left/Right turn left / right                 (0x4B / 0x4D);  Ctrl+Left / Ctrl+Right (0x73 / 0x74) strafe
 *   A          attack / ranged action            only while a combat is active (flag 0x1000 of g_uiScratchFlags4)
 *   S          act outside combat                the same handler with flag 0x100 set; only when no combat is active
 *   C          alchemy screen
 *   D          pause dialog                      (gamedialog.h)
 *   K          unlock a door                     needs the key item (id below)
 *   M          local area map                    needs the map item
 *   W          toggle the map view mode          needs the item; Chapter 2 only
 *   T          the game clock                    needs the clock item
 *   P          party inventory screen
 *   R          rest                              RestPartyAndAdvanceClock
 *   V          a status panel message            Chapter 3 only
 * Chapter 2's debug mode (flag 0x8000 of g_uiScratchFlags1, from the command line) adds X (run the monsters), Y / Z (interact with the
 * facing tile), I (title screen), L (map editor), B ! @ # $ % ^ 0 (teleport, set a floor / overlay tile, hide a cell ...), Ins, Del and the
 * backquote / tilde keys (position overlay); Chapter 3's tables ignore all of those.
 */
typedef enum {
    MainActionNone,
    MainActionExamine,
    MainActionPartyPanel,
    MainActionMemberDetail,
    MainActionPanelMode,
    MainActionClueBook,
    MainActionMove,
    MainActionAttack,
    MainActionAct,
    MainActionAlchemy,
    MainActionGameDialog,
    MainActionUnlockDoor,
    MainActionLocalMap,
    MainActionToggleMapView,
    MainActionClock,
    MainActionPartyInventory,
    MainActionRest,
    MainActionStatusMessage,
    MainActionDebug
} MainAction;

typedef struct {
    MainAction action;
    unsigned index;          /* party member 0-3 for the panel / detail commands */
    MovementAction movement; /* for MainActionMove */
} MainCommand;

/* `key` is an ASCII code (upper case) for extended = false, a scan code (0x3B-0x76) for extended = true. */
MainCommand mainCommandForKey(GameKind game, bool extended, uint8_t key);

/* The item id the party must carry for the command (0 = none needed): Chapter 2 K 0x2F, M 0x1E, W 0x1F, T 7; Chapter 3 K 0x32, M 0x33, T 0x0C. */
unsigned mainCommandRequiredItem(GameKind game, MainAction action);

/*
 * The keys while a combat round waits for the player's command (HandleDungeonInput, yendor2.asm:10251; Chapter 3 the same): A attack
 * (the staged attack against the selected monster), C alchemy, D the pause dialog, P party inventory, 1-4 a party panel; any extended key opens
 * the member detail screen. Clicks reach them through the icon row (regions 1 attack, 2 alchemy, 4 pause dialog), the monster panels (select a
 * target or set one of four monster order flags) and the portrait strip.
 */
MainCommand combatCommandForKey(bool extended, uint8_t key);

#endif
