#include "wasteland1/actions.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/party.h"

namespace wl {
namespace actions {

namespace {

constexpr uint16_t kAfflictionTable = 0xCF4E;   // ds table: affliction bits per action parameter

uint8_t &rec(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationDetailsPtr) + i)); }
uint8_t &mapByte(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationsPtr) + i)); }
uint8_t &s2(Machine &m, uint8_t i) { return m.d8(uint16_t(g::saved2 + i)); }

// 1A4D8 and its copies: bytes 1, 2 = the pair at offset `source`
void branch(Machine &m, uint8_t source) {
	const uint8_t a = rec(m, source), b = rec(m, uint8_t(source + 1));
	rec(m, 2) = b;
	rec(m, 1) = a;
}

// the original pushes locationDetailsPtr, looks at another record with 17CB1 and restores it
struct KeepActionPointer {
	Machine &m;
	uint16_t saved;
	explicit KeepActionPointer(Machine &mm) : m(mm), saved(mm.d16(g::locationDetailsPtr)) {}
	~KeepActionPointer() { m.setD16(g::locationDetailsPtr, saved); }
};

void setCharacterById(Machine &m, uint8_t id) {   // 19624
	const uint16_t base = uint16_t(g::saved + (uint16_t(id) << 8));
	m.setD16(g::currentCharacter, base);
	m.d8(g::currentCharacterSex) = m.d8(uint16_t(base + pm::sex));
}

void addAfflictions(Machine &m) {
	const uint8_t bits = m.d8(uint16_t(kAfflictionTable + rec(m, 3)));
	m.d8(uint16_t(m.d16(g::currentCharacter) + pm::afflictions)) |= bits;
}

void patchJump(Machine &m, uint8_t index, bool withSecond) {
	const uint8_t a = rec(m, 3), b = rec(m, 4);
	KeepActionPointer keep(m);
	selectLocationRecord(m, 5, index);
	rec(m, 2) = 0x5E;
	rec(m, 3) = a;
	rec(m, 4) = withSecond ? b : 0;
}

void countDown(Machine &m, uint8_t index) {   // stores byte - 1 while the result stays in 1..7Fh
	uint8_t &v = mapByte(m, index);
	const uint8_t next = uint8_t(v - 1);
	if (next != 0 && !(next & 0x80)) v = next;
}

void countUp(Machine &m, uint8_t index, uint8_t limit) {   // stores byte + 1 (8 bit) while it is below `limit`
	uint8_t &v = mapByte(m, index);
	const uint8_t next = uint8_t(v + 1);
	if (next < limit) v = next;
}

} // namespace

void branchIfOtherPartyAt(Machine &m) {
	const uint8_t active = s2(m, g::s2::activePartyGroup);
	for (int group = s2(m, g::s2::totalPartyGroups); group >= 0; --group) {
		m.d8(0xCF30) = uint8_t(group);
		if (group == active) continue;
		setPartyIndexesPtr(m, uint8_t(group));
		const uint16_t indexes = m.d16(g::partyIndexesPtr);
		if (m.d8(uint16_t(indexes + 0x0A)) != s2(m, g::s2::field7)) continue;
		for (uint8_t at = 7; ; at = uint8_t(at + 2)) {
			m.d8(0xCF31) = at;
			const uint8_t x = rec(m, at);
			if (x == 0xFF) break;
			const uint8_t y = rec(m, uint8_t(at + 1));
			if (m.d8(uint16_t(indexes + 8)) == x && m.d8(uint16_t(indexes + 9)) == y) {
				branch(m, 3);
				setPartyIndexesPtr(m, active);
				return;
			}
			if (uint8_t(at + 2) == 0) break;
		}
	}
	branch(m, 5);
	setPartyIndexesPtr(m, active);
}

void branchIfFirstUnconscious(Machine &m) { branch(m, isMemberUnconscious(m, 1) ? 5 : 3); }

void branchByDayTime(Machine &m) {
	const uint8_t v = s2(m, 0x0C);
	branch(m, (v >= 6 && v < 0x12) ? 3 : 5);
}

