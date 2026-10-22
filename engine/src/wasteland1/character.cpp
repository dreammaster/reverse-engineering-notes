#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

namespace wl {

namespace {

uint16_t cur(const Machine &m) { return m.d16(g::currentCharacter); }
uint8_t &field(Machine &m, uint8_t offset) { return m.d8(uint16_t(cur(m) + offset)); }

constexpr uint8_t kItemTypeArmor = 15;   // item type (flags >> 3) of armor

// ItemDetails record layout (8 bytes): +3 flags (type in bits 3..7), +4 clip size, +6 armor class, +7 ammunition item
constexpr uint8_t kDetailsClipSize = 4, kDetailsArmorClass = 6, kDetailsAmmunition = 7;

} // namespace

// ---- inventory slots -----------------------------------------------------------------------------------------------

void removeItem(Machine &m, uint8_t itemOffset) {
	field(m, itemOffset) = 0;
	field(m, uint8_t(itemOffset + 1)) = 0;
}

uint8_t equippedItemOffset(Machine &m, uint8_t fieldOffset) {
	uint8_t slot = field(m, fieldOffset);
	return slot ? getInventoryItemOffset(slot) : 0;
}

void getEquippedWeaponDetails(Machine &m) {
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	getItemDetails(m, offset ? field(m, offset) : 0);
}

uint8_t getArmorClass(Machine &m) { return itemFlag(m, kDetailsArmorClass); }

void removeWeaponArmor(Machine &m, uint8_t slot) {
	if (slot == field(m, pm::weapon)) {
		field(m, pm::weapon) = 0;
	} else if (slot == field(m, pm::armor)) {
		field(m, pm::armor) = 0;
		field(m, pm::armorClass) = 0;
	}
}

void equip(Machine &m, uint8_t slot) {
	m.d8(g::selectedEquipItem) = slot;
	if (slot == field(m, pm::weapon) || slot == field(m, pm::armor)) {
		removeWeaponArmor(m, slot);
		return;
	}
	getItemDetails(m, field(m, getInventoryItemOffset(slot)));
	if (getItemType(m) == kItemTypeArmor) {
		field(m, pm::armorClass) = getArmorClass(m);
		field(m, pm::armor) = slot;
	} else {
		field(m, pm::weapon) = slot;
	}
}

bool reload(Machine &m, uint8_t ammoOffset) {
	m.d8(g::selectedItemOffset) = ammoOffset;
	const uint8_t qtyOffset = uint8_t(equippedItemOffset(m, pm::weapon) + 1);
	m.d8(g::selectedWeaponQtyOffset) = qtyOffset;
	if (field(m, qtyOffset) & 0x80) {   // jammed
		writeItemTextNormal(m, 152);
		return false;
	}
	removeItem(m, ammoOffset);
	getEquippedWeaponDetails(m);
	// keep the two flag bits (jam), the low six bits become the clip size
	field(m, qtyOffset) = uint8_t((field(m, qtyOffset) & ~0x3F) | itemFlag(m, kDetailsClipSize));
	return true;
}

bool hasNoAmmunition(Machine &m, uint8_t itemId) {
	m.d8(g::currentItemId) = itemId;
	for (uint8_t i = 0;; ++i) {
		const uint8_t entry = m.d8(uint16_t(g::ammunitionItems + i));
		if (entry & 0x80) return true;
		if (entry == itemId) return false;
		if (i == 255) return true;
	}
}

// ---- money and experience ---------------------------------------------------------------------------------------------

void setMember24(Machine &m, uint8_t offset, uint8_t value) {
	for (int i = 0; i < 3; ++i) field(m, uint8_t(offset + i)) = value;
}

namespace {

void getMember24(Machine &m, uint8_t offset) {
	for (int i = 0; i < 3; ++i) m.d8(uint16_t(g::numberValue + i)) = field(m, uint8_t(offset + i));
}

// member field += numberValue; a carry out of the top byte saturates the field to FFFFFFh
void addMember24(Machine &m, uint8_t offset) {
	unsigned carry = 0;
	for (int i = 0; i < 3; ++i) {
		const unsigned sum = field(m, uint8_t(offset + i)) + m.d8(uint16_t(g::numberValue + i)) + carry;
		carry = sum > 0xFF;
		if (i == 2 && carry) { setMember24(m, offset, 0xFF); return; }
		field(m, uint8_t(offset + i)) = uint8_t(sum);
	}
}

} // namespace

void getMemberMoney(Machine &m) { getMember24(m, pm::money); }
void addMoneyToMember(Machine &m) { addMember24(m, pm::money); }
void getMemberExperience(Machine &m) { getMember24(m, pm::experience); }
void addExperience(Machine &m) { addMember24(m, pm::experience); }

bool canAfford(Machine &m) {
	for (int i = 2; i >= 0; --i) {
		const uint8_t have = field(m, uint8_t(pm::money + i)), price = m.d8(uint16_t(g::numberValue + i));
		if (have != price) return have > price;
	}
	return true;
}

bool pay(Machine &m) {
	if (!canAfford(m)) return false;
	unsigned borrow = 0;
	for (int i = 0; i < 3; ++i) {
		const int diff = int(field(m, uint8_t(pm::money + i))) - m.d8(uint16_t(g::numberValue + i)) - int(borrow);
		borrow = diff < 0;
		field(m, uint8_t(pm::money + i)) = uint8_t(diff);
	}
	return true;
}

void getItemPrice(Machine &m) {
	m.d8(g::numberValue) = itemFlag(m, 0);
	m.d8(uint16_t(g::numberValue + 1)) = itemFlag(m, 1);
	m.d8(uint16_t(g::numberValue + 2)) = 0;
}

// ---- skills -------------------------------------------------------------------------------------------------------------

uint8_t itemSkillId(Machine &m, uint8_t itemId) {
	getItemDetails(m, itemId);
	return itemFlag(m, 5);
}

uint16_t weaponSkillBonus(Machine &m) {
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	const uint8_t itemId = offset ? field(m, offset) : 0;
	return uint16_t(getMemberSkillLevel(m, itemSkillId(m, itemId)) * 3);
}

uint8_t absByte(uint8_t v) { return (v & 0x80) ? uint8_t(0 - v) : v; }

void selectWriterForRoster(Machine &m) {
	if (m.d8(g::rosterShown) != 0) setWriteCharacterFn(m);
	else drawInfoWindow(m);   // the routine falls through into 19727
}

void trySkillImprovement(Machine &m, uint8_t skillId, uint8_t ceiling) {
	const uint8_t level = getMemberSkillLevel(m, skillId);
	if (level == 0 || level >= field(m, pm::level) || level >= ceiling) return;
	const uint8_t needed = uint8_t(((ceiling - level) >> 1) + 1);
	if (Rules(m).random(10) > needed) return;
	const uint8_t savedRoster = m.d8(g::rosterShown);
	selectWriterForRoster(m);
	writeItemTextNormal(m, 0x9E);
	// the level byte follows the skill id byte
	uint16_t slot = cur(m) + pm::skills;
	while (slot != cur(m) + pm::skillsEnd && m.d8(uint16_t(slot)) != skillId) slot = uint16_t(slot + 2);
	uint8_t newLevel = m.d8(uint16_t(slot + 1));
	if (newLevel != 0xFF) m.d8(uint16_t(slot + 1)) = ++newLevel;
	else newLevel = 0;
	writeItemText(m, m.d8(uint16_t(slot)));
	writeItemTextNormal(m, 0x9F);
	writeNumberLeft(m, newLevel);
	writeCharacterVia(m, '.');
	newline(m);
	if (savedRoster == 0) waitSpaceOrEnter(m);
}

// ---- hit points ---------------------------------------------------------------------------------------------------------

uint8_t applyDamage(Machine &m, uint16_t damage) {
	const uint16_t con = uint16_t(field(m, pm::con) | (field(m, uint8_t(pm::con + 1)) << 8));
	const uint16_t left = uint16_t(con - damage);
	field(m, pm::con) = uint8_t(left);
	field(m, uint8_t(pm::con + 1)) = uint8_t(left >> 8);
	return uint8_t(left >> 8);
}

bool healOneHitPoint(Machine &m) {
	applyDamage(m, 0xFFFF);   // subtracting 65535 modulo 2^16 adds one hit point
	return !isAtFullHealth(m);
}

bool isAtFullHealth(Machine &m) {
	return field(m, pm::maxCon) == field(m, pm::con) && field(m, uint8_t(pm::maxCon + 1)) == field(m, uint8_t(pm::con + 1));
}

uint8_t countConsciousMembers(Machine &m) {
	uint8_t count = 0;
	for (uint8_t n = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup)); n != 0; --n)   // _saved2._membersInGroup
		if (!isMemberUnconscious(m, n)) ++count;
	return count;
}

} // namespace wl
