#include "wasteland1/shop.h"
#include "wasteland1/character.h"
#include "wasteland1/combat.h"
#include "wasteland1/globals.h"
#include "wasteland1/legacy.h"
#include "wasteland1/listmenu.h"
#include "wasteland1/menu.h"
#include "wasteland1/party.h"
#include "wasteland1/roster.h"
#include "wasteland1/text.h"
#include "wasteland1/ui.h"

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
bool canAffordNumber(Machine &m) {   // 17B3E
	const uint32_t price = uint32_t(m.d8(g::numberValue)) | (uint32_t(m.d8(uint16_t(g::numberValue + 1))) << 8) |
	                       (uint32_t(m.d8(uint16_t(g::numberValue + 2))) << 16);
	const uint32_t money = uint32_t(field(m, pm::money)) | (uint32_t(field(m, pm::money + 1)) << 8) | (uint32_t(field(m, pm::money + 2)) << 16);
	return money >= price;
}
void showCursorHome(Machine &m) {   // 1C222
	m.d8(g::rectTextX) = 0x0F;
	m.d8(g::rectTextY) = 1;
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


bool raiseMaxCon(Machine &m, uint8_t amount) {
	uint8_t &low = field(m, pm::maxCon);
	uint8_t &high = field(m, uint8_t(pm::maxCon + 1));
	const unsigned sum = unsigned(low) + amount;
	low = uint8_t(sum);
	high = uint8_t(high + (sum >> 8));
	return (high & 0x80) != 0;
}

void trainingMessage(Machine &m, uint8_t id) {
	setWriteCharacterFn(m);
	m.setD16(g::textPointer1, 0xD622);
	writeEncodedText(m, id);
}

void setRankTitle(Machine &m, uint8_t index) {
	m.setD16(g::textPointer1, 0xD622);
	textDecodeInit(m, index);
	for (uint8_t at = pm::rank;; ++at) {
		m.d8(0xD430) = at;
		const uint8_t c = textDecodeChar(m);
		field(m, at) = c;
		if (c == 0 || at == 0xFF) break;
	}
}

void distributePoints(Machine &m, uint8_t member) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(0xD432) = member;
	raiseMaxCon(m, D(g::itemQuantity));
	for (;;) {
		m.setD16(g::textString, 0xD453);   // "<name> has <n> points to distribute..."
		for (uint8_t at = pm::strength; at != 0x15; ++at) {
			D(0xD431) = at;
			const uint8_t value = field(m, at);
			writeString(m);
			writeNumberLeft(m, value);
		}
		writeString(m);
		writeNumber16(m, uint16_t(field(m, pm::maxCon) | (field(m, uint8_t(pm::maxCon + 1)) << 8)));
		writeString(m);
		for (;;) {   // 1BAB7
			m.setD16(g::textString, 0xD44A);   // "12345678"
			m.setD16(g::buttonMask1, 0x0004);
			m.setD16(g::buttonMask2, 0);
			const uint8_t pick = chooseByKey(m);
			if (pick == 0xFF) continue;
			if (pick == 7) {   // one more hit point
				if (raiseMaxCon(m, 1)) {
					field(m, uint8_t(pm::maxCon + 1)) = 0x7F;   // full: stays at 7FFFh
					field(m, pm::maxCon) = 0xFF;
					continue;
				}
				const uint8_t at = findInSelectedGroup(m, D(0xD432));
				if (at != 0) {
					const uint8_t keep = D(g::itemQuantity);
					redrawRosterRow(m, at, D(0x471F));
					D(g::itemQuantity) = keep;
				}
				break;
			}
			const uint8_t offset = uint8_t(pick + pm::strength);
			if (field(m, offset) == 0xFF) continue;
			++field(m, offset);
			if (offset == pm::iq && field(m, pm::skillPoints) != 0xFF) ++field(m, pm::skillPoints);
			break;
		}
		if (--D(g::itemQuantity) == 0) {
			clearRect(m);
			return;
		}
	}
}


