/* PROG5: the dialogs of the creation screens and the four modes (create, view, remove, assemble). */

#include "prog5/Prog5Layout.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog5 {

using namespace Layout;

namespace {

const uint16 kList = 0x0A9F;
const uint16 kEntrySize = 0x36;
const uint16 kEquipRects = 0x0A6B;
const uint16 kBoxMessage = 0x0E6B, kBoxNotice = 0x0E9D, kBoxAssemble = 0x0EB1;
const uint16 kMenuSex = 0x163B, kMenuClass = 0x1643;
const uint16 kTextSex = 0x1464, kTextName = 0x1469, kTextBlankField = 0x1344;
const uint16 kTextAssigned = 0x1395, kTextNoLead = 0x13EE, kTextCannotLead = 0x1429, kTextPartyFull = 0x13B3, kTextUnable = 0x1448,
             kTextBury = 0x140D, kTextAssembling = 0x132B;
const uint16 kNumberA = 0x12DA;
const uint32_t kBufferASize = 0xF00u * 16;

uint16 drawAt(Shared::Runtime &rt, const Shared::DataSegment &ds, uint16 off) {
	rt.drawString(ds.str(off));
	return (uint16)(off + strlen(ds.str(off)));
}

} // namespace

#define DRAW(off) drawAt(_rt, _ds, (off))

// ---------------------------------------------------------------------------------------------
// text input and the small menus
// ---------------------------------------------------------------------------------------------

// shared_ede455: read up to `length` characters into DS at `dest` at the current text position; _status: 0 Enter, 1 field full, 2 Esc
void Prog5::readText(uint16 dest, int length) {
	uint16 si = dest;
	int cx = length;
	_rt.vid(0x29);
	for (;;) {
		_c.textChar = 0xAE;                               // the cursor glyph
		_rt.vid(0x08);
		_rt.vid(0x28);
		bool leave = false;
		while (!leave) {
			if (!_rt.system.pump()) {
				_quit = true;
				_status = 2;
				_ds.b(si) = 0;
				_rt.vid(0x28);
				return;
			}
			const Shared::InputEvent ev = _rt.pollInput();
			_status = ev.source;
			if (_status == 0) {
				_rt.system.idle();
				continue;
			}
			bool enter = false, escape = false;
			if (_status == 3) {
				if (_c.iconBarState == 2) {
					_ui.iconClick(0x42);
					_status = _rt.last.source;
					if (_status == 0)
						continue;
					if (_c.textChar == 0x0D)
						enter = true;
					else if (_c.textChar == 0x1B)
						escape = true;
					else
						continue;
				} else {
					continue;
				}
			} else if (_status != 1) {
				continue;
			} else if (_c.textChar == 0x0D) {
				enter = true;
			} else if (_c.textChar == 0x1B) {
				escape = true;
			} else if (_c.textChar == 8) {                // backspace
				if (si <= dest) {                         // nothing to erase: beep
					_c.textChar = 7;
					_rt.vid(0x08);
					break;
				}
				si--;
				cx++;
				_rt.vid(0x29);
				_c.textChar = 0x20;
				_rt.vid(0x08);
				_c.textCol--;
				break;
			} else if (_c.textChar < 0x20 || _c.textChar > 0x7F) {
				continue;
			} else {
				_rt.vid(0x29);
				_ds.b(si) = _c.textChar;
				si++;
				cx--;
				if (cx == 0) {                            // the field is full
					_rt.vid(0x08);
					_c.textCol++;
					_status = 1;
					_rt.vid(0x28);
					return;
				}
				_rt.vid(0x08);
				_c.textCol++;
				break;
			}
			if (enter || escape) {
				_ds.b(si) = 0;
				_status = enter ? 0 : 2;
				_rt.vid(0x29);
				_c.textChar = 0x20;
				_rt.vid(0x08);
				_rt.vid(0x28);
				return;
			}
		}
	}
}

