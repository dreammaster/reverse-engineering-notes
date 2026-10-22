#include "wasteland1/combat.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"

namespace wl {

namespace {
uint16_t cur(const Machine &m) { return m.d16(g::currentCharacter); }
uint8_t itemByte(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::itemDetailsPtr) + offset)); }
uint8_t locByte(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::savedLocationDetailsPtr) + offset)); }
}

CombatantId splitCombatantIndex(uint8_t index) {
	const uint8_t quotient = index / 3, member = index % 3;
	return {uint8_t(quotient >> 2), uint8_t(quotient & 3), member};
}

uint8_t combatantIndex(uint8_t group, uint8_t slot, uint8_t member) {
	// the original adds with carries; for valid values this is (group * 4 + slot) * 3 + member
	const uint8_t t = uint8_t((group << 2) + slot);
	const unsigned carry = t >> 7;
	const unsigned sum = unsigned(uint8_t(t << 1)) + t + carry;
	return uint8_t(sum + member + (sum > 0xFF ? 1 : 0));
}

void selectSlot(Machine &m, uint8_t group, uint8_t slot) {
	m.setD16(g::monsterPtr, uint16_t(kCombatTable + group * kGroupBytes + slot * kSlotBytes));
}

void selectMember(Machine &m, uint8_t group, uint8_t slot, uint8_t member) {
	selectSlot(m, group, slot);
	m.setD16(g::monsterPtr, uint16_t(m.d16(g::monsterPtr) + kSlotHeaderBytes + member * kMemberBytes));
}

Position slotPosition(Machine &m) {
	const uint16_t p = m.d16(g::monsterPtr);
	return {m.d8(p), m.d8(uint16_t(p + 1))};
}

void clearSlot(Machine &m) {
	const uint16_t p = m.d16(g::monsterPtr);
	for (uint16_t i = 0; i < kSlotBytes; ++i) m.d8(uint16_t(p + i)) = 0;
}

void selectPartyBlock(Machine &m, uint8_t group) {
	const uint16_t base = uint16_t(m.d8(uint16_t(g::partyBlockOffsets + group)) + 0x7169);
	m.setD16(g::partyStatusPtr, base);
	m.setD16(g::partyOrdersPtr, uint16_t(base + 8));
}

uint8_t orderLow(Machine &m, uint8_t index) { return m.d8(uint16_t(m.d16(g::partyOrdersPtr) + index)) & 0x3F; }
uint8_t orderHigh(Machine &m, uint8_t index) { return uint8_t(m.d8(uint16_t(m.d16(g::partyOrdersPtr) + index)) >> 6); }

uint8_t encounterMemberId(Machine &m, uint8_t index) { return locByte(m, m.d8(uint16_t(g::encounterOffsets + index))); }

uint8_t getMonsterDetails(Machine &m, uint8_t id) {
	m.setD16(g::itemDetailsBasePtr, m.d16(uint16_t(m.d16(g::locationsPtr) + 4)));
	m.d8(g::itemDetailsRecSize) = 8;
	getItemDetailsInner(m, id);
	const uint8_t flag = m.d8(uint16_t(g::sexFlagTable + itemByte(m, 7)));
	m.d8(g::itemSexFlag) = flag;
	return flag;
}

void selectMonsterDetails(Machine &m, uint8_t member) { getMonsterDetails(m, encounterMemberId(m, member)); }

void saveLocationRecord(Machine &m, uint8_t table, uint8_t index) {
	const uint16_t current = m.d16(g::locationDetailsPtr);
	selectLocationRecord(m, table, index);
	m.setD16(g::savedLocationDetailsPtr, m.d16(g::locationDetailsPtr));
	m.setD16(g::locationDetailsPtr, current);
}

void selectCellRecord(Machine &m, uint8_t x, uint8_t y) {
	const Cell c = cellAt(m, x, y);
	saveLocationRecord(m, c.tileClass, c.action);
}

void selectSlotCellRecord(Machine &m) {
	const Position p = slotPosition(m);
	selectCellRecord(m, p.x, p.y);
}

void selectMonsterType(Machine &m, uint8_t group, uint8_t slot, uint8_t member) {
	selectSlot(m, group, slot);
	selectSlotCellRecord(m);
	selectMonsterDetails(m, member);
}