namespace {
constexpr uint16_t schoolMember = 0xDAA5, skillBaseVar = 0xDAA2, skillLevelVar = 0xDAA1, schoolSlot = 0xDAA6, buildIndex = 0xDAA4;

void schoolMessage(Machine &m, uint8_t id, bool immediate) {   // 1BE31 / 1BE2A
	if (immediate) setWriteCharacterFn2(m);
	else setWriteCharacterFn(m);
	m.setD16(g::textPointer1, 0xDACC);
	writeEncodedText(m, id);
}
void cursorHome(Machine &m) {   // 1BE40
	m.d8(g::rectTextX) = 0x0F;
	m.d8(g::rectTextY) = 1;
}
void skillRecord(Machine &m, uint8_t id) { m.setD16(g::itemDetailsPtr, uint16_t(0xDAA8 + id)); }   // 1BE16
uint8_t skillRecordByte(Machine &m) { return m.d8(m.d16(g::itemDetailsPtr)); }
uint8_t skillIq(Machine &m) { return uint8_t(skillRecordByte(m) >> 3); }                          // 1CA98
uint8_t skillBase(Machine &m) { return uint8_t(skillRecordByte(m) & 7); }                         // 1CA8D
uint8_t skillIdAt(Machine &m, uint8_t index) { return rec(m, uint8_t(0x10 + index)); }              // 1BE1F
}

uint8_t skillCost(uint8_t base, uint8_t level) {
	uint8_t power = 1, dl = level;
	for (;;) {
		if (--dl == 0) break;
		const unsigned shifted = unsigned(power) << 1;
		if (shifted & 0x100) return 0xFF;
		power = uint8_t(shifted);
		if (power == 0) break;
	}
	uint8_t result = power;
	for (uint8_t left = base;;) {
		if (left == 1) return result;
		const unsigned sum = unsigned(result) + power;
		if (sum & 0x100) return 0xFF;
		result = uint8_t(sum);
		if (--left == 0) return 0xFF;
	}
}

bool findSkillSlot(Machine &m, uint8_t id, uint8_t &offset) {
	for (uint8_t bl = pm::skills; bl != pm::skillsEnd; bl = uint8_t(bl + 2)) {
		if (field(m, bl) == id) { offset = bl; return true; }
	}
	offset = pm::skillsEnd;
	return false;
}

void buildTrainableList(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	m.setD16(0x4696, 0xBD91);
	D(0x469D) = 1;
	D(0x469C) = 1;
	for (uint8_t bl = 0x11;; ++bl) {
		D(buildIndex) = bl;
		const uint8_t id = rec(m, bl);
		if (id == 0xFF) return;
		skillRecord(m, id);
		if (skillIq(m) <= field(m, pm::iq)) {
			++D(0x469D);
			D(0x4721) = D(0x469C);
		}
		++D(0x469C);
		if (uint8_t(bl + 1) == 0) return;
	}
}

bool trainingRow(Machine &m, uint8_t index) { return trainingRowOfSkill(m, skillIdAt(m, index)); }   // the list callback at 1BD91 first maps the row to its skill (1BE1F)

bool trainingRowOfSkill(Machine &m, uint8_t skill) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint8_t index = skill;
	D(0xDAA3) = index;
	skillRecord(m, index);
	if (skillIq(m) > field(m, pm::iq)) return true;
	call(m, 0x6EE8);   // sub_16EE8: row marker
	skillRecord(m, D(0xDAA3));
	D(0x467F) = 2;
	writeNumberShort(m, skillIq(m));
	writeCharacterVia(m, 32);
	D(skillBaseVar) = skillBase(m);
	D(skillLevelVar) = getMemberSkillLevel(m, D(0xDAA3));
	const uint8_t price = skillCost(D(skillBaseVar), uint8_t(D(skillLevelVar) + 1));
	++D(0x467F);
	writeNumberShort(m, price);
	writeCharacterVia(m, 32);
	writeNumberShort(m, D(skillLevelVar));
	++D(g::rectTextX);
	m.setD16(g::textPointer1, 0xB270);
	writeEncodedText(m, D(0xDAA3));
	return false;
}

