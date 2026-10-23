#include "prog6/Prog6.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog6 {

namespace {

// PROG6.EXE's data segment starts 1930h bytes into its load image
const uint32_t kDataBase = 0x1930;
const uint16 kBoxThree = 0x0E85, kBoxFive = 0x0E8F, kBoxFour = 0x0EA3, kBoxMessage = 0x0E99;
const uint16 kEntryText = 0x0EFE;
const uint16 kSavingText = 0x0ECD, kRestoringText = 0x0EE5;
const uint16 kHitBoxes = 0x0F84;
const uint16 kLabelTable = 0x13C9;         // near pointers to the confirmation labels, indexed by code - 0Ah
const uint16 kPanelBox = 0x1574, kPanelTitle1 = 0x154F, kPanelTitle2 = 0x1561;
const uint16 kAreaX1 = 0xC0, kAreaY1 = 0x77, kAreaX2 = 0x1E7, kAreaY2 = 0xDA;

} // namespace

Prog6::Prog6(Shared::Runtime &rt, const Shared::Params &params, Shared::Launcher &launcher)
	: _rt(rt), _c(rt.video.ctx), _ui(rt), _params(params), _launcher(launcher), _haveExe(false), _entry(1), _count(3), _state(0), _quit(false) {
	memset(_party, 0, sizeof(_party));
	std::string path = rt.dataDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "PROG6.EXE";
	_haveExe = _exe.load(path.c_str(), kDataBase);
}

const byte *Prog6::data(uint16 dsOffset, uint32_t length) const {
	return _haveExe ? _exe.bytesAt(dsOffset, length) : 0;
}

const char *Prog6::text(uint16 dsOffset) const {
	const char *s = _haveExe ? _exe.stringAt(dsOffset) : 0;
	return s ? s : "";
}

bool Prog6::readPartyRecord(uint16 record, byte *buf) {
	std::string path = _rt.workDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "CURRGAME/PARTY.DAT";
	Shared::RecordFile f;
	return f.open(path.c_str(), 100, false) && f.read(record, buf);
}

// sub_10550 / sub_10513: the page area under the popup is kept in a buffer and put back when the popup goes away
void Prog6::saveBackground() {
	if (_saved.empty())
		_saved.assign(0x39E * 16, 0);
	_c.x1 = kAreaX1; _c.y1 = kAreaY1; _c.x2 = kAreaX2; _c.y2 = kAreaY2;
	const Vid::ImagePtr old = _c.image;
	_c.image.set(&_saved[0], (uint32_t)_saved.size(), 0);
	_rt.vid(0x0B);
	_c.image = old;
}

void Prog6::restoreBackground() {
	_c.x1 = kAreaX1; _c.y1 = kAreaY1; _c.x2 = kAreaX2; _c.y2 = kAreaY2;
	if (_saved.empty())
		return;
	const Vid::ImagePtr old = _c.image;
	_c.image.set(&_saved[0], (uint32_t)_saved.size(), 0);
	_rt.vid(0x0C);
	_c.image = old;
}

// sub_1058D: the Esc icon is shown; Up/Down/Enter/Esc or a click on an entry
void Prog6::chooseEntry() {
	_ui.drawIconBar(0x40);
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	auto entryRect = [this](byte entry) { return data((uint16)(kHitBoxes + (entry - 1) * 8), 8); };
	for (;;) {
		if (const byte *r = entryRect(_entry))
			_ui.drawFrame(r, _c.colourMap[7]);
		_rt.system.flushKeys();
		for (;;) {
			_state = _ui.input(0, 0x40, 0x20);
			if (_state == 0xFF) {
				_quit = true;
				_state = 3;
				return;
			}
			if (_state == 3) {                              // a click that missed the Esc icon: on an entry?
				bool hit = false;
				for (byte i = 1; i <= _count && !hit; i++) {
					const byte *r = entryRect(i);
					if (!r)
						break;
					auto word = [r](int k) { return (int16_t)(r[k] | (r[k + 1] << 8)); };
					const int16_t x = (int16_t)_c.curX, y = (int16_t)_c.curY;
					if (x < word(0) || x > word(4) || y < word(2) || y > word(6))
						continue;
					_ui.drawFrame(entryRect(_entry), _c.colourMap[15]);
					_entry = i;
					_ui.drawFrame(entryRect(_entry), _c.colourMap[7]);
					_state = 0;
					hit = true;
				}
				if (hit)
					return;
				continue;
			}
			if (_c.textChar == 0x50) {                      // Down
				_ui.drawFrame(entryRect(_entry), _c.colourMap[15]);
				_entry = _entry == _count ? 1 : (byte)(_entry + 1);
				break;                                      // draw the new frame and flush again
			}
			if (_c.textChar == 0x48) {                      // Up
				_ui.drawFrame(entryRect(_entry), _c.colourMap[15]);
				_entry = _entry == 1 ? _count : (byte)(_entry - 1);
				break;
			}
			if (_c.textChar == 0x1B)
				_state = 3;
			if (_c.textChar == 0x0D || _c.textChar == 0x1B) {
				_ui.drawFrame(entryRect(_entry), _c.colourMap[15]);
				return;
			}
		}
	}
}

