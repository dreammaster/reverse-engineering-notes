#include "wasteland1/combatmenu.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/interact.h"
#include "wasteland1/legacy.h"
#include "wasteland1/movement.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"
#include "wasteland1/party.h"
#include "wasteland1/roster.h"
#include "wasteland1/rules.h"
#include "wasteland1/text.h"

namespace wl {

namespace {
constexpr uint16_t commandTable = 0xA43B;   // word table of the command handlers
constexpr uint16_t memberVar = 0xA432, commandVar = 0xA433;
namespace g2 { constexpr uint16_t skillsCount = 0x469D; }   // number of target groups

uint16_t cur(Machine &m) { return m.d16(g::currentCharacter); }
uint8_t &field(Machine &m, uint8_t offset) { return m.d8(uint16_t(cur(m) + offset)); }
X86Regs call(Machine &m, uint16_t offset, uint8_t al = 0, uint8_t dl = 0, uint8_t bl = 0) {
	X86Regs x;
	x.al = al; x.dl = dl; x.bl = bl;
	legacy::call(m, offset, x);
	return x;
}
// 12734 / 12738: shifts left by 6 / 4
uint8_t shiftLeft6(uint8_t v) { return uint8_t(v << 6); }
// 172BE: true when the current character has no hit points left
bool currentDown(Machine &m) {
	const uint8_t high = field(m, 30);
	if (high & 0x80) return true;
	return (high | field(m, 29)) == 0;
}
uint8_t itemDetail(Machine &m, uint8_t offset) { return m.d8(uint16_t(m.d16(g::itemDetailsPtr) + offset)); }

// 123D1: asks for a direction and redraws the roster; CF / AL are those of the direction prompt
CommandResult directionPrompt(Machine &m) {
	call(m, 0x63C4);   // sub_163C4
	const int d = askDirection(m);
	drawRosterHeadersAndMembers(m);
	call(m, 0x90B8);   // sub_190B8: clears the bottom lines
	return {d < 0, uint8_t(d)};
}

// the loop of 12179: walks the monster groups until the `skip`th targetable one
uint8_t walkTargets(Machine &m, uint8_t kind, uint8_t skip) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(0xA430) = kind;
	D(0xA467) = skip;
	D(0x46F0) = 1;
	uint8_t bl = 1;
	for (;;) {
		D(0xA431) = bl;
		if (!targetRow(m, bl) && --D(0xA467) == 0) break;
		bl = uint8_t(D(0xA431) + 1);
		if (bl == 0) break;
	}
	D(0x46F0) = 0;
	--D(0xA431);
	return uint8_t(shiftLeft6(D(0xA430)) | D(0xA431));
}
}

CommandResult commandPause(Machine &m, uint8_t) {
	const uint8_t saved = m.d8(g::rosterLocOn);
	m.d8(g::rosterLocOn) = 0;
	call(m, 0x63C4);
	m.setD16(g::buttonMask1, 0x1000);
	m.setD16(g::buttonMask2, 0);
	waitForKeypress(m);
	drawRosterHeadersAndMembers(m);
	call(m, 0x90B8);
	m.d8(g::rosterLocOn) = saved;
	return {true, saved};
}

CommandResult commandUseItem(Machine &m, uint8_t member) {
	if (countItems(m) == 0) {
		printMessage(m, 64);
		return {true, member};
	}
	m.setD16(g::buttonMask1, 0x0434);
	m.setD16(g::buttonMask2, 0);
	const ListResult r = itemScreen(m, member);
	return {r.cf, r.al};
}

CommandResult commandMove(Machine &m, uint8_t) {
	if (m.d8(uint16_t(g::saved2 + g::s2::membersInGroup)) == 1) return directionPrompt(m);
	printMessage(m, 36);
	m.setD16(g::buttonMask1, 0x0404);
	m.setD16(g::buttonMask2, 0);
	waitForKeypress(m);
	const uint8_t key = m.d8(g::lastKey);
	if (key == 27) return {true, key};
	CommandResult r = directionPrompt(m);
	if (key != 'S') r.al |= 0x80;   // not the whole group
	return r;
}

CommandResult commandReload(Machine &m, uint8_t) {
	const uint8_t quantity = weaponQuantityByte(m);
	if (quantity & 0x80) return {false, quantity};
	getEquippedWeaponDetails(m);
	const uint8_t ammo = itemDetail(m, 7);
	uint8_t message = 66;
	if (ammo != 0) {
		const X86Regs found = call(m, 0x968C, ammo);   // findItemById
		if (!found.cf) return {false, found.al};
		message = 65;
	}
	writeCharacterVia(m, 4);
	printMessage(m, message);
	writeCharacterVia(m, 6);
	return {true, message};
}

