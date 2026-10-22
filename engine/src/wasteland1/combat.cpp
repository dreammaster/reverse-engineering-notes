#include "wasteland1/combat.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

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

void printLocationMessage(Machine &m, uint8_t index) {
	printMapMessage(m, m.d8(uint16_t(m.d16(g::locationsPtr) + index)));
}

void printLocationMessageVariant(Machine &m, uint8_t base) {
	printLocationMessage(m, uint8_t(base + Rules(m).random(2)));
}

void printDamageNumber(Machine &m, uint16_t value) {
	m.d8(g::itemQuantity) = (uint8_t(value >> 8) == 0 && uint8_t(value) == 1) ? 1 : 2;
	writeNumber16(m, value);
	printMessage(m, 0x59);
}

void printHitMessage(Machine &m, uint8_t n) {
	m.d8(g::itemQuantity) = n;
	if (m.d8(g::hitFlag) == 0) printMessage(m, 0x62);
	else printLocationMessageVariant(m, 0x59);
}

void printAttackMessageA(Machine &m) {   // 15BBC
	printLocationMessage(m, isTypeGroupA(m.d8(g::weaponType)) ? 0x39 : 0x3A);
	printMessage(m, 0x2D);   // falls into 15BFE
}

void printAttackMessageB(Machine &m) {   // 15BCB
	const uint8_t type = m.d8(g::weaponType);
	uint8_t index;
	if (type == 2) index = 0x37;
	else if (type == 3 || type == 4 || isTypeGroupA(type)) index = 0x38;
	else index = isTypeGroupC(type) ? 0x3D : 0x3A;
	printLocationMessage(m, index);
	printMessage(m, 0x2D);
}

void printAttackMessageC(Machine &m) {   // 15BEF
	printLocationMessage(m, isTypeGroupA(m.d8(g::weaponType)) ? 0x3B : 0x3A);
	printMessage(m, 0x2D);
}

void setHitPointBytes(Machine &m, uint8_t value) {
	m.d8(uint16_t(cur(m) + pm::con)) = value;
	m.d8(uint16_t(cur(m) + pm::con + 1)) = value;
}

uint8_t weaponDamageClass(uint8_t type) {
	if (type == 2 || type == 5 || type == 0x0A || type == 0x0D) return 0;
	if (type == 3 || type == 6 || type == 0x0B || type == 8) return 1;
	return 2;
}

uint16_t rollMonsterDamage(Machine &m, uint8_t group, uint8_t slot, uint8_t member) {
	selectMonsterType(m, group, slot, member);
	uint8_t al;
	return Rules(m).sumOfD6(0, itemClipSizeNibble(m), al);
}

void recordWordToNumber(Machine &m) {
	m.d8(g::numberValue) = itemByte(m, 0);
	m.d8(uint16_t(g::numberValue + 1)) = itemByte(m, 1);
	m.d8(uint16_t(g::numberValue + 2)) = 0;
}

uint16_t reduceByMonsterArmor(Machine &m, uint16_t damage, uint8_t index) {
	const CombatantId id = splitCombatantIndex(index);
	const uint16_t armor = rollMonsterDamage(m, id.group, id.slot, id.member);
	if (m.d8(g::hitIgnoresArmor) == 0) damage = damage >= armor ? uint16_t(damage - armor) : 0;
	m.d8(g::hitIgnoresArmor) = 0;
	return damage;
}

void monsterExperience(Machine &m, uint8_t index) {
	const CombatantId id = splitCombatantIndex(index);
	selectSlot(m, id.group, id.slot);
	selectSlotCellRecord(m);
	selectMonsterDetails(m, id.member);
	const uint8_t count = uint8_t(itemClipSizeNibble(m) + 1);
	recordWordToNumber(m);
	const unsigned base = m.d8(g::numberValue) | (m.d8(uint16_t(g::numberValue + 1)) << 8);
	for (int i = 0; i < 3; ++i) m.d8(uint16_t(g::numberValue + i)) = 0;
	unsigned total = 0;
	for (uint8_t i = 0; i != count || count == 0; ) { total += base; if (++i == count) break; }
	m.d8(g::numberValue) = uint8_t(total);
	m.d8(uint16_t(g::numberValue + 1)) = uint8_t(total >> 8);
	m.d8(uint16_t(g::numberValue + 2)) = uint8_t(total >> 16);
}

