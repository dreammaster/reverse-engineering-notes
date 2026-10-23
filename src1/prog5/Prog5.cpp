/* PROG5: the screens (list of characters, character sheet, equipment, creation) and the four modes. The routines are transliterations
 * of the disassembly with the original's names in the comments. Only the variant started from the book menu (parm_02 = 0) is ported:
 * the in-game variants (parm_02 = 1 or 2: choosing the leader, rearranging the party) are listed in docs1/overview.md. */

#include "prog5/Prog5Layout.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog5 {

using namespace Layout;

namespace {

const uint16 kList = 0x0A9F;             // the list entries, 36h bytes each
const uint16 kEntrySize = 0x36;
const uint16 kEquipRects = 0x0A6B;       // the three equipment rows (weapon, armour, shield), 8 bytes each
const uint16 kBoxMessage = 0x0E6B, kBoxList = 0x0E75, kBoxSheet = 0x0E7F, kBoxLabels = 0x0E89, kBoxNotice = 0x0E9D, kBoxAssemble = 0x0EB1;
const uint16 kNumberA = 0x12DA, kNumberB = 0x12E4;       // number formatting buffers
const uint16 kMenuSex = 0x163B, kMenuClass = 0x1643;
const uint16 kLabelLines = 0x1477, kTextSex = 0x1464, kTextName = 0x1469, kTextBlankField = 0x1344;
const uint16 kLevelCost = 0x4B85;        // four BCD bytes per level: the gold needed to train
const uint16 kClassRanges = 0x160B;      // (min, max) per attribute and class: 8 tables of 8 byte pairs (the original's loops use +8 per attribute)
const uint32_t kBufferASize = 0xF00u * 16, kBufferBSize = 0x272u * 16;

} // namespace

Prog5::Prog5(Shared::Runtime &rt, const Shared::Params &params)
	: _rt(rt), _c(rt.video.ctx), _ui(rt), _params(params), _haveExe(false), _descRecord(0), _status(0), _cur(1), _savedCur(0), _iconMask(0),
	  _record(0), _listCount(0), _firstRow(0), _scratchX(0), _scratchY(0), _destX(0), _destY(0), _file(0), _tile(0), _quit(false) {
	memset(_descBuffer, 0, sizeof(_descBuffer));
	std::string path = rt.dataDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "PROG5.EXE";
	Data::ExeImage exe;
	if (exe.load(path.c_str(), 0x3550))
		_haveExe = _ds.load(exe);
}

uint16 Prog5::slotWord(int slot) const {
	return _ds.w((uint16)(kSlots + 2 * slot));
}

byte Prog5::waitFor(byte keys, byte icons, byte flags) {
	_status = _ui.input(keys, icons, flags);
	if (_status == 0xFF) {
		_quit = true;
		_status = 1;
	}
	return _status;
}

// shared_b65991 and its twin: the page area under the screens is kept in buffer A and put back afterwards
void Prog5::saveBackground() {
	if (_bufferA.empty())
		_bufferA.assign(kBufferASize, 0);
	_c.x1 = 0x70; _c.y1 = 0x13; _c.x2 = 0x210; _c.y2 = 0x12D;
	const Vid::ImagePtr old = _c.image;
	_c.image.set(&_bufferA[0], (uint32_t)_bufferA.size(), 0);
	_rt.vid(0x0B);
	_c.image = old;
}

void Prog5::restoreBackground() {
	_c.x1 = 0x70; _c.y1 = 0x13; _c.x2 = 0x210; _c.y2 = 0x12D;
	if (_bufferA.empty())
		return;
	const Vid::ImagePtr old = _c.image;
	_c.image.set(&_bufferA[0], (uint32_t)_bufferA.size(), 0);
	_rt.vid(0x0C);
	_c.image = old;
}

// shared_0bb58f on the list entries (or any table of rectangles): `entry` is 1-based
void Prog5::putFrame(uint16 entry, byte colour) {
	_ui.drawFrame(_ds.p((uint16)(kList + (entry - 1) * kEntrySize)), colour);
}

// drawString that hands back where the string ended (the original leaves BX at the terminating NUL)
static uint16 drawAt(Shared::Runtime &rt, const Shared::DataSegment &ds, uint16 off) {
	rt.drawString(ds.str(off));
	return (uint16)(off + strlen(ds.str(off)));
}

#define DRAW(off) drawAt(_rt, _ds, (off))

// ---------------------------------------------------------------------------------------------
// the character sheet
// ---------------------------------------------------------------------------------------------

uint16 Prog5::itemNameOffset(uint16 item) const {          // sub_10F7D
	return (uint16)(kItems + item * 0x23 + 0x0E);
}

