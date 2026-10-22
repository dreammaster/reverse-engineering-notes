#include "wasteland1/party.h"
#include "wasteland1/globals.h"

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

} // namespace wl