uint8_t itemDamageDice(Machine &m) { return itemByte(m, 5) & 0x0F; }
uint8_t itemClipSizeNibble(Machine &m) { return itemByte(m, 4) & 0x0F; }
uint8_t itemAmmunitionType(Machine &m) { return itemByte(m, 7); }
uint8_t itemArmorClassByte(Machine &m) { return itemByte(m, 6); }

uint8_t locationRecordBits2(Machine &m) { return uint8_t(locByte(m, 9) >> 2); }
uint8_t locationRecordBits4(Machine &m) { return uint8_t(locByte(m, 9) >> 4); }
void markLocationVisited(Machine &m) {
	const uint16_t a = uint16_t(m.d16(g::savedLocationDetailsPtr) + 9);
	m.d8(a) = uint8_t((m.d8(a) | 1) & 0xFD);
}

void markReady(Machine &m) { m.d8(g::readiness) = 0x0F; }

uint8_t combatReadiness(Machine &m, uint8_t partyNum) {
	if (isMemberUnconscious(m, partyNum)) {
		m.d8(g::readiness) = 0;
		return 0;
	}
	m.d8(g::readiness) = 0x0F;
	getEquippedWeaponDetails(m);
	const uint8_t type = getItemType(m);
	m.d8(g::weaponType) = type;
	if (type != 0 && !hasNoAmmunition(m, type)) {
		m.d8(g::readiness) = 0xFE;   // a weapon that needs ammunition
	} else {
		m.d8(g::weaponType) = 1;
	}
	useAmmunition(m, 0);
	return m.d8(g::readiness);
}

uint8_t useAmmunition(Machine &m, uint8_t rounds) {
	const uint8_t negated = uint8_t(0 - rounds);
	getEquippedWeaponDetails(m);
	const uint8_t type = getItemType(m);
	if (type == 0 || hasNoAmmunition(m, type) || itemByte(m, 4) == 0) return 1;
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	if (offset == 0) { markReady(m); return 0x0F; }
	const uint16_t qty = uint16_t(cur(m) + offset + 1);
	uint8_t value = m.d8(qty);
	if (value & 0x80) { markReady(m); return 0x0F; }   // jammed
	const uint8_t flags = value & 0xC0;
	uint8_t left = uint8_t((value & 0x3F) + negated);
	if (left == 0 || (left & 0x80)) {
		markReady(m);
		left = 0;
	}
	m.d8(qty) = uint8_t(left | flags);
	return left & 0x3F;
}

uint16_t attributeBonus(Machine &m, uint8_t attributeOffset) {
	const uint8_t value = m.d8(uint16_t(cur(m) + attributeOffset));
	if (value >= 13) return uint8_t((value - 12) >> 1);
	if (value >= 9) return 0;
	const uint8_t shifted = uint8_t(value - 9);
	return uint16_t(0xFF00 | (0x80 | (shifted >> 1)));   // a negative bonus
}

bool isTypeGroupA(uint8_t type) { return type == 5 || type == 6 || type == 7; }
bool isTypeGroupB(uint8_t type) { return type == 0x0A || type == 0x0B || type == 0x0C; }
bool isTypeGroupC(uint8_t type) { return type == 8 || type == 9; }

uint8_t countAliveMonsters(Machine &m, uint8_t group, uint8_t slot, uint8_t member) {
	selectMember(m, group, slot, member);
	const uint16_t p = m.d16(g::monsterPtr);
	uint8_t alive = 0;
	for (int hp = 0; hp < 10; ++hp)
		if (m.d16(uint16_t(p + hp * 2)) != 0) ++alive;
	return alive;
}

uint8_t weaponQuantityByte(Machine &m) {
	return m.d8(uint16_t(cur(m) + equippedItemOffset(m, pm::weapon) + 1));
}

uint8_t timesTen(uint8_t n) { return uint8_t(n * 10); }

uint8_t randomDigit(Machine &m) { return uint8_t(Rules(m).random(10) - 1); }

uint8_t setHitFlag(Machine &m, uint16_t damage) {
	const uint8_t lo = uint8_t(damage), hi = uint8_t(damage >> 8);
	uint8_t flag = 1;
	if (uint8_t(lo - 1) >= 0xF1 && uint8_t(hi - 1) >= 0xFE) flag = 0;
	m.d8(g::hitFlag) = flag;
	return flag;
}

} // namespace wl