void Prog5::drawItemC() {                                  // sub_113CB: the weapon
	_c.textFg = _c.colourMap[0x0E];
	_c.textCol = 0x28;
	_c.textRow = 7;
	DRAW(itemNameOffset(_ds.w(kWeapon)));
}

void Prog5::drawItemA() {                                  // sub_110C3: the armour
	_c.textFg = _c.colourMap[0x0E];
	_c.textCol = 0x28;
	_c.textRow = 8;
	DRAW(itemNameOffset(_ds.w(kArmour)));
}

void Prog5::drawItemB() {                                  // sub_113AB: the shield
	_c.textFg = _c.colourMap[0x0E];
	_c.textCol = 0x28;
	_c.textRow = 9;
	DRAW(itemNameOffset(_ds.w(kShield)));
}

void Prog5::drawAttributes() {                             // sub_110E3
	_c.textFg = _c.colourMap[9];
	auto put = [&](byte col, byte row, uint16 value) {
		_c.textCol = col;
		_c.textRow = row;
		const uint16 end = formatNumber(value, kNumberA);
		DRAW((uint16)(end - 3));
	};
	put(0x28, 0x0B, _ds.b(kStat0));
	put(0x3B, 0x0B, _ds.b((uint16)(kStat0 + 1)));
	put(0x28, 0x0C, _ds.b((uint16)(kStat0 + 3)));
	put(0x3B, 0x0C, _ds.b((uint16)(kStat0 + 2)));
	put(0x28, 0x0D, _ds.b((uint16)(kStat0 + 4)));
	put(0x3B, 0x0D, _ds.b((uint16)(kStat0 + 5)));
	_c.textFg = _c.colourMap[0x0C];
	put(0x3B, 0x0F, _ds.w(kDerived73));
}

void Prog5::drawCombat() {                                 // sub_1129A
	_c.textFg = _c.colourMap[0x0C];
	auto put = [&](byte col, byte row, uint16 value) {
		_c.textCol = col;
		_c.textRow = row;
		const uint16 end = formatNumber(value, kNumberA);
		DRAW((uint16)(end - 3));
	};
	put(0x3B, 0x10, _ds.w(kDerived81));
	put(0x3B, 0x11, _ds.w(kDerived7B));
}

void Prog5::drawPoints() {                                 // sub_111C7
	auto put = [&](byte col, byte row, uint16 value) {
		_c.textCol = col;
		_c.textRow = row;
		const uint16 end = formatNumber(value, kNumberA);
		DRAW((uint16)(end - 3));
	};
	put(0x1F, 0x10, _ds.w(kPoints + 2));                   // hit points (word_13F67)
	put(0x28, 0x10, _ds.w(kPoints));                       // its maximum (word_13F65)
	const byte cls = _ds.b(kClass);
	if ((int8_t)cls <= 1 || cls == 4 || cls == 5) {        // classes without magic show a dash instead of the magic points
		const byte fg = _c.textFg;
		_c.textFg = _c.colourMap[0];
		_ds.setW(kNumberA, 0x0F);
		_c.textCol = 0x21;
		_c.textRow = 0x11;
		DRAW(kNumberA);
		_c.textCol = 0x2A;
		DRAW(kNumberA);
		drawCombat();
		_c.textFg = fg;
		return;
	}
	put(0x1F, 0x11, _ds.w(kPoints + 6));
	put(0x28, 0x11, _ds.w(kPoints + 4));
	drawCombat();
}

void Prog5::drawGoldText() {                               // sub_112E1: the gold, formatted
	_c.textFg = _c.colourMap[0x0C];
	_c.textCol = 0x21;
	_c.textRow = 0x12;
	moneyAsText(kNumberA);
	DRAW(kNumberA);
}

void Prog5::drawStatus() {                                 // sub_11305
	_c.textFg = _c.colourMap[0x0C];
	_c.textCol = 0x18;
	_c.textRow = 0x0F;
	const uint16 table = kStatusNames;
	DRAW((uint16)(table + 1 + _ds.b(kStatus) * _ds.b(table)));
}

void Prog5::drawLevel() {                                  // sub_11325
	_c.textFg = _c.colourMap[0x0C];
	_c.textCol = 0x3B;
	_c.textRow = 0x12;
	const uint16 end = formatNumber(_ds.b(kLevel), kNumberA);
	DRAW((uint16)(end - 3));
}

