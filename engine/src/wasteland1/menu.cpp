#include "wasteland1/menu.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/mapcells.h"
#include "wasteland1/movement.h"
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


namespace {
// 12B8A: lists the members that are not placed yet (ds:46CF) on the roster window lines
void listUnplaced(Machine &m) {
    m.d8(0x4691) = 2;
    for (uint8_t bl = 1; bl < 8; bl = uint8_t(m.d8(0x469F) + 1)) {
        m.d8(0x469F) = bl;
        const uint8_t id = m.d8(uint16_t(0x46CF + bl));
        if (id == 0) continue;
        --m.d8(0x469F);
        m.d8(0x4721) = 0xFF;
        m.d8(0x469C) = 0;
        call(m, 0x6EE8);   // sub_16EE8: row marker
        ++m.d8(0x469F);
        setCurrentRosterCharacter(m, id);
        writeMemberName(m);
    }
}
}

void rearrangeParty(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    const uint16_t members = uint16_t(g::saved2 + g::s2::membersInGroup);
    if (D(members) == 1) return;
    uint16_t indexes = m.d16(g::partyIndexesPtr);
    for (int bl = 7; bl >= 0; --bl) {   // take everybody out of the group and show the loose list
        D(uint16_t(0x46CF + bl)) = D(uint16_t(indexes + bl));
        D(uint16_t(indexes + bl)) = 0;
    }
    const uint8_t count = D(members);
    D(0xA4C0) = 0;
    D(members) = 0;
    call(m, 0x728C);   // writeRosterHeadersAndMembers
    D(members) = count;
    for (;;) {
        printMessage(m, 15);
        listUnplaced(m);
        for (;;) {   // 12B18: who goes next
            const uint8_t pick = pickGroupMember(m, true);
            if (pick == 0) {
                if (D(0xA4C0) != 0) continue;   // ESC after the first placement is ignored
                indexes = m.d16(g::partyIndexesPtr);
                for (int bl = 7; bl >= 0; --bl) D(uint16_t(indexes + bl)) = D(uint16_t(0x46CF + bl));
                call(m, 0x63C4);   // sub_163C4
                return;
            }
            const uint8_t id = D(uint16_t(0x46CF + pick));
            if (id == 0) continue;
            D(uint16_t(0x46CF + pick)) = 0;
            ++D(0xA4C0);
            indexes = m.d16(g::partyIndexesPtr);
            D(uint16_t(indexes + D(0xA4C0))) = id;
            call(m, 0x7033, D(0xA4C0));   // writePartyMemberCustomHilite
            break;
        }
        if (uint8_t(D(0xA4C0) + 1) != D(members)) continue;
        uint8_t bl = 1, rest = 0;   // the one member left goes last
        do { rest = D(uint16_t(0x46CF + bl)); if (rest) break; ++bl; } while (bl != 0);
        indexes = m.d16(g::partyIndexesPtr);
        D(uint16_t(indexes + D(members))) = rest;
        call(m, 0x63C4);
        call(m, 0x728C);
        return;
    }
}


bool targetRow(Machine &m, uint8_t index) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    const CombatantId id = splitCombatantIndex(uint8_t(index - 1));
    D(0xA42C) = id.group;
    D(0xA42A) = id.member;
    selectSlot(m, id.group, id.slot);
    uint8_t distance = 0;
    if (!slotTargetable(m, distance)) return true;
    D(0xA42B) = distance;
    const uint8_t offset = D(uint16_t(g::encounterCountOffsets + D(0xA42A)));
    if (D(uint16_t(m.d16(g::savedLocationDetailsPtr) + offset)) == 0) return true;
    D(g::inverseText) = 0;
    if (D(0x4654) == D(0xA42C)) D(g::inverseText) = 0xFF;
    call(m, 0x6EE8);   // sub_16EE8: row marker
    D(g::inverseText) = 0;
    writeEncounterGroup(m, D(0xA42A));
    D(0x4672) = uint8_t(D(g::rectRightCol) - 3);
    writeCharacterVia(m, 32);
    D(0x467F) = 2;
    writeNumberShort(m, D(0xA42B));
    writeCharacter2(m, 39);
    return false;
}

void showTargetList(Machine &m, uint8_t groups) {
    m.d8(0x469D) = uint8_t(groups + 1);
    printMessage(m, 63);
    m.d8(0x469C) = 65;
    m.d8(0x4721) = 0xFF;
    m.d8(0x4699) = 1;
    m.setD16(0x4696, 0x2284);
    pagedListDraw(m);
}

