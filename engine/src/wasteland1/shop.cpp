#include "wasteland1/shop.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/menu.h"
#include "wasteland1/party.h"
#include "wasteland1/roster.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
constexpr uint16_t serviceFlag = 0xDCE0, memberVar = 0xDCE1, cureFlag = 0xDCE2, afflictionIndex = 0xDCE3, healFlag = 0xDCE4,
                   afflictionMasks = 0xDCE5;

uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
uint8_t &field(Machine &m, uint8_t offset) { return m.d8(uint16_t(cur(m) + offset)); }
uint8_t &rec(Machine &m, uint8_t i) { return m.d8(uint16_t(m.d16(g::locationDetailsPtr) + i)); }
X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0, uint8_t dl = 0, uint8_t bl = 0) {
	X86Regs x;
	x.al = al; x.dl = dl; x.bl = bl;
	legacy::call(m, offset, x);
	return x;
}
void shopMessage(Machine &m, uint8_t id) {   // 1C561
	setWriteCharacterFn(m);
	m.setD16(g::textPointer1, 0xDCED);
	writeEncodedText(m, id);
}
void amountOf(Machine &m, uint8_t at) {   // 1C548: numberValue = byte `at` of the shop record
	m.d8(g::numberValue) = rec(m, at);
	m.d8(uint16_t(g::numberValue + 1)) = 0;
	m.d8(uint16_t(g::numberValue + 2)) = 0;
}
void showMoney(Machine &m) {   // 1C22F
	const uint8_t x = m.d8(g::rectTextX), y = m.d8(g::rectTextY);
	m.d8(g::rectTextX) = 0x0F;
	m.d8(g::rectTextY) = 1;
	setWriteCharacterFn2(m);
	m.setD16(g::textPointer1, 0xDBF8);
	writeEncodedText(m, 4);
	m.d8(0x467F) = 8;
	getMemberMoney(m);
	writeNumber(m);
	m.d8(g::rectTextX) = x;
	m.d8(g::rectTextY) = y;
}
void listAfflictions(Machine &m, uint8_t bits) {   // 1C510
	for (uint8_t i = 0; i != 8; ++i) {
		m.d8(0x469F) = i;
		const bool set = bits & 1;
		bits >>= 1;
		if (!set) continue;
		writeCharacter(m, 0x10);
		writeCharacter(m, uint8_t(i + '1'));
		writeCharacter(m, ')');
		writeCharacterVia(m, 32);
		shopMessage(m, uint8_t(i + 1));
		newline(m);
	}
}
}

bool payNumberValue(Machine &m) {
	const uint32_t price = uint32_t(m.d8(g::numberValue)) | (uint32_t(m.d8(uint16_t(g::numberValue + 1))) << 8) |
	                       (uint32_t(m.d8(uint16_t(g::numberValue + 2))) << 16);
	const uint32_t money = uint32_t(field(m, pm::money)) | (uint32_t(field(m, pm::money + 1)) << 8) | (uint32_t(field(m, pm::money + 2)) << 16);
	if (money < price) return false;
	const uint32_t left = money - price;
	field(m, pm::money) = uint8_t(left);
	field(m, pm::money + 1) = uint8_t(left >> 8);
	field(m, pm::money + 2) = uint8_t(left >> 16);
	return true;
}

void poolMoney(Machine &m, uint8_t member) {
	m.d8(0xCCD2) = member;
	for (uint8_t n = 1;;) {
		m.d8(0xCCD3) = n;
		if (n != member) {
			setCurrentCharacter(m, n);
			getMemberMoney(m);
			call(m, 0x98BC, 0);   // resetMoney: the money of `n` becomes 0
			setCurrentCharacter(m, member);
			addMoneyToMember(m);
		}
		++n;
		if (n > m.d8(uint16_t(g::saved2 + g::s2::membersInGroup))) return;
	}
}

