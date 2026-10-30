#pragma once
// The command menu of a combat round: every able party member is asked what to do (WL.EXE 0x11F76..0x1243F). A command handler
// returns CF in the original (set: ask again / cancelled) and a code in AL (the order stored for the round).
#include "wasteland1/machine.h"

namespace wl {

struct CommandResult {
	bool cf = false;
	uint8_t al = 0;
};

// 11F76: asks the party for orders; true (CF) when the player backed out
bool collectOrders(Machine &m);

// the handlers of the command table at ds:A43B; `member` is the party member number
CommandResult commandPause(Machine &m, uint8_t member);        // 1208D
CommandResult commandUseItem(Machine &m, uint8_t member);      // 1234E
CommandResult commandMove(Machine &m, uint8_t member);         // 123A6 (and 123D1 for the direction prompt)
CommandResult commandReload(Machine &m, uint8_t member);       // 123E2
CommandResult commandUseSkill(Machine &m, uint8_t member);     // 1240B
CommandResult commandAttack(Machine &m, uint8_t member);       // 120C1
// 121A8: picks the first monster group in range (shooting at the nearest one) and returns its code
CommandResult pickFirstTarget(Machine &m);

} // namespace wl