uint16_t defenseRoll(Machine &m, uint8_t partyNum, uint8_t threshold) {
	combatReadiness(m, partyNum);
	const uint8_t type = m.d8(g::weaponType);
	uint8_t dice, base = 0;
	if (!hasNoAmmunition(m, type) && m.d8(g::readiness) < 0x10) {
		dice = (type == 2 || type == 0x0A) ? 1 : 2;
		base = type;
	} else {
		dice = getArmorClass(m);
		if (isTypeGroupC(type) && dice >= threshold) {
			const unsigned sum = uint8_t(dice - threshold) + dice;
			dice = sum > 0xFF ? 0xFF : uint8_t(sum);
		}
	}
	uint8_t al;
	return Rules(m).sumOfD6(base, dice, al);
}

uint16_t attackStrength(Machine &m, uint8_t partyNum, uint8_t threshold) {
	const uint16_t roll = defenseRoll(m, partyNum, threshold);
	return uint16_t(roll + attributeBonus(m, pm::luck));
}

HitResult hitMonster(Machine &m, uint8_t partyNum, uint8_t index) {
	const CombatantId id = splitCombatantIndex(index);
	selectMember(m, id.group, id.slot, id.member);
	const uint8_t target = randomDigit(m);   // which of the ten monsters
	const uint16_t record = m.d16(g::monsterPtr);
	if (m.d16(uint16_t(record + target * 2)) == 0) return {true, false};
	uint8_t entry = 0;
	for (;; ++entry) {   // the combatant's entry in the table of this round's targets
		const uint8_t t = m.d8(uint16_t(g::hitTargets + entry));
		if (t == index) break;
		if (t == 0xFF) { m.d8(uint16_t(g::hitTargets + entry)) = index; break; }
		if (entry == 255) { entry = 0; break; }
	}
	++m.d8(uint16_t(g::hitCounts + timesTen(entry) + target));
	selectMonsterType(m, id.group, id.slot, id.member);
	const uint8_t toughness = itemClipSizeNibble(m);
	uint16_t damage = attackStrength(m, partyNum, toughness);
	m.d8(g::hitIgnoresArmor) = isTypeGroupC(m.d8(g::weaponType)) ? 0 : 1;
	damage = reduceByMonsterArmor(m, damage, index);
	const uint16_t totalAddr = uint16_t(g::hitDamage + entry * 2);
	const unsigned total = m.d16(totalAddr) + damage;
	m.setD16(totalAddr, total > 0xFFFF ? 0xFFFF : uint16_t(total));
	if (damage == 0) return {false, false};
	uint8_t marker = itemDamageDice(m);
	selectMember(m, id.group, id.slot, id.member);
	const uint16_t hpAddr = uint16_t(m.d16(g::monsterPtr) + target * 2);
	const int hp = int(m.d16(hpAddr)) - damage;
	if (hp > 0) { m.setD16(hpAddr, uint16_t(hp)); return {false, false}; }
	// killed
	++m.d8(uint16_t(g::hitKills + entry));
	const uint16_t overkill = uint16_t(hp);   // the negative remainder, kept for the hit flag
	m.setD16(hpAddr, 0);
	const uint16_t statusAddr = uint16_t(m.d16(g::monsterPtr) + 0x14 + target);
	if (m.d8(statusAddr) != 0) m.d8(statusAddr) = 0xFF;
	else marker = 1;
	monsterExperience(m, index);
	addExperience(m);
	const uint8_t flag = setHitFlag(m, overkill);
	m.d8(uint16_t(g::hitFlags + entry)) &= flag;
	m.d8(uint16_t(g::hitStatus + entry)) = marker;
	return {false, true};
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