CommandResult commandUseSkill(Machine &m, uint8_t member) {
	m.d8(0xA426) = member;
	uint8_t mode = 0, target = 0, what = 0;
	if (!chooseUseAction(m, member, mode, target, what)) {
		drawRosterHeadersAndMembers(m);
		return {true, member};
	}
	m.d8(0xA425) = target;
	const uint8_t code = uint8_t(shiftLeft4(mode) | target);
	const uint8_t id = m.d8(uint16_t(m.d16(g::partyIndexesPtr) + m.d8(0xA426)));
	m.d8(uint16_t(0xA9FD + id)) = what;
	drawRosterHeadersAndMembers(m);
	return {false, code};
}

CommandResult pickFirstTarget(Machine &m) {
	m.d8(0xA430) = 0;
	return {false, walkTargets(m, 0, 1)};
}

CommandResult commandAttack(Machine &m, uint8_t member) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(0xA42F) = member;
	const X86Regs weapon = call(m, 0x96C9);   // getMemberWeaponIdPtr: al = offset of the weapon's id
	if (field(m, uint8_t(weapon.al + 1)) & 0x80) {
		printMessage(m, 0x38);
		return {true, 0};
	}
	const uint8_t groups = countTargetableGroups(m, D(0xA42F));
	if (groups == 0) {
		bool zf = false;
		uint8_t message = 57;
		if (!hasNoAmmunition(m, D(0x46CD))) {
			useAmmunition(m, 0, &zf);
			if (zf) message = 80;
		}
		printMessage(m, message);
		return {true, 0};
	}
	D(g2::skillsCount) = groups;
	if (field(m, pm::npc) != 0) {   // non-player characters pick for themselves
		if (D(g::readiness) < 0x10) return pickFirstTarget(m);
		uint8_t kind = 0;
		if (isTypeGroupA(D(0x46CD)) || isTypeGroupB(D(0x46CD))) {
			bool zf;
			const uint8_t ammo = useAmmunition(m, 0, &zf);
			if (ammo >= 3) {
				kind = 1;
				if (ammo != 3) {
					D(0xA459) = 0;
					for (uint8_t id = 1; id <= D(0x4656); ++id) {
						setCurrentRosterCharacter(m, id);
						if (currentDown(m)) ++D(0xA459);
					}
					kind = Rules(m).random(10) < D(uint16_t(0xA48C + D(0xA459))) ? 1 : 2;
				}
			}
		}
		D(0xA450) = kind;
		return {false, walkTargets(m, kind, Rules(m).random(D(g2::skillsCount)))};
	}
	uint8_t target;
	if (D(g2::skillsCount) == D(0xA457)) {
		target = pickFirstTarget(m).al;
		if (target & 0x80) return {true, target};
	} else {
		showTargetList(m, D(g2::skillsCount));
		ListResult r;
		for (;;) {
			m.setD16(g::buttonMask1, 0x0434);
			m.setD16(g::buttonMask2, 0);
			r = pagedListKeys(m);
			if (r.cf && r.al == 0xFF) return {true, r.al};
			if (r.al < 0xFD) break;
		}
		markPickedRow(m);
		target = uint8_t(r.al - 1);
	}
	D(0xA42D) = target;
	if (combatReadiness(m, D(0xA42F)) < 0x10 || !(isTypeGroupA(D(0x46CD)) || isTypeGroupB(D(0x46CD)))) return {false, D(0xA42D)};
	bool zf;
	const uint8_t ammo = useAmmunition(m, 0, &zf);
	if (ammo < 3) return {false, D(0xA42D)};
	D(0xA42E) = ammo >= 4 ? 2 : 1;
	printMessage(m, 0x3A);
	if (D(0xA42E) == 2) printMessage(m, 0x3B);
	printMessage(m, 0x3C);
	m.setD16(g::textString, 0xA458);
	uint8_t pick;
	for (;;) {
		m.setD16(g::buttonMask1, 0x0404);
		m.setD16(g::buttonMask2, 0);
		pick = chooseByKey(m);
		if (pick & 0x80) return {true, pick};
		if (pick <= D(0xA42E)) break;
	}
	return {false, uint8_t(shiftLeft6(pick) | D(0xA42D))};
}

namespace {
CommandResult runCommand(Machine &m, uint8_t command, uint8_t member) {
	m.setD16(0x4661, m.d16(uint16_t(commandTable + command * 2)));
	switch (command) {
	case 0: return commandPause(m, member);
	case 1: { uint8_t group = 0; const bool ok = chooseTargetGroup(m, group); return {!ok, group}; }
	case 2: return commandAttack(m, member);
	case 3: return commandUseItem(m, member);
	case 4: return commandMove(m, member);
	case 5: return {false, member};
	case 6: return commandReload(m, member);
	case 7: return commandUseSkill(m, member);
	default: break;
	}
	X86Regs x;
	x.al = member;
	legacy::call(m, m.d16(0x4661), x);
	return {x.cf, x.al};
}
}

