#include "wasteland1/party.h"
#include "wasteland1/globals.h"
#include "wasteland1/character.h"

namespace wl {

namespace {
uint16_t curChar(const Machine &m) { return m.d16(g::currentCharacter); }
}

void setPartyIndexesPtr(Machine &m, uint8_t group) {
	m.setD16(g::partyIndexesPtr, uint16_t(g::saved + m.d8(uint16_t(g::partiesOffsets + group))));
}

void setCurrentCharacter(Machine &m, uint8_t partyNum) {
	setPartyIndexesPtr(m, m.d8(uint16_t(g::saved2 + 6)));   // active party group
	const uint8_t index = m.d8(uint16_t(m.d16(g::partyIndexesPtr) + partyNum));
	const uint16_t base = uint16_t(g::saved + (uint16_t(index) << 8));
	m.setD16(g::currentCharacter, base);
	m.d8(g::currentCharacterSex) = m.d8(uint16_t(base + pm::sex));
}

bool isMemberUnconscious(Machine &m, uint8_t partyNum) {
	setCurrentCharacter(m, partyNum);
	const uint16_t c = curChar(m);
	const uint8_t hi = m.d8(uint16_t(c + pm::con + 1)), lo = m.d8(uint16_t(c + pm::con));
	if (hi & 0x80) return true;
	return (hi | lo) == 0;
}

uint8_t conBytesOr(Machine &m) {
	const uint16_t c = curChar(m);
	return uint8_t(m.d8(uint16_t(c + pm::con + 1)) | m.d8(uint16_t(c + pm::con)));
}

uint8_t getMemberConditionIndex(Machine &m) {
	const uint16_t c = curChar(m);
	uint8_t dl = 5;
	if ((m.d8(uint16_t(c + pm::con + 1)) | m.d8(uint16_t(c + pm::con))) == 0) return dl;
	const uint8_t al = m.d8(uint16_t(c + pm::con));
	--dl;
	for (;;) {
		const uint8_t thr = m.d8(uint16_t(g::conditionThresholds + dl));
		if (al <= thr) return dl;
		--dl;
		if (dl == 0) return dl;
	}
}

uint8_t getMemberSkillLevel(Machine &m, uint8_t skillId) {
	const uint16_t c = curChar(m);
	for (uint8_t off = pm::skills; off != pm::skillsEnd; off = uint8_t(off + 2))
		if (m.d8(uint16_t(c + off)) == skillId) return m.d8(uint16_t(c + off + 1));
	return 0;
}

int findItemById(Machine &m, uint8_t itemId) {
	const uint16_t c = curChar(m);
	for (uint8_t off = pm::items; off != pm::inventoryEnd; off = uint8_t(off + 2))
		if (m.d8(uint16_t(c + off)) == itemId) return off;
	return -1;
}

uint8_t getInventoryItemOffset(uint8_t slot) { return uint8_t(slot * 2 + pm::items - 2); }

uint8_t getMemberWeaponOffset(Machine &m, uint8_t &itemIdOut) {
	const uint16_t c = curChar(m);
	uint8_t al = m.d8(uint16_t(c + pm::weapon));
	if (al != 0) al = getInventoryItemOffset(al);
	itemIdOut = al;
	return al;
}

void getItemDetailsInner(Machine &m, uint8_t id) {
	m.setD16(g::itemDetailsPtr, uint16_t(m.d16(g::itemDetailsBasePtr) + uint16_t(id) * m.d8(g::itemDetailsRecSize)));
}

void getItemDetails(Machine &m, uint8_t id) {
	m.setD16(g::itemDetailsBasePtr, uint16_t(g::itemDetails + 8));
	m.d8(g::itemDetailsRecSize) = 8;
	getItemDetailsInner(m, id);
}

void getItemDetails2(Machine &m, uint8_t skillId) {
	m.setD16(g::itemDetailsBasePtr, g::itemDetails2);
	m.d8(g::itemDetailsRecSize) = 2;
	getItemDetailsInner(m, skillId);
}

uint8_t itemFlag(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::itemDetailsPtr) + offset)); }

