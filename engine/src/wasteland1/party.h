#pragma once
// Party, character and item record access (WL.EXE 0x17208..0x19AC8). Everything works on the data segment image so
// that results can be compared byte for byte with the original.
#include "wasteland1/machine.h"

namespace wl {

// offsets inside a 0x100 byte character record (the IDB's PartyMember struct)
namespace pm {
constexpr uint8_t name = 0x00, strength = 0x0E, iq = 0x0F, luck = 0x10, speed = 0x11, agility = 0x12, dexterity = 0x13,
                  charisma = 0x14, money = 0x15 /*3 bytes*/, sex = 0x18, nationality = 0x19, armorClass = 0x1A,
                  maxCon = 0x1B /*word*/, con = 0x1D /*word*/, weapon = 0x1F, skillPoints = 0x20, experience = 0x21 /*3*/,
                  level = 0x24, armor = 0x25, lastCon = 0x26 /*word*/, afflictions = 0x28, npc = 0x29, itemRefuse = 0x2B,
                  skillRefuse = 0x2C, attribRefuse = 0x2D, tradeRefuse = 0x2E, joinString = 0x30, willingness = 0x31,
                  rank = 0x32 /*25 bytes*/, skills = 0x80 /*30 x {id, level}*/, skillsEnd = 0xBC, items = 0xBD /*30 x {id, qty}*/,
                  inventoryEnd = 0xF9;
}

// sub_17208: points partyIndexesPtr at the 14 byte record of `group` inside the savegame image
void setPartyIndexesPtr(Machine &m, uint8_t group);
// sub_19614: makes party member `partyNum` (1 based within the active group) the current character
void setCurrentCharacter(Machine &m, uint8_t partyNum);
// sub_172BB: true when member `partyNum` is unconscious or dead (hit points <= 0)
bool isMemberUnconscious(Machine &m, uint8_t partyNum);
// sub_172AE: AL = low | high hit point byte of the current character (zero flag: no hit points at all)
uint8_t conBytesOr(Machine &m);
// sub_19A1D: index into the STATUSES table for the current character's hit points
uint8_t getMemberConditionIndex(Machine &m);
// sub_198CD: skill level of the current character (0 when he does not know the skill)
uint8_t getMemberSkillLevel(Machine &m, uint8_t skillId);
// sub_1968C: offset of the inventory slot holding `itemId` in the current character, or -1
int findItemById(Machine &m, uint8_t itemId);
// sub_19AC8: offset (inside the character record) of the inventory entry of slot `slot` (1 based)
uint8_t getInventoryItemOffset(uint8_t slot);
// sub_196C9: offset of the equipped weapon's inventory entry; 0 when nothing is equipped (item id read separately)
uint8_t getMemberWeaponOffset(Machine &m, uint8_t &itemIdOut);

// sub_16F4C / sub_17AE0: itemDetailsPtr = base + id * recSize
void getItemDetailsInner(Machine &m, uint8_t id);
void getItemDetails(Machine &m, uint8_t id);              // item table: 8 byte records after an 8 byte header
void getItemDetails2(Machine &m, uint8_t skillId);        // skill -> attribute table (2 byte records)
uint8_t getItemType(Machine &m);                          // flags >> 3 of the current item record
uint8_t itemFlag(Machine &m, uint8_t offset);             // byte at itemDetailsPtr + offset

// ---- party groups (the savegame holds up to four parties that walk the map separately) ----------------------------------------
// 171E3: selects the party record of `group` and counts its members (consecutive non-zero indexes from offset 1); returns the
// count; `stop` receives the offset where the scan ended (the original's bl)
uint8_t countPartyMembers(Machine &m, uint8_t group, uint8_t *stop = nullptr);
// 16185: stores the party position (46A6/46A7) and the map id into the active party record
void storePartyPosition(Machine &m);
// 16149: stores the position of the active group and makes `group` the active one (position, member count, map id)
void setActivePartyGroup(Machine &m, uint8_t group);
// 14A65: smallest value of the byte table at ds:7111 over the party groups 0..total; returns the value and its index
uint8_t smallestGroupValue(Machine &m, uint8_t &index);
// 14AE1 / 17FC8: scratch helpers of the movement code: the pending cell (A60D, A60C) and the pointer 3448h + row * width
void pendingCell(Machine &m, uint8_t &x, uint8_t &y);
void setViewRowPointer(Machine &m, uint8_t row);
// 12738: shift left by four (result 8 bit)
uint8_t shiftLeft4(uint8_t v);

// 17FEE: map cell (x, y) -> screen position; x = (x - viewX) * 2, y = (y - viewY) << 4 (both 8 bit). Returns true (CF) when the
// view is shown and the row is 0Dh or more
bool cellToScreen(Machine &m, uint8_t &x, uint8_t &y);
// 18016: true (CF) when (x, y) lies outside the map size (ds:46A5 wide, ds:46A4 high)
bool outsideMap(Machine &m, uint8_t x, uint8_t y);
// 18D8E: 0 when the zero terminated strings at textString and itemDetailsPtr are equal, FFh otherwise
uint8_t compareTextWithRecord(Machine &m);
// 1718B: overwrites the character record `source` (dl) with the record `dest` (al); nothing when source is 0
void copyCharacterRecord(Machine &m, uint8_t dest, uint8_t source);

// ---- moving characters between party groups -----------------------------------------------------------------------------------
// 17277: position (1..7) of roster number `id` in the selected group's list; 0 when it is not there (CF in the original)
uint8_t findInSelectedGroup(Machine &m, uint8_t id);
// 171D3: appends roster number `id` to the selected group's list and counts it in membersInGroup
void appendToSelectedGroup(Machine &m, uint8_t id);
// 16078: removes roster number `id` from the active group's list (the members behind it move up)
void removeFromActiveGroup(Machine &m, uint8_t id);
// 1619F: clears the 14 byte record of party group `group`
void clearPartyGroup(Machine &m, uint8_t group);
// 16018: moves the character at list position `position` of group `from` into group `to`; false (CF) when the position is empty or
// the character is already in the target. `movedId` is the roster number
bool moveToGroup(Machine &m, uint8_t position, uint8_t from, uint8_t to, uint8_t &movedId);
// 172D4: deletes the roster character at list position `position` of the active group: later characters move down one roster
// number, the lists of all groups are renumbered and the counts reduced
void deleteCharacter(Machine &m, uint8_t position);
// 15DAF: merges another group standing at the same position of the same map into the active group and closes the gap in the
// group table; false (CF set in the original) when there is none
bool mergeGroupsAtSamePosition(Machine &m);

} // namespace wl
