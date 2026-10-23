#include "wasteland1/combat.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"
#include "wasteland1/legacy.h"

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

uint8_t useAmmunition(Machine &m, uint8_t rounds, bool *zeroFlag) {
	bool zf = false;
	if (!zeroFlag) zeroFlag = &zf;
	const uint8_t negated = uint8_t(0 - rounds);
	getEquippedWeaponDetails(m);
	const uint8_t type = getItemType(m);
	if (type == 0) { *zeroFlag = true; return 1; }
	if (hasNoAmmunition(m, type)) { *zeroFlag = false; return 1; }
	if (itemByte(m, 4) == 0) { *zeroFlag = true; return 1; }
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	if (offset == 0) { markReady(m); *zeroFlag = true; return 0x0F; }
	const uint16_t qty = uint16_t(cur(m) + offset + 1);
	uint8_t value = m.d8(qty);
	if (value & 0x80) { markReady(m); *zeroFlag = false; return 0x0F; }   // jammed
	const uint8_t flags = value & 0xC0;
	uint8_t left = uint8_t((value & 0x3F) + negated);
	if (left == 0 || (left & 0x80)) {
		markReady(m);
		left = 0;
	}
	m.d8(qty) = uint8_t(left | flags);
	*zeroFlag = (left & 0x3F) == 0;
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

void weaponSkillRoll(Machine &m, uint8_t index) {
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	uint8_t itemId = 0;
	if (offset) {
		itemId = m.d8(uint16_t(cur(m) + offset));
		getItemDetails(m, itemId);
		if (!hasNoAmmunition(m, getItemType(m)) && useAmmunition(m, 0) == 0) return;
	}
	const uint8_t skill = itemSkillId(m, itemId);
	const CombatantId id = splitCombatantIndex(index);
	selectMonsterType(m, id.group, id.slot, id.member);
	trySkillImprovement(m, skill, uint8_t((itemClipSizeNibble(m) >> 1) + 1));
}

bool damagePartyMember(Machine &m, uint16_t damage, uint8_t member) {
	auto con = [&](uint8_t off) -> uint8_t & { return m.d8(uint16_t(cur(m) + off)); };
	setCurrentCharacter(m, member);
	setWriteCharacterFn(m);
	const uint8_t dice = itemDamageDice(m);
	uint8_t savedLo, savedHi;
	if (!(con(pm::con + 1) & 0x80)) {
		if ((con(pm::con + 1) | con(pm::con)) == 0) return true;   // already dead
		writeMemberName(m);
		savedLo = con(pm::con);
		savedHi = con(pm::con + 1);
	} else {   // unconscious
		savedLo = con(pm::lastCon);
		savedHi = con(pm::lastCon + 1);
		if (m.d8(g::quietCombat) == 0) {
			bool spared = !hasNoAmmunition(m, dice);
			if (!spared) spared = Rules(m).rollDice() >= itemFlag(m, 3);
			if (spared) {
				writeMemberName(m);
				printMessage(m, 0x5B);
				return false;
			}
		}
		writeMemberName(m);
	}
	uint16_t armorRoll = 0;
	if (m.d8(g::armorBypass) == 0) {
		const uint8_t ac = con(pm::armorClass);
		m.d8(0xA685) = ac;
		uint8_t al;
		armorRoll = Rules(m).sumOfD6(0, ac, al);
	}
	if (damage <= armorRoll) {   // the armor absorbed everything
		uint8_t message = 0x64;
		if (m.d8(g::quietCombat) == 0) {
			if (dice == 1 && m.d8(0xA685) == 0) { printMessage(m, 0x5B); return false; }
			message = 0x5E;
		}
		printMessage(m, message);
		printMessage(m, 0x65);
		return false;
	}
	const uint16_t diff = uint16_t(damage - armorRoll);
	if (m.d8(g::quietCombat) != 0) printMessage(m, 0x63);
	else printLocationMessage(m, 0x51);
	printDamageNumber(m, diff);
	applyDamage(m, diff);
	const bool negative = con(pm::con + 1) & 0x80;
	bool knockedOut = negative;
	if (!negative) {
		if ((con(pm::con + 1) | con(pm::con)) != 0) knockedOut = false;
		else { setHitPointBytes(m, 0xFF); knockedOut = true; }
	}
	if (knockedOut) {
		con(pm::lastCon) = savedLo;
		con(pm::lastCon + 1) = savedHi;
		if (con(pm::con + 1) != 0xFF || con(pm::con) <= 0xCE) setHitPointBytes(m, 0);   // too far gone: dead
		printMessage(m, 0x2D);
		printMessage(m, m.d8(uint16_t(g::conditionMessages + getMemberConditionIndex(m))));
	}
	printMessage(m, 0x5A);
	X86Regs x;
	x.al = member;
	legacy::call(m, 0x7033, x);   // writePartyMemberCustomHilite
	return false;
}

void setItemQuantityOne(Machine &m) { m.d8(g::itemQuantity) = 1; }

void writeMonsterName(Machine &m, uint8_t index) {
	const uint16_t record = m.d16(g::savedLocationDetailsPtr);
	const bool table = (m.d8(uint16_t(record + 9)) & 1) != 0;
	selectMonsterDetails(m, index);
	if (!table) {
		printMessageKeepWriter(m, uint8_t(itemArmorClassByte(m) + 0x52));
		return;
	}
	const uint16_t block = m.d16(g::locationsPtr);
	m.setD16(g::textString, m.d16(uint16_t(block + 2)));
	uint8_t skip = m.d8(uint16_t(record + m.d8(uint16_t(g::encounterOffsets + index))));
	while (skip != 0) {
		uint8_t c;
		do {
			c = m.d8(m.d16(g::textString));
			m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
		} while (c != 0);
		--skip;
	}
	writeString2(m);
}

bool writeEncounterGroup(Machine &m, uint8_t index) {
	const uint16_t record = m.d16(g::savedLocationDetailsPtr);
	m.d8(0xA5A5) = index;
	const uint8_t count = m.d8(uint16_t(record + m.d8(uint16_t(g::encounterCountOffsets + index))));
	if (count == 0) return false;
	m.d8(g::itemQuantity) = count;
	m.d8(g::tempTextX) = 2;
	writeNumberLeft(m, count);
	++m.d8(g::rectTextX);
	writeMonsterName(m, index);
	return true;
}

void describeEncounter(Machine &m, uint8_t distance) {
	m.d8(0xA5A7) = distance;
	X86Regs x;
	legacy::call(m, 0x9EFC, x);   // sub_19EFC
	for (uint8_t i = 0; i != 3; ++i) {
		m.d8(0xA5A6) = i;
		if (!writeEncounterGroup(m, i)) continue;
		legacy::call(m, 0x9EFC, x);
	}
	++m.d8(g::tempTextX);
	printMessage(m, 0x2E);
	writeNumberLeft(m, distance);
	printMessage(m, 0x47);
}

void setSlotStatus(Machine &m, uint8_t slot, uint8_t value) {
	m.d8(0xA5A1) = slot;
	m.d8(0xA5A0) = value;
	for (uint8_t member = 0; member != 3; ++member) {
		m.d8(0xA59F) = member;
		selectMember(m, m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup)), slot, member);
		for (uint8_t i = 0x14; i != 0x1E; ++i) m.d8(uint16_t(m.d16(g::monsterPtr) + i)) = value;
	}
}

