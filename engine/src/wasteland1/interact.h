#pragma once
// Interaction with map objects: matching the condition list of an action record against what a party member carries or can do
// (WL.EXE 0x13E9B..0x14193 and 0x18146..0x180F0). The action record at locationDetailsPtr lists conditions from offset 0Ah in
// byte pairs: byte 1 = type (high three bits) and parameter (low five bits), byte 2 = what to look for, ended by FFh.
#include "wasteland1/machine.h"

namespace wl {

// 1417F / 1418A: type (bits 5..7) and parameter (bits 0..4) of the condition byte at record offset `at`
uint8_t conditionType(Machine &m, uint8_t at);
uint8_t conditionValue(Machine &m, uint8_t at);
// 18146: rolls a skill check for the current character: seed2 gets 2d6 plus the attribute plus 3 * skill. Returns the dice roll,
// or -1 when the roll was below 5 (an automatic failure; CF set in the original)
int skillRoll(Machine &m, uint8_t skillId);
// 180F0: skill check against difficulty `count` (target 15 + 5 * count). True on success; successes may improve the skill and
// give experience
bool skillCheck(Machine &m, uint8_t skillId, uint8_t count);
// 19A58: uses up one of the item at inventory offset `offset` (removing it when it was the last)
void useUpItem(Machine &m, uint8_t offset);
// 14090: looks for an item condition (type 1) naming the item at `itemOffset`; on a match one item is used up. Returns false (CF)
// when the list ended without a match; `at` receives the condition offset
bool matchItemCondition(Machine &m, uint8_t itemOffset, uint8_t &at);
// 140DD: finds a skill condition (type 0) naming `skillId` and rolls the check against its parameter. Returns true (CF) when
// there is no such condition or the check failed; `at` receives the condition offset
bool matchSkillCondition(Machine &m, uint8_t skillId, uint8_t &at);
// 14126: finds an attribute condition (type 2) naming `attributeId` and runs fieldCheck against its parameter; true (CF) when
// there is none or the check failed
bool matchAttributeCondition(Machine &m, uint8_t attributeId, uint8_t &at);

// 14193: applies the effect stored in bytes 8 and 9 of the action record (a field offset and an amount, fixed or dice) to party
// member `member` and shows the record's message (14314 / 14307 keep the text pacing variables around it)
void applyActionEffect(Machine &m, uint8_t member);
// 142ED: shows the record's message (byte 3) with the pacing variables set
void showActionMessage(Machine &m);
// 14296: applyActionEffect for every party member
void applyActionEffectToParty(Machine &m);

// 13EC9: runs the action record of the cell (x, y): every able party member is checked against the condition list, effects and
// messages are applied and the cell changes accordingly. Returns the carry of the cell change; `result` is 1 when the record is
// not interactive, else the number of failed runs
bool evaluateAction(Machine &m, uint8_t x, uint8_t y, uint8_t &result);

// 16D1A: shows the title page text named by record byte `at` when it is not zero
void showRecordString(Machine &m, uint8_t at);

// 18288: the character's refusal counter `field` is tested against a roll with difficulty `max`; true (CF) when the character
// refuses. Refusals raise the counter, passing the test can lower it
bool refusalCheck(Machine &m, uint8_t max, uint8_t field);
// 13C58 (NOT registered as a port: its paths wait for keys and could not be compared with the oracle): the current party member (ds:4661) tries to use skill / item / attribute `param` (mode 0 skill, 1 item, 2 attribute) on
// the object in direction `dir`; the NPC may refuse. Returns the carry of the original; `result` is its al
bool useSkillOnObject(Machine &m, uint8_t mode, uint8_t dir, uint8_t param, uint8_t &result);

} // namespace wl
