#pragma once
// Party, character and item record access (WL.EXE 0x17208..0x19AC8). Everything works on the data segment image so
// that results can be compared byte for byte with the original.
#include "wl/machine.h"

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

} // namespace wl