uint16_t monsterDamage(Machine &m, uint8_t group) {
	selectMonsterDetails(m, group);
	uint8_t al;
	return Rules(m).sumOfD6(uint8_t(itemByte(m, 5) >> 4), itemByte(m, 3), al);
}

void monsterHitsParty(Machine &m, uint8_t a, uint8_t b, uint8_t group) {
	m.d8(0xA4A9) = a;
	m.d8(0xA4A8) = b;
	const uint16_t damage = monsterDamage(m, group);
	m.d8(0xA4D5) = uint8_t(damage);
	m.d8(0xA4D6) = uint8_t(damage >> 8);
	for (;;) {
		const uint8_t pick = Rules(m).random(m.d8(uint16_t(g::saved2 + g::s2::membersInGroup)));
		m.d8(0xA4A7) = pick;
		setCurrentCharacter(m, pick);
		if (m.d8(uint16_t(m.d16(g::partyStatusPtr) + pick)) == 5 && b == 0 && a < 6) {
			writeMemberName(m);
			printMessage(m, 0x5D);
			return;
		}
		if (!damagePartyMember(m, damage, pick)) return;
	}
}

bool anyMemberAlive(Machine &m) {
	const uint8_t members = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	for (uint8_t member = 1;; ++member) {
		m.d8(0xA4A0) = member;
		setCurrentCharacter(m, member);
		if (conBytesOr(m) != 0) return true;
		if (member >= members) return false;
	}
}

