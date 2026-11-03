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

// 1C68E: price of the next skill level: `base` * 2^(level - 1), FFh when it overflows
uint8_t skillCost(uint8_t base, uint8_t level);
// 1BDFF: finds skill `id` in the current character's skill list; `offset` is the slot (skillsEnd when missing)
bool findSkillSlot(Machine &m, uint8_t id, uint8_t &offset);
// 1BD37: builds the list of skills the current character is clever enough for
void buildTrainableList(Machine &m);
// 1BD94: row of the training list; true (CF) when the skill is not shown
bool trainingRow(Machine &m, uint8_t index);
// 1BD94: the same for a skill number
bool trainingRowOfSkill(Machine &m, uint8_t skill);
// 1BBA0: the skill school: spend skill points on new skills and levels; always returns true
bool skillSchool(Machine &m);

// 17AF5: the shop deals in the type of the item in the details record
bool itemTypeAccepted(Machine &m);
// 1C1C2 / 1C1CC: numberValue = the selling / buying price of the item in the details record
void sellPrice(Machine &m);
void buyPrice(Machine &m);
// 1963F: number of the current character's items the shop buys (also sets ds:4721)
uint8_t countSellable(Machine &m);
// 1C01B: row of the sell list
bool sellRow(Machine &m, uint8_t index);
// 1BF5F: the sell screen
void sellScreen(Machine &m);
// 1C140: number of items the shop has in stock
uint8_t countStock(Machine &m);
// 1C188: row of the buy list (the item number is the table index)
bool buyRow(Machine &m, uint8_t item);
// 1C073: the buy screen
void buyScreen(Machine &m);

} // namespace wl
