#include "wasteland1/mainloop.h"
#include "wasteland1/character.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/party.h"
#include "wasteland1/recomp_rt.h"
#include "wasteland1/text.h"
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

void explorationLoop(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	if (std::getenv("MAINLOOPDBG")) std::fprintf(stderr, "exploration loop entered\n");
	call(m, 0x63C4);
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
				call(m, 0x63CA);
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
			call(m, 0x7A35);
		} else if (key == 0xD1) {
			call(m, 0x7A4D);
		} else if (key >= '1' && key < '8') {
			call(m, 0x2760, key);
		} else {
			uint8_t member = D(g::saved2 + g::s2::membersInGroup);
			const uint8_t pressed = D(g::lastKey);
			for (; member != 0; --member)
				if (pressed == D(uint16_t(arrayKeys + member))) break;
			if (member != 0) call(m, 0x3AAF, member);   // a party member's hot key
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