uint8_t actionByte8(Machine &m) { return m.d8(uint16_t(m.d16(g::locationDetailsPtr) + 8)) & 0x7F; }
uint8_t actionByte0(Machine &m) { return m.d8(m.d16(g::locationDetailsPtr)); }

void adjustMemberField(Machine &m, uint8_t member, uint8_t offset, uint8_t flags) {
	m.d8(0xA5C2) = offset;
	m.d8(0xA5C4) = member;
	m.d8(0xA5C3) = flags;
	setCurrentCharacter(m, member);
	Rules rules(m);
	const uint16_t amount = rules.seed2();
	const uint16_t field = uint16_t(cur(m) + offset);
	if (flags & 0x80) {
		if (offset == pm::con) {
			m.d8(g::quietCombat) = 1;
			X86Regs x;
			legacy::call(m, 0x9EFC, x);   // sub_19EFC
			m.d8(g::armorBypass) = actionByte0(m) & 1;
			damagePartyMember(m, amount, member);
			m.d8(g::quietCombat) = 0;
			m.d8(g::armorBypass) = 0;
		} else if (offset == pm::money) {
			m.d8(g::numberValue) = uint8_t(amount);
			m.d8(uint16_t(g::numberValue + 1)) = uint8_t(amount >> 8);
			m.d8(uint16_t(g::numberValue + 2)) = 0;
			pay(m);
		} else {
			const uint8_t v = m.d8(field);
			m.d8(field) = v < uint8_t(amount) ? 0 : uint8_t(v - uint8_t(amount));
		}
	} else if (offset == pm::experience) {
		m.d8(g::numberValue) = uint8_t(amount);
		m.d8(uint16_t(g::numberValue + 1)) = uint8_t(amount >> 8);
		m.d8(uint16_t(g::numberValue + 2)) = 0;
		addExperience(m);
	} else {
		const unsigned sum = m.d8(field) + uint8_t(amount);
		if (sum <= 0xFF) m.d8(field) = uint8_t(sum);
	}
}

void printMonsterMessage(Machine &m, uint8_t id, uint8_t index) {
	printMessage(m, id);
	writeMonsterName(m, index);
}

bool attackMonster(Machine &m, uint8_t index, uint8_t monster, bool &killed) {
	const uint8_t savedQuantity = m.d8(g::itemQuantity);
	const CombatantId id = splitCombatantIndex(index);
	selectMember(m, id.group, id.slot, id.member);
	const uint16_t hpAddr = uint16_t(m.d16(g::monsterPtr) + monster * 2);
	killed = false;
	if (m.d16(hpAddr) == 0) return false;
	if (m.d8(g::weaponType) != 0x0D) printLocationMessageVariant(m, 0x51);
	else {
		if (m.d8(g::weaponNoteShown) != 0) printMessage(m, 0x3E);
		++m.d8(g::weaponNoteShown);
		printMessage(m, 0x61);
	}
	const uint16_t damage = reduceByMonsterArmor(m, Rules(m).seed1(), index);
	auto finish = [&]() {
		setItemQuantityOne(m);
		printMonsterMessage(m, 0x43, id.member);
		m.d8(g::itemQuantity) = savedQuantity;
	};
	if (damage == 0) {
		selectMember(m, id.group, id.slot, id.member);
		finish();
		printLocationMessage(m, 0x4A);
	} else {
		selectMember(m, id.group, id.slot, id.member);
		const uint16_t hp = m.d16(hpAddr);
		const uint16_t remaining = uint16_t(hp - damage);
		killed = damage >= hp;
		if (killed) m.setD16(hpAddr, 0);
		else m.setD16(hpAddr, remaining);
		finish();
		if (m.d8(g::weaponType) == 0x0D) printLocationMessageVariant(m, 0x57);
		printMessage(m, 0x21);
		printDamageNumber(m, damage);
		if (killed) {
			setHitFlag(m, remaining);
			printHitMessage(m, 1);
		}
	}
	printMessage(m, 0x5A);
	X86Regs x;
	legacy::call(m, 0x9EFC, x);   // sub_19EFC
	return true;
}