bool skillSchool(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	const uint8_t keepPicture = D(0x46FA), keepRoster = D(g::rosterLocOn);
	for (uint8_t i = 0; i != 13; ++i) D(uint16_t(0x7201 + i)) = rec(m, uint8_t(4 + i));
	call(m, 0x90A6, 2);   // writeRosterLocation2
screen:   // 1BBC6
	call(m, 0x728C);
	printMapMessage(m, rec(m, 3));
	schoolMessage(m, 5, false);
	{
		const uint8_t pick = pickGroupMember(m, false);
		if (pick == 0) {
			D(g::rosterLocOn) = keepRoster;
			D(0x46FA) = keepPicture;
			return true;
		}
		D(schoolMember) = pick;
		if (isMemberUnconscious(m, pick)) {
			schoolMessage(m, 6, false);
			goto screen;
		}
		redrawRosterRow(m, pick, 0xFF);
	}
	drawInfoWindow(m);
	D(0x4699) = 1;
	D(0x4698) = 1;
train:   // 1BC0C
	{
		const uint8_t points = field(m, pm::skillPoints);
		if (points == 0) {
			schoolMessage(m, 1, false);
			goto screen;
		}
		uint8_t slot;
		if (!findSkillSlot(m, 0, slot)) {   // no free skill slot
			schoolMessage(m, 2, false);
			goto screen;
		}
	}
	buildTrainableList(m);
	if (D(0x469D) == 1) {
		schoolMessage(m, 9, false);
		goto screen;
	}
	cursorHome(m);
	schoolMessage(m, 3, true);
	pagedListDraw(m);
	for (;;) {
		cursorHome(m);   // 1BC42 / 1BD7F: the skill point header
		schoolMessage(m, 4, true);
		D(0x467F) = 2;
		writeNumberLeft(m, field(m, pm::skillPoints));
		m.setD16(g::buttonMask1, 0x0434);
		const ListResult r = pagedListKeys(m);
		if (r.al == 0xFF) goto screen;
		if (r.al >= 0xFD) continue;
		skillRecord(m, skillIdAt(m, r.al));
		const uint8_t base = skillBase(m);
		if (base > field(m, pm::skillPoints)) {   // 1BC74: not enough points
			cursorHome(m);
			schoolMessage(m, 7, true);
			call(m, 0x8DB4, 0, 7);   // delayFrames
			goto train;
		}
		uint8_t slot;
		const uint8_t id = skillIdAt(m, D(0x4698));
		if (findSkillSlot(m, id, slot)) {
			D(schoolSlot) = slot;
			const uint8_t level = field(m, uint8_t(slot + 1));
			uint8_t message = 0;
			if (level == 0xFF) message = 8;
			else if (level >= field(m, pm::level)) message = 0x0A;
			if (message) {
				cursorHome(m);
				schoolMessage(m, message, true);
				call(m, 0x8DB4, 0, 7);
				goto train;
			}
			skillRecord(m, id);
			const uint8_t price = skillCost(skillBase(m), uint8_t(level + 1));
			D(0xDAA7) = price;
			const int left = int(field(m, pm::skillPoints)) - price;
			if (uint8_t(left) & 0x80) {
				cursorHome(m);
				schoolMessage(m, 7, true);
				call(m, 0x8DB4, 0, 7);
				goto train;
			}
			field(m, pm::skillPoints) = uint8_t(left);
			++field(m, uint8_t(slot + 1));
			markPickedRow(m);
		} else {
			if (!findSkillSlot(m, 0, slot)) goto screen;
			D(schoolSlot) = slot;
			field(m, slot) = skillIdAt(m, D(0x4698));
			field(m, uint8_t(slot + 1)) = 1;
			skillRecord(m, field(m, slot));
			field(m, pm::skillPoints) = uint8_t(field(m, pm::skillPoints) - skillBase(m));
			markPickedRow(m);
		}
		if (field(m, pm::skillPoints) != 0) goto train;
		goto screen;
	}
}


