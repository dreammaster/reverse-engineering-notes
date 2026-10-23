#pragma once
// Character record operations of WL.EXE: money, experience, hit points, equipment and the inventory (0x17AC3..0x19D86).
// Like party.h everything works on the data segment image; "the current character" is the record found by
// setCurrentCharacter().
#include "wasteland1/machine.h"

namespace wl {

// ---- inventory slots ---------------------------------------------------------------------------------------------
// 19AB6: empties the inventory slot whose item id byte is at `itemOffset` (the quantity byte follows it)
void removeItem(Machine &m, uint8_t itemOffset);
// 196C9 / 196C4: inventory offset of the item equipped in the weapon / armor field of the record (`field` is pm::weapon or
// pm::armor); 0 when nothing is equipped there
uint8_t equippedItemOffset(Machine &m, uint8_t field);
// 196B2: makes itemDetailsPtr describe the equipped weapon (the record of item 0 when none is equipped)
void getEquippedWeaponDetails(Machine &m);
// 9A0F: armor class of the item described by itemDetailsPtr
uint8_t getArmorClass(Machine &m);
// 949E: equips the item in inventory slot `slot` (or takes it off when it is already equipped)
void equip(Machine &m, uint8_t slot);
// 94E8: takes the item of inventory slot `slot` off if it is the weapon or the armor (the armor class is cleared too)
void removeWeaponArmor(Machine &m, uint8_t slot);
// 196DB: loads the ammunition at inventory offset `ammoOffset` into the equipped weapon; false when the weapon is jammed
bool reload(Machine &m, uint8_t ammoOffset);
// 19D2F: true when `itemId` is not one of the items that use ammunition (AMMUNITION_ITEMS table)
bool hasNoAmmunition(Machine &m, uint8_t itemId);

// ---- money and experience (24 bit values kept in numberValue) -----------------------------------------------------------
// 198BE: writes `value` into the three bytes of the current record's field at `offset` (money, experience)
void setMember24(Machine &m, uint8_t offset, uint8_t value);
void getMemberMoney(Machine &m);        // 19A9C: numberValue = money
void addMoneyToMember(Machine &m);      // 19895: money += numberValue (saturates at 16777215)
void getMemberExperience(Machine &m);   // 19B67: numberValue = experience
void addExperience(Machine &m);         // 19BC0: experience += numberValue (saturates)
bool canAfford(Machine &m);             // 17B3E: money >= numberValue
bool pay(Machine &m);                   // 17B15: subtracts numberValue from the money if it is affordable
void getItemPrice(Machine &m);          // 17B5C: numberValue = price of the item described by itemDetailsPtr

// ---- skills -------------------------------------------------------------------------------------------------------------
// 1830E: skill id of the weapon/item record `itemId`
uint8_t itemSkillId(Machine &m, uint8_t itemId);
// 182FA: 3 * the current character's skill level with the equipped weapon (dl:al)
uint16_t weaponSkillBonus(Machine &m);
// 18D85: absolute value of a signed byte
uint8_t absByte(uint8_t v);
// 1818E: chance to improve a skill that was just used against a difficulty `ceiling`; prints the message when it improves
void trySkillImprovement(Machine &m, uint8_t skillId, uint8_t ceiling);
// 19720: selects the buffered text writer while the roster window is shown
void selectWriterForRoster(Machine &m);

// ---- hit points -------------------------------------------------------------------------------------------------------
// 19CE9: hit points -= damage (16 bit, no clamping); returns the high byte of the result
uint8_t applyDamage(Machine &m, uint16_t damage);
// 19CFE: adds one hit point; true while the hit points are still below the maximum
bool healOneHitPoint(Machine &m);
// 19A41: true when the current hit points equal the maximum
bool isAtFullHealth(Machine &m);
// 19D0E: number of members of the active group that are conscious
uint8_t countConsciousMembers(Machine &m);

// ---- health over time ---------------------------------------------------------------------------------------------------
// 19624: selects the character record of roster number `id` (1 based) as the current character
void setCurrentRosterCharacter(Machine &m, uint8_t id);
// 1724F: party group of roster number `id` (0..3; 4 when none holds it)
uint8_t partyGroupOfRoster(Machine &m, uint8_t id);
// 12440: one step of time for every character: hit points regenerate, and poisoned or dying characters get worse.
// `ds:4722` is the step counter that paces it
void healthTick(Machine &m);

} // namespace wl