bool chooseTargetGroup(Machine &m, uint8_t &group) {
    m.d8(g::readiness) = 0xFF;
    const uint8_t groups = countGroupsInRange(m);
    if (groups == 0) {
        printMessage(m, 57);
        group = 0;
        return false;
    }
    showTargetList(m, groups);
    for (;;) {
        m.setD16(g::buttonMask1, 0x0434);
        m.setD16(g::buttonMask2, 0);
        const ListResult r = pagedListKeys(m);
        if (r.cf && r.al == 0xFF) { group = 0xFF; return false; }
        if (r.al >= 0xFD) continue;
        group = uint8_t(r.al - 1);
        markPickedRow(m);
        return true;
    }
}


namespace {
// 18DBE: a busy wait of al * 256 steps
void delayLoop(Machine &, uint8_t) {}
// 1783E / 17843: shows a cursor / blank at the cursor and steps back
void cursorGlyph(Machine &m, uint8_t glyph) {
    m.d8(g::inverseText) = 0;
    writeCharacter2(m, glyph);
    --m.d8(g::rectTextX);
}
}

uint8_t textInput(Machine &m, bool numericOnly, bool keepCase) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    constexpr uint16_t position = 0x4682, maxLength = 0x4684, numericFlag = 0x467B;
    D(numericFlag) = numericOnly ? 1 : 0;
    const uint16_t text = m.d16(g::textString);
    D(g::inverseText) = 0;
    D(position) = 0;
    for (uint8_t bl = D(maxLength); bl != 0; --bl) D(uint16_t(text + bl)) = 0;
    D(text) = 0;
    cursorGlyph(m, '_');
    for (;;) {
        for (;;) {   // 17774: blink the cursor until a key comes
            delayLoop(m, 0x30);
            cursorGlyph(m, ' ');
            delayLoop(m, 0x30);
            cursorGlyph(m, '_');
            m.setD16(g::buttonMask1, 0x0400);
            m.setD16(g::buttonMask2, 0);
            const uint8_t key = keepCase ? checkKeypress(m, 0) : checkKeypress(m);
            if (key == 0x1B) return 0xFF;
            if (key == 8 || key == 0xFF) {   // backspace
                if (D(position) == 0) continue;
                cursorGlyph(m, ' ');
                --D(g::rectTextX);
                --D(position);
                D(uint16_t(text + D(position))) = 0;
                break;
            }
            if (key == 0x0D) {   // 17809: drop the trailing blanks
                uint8_t bl = uint8_t(D(position) - 1);
                for (;;) {
                    if (D(uint16_t(text + bl)) != ' ') break;
                    D(uint16_t(text + bl)) = 0;
                    --D(position);
                    if (D(position) == 0) {
                        bl = uint8_t(bl - 1);
                        if (D(uint16_t(text + bl)) == ' ') D(uint16_t(text + bl)) = 0;
                        break;
                    }
                    bl = D(position) ? uint8_t(D(position) - 1) : bl;
                }
                cursorGlyph(m, ' ');
                return 0;
            }
            if (key < 0x20) continue;
            if (D(numericFlag) && (key < '0' || key >= ':')) continue;
            const uint8_t c = key & 0x7F;
            if (D(position) == D(maxLength)) continue;
            D(uint16_t(text + D(position))) = c;
            const X86Regs at = call(m, 0xA26C);   // saveFields1: dl, bl = cursor
            call(m, 0x9DC0, c, at.dl, at.bl);   // restoreFields1 + writeCharacter2
            if (++D(position) != 0) break;
        }
        cursorGlyph(m, '_');   // 17771
    }
}