namespace {
constexpr uint16_t shopPooler = 0xDBF5, shopDirty = 0xDBF6, shopDirty2 = 0xDBF7, shopItem = 0xDBF0, shopId = 0xDBF1, shopCount = 0xDBF2,
                   sellRowVar = 0xDBF3;

void goodsMessage(Machine &m, uint8_t id, bool immediate) {   // 1C20C / 1C213
	if (immediate) setWriteCharacterFn2(m);
	else setWriteCharacterFn(m);
	m.setD16(g::textPointer1, 0xDBF8);
	writeEncodedText(m, id);
}
void priceOfItem(Machine &m) {   // 17B5C: numberValue = the item's price (bytes 0 and 1)
	m.d8(g::numberValue) = m.d8(m.d16(g::itemDetailsPtr));
	m.d8(uint16_t(g::numberValue + 1)) = m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 1));
	m.d8(uint16_t(g::numberValue + 2)) = 0;
}
void discountedPrice(Machine &m, uint8_t shift) {   // 1C1D3: price - price / 2^shift
	if (shift == 0) return;
	uint32_t v = uint32_t(m.d8(g::numberValue)) | (uint32_t(m.d8(uint16_t(g::numberValue + 1))) << 8) |
	             (uint32_t(m.d8(uint16_t(g::numberValue + 2))) << 16);
	for (uint8_t i = 0; i != shift; ++i) v >>= 1;
	const uint32_t price = uint32_t(m.d8(m.d16(g::itemDetailsPtr))) | (uint32_t(m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 1))) << 8);
	const uint32_t result = (price - v) & 0xFFFFFF;
	m.d8(g::numberValue) = uint8_t(result);
	m.d8(uint16_t(g::numberValue + 1)) = uint8_t(result >> 8);
	m.d8(uint16_t(g::numberValue + 2)) = uint8_t(result >> 16);
}
}

bool itemTypeAccepted(Machine &m) {
	const uint8_t type = getItemType(m);
	m.d8(0xB9C0) = type;
	const uint16_t list = m.d16(0x46EC);
	for (uint8_t at = 0;; ++at) {
		const uint8_t t = m.d8(uint16_t(list + at));
		if (t == 0xFF) return false;
		if (t == type) return true;
		if (at == 0xFF) return false;
	}
}

void sellPrice(Machine &m) {
	priceOfItem(m);
	discountedPrice(m, m.d8(0x46C2));
}

void buyPrice(Machine &m) {
	priceOfItem(m);
	discountedPrice(m, m.d8(0x46C3));
}

uint8_t countSellable(Machine &m) {
	m.d8(0xCCE4) = 0xFF;
	uint8_t found = 0;
	for (uint8_t bl = 0xBD; bl != 0xF9; bl = uint8_t(bl + 2)) {
		m.d8(0xCCE3) = bl;
		const uint8_t id = field(m, bl);
		if (id == 0) continue;
		getItemDetails(m, id);
		if (!itemTypeAccepted(m)) continue;
		m.d8(0x4721) = uint8_t((bl - 0xBB) >> 1);
		++found;
		m.d8(0xCCE2) = found;
	}
	return found;
}

bool sellRow(Machine &m, uint8_t index) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(sellRowVar) = index;
	const uint8_t slot = uint8_t(index * 2 + 0xBB);
	const uint8_t id = field(m, slot);
	if (id == 0) return true;
	getItemDetails(m, id);
	if (!itemTypeAccepted(m)) return true;
	call(m, 0x6EE8);   // row marker
	D(0x467F) = 5;
	getItemDetails(m, id);
	sellPrice(m);
	call(m, 0x7690);   // sub_17690: price with space padding
	writeCharacterVia(m, 32);
	setItemQuantityOne(m);
	if (D(sellRowVar) == field(m, pm::armor) || D(sellRowVar) == field(m, pm::weapon)) D(g::inverseText) = 0xFF;
	writeItemName(m, id);
	D(g::inverseText) = 0;
	return false;
}