uint8_t memberTargetsAlive(Machine &m, uint8_t member) {
	const CombatantId id = splitCombatantIndex(orderLow(m, member));
	return countAliveMonsters(m, id.group, id.slot, id.member);
}

uint8_t targetMembersAlive(Machine &m) {
	const CombatantId id = splitCombatantIndex(m.d8(0xA55B));
	return countAliveMonsters(m, id.group, id.slot, m.d8(0xA559));
}

void printWeaponNote(Machine &m) {
	if (m.d8(g::weaponNoteShown) != 0) printMessage(m, 0x3E);
}

uint8_t countHitMonsters(Machine &m) {
	const uint16_t base = uint16_t(g::hitCounts + timesTen(m.d8(0xA55A)));
	uint8_t count = 0;
	for (int i = 0; i < 10; ++i)
		if (m.d8(uint16_t(base + i)) != 0) ++count;
	m.d8(g::itemQuantity) = count;
	m.d8(0xA55E) = count;
	return count;
}

uint8_t cellDistance(Machine &m, uint8_t x, uint8_t y) {
	const uint8_t a = absByte(uint8_t(y - m.d8(0x46A7)));
	m.d8(0xCD41) = a;
	const unsigned t = uint8_t(a << 3);
	const unsigned s1 = t + a;
	const uint8_t row = uint8_t(s1 + a + (s1 > 0xFF ? 1 : 0));
	const uint8_t b = absByte(uint8_t(x - m.d8(0x46A6)));
	m.d8(0xCD40) = b;
	return m.d8(uint16_t(0xCD0D + uint8_t(row + b)));
}

uint8_t burstHits(Machine &m, uint8_t n) {
	n >>= 2;
	if (n == 0) n = 1;
	uint8_t sum = 0;
	m.d8(0xA684) = 0;
	for (uint8_t i = 0; i < n; ++i) {
		sum = uint8_t(sum + Rules(m).random(4));
		m.d8(0xA684) = sum;
	}
	return sum;
}

uint8_t burstSize(Machine &m, uint8_t member) {
	setCurrentCharacter(m, member);
	return burstHits(m, useAmmunition(m, 0));
}

bool rollToFire(Machine &m, uint8_t member) {
	m.d8(0xA551) = member;
	setCurrentCharacter(m, member);
	Rules rules(m);
	rules.setSeed2(0);
	const uint8_t roll = rules.rollDice();
	bool normal = roll >= 4;
	if (!normal) normal = rules.d6(roll) != 1;
	if (!normal) {
		const uint8_t type = m.d8(g::weaponType);
		if (isTypeGroupC(type) || type == 0x0D) { printMessage(m, 0x29); return true; }
		printMessage(m, 0x2A);
		m.d8(uint16_t(cur(m) + equippedItemOffset(m, pm::weapon) + 1)) = 0x80;
		X86Regs x;
		x.al = member;
		legacy::call(m, 0x7033, x);   // writePartyMemberCustomHilite
		return true;
	}
	rules.addRoll(roll);
	rules.addRoll(m.d8(uint16_t(cur(m) + pm::dexterity)));
	rules.addRoll(weaponSkillBonus(m));
	uint8_t al;
	if (m.d8(g::weaponType) == 0x0D) al = 0x0A;
	else {
		al = uint8_t((m.d8(uint16_t(m.d16(g::partyOrdersPtr) + member)) >> 4) & 0x0C);
		if (al == 8) al = useAmmunition(m, 0);
	}
	rules.addRoll(al);
	return false;
}