// sub_12DFC: a one-line menu of fixed-width choices drawn in reverse colours; Left/Right change, Enter takes (the index goes to
// the byte the table names), Esc cancels. The table at `di`: icon mask, last index, column, row, pointer to the strings (first byte =
// the width of one), pointer to the result byte.
void Prog5::menuChoice(uint16 di) {
	_rt.vid(0x14);
	const byte iconMask = _ds.b(di);
	_ui.drawIconBar(iconMask);
	_rt.vid(0x12, 0);
	_c.textCol = _ds.b((uint16)(di + 2));
	_c.textRow = _ds.b((uint16)(di + 3));
	const uint16 strings = _ds.w((uint16)(di + 4));
	const uint16 stride = _ds.b(strings);
	const uint16 first = (uint16)(strings + 1);
	const byte last = _ds.b((uint16)(di + 1));
	{
		const byte fg = _c.textFg;
		_c.textFg = _c.textBg;
		_c.textBg = fg;
	}
	_cur = 0;
	uint16 bx = first;
	for (;;) {
		bx = DRAW(bx);                                    // the string for _cur; bx is left at its NUL
		for (;;) {
			waitFor(0x4E, iconMask, 0);
			if (_quit) {
				_status = 1;
				goto done;
			}
			if (_c.textChar == 0x0D) {
				_ds.b(_ds.w((uint16)(di + 6))) = _cur;
				_status = 0;
				goto done;
			}
			if (_c.textChar == 0x1B) {
				_status = 1;
				goto done;
			}
			if (_c.textChar == 0x4B) {                    // Left
				if (_cur == 0) {
					bx = first;                           // wrap to the last choice
					for (int i = 0; i < last; i++)
						bx = (uint16)(bx + stride);
					_cur = last;
				} else {
					bx = (uint16)(bx + 1 - 2 * stride);
					_cur--;
				}
				break;
			}
			if (_c.textChar == 0x4D) {                    // Right
				if ((int8_t)_cur < (int8_t)last) {
					_cur++;
					bx = (uint16)(bx + 1);
				} else {
					_cur = 0;
					bx = first;
				}
				break;
			}
		}
	}
done:
	{
		bx = (uint16)(bx + 1 - stride);                   // back to the start of the string that is showing
		const byte fg = _c.textFg;
		_c.textFg = _c.textBg;
		_c.textBg = fg;
		DRAW(bx);
	}
}

void Prog5::chooseSex() {                                  // sub_11740
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	menuChoice(kMenuSex);
}

void Prog5::chooseClass() {                                // sub_11569
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	menuChoice(kMenuClass);
}

// sub_11582: the name
void Prog5::chooseName() {
	_rt.vid(0x29);
	_rt.vid(0x14);
	_ui.drawIconBar(0x42);
	_c.textCol = 0x24;
	_c.textRow = 4;
	_c.textFg = _c.colourMap[15];
	_c.textBg = _c.colourMap[11];
	DRAW(kTextBlankField);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	_c.textCol = 0x25;
	readText(kName, 0x19);
	if (_status == 2) {
		_status = 1;
		return;
	}
	_c.textCol = 0x24;
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	DRAW(kTextBlankField);
	// shared_ed3965: trailing blanks go
	auto trim = [&]() {
		int len = (int)strlen(_ds.str(kName));
		for (int i = len - 1; i >= 0 && _ds.b((uint16)(kName + i)) == 0x20; i--) {
			_ds.b((uint16)(kName + i)) = 0;
			if (i == 0)
				break;
		}
	};
	trim();
	if (strlen(_ds.str(kName)) == 0) {                      // no name typed: the class name is used
		const byte width = _ds.b(0x165E);
		_ds.copy(kName, (uint16)(kClassNames + _ds.b(kClass) * width + 1), width);
	}
	_c.textCol = 0x25;
	trim();
	DRAW(kName);
	_status = 0;
}