bool doctorShop(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint8_t keepPicture = D(0x46FA), keepRoster = D(g::rosterLocOn);
	for (uint8_t i = 0; i != 13; ++i) D(uint16_t(0x7201 + i)) = rec(m, uint8_t(7 + i));   // the location's name
	call(m, 0x90A6, 0);   // writeRosterLocation2
screen:   // 1C286
	call(m, 0x728C);
	D(serviceFlag) = 1;
	D(cureFlag) = 0;
	D(healFlag) = 0;
	printMapMessage(m, rec(m, 3));
	shopMessage(m, 0x17);
	{
		const uint8_t pick = pickGroupMember(m, false);
		if (pick == 0) {
			D(g::rosterLocOn) = keepRoster;
			D(0x46FA) = keepPicture;
			return true;
		}
		D(memberVar) = pick;
		redrawRosterRow(m, pick, 0xFF);
	}
	if (conBytesOr(m) == 0) {   // already dead
		shopMessage(m, 9);
		goto screen;
	}
patient:   // 1C2CE
	if (D(serviceFlag) == 0 && field(m, pm::afflictions) != 0) D(cureFlag) = 1;
	if (!isAtFullHealth(m)) D(healFlag) = 1;
	if ((D(serviceFlag) | D(cureFlag) | D(healFlag)) == 0) goto screen;
	shopMessage(m, 0x0A);
	if (D(cureFlag)) shopMessage(m, 0x0D);
	if (D(serviceFlag)) {
		shopMessage(m, 0x0B);
		writeNumberLeft(m, rec(m, 5));
	}
	if (D(healFlag)) shopMessage(m, 0x0C);
	newline(m);
	showMoney(m);
menu:   // 1C345
	m.setD16(g::buttonMask1, 0x8404);
	m.setD16(g::buttonMask2, 0);
	{
		waitForKeypress(m);
		const uint8_t key = D(g::lastKey);
		if (key == 0x1B) goto screen;
		if (key == 'P') {
			poolMoney(m, D(memberVar));
			goto patient;
		}
		if (key == 'H') {
			if (!D(healFlag)) goto menu;
		heal:   // 1C375
			shopMessage(m, 0x0E);
			writeNumberLeft(m, rec(m, 4));
			shopMessage(m, 0x0F);
			if (isMemberUnconscious(m, D(memberVar))) {
				shopMessage(m, 0x16);
			} else {
				const uint16_t gap = uint16_t((field(m, pm::maxCon) | (field(m, pm::maxCon + 1) << 8)) -
				                              (field(m, pm::con) | (field(m, pm::con + 1) << 8)));
				writeNumber16(m, gap);
			}
			shopMessage(m, 0x10);
			showMoney(m);
			for (;;) {   // 1C3BD
				m.setD16(g::buttonMask1, 0x8404);
				m.setD16(g::buttonMask2, 0);
				waitForKeypress(m);
				const uint8_t answer = D(g::lastKey);
				if (answer == 0x1B || answer == 'C') goto patient;
				if (answer == 'P') {
					poolMoney(m, D(memberVar));
					showMoney(m);
					continue;
				}
				if (answer != 'H') continue;
				amountOf(m, 4);
				if (!payNumberValue(m)) {
					shopMessage(m, 0x12);   // not enough money
					goto patient;
				}
				const bool below = healOneHitPoint(m);
				if (conBytesOr(m) == 0) healOneHitPoint(m);
				redrawRosterRow(m, D(memberVar), D(0x471F));
				if (below) goto heal;
				D(healFlag) = 0;
				goto patient;
			}
		}
		if (key == 'C') goto cureMenu;
		if (key == 'E') {
			if (!D(serviceFlag)) goto menu;
			amountOf(m, 5);
			if (!payNumberValue(m)) {
				shopMessage(m, 0x12);
				goto patient;
			}
			if (field(m, pm::afflictions) == 0) {
				shopMessage(m, 0x11);
			} else {
				shopMessage(m, 0x13);
				listAfflictions(m, field(m, pm::afflictions));
				writeCharacter(m, 6);
			}
			D(serviceFlag) = 0;
			goto patient;
		}
		goto menu;
	}
cureMenu:   // 1C418
	if (!D(cureFlag)) goto menu;
	call(m, 0x9DC0, 4);
	call(m, 0x9DC0, 0x0D);
	shopMessage(m, 0x14);
	writeNumberLeft(m, rec(m, 6));
	shopMessage(m, 0x15);
	listAfflictions(m, field(m, pm::afflictions));
	showMoney(m);
	for (;;) {   // 1C44F
		m.setD16(g::buttonMask1, 0x8404);
		m.setD16(g::buttonMask2, 0);
		waitForKeypress(m);
		const uint8_t key = D(g::lastKey);
		if (key == 0x1B) goto patient;
		if (key == 'P') {
			poolMoney(m, D(memberVar));
			showMoney(m);
			continue;
		}
		if (key < '1' || key >= '9') continue;
		const uint8_t index = uint8_t((key & 0x0F) - 1);
		const uint8_t mask = D(uint16_t(afflictionMasks + index));
		if (!(field(m, pm::afflictions) & mask)) continue;
		D(afflictionIndex) = index;
		amountOf(m, 6);
		if (!payNumberValue(m)) {
			shopMessage(m, 0x12);
			goto patient;
		}
		field(m, pm::afflictions) &= uint8_t(~mask);
		redrawRosterRow(m, D(memberVar), D(0x471F));
		if (field(m, pm::afflictions) != 0) goto cureMenu;
		D(cureFlag) = 0;
		goto patient;
	}
}

} // namespace wl
