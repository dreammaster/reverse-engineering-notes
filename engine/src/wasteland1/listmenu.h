#pragma once
// The paged list used by the skill / item / roster screens of WL.EXE (0x16D34..0x16EE8 and the screens built on it): a window of
// up to nine rows is drawn through a row callback (address at ds:4696), keys 1..9 pick a row, I / K and the cursor keys page.
#include "wasteland1/machine.h"

namespace wl {

struct ListResult {
    bool cf = false;   // CF of the original: set when no row was picked
    uint8_t al = 0;
    bool empty = false;   // there was nothing to list (the original returns before drawing)
};

// 16EAA: sets up the list window (first text line, nine rows)
void listWindowSetup(Machine &m);
// 16DB4: draws the page that starts at the item numbered ds:4699
void pagedListDraw(Machine &m);
// 16E43: draws the previous page
void pagedListBack(Machine &m);
// 16D34: waits for a key and returns the picked row's value (CF clear) or a key code: FFh ESC, FEh the extra key at ds:470E,
// FDh ENTER / SPACE (CF set)
ListResult pagedListKeys(Machine &m);
// 16F20: marks the picked row (ds:46A0) with the inverse pointer
void markPickedRow(Machine &m);

// 19582: row callback of the skill list: draws skill number `index`; false (CF) when the slot is empty
bool skillRow(Machine &m, uint8_t index);
// 195C6: row callback of the item list: draws the item in slot `index`; false when the slot is empty
bool itemRow(Machine &m, uint8_t index);

// 1997B: shows the list of the current character's skills and runs the keys; `title` is the letter shown after the heading
ListResult skillList(Machine &m, uint8_t title);
// 198F0: the skill list screen with the option to rearrange the skills
ListResult skillScreen(Machine &m);
// 19454: shows the list of the current character's items and runs the keys
ListResult itemList(Machine &m, uint8_t title);
// 19394: the item list screen with the option to rearrange the items
ListResult itemScreen(Machine &m, uint8_t member);

} // namespace wl
