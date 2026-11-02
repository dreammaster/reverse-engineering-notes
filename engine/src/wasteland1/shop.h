#pragma once
// Town shops: the doctor (WL.EXE 0x1C260..0x1C561) and the money helpers they share.
#include "wasteland1/machine.h"

namespace wl {

// 17B15: takes the amount in numberValue from the current character's money; false (CF clear) when there is not enough
bool payNumberValue(Machine &m);
// 19B81: gives all the group's money to party member `member`
void poolMoney(Machine &m, uint8_t member);
// 1C260: the doctor: heals hit points, cures afflictions and talks about prices; always returns true (CF)
bool doctorShop(Machine &m);

// 1BB32: maximum hit points += amount; true when the result has the sign bit (the caller then clamps it)
bool raiseMaxCon(Machine &m, uint8_t amount);
// 1BB5D: message `id` of the training texts
void trainingMessage(Machine &m, uint8_t id);
// 1BB6C: copies the rank title `index` into the current character's rank field
void setRankTitle(Machine &m, uint8_t index);
// 1BA72: the player spends ds:4687 attribute points of party member `member` (keys 1..7 attributes, 8 hit points)
void distributePoints(Machine &m, uint8_t member);

} // namespace wl
