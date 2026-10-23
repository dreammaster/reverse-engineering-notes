#include "wasteland1/interact.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"

namespace wl {

namespace {
uint8_t rec(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationDetailsPtr) + i)); }
uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
}

uint8_t conditionType(Machine &m, uint8_t at) { return uint8_t(rec(m, at) >> 5); }
uint8_t conditionValue(Machine &m, uint8_t at) { return uint8_t(rec(m, at) & 0x1F); }

int skillRoll(Machine &m, uint8_t skillId) {
	m.d8(0xBA72) = skillId;
	Rules rules(m);
	rules.setSeed2(0);
	const uint8_t roll = rules.rollDice();
	if (roll < 5) return -1;
	m.d8(0xBA71) = roll;
	rules.addRoll(roll);
	getItemDetails2(m, skillId);
	const uint8_t attribute = itemFlag(m, 1);
	if (attribute != 0) rules.addRoll(m.d8(uint16_t(cur(m) + attribute)));
	rules.addRoll(int32_t(getMemberSkillLevel(m, skillId)) * 3);
	return roll;
}

bool skillCheck(Machine &m, uint8_t skillId, uint8_t count) {
	m.d8(0xBA74) = skillId;
	m.d8(0xBA73) = count;
	Rules rules(m);
	rules.setSeed1(0);
	rules.addTarget(int32_t(Rules::targetBase(count)));
	const int roll = skillRoll(m, skillId);
	if (roll < 0) return false;
	if (m.d8(0x916B) != 0) {
		m.d8(g::numberValue) = uint8_t(roll);
		m.d8(uint16_t(g::numberValue + 1)) = 0;
		m.d8(uint16_t(g::numberValue + 2)) = 0;
		addExperience(m);
	}
	if (!rules.rollBeatsTarget()) return false;
	trySkillImprovement(m, skillId, count);
	return true;
}

void useUpItem(Machine &m, uint8_t offset) {
	m.d8(0xCCD8) = offset;
	const uint8_t itemId = m.d8(uint16_t(cur(m) + offset));
	m.d8(0xCCD7) = itemId;
	const uint8_t quantity = m.d8(uint16_t(cur(m) + offset + 1)) & 0x3F;
	if (quantity != 0) {
		if (quantity == 1) {
			removeWeaponArmor(m, uint8_t((offset - 0xBB) >> 1));
			removeItem(m, m.d8(0xCCD8));
		} else {
			const uint8_t left = uint8_t(quantity - 1);
			m.d8(0xCCD6) = left;
			uint8_t &q = m.d8(uint16_t(cur(m) + offset + 1));
			q = uint8_t((q & 0xC0) | left);
		}
	}
	getItemDetails(m, itemId);   // 199FC
	if ((itemFlag(m, 3) & 7) != 0) {
		X86Regs x;
		legacy::call(m, 0x142B, x);   // sub_1142B
	}
}

bool matchItemCondition(Machine &m, uint8_t itemOffset, uint8_t &at) {
	m.d8(0xA5CE) = itemOffset;
	const uint8_t itemId = m.d8(uint16_t(cur(m) + itemOffset));
	m.d8(0xA5CD) = itemId;
	for (uint8_t i = 0x0A;; i = uint8_t(i + 2)) {
		m.d8(0xA5CC) = i;
		if (rec(m, i) == 0xFF) return false;
		if (conditionType(m, i) == 1 && rec(m, uint8_t(i + 1)) == itemId) { at = i; break; }
		if (uint8_t(i + 2) == 0) { at = i; break; }
	}
	useUpItem(m, itemOffset);
	return true;
}

bool matchSkillCondition(Machine &m, uint8_t skillId, uint8_t &at) {
	m.d8(0xA5CB) = skillId;
	uint8_t passed = skillId;
	for (uint8_t i = 0x0A;; i = uint8_t(i + 2)) {
		m.d8(0xA5CA) = i;
		if (rec(m, i) == 0xFF) return true;
		if (conditionType(m, i) == 0 && rec(m, uint8_t(i + 1)) == skillId) { at = i; break; }
		if (uint8_t(i + 2) == 0) { at = i; passed = conditionType(m, i); break; }   // falls out with the last type in al
	}
	return !skillCheck(m, passed, conditionValue(m, at));
}

bool matchAttributeCondition(Machine &m, uint8_t attributeId, uint8_t &at) {
	m.d8(0xA5C9) = attributeId;
	for (uint8_t i = 0x0A;; i = uint8_t(i + 2)) {
		m.d8(0xA5C8) = i;
		if (rec(m, i) == 0xFF) return true;
		if (conditionType(m, i) == 2 && rec(m, uint8_t(i + 1)) == attributeId) { at = i; break; }
		if (uint8_t(i + 2) == 0) return true;   // (this scan falls out at the end instead of into the match)
	}
	return !fieldCheck(m, conditionValue(m, at), rec(m, uint8_t(at + 1)));
}

} // namespace wl