// sub_104BF: "Are you certain that you want to SAVE / RESTORE / EXIT?"
bool Prog6::confirm() {
	int code;
	if (_entry == 1 || _entry == 2)
		code = 0x1E;
	else if (_entry == 3)
		code = 0x1F;
	else if (_entry == 4)
		code = 0x20;
	else
		return false;
	const byte *table = data((uint16)(kLabelTable + 2 * (code - 0x0A)), 2);
	const uint16 labelOffset = table ? (uint16)(table[0] | (table[1] << 8)) : 0;
	if (const byte *box = data(kPanelBox, 10))
		_ui.openConfirmPanel(box, text(kPanelTitle1), text(kPanelTitle2), text(labelOffset), 0x42);
	if (_ui.input(0x42, 0x42, 0) == 0xFF)
		_quit = true;
	_rt.vid(0x29);
	_ui.closeInfoPanel();
	return _c.textChar == 0x0D;
}

// loc_102C7 / loc_102DE: take the popup away (the area is put back only for the first), show the page
void Prog6::leave(bool restoreArea) {
	_rt.vid(0x29);
	_rt.vid(0x14);
	if (restoreArea)
		restoreBackground();
	_ui.drawBottomDecorations();
	_rt.vid(0x12, 0);
}

void Prog6::drawPartyStrip(const byte *) {
	// The party strip (a character's portrait and status in the strip at the bottom of the page) is drawn by shared character
	// rendering code (drawPartyStrip: equipment layers, hit points ...) that is ported together with PROG5. Nothing is drawn yet.
}

// loc_10322 (mode 2: saving) and loc_103C6 (mode 3: restoring): a message box, then PROG0 in the given mode. Returns the code
// PROG0 ended with, or kNotImplemented.
int Prog6::saveOrRestore(int mode, const char *message) {
	readPartyRecord(0, _party);
	_rt.vid(0x29);
	_rt.vid(0x14);
	restoreBackground();
	_c.textCol = 0x1E;
	_c.textRow = 0x0A;
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	if (const byte *box = data(kBoxMessage, 10))
		_ui.drawBox(box);
	_rt.drawString(message);
	_ui.drawBottomDecorations();
	_rt.vid(0x12, 0);
	_params.setParm(0x0E, (byte)mode);
	_params.setParm(0x10, _party[0x21]);                  // byte_11959: the current save slot
	Shared::Params child = _params;
	const Vid::ImagePtr saved = _c.image;
	const int code = _launcher.run(0, child, _rt);
	_c.image = saved;
	_params.setParm(0x0E, 0);
	_params.setParm(0x10, 0);
	return code;
}

int Prog6::run() {
	_rt.vid(0x29);
	_rt.vid(0x2D, 0);
	_rt.vid(0x14);
	uint16 box;
	if (_params.parm(2) == 1) {
		saveBackground();
		box = kBoxFive;
		_count = 5;
		const uint16 p12 = (uint16)(_params.parm(0x12) | (_params.parm(0x13) << 8));
		const bool camp = p12 == 4 || p12 == 0x0B || p12 == 0x12 || p12 == 0x14 || (p12 >= 0xFFC0 && p12 <= 0xFFC4);
		if (_params.parm(3) == 1 && _params.parm(5) == 0 && !camp) {
			_count = 4;
			box = kBoxFour;
		}
	} else if (_params.parm(2) == 0) {
		saveBackground();
		_count = 3;
		box = kBoxThree;
	} else {
		return 0xFD;
	}
	if (const byte *b = data(box, 10))
		_ui.drawBox(b);
	_c.textCol = 0x1C;
	_c.textRow = 9;
	_c.textFg = _c.colourMap[0];
	_c.textBg = _c.colourMap[15];
	{
		std::vector<const char *> lines;
		uint32_t off = kEntryText;
		for (int i = 0; i < _count; i++) {
			const char *s = text((uint16)off);
			lines.push_back(s);
			off += (uint32_t)strlen(s) + 1;
		}
		_rt.drawStringLines(&lines[0], _count);
	}

	for (;;) {
		chooseEntry();
		if (_quit || _state == 3) {                         // Esc (or the platform quit)
			_entry = 0;
			leave(true);
			break;
		}
		if (_entry >= 6) {
			_entry = 0xFD;
			leave(false);
			break;
		}
		if (_entry == 1 || _entry == 2) {
			if (!confirm())
				continue;
			const int code = saveOrRestore(2, text(kSavingText));
			if (code == Shared::Launcher::kNotImplemented)
				return code;
			if (code != 0)
				_entry = (byte)code;
			_rt.vid(0x14);
			restoreBackground();
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
			break;
		}
		if (_entry == 3) {
			if (!confirm())
				continue;
			const int code = saveOrRestore(3, text(kRestoringText));
			if (code == Shared::Launcher::kNotImplemented)
				return code;
			if (code != 0) {
				_entry = (byte)code;
				_rt.vid(0x14);
				leave(true);
				break;
			}
			_rt.vid(0x14);
			readPartyRecord(0, _party);
			for (int i = 0; i < 6; i++) {                   // redraw the strip of every party member
				const uint16 id = (uint16)(_party[0x26 + 2 * i] | (_party[0x27 + 2 * i] << 8));
				if (id == 0)
					break;
				byte member[100];
				if (readPartyRecord(id, member))
					drawPartyStrip(member);
			}
			_rt.vid(0x1E);
			_rt.vid(0x14);
			if (_params.parm(2) == 0)
				restoreBackground();
			_rt.vid(0x12, 0);
			_rt.vid(0x28);
			break;
		}
		if (_entry == 4) {
			if (!confirm())
				continue;
			leave(false);
			break;
		}
		if (_entry == 5) {
			_c.iconBarState = 0;
			leave(true);
			break;
		}
		_entry = 0xFC;
		leave(false);
		break;
	}
	if (_params.parm(2) != 0)
		_rt.vid(0x2D, 2);
	_rt.vid(0x28);
	return _entry;
}

} // namespace Prog6
} // namespace Yendor1