bool collectOrders(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint16_t members = uint16_t(g::saved2 + g::s2::membersInGroup);
	selectPartyBlock(m, D(0x4654));
	for (uint8_t bl = D(members); bl != 0; --bl) D(uint16_t(m.d16(g::partyStatusPtr) + bl)) = 0;
	if (countConsciousMembers(m) == 0) return false;
	bool monsters = false;
	for (int slot = 3; slot >= 0 && !monsters; --slot) {
		D(0xA434) = uint8_t(slot);
		selectSlot(m, D(0x4654), uint8_t(slot));
		monsters = m.d8(m.d16(g::monsterPtr)) != 0;
	}
	if (!monsters) {   // nothing to fight: confirm that the party wants to pass the round
		D(g::rosterLocOn) = 0;
		drawRosterWindow(m);
		printMessage(m, 20);
		printMessage(m, 76);
		const bool yes = askYesNo(m);
		D(g::rosterLocOn) = 1;
		if (!yes) return false;
	}
	call(m, 0x2636);   // sub_12636
	D(memberVar) = 1;
	for (;;) {
		if (!isMemberUnconscious(m, D(memberVar))) {
			bool again;
			do {   // 11FFC
				again = false;
				call(m, 0x7357);   // sub_17357
				redrawRosterRow(m, D(memberVar), 0xFF);
				drawInfoWindow(m);
				writeMemberName(m);
				printMessage(m, 55);
				D(0xA457) = 1;
				m.setD16(g::textString, 0xA44D);
				m.setD16(g::buttonMask1, 0x0404);
				m.setD16(g::buttonMask2, 8);
				uint8_t command = chooseByKey(m);
				if (command & 0x80) {   // 12085: back to the previous able member
					do {
						if (--D(memberVar) == 0) return true;
					} while (isMemberUnconscious(m, D(memberVar)));
					again = true;
					continue;
				}
				for (;;) {   // 12034
					D(commandVar) = command;
					D(uint16_t(m.d16(g::partyStatusPtr) + D(memberVar))) = command;
					if (command != 8) break;
					D(0xA457) = 0;   // the attack shortcut
					command = 2;
				}
				const CommandResult r = runCommand(m, command, D(memberVar));
				if (r.cf) { again = true; continue; }
				if (D(commandVar) == 4 && (r.al & 0x80)) {   // 11F93: everybody moves the same way
					D(memberVar) = r.al & 0x7F;
					for (uint8_t bl = D(members); bl != 0; --bl) {
						D(uint16_t(m.d16(g::partyOrdersPtr) + bl)) = D(memberVar);
						D(uint16_t(m.d16(g::partyStatusPtr) + bl)) = 8;
					}
					return false;
				}
				D(uint16_t(m.d16(g::partyOrdersPtr) + D(memberVar))) = r.al;
			} while (again);
		}
		if (D(memberVar) == D(members)) return false;
		++D(memberVar);
	}
}


