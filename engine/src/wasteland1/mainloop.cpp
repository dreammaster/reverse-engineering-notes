#include "wasteland1/mainloop.h"
#include "wasteland1/character.h"
#include "wasteland1/buttons.h"
#include "wasteland1/charscreen.h"
#include "wasteland1/interact.h"
#include "wasteland1/menu.h"
#include "wasteland1/roster.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/recomp_rt.h"
#include "wasteland1/text.h"
#include "wasteland1/ui.h"
#include <cstdio>
#include <cstdlib>

namespace wl {

namespace {
constexpr uint16_t quickTimer = 0xAAE5, lastTimer = 0xAAE1, lastEncounterTimer = 0xAAE3, encounterCounter = 0x4704, timerWord = 0xD150,
                   viewActive = 0x46E1, idleLimit = 0x46EE, inCombatHint = 0x46B9, memberCounter = 0xAAE0, arrayKeys = 0xA9C4;

X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0) {
	X86Regs x;
	x.al = al;
	legacy::call(m, offset, x);
	return x;
}

bool currentDown(Machine &m) {   // 172BE: the current character is dead or unconscious
	const uint16_t c = m.d16(g::currentCharacter);
	const uint8_t high = m.d8(uint16_t(c + 30));
	if (high & 0x80) return true;
	return (high | m.d8(uint16_t(c + 29))) == 0;
}
}

void showMessageLines(Machine &m, uint8_t last) {   // 179B7: redraws the message window from the scroll-back buffer (ends with line `last`)
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	if (D(0x46FE) != 0) moreButton(m);
	const uint8_t savedX = D(g::rectTextX), savedY = D(g::rectTextY);
	D(0x8DF7) = last;
	uint16_t bx = uint16_t(uint8_t(last - 5) * 40 + 0x8CE0);
	D(g::rectTextY) = 18;
	do {
		for (uint8_t x = 1; x != 39; ++x) {
			D(g::rectTextX) = x;
			writeCharacterAt(m, m.mem[0x2AE20 + uint16_t(bx + x)]);
		}
		bx = uint16_t(bx + 0x28);
		if (bx >= 0xB4DF) bx = uint16_t(bx - 0x2800);
		++D(g::rectTextY);
	} while (D(g::rectTextY) != 24);
	D(g::rectTextX) = savedX;
	D(g::rectTextY) = savedY;
	D(0x46FE) = 0;
}

void scrollMessagesUp(Machine &m) {   // 17A35: PgUp
	const uint8_t shown = m.d8(0x8DF7), oldest = m.d8(0x8DF6);
	uint8_t line = uint8_t(shown - 1);
	if (uint8_t(line - oldest) < 6) line = uint8_t(oldest + 6);
	showMessageLines(m, line);
}

void scrollMessagesDown(Machine &m) {   // 17A4D: PgDn
	const uint8_t shown = m.d8(0x8DF7), oldest = m.d8(0x8DF6);
	uint8_t line = uint8_t(shown + 1);
	if (uint8_t(line - oldest) <= 6) line = oldest;
	showMessageLines(m, line);
}

void refreshMapAndStatus(Machine &m) {   // 163CA: redraws the map view, the monsters and the party markers
	const uint16_t keepDetails = m.d16(g::locationDetailsPtr);
	call(m, 0x7DBB);
	call(m, 0x6675);
	call(m, 0x67CE);
	const uint8_t keepFlag = m.d8(0x46DC);
	m.d8(0x46DC) = m.d8(0x4655);
	call(m, 0x4664);   // scanForMonsters
	m.d8(0x46DC) = keepFlag;
	call(m, 0x6428);
	call(m, 0x6716);
	call(m, 0x7DF1);
	m.setD16(g::locationDetailsPtr, keepDetails);
}

void refreshMessagesAndMap(Machine &m) {   // 163C4: redraws the message window, then the map
	showMessageLines(m, m.d8(0x8DF7));
	refreshMapAndStatus(m);
}

void refreshAfterMove(Machine &m) {   // 163A0
	call(m, 0x7DBB);
	call(m, 0x67CE);
	call(m, 0x6428);
	call(m, 0x6716);
	call(m, 0x7DF1);
	call(m, 0x5DAF);   // mergeGroupsAtSamePosition
	if (countConsciousMembers(m) == 0) return;
	call(m, 0x63F6);
	call(m, 0x69BC);
	call(m, 0x6716);
	call(m, 0x63C1);
}

void characterKey(Machine &m, uint8_t key) {   // 12760: digit keys open the character screen of that party member
	(void)key;
	const uint8_t member = m.d8(g::lastKey) & 0x0F;   // the original reads the last key, not al
	if (member > m.d8(uint16_t(g::saved2 + g::s2::membersInGroup))) return;
	characterInfo(m, member);
	refreshMessagesAndMap(m);
	refreshAfterMove(m);
}

