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

} // namespace wl