uint8_t rangeModifier(Machine &m, uint8_t distance, uint8_t ammoFlag, uint8_t weaponClass) {
	const uint8_t offset = uint8_t(ammoFlag + 1);
	m.d8(0xA594) = offset;
	uint8_t base = m.d8(uint16_t(0xA5B8 + weaponClass));
	const uint8_t step = m.d8(uint16_t(0xA5BB + weaponClass));
	m.d8(0xA593) = base;
	m.d8(0xA592) = step;
	uint8_t index = 0;
	for (uint8_t threshold = 0;; threshold = uint8_t(threshold + 5)) {
		m.d8(uint16_t(0x7931 + index)) = threshold;
		base = uint8_t(base + step);
		m.d8(uint16_t(0x7932 + index)) = base;
		m.d8(0xA593) = base;
		m.d8(uint16_t(0x7933 + index)) = uint8_t(base + 7);
		index = uint8_t(index + 3);
		if (uint8_t(threshold + 5) >= 0x69) break;
	}
	uint8_t bl = 0;
	for (;;) {
		if (distance <= m.d8(uint16_t(0x7931 + bl))) break;
		bl = uint8_t(bl + 3);
		if (bl == 0) break;
	}
	return m.d8(uint16_t(0x7931 + uint8_t(bl + offset)));
}

uint8_t attackDifficulty(Machine &m, uint8_t member) {
	m.d8(0xA68E) = member;
	const CombatantId id = splitCombatantIndex(orderLow(m, member));
	m.d8(0xA68D) = id.slot;
	selectSlot(m, id.group, id.slot);
	const uint16_t record = m.d16(g::monsterPtr);
	const uint8_t distance = cellDistance(m, m.d8(record), m.d8(uint16_t(record + 1)));
	combatReadiness(m, member);
	selectSlotCellRecord(m);
	selectMonsterDetails(m, id.slot);
	const bool noAmmunition = hasNoAmmunition(m, itemDamageDice(m));
	return rangeModifier(m, distance, noAmmunition ? 1 : 0, weaponDamageClass(m.d8(g::weaponType)));
}

void finishTurn(Machine &m, uint8_t member) {
	m.d8(uint16_t(m.d16(g::partyStatusPtr) + member)) = 0;
	m.d8(uint16_t(m.d16(g::partyOrdersPtr) + member)) = 0;
	getEquippedWeaponDetails(m);
	if (itemByte(m, 4) != 1) return;
	const uint8_t type = getItemType(m);
	m.d8(g::weaponType) = type;
	if (hasNoAmmunition(m, type) && type != 1) return;
	const uint8_t offset = equippedItemOffset(m, pm::weapon);
	const uint8_t itemId = m.d8(uint16_t(cur(m) + offset));
	m.d8(0xA552) = itemId;
	m.d8(uint16_t(cur(m) + offset)) = 0;
	m.d8(uint16_t(cur(m) + offset + 1)) = 0;
	m.d8(uint16_t(cur(m) + 0x1F)) = 0;
	if (m.d8(g::weaponType) != 0x0D) return;
	const int found = findItemById(m, itemId);
	if (found < 0) return;
	m.d8(uint16_t(cur(m) + pm::weapon)) = uint8_t((uint8_t(found) - 0xBB) >> 1);
}

