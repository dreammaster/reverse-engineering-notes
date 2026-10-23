#include "wasteland1/interact.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/movement.h"
#include "wasteland1/party.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

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
		if (rec(m, i) == 0xFF) { at = 0xFF; return false; }
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
		if (rec(m, i) == 0xFF) { at = 0xFF; return true; }
		if (conditionType(m, i) == 0 && rec(m, uint8_t(i + 1)) == skillId) { at = i; break; }
		if (uint8_t(i + 2) == 0) { at = i; passed = conditionType(m, i); break; }   // falls out with the last type in al
	}
	return !skillCheck(m, passed, conditionValue(m, at));
}

bool matchAttributeCondition(Machine &m, uint8_t attributeId, uint8_t &at) {
	m.d8(0xA5C9) = attributeId;
	for (uint8_t i = 0x0A;; i = uint8_t(i + 2)) {
		m.d8(0xA5C8) = i;
		if (rec(m, i) == 0xFF) { at = 0xFF; return true; }
		if (conditionType(m, i) == 2 && rec(m, uint8_t(i + 1)) == attributeId) { at = i; break; }
		if (uint8_t(i + 2) == 0) return true;   // (this scan falls out at the end instead of into the match)
	}
	return !fieldCheck(m, conditionValue(m, at), rec(m, uint8_t(at + 1)));
}

namespace {
void setPacing(Machine &m, uint8_t value) {   // 14314
	m.d8(0xA5E7) = m.d8(0x46F7);
	m.d8(0xA5E6) = m.d8(uint16_t(g::saved2 + 0x0D));
	m.d8(0x46F7) = 1;
	m.d8(uint16_t(g::saved2 + 0x0D)) = value;
}
void restorePacing(Machine &m) {   // 14307
	m.d8(uint16_t(g::saved2 + 0x0D)) = m.d8(0xA5E6);
	m.d8(0x46F7) = m.d8(0xA5E7);
}
}

void showActionMessage(Machine &m) {
	selectWriterForRoster(m);
	setPacing(m, m.d8(0xA5C5));
	printMapMessage(m, rec(m, 3));
	X86Regs x;
	x.dl = 4;
	legacy::call(m, 0x8DB4, x);   // delayFrames
	restorePacing(m);
}

void applyActionEffect(Machine &m, uint8_t member) {
	m.d8(0xA5C7) = member;
	setCurrentCharacter(m, member);
	const uint8_t field = rec(m, 8);
	if (field == 0) return;
	m.d8(0xA5C6) = field & 0x7F;
	Rules rules(m);
	if (field & 0x80) rules.setSeed2(rec(m, 9) & 0x7F);
	else {
		uint8_t al;
		rules.setSeed2(rules.sumOfD6(0, rec(m, 9) & 0x7F, al));
	}
	setPacing(m, m.d8(0xA5C5));
	adjustMemberField(m, member, uint8_t(field & 0x7F), rec(m, 9));
	m.d8(0xA5C5) = m.d8(uint16_t(g::saved2 + 0x0D));
	restorePacing(m);
}

void applyActionEffectToParty(Machine &m) {
	const uint8_t members = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	for (uint8_t member = 1;; ++member) {
		m.d8(0xA5C1) = member;
		applyActionEffect(m, member);
		if (member >= members) break;
	}
}


// 16D1A: shows string rec[at] (through the title page display) when it is not zero
void showRecordString(Machine &m, uint8_t at) {
	const uint8_t id = rec(m, at);
	if (id == 0) return;
	X86Regs x;
	x.al = id;
	legacy::call(m, 0x7920, x);   // sub_17920
}

namespace {
// 17B3E: true (CF) when the current character has at least the amount in the number value
bool canPay(Machine &m) {
	const uint16_t c = cur(m);
	const uint32_t money = m.d8(uint16_t(c + pm::money)) | (m.d8(uint16_t(c + pm::money + 1)) << 8) | (m.d8(uint16_t(c + pm::money + 2)) << 16);
	const uint32_t need = m.d8(g::numberValue) | (m.d8(uint16_t(g::numberValue + 1)) << 8) | (m.d8(uint16_t(g::numberValue + 2)) << 16);
	return money >= need;
}



// 142B1: offset of the data that follows the condition list, relative to `at`
uint8_t afterConditions(Machine &m, uint8_t at) {
	m.d8(0xA5C0) = at;
	uint8_t bl = at;
	for (;;) {
		if (rec(m, bl) == 0xFF) break;
		bl = uint8_t(bl + 2);
		if (bl == 0) break;
	}
	++bl;
	m.d8(0xA5C0) = bl;
	return uint8_t(uint8_t(at - 0x0A) + bl);
}
}