bool askKeyword(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    auto rec = [&](uint8_t i) -> uint8_t & { return D(uint16_t(m.d16(g::locationDetailsPtr) + i)); };
    constexpr uint16_t answers = 0xA650, tried = 0xA651, listed = 0xA653, typed = 0x7931, wordBuffer = 0x79B1;
    uint8_t count = 0;
    for (uint8_t bl = 3;; ++bl) {   // how many keyword entries the record lists
        ++count;
        if ((rec(bl) & 0x80) || bl == 0xFF) break;
    }
    D(listed) = count;
    drawInfoWindow(m);
    const uint8_t first = rec(0);
    printMapMessage(m, first & 0x7F);
    uint8_t index = 1;   // 15242: record offset 1 when the question is dropped
    bool answered = false;
    if (first & 0x80) {   // answered with a single key
        m.setD16(g::buttonMask1, 0x0404);
        m.setD16(g::buttonMask2, 0);
        waitForKeypress(m);
        const uint8_t key = D(g::lastKey);
        if (key != 0x0D && key != 0x1B) {
            D(typed) = key & 0x7F;
            D(typed + 1) = 0;
            answered = true;
        }
    } else {
        scrollLine(m);
        writeCharacter2(m, '>');
        m.setD16(g::textString, typed);
        D(0x4684) = 0x10;
        answered = textInput(m, false) == 0 && D(typed) != 0;
    }
    if (answered) {   // 151D7: which keyword is it
        D(tried) = 0;
        for (uint8_t bl = 3;;) {
            D(answers) = bl;
            call(m, 0x6D14, rec(bl) & 0x7F);   // sub_16D14: starts decoding keyword text
            for (uint8_t at = 0;; ++at) {
                const uint8_t c = textDecodeChar(m);
                D(uint16_t(wordBuffer + at)) = c;
                if (c == 0 || at == 0xFF) break;
            }
            bool same = true;   // 18D8E
            for (uint8_t at = 0;; ++at) {
                const uint8_t a = D(uint16_t(wordBuffer + at)), b = D(uint16_t(typed + at));
                if ((a | b) == 0) break;
                if (a != b) { same = false; break; }
                if (at == 0xFF) { same = false; break; }
            }
            m.setD16(g::textString, wordBuffer);
            m.setD16(g::itemDetailsPtr, typed);
            if (same) break;
            ++D(tried);
            if (rec(D(answers)) & 0x80) break;
            bl = uint8_t(D(answers) + 1);
            if (bl == 0) break;
        }
        D(answers) = uint8_t(3 + D(listed));
        index = uint8_t(D(tried) * 2 + D(answers));
    }
    const bool cf = changeCellFromLocation(m, index, D(0x46A6), D(0x46A7));
    call(m, 0x63C4);   // sub_163C4
    return cf;
}


void splitParty(Machine &m) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    constexpr uint16_t ableCount = 0xA6C8, chosen = 0xA6C6, moved = 0xA6C7, newGroup = 0xA6C5;
    const uint16_t members = uint16_t(g::saved2 + g::s2::membersInGroup), groups = uint16_t(g::saved2 + g::s2::totalPartyGroups);
    const uint16_t fieldEight = uint16_t(g::saved2 + g::s2::field8);
    auto cur = [&]() { return m.d16(g::currentCharacter); };
    D(ableCount) = 0;
    for (uint8_t id = 1; id <= D(fieldEight); ++id) {   // how many characters are on their feet
        D(chosen) = id;
        setCurrentRosterCharacter(m, id);
        const uint8_t high = D(uint16_t(cur() + pm::con + 1));
        const bool down = (high & 0x80) || (high | D(uint16_t(cur() + pm::con))) == 0;
        if (!down) ++D(ableCount);
    }
    if (D(groups) == 3) {
        printMessage(m, 0x15);
        call(m, 0x63C4);
        return;
    }
    call(m, 0x728C);
    D(moved) = 0;
restart:   // 15EBC
    D(newGroup) = uint8_t(D(groups) + 1);
    setPartyIndexesPtr(m, D(newGroup));
    printMessage(m, 0x16);   // 15EC7