void partyMemberAttack(Machine &m, uint8_t member) {
	Rules rules(m);
	m.d8(0xA562) = member;
	if (memberTargetsAlive(m, member) == 0) return;
	for (int i = 0; i < 0x30; ++i) m.d8(uint16_t(g::hitTargets + i)) = 0;
	for (int i = 0; i < 3; ++i) { m.d8(uint16_t(g::hitTargets + i)) = 0xFF; m.d8(uint16_t(g::hitFlags + i)) = 0xFF; }
	{   // 15C19: the target slot of the member, marked as visited
		const CombatantId t = splitCombatantIndex(orderLow(m, member));
		selectSlot(m, t.group, t.slot);
		selectSlotCellRecord(m);
		markLocationVisited(m);   // 15C19 falls into 15C27
	}
	rules.setSeed1(0);
	rules.addTarget(attackDifficulty(m, member));
	if (rollToFire(m, member)) { finishTurn(m, member); return; }

	const uint8_t targetIndex = orderLow(m, member);
	m.d8(0xA55B) = targetIndex;
	const CombatantId target = splitCombatantIndex(targetIndex);
	uint8_t dl = target.slot;
	if (target.group != m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup))) {
		rules.addRoll(8);
		dl = uint8_t(rules.seed2() >> 8);
	}
	X86Regs x;
	x.dl = dl;
	legacy::call(m, 0x9EFC, x);   // sub_19EFC
	dl = x.dl;

	if (!rules.rollBeatsTarget()) {   // the attack misses
		printMessage(m, 0x1C);
		const CombatantId t = splitCombatantIndex(m.d8(0xA55B));
		selectMember(m, t.member, t.slot, t.member);   // (sic) the group argument is the member number
		setItemQuantityOne(m);
		printMonsterMessage(m, 0x43, t.member);
		printMessage(m, 0x20);
		if (m.d8(g::weaponType) == 0x0D) { finishTurn(m, member); return; }
		const uint8_t high = orderHigh(m, member);
		uint8_t rounds;
		if (high == 0) rounds = 1;
		else if (high == 1) rounds = 3;
		else rounds = useAmmunition(m, 0);
		useAmmunition(m, rounds);
		finishTurn(m, member);
		return;
	}

	uint8_t rounds;
	if (m.d8(g::weaponType) == 0x0D) {
		printLocationMessage(m, 0x3C);
		rules.setSeed1(defenseRoll(m, member, dl));
		m.d8(g::weaponNoteShown) = 0;
		for (uint8_t slotMember = 0; slotMember != 3; ++slotMember) {
			m.d8(0xA559) = slotMember;
			if (targetMembersAlive(m) == 0) continue;
			for (uint8_t monster = 0; monster != 10; ++monster) {
				m.d8(0xA563) = monster;
				const CombatantId t = splitCombatantIndex(m.d8(0xA55B));
				const uint8_t combatant = combatantIndex(t.group, t.slot, slotMember);
				bool killed;
				const bool alive = attackMonster(m, combatant, monster, killed);
				if (alive && killed) {
					monsterExperience(m, combatant);
					addExperience(m);
				}
			}
		}
		weaponSkillRoll(m, m.d8(0xA55B));
		finishTurn(m, member);
		return;
	}
	const uint8_t high = orderHigh(m, member);
	if (high == 0) {
		printAttackMessageB(m);
		while (hitMonster(m, member, m.d8(0xA55B)).alreadyDead) {}
		rounds = 1;
	} else if (high == 2) {
		uint8_t shots = burstSize(m, member);
		m.d8(0xA55A) = shots;
		X86Regs y;
		y.setAx(5);
		legacy::call(m, 0xCBD3, y);   // sub_1CBD3 (reached through sub_11438)
		printAttackMessageC(m);
		for (;;) {
			const CombatantId t = splitCombatantIndex(m.d8(0xA55B));
			const uint8_t pick = uint8_t(rules.random(3) - 1);
			const uint8_t combatant = combatantIndex(t.group, t.slot, pick);
			if (hitMonster(m, member, combatant).alreadyDead) continue;
			bool anyAlive = false;
			for (int8_t mm = 2; mm >= 0; --mm) {
				m.d8(0xA559) = uint8_t(mm);
				const CombatantId s = splitCombatantIndex(m.d8(0xA55B));
				if (countAliveMonsters(m, s.group, s.slot, uint8_t(mm)) != 0) { anyAlive = true; break; }
			}
			if (!anyAlive) { rounds = useAmmunition(m, 0); break; }
			m.d8(0xA55A) = --shots;
			if (shots == 0) { rounds = useAmmunition(m, 0); break; }
		}
	} else {
		uint8_t shots = rules.random(3);
		m.d8(0xA55A) = shots;
		printAttackMessageA(m);
		for (;;) {
			while (hitMonster(m, member, m.d8(0xA55B)).alreadyDead) {}
			m.d8(0xA55A) = --shots;
			if (shots == 0) break;
			if (memberTargetsAlive(m, member) == 0) break;
		}
		rounds = 3;
	}
	m.d8(0xA561) = rounds;

	// the result of this round per hit target
	m.d8(g::weaponNoteShown) = 0;
	for (uint8_t entry = 0; entry != 3; ++entry) {
		m.d8(0xA55A) = entry;
		const uint8_t combatant = m.d8(uint16_t(g::hitTargets + entry));
		if (combatant == 0xFF) break;
		m.d8(0xA55B) = combatant;
		const CombatantId t = splitCombatantIndex(combatant);
		m.d8(0xA559) = t.member;
		const uint16_t damage = m.d16(uint16_t(g::hitDamage + entry * 2));
		m.d8(0xA560) = uint8_t(damage);
		m.d8(0xA55F) = uint8_t(damage >> 8);
		auto announce = [&]() {   // 13061: the monster's name
			++m.d8(g::weaponNoteShown);
			writeMonsterName(m, m.d8(0xA559));
		};
		if (damage == 0) {
			printWeaponNote(m);
			countHitMonsters(m);
			printMessage(m, 0x2F);
			announce();
			printLocationMessage(m, 0x4A);
		} else {
			countHitMonsters(m);
			printWeaponNote(m);
			const uint8_t weaponOffset = equippedItemOffset(m, pm::weapon);
			const uint8_t weaponId = m.d8(uint16_t(cur(m) + weaponOffset));
			uint8_t kind = 5;
			const uint8_t type = m.d8(g::weaponType);
			if (weaponId != 0x11) {
				kind = 4;
				if (!isTypeGroupC(type)) {
					kind = 3;
					if (!isTypeGroupB(type)) {
						kind = 2;
						if (!isTypeGroupA(type)) {
							kind = 1;
							if (type != 3 && type != 4) kind = 0;
						}
					}
				}
			}
			const uint8_t message = uint8_t(kind * 2 + 0x3E);
			m.d8(0xA55D) = message;
			printLocationMessage(m, message);
			announce();
			printLocationMessage(m, uint8_t(message + 1));
			printDamageNumber(m, damage);
			uint8_t kills = m.d8(uint16_t(g::hitKills + entry));
			if (kills != 0) {
				m.d8(0xA55C) = kills;
				if (kills == 1 && m.d8(0xA55E) != 1) kills = 0;
				m.d8(g::hitFlag) = m.d8(uint16_t(g::hitFlags + entry));
				printHitMessage(m, kills);
				if (!hasNoAmmunition(m, m.d8(uint16_t(g::hitStatus + entry)))) {
					m.d8(g::itemQuantity) = m.d8(0xA55C);
					printMessage(m, 0x22);
				}
			}
		}
		printMessage(m, 0x5A);
		weaponSkillRoll(m, m.d8(0xA55B));
		X86Regs fx;
		legacy::call(m, 0x9EFC, fx);   // sub_19EFC
	}
	useAmmunition(m, m.d8(0xA561));
	finishTurn(m, member);
}