void stashFirstMember(Machine &m) {
	setCurrentCharacter(m, 1);
	const uint8_t slot = rec(m, 3);
	const uint16_t source = m.d16(g::currentCharacter);
	const uint16_t dest = uint16_t(0x65C + (slot << 8));
	for (int i = 0; i < 256; ++i) m.d8(uint16_t(dest + i)) = m.d8(uint16_t(source + i));
	for (int i = pm::items; i != pm::inventoryEnd; ++i) m.d8(uint16_t(dest + i)) = 0;
	m.d8(uint16_t(dest + pm::weapon)) = 0;
	m.d8(uint16_t(dest + pm::armorClass)) = 0;
	m.d8(uint16_t(dest + pm::armor)) = 0;
	for (int i = 0; i != 3; ++i) m.d8(uint16_t(dest + pm::money + i)) = 0;
	const uint8_t offset = uint8_t(slot << 2);
	m.d8(uint16_t(0xA5C + offset)) = s2(m, 2);
	m.d8(uint16_t(0xA5D + offset)) = s2(m, 3);
	m.d8(uint16_t(0xA5E + offset)) = s2(m, 4);
	m.d8(uint16_t(0xA5F + offset)) = m.d8(uint16_t(g::saved + 0xFD));
}

void copyActionChain(Machine &m) {
	const uint8_t from = rec(m, 3), to = rec(m, 4);
	KeepActionPointer keep(m);
	selectLocationRecord(m, 5, from);
	const uint16_t source = m.d16(g::locationDetailsPtr);
	m.setD16(0x4663, source);
	selectLocationRecord(m, 5, to);
	const uint16_t dest = m.d16(g::locationDetailsPtr);
	for (uint8_t at = 2;; at = uint8_t(at + 2)) {
		const uint8_t v = m.d8(uint16_t(source + at));
		m.d8(uint16_t(dest + at)) = v;
		if (v == 0x5E || v == 0xDE) {
			m.d8(uint16_t(dest + at + 1)) = m.d8(uint16_t(source + at + 1));
			m.d8(uint16_t(dest + at + 2)) = m.d8(uint16_t(source + at + 2));
			break;
		}
		if (v == 0xFF) break;
		m.d8(uint16_t(dest + at + 1)) = m.d8(uint16_t(source + at + 1));
		if (uint8_t(at + 2) == 0) {   // bl wrapped: the code falls into the terminator copy
			m.d8(uint16_t(dest + 1)) = m.d8(uint16_t(source + 1));
			m.d8(uint16_t(dest + 2)) = m.d8(uint16_t(source + 2));
			break;
		}
	}
}

void branchIfItemOrDrop(Machine &m) {
	uint8_t nobody = 1;
	const uint8_t members = s2(m, g::s2::membersInGroup);
	for (uint8_t member = 1;; ++member) {
		m.d8(0xCF4D) = member;
		setCurrentCharacter(m, member);
		if (findItemById(m, 0x2F) >= 0) {
			nobody = 0;
			m.d8(0xCF2C) = 0;
		} else {
			const uint16_t c = m.d16(g::currentCharacter);
			const uint16_t old = m.d16(uint16_t(c + pm::con));
			m.setD16(uint16_t(c + pm::con), 0xFFFB);
			m.setD16(uint16_t(c + pm::lastCon), old);
		}
		if (member >= members) break;
	}
	branch(m, nobody ? 5 : 3);
}

void patchJumpAction9(Machine &m) { patchJump(m, 9, true); }
void patchJumpActionShort(Machine &m) { patchJump(m, 0x19, false); }
void patchJumpActionF(Machine &m) { patchJump(m, 0x0F, true); }

void afflictSelected(Machine &m) {
	setCurrentCharacter(m, m.d8(0x470D));
	addAfflictions(m);
}

void afflictFirst(Machine &m) {
	setCurrentCharacter(m, 1);
	addAfflictions(m);
}

void afflictAll(Machine &m) {
	const uint8_t members = s2(m, g::s2::membersInGroup);
	for (uint8_t member = 1;; ++member) {
		m.d8(0xCF2A) = member;
		setCurrentCharacter(m, member);
		addAfflictions(m);
		if (member >= members) break;
	}
}

void afflictRoster(Machine &m) {
	const uint8_t last = s2(m, g::s2::field8);
	for (uint8_t id = 1;; ++id) {
		m.d8(0xCF25) = id;
		setCharacterById(m, id);
		addAfflictions(m);
		if (id >= last) break;
	}
}

void branchIfPartyAt(Machine &m) {
	branch(m, (rec(m, 7) == m.d8(0x46A6) && rec(m, 8) == m.d8(0x46A7)) ? 3 : 5);
}