// sub_11651: choose the picture: Left/Right go through the pictures of the class, Enter takes it
void Prog5::choosePortrait() {
	const byte cls = _ds.b(kClass);
	_ds.setW(kHeadBase, (uint16)(cls * 0x50 + 0x320));
	_ds.setW(kSpriteBase, (uint16)(cls * 0xC8));
	_ds.b(kSpriteFile) = 1;
	if (_ds.b(0x12CE) != 0)                                   // byte_1481E: the sex chosen (1 = female)
		_ds.b(kSpriteFile) = 6;
	for (;;) {
		const int16_t cx = (int16_t)_c.curX, cy = (int16_t)_c.curY;
		if (!(cx > 0xDE || cx + 0x10 < 0x7E || cy > 0xB4 || cy + 0x10 < 0x34)) {
			_rt.vid(0x29);                                    // the pointer is over the picture
			_rt.vid(0x14);
			drawPortraitChoice();
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
		} else {
			_rt.vid(0x14);
			drawPortraitChoice();
			_rt.vid(0x12, 0);
		}
		for (;;) {
			waitFor(0x4E, 0x4E, 0);
			if (_quit) {
				_status = 1;
				return;
			}
			_status = 1;
			if (_c.textChar == 0x1B)
				return;
			if (_c.textChar == 0x0D) {
				_status = 0;
				return;
			}
			if (_c.textChar == 0x4B) {                        // Left
				if (_ds.w(kSpriteBase) == 0) {
					_ds.setW(kHeadBase, 0x450);
					_ds.setW(kSpriteBase, 0x2F8);
				} else {
					_ds.addW(kHeadBase, (uint16)-0x10);
					_ds.addW(kSpriteBase, (uint16)-0x28);
				}
				break;
			}
			if (_c.textChar == 0x4D) {                        // Right
				_ds.addW(kHeadBase, 0x10);
				_ds.addW(kSpriteBase, 0x28);
				if (_ds.w(kSpriteBase) == 0x320) {
					_ds.setW(kHeadBase, 0x320);
					_ds.setW(kSpriteBase, 0);
				}
				break;
			}
		}
	}
}

// sub_11D79: the attributes are rolled; Space rolls again, Enter keeps them, Esc leaves
void Prog5::accept() {
	_rt.vid(0x29);
	_rt.vid(0x14);
	_ui.drawIconBar(0xC2);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	for (;;) {
		_c.textFg = _c.colourMap[9];
		_c.textBg = _c.colourMap[15];
		rollStats();
		deriveStats();
		drawAttributes();
		_c.textFg = _c.colourMap[12];
		drawPoints();
		drawCombat();
		for (;;) {
			waitFor(0xC2, 0xC2, 0);
			_status = 1;
			if (_quit)
				return;
			if (_c.textChar == 0x20)
				break;
			if (_c.textChar == 0x1B)
				return;
			if (_c.textChar == 0x0D) {
				_status = 0;
				return;
			}
		}
	}
}

// sub_10F8D: the first party member has to be able to lead; otherwise a notice says why. Sets _status (1 = shown the notice).
bool Prog5::noPurchaseRoom() {
	_status = 0;
	readRecord(_ds.w(kSlots), kRec);
	_status = 0;
	const byte status = _ds.b(kStatus);
	if (status != 6) {
		if ((int8_t)status < 9)
			return false;
		if (status == 0x0A)
			return false;
	}
	_rt.vid(0x29);
	_rt.vid(0x14);
	_c.textCol = (byte)(_c.textCol - 5);
	_c.textRow = 0x0C;
	_ui.drawBox(_ds.p(kBoxNotice));
	DRAW((int8_t)status >= 0x0E ? kTextNoLead : kTextCannotLead);
	_c.textCol = (byte)(_c.textCol + 5);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	for (;;) {                                              // waitInput
		if (!_rt.system.pump()) {
			_quit = true;
			break;
		}
		if (_rt.pollInput().source != Shared::InputEvent::None)
			break;
		_rt.system.idle();
	}
	_rt.vid(0x12, 1);
	_rt.vid(0x1E);
	_status = 1;
	return true;
}

// ---------------------------------------------------------------------------------------------
// the modes
// ---------------------------------------------------------------------------------------------

void Prog5::finish() {                                      // loc_10AB7
	_rt.vid(0x29);
	_rt.vid(0x14);
	restoreBackground();
	_ui.drawBottomDecorations();
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
}

