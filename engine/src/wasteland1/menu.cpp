#include "wasteland1/menu.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0, uint8_t dl = 0, uint8_t bl = 0) {
	X86Regs x;
	x.al = al; x.dl = dl; x.bl = bl;
	legacy::call(m, offset, x);
	return x;
}
}

uint8_t chooseByKey(Machine &m) {
	for (;;) {
		if (waitForKeypress(m)) return 0xFF;
		const uint16_t text = m.d16(g::textString);
		for (uint8_t at = 0;; ++at) {
			const uint8_t c = m.d8(uint16_t(text + at));
			if (c == 0) break;   // not in the string: wait for the next key
			if (c == m.d8(g::lastKey)) return at;
			if (at == 0xFF) return 0xFF;
		}
	}
}

uint8_t pickGroupMember(Machine &m, bool wide) {
	m.setD16(g::buttonMask1, wide ? 0x0404 : 0x0401);
	m.setD16(g::buttonMask2, 0);
	for (;;) {
		if (waitForKeypress(m)) return 0;
		const uint8_t key = m.d8(g::lastKey);
		if (key < '1' || key > '9') continue;
		const uint8_t number = key & 0x0F;
		const uint8_t count = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
		if (number <= count) return number;
	}
}

int askDirection(Machine &m) {
	m.d8(0x8DDA) = 1;   // 25BFA
	const uint8_t shown = m.d8(g::rosterLocOn);
	m.d8(g::rosterLocOn) = 0;   // 125B7
	call(m, 0x7A6B);   // clearBottomArea
	call(m, 0x9083);   // saveRectBounds
	m.d8(g::rectLeftCol) = 0x0F;
	m.setD16(g::textString, 0xA469);   // "Which way?"
	call(m, 0x7868, 0, 0x0F, 0x12);   // writeTextStringSupportsLowercase: "Which way?"
	call(m, 0x9060);   // restoreRectBounds
	m.setD16(g::textString, 0xA45C);   // "IKJL "
	m.setD16(g::buttonMask1, 0x1002);
	m.setD16(g::buttonMask2, 0);
	const uint8_t index = chooseByKey(m);
	m.d8(g::rosterLocOn) = shown;
	m.d8(0x8DDA) = 0;
	if (index & 0x80) return -1;
	return index > 4 ? index - 5 : index;
}


// 13AE4: the "Skill / Item / Attribute" menu for party member `member`: asks what the member should use and on whom. Returns
// false (CF) when cancelled. `mode` is 0 skill, 1 item, 2 attribute; `target` the party member (or direction), `what` the
// skill, item or attribute code. ds:A5E1..A5E4 hold the working copies
bool chooseUseAction(Machine &m, uint8_t member, uint8_t &mode, uint8_t &target, uint8_t &what) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	enum : uint16_t { Member = 0xA5E1, Target = 0xA5E2, Mode = 0xA5E3, What = 0xA5E4, Fixed = 0x472C };
	uint8_t al;
	D(Member) = member;
restart:   // 3AE7
	call(m, 0x3C52, D(Member));   // sub_13C52: highlight the member
	if (D(Fixed)) { al = 0; goto haveMode; }
	printMessage(m, 4);
	m.setD16(g::textString, 0xA5E8);   // "SIA"
	m.setD16(g::buttonMask1, 0x0404);
	m.setD16(g::buttonMask2, 0);
	al = chooseByKey(m);
	if (al & 0x80) return false;
haveMode:   // 3B12
	D(Mode) = al;
again:   // 3B15
	if (D(Mode) == 0) goto skill;
	if (D(Mode) == 1) goto itemMenu;
	goto askTarget;

skill:   // 3B29
	setCurrentCharacter(m, D(Member));
	if (D(Fixed)) {
		if (m.d8(uint16_t(m.d16(g::currentCharacter) + pm::skills)) == 0) return false;
		al = 1;
	} else {
		m.setD16(g::buttonMask1, 0x0434);
		m.setD16(g::buttonMask2, 0);
		al = call(m, 0x98F0).al;   // sub_198F0: pick a skill
		if (al == 0xFF) goto restart;
		if (al >= 0xFD) goto skill;
	}
selected:   // 3B5E
	al = uint8_t(al * 2 + 0x7E);
	al = m.d8(uint16_t(m.d16(g::currentCharacter) + al));
	D(What) = al;
	if (!D(Fixed)) call(m, 0x6F20);   // sub_16F20
	D(Fixed) = 0;
	D(Mode) = 0;
	if (al == 0x19 || al == 0x20) {
		call(m, 0x728C);   // writeRosterHeadersAndMembers
		for (;;) {   // 3B91: whom to help
			printMessage(m, 5);
			al = pickGroupMember(m, false);
			if (al == 0) goto skill;
			if (al == D(Member)) { printMessage(m, 6); continue; }
			D(Target) = al;
			setCurrentCharacter(m, al);
			if (conBytesOr(m) == 0) { printMessage(m, 7); continue; }
			if (!isMemberUnconscious(m, D(Target))) { printMessage(m, 8); continue; }
			al = m.d8(uint16_t(m.d16(g::partyIndexesPtr) + D(Target)));
			D(Target) = al;
			if (!(al & 0x80)) goto done;
			break;
		}
		goto itemKeys;
	}
	goto chosen;

itemKeys:   // 3BDE
askTarget:
	printMessage(m, 0x0B);
	for (;;) {   // 3BE3: attribute 1..8
		m.setD16(g::buttonMask1, 0x0404);
		m.setD16(g::buttonMask2, 0);
		const bool esc = waitForKeypress(m);
		al = m.d8(g::lastKey);
		if (esc) goto restart;
		if (al < '1' || al > '8') continue;
		al = uint8_t(al - '1' + 0x0E);
		D(Mode) = 2;
		goto chosen2;
	}

itemMenu:   // 3C0B
	m.setD16(g::buttonMask1, 0x0434);
	m.setD16(g::buttonMask2, 0);
	{
		const X86Regs x = call(m, 0x9394, D(Member));   // sub_19394: pick an item
		if (x.cf) goto restart;
		al = uint8_t(x.al * 2 + 0xBB);
	}
	D(Mode) = 1;
	goto chosen2;

chosen:   // 3C25 with dl = 0 (3B7B cleared it)
	D(Mode) = 0;
chosen2:
	D(What) = al;
	call(m, 0x63C4);   // sub_163C4
	for (;;) {   // 3C2F: which way
		const int d = askDirection(m);
		if (d < 0) {
			call(m, 0x728C);
			call(m, 0x3C52, D(Member));
			goto again;
		}
		D(Target) = uint8_t(d);
		if (!call(m, 0x64E0).cf) break;   // sub_164E0
	}
done:   // 3C45
	mode = D(Mode);
	target = D(Target);
	what = D(What);
	return true;
}

} // namespace wl