void Prog5::drawSheet() {                                  // sub_1134F: the whole character sheet
	_c.textCol = 0x25;
	_c.textRow = 4;
	_c.textFg = _c.colourMap[11];
	DRAW(kName);
	_c.textCol = _ds.b(0x1645);
	_c.textRow = _ds.b(0x1646);
	DRAW((uint16)(0x165E + 1 + _ds.b(kClass) * _ds.b(0x165E)));
	drawItemC();
	drawItemA();
	drawItemB();
	deriveStats();
	drawAttributes();
	drawPoints();
	drawCombat();
	drawLevel();
	drawGoldText();
	drawStatus();
	drawPortraitChoice();
}

// sub_11759: the 3x4 tiles of the portrait choice
void Prog5::drawPortraitChoice() {
	_c.x1 = 0x7E;
	_c.y1 = 0x34;
	_file = _ds.b(kSpriteFile);
	_tile = _ds.w(kSpriteBase);
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 3; col++) {
			_rt.drawTile(_file, _tile);
			_c.x1 = (uint16)(_c.x1 + 0x20);
			_tile++;
		}
		_tile = (uint16)(_tile + 7);
		_c.y1 = (uint16)(_c.y1 + 0x20);
		_c.x1 = 0x7E;
	}
}

// sub_11008: 0 when the item can be equipped in slot _cur by this character, else 1
uint16 Prog5::itemLacks(uint16 item) {
	const uint16 stock = _ds.w((uint16)(kStock + 2 * (item - 1)));
	if (stock == 0)
		return 1;
	const uint16 entry = (uint16)(kItems + item * 0x23);
	const byte flags = _ds.b(entry);
	if (_cur == 1) {                                       // weapon
		if (!(flags & 0x80))
			return 1;
		if (_ds.b((uint16)(entry + 5)) > _ds.b(kStat0))
			return 1;
		if (_ds.b((uint16)(entry + 6)) > _ds.b((uint16)(kStat0 + 3)))
			return 1;
		if (_ds.b((uint16)(entry + 7)) == 2 && _ds.w(kShield) != 0)
			return 1;
	} else if (_cur == 2) {                                // armour
		if (!(flags & 0x40))
			return 1;
		if (_ds.b((uint16)(entry + 6)) > _ds.b((uint16)(kStat0 + 3)))
			return 1;
	} else {                                               // shield
		if (!(flags & 0x20))
			return 1;
		if (_ds.b((uint16)(entry + 5)) > _ds.b(kStat0))
			return 1;
		if (_ds.w(kWeapon) != 0) {
			const uint16 weapon = (uint16)(kItems + _ds.w(kWeapon) * 0x23);
			if (_ds.b((uint16)(weapon + 7)) == 2)
				return 1;
		}
	}
	return (_ds.b((uint16)(entry + 1)) & 0x40) ? 0 : 1;
}

// sub_113EB: put `item` in slot _cur (1 weapon, 2 armour, 3 shield), take it from the party's stock, and work out the bonuses again
void Prog5::changeItem(uint16 item) {
	if (_cur == 1) {
		_ds.b(kFlags) |= 0x40;
		_ds.setW(kWeapon, item);
		if (item != 0)
			_ds.addW((uint16)(kStock + 2 * (item - 1)), (uint16)-1);
		drawItemC();
	} else if (_cur == 2) {
		_ds.setW(kArmour, item);
		if (item != 0)
			_ds.addW((uint16)(kStock + 2 * (item - 1)), (uint16)-1);
		drawItemA();
	} else {
		_ds.b(kFlags) |= 0x40;
		_ds.setW(kShield, item);
		if (item != 0)
			_ds.addW((uint16)(kStock + 2 * (item - 1)), (uint16)-1);
		drawItemB();
	}
	_ds.setW(kBonus79, 0);
	_ds.setW(kBonus7F, 0);
	if (_ds.w(kWeapon) != 0)
		_ds.setW(kBonus7F, _ds.b((uint16)(itemNameOffset(_ds.w(kWeapon)) - 0x0E + 4)));
	if (_ds.w(kArmour) != 0)
		_ds.setW(kBonus79, _ds.b((uint16)(itemNameOffset(_ds.w(kArmour)) - 0x0E + 4)));
	if (_ds.w(kShield) != 0)
		_ds.addW(kBonus79, _ds.b((uint16)(itemNameOffset(_ds.w(kShield)) - 0x0E + 4)));
	deriveStats();
	drawCombat();
}

// ---------------------------------------------------------------------------------------------
// the list of characters
// ---------------------------------------------------------------------------------------------