bool evaluateAction(Machine &m, uint8_t x, uint8_t y, uint8_t &result) {
	uint8_t al = 0, bl = 0, member = 0, at = 0;
	const uint8_t members = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	m.d8(0xA5D6) = x;
	m.d8(0xA5D5) = y;
	selectCellLocation(m, x, y);
	m.d8(0xA5D0) = 0; m.d8(0xA5D4) = 0; m.d8(0xA5D2) = 0; m.d8(0xA5D1) = 0;
	showRecordString(m, 1);
	if (!(rec(m, 0) & 0x40)) { result = 1; return false; }
	member = 1;
memberLoop:   // 13EF6
	m.d8(0xA5D3) = member;
	if (isMemberUnconscious(m, member)) goto nextMember;
	at = 0x0A;
conditionLoop:   // 13F03
	m.d8(0xA5CF) = at;
	if (rec(m, at) == 0xFF) goto conditionsEnd;
	switch (conditionType(m, at)) {
	case 4:   // money
		m.d8(g::numberValue) = rec(m, uint8_t(at + 1));
		m.d8(uint16_t(g::numberValue + 1)) = 0;
		m.d8(uint16_t(g::numberValue + 2)) = 0;
		if (canPay(m)) goto conditionsEnd;
		goto conditionMet;
	case 3:   // party size
		if (rec(m, uint8_t(at + 1)) != members) goto conditionFailed;
		goto conditionMet;
	case 2: {   // attribute
		const uint8_t count = conditionValue(m, at);
		if (!fieldCheck(m, count, rec(m, uint8_t(at + 1)))) goto conditionFailed;
		goto conditionMet;
	}
	case 0: {   // skill
		const uint8_t skill = rec(m, uint8_t(at + 1));
		if (getMemberSkillLevel(m, skill) == 0) goto conditionFailed;
		const uint8_t value = conditionValue(m, m.d8(0xA5CF));
		if (value == 0) goto conditionMet;
		if (skillCheck(m, rec(m, uint8_t(m.d8(0xA5CF) + 1)), value)) goto conditionMet;
		goto conditionFailed;
	}
	default: {   // item
		const int found = findItemById(m, rec(m, uint8_t(at + 1)));
		if (found < 0) goto conditionFailed;
		useUpItem(m, uint8_t(found));
		goto conditionSatisfied;
	}
	}
conditionMet:   // 13F62
conditionSatisfied:   // 1404B
	++m.d8(0xA5D0);
	if (actionByte0(m) & 8) {
		showRecordString(m, 2);
		bl = m.d8(0xA5CF);
		al = afterConditions(m, bl);
		goto finish;
	}
	if (actionByte0(m) & 0x20) goto nextMember;
	goto okExit;
conditionFailed:   // 13FA4
	at = uint8_t(m.d8(0xA5CF) + 2);
	if (at == 0) goto conditionsEnd;
	goto conditionLoop;
conditionsEnd:   // 13FB1
	++m.d8(0xA5D4);
	if (actionByte0(m) & 0x10) {   // 14010: the effect hits the whole party
		++m.d8(0xA5D2);
		showActionMessage(m);
		applyActionEffectToParty(m);
		goto afterEffects;
	}
	if (actionByte8(m) != 0) {
		if (m.d8(0xA5D2) == 0) {
			const uint8_t b8 = actionByte8(m);
			if (b8 == 0x1D) showActionMessage(m);
			else if (b8 == 0x15) showRecordString(m, 3);
		}
		applyActionEffect(m, m.d8(0xA5D3));
		++m.d8(0xA5D2);
		if ((actionByte0(m) & 0x20) == 0) goto failExit;
	} else {
		if ((actionByte0(m) & 0x20) == 0) goto failExit;
	}
nextMember:   // 13FED
	if (uint8_t(m.d8(0xA5D3) + 1) <= members) {
		member = uint8_t(m.d8(0xA5D3) + 1);
		goto memberLoop;
	}
	if (m.d8(0xA5D4) == 0) goto okExit;
afterEffects:   // 1401A
	if ((actionByte0(m) & 4) == 0) goto failExit;
	if (m.d8(0xA5D0) != 0) goto okExit;
failExit:   // 14028
	if (m.d8(0xA5D2) == 0) showRecordString(m, 3);
	else if (actionByte8(m) == 0x1D) {
		writeCharacter(m, 6);
	}
	++m.d8(0xA5D1);
	al = 6;
	goto finish;
okExit:   // 1406A
	showRecordString(m, 2);
	al = 4;
finish:   // 1406F
	{
		X86Regs r;
		r.al = al;
		r.dl = m.d8(0xA5D6);
		r.bl = m.d8(0xA5D5);
		legacy::call(m, 0x7CFF, r);   // sub_17CFF
		result = m.d8(0xA5D1);
		return r.cf;
	}
}