void runCombat(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	constexpr uint16_t trackedMaps = 0xA9B0, visited = 0x720E, groupVar = 0xA435, acted = 0xA436, ordered = 0xA437, loopGroup = 0xA438;
	const uint16_t totalGroups = uint16_t(g::saved2 + g::s2::totalPartyGroups), field7 = uint16_t(g::saved2 + g::s2::field7);
	const uint16_t members = uint16_t(g::saved2 + g::s2::membersInGroup);
	const uint16_t sharedRoll = 0x46DC;   // the map number of the fight in progress

	D(0x4706) = D(uint16_t(g::saved2 + g::s2::activePartyGroup));
	D(trackedMaps) = D(field7);
	uint8_t group;
restart:   // 11D48
	for (int i = 3; i >= 0; --i) D(uint16_t(visited + i)) = 0;
	group = 0;
nextGroup:   // 11D56
	D(groupVar) = group;
	if (D(uint16_t(visited + group))) goto afterGroup;
	setActivePartyGroup(m, group);
	D(sharedRoll) = D(field7);
	{
		bool known = D(0x46DC) == D(0x46E0);
		for (int i = 3; i >= 0 && !known; --i) known = D(uint16_t(trackedMaps + i)) == D(field7);
		if (!known) {   // a new group wants into the fight
			if (countConsciousMembers(m) == 0) goto leaveFight;
			drawRosterHeadersAndMembers(m);
			D(g::rosterLocOn) = 0;
			drawRosterWindow(m);
			printMessage(m, 0x36);
			printMessage(m, 0x4C);
			if (!askYesNo(m)) goto leaveFight;
		}
	}
	call(m, 0x8350);   // sub_18350: loads the map of the fight
	scanForMonsters(m, false);
	placeTrackedMonsters(m);
	updateSlotDistances(m);
	planMonsterMoves(m);
	D(g::rosterLocOn) = 1;
orderRound:   // 11DBB
	D(loopGroup) = 0;
	D(ordered) = 0;
	do {   // 11DC3
		setActivePartyGroup(m, D(loopGroup));
		if (D(field7) == D(sharedRoll)) {
			drawRosterHeadersAndMembers(m);
			drawInfoWindow(m);
			if (collectOrders(m)) goto orderRound;
			call(m, 0x7357);   // sub_17357
			for (uint8_t bl = D(members); bl != 0; --bl)
				if (D(uint16_t(m.d16(g::partyStatusPtr) + bl)) != 0) { ++D(ordered); break; }
		}
		++D(loopGroup);
	} while (D(loopGroup) <= D(totalGroups));
	if ((D(ordered) | D(acted)) == 0) {   // nobody wants to do anything: the fight is over
		D(g::rosterLocOn) = 0;
		goto endOfFight;
	}
	printMessage(m, 0x46);
	if (!askYesNo(m)) goto orderRound;
	D(acted) = 1;
	setActivePartyGroup(m, D(groupVar));
	call(m, 0x2551);   // sub_12551: header of the round
	{
		const uint8_t keep = D(0x4655);
		D(0x4655) = D(0x46DC);
		call(m, 0x8350);
		D(0x4655) = keep;
	}
	setActivePartyGroup(m, D(groupVar));
	updateAllSlotSurvivors(m);
	selectWriterForRoster(m);
	D(g::textSpeedFlag) = 1;
	moveMonsters(m);
	newline(m);
	D(g::textSpeedFlag) = 0;
	{
		const bool scanCf = scanForMonsters(m, false);
		int i;
		if (!scanCf) {   // 11E4A: remember the map of this fight
			for (i = 3; i >= 0; --i) if (D(uint16_t(trackedMaps + i)) == D(0x46DC)) goto leaveFight;
			for (i = 3; i >= 0; --i) if (D(uint16_t(trackedMaps + i)) == 0xFF) break;
			D(uint16_t(trackedMaps + uint8_t(i))) = D(0x46DC);
		} else {   // 11E72: the fight on this map is over
			for (i = 3; i >= 0; --i) if (D(uint16_t(trackedMaps + i)) == D(0x46DC)) { D(uint16_t(trackedMaps + i)) = 0xFF; break; }
		}
	}
leaveFight:   // 11E8A
	for (int g3 = 3; g3 >= 0; --g3) {
		D(loopGroup) = uint8_t(g3);
		if (g3 <= D(totalGroups)) {
			setActivePartyGroup(m, uint8_t(g3));
			if (D(field7) == D(0x46DC)) D(uint16_t(visited + g3)) = 1;
		}
	}
	for (int i = 3; i >= 0; --i) if (!(D(uint16_t(trackedMaps + i)) & 0x80)) goto afterGroup;
endOfFight:   // 11EC6
	setActivePartyGroup(m, D(0x4706));
	D(g::textSpeedFlag) = 1;
	D(loopGroup) = 1;
	D(groupVar) = 1;
	for (;;) {   // 11ED7: every character's share of the experience
		setCurrentRosterCharacter(m, D(groupVar));
		getMemberExperience(m);
		const uint16_t at = D(loopGroup);
		uint8_t borrow = 0, any = 0;
		for (int k = 0; k < 3; ++k) {
			const int diff = int(D(uint16_t(g::numberValue + k))) - int(D(uint16_t(0x71B7 + at + k))) - borrow;
			D(uint16_t(g::numberValue + k)) = uint8_t(diff);
			borrow = diff < 0 ? 1 : 0;
			any |= uint8_t(diff);
		}
		D(loopGroup) = uint8_t(at + 3);
		if (any != 0) {
			printMessage(m, 0x27);
			writeNumber(m);
			printMessage(m, 0x28);
		}
		const uint8_t next = uint8_t(D(groupVar) + 1);
		if (next > D(uint16_t(g::saved + 0x86))) break;
		D(groupVar) = next;
	}
	D(g::textSpeedFlag) = 0;
	writeCharacter(m, 6);
	D(g::rosterLocOn) = 0;
	D(0x4724) = 0;
	call(m, 0x8350);
	call(m, 0x63C4);
	return;

afterGroup:   // 11F47
	group = uint8_t(D(groupVar) + 1);
	if (group <= D(totalGroups)) goto nextGroup;
	advanceTime(m);
	healthTick(m);
	D(g::textSpeedFlag) = 1;
	for (int i = 0; i < 6; ++i) newline(m);
	D(g::textSpeedFlag) = 0;
	goto restart;
}

} // namespace wl