// loc_101E4: create a character
void Prog5::modeCreate() {
	_rt.vid(0x29);
	saveBackground();
	loadParty();
	_rt.vid(0x14);
	_ui.drawIconBar(0x4E);
	_c.cursorShape = 1;
	drawCreateFrame();
	drawCreateLabels();
	clearRecord();
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	_c.textCol = 0x20;
	_c.textRow = 4;
	DRAW(kTextSex);
	_status = 0;
	chooseSex();
	if (_status != 0)
		return finish();
	_status = 0;
	chooseClass();
	if (_status != 0)
		return finish();
	choosePortrait();
	if (_status != 0)
		return finish();
	_c.textFg = _c.colourMap[1];
	_c.textCol = 0x20;
	_c.textRow = 4;
	DRAW(kTextName);
	chooseName();
	if (_status != 0)
		return finish();
	_ds.b(0x0A07) = 1;
	_ds.setW(kArmour, 0x1A);
	_ds.setW(kBonus79, 1);
	_ds.setW(kShield, 0);
	const byte cls = _ds.b(kClass);
	if (cls == 0) {
		_ds.setW(kWeapon, 0x2C);
		_ds.setW(kBonus7F, 3);
	} else if (cls == 1) {
		_ds.setW(kWeapon, 0x24);
		_ds.setW(kBonus7F, 4);
	} else if (cls == 2) {
		_ds.setW(kWeapon, 0x23);
		_ds.setW(kBonus7F, 4);
	} else {
		_ds.setW(kWeapon, 0x38);
		_ds.setW(kBonus7F, 3);
	}
	drawItemC();
	drawItemA();
	drawItemB();
	drawLevel();
	drawGoldText();
	drawStatus();
	accept();
	if (_status != 0)
		return finish();
	loadParty();
	writeRecord(_ds.w(kCount), kRec);
	_ds.addW(kCount, 1);
	pickLeaderRecord();
	saveParty();
	finish();
}

// loc_1032D: view the characters, and change the equipment of those in the party
void Prog5::modeView() {
	_rt.vid(0x29);
	saveBackground();
	loadParty();
	_rt.vid(0x14);
	for (;;) {                                              // loc_1032D
		if (!buildList())
			return finish();
		for (;;) {                                          // loc_1033A
			selectList();
			if (_status != 0 && _status != 3)
				return finish();                            // (parm_02 == 1 would go on to choose a leader)
			if (_c.textChar != 0x20)
				break;
		}
		_savedCur = _cur;
		_rt.vid(0x29);
		_rt.vid(0x14);
		restoreBackground();
		_c.x1 = 0x8D; _c.y1 = 0x13; _c.x2 = 0x1F1; _c.y2 = 0x12A;
		_c.colour = _c.colourMap[15];
		_rt.vid(0x07);
		drawCreateFrame();
		drawCreateLabels();
		readRecord(_record, kRec);
		drawSheet();
		if (!(_ds.b(kFlags) & 1)) {                          // not in the party: just look
			_ui.drawIconBar(2);
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
			waitFor(2, 2, 0);
			loadParty();
		} else {
			editEquipment();
		}
		_rt.vid(0x29);
		_rt.vid(0x14);
		restoreBackground();
		if (_quit)
			return finish();
	}
}