namespace {
void pageText(Machine &m, uint8_t page) {   // 13E64: title page `page` after preparing the message pointer
	m.setD16(g::textPointer1, 0xAB3E);   // 16CBD
	X86Regs x;
	x.al = page;
	legacy::call(m, 0x7923, x);   // displayTitleText2
}
void refreshLayoutIfShown(Machine &m) {   // 13E85
	if (m.d8(0x4724) == 0) { X86Regs x; legacy::call(m, 0x63C4, x); }
}
void newlineIfShown(Machine &m) {   // 13E8F
	if (m.d8(0x4724) == 0) writeCharacter(m, 6);
}
bool npcRefuses(Machine &m, uint8_t field) {   // 13E13 / 13E18 / 13E1D: the NPC's willingness against the given refusal counter
	m.d8(0xA5DA) = field;
	return refusalCheck(m, m.d8(uint16_t(cur(m) + pm::willingness)), field);
}
}

bool refusalCheck(Machine &m, uint8_t max, uint8_t field) {
	m.d8(0xBA69) = max;
	m.d8(0xBA68) = field;
	Rules rules(m);
	uint8_t &value = m.d8(uint16_t(cur(m) + field));
	if (value == 0xFF) return true;
	rules.setSeed1(0);
	rules.addTarget(int32_t(Rules::targetBase(0)));   // (resetRandomSeed1 clears dl, so the value never counts)
	rules.setSeed2(0);
	rules.addRoll(max);
	const uint8_t roll = rules.rollDice();
	if (roll < 5) {
		if (value != 0x0A) ++value;
		return true;
	}
	rules.addRoll(roll);
	if (!rules.rollBeatsTarget()) return true;
	if (rules.random(20) == 1 && value != 0) --value;
	return false;
}

// 13E3D: true (CF) when the cell in direction A5D9 holds no object; the "nothing there" text has been shown then
static bool noObjectInWay(Machine &m) {
	uint8_t x, y;
	partyCell(m, x, y);
	stepTarget(m.d8(0xA5D9), x, y);
	if (selectCellLocation(m, x, y) == 2) return false;
	refreshLayoutIfShown(m);
	pageText(m, (m.d8(0xA5E0) == 0 && m.d8(0xA5DB) == 9) ? 0x26 : 0x0C);
	return true;
}