// sub_10D1C (variant parm_02 = 0): one list entry for the record in the buffer
void Prog5::listEntry(uint16 entry) {
	_ds.setW((uint16)(entry + 8), _descRecord);
	byte marker = 0x20;
	for (int s = 0; s < 6; s++)
		if (_descRecord == slotWord(s))
			marker = 0xFB;
	const byte status = _ds.b(kStatus);
	_ds.b((uint16)(entry + 0x0B)) = status;
	if ((int8_t)status >= 0x0E)
		marker = 0x0F;
	_ds.b((uint16)(entry + 0x0D)) = marker;
	_ds.b((uint16)(entry + 0x0C)) = _ds.b(kFlags);
	_ds.copy((uint16)(entry + 0x0E), (uint16)(kClassNames + _ds.b(kClass) * 15), 0x0E);
	_ds.copy((uint16)(entry + 0x1C), kRec, 0x1A);
	DRAW((uint16)(entry + 0x0D));
	const uint16 x1 = (uint16)(((_c.textCol + 0x0F) * 8) - 4);
	_ds.setW(entry, x1);
	const uint16 y1 = (uint16)(_c.textRow * 14);
	_ds.setW((uint16)(entry + 2), y1);
	_ds.setW((uint16)(entry + 6), (uint16)(y1 + 14));
	_ds.setW((uint16)(entry + 4), (uint16)(strlen(_ds.str(kName)) * 8 + 4 + x1));
	_c.textRow++;
}

// sub_10AEF (variant parm_02 = 0): list every character of the party file; false when there is none
bool Prog5::buildList() {
	const uint16 count = _ds.w(kCount);
	if (count == 10) {
		_status = 1;
		return false;
	}
	_c.textFg = _c.colourMap[0];
	_c.textBg = _c.colourMap[15];
	// variant parm_02 != 0 (choosing a party member in the game) is not ported
	_c.textFg = _c.colourMap[0];
	_c.x1 = 0x75; _c.y1 = 0x23; _c.x2 = 0x209; _c.y2 = 0x117;
	_c.colour = _c.colourMap[15];
	_rt.vid(0x07);
	_descRecord = 0x0A;
	const uint16 n = (uint16)(count - 0x0A);
	_listCount = n;
	_ds.setW(kBoxList + 2, 0x93);
	_ds.setW(kBoxList + 6, 0xA1);
	uint16 half = (uint16)((n & 0xFF) * 0x0E) >> 1;
	_ds.addW(kBoxList + 6, half);
	_ds.setW(kBoxList + 2, (uint16)(_ds.w(kBoxList + 2) - half));
	byte shifted = (byte)(n >> 1);
	if (n & 1) {
		_ds.addW(kBoxList + 2, 7);
		_ds.addW(kBoxList + 6, 7);
	}
	_c.textRow = 0x0B;
	_c.textRow = (byte)(_c.textRow - shifted);
	_firstRow = _c.textRow;
	_ds.setW(kBoxList, 0x8E);
	_ds.setW(kBoxList + 4, 0x1E2);
	_ds.b(kBoxList + 8) = _c.colourMap[0];
	_ds.b(kBoxList + 9) = 0x0E;
	_ui.drawBox(_ds.p(kBoxList));
	_c.textCol = 0x13;
	_descRecord = 0x0A;
	uint16 di = kList;
	_cur = 1;
	for (uint16 i = 0; i < n; i++) {
		readRecord(_descRecord, kRec);
		_ds.b((uint16)(di + 0x0A)) = _cur;
		listEntry(di);
		di = (uint16)(di + kEntrySize);
		_cur++;
		_descRecord++;
	}
	if (_savedCur == 0) {
		_savedCur = 1;
		// (parm_02 == 2 would look for the current member here)
	}
	_cur = _savedCur;
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	_status = 0;
	return true;
}

