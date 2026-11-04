#include "wasteland1/charscreen.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/interact.h"
#include "wasteland1/legacy.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"
#include "wasteland1/party.h"
#include "wasteland1/roster.h"
#include "wasteland1/shop.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
constexpr uint16_t memberVar = 0xCCED, tradeId = 0xCCEE, tradeQuantity = 0xCCEF, tradeSlot = 0xCCF0, itemOffsetVar = 0xCCF1;

uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
uint8_t &field(Machine &m, uint8_t offset) { return m.d8(uint16_t(cur(m) + offset)); }
void waitForSpaceKey(Machine &m) { waitSpaceOrEnter(m); }   // 18EAD
bool currentDown(Machine &m) {   // 172BE
	const uint8_t high = field(m, 30);
	if (high & 0x80) return true;
	return (high | field(m, 29)) == 0;
}
}

void divideMoney(Machine &m, uint8_t &high, uint8_t &mid, uint8_t &low, uint8_t divisor) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint8_t saved = low;
	unsigned n = D(0xBA86);
	D(0xBA86) = uint8_t(n / divisor);
	n = ((n % divisor) << 8) | mid;
	mid = uint8_t(n / divisor);
	D(0xBA85) = mid;
	n = ((n % divisor) << 8) | saved;
	low = uint8_t(n / divisor);
	D(0xBA84) = low;
	D(0xBA80) = uint8_t(n % divisor);
	high = D(0xBA86);
}

void useItem(Machine &m, uint8_t slot) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(tradeSlot) = slot;
	getEquippedWeaponDetails(m);
	const uint8_t offset = getInventoryItemOffset(D(tradeSlot));
	D(itemOffsetVar) = offset;
	const uint8_t id = field(m, offset);
	if (id == m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 7))) {
		writeItemTextNormal(m, 141);   // "Reload?"
		if (!askYesLoose(m)) {
			reload(m, D(itemOffsetVar));
			return;
		}
	}
	if (field(m, uint8_t(D(itemOffsetVar) + 1)) & 0x80) {
		writeItemTextNormal(m, 134);   // "Unjam?"
		if (!askYesLoose(m)) {
			unjam(m, D(memberVar), D(itemOffsetVar));
			waitForSpaceKey(m);
			return;
		}
	}
	writeItemTextNormal(m, 142);   // "Do you want to ..."
	for (;;) {
		m.setD16(g::buttonMask1, 0x0404);
		m.setD16(g::buttonMask2, 0);
		if (waitForKeypress(m)) return;
		const uint8_t key = D(g::lastKey);
		if (key == 'D') {
		drop:   // 1927D
			const uint8_t s = D(tradeSlot);
			removeItem(m, getInventoryItemOffset(s));
			removeWeaponArmor(m, s);
			return;
		}
		if (key == 'E') {
			equip(m, D(tradeSlot));
			redrawRosterRow(m, D(memberVar), D(0x471F));
			return;
		}
		if (key != 'T') continue;
		const uint8_t members = D(uint16_t(g::saved2 + g::s2::membersInGroup));
		if (members == 1) {
			writeItemTextNormal(m, 143);   // no one to trade with
			return;
		}
		uint8_t target;
		if (members == 2 && (D(memberVar) ^ 3) != 0) {
			target = uint8_t(D(memberVar) ^ 3);
		} else {
			drawRosterHeadersAndMembers(m);
			setCurrentCharacter(m, D(memberVar));
			for (;;) {
				writeItemTextNormal(m, 144);
				target = pickGroupMember(m, false);
				if (target == 0) return;
				if (target != D(memberVar)) break;
			}
		}
		{
			const uint8_t from = getInventoryItemOffset(D(tradeSlot));
			D(tradeId) = field(m, from);
			D(tradeQuantity) = field(m, uint8_t(from + 1));
			if (field(m, pm::npc) != 0 && !currentDown(m)) {
				setCurrentCharacter(m, target);
				const uint8_t charisma = field(m, pm::charisma);
				setCurrentCharacter(m, D(memberVar));
				if (refusalCheck(m, charisma, pm::tradeRefuse)) {
					writeItemTextNormal(m, 135);   // refuses
					return;
				}
			}
			setCurrentCharacter(m, target);
			const int free = findItemById(m, 0);
			if (free < 0) {
				writeItemTextNormal(m, 150);   // can't carry any more
				return;
			}
			field(m, uint8_t(free)) = D(tradeId);
			field(m, uint8_t(free + 1)) = D(tradeQuantity);
			setCurrentCharacter(m, D(memberVar));
			goto drop;
		}
	}
}

uint8_t useItemScreen(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	for (;;) {   // 19343
		m.setD16(g::buttonMask1, 0x0C34);
		m.setD16(g::buttonMask2, 0x10);
		const ListResult r = itemScreen(m, D(memberVar));
		if (r.cf) return r.al;
		useItem(m, r.al);
		redrawRosterRow(m, D(memberVar), D(0x471F));
	}
}

void characterInfo(Machine &m, uint8_t member) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	// 191E2: the skill screen; true when the whole character screen is left
	auto skills = [&]() -> bool {
		for (;;) {
			m.setD16(g::buttonMask1, 0x0C30);
			m.setD16(g::buttonMask2, 0x10);
			const ListResult s = skillScreen(m);
			if (!s.cf || s.al != 0xFF || s.empty) return true;
			const uint8_t r = useItemScreen(m);   // 191D4
			if (r != 0xFD) return false;
		}
	};
	D(memberVar) = member;
	redrawRosterRow(m, member, 0xFF);
	for (;;) {
		writeSummary(m);
		for (;;) {   // 19139: waiting for a key
			m.setD16(g::buttonMask1, 0x0C04);
			m.setD16(g::buttonMask2, 6);
			if (waitForKeypress(m)) goto done;
			const uint8_t key = D(g::lastKey);
			if (key == 13 || key == 32) {
				const uint8_t r = useItemScreen(m);
				if ((r == 0 || r == 0xFD) && skills()) goto done;
				break;
			}
			if (key == 'D') {   // divide the money
				getMemberMoney(m);
				uint8_t hi = D(uint16_t(g::numberValue + 2)), mid = D(uint16_t(g::numberValue + 1)), low = D(g::numberValue);
				D(0xBA86) = hi;
				divideMoney(m, hi, mid, low, D(uint16_t(g::saved2 + g::s2::membersInGroup)));
				D(g::numberValue) = low;
				D(uint16_t(g::numberValue + 1)) = mid;
				D(uint16_t(g::numberValue + 2)) = D(0xBA86);
				X86Regs x;
				x.al = 0;
				legacy::call(m, 0x98BC, x);   // resetMoney of the current character
				for (uint8_t n = 1; n <= D(uint16_t(g::saved2 + g::s2::membersInGroup)); ++n) {
					setCurrentCharacter(m, n);
					addMoneyToMember(m);
				}
				setCurrentCharacter(m, D(memberVar));
				D(g::numberValue) = D(0xBA80);
				D(uint16_t(g::numberValue + 1)) = D(0xBA81);
				D(uint16_t(g::numberValue + 2)) = D(0xBA82);
				addMoneyToMember(m);
				redrawRosterRow(m, D(memberVar), 0xFF);   // 19160: character_info again
				break;
			}
			if (key == 'P') {
				poolMoney(m, D(memberVar));
				redrawRosterRow(m, D(memberVar), 0xFF);
				break;
			}
		}
	}
done:
	redrawRosterRow(m, D(memberVar), 0);
}

} // namespace wl