uint8_t getItemType(Machine &m) { return uint8_t(itemFlag(m, 3) >> 3); }

uint8_t countPartyMembers(Machine &m, uint8_t group, uint8_t *stop) {
	setPartyIndexesPtr(m, group);
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	uint8_t count = 0, at = 1;
	for (;;) {
		if (m.d8(uint16_t(indexes + at)) == 0) break;
		++count;
		++at;
		if (at == 8) break;
	}
	if (count == 0) at = 1;
	if (stop) *stop = at;
	return count;
}

void storePartyPosition(Machine &m) {
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	m.d8(uint16_t(indexes + 8)) = m.d8(0x46A6);
	m.d8(uint16_t(indexes + 9)) = m.d8(0x46A7);
	m.d8(uint16_t(indexes + 10)) = m.d8(uint16_t(g::saved2 + g::s2::field7));
}

void setActivePartyGroup(Machine &m, uint8_t group) {
	setPartyIndexesPtr(m, m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup)));
	storePartyPosition(m);
	m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup)) = group;
	m.d8(uint16_t(g::saved2 + g::s2::membersInGroup)) = countPartyMembers(m, group);
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	m.d8(0x46A6) = m.d8(uint16_t(indexes + 8));
	m.d8(g::saved2) = uint8_t(m.d8(0x46A6) - m.d8(0x46A8));
	m.d8(0x46A7) = m.d8(uint16_t(indexes + 9));
	m.d8(uint16_t(g::saved2 + 1)) = uint8_t(m.d8(0x46A7) - m.d8(0x46A9));
	m.d8(uint16_t(g::saved2 + g::s2::field7)) = m.d8(uint16_t(indexes + 10));
}

uint8_t smallestGroupValue(Machine &m, uint8_t &index) {
	uint8_t best = 0xFF;
	index = 0;
	const uint8_t total = m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups));
	for (uint8_t i = 0; i <= total; ++i) {
		const uint8_t v = m.d8(uint16_t(0x7111 + i));
		if (best > v) { best = v; index = i; }
	}
	return best;
}

void pendingCell(Machine &m, uint8_t &x, uint8_t &y) {
	x = m.d8(0xA60D);
	y = m.d8(0xA60C);
}

void setViewRowPointer(Machine &m, uint8_t row) {
	uint16_t pointer = row;
	uint8_t width = m.d8(0x46A5);
	while (width != 0 && !(width & 1)) {
		pointer = uint16_t(pointer << 1);
		width >>= 1;
	}
	m.setD16(0x46CA, uint16_t(pointer + 0x3448));
}

uint8_t shiftLeft4(uint8_t v) { return uint8_t(v << 4); }

bool cellToScreen(Machine &m, uint8_t &x, uint8_t &y) {
	x = uint8_t((x - m.d8(g::saved2)) * 2);
	y = shiftLeft4(uint8_t(y - m.d8(uint16_t(g::saved2 + 1))));
	return m.d8(0x46B9) != 0 && y >= 0x0D;
}

bool outsideMap(Machine &m, uint8_t x, uint8_t y) { return y >= m.d8(0x46A4) || x >= m.d8(0x46A5); }

uint8_t compareTextWithRecord(Machine &m) {
	for (uint8_t i = 0;; ++i) {
		const uint8_t a = m.d8(uint16_t(m.d16(g::textString) + i)), b = m.d8(uint16_t(m.d16(g::itemDetailsPtr) + i));
		if ((a | b) == 0) return 0;
		if (a != b) return 0xFF;
		if (i == 255) return 0xFF;
	}
}