// loc_10407: the three equipment rows: Up/Down choose the row, Left/Right go through the items, Enter keeps the changes
void Prog5::editEquipment() {
	_cur = 1;
	_ui.drawIconBar(0x4E);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	auto rect = [&](byte i) { return _ds.p((uint16)(kEquipRects + (i - 1) * 8)); };
	auto step = [&](int direction) {                        // loc_1053A (Right: +1) and loc_1057E (Left: -1)
		uint16 item = _cur == 1 ? _ds.w(kWeapon) : (_cur == 2 ? _ds.w(kArmour) : _ds.w(kShield));
		if (item != 0)
			_ds.addW((uint16)(kStock + 2 * (item - 1)), 1);   // back into the stock
		for (;;) {
			item = (uint16)(item + direction);
			if (direction > 0) {
				if ((int16_t)item >= 0x190) {
					changeItem(0);
					return;
				}
			} else {
				if (item == 0) {
					changeItem(0);
					return;
				}
				if ((int16_t)item < 0)
					item = 0x190;
			}
			if (itemLacks(item) == 0) {
				changeItem(item);
				return;
			}
		}
	};
	for (;;) {
		_ui.drawFrame(rect(_cur), _c.colourMap[7]);
		_rt.system.flushKeys();
		bool again = true;
		while (again) {
			again = false;
			waitFor(0, 0x4E, 0x20);
			if (_quit) {
				loadParty();
				return;
			}
			if (_status == 3) {                              // a click: on one of the rows?
				const byte previous = _cur;
				_cur = 1;
				bool hit = false;
				for (int i = 0; i < 3; i++, _cur++) {
					const byte *r = rect(_cur);
					auto word = [r](int k) { return (int16_t)(r[k] | (r[k + 1] << 8)); };
					const int16_t x = (int16_t)_c.curX, y = (int16_t)_c.curY;
					if (x < word(0) || x > word(4) || y < word(2) || y > word(6))
						continue;
					_ui.drawFrame(_ds.p((uint16)(kEquipRects + (previous - 1) * 8)), _c.colourMap[15]);
					hit = true;
					break;
				}
				if (hit)
					break;                                    // redraw the frame on the new row
				_cur = previous;
				again = true;
				continue;
			}
			if (_c.textChar == 0x50) {
				_ui.drawFrame(rect(_cur), _c.colourMap[15]);
				_cur = _cur == 3 ? 1 : (byte)(_cur + 1);
				break;
			}
			if (_c.textChar == 0x48) {
				_ui.drawFrame(rect(_cur), _c.colourMap[15]);
				_cur = _cur == 1 ? 3 : (byte)(_cur - 1);
				break;
			}
			if (_c.textChar == 0x4D) {
				step(+1);
				again = true;
				continue;
			}
			if (_c.textChar == 0x4B) {
				step(-1);
				again = true;
				continue;
			}
			if (_c.textChar == 0x0D) {                       // keep
				_status = 0;
				_ui.drawFrame(rect(_cur), _c.colourMap[15]);
				if (_ds.b(kFlags) & 0x40) {
					_rt.vid(0x29);
					_rt.vid(0x14);
					drawPartyStrip();
					_rt.vid(0x1E);
					_rt.vid(0x28);
				}
				writeRecord(_record, kRec);
				pickLeaderRecord();
				saveParty();
				return;
			}
			if (_c.textChar == 0x1B) {                       // leave without keeping
				loadParty();
				return;
			}
			again = true;
		}
	}
}

// loc_1062B: remove a character from the party file
void Prog5::modeRemove() {
	_rt.vid(0x29);
	saveBackground();
	loadParty();
	_rt.vid(0x14);
	for (;;) {                                              // loc_1062B
		if (!buildList())
			return finish();
		for (;;) {                                          // loc_10638
			selectList();
			if (_status != 0 && _status != 3)
				return finish();
			if (_c.textChar != 0x20)
				break;
		}
		_savedCur = _cur;
		_rt.vid(0x29);
		_rt.vid(0x14);
		readRecord(_record, kRec);
		if (_ds.b(kFlags) & 1) {                             // in the party: cannot be removed
			_c.textRow = 0x0C;
			_c.textCol = 0x18;
			_ui.drawBox(_ds.p(kBoxMessage));
			DRAW(kTextAssigned);
			_ui.drawIconBar(2);
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
			waitFor(2, 2, 0);
			_rt.vid(0x29);
			_rt.vid(0x14);
			restoreBackground();
			continue;
		}
		_c.x1 = 0x8D; _c.y1 = 0x13; _c.x2 = 0x1F1; _c.y2 = 0x12A;
		_c.colour = _c.colourMap[15];
		_rt.vid(0x07);
		drawCreateFrame();
		drawCreateLabels();
		drawSheet();
		_ui.drawIconBar(0x42);
		_rt.vid(0x12, 0);
		bool confirmed = false;
		for (;;) {
			_rt.vid(0x28);
			waitFor(0x42, 0x42, 0);
			if (_quit)
				return finish();
			if (_c.textChar == 0x0D) {
				confirmed = true;
				break;
			}
			if (_c.textChar == 0x1B)
				break;
		}
		if (!confirmed) {
			_rt.vid(0x29);
			_rt.vid(0x14);
			restoreBackground();
			continue;
		}
		_savedCur = 1;
		uint16 next = (uint16)(_record + 1);
		_descRecord = next;
		if ((int16_t)_ds.w(kCount) > (int16_t)next) {         // close the gap: every later record moves down
			for (;;) {
				readRecord(_descRecord, kRec);
				const uint16 at = _descRecord;
				writeRecord((uint16)(at - 1), kRec);
				_descRecord = (uint16)(at - 1 + 2);
				if (_ds.w(kCount) == _descRecord)
					break;
			}
		}
		_ds.fill(kRec, 0, kRecordSize);
		_ds.addW(kCount, (uint16)-1);
		writeRecord(_ds.w(kCount), kRec);
		// (the original then writes zero bytes to the file, which cuts it at the end of the record just written: no change)
		pickLeaderRecord();
		saveParty();
		for (int s = 0; s < 6; s++) {
			const uint16 id = slotWord(s);
			if ((int16_t)id >= (int16_t)_record)
				_ds.setW((uint16)(kSlots + 2 * s), (uint16)(id - 1));
		}
		saveParty();
		_rt.vid(0x29);
		_rt.vid(0x14);
		restoreBackground();
	}
}

