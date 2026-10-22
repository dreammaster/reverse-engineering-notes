#pragma once
// Combat and encounter data helpers of WL.EXE (0x12A40..0x13A56 and 0x15xxx).
//
// The monsters of an encounter live in a table at ds:6B31: 4 groups x 4 slots x 94 bytes. A slot has a 4 byte header
// (x, y, ...) followed by 3 "members" of 30 bytes; a member holds the hit points (words) of up to 10 monsters followed
// by 10 status bytes. Combatants are numbered 0..47 as (group * 4 + slot) * 3 + member.
#include "wasteland1/machine.h"

namespace wl {

constexpr uint16_t kCombatTable = 0x6B31;
constexpr uint16_t kGroupBytes = 0x178, kSlotBytes = 0x5E, kMemberBytes = 0x1E, kSlotHeaderBytes = 4;

struct CombatantId { uint8_t group, slot, member; };

// 13A56 / 13A37: combatant number <-> (group, slot, member)
CombatantId splitCombatantIndex(uint8_t index);
uint8_t combatantIndex(uint8_t group, uint8_t slot, uint8_t member);

// 137F4: monsterPtr = the slot record; 137CE: monsterPtr = the member record inside it
void selectSlot(Machine &m, uint8_t group, uint8_t slot);
void selectMember(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 13787: (x, y) stored in the header of the selected slot
struct Position { uint8_t x, y; };
Position slotPosition(Machine &m);
// 138B0: zeroes the 94 bytes of the selected slot
void clearSlot(Machine &m);
// 13924: partyStatusPtr / partyOrdersPtr = the two 8 byte arrays of party group `group` inside the savegame (offsets 0x38..)
void selectPartyBlock(Machine &m, uint8_t group);
// 13449 / 1353C: low six bits / top two bits of the order byte of party member `index`
uint8_t orderLow(Machine &m, uint8_t index);
uint8_t orderHigh(Machine &m, uint8_t index);

// ---- encounter (location record) and item record accessors --------------------------------------------------------------
// 12A40 (continuing into 12A4C): the monster type id of member `index` (0..2) of the saved location record, then
// getMonsterDetails of it
uint8_t encounterMemberId(Machine &m, uint8_t index);
// 12A4C: itemDetailsPtr = record `id` of the map's monster table (8 byte records; the pointer variable is shared with the
// item table); returns the sex flag table entry stored in itemSexFlag
uint8_t getMonsterDetails(Machine &m, uint8_t id);
void selectMonsterDetails(Machine &m, uint8_t member);       // 12A40
// 1379E / 13797 / 13787: savedLocationDetailsPtr = the location record named by (class, action) / by the cell (x, y) /
// by the position of the selected slot
void saveLocationRecord(Machine &m, uint8_t table, uint8_t index);
void selectCellRecord(Machine &m, uint8_t x, uint8_t y);
void selectSlotCellRecord(Machine &m);
// 15CC0: selectSlot, selectSlotCellRecord and selectMonsterDetails(member)
void selectMonsterType(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 12A8D / 12A92 / 12A9D / 12AA2 / 12AAB / 12ABA: fields of the item record found by getItemDetails
uint8_t itemDamageDice(Machine &m);           // 12A8D: low nibble of byte 5 (dice count)
uint8_t itemClipSizeNibble(Machine &m);       // 12A92: low nibble of byte 4
uint8_t itemAmmunitionType(Machine &m);       // 12A9D: byte 7
uint8_t itemArmorClassByte(Machine &m);       // 12AA2: byte 6
// 12AC5 / 15C08 / 15C27: flags of the saved location record
uint8_t locationRecordBits2(Machine &m);      // byte 9 >> 2
uint8_t locationRecordBits4(Machine &m);      // byte 9 >> 4
void markLocationVisited(Machine &m);         // byte 9 = (byte 9 | 1) & 0FDh

// 1391E / 13878 / 138C1: readiness of the combatant (state byte at 46CC) and ammunition accounting
void markReady(Machine &m);
uint8_t combatReadiness(Machine &m, uint8_t partyNum);
uint8_t useAmmunition(Machine &m, uint8_t rounds, bool *zeroFlag = nullptr);   // zeroFlag: the zero flag the original returns
// 15705..15714: damage bonus of the current character's luck / dexterity / agility / strength (16 bit, may be negative)
uint16_t attributeBonus(Machine &m, uint8_t attributeOffset);
// weapon class predicates (item types): true when the type belongs to the class
bool isTypeGroupA(uint8_t type);      // 15C6E: 5, 6, 7
bool isTypeGroupB(uint8_t type);       // 15C7E: 0Ah, 0Bh, 0Ch
bool isTypeGroupC(uint8_t type);       // 15C8C: 8, 9
// 15CB2: n * 10 (8 bit)
uint8_t timesTen(uint8_t n);
// 157AD: number of monsters of the member (group, slot, member) that are still alive (hit points != 0)
uint8_t countAliveMonsters(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 15C96: quantity and jam byte of the equipped weapon
uint8_t weaponQuantityByte(Machine &m);

// ---- combat messages ----------------------------------------------------------------------------------------------------
// 1393E: prints string `bl` of the string index at the start of the map block's location data
void printLocationMessage(Machine &m, uint8_t index);
// 13939 / 1394D: location message `base` + random(2)
void printLocationMessageVariant(Machine &m, uint8_t base);
// 15A84: number in dl:al, then message 59h ("... N damage"-style); itemQuantity is 1 for exactly one point
void printDamageNumber(Machine &m, uint16_t value);
// 15C36: itemQuantity = n; message 62h or a variant of 59h depending on the hit flag
void printHitMessage(Machine &m, uint8_t n);
// 15BBC / 15BCB / 15BEF: attack announcement chosen from the weapon type
void printAttackMessageA(Machine &m);
void printAttackMessageB(Machine &m);
void printAttackMessageC(Machine &m);
// 1592C: both bytes of the current character's hit points = value
void setHitPointBytes(Machine &m, uint8_t value);
// 156DE: 0, 1 or 2 depending on the weapon type (damage class)
uint8_t weaponDamageClass(uint8_t type);
// 15672: damage dice of a monster member: sumOfD6(0, dice count of the member's record)
uint16_t rollMonsterDamage(Machine &m, uint8_t group, uint8_t slot, uint8_t member);
// 15A18: numberValue = the first two bytes of the item/monster record (0 as third byte)
void recordWordToNumber(Machine &m);
// 1597E: reduces `damage` by the armor roll of the monster combatant `index` (the roll happens even when armor is ignored,
// byte hitIgnoresArmor); the result never goes below 0
uint16_t reduceByMonsterArmor(Machine &m, uint16_t damage, uint8_t index);
// 159C7: numberValue = experience for killing one monster of combatant `index`: record word * (dice nibble + 1)
void monsterExperience(Machine &m, uint8_t index);
// 15755: number of dice rolled for the defense of party member `partyNum` against a monster of toughness `threshold`
uint16_t defenseRoll(Machine &m, uint8_t partyNum, uint8_t threshold);
// 15738: defenseRoll plus the luck bonus of the current character
uint16_t attackStrength(Machine &m, uint8_t partyNum, uint8_t threshold);
// 15500: party member `partyNum` hits a random monster of combatant `index`. Result: alreadyDead when the chosen monster
// had no hit points (try again); otherwise `killed` is true when the hit killed it
struct HitResult { bool alreadyDead, killed; };
HitResult hitMonster(Machine &m, uint8_t partyNum, uint8_t index);
// 15A30: a combatant was fought with the equipped weapon: maybe improve the weapon skill (needs ammunition left)
void weaponSkillRoll(Machine &m, uint8_t index);
// 157D6: damage (16 bit) to party member `member`; true when the member was already dead (nothing happens)
bool damagePartyMember(Machine &m, uint16_t damage, uint8_t member);
// 15CD1: random 0..9
uint8_t randomDigit(Machine &m);
// 15C4A: remembers whether a damage value (dl:al) is below the "overkill" limit in the hit flag
uint8_t setHitFlag(Machine &m, uint16_t damage);

} // namespace wl