void copyCharacterRecord(Machine &m, uint8_t dest, uint8_t source) {
	if (source == 0) return;
	const uint16_t from = uint16_t(g::saved + (uint16_t(source) << 8));
	m.setD16(g::currentCharacter, from);
	m.setD16(0x4661, from);
	const uint16_t to = uint16_t(g::saved + (uint16_t(dest) << 8));
	m.setD16(g::currentCharacter, to);
	m.d8(g::currentCharacterSex) = m.d8(uint16_t(to + pm::sex));
	for (int i = 0; i < 256; ++i) m.d8(uint16_t(from + i)) = m.d8(uint16_t(to + i));
}

uint8_t findInSelectedGroup(Machine &m, uint8_t id) {
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	for (uint8_t at = 1; at != 8; ++at)
		if (m.d8(uint16_t(indexes + at)) == id) return at;
	return 0;
}

void appendToSelectedGroup(Machine &m, uint8_t id) {
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	uint8_t at = 1;
	while (at != 8 && m.d8(uint16_t(indexes + at)) != 0) ++at;
	m.d8(uint16_t(indexes + at)) = id;   // (a full list overwrites byte 8, the party's x position)
	++m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
}

void removeFromActiveGroup(Machine &m, uint8_t id) {
	setPartyIndexesPtr(m, m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup)));
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	uint8_t at = findInSelectedGroup(m, id);
	if (at == 0) at = 8;   // (not found: the shifting below then runs through all 256 offsets, as the original does)
	for (uint8_t bl = uint8_t(at + 1); bl != 8; ++bl) m.d8(uint16_t(indexes + uint8_t(bl - 1))) = m.d8(uint16_t(indexes + bl));
	m.d8(uint16_t(indexes + 7)) = 0;
	--m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
}

void clearPartyGroup(Machine &m, uint8_t group) {
	setPartyIndexesPtr(m, group);
	const uint16_t indexes = m.d16(g::partyIndexesPtr);
	for (int i = 0x0D; i >= 0; --i) m.d8(uint16_t(indexes + i)) = 0;
}

bool moveToGroup(Machine &m, uint8_t position, uint8_t from, uint8_t to, uint8_t &movedId) {
	m.d8(0xA6C4) = to;
	setPartyIndexesPtr(m, from);
	uint16_t indexes = m.d16(g::partyIndexesPtr);
	const uint8_t id = m.d8(uint16_t(indexes + position));
	if (id == 0) return false;
	for (int i = 2; i >= 0; --i) m.d8(uint16_t(0xA6D2 + i)) = m.d8(uint16_t(indexes + 0x0B + (2 - i)));
	setPartyIndexesPtr(m, to);
	if (findInSelectedGroup(m, id) != 0) return false;
	appendToSelectedGroup(m, id);
	--m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	storePartyPosition(m);
	indexes = m.d16(g::partyIndexesPtr);
	for (int i = 2; i >= 0; --i) m.d8(uint16_t(indexes + 0x0B + (2 - i))) = m.d8(uint16_t(0xA6D2 + i));
	movedId = id;
	return true;
}

void deleteCharacter(Machine &m, uint8_t position) {
	m.d8(0xB23E) = position;
	setPartyIndexesPtr(m, m.d8(uint16_t(g::saved2 + g::s2::activePartyGroup)));
	uint16_t indexes = m.d16(g::partyIndexesPtr);
	const uint8_t id = m.d8(uint16_t(indexes + position));
	for (uint8_t at = id != 0 ? uint8_t(id + 1) : uint8_t(0); at != 8; ++at) {
		m.d8(0xB23F) = at;
		copyCharacterRecord(m, at, uint8_t(at - 1));
	}
	m.d8(0xB23F) = id;
	uint8_t bl = position;
	if (bl == 0) {   // (the original moves idx[0] to idx[255] once and then carries on at 1)
		m.d8(uint16_t(indexes + 0xFF)) = m.d8(indexes);
		bl = 1;
	} else {
		++bl;
	}
	for (; bl != 8; ++bl) m.d8(uint16_t(indexes + uint8_t(bl - 1))) = m.d8(uint16_t(indexes + bl));
	m.d8(uint16_t(indexes + 7)) = 0;
	--m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	--m.d8(uint16_t(g::saved2 + g::s2::field8));
	for (int group = m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups)); group >= 0; --group) {
		m.d8(0xB23E) = uint8_t(group);
		setPartyIndexesPtr(m, uint8_t(group));
		indexes = m.d16(g::partyIndexesPtr);
		for (uint8_t at = 7; at != 0; --at) {
			const uint8_t v = m.d8(uint16_t(indexes + at));
			if (v >= id) m.d8(uint16_t(indexes + at)) = uint8_t(v - 1);
		}
	}
}