// sub_1188A: choose a character from the list: Up/Down/Enter/Esc, space (go on without choosing), a click on an entry
void Prog5::selectList() {
	_rt.vid(0x14);
	byte mask = 0x40;
	if (_params.parm(0x0E) == 4)
		mask |= 2;
	_iconMask = mask;
	_ui.drawIconBar(mask);
	_rt.vid(0x12, 0);
	for (;;) {
		putFrame(_cur, _c.colourMap[7]);
		_rt.system.flushKeys();
		for (;;) {
			if (waitFor(0, _iconMask, 0x20) == 3 && !_quit) {
				// a click: on an entry?
				const byte previous = _cur;
				bool hit = false;
				_cur = 1;
				for (uint16 i = 0; i < _listCount; i++, _cur++) {
					const uint16 e = (uint16)(kList + i * kEntrySize);
					const int16_t x = (int16_t)_c.curX, y = (int16_t)_c.curY;
					if (x < (int16_t)_ds.w(e) || x > (int16_t)_ds.w((uint16)(e + 4)) || y < (int16_t)_ds.w((uint16)(e + 2)) ||
					    y > (int16_t)_ds.w((uint16)(e + 6)))
						continue;
					_record = _ds.w((uint16)(e + 8));
					if (previous != _cur) {
						putFrame(previous, _c.colourMap[15]);
						putFrame(_cur, _c.colourMap[7]);
					}
					_status = 3;
					hit = true;
					break;
				}
				if (hit)
					return;
				_cur = previous;
				continue;
			}
			if (_quit) {
				_status = 1;
				return;
			}
			if (_c.textChar == 0x50) {                    // Down
				putFrame(_cur, _c.colourMap[15]);
				_cur = _cur == (byte)_listCount ? 1 : (byte)(_cur + 1);
				break;
			}
			if (_c.textChar == 0x48) {                    // Up
				putFrame(_cur, _c.colourMap[15]);
				_cur = _cur == 1 ? (byte)_listCount : (byte)(_cur - 1);
				break;
			}
			if (_c.textChar == 0x0D) {
				putFrame(_cur, _c.colourMap[15]);
				_record = _ds.w((uint16)(kList + (_cur - 1) * kEntrySize + 8));
				_status = 0;
				return;
			}
			if (_c.textChar == 0x1B) {
				_status = 1;
				return;
			}
			if (_c.textChar == 0x20) {
				_status = 0;
				return;
			}
			// 'K' (rearranging the party) belongs to the in-game variant
		}
	}
}

// ---------------------------------------------------------------------------------------------
// creating a character
// ---------------------------------------------------------------------------------------------

void Prog5::clearRecord() {                                // sub_110A9
	_ds.fill(kRec, 0, kRecordSize);
	_ds.b(kLevel) = 1;
	_ds.setW(kDerived81, 0);
}

void Prog5::drawCreateFrame() {                            // sub_10F44
	_ui.drawBox(_ds.p(kBoxSheet));
}

void Prog5::drawCreateLabels() {                           // sub_10F4D
	_c.textRow = 3;
	_c.textCol = 0x11;
	_c.textFg = _c.colourMap[1];
	_c.textBg = _c.colourMap[15];
	std::vector<const char *> lines;
	uint16 off = kLabelLines;
	for (int i = 0; i < 0x10; i++) {
		lines.push_back(_ds.str(off));
		off = (uint16)(off + strlen(_ds.str(off)) + 1);
	}
	_rt.drawStringLines(&lines[0], 0x10);
	_ui.drawBox(_ds.p(kBoxLabels));
}

// the attributes are drawn from the class's ranges (a random value between the minimum and the maximum for each)
void Prog5::rollStats() {                                  // sub_11498
	const uint16 cls = _ds.b(kClass);
	auto roll = [&](uint16 table, uint16 dst, uint16 dstMax) {
		const uint16 at = (uint16)(table + cls * 2);
		const byte lo = _ds.b(at), hi = _ds.b((uint16)(at + 1));
		const byte v = (byte)(_ui.random((uint16)(hi - lo)) + lo);
		_ds.b(dst) = v;
		if (dstMax)
			_ds.b(dstMax) = v;
		return v;
	};
	byte v = roll(kClassRanges, kStat0, kStatMax0);
	(void)v;
	v = roll((uint16)(kClassRanges + 8), (uint16)(kStat0 + 1), (uint16)(kStatMax0 + 1));
	if (cls == 3) {
		_ds.setW((uint16)(kPoints + 6), (uint16)(v >> 2));
		_ds.setW((uint16)(kPoints + 4), (uint16)(v >> 2));
	}
	v = roll((uint16)(kClassRanges + 16), (uint16)(kStat0 + 2), (uint16)(kStatMax0 + 2));
	if (cls == 2) {
		_ds.setW((uint16)(kPoints + 6), (uint16)(v >> 2));
		_ds.setW((uint16)(kPoints + 4), (uint16)(v >> 2));
	}
	roll((uint16)(kClassRanges + 24), (uint16)(kStat0 + 3), (uint16)(kStatMax0 + 3));
	v = roll((uint16)(kClassRanges + 32), (uint16)(kStat0 + 4), (uint16)(kStatMax0 + 4));
	_ds.setW((uint16)(kPoints + 2), (uint16)(v >> 2));
	_ds.setW(kPoints, (uint16)(v >> 2));
	roll((uint16)(kClassRanges + 40), (uint16)(kStat0 + 5), (uint16)(kStatMax0 + 5));
}

} // namespace Prog5
} // namespace Yendor1
