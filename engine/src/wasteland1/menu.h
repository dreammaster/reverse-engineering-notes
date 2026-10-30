#pragma once
// Key driven menus of WL.EXE: choosing from a hot key string, choosing a party member by number, the direction prompt and the
// screens built from them.
#include "wasteland1/machine.h"


namespace wl {

// 173B0: waits for a key and returns its position in the string at textString (a key that is not in it is ignored); 0xFF when
// ESC was pressed
uint8_t chooseByKey(Machine &m);
// 1721B: waits for a key 1..9 naming a member of the active group (`wide`: all keys 1..membersInGroup can be taken, else only
// 1); returns the number, or 0 on ESC
uint8_t pickGroupMember(Machine &m, bool wide);
// 125BD: asks "Which way?" (I K J L or space); returns the direction 0..4, or -1 when ESC was pressed
int askDirection(Machine &m);

// 13AE4: the "Skill / Item / Attribute" menu: asks what party member `member` should use and on whom; false when cancelled
bool chooseUseAction(Machine &m, uint8_t member, uint8_t &mode, uint8_t &target, uint8_t &what);

// 12AE0: lets the player put the members of the active group in a new order
void rearrangeParty(Machine &m);

// 12284: row of the target list: monster group `index` (1 based over all slots); false (CF) when the group is shown
bool targetRow(Machine &m, uint8_t index);
// 12262: opens the target list for `groups` entries
void showTargetList(Machine &m, uint8_t groups);
// 1236F: lets the player pick one of the monster groups in range (readiness ds:46CC set to FFh = any weapon); returns false (CF)
// when cancelled or when there is nothing to pick; `group` is the 0 based list position
bool chooseTargetGroup(Machine &m, uint8_t &group);

} // namespace wl