bool mergeGroupsAtSamePosition(Machine &m) {
	auto S = [&](uint8_t o) -> uint8_t & { return m.d8(uint16_t(g::saved2 + o)); };
	m.d8(0xA6C9) = S(g::s2::totalPartyGroups);
	uint8_t other;
	for (;;) {
		other = m.d8(0xA6C9);
		if (other != S(g::s2::activePartyGroup)) {
			setPartyIndexesPtr(m, other);
			const uint16_t indexes = m.d16(g::partyIndexesPtr);
			if (m.d8(uint16_t(indexes + 0x0A)) == S(g::s2::field7) && m.d8(uint16_t(indexes + 8)) == m.d8(0x46A6) &&
			    m.d8(uint16_t(indexes + 9)) == m.d8(0x46A7)) break;
		}
		if (--m.d8(0xA6C9) & 0x80) {
			setPartyIndexesPtr(m, S(g::s2::activePartyGroup));   // 15DC4
			return false;
		}
	}
	for (uint8_t position = 1;; ++position) {   // 15DF3: move everybody over
		m.d8(0xA6CC) = position;
		uint8_t moved;
		if (!moveToGroup(m, position, other, S(g::s2::activePartyGroup), moved)) break;
		++S(g::s2::membersInGroup);
	}
	if (!(m.d8(0x4706) < S(g::s2::activePartyGroup)) && !(m.d8(0x4706) < other)) --m.d8(0x4706);
	if (S(g::s2::activePartyGroup) >= other) --S(g::s2::activePartyGroup);
	for (uint8_t group = other; group != 4;) {   // 15E32: close the gap in the group table
		setPartyIndexesPtr(m, group);
		const uint16_t indexes = m.d16(g::partyIndexesPtr);
		for (int i = 0x0D, from = 0x1B; i >= 0; --i, --from) m.d8(uint16_t(indexes + i)) = m.d8(uint16_t(indexes + from));
		++group;
		m.d8(0xA6C9) = group;
	}
	--S(g::s2::totalPartyGroups);
	clearPartyGroup(m, 3);
	setPartyIndexesPtr(m, S(g::s2::activePartyGroup));
	return true;
}

void splitOffCharacter(Machine &m, uint8_t id, uint8_t x, uint8_t y) {
	m.d8(0xA6CE) = id;
	m.d8(0xA6D1) = x;
	m.d8(0xA6D0) = y;
	const uint8_t group = partyGroupOfRoster(m, id);
	m.d8(0xA6CD) = group;
	setCurrentRosterCharacter(m, id);
	const uint16_t c = m.d16(g::currentCharacter);
	if (m.d8(uint16_t(c + pm::con + 1)) & 0x80) return;
	if ((m.d8(uint16_t(c + pm::con + 1)) | m.d8(uint16_t(c + pm::con))) == 0) return;
	uint8_t target = group;
	if (m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups)) != 3 && countPartyMembers(m, group) >= 2) {
		const uint8_t position = findInSelectedGroup(m, id);
		uint8_t moved;
		moveToGroup(m, position, group, uint8_t(m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups)) + 1), moved);
		setActivePartyGroup(m, group);
		removeFromActiveGroup(m, id);
		target = ++m.d8(uint16_t(g::saved2 + g::s2::totalPartyGroups));
		m.d8(0xA6CD) = target;
	}
	setActivePartyGroup(m, target);
	m.d8(0x46A6) = x;
	m.d8(0x46A7) = y;
	setActivePartyGroup(m, target);
}

} // namespace wl