void toggleSex(Machine &m) {
	const uint8_t last = s2(m, g::s2::field8);
	for (uint8_t id = 1;; ++id) {
		m.d8(0xCF26) = id;
		setCharacterById(m, id);
		m.d8(uint16_t(m.d16(g::currentCharacter) + pm::sex)) ^= 1;
		if (id >= last) break;
	}
}

void decrement31(Machine &m) { countDown(m, 0x31); }
void decrement32(Machine &m) { countDown(m, 0x32); }

void increment2F_2(Machine &m) {
	uint8_t &v = mapByte(m, 0x2F);
	if (v != 0 && v + 2 <= 0xFF) v = uint8_t(v + 2);
}

void decrement2F_2(Machine &m) {
	uint8_t &v = mapByte(m, 0x2F);
	if (v != 0) {
		const int next = v - 2;
		if (next > 0 && next < 0x80) v = uint8_t(next);
	}
}

void increment32(Machine &m) { countUp(m, 0x32, 0x15); }
void increment31Max0B(Machine &m) { countUp(m, 0x31, 0x0B); }
void increment31Max0C(Machine &m) { countUp(m, 0x31, 0x0C); }
void increment31Max11(Machine &m) { countUp(m, 0x31, 0x11); }
void set2F_14(Machine &m) { mapByte(m, 0x2F) = 0x14; }

void increment2F_10(Machine &m) {
	uint8_t &v = mapByte(m, 0x2F);
	const uint8_t next = uint8_t(v + 0x0A);
	if (next < 0xC8) v = next;
}

void set2F_0(Machine &m) { mapByte(m, 0x2F) = 0; }
void set2F_1E(Machine &m) { mapByte(m, 0x2F) = 0x1E; }
void set2F_64(Machine &m) { mapByte(m, 0x2F) = 0x64; }

void setTable3Byte9Range10_21(Machine &m) {
	KeepActionPointer keep(m);
	const uint8_t value = rec(m, 3);
	for (uint8_t index = 0x10; index != 0x22; ++index) {
		m.d8(0xCF3C) = index;
		selectLocationRecord(m, 3, index);
		rec(m, 9) = value;
	}
}

void setTable3Byte9At22(Machine &m) {
	KeepActionPointer keep(m);
	const uint8_t value = rec(m, 3);
	selectLocationRecord(m, 3, 0x22);
	rec(m, 9) = value;
}

void setTable3Params0_1D(Machine &m) {
	KeepActionPointer keep(m);
	const uint8_t a = rec(m, 3), b = rec(m, 4);
	for (int index = 0x1D; index >= 0; --index) {
		m.d8(0xCF3E) = uint8_t(index);
		selectLocationRecord(m, 3, uint8_t(index));
		rec(m, 3) = a;
		rec(m, 9) = b;
	}
}

void setTable3Byte9To1Range21_28(Machine &m) {
	KeepActionPointer keep(m);
	for (uint8_t index = 0x21;; ++index) {
		m.d8(0xCF3E) = index;
		selectLocationRecord(m, 3, index);
		rec(m, 9) = 1;
		if (index == 0x28) break;
	}
}

void setTable3Byte9Range0_9(Machine &m) {
	KeepActionPointer keep(m);
	const uint8_t value = rec(m, 3);
	m.d8(0xCF46) = value;
	for (int index = 9; index >= 0; --index) {
		m.d8(0xCF45) = uint8_t(index);
		selectLocationRecord(m, 3, uint8_t(index));
		rec(m, 9) = value;
	}
}

void checkGateCells(Machine &m) {
	for (uint8_t y = 9; y <= 11; ++y) {
		const Cell c = cellAt(m, 0x13, y);
		if (c.tileClass != 4 || c.action != 2) return;
	}
	rec(m, 1) = 0x0C;
	rec(m, 2) = 3;
}

void branchIfItemListed(Machine &m) {
	const uint8_t members = s2(m, g::s2::membersInGroup);
	for (uint8_t member = 1;; ++member) {
		m.d8(0xCF20) = member;
		setCurrentCharacter(m, member);
		for (uint8_t at = 7; ; ++at) {
			m.d8(0xCF21) = at;
			const uint8_t item = rec(m, at);
			if (item == 0xFF) break;
			if (findItemById(m, item) >= 0) { branch(m, 3); return; }
			if (uint8_t(at + 1) == 0) break;
		}
		if (member >= members) break;
	}
	branch(m, 5);
}

} // namespace actions
} // namespace wl
