#include "wasteland1/roster.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
constexpr uint16_t rosterPosition = 0xB242, rosterGroup = 0xB1E0, rosterInverse = 0x471F;

uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
uint8_t &field(Machine &m, uint8_t offset) { return m.d8(uint16_t(cur(m) + offset)); }
X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0, uint8_t dl = 0, uint8_t bl = 0) {
	X86Regs x;
	x.al = al; x.dl = dl; x.bl = bl;
	legacy::call(m, offset, x);
	return x;
}
}

void drawRosterRow(Machine &m) {
	m.d8(0x467F) = 1;
	setWriteCharacterFn2(m);
	m.d8(g::inverseText) = m.d8(rosterInverse);
	writeNumberShort(m, m.d8(rosterPosition));
	writeCharacter2(m, '>');
	m.d8(g::inverseText) = 0;
	writeMemberName(m);
	m.d8(0x4672) = 17;
	++m.d8(0x467F);
	writeNumberShort(m, field(m, 26));   // level
	m.d8(0x4672) = 21;
	uint8_t value = 0;
	if (const uint8_t weapon = field(m, 31)) {   // ammunition of the weapon in hand
		const uint8_t quantity = field(m, uint8_t(weapon * 2 + 188));
		if (!(quantity & 0x80)) {
			value = quantity & 63;
			getEquippedWeaponDetails(m);
			if (hasNoAmmunition(m, uint8_t(m.d8(uint16_t(m.d16(0x4665) + 3)) >> 3))) value = 0;
		}
	}
	writeNumberShort(m, value);
	if (field(m, 40)) m.d8(g::inverseText) = 0xFF;
	m.d8(0x4672) = 24;
	++m.d8(0x467F);
	writeSpaceNumber(m, uint16_t(field(m, 27) | (field(m, 28) << 8)));   // hit points
	m.d8(g::inverseText) = 0;
	m.d8(0x4672) = 28;
	const uint8_t high = field(m, 30);
	if (high & 0x80 || (high | field(m, 29)) == 0) {   // a condition letter
		const uint8_t dl = high & 0x80 ? getMemberConditionIndex(m) : 5;
		setWriteCharacterFn2(m);
		writeItemText(m, m.d8(uint16_t(0xB233 + dl)));
	} else {
		writeSpaceNumber(m, uint16_t(field(m, 29) | (high << 8)));   // hit points while down
	}
	m.d8(0x4672) = 32;
	const X86Regs w = call(m, 0x96C9);   // getMemberWeaponIdPtr: al = offset of the weapon's id, zf: none
	uint8_t item = 0;
	if (!w.zf) {
		item = field(m, w.al);
		if (field(m, uint8_t(w.al + 1)) & 0x80) m.d8(g::inverseText) = 0xFF;
	}
	m.d8(0x4687) = 1;
	writeItemName(m, item);
	m.d8(g::inverseText) = 0;
}

void redrawRosterRow(Machine &m, uint8_t position, uint8_t inverse) {
	m.d8(rosterInverse) = inverse;
	m.d8(rosterPosition) = position;
	setCurrentCharacter(m, position);
	if (m.d8(0x46B9) == 0) return;
	if (m.d8(0x4654) != m.d8(rosterGroup)) {
		call(m, 0x728C);   // writeRosterHeadersAndMembers
		return;
	}
	saveRectBounds(m);
	m.d8(0x4675) = 38;
	m.d8(0x4674) = 1;
	m.d8(0x4673) = uint8_t(position + 15);
	m.d8(0x4672) = 1;
	writeCharacter2(m, 3);
	if (position <= m.d8(0x4653)) drawRosterRow(m);
	restoreRectBounds(m);
	setWriteCharacterFn(m);
}

void drawRosterFrame(Machine &m) {
	if (m.d8(0x46B9) == 0) return;
	m.d8(rosterGroup) = m.d8(0x4654);
	m.d8(0xB1E2) = uint8_t(m.d8(rosterGroup) + 87);
	m.d8(0xB20B) = uint8_t(m.d8(0xB1E2) + 62);
	m.d8(0xB1E4) = uint8_t(m.d8(0x4657) + 87);
	m.d8(0xB20D) = uint8_t(m.d8(0xB1E4) + 62);
	auto frameString = [&](uint16_t text, uint8_t line) {
		m.setD16(g::textString, text);
		m.d8(0x8DD0) = 0;
		m.d8(0x8DD1) = line;
		for (;;) {
			const uint8_t c = m.d8(m.d16(g::textString));
			if (c == 0) break;
			call(m, 0x0039, c);   // draws one frame character
			m.setD16(g::textString, uint16_t(m.d16(g::textString) + 1));
			if (m.d16(g::textString) == 0) break;
		}
	};
	frameString(0xB1E1, 14);
	frameString(0xB20A, 15);
	while (++m.d8(0x8DD1) != 23) {
		m.d8(0x8DD0) = 0;
		call(m, 0x0039, 13);
		m.d8(0x8DD0) = 39;
		call(m, 0x0039, 19);
	}
	m.d8(0x8DD0) = 0;
	call(m, 0x0039, 16);
	do call(m, 0x0039, 18); while (m.d8(0x8DD0) != 39);
	call(m, 0x0039, 17);
	for (uint8_t position = 1; position != 8; ++position) redrawRosterRow(m, position, 0);
	m.d8(0xB243) = 7;   // (the loop counter of the original)
}


void drawRosterHeadersAndMembers(Machine &m) {
	m.setD16(0x8DD2, uint16_t(m.d16(0x8DD2) & 0xAF3F));
	m.d8(0x46B9) = 1;
	m.d8(0x4719) = 0;
	if (m.d8(0x46FE) != 0) call(m, 0xA1EF);   // sub_1A1EF
	drawRosterFrame(m);
}

void showTitleText(Machine &m, uint8_t page) {
	m.d8(0xB262) = page;
	saveRectBounds(m);
	const uint8_t keep = m.d8(0x46F7);
	m.d8(0x46F7) = 0;
	m.d8(0x46FF) = 1;
	if (m.d8(0x46B9) != 0 && m.d8(0x4719) == 0) {
		m.d8(0x8DD9) = 1;
		call(m, 0x63C4);   // sub_163C4
		m.d8(0x8DD9) = 0;
	}
	if (m.d8(0x8DF6) != m.d8(0x8DF7)) call(m, 0x79B7, m.d8(0x8DF6));   // sub_179B7: the background picture
	m.d8(0x4672) = m.d8(0xB261);
	m.d8(0x4673) = m.d8(0xB260);
	m.d8(0x4676) = 144;
	m.d8(0x4677) = 192;
	m.d8(0x4674) = 1;
	m.d8(0x4675) = 38;
	setWriteCharacterFn(m);
	m.d8(0x8DF8) = 1;
	writeEncodedText(m, m.d8(0xB262));
	m.d8(0x8DF8) = 0;
	m.d8(0x8DF7) = m.d8(0x8DF6);
	m.d8(0xB261) = m.d8(0x4672);
	m.d8(0xB260) = m.d8(0x4673);
	--m.d8(0x46FF);
	m.d8(0x46F7) = keep;
	restoreRectBounds(m);
}

} // namespace wl