keyLoop:   // 15ECC
    m.setD16(g::buttonMask1, 0x0401);
    m.setD16(g::buttonMask2, 0);
    {
        waitForKeypress(m);
        const uint8_t key = D(g::lastKey);
        if (key == 0x1B) goto cancel;
        if (key == 0x0D) goto finish;
        if (key < '1' || key > '9') goto keyLoop;
        const uint8_t member = key & 0x0F;
        if (member > D(members)) goto keyLoop;
        D(chosen) = member;
        setCurrentCharacter(m, member);
        if (D(uint16_t(cur() + pm::npc)) != 0 && D(ableCount) >= 2) {   // a monster friend: leave the party for good?
            printMessage(m, 0x4F);
            printMessage(m, 0x4D);
            for (;;) {
                m.setD16(g::buttonMask1, 0x0404);
                m.setD16(g::buttonMask2, 0);
                waitForKeypress(m);
                const uint8_t answer = D(g::lastKey);
                if (answer == 'N') goto checkMember;
                if (answer == 0x1B) goto cancel;
                if (answer == 'Y') {
                    deleteCharacter(m, D(chosen));
                    goto cancel;
                }
            }
        }
    checkMember:   // 15F46
        {
            const uint8_t high = D(uint16_t(cur() + pm::con + 1));
            if (high & 0x80) {   // down
                writeMemberName(m);
                printMessage(m, 0x18);
                goto keyLoop;
            }
            if ((high | D(uint16_t(cur() + pm::con))) == 0) {   // dead
                printMessage(m, 0x4F);
                printMessage(m, 0x4E);
                for (;;) {
                    m.setD16(g::buttonMask1, 0x0404);
                    m.setD16(g::buttonMask2, 0);
                    waitForKeypress(m);
                    const uint8_t answer = D(g::lastKey);
                    if (answer == 'N') goto restart;
                    if (answer == 0x1B) goto cancel;
                    if (answer == 'Y') {
                        deleteCharacter(m, D(chosen));
                        goto cancel;
                    }
                }
            }
        }
        writeMemberName(m);
        uint8_t id;
        if (moveToGroup(m, D(chosen), D(uint16_t(g::saved2 + g::s2::activePartyGroup)), D(newGroup), id)) {
            ++D(moved);
            newline(m);
            if (uint8_t(D(moved) + 1) != D(members)) goto keyLoop;
            goto finish;
        }
        printMessage(m, 0x17);
        goto keyLoop;
    }
cancel:   // 15F7D
    clearPartyGroup(m, D(newGroup));
    call(m, 0x63C4);
    return;
finish:   // 15F99
    call(m, 0x63C4);
    if (D(moved) == 0) return;
    {
        const int dir = askDirection(m);
        if (dir < 0) goto cancel;
        ++D(groups);
        for (uint8_t bl = 1; bl != 8; ++bl) {
            D(chosen) = bl;
            setPartyIndexesPtr(m, D(newGroup));
            const uint8_t id = D(uint16_t(m.d16(g::partyIndexesPtr) + bl));
            if (id == 0) break;
            removeFromActiveGroup(m, id);
        }
        call(m, 0x60E1, D(newGroup));   // sub_160E1: the new group's position
        tryStep(m, uint8_t(dir));
        call(m, 0x63A0);   // sub_163A0
    }
}


bool askYesLoose(Machine &m) {
    m.setD16(g::buttonMask1, 0x0404);
    m.setD16(g::buttonMask2, 0);
    if (waitForKeypress(m)) return true;
    return m.d8(g::lastKey) == 'N';
}

bool switchToGroup(Machine &m, uint8_t group) {
    auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
    const uint16_t active = uint16_t(g::saved2 + g::s2::activePartyGroup), field7 = uint16_t(g::saved2 + g::s2::field7);
    const uint8_t oldMap = D(field7);
    const uint8_t oldGroup = D(active);
    D(0xA6C1) = oldGroup;
    setActivePartyGroup(m, group);
    if (countConsciousMembers(m) == 0) {   // nobody on their feet in that group
        if (D(uint16_t(g::saved2 + g::s2::totalPartyGroups)) != 0) {
            for (uint8_t id = 1; id <= D(uint16_t(g::saved2 + g::s2::field8)); ++id) {
                D(0xA6C0) = id;
                setCurrentRosterCharacter(m, id);
                const uint16_t c = m.d16(g::currentCharacter);
                const uint8_t high = D(uint16_t(c + pm::con + 1));
                const bool down = (high & 0x80) || (high | D(uint16_t(c + pm::con))) == 0;
                if (!down) {
                    call(m, 0x728C);
                    printMessage(m, 0x0E);
                    break;
                }
            }
        }
        setActivePartyGroup(m, D(0xA6C1));
        return true;
    }
    if (oldMap != D(field7)) {   // the group stands on another map: really go there?
        call(m, 0x728C);
        printMessage(m, 0x1B);
        if (askYesLoose(m)) {
            setActivePartyGroup(m, D(0xA6C1));
            return true;
        }
        call(m, 0x8350);   // sub_18350: loads the other map
    }
    call(m, 0x63C4);
    return false;
}

} // namespace wl