void useMember(Machine &m, uint8_t member) {   // 13AAF: asks what member `member` should use and on what
	for (;;) {
		m.d8(0xA5E5) = member;
		if (isMemberUnconscious(m, member)) {
			selectWriterForRoster(m);   // 19720
			printMessage(m, 3);
			refreshMessagesAndMap(m);
			return;
		}
		uint8_t mode = 0, target = 0, what = 0;
		if (chooseUseAction(m, member, mode, target, what)) {
			m.d8(0x4661) = member;
			uint8_t result = 0;
			if (useSkillOnObject(m, mode, target, what, result)) {
				if (result == 0) refreshMessagesAndMap(m);
				else refreshMapAndStatus(m);
			} else {
				refreshAfterMove(m);
			}
			return;
		}
		// 13A9E: cancelled
		if (m.d8(uint16_t(g::saved2 + g::s2::membersInGroup)) == 1) {
			refreshMessagesAndMap(m);
			return;
		}
		drawRosterFrame(m);   // 16F70
		printMessage(m, 2);
		member = pickGroupMember(m, false);
		if (member == 0) {
			refreshMessagesAndMap(m);
			return;
		}
	}
}

void useButton(Machine &m) {   // 13A80: the USE button
	call(m, 0x9727);
	drawRosterHeadersAndMembers(m);
	uint8_t member = m.d8(uint16_t(g::saved2 + g::s2::membersInGroup));
	if (member != 1) {
		printMessage(m, 2);
		member = pickGroupMember(m, false);
		if (member == 0) {
			refreshMessagesAndMap(m);
			return;
		}
	}
	useMember(m, member);
}

void explorationLoop(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	if (std::getenv("MAINLOOPDBG")) std::fprintf(stderr, "exploration loop entered\n");
	refreshMessagesAndMap(m);
	if (D(viewActive) != 0) call(m, 0x656D);
	for (;;) {   // 16B49
		if (rc::Regs *r = m.sys ? m.sys->current : nullptr) rc::backedge(m, *r, 0x6C6C);   // the timer interrupt is driven by the loops
		if (D(idleLimit) != 0xFF) {   // the party stands still: count down to the idle event
			if (D(quickTimer) == 0) D(quickTimer) = uint8_t(call(m, 0x8E41, uint8_t(D(idleLimit) + 1)).al + 3);
			const uint16_t now = m.d16(timerWord);
			if (now != m.d16(lastTimer)) {
				m.setD16(lastTimer, now);
				if (--D(quickTimer) == 0) {
					X86Regs x;
					x.al = 2;
					legacy::call(m, 0xCBD3, x);
				}
			}
		}
		const uint16_t now = m.d16(timerWord);
		if (now != m.d16(lastEncounterTimer)) {   // every 1024 ticks a random encounter may happen
			m.setD16(lastEncounterTimer, now);
			m.setD16(encounterCounter, uint16_t(m.d16(encounterCounter) + 1));
			if ((m.d16(encounterCounter) & 0x3FF) == 0) {
				call(m, 0x651A, 4);
				call(m, 0x6890);
				refreshMapAndStatus(m);
				call(m, 0x63C1);
			}
		}
		m.setD16(g::buttonMask1, D(inCombatHint) != 0 ? 0x200B : 0x40CA);
		m.setD16(g::buttonMask2, 0);
		const uint8_t key = checkKeypress(m);
		if (key != 0) {
			m.setD16(g::buttonMask1, 2);
			m.setD16(g::buttonMask2, 0);
			D(quickTimer) = key;
		}
		if (key == 'P' && D(inCombatHint) != 0) {
			call(m, 0xB170);   // print
		} else if (key == 0xC9) {
			scrollMessagesUp(m);
		} else if (key == 0xD1) {
			scrollMessagesDown(m);
		} else if (key >= '1' && key < '8') {
			characterKey(m, key);
		} else {
			uint8_t member = D(g::saved2 + g::s2::membersInGroup);
			const uint8_t pressed = D(g::lastKey);
			for (; member != 0; --member)
				if (pressed == D(uint16_t(arrayKeys + member))) break;
			if (member != 0) useMember(m, member);   // a party member's hot key
			else call(m, 0x7574);                         // the buttons row
		}
		call(m, 0xCB30);
		const uint8_t total = D(uint16_t(g::saved2 + g::s2::field8));
		bool alive = false;
		if (total != 0) {
			for (uint8_t n = 1;; ++n) {   // somebody must still be able to act
				D(memberCounter) = n;
				setCurrentRosterCharacter(m, n);
				if (!currentDown(m)) { alive = true; break; }
				const uint8_t condition = getMemberConditionIndex(m);
				if (condition == 0 && D(uint16_t(m.d16(g::currentCharacter) + 40)) == 0) { alive = true; break; }
				if (uint8_t(n + 1) > total) break;
			}
		}
		if (!alive) {
			call(m, 0xC570);   // game over
			return;
		}
		if (countConsciousMembers(m) == 0) call(m, 0x60A8);   // 160A8: the active group is out: another group takes over
	}
}

} // namespace wl