bool useSkillOnObject(Machine &m, uint8_t mode, uint8_t dir, uint8_t param, uint8_t &result) {
	m.d8(0xA5E0) = mode;
	m.d8(0xA5D9) = dir;
	m.d8(0xA5DB) = param;
	const uint8_t member = m.d8(0x4661);
	m.d8(0xA5DC) = member;
	setCurrentCharacter(m, member);
	auto changeCell = [&](uint8_t offset) -> bool {   // 13E7F + 17CFF: the cell in direction A5D9
		uint8_t x, y;
		partyCell(m, x, y);
		stepTarget(m.d8(0xA5D9), x, y);
		X86Regs r;
		r.al = offset; r.dl = x; r.bl = y;
		legacy::call(m, 0x7CFF, r);   // sub_17CFF
		return r.cf;
	};
	auto npcZero = [&]() { return m.d8(uint16_t(cur(m) + pm::npc)) == 0; };
	uint8_t at = 0, none = 0;   // none: the al the failed attempt carries (FFh when the record has no such condition)
	uint8_t offset;

	if (mode == 2) {
		if (!npcZero() && npcRefuses(m, 0x2D)) goto refused;
		if (noObjectInWay(m)) { result = 1; return true; }
		if (matchAttributeCondition(m, param, at)) { none = at == 0xFF ? 0xFF : 0; goto failed; }
		goto passed;
	}
	if (mode == 1) {
		if (!npcZero() && npcRefuses(m, 0x2B)) goto refused;
		if (noObjectInWay(m)) { result = 1; return true; }
		if (!matchItemCondition(m, param, at)) { none = 0xFF; goto failed; }
		goto passed;
	}
	if (!npcZero() && npcRefuses(m, 0x2C)) goto refused;
	if (param == 0x19 || param == 0x20) {
		// 13D82: helping an unconscious character up
		const uint8_t shift = param == 0x19 ? 2 : 3;
		m.d8(0xA5DE) = shift;
		setCurrentRosterCharacter(m, m.d8(0xA5D9));
		if (conBytesOr(m) == 0 || !(m.d8(uint16_t(cur(m) + pm::con + 1)) & 0x80)) { result = 0; return true; }
		setCurrentCharacter(m, member);
		const int roll = skillRoll(m, param);
		setCurrentRosterCharacter(m, m.d8(0xA5D9));
		bool success = false;
		if (roll >= 0) {
			Rules rules(m);
			rules.setSeed1(0);
			const uint8_t v = uint8_t(absByte(m.d8(uint16_t(cur(m) + pm::con))) >> shift);
			m.d8(0xA5DD) = v;
			rules.addTarget(int32_t(Rules::targetBase(v)));
			success = rules.rollBeatsTarget();
		}
		if (!success) {
			setCurrentRosterCharacter(m, m.d8(0xA5D9));
			printMessage(m, 9);
			newlineIfShown(m);
			result = 0;
			return true;
		}
		const uint16_t c = cur(m);
		const uint16_t con = uint16_t(m.d8(uint16_t(c + pm::con)) | (m.d8(uint16_t(c + pm::con + 1)) << 8));
		m.setD16(uint16_t(c + pm::con), uint16_t((con >> 1) | 0x8000));
		printMessage(m, 0x0A);
		setCurrentCharacter(m, member);
		trySkillImprovement(m, param, m.d8(0xA5DD));
		newlineIfShown(m);
		result = 0;
		return true;
	}
	if (noObjectInWay(m)) { result = 1; return true; }
	if (matchSkillCondition(m, param, at)) {
		none = at == 0xFF ? 0xFF : 0;
		if (param == 9 && none != 0) {
			X86Regs x;
			legacy::call(m, 0x7A6B, x);   // clearBottomArea
			pageText(m, 0x26);
			result = 1;
			return true;
		}
		goto failed;
	}

passed:   // 13CE8
	m.d8(0xA5DA) = at;
	refreshLayoutIfShown(m);
	showRecordString(m, 2);
	offset = 4;
	if (actionByte0(m) & 8) {
		offset = at;
		{
			// 142B1: where the data that follows the condition list starts
			uint8_t bl = at;
			m.d8(0xA5C0) = bl;
			for (;;) {
				if (m.d8(uint16_t(m.d16(g::locationDetailsPtr) + bl)) == 0xFF) break;
				bl = uint8_t(bl + 2);
				if (bl == 0) break;
			}
			++bl;
			m.d8(0xA5C0) = bl;
			offset = uint8_t(uint8_t(at - 0x0A) + bl);
		}
	}
	m.d8(0xA5DA) = offset;
	if (changeCell(offset)) goto stepFailed;
	moveParty(m, m.d8(0xA5D9));
	result = offset;   // (al is left over from the move)
	return false;

stepFailed:   // 13D30
	if (!tryStep(m, m.d8(0xA5D9))) m.d8(0x4716) = 0;
	result = 0;
	return false;

failed:   // 13D3F
	m.d8(0xA5DF) = none;
	m.d8(0x470D) = m.d8(0xA5DC);
	if (actionByte8(m) == 0) {
		X86Regs x;
		x.al = m.d8(0x8DF7);
		legacy::call(m, 0x79B7, x);   // sub_179B7
		if ((actionByte0(m) & 2) || !(none & 0x80)) showRecordString(m, 3);   // 13E6E
		else pageText(m, 0x3D);
	} else {
		showActionMessage(m);
		if (!(actionByte0(m) & 0x10)) applyActionEffect(m, m.d8(0xA5DC));
		else applyActionEffectToParty(m);
		writeCharacter(m, 6);
		refreshLayoutIfShown(m);
	}
	if (changeCell(6)) goto stepFailed;
	result = 6;
	return false;

refused:   // 13D07
	selectWriterForRoster(m);
	printMessage(m, 0x45);
	newlineIfShown(m);
	result = 0;
	return true;
}

} // namespace wl