// loc_107C9: choose who is in the party: Space or a click on a name puts it in or takes it out, Enter assembles, Esc leaves
void Prog5::modeAssemble() {
	_rt.vid(0x29);
	saveBackground();
	loadParty();
	_rt.vid(0x14);
	if (!buildList())
		return finish();
	for (;;) {
		selectList();
		if (_status != 3 && _c.textChar != 0x20) {
			if (_c.textChar == 0x0D) {
				assembleParty();
				return finish();
			}
			if (_c.textChar != 0x1B)
				continue;
			loadParty();
			return finish();
		}
		_savedCur = _cur;
		const uint16 e = (uint16)(kList + (_cur - 1) * kEntrySize);
		auto showNotice = [&](uint16 boxText) {
			_c.textRow = 0x0C;
			_c.textCol = 0x18;
			_ui.drawBox(_ds.p(kBoxMessage));
			DRAW(boxText);
			_ui.drawIconBar(2);
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
			waitFor(2, 2, 0);
			_rt.vid(0x29);
			_rt.vid(0x1E);
		};
		bool changed = false;
		if (_ds.b((uint16)(e + 0x0D)) != 0x20) {              // in the party (or gone): take out
			if ((int8_t)_ds.b((uint16)(e + 0x0B)) >= 0x0E) {
				_rt.vid(0x29);
				_rt.vid(0x14);
				showNotice(kTextBury);
				continue;
			}
			_ds.b((uint16)(e + 0x0D)) = 0x20;
			for (int s = 0; s < 6; s++) {
				if (_ds.w((uint16)(e + 8)) == slotWord(s)) {
					_ds.setW((uint16)(kSlots + 2 * s), 0);
					_ds.b((uint16)(e + 0x0A)) = 0;
					break;
				}
			}
			changed = true;
		} else {                                              // put in
			_ds.b((uint16)(e + 0x0A)) = 1;
			int s = 0;
			if (_ds.b((uint16)(e + 0x0B)) >= 0x0A) {          // the badly off cannot be in front
				_ds.b((uint16)(e + 0x0A))++;
				s = 1;
			}
			bool placed = false;
			for (; s < 6; s++) {
				if (slotWord(s) == 0) {
					_ds.b((uint16)(e + 0x0D)) = 0xFB;
					_ds.setW((uint16)(kSlots + 2 * s), _ds.w((uint16)(e + 8)));
					placed = true;
					break;
				}
				_ds.b((uint16)(e + 0x0A))++;
			}
			if (!placed) {
				_rt.vid(0x29);
				_rt.vid(0x14);
				_ds.b((uint16)(e + 8)) = 0;
				showNotice(_ds.b((uint16)(e + 0x0B)) >= 0x0A ? kTextUnable : kTextPartyFull);
				continue;
			}
			changed = true;
		}
		if (changed) {
			_c.textCol = 0x13;
			_c.textRow = (byte)(_firstRow + _cur - 1);
			_ds.setW(kNumberA, _ds.b((uint16)(e + 0x0D)));
			DRAW(kNumberA);
		}
	}
}