uint8_t monsterRangeModifier(Machine &m, uint8_t distance, uint8_t group) {
	selectMonsterDetails(m, group);
	return rangeModifier(m, distance, 0, weaponDamageClass(itemDamageDice(m)));
}

void ambush(Machine &m, uint8_t partyGroup) {
	selectPartyBlock(m, partyGroup);
	if (!anyMemberAlive(m)) return;
	Rules rules(m);
	const uint8_t activeGroup = m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup));
	X86Regs flush;
	for (int slot = 3; slot >= 0; --slot) {
		m.d8(0xA4AA) = uint8_t(slot);
		selectSlot(m, activeGroup, uint8_t(slot));
		const uint16_t slotRecord = m.d16(g::monsterPtr);
		if (m.d8(slotRecord) != 0) {
			m.d8(0xA4B3) = m.d8(uint16_t(slotRecord + 3));
			selectSlotCellRecord(m);
			for (int group = 2; group >= 0; --group) {
				m.d8(0xA4AB) = uint8_t(group);
				m.d8(0xA4B0) = 0;
				if ((m.d8(uint16_t(m.d16(g::savedLocationDetailsPtr) + 9)) >> 1) & 1) break;
				const uint16_t record = m.d16(g::savedLocationDetailsPtr);
				if (m.d8(uint16_t(record + m.d8(uint16_t(g::encounterOffsets + group)))) != 0) {
					selectMonsterDetails(m, uint8_t(group));
					if (hasNoAmmunition(m, itemDamageDice(m))) break;
					selectMember(m, activeGroup, uint8_t(slot), uint8_t(group));
					for (int monster = 9; monster >= 0; --monster) {
						const uint16_t base = m.d16(g::monsterPtr);
						const uint8_t statusOffset = uint8_t(0x14 + monster);
						m.d8(0xA4AC) = statusOffset;
						m.d8(0xA4B2) = uint8_t(monster * 2 + 1);
						const bool alive = m.d16(uint16_t(base + monster * 2)) != 0;
						const uint8_t status = m.d8(uint16_t(base + statusOffset));
						if (!alive && !(status & 0x80)) continue;
						if (status == 0) continue;
						m.d8(uint16_t(base + statusOffset)) = 0;
						markLocationVisited(m);
						rules.setSeed1(0);
						rules.addTarget(monsterRangeModifier(m, m.d8(0xA4B3), uint8_t(group)));
						rules.setSeed2(0);
						const uint8_t roll = rules.rollDice();
						if (roll < 4) { ++m.d8(0xA4B0); continue; }
						rules.addRoll(roll);
						selectMonsterDetails(m, uint8_t(group));
						rules.addRoll(itemByte(m, 2));
						const uint8_t dice = itemDamageDice(m);
						uint8_t kind = 2;
						if (dice != 0x0B && dice != 0x0C) kind = isTypeGroupA(dice) ? 1 : 0;
						m.d8(0xA4B1) = kind;
						rules.addRoll(kind << 3);
						if (!rules.rollBeatsTarget()) { ++m.d8(0xA4B0); continue; }
						const uint16_t margin = uint16_t(rules.seed2() - rules.seed1());
						m.d8(0xA4AF) = uint8_t(margin);
						m.d8(0xA4AE) = uint8_t(margin >> 8);
						legacy::call(m, 0x9EFC, flush);
						setItemQuantityOne(m);
						printMonsterMessage(m, 0x1A, uint8_t(group));
						const uint8_t a = uint8_t(margin), b = uint8_t(margin >> 8);
						if (kind == 0) {
							printLocationMessageVariant(m, 0x4A);
							monsterHitsParty(m, a, b, uint8_t(group));
							continue;
						}
						uint8_t shots, message;
						if (kind == 1) { message = 0x4C; shots = rules.random(3); }
						else { message = 0x4E; shots = burstHits(m, 0x0C); }
						m.d8(0xA4AD) = message;
						m.d8(0xA4B4) = shots;
						m.d8(g::weaponNoteShown) = 0;
						for (;;) {
							printWeaponNote(m);
							printLocationMessageVariant(m, message);
							monsterHitsParty(m, a, b, uint8_t(group));
							legacy::call(m, 0x9EFC, flush);
							++m.d8(g::weaponNoteShown);
							if (!anyMemberAlive(m)) return;
							m.d8(0xA4B4) = --shots;
							if (shots == 0) break;
						}
					}
				}
				const uint8_t missed = m.d8(0xA4B0);
				if (missed != 0) {
					m.d8(g::itemQuantity) = missed;
					legacy::call(m, 0x9EFC, flush);
					writeNumberLeft(m, m.d8(g::itemQuantity));
					writeCharacter(m, 0x20);
					writeMonsterName(m, uint8_t(group));
					printMessage(m, 0x1F);
				}
			}
		}
	}
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
