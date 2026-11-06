#pragma once
// Town shops: the doctor (WL.EXE 0x1C260..0x1C561) and the money helpers they share.
#include "wasteland1/machine.h"

namespace wl {

// 17B15: takes the amount in numberValue from the current character's money; false (CF clear) when there is not enough
bool payNumberValue(Machine &m);
// 19B81: gives all the group's money to party member `member`
void poolMoney(Machine &m, uint8_t member);
// 1BE50: the general store (buy, sell, pool money); always returns true (CF)
bool generalStore(Machine &m);
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

// 1CAD1: an attribute roll: the best three of five d6 (`carry` is the random generator's carried-over value)
uint8_t rollAttribute(Machine &m, uint8_t carry);
// 1C9DE: gives the new character the starting items from the list at ds:4663
void startingItems(Machine &m);
// 1C622: row of the skill list of character creation
bool creationRow(Machine &m, uint8_t skill);
// 1C6C9: the "create character" screen (the oracle and native runs only agree on dice rolled before the first keyboard poll: each poll stirs the generator with the portrait timer tick of 190F8): rolls attributes, asks name, sex and nationality, spends the skill points
void createCharacter(Machine &m);

// 1A3E1: message `id` of the roster texts
void deleteCharacterText(Machine &m, uint8_t id);
// 160A8 / 160DC: switches to the next group that can act (letting time pass when none can)
void cycleGroups(Machine &m, bool startWithOriginal);
// 1A430: the "delete character" menu
void deleteCharacterMenu(Machine &m);

} // namespace wl