// loc_10925: pack the party towards the front, clear and set the "in party" flags, render the portraits, save
void Prog5::assembleParty() {
	// pack: an empty slot takes the next member that may stand there
	for (int slot = 0; slot < 5; slot++) {
		if (slotWord(slot) != 0)
			continue;
		bool moved = false;
		bool stop = true;
		for (int later = slot + 1; later < 6; later++) {
			const uint16 id = slotWord(later);
			if (id == 0)
				continue;
			for (uint16 i = 0; i < _listCount; i++) {
				const uint16 e = (uint16)(kList + i * kEntrySize);
				if (_ds.w((uint16)(e + 8)) != id)
					continue;
				if (slotWord(0) == 0 && (int8_t)_ds.b((uint16)(e + 0x0B)) >= 0x0A)
					break;                                    // cannot lead
				_ds.setW((uint16)(kSlots + 2 * later), 0);
				_ds.setW((uint16)(kSlots + 2 * slot), id);
				moved = true;
				break;
			}
			if (moved)
				break;
		}
		if (!moved && stop)
			break;
	}
	_rt.vid(0x29);
	_rt.vid(0x14);
	_ui.drawBox(_ds.p(kBoxAssemble));
	_c.textCol = 0x1C;
	_c.textRow = 0x0A;
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	DRAW(kTextAssembling);
	_rt.vid(0x12, 0);
	_record = 0x0A;
	while (_record != _ds.w(kCount)) {                       // nobody is in the party any more
		readRecord(_record, kRec);
		_ds.b(kFlags) &= 0xFE;
		writeRecord(_record, kRec);
		_record++;
	}
	_rt.vid(0x14);
	_cur = 1;
	for (int s = 0; s < 6; s++, _cur++) {
		const uint16 id = slotWord(s);
		if (id == 0)
			continue;
		readRecord(id, kRec);
		if (_cur == 1) {
			_ds.b(0x062A) = _ds.b(kSpriteFile);
			_ds.setW(0x062B, _ds.w(kSpriteBase));
			_ds.setW(0x0633, _ds.w(kHeadBase));
		}
		if (!(_ds.b(kFlags) & 0x80)) {                        // first time in a party: the starting supplies
			_ds.b(kFlags) |= 0x80;
			_ds.addW(kStock, 0x32);
			const byte cls = _ds.b(kClass);
			if (cls == 0)
				_ds.addW((uint16)(kStock + 2 * 0x10A), 1);
			else if (cls == 1)
				_ds.addW((uint16)(kStock + 2 * 0x10B), 1);
			else
				_ds.addW((uint16)(kStock + 2 * 3), 5);
		}
		_ds.b(kFlags) |= 1;
		_ds.setW(kPortrait, (uint16)((_cur - 1) * 20));
		drawPartyStrip();
		writeRecord(id, kRec);
	}
	_rt.vid(0x1E);
	_rt.vid(0x28);
	pickLeaderRecord();
	saveParty();
}

// ---------------------------------------------------------------------------------------------
// the program
// ---------------------------------------------------------------------------------------------

int Prog5::run() {
	if (!_haveExe)
		return 0xFE;
	_rt.vid(0x2D, 0);
	// the boxes' fill colours start out as the first palette colour
	static const uint16 boxFill[8] = {0x0E73, 0x0E91, 0x0E9B, 0x0EA5, 0x0EAF, 0x0E7D, 0x0E87, 0x0EB9};
	for (int i = 0; i < 8; i++)
		_ds.b(boxFill[i]) = _c.colourMap[0];
	_bufferA.clear();
	_ds.setW(0x0A93, 0);
	static const uint16 table[5] = {0, 0x1E4, 0x31D, 0x61B, 0x7C9};
	const byte mode = _params.parm(0x0E);
	if (mode == 0 || mode > 4)
		return 0xFD;
	if (mode != 1 && _params.parm(2) != 0)
		return 0xFC;                                         // the in-game variants (choose the leader, rearrange) are not ported yet
	(void)table;
	_scratchX = 0;
	_scratchY = 0;
	switch (mode) {
	case 1: modeCreate(); break;
	case 2: modeView(); break;
	case 3: modeRemove(); break;
	case 4: modeAssemble(); break;
	}
	return 0;
}

} // namespace Prog5
} // namespace Yendor1