void sellScreen(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	if (countSellable(m) == 0) {
		goodsMessage(m, 5, false);
		return;
	}
	goodsMessage(m, 7, true);
	for (;;) {   // 1BF75
		const uint8_t count = countSellable(m);
		if (count == 0) {
			goodsMessage(m, 5, false);
			return;
		}
		D(0x469D) = uint8_t(count + 1);
		D(0x469C) = 0x1F;
		D(0x470E) = 0x50;
		m.setD16(0x4696, 0xC01B);
		pagedListDraw(m);
		showMoney(m);
		m.setD16(g::buttonMask1, 0x8434);
		m.setD16(g::buttonMask2, 0);
		const ListResult r = pagedListKeys(m);
		if (r.al == 0xFF) break;
		if (r.al == 0xFE) {
			poolMoney(m, D(shopPooler));
			continue;
		}
		if (r.al & 0x80) continue;
		removeWeaponArmor(m, r.al);
		const uint8_t slot = uint8_t(r.al * 2 + 0xBB);
		const uint8_t id = field(m, slot);
		field(m, slot) = 0;
		field(m, uint8_t(slot + 1)) = 0;
		getItemDetails(m, id);
		sellPrice(m);
		addMoneyToMember(m);
		markPickedRow(m);
		uint8_t &stock = m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 2));
		if (stock != 0xFF) ++stock;
		D(shopDirty) = 1;
		D(shopDirty2) = 1;
		const uint8_t left = countSellable(m);
		D(0x469D) = uint8_t(left + 1);
		if (left + 1 == 1) break;
		pagedListDraw(m);
	}
	if (D(shopDirty) != 0) call(m, 0x8654);   // loc_18654: writes the changed stock back
}

uint8_t countStock(Machine &m) {
	m.d8(shopId) = 1;
	getItemDetails(m, 1);
	m.d8(shopCount) = 0;
	for (;;) {
		if (m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 2)) != 0 && itemTypeAccepted(m)) {
			++m.d8(shopCount);
			m.d8(0x4721) = m.d8(shopId);
		}
		if (++m.d8(shopId) == 0x5E) break;
		m.setD16(g::itemDetailsPtr, uint16_t(m.d16(g::itemDetailsPtr) + 8));
	}
	return m.d8(shopCount);
}

bool buyRow(Machine &m, uint8_t item) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	D(shopItem) = item;
	getItemDetails(m, item);
	if (D(uint16_t(m.d16(g::itemDetailsPtr) + 2)) == 0) return true;
	if (!itemTypeAccepted(m)) return true;
	call(m, 0x6EE8);
	D(0x467F) = 5;
	D(g::inverseText) = 0;
	buyPrice(m);
	call(m, 0x7690);
	writeCharacterVia(m, 32);
	D(g::itemQuantity) = 1;
	writeItemName(m, D(shopItem));
	return false;
}

void buyScreen(Machine &m) {
	auto D = [&](uint16_t a) -> uint8_t & { return m.d8(a); };
	if (findItemById(m, 0) < 0) {
		goodsMessage(m, 8, false);
		return;
	}
	for (;;) {   // 1C081 / 1C091
		goodsMessage(m, 7, true);
		D(0x470E) = 0x50;
		m.setD16(0x4696, 0xC188);
		const uint8_t stock = countStock(m);
		if (stock == 0) {
			goodsMessage(m, 1, false);
			return;
		}
		for (;;) {
			D(0x469D) = stock;
			D(0x469C) = 0x5E;
			pagedListDraw(m);
			showMoney(m);
			m.setD16(g::buttonMask1, 0x8434);
			m.setD16(g::buttonMask2, 0);
			const ListResult r = pagedListKeys(m);
			if (r.al == 0xFF) goto leave;
			if (r.al == 0xFE) {
				poolMoney(m, D(shopPooler));
				break;
			}
			if (r.al & 0x80) break;
			getItemDetails(m, r.al);
			buyPrice(m);
			if (!canAffordNumber(m)) {
				showCursorHome(m);
				goodsMessage(m, 9, true);
				call(m, 0x8DB4, 0, 7);   // delayFrames
				break;
			}
			{
				const uint8_t quantity = m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 4));
				const int slot = findItemById(m, 0);
				field(m, uint8_t(slot)) = D(0x4698);
				field(m, uint8_t(slot + 1)) = quantity;
			}
			payNumberValue(m);
			markPickedRow(m);
			--m.d8(uint16_t(m.d16(g::itemDetailsPtr) + 2));
			D(shopDirty) = 1;
			D(shopDirty2) = 1;
			pagedListDraw(m);
			if (findItemById(m, 0) >= 0) break;
			showCursorHome(m);
			goodsMessage(m, 8, false);
			goto leave;
		}
	}
leave:   // 1C119
	if (D(shopDirty) != 0) call(m, 0x8654);
}

} // namespace wl
